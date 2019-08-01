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
/*
 * odb - v1.1.0
 * vim:ru:scs:si:sm:sw=4:sta:ts=4:tw=0
 *
 * Perfection is achieved, not when there is nothing more to add, but when there is nothing left to take away.
 * -- Antoine de Saint Exupéry
*/
char *odbid = "odb version 1.1.0";
char *odbauth = "Trafodion Dev <trafodion-development@lists.launchpad.net>";

#ifndef ODBBLD
    #define ODBBLD "odb Undefined Platform"
#endif
#define CMD_CHUNK   512         /* Granularity of SQL command memory allocation */
#define ETAB_CHUNK  8           /* Granularity of etab[] memory allocation */
#define TD_CHUNK    32          /* Granularity of td[] memory allocation */
#define MAX_ARGS    11          /* Max arguments for interactive mode */
#define ARG_LENGTH  128         /* Max argument length in interactive mode */
#define LINE_CHUNK  51200       /* size of memory chunks allocated to store lines */
#define ERR_MSG_LEN 512         /* size of error message buffer */
#define MAX_VNLEN   32          /* Max variable name length */
#define MAX_PK_COLS 16          /* Max number of PK elements */
#define MAXCOL_LEN  128         /* Max table column name length */
#define MAXOBJ_LEN  128         /* Max catalog/schema/table name length */
#define MAX_CLV 64          /* Max command line variables (-V) */
#define QLABEL_LEN  16          /* Query Label Max Length */
#define WORDSZ  sizeof(unsigned long int)   /* CPU word size for memcmp memory aligned ops */
#define BYTESPWCHAR 4           /* Default Bytes per (Wide) Character */
#define BYTESPCHAR  1           /* Default Bytes per (non-Wide) Character */
#define NUMLOADERS  2           /* Default number of loaders per extractor */
#define EMPTY       (-9999)     /* used for EMPTYASEMPTY */
#define EX_OK       0           /* exit status: successfull termination */
#define EX_USAGE    64          /* exit status: invalid command line, bad or missing parameter */
#define EX_DATAERR  65          /* exit status: input data was incorrect in some way */
#define EX_NOINPUT  66          /* exit status: input does not exists or is unreadable */
#define EX_NOSRVC   67          /* exit status: service is not available */
#define EX_ODBCERR  68          /* exit status: unrecoverable ODBC API error */
#define EX_OSERR    69          /* exit status: OS error (cannot alloc, cannot create thread,... */
#define EX_NOUTPUT  70          /* exit status: cannot create/write output file */
#define EX_SIGNAL   71          /* exit status: timeout or user interrupt */
#define RWBUFF_LEN  262144      /* default read/write buffer length */

#include <zlib.h>
#include "JsonReader.h"
#define windowBits 15
#define GZIP_ENCODING 16
#define ENABLE_ZLIB_GZIP 32
#define GZBUFF_LEN  262144      /* gzip write buffer length */

#ifdef __CYGWIN__
    #undef WORD
    #include <windows.h>
    #include <sql.h>
    #include <sqltypes.h>
    #include <sqlext.h>
    #undef _WIN32
#endif  
#ifdef _WIN32
    #include <windows.h>
    #include <process.h>
    #include <sys/timeb.h>
    #include <direct.h>
    #include <float.h>
    #define isnan(x) _isnan(x)
    #define isinf(x) ((_fpclass(x) == _FPCLASS_PINF) ? 1 : ((_fpclass(x) == _FPCLASS_NINF) ? -1 : 0))
    #if defined(_MSC_VER) || defined(_MSC_EXTENSIONS)
        #define DELTA_EPOCH_IN_MICROSECS 11644473600000000Ui64
    #else
        #define DELTA_EPOCH_IN_MICROSECS 11644473600000000ULL
    #endif
    #define SIZET_SPEC "%Iu"
    #define FindFirstFile FindFirstFileA
    int gettimeofday(struct timeval* tp, void* tzp) {
        FILETIME ft;
        unsigned __int64 tmpres = 0;
                         
        if (tp != NULL) {
            GetSystemTimeAsFileTime(&ft);
            tmpres |= ft.dwHighDateTime;
            tmpres <<= 32;
            tmpres |= ft.dwLowDateTime;

            /* converting file time to unix epoch */
            tmpres /= 10;
            tmpres -= DELTA_EPOCH_IN_MICROSECS; 
            /* convert into microseconds*/
            tp->tv_sec = (long)(tmpres / 1000000UL);
            tp->tv_usec = (long)(tmpres % 1000000UL);
        }
        return (0);
    }
    #undef UNICODE
    #define strcasecmp _stricmp
    #define snprintf sprintf_s
    #define chdir _chdir
    #define getcwd _getcwd
    #define Setenv(x,y) _putenv_s(x,y)
    #define MutexLock(x) EnterCriticalSection(x)
    #define MutexUnlock(x) LeaveCriticalSection(x)
    #define MutexDestroy(x) DeleteCriticalSection(x)
    #define CondWait(x,y) SleepConditionVariableCS(x, y, INFINITE)
    #define CondWake(x) WakeConditionVariable(x)
    #define CondWakeAll(x) WakeAllConditionVariable(x)
    #define CondDestroy(x) ;
    typedef CRITICAL_SECTION Mutex;
    typedef CONDITION_VARIABLE CondVar;
    static DWORD WINAPI Oruncmd(void *tid);
    HANDLE h;               /* used to expand file wildcards [-P, -S] */
    HANDLE *thhn = 0;       /* thread handle pointer - array will contain tn entries */
    CRITICAL_SECTION dlbmutex;  /* dlb mutex */
    WIN32_FIND_DATA finfo;  /* used to expand file wildcards [-P, -S] */
    int st, en;             /* used for round-robin replication [-P, -S] */
#else
    #include <sys/time.h>
    #include <sys/types.h>
    #include <sys/stat.h>
    #include <dirent.h>
    #include <unistd.h>
    #define SIZET_SPEC "%zu"
    #include <pthread.h>
    #include <strings.h>
    #ifdef __hpux
        #define Sleep(x) \
            if ( x > 1000 ) { \
                (void) sleep ( x / 1000 ) ; \
                (void) usleep ( (useconds_t)(1000 * x%1000) ); \
            } else { \
                (void) usleep ( (useconds_t)(1000 * x) ); \
            }
        #include <sys/pstat.h>
        #ifndef ODBVER
            #define ODBVER "odb"
        #endif
        union pstun pst;
    #else
        #define Sleep(x) (void) usleep ( (useconds_t)(1000 * x) )   /* sleep x ms */
    #endif
    #include <termios.h>
    #include <math.h>
    static void *Oruncmd(void *tid);
    pthread_t *thid = 0;        /* thread id pointer - array will contain tn entries */
    int thres = 0;              /* thread result */
    pthread_mutex_t dlbmutex = PTHREAD_MUTEX_INITIALIZER;   /* dlb mutex */
    #define MutexLock(x) pthread_mutex_lock(x)
    #define MutexUnlock(x) pthread_mutex_unlock(x)
    #define MutexDestroy(x) pthread_mutex_destroy(x)
    #define CondWait(x,y) pthread_cond_wait(x, y)
    #define CondWake(x) pthread_cond_signal(x)
    #define CondWakeAll(x) pthread_cond_broadcast(x)
    #define CondDestroy(x) pthread_cond_destroy(x)
    #define Setenv(x,y) setenv(x,y,1)
    typedef pthread_mutex_t Mutex;
    typedef pthread_cond_t CondVar;
    extern int errno;
    DIR *dp;
    struct dirent *de;
    struct stat flinfo;         /* used to check if direntry are regular files */
#endif
#ifdef HDFS
    #include <dlfcn.h>          /* dlopen() include file */
    #include <hdfs.h>           /* libhdfs include file */
    hdfsFS hfs = 0;             /* Hadoop FS to connect to */
    void *hdfs_handle = 0;      /* dlopen() handle for libhdfs.so */
    hdfsFS (*hdfsconn) ();      /* pointer to hdfsConnectAsUser() */
    hdfsFile (*hdfsopen) ();    /* pointer to hdfsOpenFile() */
    int (*hdfsclose) ();        /* pointer to hdfsCloseFile() */
    int (*hdfsseek) ();         /* pointer to hdfsSeek() */
    tSize (*hdfswrite) ();      /* pointer to hdfsWrite() */
    tSize (*hdfsread) ();       /* pointer to hdfsRead() */
#endif
#ifdef XML
    #ifndef HDFS
        #include <dlfcn.h>          /* dlopen() include file */
    #endif
    #include <libxml/xmlreader.h>
    void *xml_handle = 0;           /* dlopen() handle for libxml2.so */
    xmlTextReaderPtr (*xmlfile) (); /* pointer to xmlNewTextReaderFilename() */
    int (*xmlread) ();              /* pointer to xmlTextReaderRead() */
    int (*xmltype) ();              /* pointer to xmlTextReaderNodeType() */
    int (*xmldepth) ();             /* pointer to xmlTextReaderDepth() */
    xmlChar *(*xmllname) ();        /* pointer to xmlTextReaderLocalName() */
    int (*xmlnextattr) ();          /* pointer to xmlTextReaderMoveToNextAttribute() */
    xmlChar *(*xmlvalue) ();        /* pointer to xmlTextReaderValue() */
    int (*xmlfree) ();              /* pointer to xmlFreeTextReader() */
    #if defined __CYGWIN__
        #define XML2LIBNAME "cygxml2-2.dll"
    #elif defined __APPLE__ && defined __MACH__
        #define XML2LIBNAME "libxml2.dylib"
    #else
        #define XML2LIBNAME "libxml2.so"
    #endif
#endif
#define RAND(min,max) (min+rand()/(RAND_MAX+1.0)*(max-min+1))
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sql.h>
#include <sqlext.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <ctype.h>
#if defined __APPLE__ && defined __MACH__
    #include <sys/malloc.h>
#else
    #include <malloc.h>
#endif

#define SQL_ATTR_ROWCOUNT64_PTR     5001    /* Attribute to get the 64bit rowcount when using the 32-bit ODBC driver */
#define SQL_ATTR_FETCHAHEAD         5003    /* Attribute to set fetchahead connection attribute */
#define SQL_ATTR_APPLNAME           5100    /* wms_mapping */
#define SQL_ATTR_CERTIFICATE_DIR            5200    /*Security */
#define SQL_ATTR_CERTIFICATE_FILE           5201
#define SQL_ATTR_CERTIFICATE_FILE_ACTIVE    5202

#define VTYPE_A     1   /* 0001 Variable Type = Alias */
#define VTYPE_U     2   /* 0010 Variable Type = User */
#define VTYPE_I     4   /* 0100 Variable Type = Internal */
#define VTYPE_UI    6   /* 0110 Variable Type = either User or Intenal */

#define MEMCPY(t,s,l) \
    do { \
        switch ( l ) {\
        case 0:\
            break;\
        case 1:\
            *(t) = *(s);\
            break;\
        case 2:\
            *(t) = *(s);\
            *((t)+1) = *((s)+1);\
            break;\
        case 3:\
            *(t) = *(s);\
            *((t)+1) = *((s)+1);\
            *((t)+2) = *((s)+2);\
            break;\
        case 4:\
            *(t) = *(s);\
            *((t)+1) = *((s)+1);\
            *((t)+2) = *((s)+2);\
            *((t)+3) = *((s)+3);\
            break;\
        case 5:\
            *(t) = *(s);\
            *((t)+1) = *((s)+1);\
            *((t)+2) = *((s)+2);\
            *((t)+3) = *((s)+3);\
            *((t)+4) = *((s)+4);\
            break;\
        case 6:\
            *(t) = *(s);\
            *((t)+1) = *((s)+1);\
            *((t)+2) = *((s)+2);\
            *((t)+3) = *((s)+3);\
            *((t)+4) = *((s)+4);\
            *((t)+5) = *((s)+5);\
            break;\
        case 7:\
            *(t) = *(s);\
            *((t)+1) = *((s)+1);\
            *((t)+2) = *((s)+2);\
            *((t)+3) = *((s)+3);\
            *((t)+4) = *((s)+4);\
            *((t)+5) = *((s)+5);\
            *((t)+6) = *((s)+6);\
            break;\
        default:\
            (void)memcpy( (void *)(t) , (void *)(s) , (size_t)(l) ) ;\
            break;\
        }\
    } while ( 0 ) 

/* Struct and variables declaration */
struct info {                   /* ODBC/DBMS infor optional printed [-i] */
    char *desc;                 /* Info description */
    SQLUSMALLINT Oid;           /* Info ID */
} Oinfo [] = {                  /* DBMS printed with -i option */
    {"DBMS product name (SQL_DBMS_NAME)",SQL_DBMS_NAME},
    {"DBMS product version (SQL_DBMS_VER)",SQL_DBMS_VER},
    {"Database name (SQL_DATABASE_NAME)",SQL_DATABASE_NAME},
    {"Server name (SQL_SERVER_NAME)",SQL_SERVER_NAME},
    {"Data source name (SQL_DATA_SOURCE_NAME)",SQL_DATA_SOURCE_NAME},
    {"Data source RO (SQL_DATA_SOURCE_READ_ONLY)",SQL_DATA_SOURCE_READ_ONLY},
    {"ODBC Driver name (SQL_DRIVER_NAME)",SQL_DRIVER_NAME},
    {"ODBC Driver version (SQL_DRIVER_VER)",SQL_DRIVER_VER},
    {"ODBC Driver level (SQL_DRIVER_ODBC_VER)",SQL_DRIVER_ODBC_VER},
    {"ODBC Driver Manager version (SQL_DM_VER)",SQL_DM_VER},
    {"ODBC Driver Manager level (SQL_ODBC_VER)",SQL_ODBC_VER},
};
struct Otype {                  /* Database Object types */
    char type;                  /* code */
    SQLCHAR *Otype;             /* extended name */
} Otypes[] = {
    {'a', (SQLCHAR *)"ALIAS"},              /* alias */
    {'t', (SQLCHAR *)"TABLE"},              /* table */
    {'v', (SQLCHAR *)"VIEW"},               /* view */
    {'y', (SQLCHAR *)"SYNONYM"},            /* synonym */
    {'e', (SQLCHAR *)"SYSTEM TABLE"},       /* system table */
    {'g', (SQLCHAR *)"GLOBAL TEMPORARY"},   /* global temporary */
    {'l', (SQLCHAR *)"LOCAL TEMPORARY"},    /* local temporary */
    {'m', (SQLCHAR *)"MV"},                 /* materialized views */
    {'M', (SQLCHAR *)"MVGROUPS"},           /* materialized view groups */
    {'T', (SQLCHAR *)"TABLE"},              /* Olist print table desc */
    {'D', (SQLCHAR *)"TABLE"},              /* Olist print table DDL */
    {'U', (SQLCHAR *)"TABLE"},              /* Olist print table DDL multiplying CHAR/VARCHAR field by 4 */
    {'A', (SQLCHAR *)""},                   /* Olist uses SQL_ALL_TABLE_TYPES */
    {'s', (SQLCHAR *)""},                   /* Olist list schemas */
    {'c', (SQLCHAR *)""},                   /* Olist list catalogs */
};
struct rm {
    char *cmd;                  /* initial command string */
    char *mex;                  /* xx row(s) <mex> */
} rmess [] = {
    {"other", "other"},
    {"SELECT ", "selected"},
    {"UPDATE ", "updated"},
    {"DELETE ", "deleted"},
    {"INSERT ", "inserted"}
};
char *olbmex[] = { "inserted", "copied", "piped" } ;

struct dbscmd {             /* Database specific data */
    char *dbname;           /* database name */
    char *trunc;            /* fast delete template */
    char *uncomm;           /* uncommitted read syntax */
} dbscmds[] = {
    { "Generic", "DELETE FROM &tgt", "" } ,                                /* dbt = 0 (GENERIC) */
    { "PostgreSQL", "TRUNCATE &tgt", "" } ,                                /* dbt = 1 (POSTGRESQL) */
    { "Vertica Database", "TRUNCATE TABLE &tgt", "" } ,                /* dbt = 2 (VERTICA) */
    { "Microsoft SQL Server", "TRUNCATE TABLE &tgt", "WITH (NOLOCK)" } ,       /* dbt = 3 (SQLSERVER) */
    { "Teradata", "DELETE FROM &tgt", "" } ,                   /* dbt = 4 (TERADATA) */
    { "NonStop SQL/MX", "PURGEDATA &tgt", "FOR READ UNCOMMITTED ACCESS" } ,    /* dbt = 5 (SQLMX) */
    { "Oracle", "TRUNCATE TABLE &tgt", "" } ,                  /* dbt = 6 (ORACLE) */
    { "MySQL", "TRUNCATE TABLE &tgt", "" } ,                   /* dbt = 7 (MYSQL) */
    { "Trafodion", "PURGEDATA &tgt", "FOR READ UNCOMMITTED ACCESS" }       /* dbt = 8 (TRAFODION) */
};
enum DBs { 
    GENERIC = 0,
    POSTGRESQL,
    VERTICA,
    SQLSERVER,
    TERADATA,
    SQLMX,
    ORACLE,
    MYSQL,
    TRAFODION
};

struct tdesc {              /* ODBC Table description for insert ops */
    SQLSMALLINT Otype;      /* ODBC Data Type */
    SQLSMALLINT Octype;     /* ODBC C data type */
    SQLSMALLINT Odec;       /* ODBC Decimal digits */
    SQLSMALLINT Onull;      /* ODBC Column nullability */
    SQLSMALLINT OnameLen;   /* ODBC column name length */
    SQLCHAR *Oname;         /* ODBC Column name */
    SQLULEN Osize;          /* ODBC Column Size */
    SQLLEN OdisplaySize;    /* ODBC column display size */
    SQLLEN Ocdatabufl;      /* ODBC C type buffer Size */
    size_t dl;              /* Default string length */
    size_t start;           /* Start address in rowset */
    size_t pad;             /* Used to memory align ODBC buffer elements on HP-UX */
};
struct execute {
    char type;                  /* Possible types:
        x = command specified through -x                      e=extracta(extract to file)   D=diff (extract from tgt)
        f = file specified through -f/-S/-P                   c=copy (extract from source)  Z=diff (compares)
        F = file specified with -S/-P under Win (free memory) C=copy (load to target)       s=diff (sync writer - future use)
        h = runsql file execution                             i=info                        p=pipe (extract from source)
        l = loader                                            I=interpreter                 P=pipe (load to target)
        L = load buffer helper threads                        d=diff(extract from source) */
    char *run;                  /* command to run or filename or query id, diff=output file */
    char *pre;                  /* SQL script to be executed before load/copy/extract */
    char *post;                 /* SQL script to be executed after load/copy/extract */
    char *mpre;                 /* SQL script to be executed by each thread before extract */
    char *cols;                 /* extract/copy: src table column list or original custom sql file name */
    char *tgtsql;               /* copy: target SQL to be executed instead of INSERT */
    unsigned int *tgtp;         /* copy: target SQL parameter position */
    unsigned int dbt;           /* Source/Target database type (see dbscmds[] structure */
    int id;                     /* thread ID */
    char tbe;                   /* to be executed flag */
    unsigned char fs;           /* field separator */
    unsigned char rs;           /* record separator */
    unsigned char sq;           /* string qualifier extract=type: 0+exit first diff, 1/2=print rows only in src/tgt */
    unsigned char ec;           /* escape character */
    unsigned char pc;           /* pad character , diff=number of key columns, copy/extract */
    unsigned char em;           /* embed file character, diff: key memory buffer to be freed */
    size_t fsl;                 /* field separator length */
    size_t r;                   /* initial rowset, 'Z' thread: key section length */
    size_t ar;                  /* "actual" rowset (could be < r at the end) */
    size_t rbs;                 /* rowset buffer size, Z thread: "actual" rowset for tgt threads */
    size_t s;                   /* rowbuffer length (includes data & indicator) */
    size_t sbl;                 /* allocated splitby buffer length (if !=0 grandfather should free etab[].sb) */
    size_t iobuff;              /* low level io buffer size. */
    size_t buffsz;              /* load: fread buffer size */
    unsigned long mr;           /* max number of records to insert/fetch; 0 = unlimited. Extract returns here tinit. Zthread:#records 'I" */
    unsigned long TotalMaxRecords;  /* total number of records */
    int mer;                    /* max number of errors */
    int roe;                    /* restart on error */
    unsigned int roedel;        /* delay before restarting on error */
    unsigned long nr;           /* number of records loaded/extracted by thread, Zthread: #records compared, general=no of output rows */
    unsigned long nrt;          /* total number of records inserted by "pool": Zthread: #records 'D' */
    unsigned long nbs;          /* total "base" number of records inserted. Extract returns here no. of extr bytes. Zthread: #records 'C' */
    unsigned long nt;           /* number of total load cycles */
    unsigned long nw;           /* number of wait load cycles */
    unsigned int bpwc;          /* copy: bytes per wide character (default=4) */
    unsigned int bpc;           /* copy: bytes per non-wide character (default=1) */
    uint64_t seq;               /* copy: sequence */
    unsigned int seqp;          /* copy: sequence position (tgt table field number (1=first field) */
    unsigned int flg;           /* etab bit mask flag (0=false 1=true):
        0001 = FOR READ UNCOMMITTED ACCESS      010000 = quiet (no commands)      0100000000 = Oexec silent mode
        0002 = truncate destination table       020000 = quiet (no results)       0200000000 = diff: tgt EOD
        0004 = Stop On Error                    040000 = print line mode          0400000000 = trim CHAR fields
        0010 = Null Run                         100000 = Interpreter Connected    1000000000 = cast non text fields
        0020 = GZ Output (extract)              200000 = Save History & ifempty   2000000000 = diff: print 'I'
        0040 = trunc decimal TIME/TS            400000 = desc result set          4000000000 = diff: print 'D'
        0100 = binary data extraction          1000000 = complex load (Oload)    10000000000 = diff: print 'C'
        0200 = print column names              2000000 = rtrim CHAR fields       20000000000 = diff: sync (future use)
        0400 = print mark                      4000000 = rtrim VARCHAR fields
        1000 = multi-parted output            10000000 = ucs2toutf8 conversion
        2000 = loader:EOF/copy-diff:src EOD   20000000 = NOT USED           
        4000 = loadbuff error                 40000000 = Vertica's direct hint */
    unsigned int flg2;          /* etab bit mask flag (0=false 1=true):
        0001 = Commit as a rowset multiplier    010000 = NOT USED                 0100000000 = NOT USED
        0002 = WITH NO ROLLBACK                 020000 = print timeline           0200000000 = xml dump
        0004 = quiet (no timing)                040000 = custom SQL extract/copy  0400000000 = user defined iobuff
        0010 = diff "quick" mode                100000 = sql from file. free .sql 1000000000 = use libMapRClient.so instead of libhdfs
        0020 = load "show" mode                 200000 = force cdef binding       2000000000 = NOT USED
        0040 = copy: cols to be excluded        400000 = NOT USED                 4000000000 = NOT USED
        0100 = src or tgt HDFS                 1000000 = diff:odad (Or dt As dt) 10000000000 = NOT USED
        0200 = flag used to print header line  2000000 = xml attr/text set/unset 20000000000 = NOT USED
        0400 = diff gpar tgt checkdb/pre done  4000000 = xml export
        1000 = tgt is set SQL_PARC_NO_BATCH   10000000 = xml fast (do not check element names)
        2000 = diff pre SQL errors            20000000 = Oexec to create its own statement handle
        4000 = copy: force SQL_C_CHAR bind    40000000 = trim VARCHAR cols other than CHAR */
    int parent;                 /* Used to "group" executions */
    int child;                  /* used by copy extractors to identify their loaders */
    int cmt;                    /* commit: -1=end, 0=auto, >0 num rows, extract: fields in source table if analyzed */
    int gzlev;                  /* extract: gzip compression level */
    unsigned char lstat;        /* load statuses (decimal):
                                        0=write_avail
                                        1=read_avail
                                        2=write_busy
                                        3=read_busy
                                        4=pre_SQL_busy
                                        5=pre-SQL-Error
                                        10=write_job_completed
                                   extract statuses (bit mask - octal):
                                        0001=XML output to initialize
                                        0002=pre-extract errors.
                                        0004=pre-SQL to run.
                                        0010=XML init errors
                                   copy statuses (decimal):
                                        0=write_avail
                                        1=read_avail
                                        2=write_busy
                                        3=read_busy
                                        4=pre_SQL_busy
                                        5=pre-SQL-Error
                                   diff statuses (octal bit mask):
                                        0001 = src_write_avail
                                        0002 = tgt_write_avail
                                        0004 = src_data_ready
                                        0010 = tgt_data_ready
                                        0020 = src_write_busy 
                                        0040 = tgt_write_busy 
                                        0100 = src_read_busy
                                        0200 = tgt_read_busy */
    FILE *fso;                  /* Spool file pointer */
    FILE *fo;                   /* Output file pointer (default stdout) */
    FILE *fdmp;                 /* Copy: error dump file pointer */
#ifdef HDFS
    hdfsFile fho;               /* Hadoop File Handle */
    char *hdfshost;             /* hdfs host */
    tPort hdfsport;             /* hdfs port number */
    char *hdfsuser;             /* hdfs user */
    tSize hblock;               /* HDFS block size */
#endif
    Mutex gmutex;               /* Group mutex to synch pre-extract/copy/diff activities */
    Mutex pmutex;               /* Parallel load/extract/copy mutex */
    CondVar pcvp;               /* Parallel load/extract/copy condition variable - Producer */
    CondVar pcvc;               /* Parallel load/extract/copy condition variable - Consumer */
    char *src;                  /* load/extract source */
    char *tgt;                  /* load/extract target */
    char *map;                  /* load=map file, extract/copy/diff=pwhere */
    char *ns;                   /* load/extract Null string */
    char *es;                   /* extract=emptystring, copy: tgtsql with positional parameters */
    char *sql;                  /* extract custom SQL */
    char *sb;                   /* extract split by column name */
    char *bad;                  /* bad file */
    char *key[MAX_PK_COLS];     /* diff key column names */
    char loadcmd[3];                /* load/copy=command to use INSERT/UPSERT/UPSERT USING LOAD, IN/UP/UL */ 
#ifdef XML
    char *xrt;                  /* load: xml row tag */
#endif
    char *jsonKey;              /* load: json key */
    unsigned int nl;            /* Null string length. */
    unsigned int nloader;       /* Copy: number of loaders per extraction thread */
    unsigned int el;            /* Empty String Length. */
    unsigned int nltotal;       /* Copy: total number of loaders per grandparent (ps * nl) */
    unsigned int k;             /* load lines to skip. extract: returned rsds. Copy: grandparent ID */
    unsigned int sp;            /* extract Start Partition load/copy: number of target table fields */
    unsigned int ep;            /* extract End Partition, copy: number of tgtsql parameters */
    unsigned int ps;            /* load/extract number of (still active) parallel streams, workload: number of agents */
    unsigned int cr;            /* Number of rows inserted since last commit. diff=1 first D-thread, =2 last D-thread */
    unsigned int bucs2;         /* bad UCS-2 character management: 0=skip, 1=force, 2=cpucs2 3=qmark */
    long sbmin;                 /* Splitby min value */
    long sbmax;                 /* Splitby max value */
    char *gzp;                  /* extract gzip parameter */
    SQLCHAR *Ocso[3];           /* load/extract ODBC array of catalog/schema/object pointers */
    SQLCHAR *Orowsetl;          /* ODBC pointer to the dynamically allocated rowset */
    SQLCHAR *Orowsetl2;         /* ODBC pointer to the dynamically allocated rowset used by 'Z' type threads (diff) */
    SQLUSMALLINT *Ostatusl;     /* ODBC pointer to the rowset status array */
    SQLULEN Oresl;              /* ODBC ulen to store SQLGetInfo results */
    SQLULEN Omaxl;              /* ODBC max char/varchar/binary column length to be loaded/retrieved */
    struct tdesc *td;           /* load target table definition structure */
    char fldtr;                 /* Field truncation managenet during loads
                                    0   truncates, warn and "load if text" (default)
                                    1   truncates and "load if text"
                                    2   warn and skip record
                                    3   truncates, warn and load
                                    4   truncates and load
                                    5   do not check for field truncation
                                */
    char *mcfs;                 /* multi character field separator */
    char *mcrs;                 /* multi character record separator */
} *etab = 0;                    /* Execution TABle: one entry per command or script */
struct ovar {
    char type;                  /* Variable type: 'a'=alias, 'i'=interal, 'u'=user defined */
    char name[MAX_VNLEN];       /* Variable name */
    char *value;                /* Variable value */
    size_t nlen;                /* Variable name length*/
    size_t vlen;                /* Variable value length*/
    struct ovar *next;          /* Pointer to the next element */
    struct ovar *prev;          /* Pointer to the previous element */
} **vars;                       /* pointer to array of "struct var" pointers */
struct thp {                    /* per thread properties */
    int cr;                     /* Connection Redirection: >0 use this tid ODBC conn; -1 no connection */
    int tid;                    /* Thread id */
    SQLHDBC Oc;                 /* ODBC Connection handle pointer */
    SQLHSTMT Os;                /* ODBC Statement handle pointer */
    struct ovar *tva;           /* Chain of variables */
    unsigned int cd;            /* delay before starting the next command */
    unsigned int cd2;           /* (max) delay before starting the next command */
} *thps;
struct timeval tvi,             /* init timeval struct */
               tvs,             /* start timeval struct */
               tvn;             /* now timeval struct */
SQLCHAR Odsn[64], Odsn2[64],    /* ODBC data source name string */
        Ouser[64], Ouser2[64],  /* ODBC user name string */
        Opwd[64], Opwd2[64],    /* ODBC password string */
        Oics[1024],             /* ODBC SQLDriverConnect() Input Connection String */
        Oocs[1024],             /* ODBC SQLDriverConnect() Output Connection String */
        Oaot[256],              /* ODBC All Object Types list */
        Obuf[3][1024];          /* ODBC buffer(s) */
SQLUINTEGER Onps=0, Onps2=0;    /* ODBC Network Packet Size */
SQLHENV Oenv;                   /* ODBC environment handle */
SQLSMALLINT Osio[3];            /* ODBC smallint output */
SQLLEN Olen[7];                 /* ODBC returned buff length */
SQLINTEGER Odps=0;              /* ODBC display size for the given result column */
SQLRETURN Oret=0;               /* ODBC return value */
SQLCHAR *Orowset = 0;           /* ODBC pointer to the dynamically allocated rowset */
SQLHDBC Oc=0;                   /* ODBC Connection handle used by etabadd */
unsigned int f=000000000;       /* global/command line flag mask:
    Bit#     Octal  Meaning                                     Bit#       Octal  Meaning
       0         1  Data Source (-d) in cmd line                  16      200000  Stop On Error (-soe / set soe)
       1         2  User Name (-u) in cmd line                    17      400000  Quiet Mode RES (-q res / set quiet res)
       2         4  Password (-p) in cmd line                     18     1000000  start time in CSV header (-b)
       3        10  List Drivers (-lsdrvr)                        19     2000000  Shuffle etab before execute (-Z)
       4        20  List Data Sources (-lsdsn)                    20     4000000  SQL_TXN_READ_UNCOMMITTED isolation level (-U)
       5        40  Quiet Mode CMD (-q cmd / set quiet cmd)       21    10000000  Serial Run
       6       100  Print Info (-i)                               22    20000000  No Schema (-noschema / set noschema)
       7       200  Exeute scripts/commands (-f/-x/-S/-P/-l/-e)   23    40000000  Print Line Mode (global)
       8       400  Produce CSV output (-c)                       24   100000000  No Catalog (-nocatalog /set nocatalog)
       9      1000  Print Execution Table (-vv)                   25   200000000  Print Column Names in command line
      10      2000  Interpreter Mode (-I) in cmd line             26   400000000  UCS-2 to UTF-8 conversion in odb
      11      4000  binary mode (copied to etab[eid].flg 0100)    27  1000000000  Dynamic Load Balancing
      12     10000  Null Run (-N)                                 28  2000000000  Rolling DSN sequential (instead of RR)
      13     20000  Verbose (-v)                                  29  4000000000  No Catalog as NULL
      14     40000  do not connect (SQL Interpreter) startup      30 10000000000  Interpreter: case sensitive DB (set casesens)
      15    100000  Desc result set (-drs)                        31 20000000000  Quiet Mode TIMING (-q timing / set quiet timing) */
unsigned int f2 = 0000;         /* second global f;ag mask:
    Bit#     Octal  Meaning                                     Bit#       Octal  Meaning
       0         1  C-style comments as hint (not stripped)       16      200000  NOT USED
       1         2  Global sequence during copy operations        17      400000  NOT USED
       2         4  NOT USED                                      18     1000000  NOT USED
       3        10  NOT USED                                      19     2000000  NOT USED
       4        20  NOT USED                                      20     4000000  NOT USED
       5        40  NOT USED                                      21    10000000  NOT USED
       6       100  NOT USED                                      22    20000000  NOT USED
       7       200  NOT USED                                      23    40000000  NOT USED
       8       400  NOT USED                                      24   100000000  NOT USED
       9      1000  NOT USED                                      25   200000000  NOT USED
      10      2000  NOT USED                                      26   400000000  NOT USED
      11      4000  NOT USED                                      27  1000000000  NOT USED
      12     10000  NOT USED                                      28  2000000000  NOT USED
      13     20000  NOT USED                                      29  4000000000  NOT USED
      14     40000  NOT USED                                      30 10000000000  NOT USED
      15    100000  NOT USED                                      31 20000000000  NOT USED */

unsigned int go = 1;            /* General Flag: all threads to exit ASAP as soon as go = 0 */
unsigned int nloop=1;           /* number of loops for command execution */
unsigned int ld=0;              /* loop delay */
unsigned int ndsn=0;            /* number of "rolling DSNs" */
unsigned int mf=0;              /* Max number of rows to be fetched */
unsigned int mult = 4 ;         /* char/varchar multiplier for -i U */
unsigned int wmult = 4 ;        /* wide char/varchar multiplier for -i U */
uint64_t gseq = 0 ;             /* global sequence value */
int exstat = EX_OK ;            /* global exit status variable */
int cd=0,                       /* delay before executing the next command within a thread */
    cd2=0,                      /* (max) delay before executing the same command within a thread */
    nae=0,                      /* Number of Allocated etab[] chunks */
    nc=1,                       /* number of output columns */
    no=0,                       /* number of commands/scripts to run (entries in etab[]) */
    cs=80,                      /* column size when multi-col output is used */
    hist=200,                   /* Default History Size */
    ssl=0,                      /* Set Schema Command length (before CAT.SCH) */
    tn=0;                       /* number of threads */
unsigned char pad=0,            /* Pad columns to their display size: 0=off, 1=full, 2=fit, 3=temporary full */
              sq=0,             /* String Qualifier */
              fs=',',           /* Field Separator */
              ksep = ',',       /* Default Thousand Separator */
              dsep = '.',       /* Default Decimal Separator */
              rs='\n',          /* Record Separator */
              ec='\\';          /* Escape Character */
char *ns=0;                     /* Command line provided Null String */
extern unsigned int mrcol;      /* screen columns defined in mreadline */
unsigned int w=0;               /* general timeout in seconds */
size_t r=100;                   /* rowset for file loading */
SQLULEN Ores,                   /* ODBC ulen to store SQLGetInfo results */
        Oqt = 0;                /* ODBC Query Timeout (def = 0 unlimited) */
SQLSMALLINT Oocsl;              /* ODBC output connection string length */
unsigned char clvn[MAX_CLV],    /* command line arg numbers containing variable names */
              clvv[MAX_CLV],    /* command line arg numbers containing variable values */
              clv = 0;          /* Command line variables number */
char prompt[128],               /* prompt string */
     tprompt[128],              /* prompt template */
     *odbcmd=0,                 /* odb command (used to save av[0] */
     *clca=0,                   /* command line connection attributes pointer */
     *esrc=0,                   /* pointer to buffer containing src input file */
     *tmpre=0;                  /* tmp pointer to buffer containing target mpre script name */
FILE *fp=0;                     /* File pointer */
FILE *fdmp=0;                   /* Dump file pointer */
struct tm *nowt;                /* struct to print start date/time */
char nows[20],                  /* string with date/time in YYYY-MM-DD HH:MM:SS */
     sfile[128],                /* spool file name */
     hfile[128],                /* history file name */
     cwd[64],                   /* current working directory */
     chsch[64];                 /* Set Schema command */
struct ovar *vv=0;              /* struct var pointer for the Interpreter */
const char alnum[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz" ; /* look-up tab to generate random strings */
void **globalPointers = NULL;       /* save pointers to buffers which may shared by many thread and length determined at run time */
int nGlobalPointers = 0;            /* number of pointers in globalPointers */
int naGlobalPointers = 0;           /* number of globalPointers allocated chunk */

/* Functions prototypes */
static void Oerr(int eid, int tid, unsigned int line, SQLHANDLE Ohandle, SQLSMALLINT Otype);
static void Otinfo(SQLCHAR *Oca, SQLCHAR *Osc, SQLCHAR *Ota, char ddl);
static void Olist(SQLCHAR *Oca, SQLCHAR *Osc, SQLCHAR *Ota, char otype);
static void Odinfo();
static int Oexec(int tid, int eid, int ten, int scn, SQLCHAR *Ocmd, char *label); /* Execute a command */
static void usagexit();                 /* Print usage info and exit */
static void sigcatch(int sig);          /* signal catch routine */
static void cancel(int sig);            /* executed in interactive mode when ^C */
static void etabadd(char type, char *run, int id);  /* add entry(ies) to etab[] */
static void etabnew(int ix);
static void etabshuffle();              /* randomize etab[] entries */
static int etabout ( int eid , char *file );
static void Oload(int eid);             /* load tables using etab entries #eid */
static void Oload2(int eid);            /* fast load tables using etab entries #eid (only CSV with no map, no string qualifiers no escapes */
static void OloadX(int eid);            /* load tables using etab entries #eid (only from XML) */
static void OloadJson(int eid);         /* load tables using etab entries #eid (only from JSON) */
static void Oextract(int eid);          /* extract tables using etab entries #eid */
static void strmcpy(char *tgt, char *src, size_t len);
static unsigned int strmcat(char *tgt, char *src, size_t len, char cas);
static unsigned int strmicmp(char *tgt, char *src, size_t len);
static char *strmprint(char *s);
static char *strmnum(double d, char *s, size_t l, unsigned int dd);
static char *strmtime(double secs, char *str);
static void splitcso(char *s, SQLCHAR **p, int a);
static int var_set(struct ovar **chain, char type, char *name, char *value);
static struct ovar *var_idx(struct ovar **chain, char type, char *name);
static int var_del(struct ovar **chain, char type, char *name);
static char *var_exp(char *str, size_t *lp, struct ovar **chain);
static unsigned int dconv ( char *fmt, char *str, char *out, size_t len, char t );
static int tokenize(char *s, int l, char *ss[]);
static void bcs(SQLCHAR *Oics, size_t csl, SQLCHAR *Oser, SQLCHAR *Opwd, SQLCHAR *Odsn, char *ca, int ctn);
static int mfprintf( FILE *fl1, FILE *fl2, char *fmt, ... );
static int mfputc( FILE *fl1, FILE *fl2, int ch );
static unsigned int checkdb(int eid, SQLHDBC *Oc, char *c, char *s);
static unsigned char mchar ( char *s );
static char* replace_escapes( char *s );
static void tclean(void *tid);          /* Thread cleanup routine */
#ifndef _WIN32
static void tunlock(void *eid);         /* Thread cleanup routine */
#endif
static void gclean(void);               /* Global cleanup routine */
static int Oloadbuff(int eid);
static int Ocopy(int eid);
static void Odiff(int eid);
static void Ocompare(int eid);
static void cimutex(Mutex *mutexp);
static void cicondvar(CondVar *condvarp);
static void setan ( int eid, int tid, int nrag, char *rag[], char *ql );
static int Omexec(int tid, int eid, int ten, int scn, SQLCHAR *Ocmd, char *label, char *dcat, char *dsch);
static char *strup ( char *s );
static char *strlo ( char *s );
static char *strtrim(char *str);
int mrinit ( void );
void mrend ( void );
char *mreadline ( char *prompt, unsigned int *length );
int mrhadd(char *hrec, size_t hl);
int mrhinit(size_t hsize, char *hfile);
int mrhsave(char *hfile);
char *mresize(size_t size);
static char *expandtype( SQLSMALLINT Odt );
static void prec(char type, unsigned char *podbc, int eid);
static int runsql(int tid, int eid, int ten, char *script);
static unsigned int ifempty(int eid, char *table);
static int comp2asc(char *srcfld, char *tgtfld,unsigned int srclen, unsigned int maxlen);
static int comp32asc(char * srcfld, char *tgtfld, unsigned int srclen, unsigned int maxlen, int precision, int scale);
static int zoned2asc(char * srcfld, char *tgtfld, unsigned int srclen, unsigned int maxlen, int precision, int scale);
static int Otcol(int eid, SQLHDBC *Oc);
static unsigned int parseopt(char type, char *str, unsigned int len, unsigned int rp);
static void excol ( int eid, SQLHDBC *Ocn );
static void dumpbuff ( FILE *fd, int eid, unsigned char *buff, size_t buffl, unsigned int v );
#ifdef HDFS
static char *parsehdfs(int eid, char *hdfs);
#endif
#ifdef ODB_PROFILE
unsigned long tspdiff ( struct timespec *start, struct timespec *end ) ;
#endif
static int ucs2toutf8 ( SQLCHAR *str, SQLLEN *len, SQLULEN bs, int bu2 ) ;
static void addGlobalPointer(void *ptr);
static int is_valid_numeric(const char* str, size_t n);

int main(int ac, char *av[])
{
    char *c=0,                  /* loop variable for command line parsing */
         *line=0,               /* pointer to input line - interactive mode */
         *buff=0,               /* pointer to the buffer to edit */
         otype='d',             /* object type:
                                     'd':   database info 
                                     'c':   catalogs info
                                     's':   schema info
                                     'A':   all object types
                                     't':   table
                                     'v':   view
                                     'a':   alias
                                     'y':   synonym
                                     'm':   materialized view
                                     'e':   system table
                                     'g':   global temporary
                                     'l':   local temporary
                                     'M':   materialized view group
                                     'T':   desc tables
                                     'D':   desc tables (ddl output)
                                     'U':   desc tables (ddl output char/varchar x multiplier) */
         *rag[MAX_ARGS],        /* arguments in a row */
#ifdef __hpux
         title[16],             /* used by HP-UX pstat to rewrite ps command name */
#endif
         odbcl[LINE_CHUNK],     /* odb command line to rerun odb from the interpreter */
         num[32],               /* formatted numbers */
         csn[MAXOBJ_LEN],       /* current schema name */
         ccn[MAXOBJ_LEN];       /* current catalog name */
    int ch,                     /* char to read data file */
        d=0,                    /* delay starting threads */
        nrag=0,                 /* Number of rag[] arguments */
        n=0,                    /* loop variable */
        o=0,                    /* loop variable */
        i=0,                    /* loop variable */
        l=0,                    /* loop variable */
        k=0,                    /* loop variable */
        j=0;                    /* loop variable */
    size_t rb = 0,              /* rowset buffer */
           rl = 0,              /* to save rag[0] length */
           bs = CMD_CHUNK;      /* Ocmd buffer size */
    unsigned int ll = 0,        /* line length */
                 ol = 0;        /* odb command line length */
    time_t ptime;               /* to register prompt time */
    SQLUSMALLINT Odir;          /* ODBC direction */
    SQLCHAR *Ocso[3];           /* ODBC array of catalog/schema/object pointers */
    SQLCHAR *Ocmd;              /* ODBC command buffer */
    SQLCHAR *O;                 /* ODBC Ctemp variable for memory realloc */
#ifdef _WIN32
    HANDLE sts;
    DWORD mode;
#else
    struct termios sts,         /* standard termios structure */
                   pts;         /* password entry termios structure (no echo) */
#endif
    char tim[15];               /* Formatted Time String */
    double seconds = 0;         /* seconds used for timings */

    /* cleaning buffers */
    memset(Odsn, 0, sizeof(Odsn));
    memset(Ouser, 0, sizeof(Ouser));
    memset(Opwd, 0, sizeof(Opwd));
    memset(Odsn2, 0, sizeof(Odsn2));
    memset(Ouser2, 0, sizeof(Ouser2));
    memset(Opwd2, 0, sizeof(Opwd2));
    memset(Oics, 0, sizeof(Oics));
    memset(Oocs, 0, sizeof(Oocs));
    memset(clvn, 0, sizeof(clvn));
    memset(clvv, 0, sizeof(clvv));
    for ( i = 0 ; i < 3 ; i++ )
        memset (Obuf[i], 0, sizeof(Obuf[i]));
    Ocso[0] = Ocso[1] = Ocso[2] = 0;

    /* save odb command invocation */
    odbcmd = av[0] ;

    /* check ODB_USER, ODB_PWD, ODB_DSN env variables */
    if ( ( c = getenv ("ODB_DSN") ) ) {
        f |= 00001;
        strmcpy((char *)Obuf[0], c, 2 * sizeof(Odsn));
        for ( j = 0 ; Obuf[0][j] && Obuf[0][j] != ':' ; j++)
            Odsn[j] = Obuf[0][j];
        if ( Obuf[0][j] )   /* we found ':' */
            strmcpy((char *)Odsn2, (char *)&Obuf[0][j+1], sizeof(Odsn2));
    }
    if ( ( c = getenv ("ODB_USER") ) ) {
        f |= 00002;
        strmcpy((char *)Obuf[0], c, 2 * sizeof(Ouser));
        for ( j = 0 ; Obuf[0][j] && Obuf[0][j] != ':' ; j++)
            Ouser[j] = Obuf[0][j];
        if ( Obuf[0][j] )   /* we found ':' */
            strmcpy((char *)Ouser2, (char *)&Obuf[0][j+1], sizeof(Ouser2));
    }
    if ( ( c = getenv ("ODB_PWD") ) ) {
        f |= 00004;
        strmcpy((char *)Obuf[0], c, 2 * sizeof(Opwd));
        for ( j = 0 ; Obuf[0][j] && Obuf[0][j] != ':' ; j++)
            Opwd[j] = Obuf[0][j];
        if ( Obuf[0][j] )   /* we found ':' */
            strmcpy((char *)Opwd2, (char *)&Obuf[0][j+1], sizeof(Opwd2));
    }
    memset(Obuf[0], 0, sizeof(Obuf[0]));

    /* Allocate environment handle & set ODBC 3 */
    if (!SQL_SUCCEEDED(Oret=SQLAllocHandle(SQL_HANDLE_ENV,
            (SQLHANDLE)SQL_NULL_HANDLE, &Oenv))){
        fprintf(stderr, "odb [main(%d)] - Error allocating Environment Handle\n", __LINE__);
        exit(EX_ODBCERR);
    }
    if (!SQL_SUCCEEDED(Oret=SQLSetEnvAttr(Oenv, SQL_ATTR_ODBC_VERSION,
            (void *) SQL_OV_ODBC3, 0))){
        fprintf(stderr, "odb [main(%d)] - Error setting SQL_OV_ODBC3\n", __LINE__);
        exit(EX_ODBCERR);
    }
    /* Register start time */
    gettimeofday(&tvi, (void *)NULL);

    /* handling command line options (no getopt under Win32) */
    if ( ac == 1 ) {
        fprintf(stderr, "odb [main(%d) - No command line options. Check \"-h\"\n", __LINE__);
        exit( EX_USAGE );
    }
    for(i=1; i<ac; i++) {
        if ( !strcmp(av[i], "-h") ) {                       /* help */
            usagexit();
        } else if ( !strcmp(av[i], "-version") ) {          /* print odb version */
            printf("%s - %s\nBuild [%s %s]: %s\n", odbid, odbauth, __DATE__, __TIME__, ODBBLD);
            exit ( EX_OK );
        } else if ( !strcmp(av[i], "-d") ) {                /* data source name */
            f |= 00001;
            if ( ++i < ac ) {
                for ( j = 0 ; av[i][j] && av[i][j] != ':' ; j++);
                if ( av[i][j] )     /* we found ':' */
                    strmcpy((char *)Odsn2, &av[i][j+1], sizeof(Odsn2));
                strmcpy((char *)Odsn, av[i], (size_t)j);
            } else {
                fprintf(stderr, "odb [main(%d)] - Missing data source name after -%s\n", __LINE__, av[i-1]);
                exit( EX_USAGE );
            }
        } else if ( !strcmp(av[i], "-ca") ) {               /* Connection String */
            f |= 00001;
            if ( ++i < ac ) {
                clca = av[i];
            } else {
                fprintf(stderr, "odb [main(%d)] - Missing connection string after -%s\n", __LINE__, av[i-1]);
                exit( EX_USAGE );
            }
        } else if ( !strcmp(av[i], "-u") ) {                /* username */
            f |= 00002;
            if ( ++i < ac ) {
                for ( j = 0 ; av[i][j] && av[i][j] != ':' ; j++);
                if ( av[i][j] )     /* we found ':' */
                    strmcpy((char *)Ouser2, &av[i][j+1], sizeof(Ouser2));
                strmcpy((char *)Ouser, av[i], (size_t)j);
            } else {
                fprintf(stderr, "odb [main(%d)] - Missing user name after %s\n", __LINE__, av[i-1]);
                exit( EX_USAGE );
            }
        } else if ( !strcmp(av[i], "-p") ) {                /* password */
            f |= 00004;
            if ( ++i < ac ) {
                for ( j = 0 ; av[i][j] && av[i][j] != ':' ; j++);
                if ( av[i][j] )     /* we found ':' */
                    strmcpy((char *)Opwd2, &av[i][j+1], sizeof(Opwd2));
                strmcpy((char *)Opwd, av[i], (size_t)j);
#ifdef __hpux
                strmcpy(title, ODBVER, sizeof(title));
                pst.pst_command = title ;
                pstat(PSTAT_SETCMD, pst, strlen(title), 0, 0);
#else
                av[i][0] = '?'; /* hide command line password */
#endif
                for ( j = 1 ; av[i][j] ; j++ )
                    av[i][j] = '\0';
            } else {
                fprintf(stderr, "odb [main(%d)] - Missing password after %s\n", __LINE__, av[i-1]);
                exit( EX_USAGE );
            }
        } else if ( !strcmp(av[i], "-lsdrv") ) {            /* list available Drivers */
            f |= 00010;
        } else if ( !strcmp(av[i], "-lsdsn") ) {            /* list available Data Sources */
            f |= 00020;
        } else if ( !strcmp(av[i], "-q") ) {                /* quiet (do not print cmd & output) */
            if ( (i+1) < ac ) {
                if ( !strcmp(av[i+1], "all") ) {
                    i++;
                    f |= 020000400040;
                } else if ( !strcmp(av[i+1], "cmd") ) {
                    i++;
                    f |= 00040;
                } else if ( !strcmp(av[i+1], "res") ) {
                    i++;
                    f |= 0400000;
                } else if ( !strcmp(av[i+1], "timing") ) {
                    i++;
                    f |= 020000000000;
                } else {
                    if ( av[i+1][0] == '-' ) {
                        f |= 0400040;
                    } else {
                        fprintf(stderr, "odb [main(%d)] - Wrong parameter >%s< after %s\n", __LINE__, av[i+1], av[i]);
                        exit( EX_USAGE );
                    }
                }
            } else {
                f |= 0400040;
            }
        } else if ( !strcmp(av[i], "-i") ) {            /* print database / schema / table info */
            f |= 00100;
            etabadd('i', "", 0);
            if ( (i+1) < ac && av[i+1][0] != '-' ) { /* TYPE[multiplier]:CATALOG.SCHEMA.[TABLE] */
                switch(av[++i][0]) {
                    case 'd':   /* database info */
                    case 'c':   /* catalogs info */
                    case 's':   /* schema info */
                    case 'A':   /* all object types */
                    case 't':   /* table */
                    case 'v':   /* view */
                    case 'a':   /* alias */
                    case 'y':   /* synonym */
                    case 'm':   /* materialized view */
                    case 'e':   /* system table */
                    case 'g':   /* global temporary */
                    case 'l':   /* local temporary */
                    case 'M':   /* materialized view group */
                    case 'T':   /* desc tables */
                    case 'D':   /* desc tables (ddl output) */
                        otype = av[i][0] ;
                        rag[0] = &av[i][2] ;
                        break ;
                    case 'U':   /* desc tables (ddl output char/varchar x multiplier) */
                        otype = av[i][0] ;
                        if ( av[i][1] >= '0' && av[i][1] <= '9' ) {
                            mult = (unsigned int)strtol(&av[i][1], (char **)NULL, 10);
                            for ( j = 0; av[i][j] && av[i][j] != ','; j++);                 
                            wmult = (unsigned int)strtol(&av[i][++j], (char **)NULL, 10);
                        } else {
                            fprintf(stderr, "odb [main(%d)] - Invalid multiplier \'%s\'\n", __LINE__, &av[i][1] );
                            exit ( EX_USAGE ) ;
                        }
                        break ;
                    default:
                        fprintf(stderr, "odb [main(%d)] - Invalid object type \'%c\'\n", __LINE__, av[i][0] );
                        exit ( EX_USAGE ) ;
                        break ;
                }
                for ( j = 0; av[i][j] && av[i][j] != ':'; j++);                 
                if ( av[i][++j] ) {
                    rag[0] = &av[i][j] ;
                } else {
                    fprintf(stderr, "odb [main(%d)] - Missing parameter. Use -i type[mult,wmult]:catalog.schema[.table]\n", __LINE__ );
                    exit ( EX_USAGE ) ;
                }
            }
        } else if ( !strcmp(av[i], "-I") ) {            /* Interpreter mode */
            f |= 022000;    /* interactive and verbose flags on */
            if ( ( i + 1 ) < ac && av[i+1][0] != '-' )
                etabadd('I', av[++i], 0);
            else
                etabadd('I', "", 0);
            printf("%s - SQL Interpreter Mode\nBuild: %s [%s %s]\n", odbid, ODBBLD, __DATE__, __TIME__);
        } else if ( !strcmp(av[i], "-noconnect") ) {    /* do not connect on SQL int startup */
            f |= 040000;
        } else if ( !strcmp(av[i], "-x") ||             /* execute command(s) */
                    !strcmp(av[i], "-f") ) {            /* run script file(s) */
            f |= 00200;
            if ( ++i < ac ) {
                for ( j = 0; av[i][j] && isdigit((int)av[i][j]) ; j++);                 
                for ( l = 0 ; l < (atoi(av[i])?atoi(av[i]):1) ; l++)
                    etabadd(av[i-1][1], (av[i][j]==':')?&av[i][j+1]:av[i], no);
            } else {
                fprintf(stderr, "odb [main(%d)] - Missing command/file after %s\n", __LINE__, av[i-1]);
                exit( EX_USAGE );
            } 
        } else if ( !strcmp(av[i], "-l") ||             /* Load */
                    !strcmp(av[i], "-e") ||             /* Extract */
                    !strcmp(av[i], "-cp") ||            /* Copy */
                    !strcmp(av[i], "-pipe") ||          /* Pipe */
                    !strcmp(av[i], "-diff") ) {         /* Table diff */
            f |= 0200;
            if ( ++i < ac ) {
                etabadd(av[i-1][1], av[i], no);
            } else {
                fprintf(stderr, "odb [main(%d)] - Missing parameters after %s\n", __LINE__, av[i-1]);
                exit( EX_USAGE );
            } 
        } else if ( !strcmp(av[i], "-c") ) {            /* output in csv format */
            f |= 00400;
        } else if ( !strcmp(av[i], "-r") ) {            /* rowset */
            if ( (i+1) < ac ) {
                if ( av[++i][0] == 'k' || av[i][0] =='K' ) {
                    r = 0 ;
                    rb = 1024 * atoi (&av[i][1]);
                } else if ( av[i][0] == 'm' || av[i][0] =='M' ) {
                    r = 0 ;
                    rb = 1024 * 1024 * atoi (&av[i][1]);
                } else {
                    r = (size_t) atoi(av[i]);
                    rb = 0;
                }
            } else {
                fprintf(stderr, "odb [main(%d)] - Missing rowset after %s\n", __LINE__, av[i]);
                exit( EX_USAGE );
            }
        } else if ( !strcmp(av[i], "-N") ) {                /* NULL run */
            f |= 010000;
        } else if ( !strcmp(av[i], "-fs") ) {               /* field separator */
            if ( (i+1) < ac ) {
                fs = mchar(av[++i]);
            } else {
                fprintf(stderr, "odb [main(%d)] - Missing field separator after %s\n", __LINE__, av[i]);
                exit( EX_USAGE );
            }
        } else if ( !strcmp(av[i], "-rs") ) {               /* record separator */
            if ( (i+1) < ac ) {
                rs = mchar(av[++i]);
            } else {
                fprintf(stderr, "odb [main(%d)] - Missing field separator after %s\n", __LINE__, av[i]);
                exit( EX_USAGE );
            }
        } else if ( !strcmp(av[i], "-ec") ) {               /* escape character */
            if ( (i+1) < ac ) {
                ec = mchar(av[++i]);
            } else {
                fprintf(stderr, "odb [main(%d)] - Missing escape character after %s\n", __LINE__, av[i]);
                exit( EX_USAGE );
            }
        } else if ( !strcmp(av[i], "-ns") ) {               /* null string */
            if ( (i+1) < ac ) {
                ns = av[++i];
            } else {
                fprintf(stderr, "odb [main(%d)] - Missing null string after %s\n", __LINE__, av[i]);
                exit( EX_USAGE );
            }
        } else if ( !strcmp(av[i], "-ksep") ) {             /* thousands separator */
            if ( (i+1) < ac ) {
                ksep = mchar(av[++i]);
            } else {
                fprintf(stderr, "odb [main(%d)] - Missing thousands separator after %s\n", __LINE__, av[i]);
                exit( EX_USAGE );
            }
        } else if ( !strcmp(av[i], "-dsep") ) {             /* decimal separator */
            if ( (i+1) < ac ) {
                dsep = mchar(av[++i]);
            } else {
                fprintf(stderr, "odb [main(%d)] - Missing decimal separator after %s\n", __LINE__, av[i]);
                exit( EX_USAGE );
            }
        } else if ( !strcmp(av[i], "-delay") ) {            /* Delay starting threads */
            if ( (i+1) < ac ) {
                d = atoi(av[++i]);
            } else {
                fprintf(stderr, "odb [main(%d)] - Missing delay (ms) after %s\n", __LINE__, av[i]);
                exit ( EX_USAGE ) ;
            }
        } else if ( !strcmp(av[i], "-ldelay") ) {           /* Delay starting threads */
            if ( (i+1) < ac ) {
                ld = (unsigned int)atoi(av[++i]);
            } else {
                fprintf(stderr, "odb [main(%d)] - Missing delay (ms) after %s\n", __LINE__, av[i]);
                exit ( EX_USAGE ) ;
            }
        } else if ( !strcmp(av[i], "-ndsn") ) {             /* Rolling DSN increment */
            if ( (i+1) < ac ) {
                if ( av[++i][0] == '+' ) {
                    f |= 02000000000;                       /* Sequential Rolling DSN (instead of RR) */
                    ndsn = atoi(&av[i][1]);
                } else {
                    ndsn = atoi(av[i]);
                }
            } else {
                fprintf(stderr, "odb [main(%d)] - Missing number of rollong DSN after %s\n", __LINE__, av[i]);
                exit ( EX_USAGE ) ;
            }
        } else if ( !strcmp(av[i], "-soe") ) {              /* Stop On Error */
            f |= 0200000;
        } else if ( !strcmp(av[i], "-pcn") ) {              /* Print Column Names */
            f |= 0200000000;
        } else if ( !strcmp(av[i], "-pad") ) {              /* Pad columns & Print Column Names */
            pad = 1 ;
            fs = '|';
            f |= 0200000000;
        } else if ( !strcmp(av[i], "-dlb") ) {              /* Dynamic Load Balancing */
            f |= 01000000000;
        } else if ( !strcmp(av[i], "-plm") ) {              /* Print Line Mode */
            f |= 040000000;
        } else if ( !strcmp(av[i], "-P") ||                 /* run dir scripts in parallel */
                    !strcmp(av[i], "-S") ) {                /* run dir scripts serially */
            f |= 00200;
            if ( av[i][1] == 'S' )
                f |= 010000000;
            if ( ( i + 1 ) < ac ) {
                for ( j = 0; av[i+1][j] && av[i+1][j] != ':'; j++);                 
                if ( av[i+1][j] ) {
                    n = atoi(av[i+1]);
                    i++;
                } else {
                    n = 1;
                }
            } else {
                fprintf(stderr, "odb [main(%d)] - Missing command/file after %s\n", __LINE__, av[i]);
                exit ( EX_USAGE ) ;
            } 
#ifdef _WIN32
            for ( j = (int) strlen(av[++i]) -1 ; 
                    j && av[i][j] != '\\' && av[i][j] != '/' ; j-- );
            if( (h = FindFirstFile((LPCSTR)av[i], &finfo)) == INVALID_HANDLE_VALUE ) {
                fprintf(stderr, "odb [main(%d)] - Invalid file handle expanding %s: %d\n", 
                        __LINE__, av[i], GetLastError());
                exit(EX_NOINPUT);
            } else {
                st = no;    /* save start point */
                do {
                    l = (int) strlen((char *)finfo.cFileName) + j + 2;
                    if ( ( buff = (char *)calloc ( 1, l + 1 ) ) == (void *)NULL ) {
                        fprintf(stderr, "odb [main(%d)] - Error allocating memory for %s: [%d] %s\n",
                                __LINE__, (char *) finfo.cFileName, errno, strerror(errno) );
                        exit(EX_OSERR);
                    }
                    if ( j )
                        strmcpy (buff, av[i], (size_t) j + 1 ); 
                        strmcpy (buff, av[i], (size_t) j + 1 ); 
                    (void) strmcat(buff, (char *)finfo.cFileName, l, 0);
                    etabadd('F', buff, ( f & 010000000 ) ? 0 : no);
                    if ( n > 0 )    /* serial replication. Ex: -P 3: ... */
                        for ( o = n - 1 ; o ; o-- )
                            etabadd('f', buff, ( f & 010000000 ) ? 0 : no);
                } while(FindNextFile(h, &finfo) && GetLastError() != ERROR_NO_MORE_FILES);
                FindClose(h);
                en = no;    /* save end point */
            }
            if ( n < 0 )    /* Round Robin replication. Ex: -P -3: ... */
                for ( o = - ( n + 1 ) ; o ; o-- )
                    for ( j = st ; j < en ; j++ )
                        etabadd('f', etab[j].run, ( f & 010000000 ) ? 0 : no);
#else
            l = (int) strlen(av[++i]);
            if ( av[i][l-1] == '/' ) {
                if ( ( dp = opendir (av[i]) ) == (DIR *)NULL ) {
                    fprintf(stderr, "odb [main(%d)] - Error opening input dir %s: [%d] %s\n",
                        __LINE__, av[i], errno, strerror(errno) );
                        exit(EX_NOINPUT);
                }
                if ( n > 0 ) {
                    while ( ( de = readdir ( dp ) ) != (struct dirent *)NULL ) {
                        l += (int) strlen ( de->d_name ) + 1;
                        if ( ( buff = calloc ( 1, l + 1 ) ) == (void *)NULL ) {
                            fprintf(stderr, "odb [main(%d)] - Error allocating memory for %s: [%d] %s\n",
                                __LINE__, (char *) de->d_name, errno, strerror(errno) );
                            exit(EX_OSERR);
                        }
                        strmcpy ( buff, av[i], sizeof(sfile) );
                        (void) strmcat ( buff, de->d_name, sizeof(sfile), 0 );
                        if ( ! lstat(buff, &flinfo ) && S_ISREG(flinfo.st_mode) ) {
                            for ( o = n ; o ; o-- ) {
                                etabadd('F', buff, ( f & 010000000 ) ? 0 : no);
                            }
                        }
                    }
                } else {
                    for ( o = n ; o ; o++ ) {
                        while ( ( de = readdir ( dp ) ) != (struct dirent *)NULL ) {
                            l += (int) strlen ( de->d_name ) + 1;
                            if ( ( buff = calloc ( 1, l + 1) ) == (void *)NULL ) {
                                fprintf(stderr, "odb [main(%d)] - Error allocating memory for %s: [%d] %s\n",
                                    __LINE__, (char *) de->d_name, errno, strerror(errno) );
                                exit(EX_OSERR);
                            }
                            strmcpy ( buff, av[i], sizeof(sfile) );
                            (void) strmcat ( buff, de->d_name, sizeof(sfile), 0 );
                            if ( ! lstat(buff, &flinfo ) && S_ISREG(flinfo.st_mode) ) {
                                etabadd('F', buff, ( f & 010000000 ) ? 0 : no);
                            }
                        }
                        rewinddir( dp );
                    }
                }
                closedir ( dp );
            } else {
                if ( n > 0 ) {
                    for ( ; ( i + 1 ) < ac && av[i][0] != '-' ; i++ ) {
                        for ( o = n ; o ; o-- )
                            etabadd('f', av[i], ( f & 010000000 ) ? 0 : no);
                    }
                } else {
                    for ( l = i, o = n ; o ; o++, i = l ) {
                        for ( ; ( i + 1 ) < ac && av[i][0] != '-' ; i++ )
                            etabadd('f', av[i], ( f & 010000000 ) ? 0 : no);
                    }
                }
                i--;
            }
#endif
        } else if ( !strcmp(av[i], "-T") ) {                /* Max number of parallel threads */
            if ( (i+1) < ac ) {
                tn = atoi(av[++i]);
            } else {
                fprintf(stderr, "odb [main(%d)] - Missing max threads after %s\n", __LINE__, av[i]);
                exit ( EX_USAGE ) ;
            }
        } else if ( !strcmp(av[i], "-F") ) {                /* Max number of rows to be fetched */
            if ( (i+1) < ac ) {
                mf = atoi(av[++i]);
            } else {
                fprintf(stderr, "odb [main(%d)] - Missing max rows to be fetched after %s\n", __LINE__, av[i]);
                exit ( EX_USAGE ) ;
            }
        } else if ( !strcmp(av[i], "-ttime") ) {
            if ( (i+1) < ac ) {
                cd = (unsigned int)atoi(av[++i]);
                for ( j = 0; av[i][j] && av[i][j] != ':'; j++);                 
                cd2 = (unsigned int)atoi(&av[i][j+1]);
            } else {
                fprintf(stderr, "odb [main(%d)] - Missing delay (ms) after %s\n", __LINE__, av[i]);
                exit ( EX_USAGE ) ;
            }
        } else if ( !strcmp(av[i], "-L") ) {                /* Number of loops */
            if ( (i+1) < ac ) {
                n = atoi(av[++i]);
            } else {
                fprintf(stderr, "odb [main(%d)] - Missing number of loops after %s\n", __LINE__, av[i]);
                exit ( EX_USAGE ) ;
            }
            if ( n < 0 ) {          /* negative number of loops: reset to 1 */
                fprintf(stderr, "odb [main(%d)] - Inconsistent #loops: %d. Forced to 1\n" , __LINE__, n);
                nloop = 1;
            } else {                /* positive number of loops: ok */
                nloop = (unsigned int)n;
            }
        } else if ( !strcmp(av[i], "-timeout") ) {          /* General timeout */
#ifdef _WIN32
            fprintf(stderr, "odb [main(%d)] - timeout not available under Win32\n", __LINE__);
            exit ( EX_USAGE ) ;
#else
            if ( (i+1) < ac ) {
                if ( av[++i][0] == 'h' || av[i][0] == 'H' )
                    w = 3600*(unsigned int)atoi(&av[i][1]);
                else if ( av[i][0] == 'm' || av[i][0] == 'M' )
                    w = 60*(unsigned int)atoi(&av[i][1]);
                else
                    w = (unsigned int)atoi(av[i]);
            } else {
                fprintf(stderr, "odb [main(%d)] - Missing timeout after %s\n", __LINE__, av[i]);
                exit ( EX_USAGE ) ;
            }
#endif
        } else if ( !strcmp(av[i], "-U") ) {                /* sets SQL_TXN_READ_UNCOMMITTED */
            f |= 04000000;
        } else if ( !strcmp(av[i], "-v") ) {                /* be verbose */
            f |= 020000;
        } else if ( !strcmp(av[i], "-vv") ) {               /* Print Execution Table */
            f |= 001000;
        } else if ( !strcmp(av[i], "-b") ) {                /* add start time in the CSV header */
            f |= 01000000;
        } else if ( !strcmp(av[i], "-noschema") ) {         /* no schema */
            f |= 020000000;
        } else if ( !strcmp(av[i], "-nocatalog") ) {        /* no catalog */
            f |= 0100000000;
        } else if ( !strcmp(av[i], "-nocatnull") ) {        /* no catalog: use NULL instead of empty strings */
            f |= 04100000000;
        } else if ( !strcmp(av[i], "-ucs2toutf8") ) {       /* set UCS-2 to UTF-8 conversion in odb */
            f |= 0400000000;
        } else if ( !strcmp(av[i], "-Z") ) {                /* shuffle etab[] */
            f |= 02000000;
        } else if ( !strcmp(av[i], "-var") ) {              /* User defined variables */
            if ( clv >= MAX_CLV) {
                fprintf(stderr, "odb [main(%d)] - Max number of command line variables (%d) reached\n",
                    __LINE__, MAX_CLV);
            } else if ( (i+2) < ac ) {
                clvn[clv]=(unsigned char)++i;
                clvv[clv++]=(unsigned char)++i;
            } else {
                fprintf(stderr, "odb [main(%d)] - Missing parameters after %s\n", __LINE__, av[i]);
                exit ( EX_USAGE ) ;
            }
        } else if ( !strcmp(av[i], "-sq") ) {               /* string qualifier */
            if ( (i+1) < ac ) {
                sq = mchar(av[++i]);
            } else {
                fprintf(stderr, "odb [main(%d)] - Missing parameters after %s\n", __LINE__, av[i]);
                exit ( EX_USAGE ) ;
            }
        } else if ( !strcmp(av[i], "-drs") ) {              /* describe the result set */
            f |= 0100000;
        } else if ( !strcmp(av[i], "-bin") ) {              /* bind to SQL_C_BINARY */
            f |= 04000;
        } else if ( !strcmp(av[i], "-dump") ) {             /* dump ODBC buffers to file */
            if ( (i+1) < ac ) {
                if ( !(strcmp(av[++i], "stdout") ) ) {
                    fdmp = stdout ;
                } else {
                    if ( ( fdmp = fopen(av[i], "w") ) == (FILE *)NULL )
                        fprintf(stderr, "odb [main(%d)] - Cannot open %s: [%d] %s\n",
                            __LINE__, av[i], errno, strerror(errno));
                }
            } else {
                fprintf(stderr, "odb [main(%d)] - Missing dumpfile after %s\n", __LINE__, av[i]);
                exit ( EX_USAGE ) ;
            }
        } else if ( !strcmp(av[i], "-hint") ) {             /* C-Style comments as hints */
            f2 |= 0001 ;
        } else if ( !strcmp(av[i], "-nps") ) {              /* Network Packet Size */
            if ( (i+1) < ac ) {
                Onps = (SQLUINTEGER)strtol(av[++i], (char **)NULL, 10);
                for ( j = 0; av[i][j] && av[i][j] != ':'; j++);                 
                Onps2 = (SQLUINTEGER)strtol(&av[i][j+1], (char **)NULL, 10);
            } else {
                fprintf(stderr, "odb [main(%d)] - Missing parameter after %s\n", __LINE__, av[i]);
                exit ( EX_USAGE ) ;
            }
        } else if ( !strcmp(av[i], "-casesens") ) {         /* Case sensitive DB */
            f |= 010000000000 ;
        } else {
            fprintf(stderr, "odb [main(%d)] - Unknow option %s. Ignored\n", __LINE__, av[i]);
            exstat = EX_USAGE ; /* This is the right exit status now... unless valid jobs overwrite it */
        }
    }

    /* Close etabadd() connection (if any) */
    if (Oc) {
        if (!SQL_SUCCEEDED(Oret=SQLDisconnect(Oc)))
            Oerr(-1, -1, __LINE__, Oc, SQL_HANDLE_DBC);
        (void)SQLFreeHandle(SQL_HANDLE_DBC, Oc); /* free conn handle */
        Oc = 0;
    }

    /* Exit if no etab entries and !-I !-lsdsn !-lsdrv */
    if ( !no && !(f & 020030) )
        exit( exstat );

    if ( ! ( f & 02000 ) )	/* Not Interpreter mode */
      (void)signal(SIGINT, SIG_IGN);  /* Ignore signal interrupts for now */

#ifdef _WIN32
    /* Initialize dlbmutex (Win32) */
    InitializeCriticalSection(&dlbmutex);
#endif


    if ( f & 00010 ) {  /* [-lsdrv] list available driver */
        for(Odir=SQL_FETCH_FIRST; SQL_SUCCEEDED(Oret=SQLDrivers(Oenv, Odir,
                Obuf[0], (SQLSMALLINT)sizeof(Obuf[0]), &Osio[0],
                Obuf[1], (SQLSMALLINT)sizeof(Obuf[1]), &Osio[1]));
                Odir=SQL_FETCH_NEXT) {
            printf("%s - %s\n", (char *)Obuf[0], (char *)Obuf[1]);
            if (Oret == SQL_SUCCESS_WITH_INFO)
                fprintf(stderr, "odb [main(%d)] - Warning data truncation\n", __LINE__);
        }
        (void)SQLFreeHandle(SQL_HANDLE_ENV, Oenv);
        exit( EX_OK );
    }

    if ( f & 00020 ) {  /* [-lsdsn] list available data sources */
        for(Odir=SQL_FETCH_FIRST; SQL_SUCCEEDED(Oret=SQLDataSources(Oenv, Odir,
                Obuf[0], (SQLSMALLINT)sizeof(Obuf[0]), &Osio[0],
                Obuf[1], (SQLSMALLINT)sizeof(Obuf[1]), &Osio[1]));
                Odir=SQL_FETCH_NEXT) {
            printf("%s - %s\n", (char *)Obuf[0], (char *)Obuf[1]);
            if (Oret == SQL_SUCCESS_WITH_INFO)
                fprintf(stderr, "odb [main(%d)] - Warning data truncation\n", __LINE__);
        }
        (void)SQLFreeHandle(SQL_HANDLE_ENV, Oenv);
        exit( EX_OK );
    }

    /* Ask the password if missing in interactive mode */
    if ( ( f & 020003 ) && ! ( f & 00004 ) ) {
#ifdef _WIN32
        sts = GetStdHandle(STD_INPUT_HANDLE);       /* get STDIN Handle */
        GetConsoleMode(sts, (LPDWORD)&mode);        /* get current STDIN mode*/
        SetConsoleMode(sts, mode & ~ENABLE_ECHO_INPUT); /* Set mode for STDIN with no ECHO */ 
        do {
            fputs("Password:", stdout);                 /* Password Prompt */
        } while (!fgets((char *)Opwd, sizeof(Opwd), stdin));    /* Get Password */
        SetConsoleMode(sts, mode);                  /* Reset Original console mode */
        putc('\n', stdout);
#else
        tcgetattr(0,&sts);          /* get current settings for file descriptor 0 */
        pts = sts;                  /* copy the current settings */
        pts.c_lflag &= ~ECHO;       /* change password settings to no echo */
        pts.c_lflag |= ECHONL;      /* ... but echo New Line */
        tcsetattr(0,TCSANOW,&pts);  /* use passord settings */
        do {
            fputs("Password:", stdout); /* Password prompt */
        } while (!fgets((char *)Opwd, sizeof(Opwd), stdin));
        tcsetattr(0,TCSANOW,&sts);  /* reset previous settings */
#endif
        Opwd[strlen((char *)Opwd) - 1] = '\0';  /* Clear ending new line */
        f |= 00004;
    }

    /* From now on we need a connection unless -I */
    if ( ( f & 040000 ) && ! ( f & 020000 ) ) {
        fprintf(stderr, "odb [main(%d)] - Connection required\n", __LINE__);
        exit ( EX_USAGE );
    }

    /* passwd (04), user (02) and DSN (01) needed if not interactive*/
    if ( !(f & 02000 ) ) {
        if ( ! ( f & 0001 ) ) {
            fprintf(stderr, "odb [main(%d)] - Missing DSN\n", __LINE__ );
            exit ( EX_USAGE ) ;
        }
        if ( ! ( f & 0002 ) ) {
            fprintf(stderr, "odb [main(%d)] - Missing User\n", __LINE__ );
            exit ( EX_USAGE ) ;
        }
        if ( ! ( f & 0004 ) ) {
            fprintf(stderr, "odb [main(%d)] - Missing password\n", __LINE__ );
            exit ( EX_USAGE ) ;
        }
    }

    /* Check threads number vs Execution Table entries */
    if ( tn > no ) {
        fprintf(stderr, "odb [main(%d)] - Warning: won't be created more thread (%d) then needed (%d).\n",
            __LINE__, tn, no);
        tn = no;
    }
    /* Check threads number and Serial Execution */
    if ( f & 010000000 && tn > 1 ) {
        fprintf(stderr, "odb [main(%d)] - Warning: only one thread will be used for serial runs.\n",
            __LINE__);
        tn = 1;
    }
    
    /* Distribute work across threads */
    if ( tn ) {
        for ( i = j = 0; i < no; i++, j = ( j + 1 == tn ? 0 : j + 1 ) )
            etab[i].id = j;
    } else {
        tn = ( f & 010000000 ) || !no ? 1 : no ;
    } 

    /* Copy Global Flags into Single Execution flags */
    for ( i = 0 ; i < no ; i++ ) {
        if( f & 040 ) etab[i].flg |= 010000;        /* Quiet cmd */
        if( f & 04000 ) etab[i].flg |= 0100;        /* Binary mode */
        if( f & 010000 ) etab[i].flg |= 0010;       /* Null Run */
        if( f & 0100000 ) etab[i].flg |= 0400000;   /* Describe Result Set */
        if( f & 0200000 ) etab[i].flg |= 0004;      /* Stop On Error */
        if( f & 0400000 ) etab[i].flg |= 020000;    /* Quiet results */
        if( f & 040000000 ) etab[i].flg |= 040000;  /* Print Line Mode */
        if( f & 0200000000 ) etab[i].flg |= 0200;   /* Print Column Names */
        if( f & 0400000000 ) etab[i].flg |= 010000000;  /* ucs2toutf8 */
        if( f & 020000000000 ) etab[i].flg2 |= 0004;    /* Quiet Timing */
        if ( r && !etab[i].r )
            etab[i].r = r;
        if ( rb && !etab[i].rbs )
            etab[i].rbs = rb;
        if ( mf ) {
            etab[i].mr = mf;                        /* max number of rows to fetch */
            if ( etab[i].r > etab[i].mr )           /* if # records to fetch < rowset ... */
                etab[i].r = etab[i].mr;             /* make rowset = records to fetch */
        }
        if ( fs && !etab[i].fs )
            etab[i].fs = fs;
        if ( rs && !etab[i].rs )
            etab[i].rs = rs;
        if ( sq && !etab[i].sq )
            etab[i].sq = sq;
        if ( ec && !etab[i].ec )
            etab[i].ec = ec;
        if ( ns && !etab[i].ns ) {
            etab[i].ns = ns;    /* calculate nullstr length only once... */
            etab[i].nl = i ? etab[0].nl :  (unsigned int) strlen(ns);
        }
    }

    /* Print Execution Table content */
    if ( f & 001000 ) {
        fprintf (stderr, "odb [main(%d)] - Execution Table content (%d items):\n", __LINE__, no);
        for ( i = 0 ; i < no ; i++ ) {
            fprintf(stderr, "etab[%d]:\n", i);
            fprintf(stderr, "\tType (.type): %c\n", etab[i].type);
            fprintf(stderr, "\tThread ID (.id): %d\n", etab[i].id);
            fprintf(stderr, "\tParent EID (.parent): %d\n", etab[i].parent);
            fprintf(stderr, "\tChild EID (.child): %d\n", etab[i].child);
            fprintf(stderr, "\tTBE flag (.tbe): %d\n", etab[i].tbe);
            fprintf(stderr, "\tField Separator (.fs): %d (decimal value)\n", etab[i].fs);
            fprintf(stderr, "\tMulti Characters Field Separator (.mcfs): %s\n", etab[i].mcfs);
            fprintf(stderr, "\tRecord Separator (.rs): %d (decimal value)\n", etab[i].rs);
            fprintf(stderr, "\tMulti Characters Record Separator (.mcrs): %s\n", etab[i].mcrs);
            fprintf(stderr, "\tString Qualifier (.sq): %d (decimal value)\n", etab[i].sq);
            fprintf(stderr, "\tEscape Character (.ec): %d (decimal value)\n", etab[i].ec);
            fprintf(stderr, "\tEmbed file Character (.em): %d (decimal value)\n", etab[i].em);
            fprintf(stderr, "\tPad Character (.pc): %d (decimal value)\n", etab[i].pc);
            fprintf(stderr, "\tRun (.run): %s\n", etab[i].run);
            fprintf(stderr, "\tMax Rows (.mr): %lu\n", etab[i].mr);
            fprintf(stderr, "\tMax Errors (.mer): %d\n", etab[i].mer);
            fprintf(stderr, "\tEID flags (.flg/.flg2): %o/%o (octal values)\n", etab[i].flg, etab[i].flg2);
            fprintf(stderr, "\tRowset (.r): " SIZET_SPEC " rows\n", etab[i].r);
            fprintf(stderr, "\tRowset Buffer Size (.rbs): " SIZET_SPEC " bytes\n", etab[i].rbs);
            fprintf(stderr, "\tRow Buffer Size (.s): " SIZET_SPEC " bytes\n", etab[i].s);
            fprintf(stderr, "\tIO Buffer Size (.iobuff): %zu bytes\n", etab[i].iobuff);
            fprintf(stderr, "\tRW Buffer Size (.buffsz): %zu bytes\n", etab[i].buffsz);
            fprintf(stderr, "\tPre SQL (.pre): %s\n", etab[i].pre);
            fprintf(stderr, "\tMPre SQL (.mpre): %s\n", etab[i].mpre);
            fprintf(stderr, "\tPost SQL (.post): %s\n", etab[i].post);
            fprintf(stderr, "\tDatabase Type (.dbt): %u\n", etab[i].dbt);
            switch ( etab[i].type ) {
            case 'l':
            case 'L':
                fprintf(stderr, "\tSource (.src): %s\n", etab[i].src);
                fprintf(stderr, "\tTarget (.Ocso[0-2]): %s.%s.%s\n", etab[i].Ocso[0], etab[i].Ocso[1], etab[i].Ocso[2]);
                fprintf(stderr, "\tMap File (.map): %s\n", etab[i].map);
                fprintf(stderr, "\tLines to skip (.k): %u\n", etab[i].k);
                fprintf(stderr, "\tParallel Streams (.ps): %u\n", etab[i].ps);
                fprintf(stderr, "\tNull String (.ns): %s\n", etab[i].ns);
                fprintf(stderr, "\tCommit (.cmt): %d (-1=end, 0=auto, >0 num rows)\n", etab[i].cmt);
                fprintf(stderr, "\tField Truncation (.fldtr): %d\n", etab[i].cmt);
                fprintf(stderr, "\tLoad Command (.loadcmd): %s\n", etab[i].loadcmd);
                break;
            case 'e':
                fprintf(stderr, "\tSQL extract (.sql): %s\n", etab[i].sql);
                fprintf(stderr, "\tSource (.src): %s\n", etab[i].src);
                fprintf(stderr, "\tTarget (.tgt): %s\n", etab[i].tgt);
                fprintf(stderr, "\tParallel Streams (.ps): %u\n", etab[i].ps);
                fprintf(stderr, "\tPWhere Condition (.map): %s\n", etab[i].map);
                fprintf(stderr, "\tSplit By (.sb): %s\n", etab[i].sb);
                fprintf(stderr, "\tSplitby min/max (.sbmin/.sbmax): %ld/%ld\n", etab[i].sbmin, etab[i].sbmax);
                fprintf(stderr, "\tStart/End (.sp/.ep): %u/%u\n", etab[i].sp, etab[i].ep);
                fprintf(stderr, "\tNull String (.ns): %s\n", etab[i].ns);
                fprintf(stderr, "\tEmpty String (.es): %s\n", etab[i].es);
                fprintf(stderr, "\tGZIP Parameters (.gzp): %s\n", etab[i].gzp);
                fprintf(stderr, "\tColumns (.cols): %s\n", etab[i].cols);
                break;
            case 'd':
            case 'D':
            case 'Z':
                fprintf(stderr, "\tGrand Parent EID (.k): %u\n", etab[i].k);
                fprintf(stderr, "\tSource (.src): %s\n", etab[i].src);
                fprintf(stderr, "\tTarget (.tgt): %s\n", etab[i].tgt);
                fprintf(stderr, "\tKey (.key): %s", etab[i].key[0]);
                for ( j = 1 ; j < (int)etab[i].pc ; j++ )
                    fprintf(stderr, ",%s", etab[i].key[j]);
                fprintf(stderr, "\n\tParallel Streams (.ps): %u\n", etab[i].ps);
                fprintf(stderr, "\tPWhere Condition (.map): %s\n", etab[i].map);
                fprintf(stderr, "\tStart/End (.sp/.ep): %u/%u\n", etab[i].sp, etab[i].ep);
                fprintf(stderr, "\tSplitby min/max (.sbmin/.sbmax): %ld/%ld\n", etab[i].sbmin, etab[i].sbmax);
                fprintf(stderr, "\tSplit By (.sb): %s\n", etab[i].sb);
                fprintf(stderr, "\tOutput File (.map): %s\n", etab[i].map);
                break;
            case 'c':
            case 'C':
                fprintf(stderr, "\tSQL extract (.sql): %s\n", etab[i].sql);
                fprintf(stderr, "\tGrand Parent EID (.k): %u\n", etab[i].k);
                fprintf(stderr, "\tSource (.src): %s\n", etab[i].src);
                fprintf(stderr, "\tTarget (.tgt): %s\n", etab[i].tgt);
                fprintf(stderr, "\tParallel Streams (.ps): %u\n", etab[i].ps);
                fprintf(stderr, "\tPWhere Condition (.map): %s\n", etab[i].map);
                fprintf(stderr, "\tSplit By (.sb): %s\n", etab[i].sb);
                fprintf(stderr, "\tStart/End (.sp/.ep): %u/%u\n", etab[i].sp, etab[i].ep);
                fprintf(stderr, "\tSplitby min/max (.sbmin/.sbmax): %ld/%ld\n", etab[i].sbmin, etab[i].sbmax);
                fprintf(stderr, "\tLoad Command (.loadcmd): %s\n", etab[i].loadcmd);
                break;
            case 'p':
            case 'P':
                fprintf(stderr, "\tSQL from source (.sql): %s\n", etab[i].sql);
                fprintf(stderr, "\tSQL to target (.tgtsql): %s\n", etab[i].tgtsql);
                fprintf(stderr, "\tGrand Parent EID (.k): %u\n", etab[i].k);
                fprintf(stderr, "\tParallel Streams (.ps): %u\n", etab[i].ps);
                fprintf(stderr, "\tSplit By (.sb): %s\n", etab[i].sb);
                fprintf(stderr, "\tStart/End (.sp/.ep): %u/%u\n", etab[i].sp, etab[i].ep);
                fprintf(stderr, "\tSplitby min/max (.sbmin/.sbmax): %ld/%ld\n", etab[i].sbmin, etab[i].sbmax);
                fprintf(stderr, "\tLoad Command (.loadcmd): %s\n", etab[i].loadcmd);
                break;
            }
        }
        if ( f & 0400040 )  /* exit if -q all ( not documented hack ) */
            exit ( EX_OK );
    }

    /* Allocate memory for the thread structures */
    if ( !( f & 010000000 ) ) {
#ifdef _WIN32
        if ( (thhn = (HANDLE *)calloc ((size_t)tn, sizeof(HANDLE))) == (void *)NULL ) {
            fprintf(stderr, "odb [main(%d)] - Error allocating thhn memory: [%d] %s\n",
                __LINE__, errno, strerror(errno));
            exit( EX_OSERR );
        }   
#else
        if ( (thid = calloc ((size_t)tn, sizeof(pthread_t))) == (void *)NULL ) {
            fprintf(stderr, "odb [main(%d)] - Error allocating thid memory: [%d] %s\n",
                __LINE__, errno, strerror(errno));
            exit( EX_OSERR );
        }   
#endif
    }
    if ( (thps = (struct thp *)calloc ((size_t)tn, sizeof(struct thp))) == (void *)NULL ) {
        fprintf(stderr, "odb [main(%d)] - Error allocating thps memory: [%d] %s\n",
            __LINE__, errno, strerror(errno));
        exit( EX_OSERR );
    }   

    /* Initialize thps structure with default/command line options */
    for( i = 0; i < tn ; i++ ) {
        thps[i].tid = i;
        thps[i].cd = cd;
        thps[i].cd2 = cd2;
        for ( j = 0 ; j < clv ; j++ )   /* initialize variables */
            var_set ( &thps[i].tva, VTYPE_U, av[clvn[j]], av[clvv[j]] );
    }   

    /* Identify threads not requiring connection (.cr>0) or using 2nd user/dsn (.cr<0) */
    for ( j = 0 ; j < tn ; j++ ) {
        for ( i = 0 ; i < no ; i++ ) {
            if ( etab[i].id == j && etab[i].type == 'l' && etab[i].ps &&
                 !etab[i].pre && !etab[i].post &&
                 etab[i+1].type == 'L' && etab[i+1].parent == i && i < no )
                thps[j].cr = etab[i+1].id;
            if ( etab[i].id == j && ( etab[i].type == 'C' || etab[i].type == 'D' || etab[i].type == 'P' ) ) {
                thps[j].cr = -1;
                if ( !Ouser2[0] ) 
                    strmcpy ( (char *)Ouser2, (char *)Ouser, sizeof(Ouser2) );
                if ( !Odsn2[0] ) 
                    strmcpy ( (char *)Odsn2, (char *)Odsn, sizeof(Odsn2) );
                if ( !Opwd2[0] ) 
                    strmcpy ( (char *)Opwd2, (char *)Opwd, sizeof(Opwd2) );
            } else if ( etab[i].id == j && ( etab[i].type == 'Z' ) ) {
                thps[j].cr = -2;
            }
        }
    }

    /* Create ODBC connections (if thps[].cr <=0). We do need:
        (1) username, (2) password and (3) either DSN or Connection Attributes */
    if ( ! ( f & 00040 ) ) {
        ptime = (time_t)tvi.tv_sec; /* WIN localtime() requires time_t input */
        nowt = localtime(&ptime);
        strftime(nows, sizeof(nows), "%Y-%m-%d %H:%M:%S", nowt);
        fprintf(stderr, "odb [%s]: starting", nows);
    }
    if ( ( f & 00006 ) && ( Odsn[0] || clca ) && !( f & 040000 ) ) {
        if ( ! ( f & 00040 ) )
            fprintf(stderr, " ODBC connection(s)...");
        if ( Ouser2[0] && Opwd2[0] && Odsn2[0] )
            (void) bcs ( Obuf[0], sizeof(Obuf[0]), Ouser2, Opwd2, Odsn2, NULL, 0 );
        for( i = 0; i < tn ; i++ ) {
            if ( thps[i].cr > 0 ) {
                fprintf(stderr, " (%d)", thps[i].cr);
                continue;
            } else if ( thps[i].cr == ( -2 ) ) {
                fprintf(stderr, " [%d]", i);
                continue;
            }
            if ( ! ( f & 00040) )
                fprintf(stderr, thps[i].cr ? " >%d" : " %d", i);
            fflush(stderr);
            (void) bcs ( Oics, sizeof(Oics), Ouser, Opwd, Odsn, clca, i+1 );
            if (!SQL_SUCCEEDED(Oret=SQLAllocHandle(SQL_HANDLE_DBC, Oenv, &thps[i].Oc))){
                Oerr(-1, -1, __LINE__, Oenv, SQL_HANDLE_ENV);
                exit( EX_ODBCERR );
            }
            if ( f & 0400000000 ) {         /* uc2toutf8 is set. change ODBCINI to ${ODBCINI}0 */
                if (Oc) {
                    if (!SQL_SUCCEEDED(Oret=SQLDisconnect(Oc)))
                        Oerr(-1, -1, __LINE__, Oc, SQL_HANDLE_DBC);
                    (void)SQLFreeHandle(SQL_HANDLE_DBC, Oc); /* free conn handle */
                    Oc = 0;
                }
                if ( Setenv ( "ODBCINI", getenv("ODBCINI0") ) ) {
                    fprintf(stderr, "odb [main(%d)] - Error allocating setting %s: [%d] %s\n",
                        __LINE__, sfile, errno, strerror(errno));
                    exit( EX_OSERR );
                }
            }
            if ( i == 0 && Oc ) {   /* re-use etabadd Oc connection as thps[0].Oc if uc2toutf8 is NOT set */
                thps[0].Oc = Oc ;
                (void)SQLFreeHandle(SQL_HANDLE_DBC, Oc);
                Oc = 0;
            } else {
                if ( thps[i].cr && Onps2 && !SQL_SUCCEEDED(Oret=SQLSetConnectAttr(thps[i].Oc, 
                        (SQLINTEGER)SQL_ATTR_PACKET_SIZE, (SQLPOINTER)&Onps2, SQL_IS_UINTEGER ) ) )
                    Oerr(-1, -1, __LINE__, Oenv, SQL_HANDLE_ENV);
                if ( !thps[i].cr && Onps && !SQL_SUCCEEDED(Oret=SQLSetConnectAttr(thps[i].Oc, 
                        (SQLINTEGER)SQL_ATTR_PACKET_SIZE, (SQLPOINTER)&Onps, SQL_IS_UINTEGER ) ) )
                    Oerr(-1, -1, __LINE__, Oenv, SQL_HANDLE_ENV);
                if ((Oret=SQLDriverConnect(thps[i].Oc, (SQLHWND) NULL,
                        thps[i].cr ? Obuf[0] : Oics, SQL_NTS, Oocs, (SQLSMALLINT) sizeof(Oocs),
                        &Oocsl, (SQLUSMALLINT)SQL_DRIVER_NOPROMPT)) != SQL_SUCCESS) {
                    Oerr(-1, -1, __LINE__, thps[i].Oc, SQL_HANDLE_DBC);
                }
            }
            if ( ! SQL_SUCCEEDED(Oret) )
                exit( EX_ODBCERR );
            if ( ( f & 04000000 ) &&
                    !SQL_SUCCEEDED(Oret=SQLSetConnectAttr(thps[i].Oc, 
                    (SQLINTEGER)SQL_ATTR_TXN_ISOLATION, 
                    (SQLPOINTER)SQL_TXN_READ_UNCOMMITTED, 0))){
                Oerr(-1, -1, __LINE__, Oenv, SQL_HANDLE_ENV);
                exit( EX_ODBCERR );
            }
            if ( !SQL_SUCCEEDED(Oret=SQLAllocHandle(SQL_HANDLE_STMT, thps[i].Oc, &thps[i].Os))){
                Oerr(-1, -1, __LINE__, thps[i].Oc, SQL_HANDLE_DBC);
                exit( EX_ODBCERR );
            }
            /* Driver not to scan SQL strings for ODBC Escapes (improve perf) */
            if (!SQL_SUCCEEDED(Oret=SQLSetStmtAttr(thps[i].Os, SQL_ATTR_NOSCAN,
                    (SQLPOINTER)SQL_NOSCAN_ON, 0))) {
                Oerr(-1, -1, __LINE__, thps[i].Os, SQL_HANDLE_STMT);
                exit( EX_ODBCERR );
            }
        }
        etab[0].flg |= 0100000;
    }
    if ( ! ( f & 00040 ) )
        fputc('\n', stderr);

    /* shuffle etab[] */
    if ( f & 02000000 )
        etabshuffle();

    /* initialize random number generator */
    srand((unsigned int)time(NULL));

    if ( f & 00100) {   /* [-i] print database/schema/table info */
        switch ( otype ) {
        case 'd':   /* database info */
            Odinfo();
            break;
        case 'c':   /* catalogs info */
            if ( !(f & 0100000000) )    /* if no nocatalog */
                Olist((SQLCHAR *)SQL_ALL_CATALOGS, (SQLCHAR *)"", (SQLCHAR *)"", 'c');
            break;
        case 's':   /* schema info */
            Olist((SQLCHAR *)"", (SQLCHAR *)SQL_ALL_SCHEMAS, (SQLCHAR *)"", 's');
            break;
        case 'A':   /* all object types */
        case 't':   /* table */
        case 'v':   /* view */
        case 'a':   /* alias */
        case 'y':   /* synonym */
        case 'm':   /* materialized view */
        case 'e':   /* system table */
        case 'g':   /* global temporary */
        case 'l':   /* local temporary */
        case 'M':   /* materialized view group */
        case 'T':   /* desc tables */
        case 'D':   /* desc tables (ddl output) */
        case 'U':   /* desc tables (ddl output char/varchar x4) */
            etab[0].dbt = checkdb(0, &thps[0].Oc, NULL, NULL);
            splitcso(rag[0], Ocso, 0);
            Olist((f & 0100000000)?(SQLCHAR *)"":Ocso[0], (f & 020000000)?(SQLCHAR *)"":Ocso[1], Ocso[2], otype);
            break;
        default:
            fprintf(stderr, "odb [main(%d)] - Invalid object type >%c<\n", __LINE__, otype);
            break;
        }
    }

    /* Interpreter: */
    if ( f & 02000 ) {  /* [-I] Interpreter mode */
        /* Initializing  variables */
        csn[0] = ccn[0] = '\0';
        strmcpy(prompt, Oocs[0] ? "SQL> " : "NDC> ", sizeof(prompt));
        strmcpy(tprompt, "%M> ", sizeof(tprompt));
        strmcpy(chsch, "set schema", sizeof(chsch));    /* default set schema cmd */
        ol = (unsigned int) snprintf(odbcl, sizeof(odbcl), "%s -u %s -p %s %s%s %s%s",
            odbcmd, (char *)Ouser, (char *)Opwd, Odsn[0] ? "-d " : "", Odsn[0] ? (char *)Odsn : "",
            clca ? "-ca" : "", clca ? clca : "" );
        if ( ol == (unsigned int)sizeof(odbcl) ) 
            fprintf(stderr, "odb [main(%d)] - Warning odb command truncation\n", __LINE__);
        ssl = 10;                       /* default "set schema" length */
        f |= 010000000;                 /* assume serial run for scripts */
        j = 0;                          /* #char in Ocmd */
        k = 0;                          /* current arg# */
        l = 1;                          /* l>0 new cmd; l=0 continuation */
        i = 1;                          /* loop variable */

        /* Set database type, csn and ccn */
        if ( thps[0].Oc ) {
            etab[0].dbt = checkdb(0, &thps[0].Oc, ccn, csn);
            var_set ( &thps[0].tva, VTYPE_I, "cs", (char *)Oocs );
            var_set ( &thps[0].tva, VTYPE_I, "catalog", ccn[0] ? (char *)ccn : "unknown" );
            var_set ( &thps[0].tva, VTYPE_I, "schema", csn[0] ? (char *)csn : "unknown" );
        }
        if ( etab[0].dbt != VERTICA )
          (void)signal(SIGINT, cancel);	/* Keyboard Ctrl-C (interactive) */

        /* Opening ODB_INI file & reading initial section */
        if ( etab[0].run ) {
            strmcpy(sfile, getenv("ODB_INI"), sizeof(sfile));
            if ( !sfile[0] ) {  /* ODB_INI is empty */
#ifdef _WIN32
                strmcpy(sfile, getenv("HOMEDRIVE"), sizeof(sfile));
                strmcat(sfile, getenv("HOMEPATH"), sizeof(sfile), 0);
                strmcat(sfile, "\\_odbrc", sizeof(sfile), 0);
#else
                strmcpy(sfile, getenv("HOME"), sizeof(sfile));
                strmcat(sfile, "/.odbrc", sizeof(sfile), 0);
#endif
            }
            if ((fp=fopen(sfile, "r"))==(FILE *)NULL) {
                fprintf(stderr,"odb [main(%d)] - Warning. Cannot open ODB_INI file (%s): [%d] %s\n",
                        __LINE__, sfile, errno, strerror(errno) );
            } else {
                if ( ( line = (char *)malloc ( LINE_CHUNK ) ) == (void *) NULL ) {
                    fprintf(stderr, "odb [main(%d)] - Error Allocating LINE_CHUNK memory\n", __LINE__);
                        exit( EX_OSERR );
                }
                l = (int)strlen(etab[0].run);
                while ( fgets(line, LINE_CHUNK, fp ) ) {    /* skip to av[i] section */
                    if ( line[0] == '[' && !strncmp(&line[1], etab[0].run, l) && line[l+1] == ']' ) {
                        i = 0;
                        break;
                    }
                }
                if ( i )
                    fprintf(stderr, "odb [main(%d)] - Warning: Section [%s] not found in %s\n", __LINE__, etab[0].run, sfile);
            }
        }

        /* allocating intial memory for Ocmd */
        if ( (Ocmd = (SQLCHAR *)malloc ( bs ) ) == (void *)NULL ) {
                fprintf(stderr, "odb [main(%d)] - Error allocating Ocmd memory: [%d] %s\n",
                    __LINE__, errno, strerror(errno));
            free(line);
            exit( EX_OSERR );
        }

        /* Setting history file */
        strmcpy(hfile, getenv("ODB_HIST"), sizeof(hfile));
        if ( !hfile[0] ) {  /* ODB_HIST is empty */
#ifdef _WIN32
            strmcpy(hfile, getenv("HOMEDRIVE"), sizeof(hfile));
            strmcat(hfile, getenv("HOMEPATH"), sizeof(hfile), 0);
            strmcat(hfile, "\\_odbhist", sizeof(hfile), 0);
#else
            strmcpy(hfile, getenv("HOME"), sizeof(hfile));
            strmcat(hfile, "/.odbhist", sizeof(hfile), 0);
#endif
        }
        /* mreadline initialization */
        if ( mrinit() ) {               /* Initialize mreadline */
            fprintf(stderr, "odb [main(%d)] - Error during mreadline initialization\n", __LINE__);
            goto oint_exit;
        }
        if ( mrhinit(hist, hfile) ) {   /* Initialize mrhistory */
            fprintf(stderr, "odb [main(%d)] - Error during mrhistory initialization\n", __LINE__);
            goto oint_exit;
        }

        /* Clean sfile & Ocmd */
        sfile[0] = Ocmd[0] = '\0';

        /* Interpreter loop */
        while( (fp) || (line=mreadline(prompt, &ll)) != (char *)EOF ){
            if ( fp ) { /* line read from the ODB_INI file */
                if ((!fgets(line, LINE_CHUNK, fp)) || (line[0]=='[')) {
                    fclose(fp);
                    free(line);
                    fp = 0;
                    continue;
                } else if ( line[0] == '#' ) {
                    continue; /* skip comments */
                } else {
                    ll = (unsigned int) strlen ( line ) - 1;
                    line[ll] = '\0';    /* remove ending new line */
                    if (line[0])
                        printf("%s\n", line);
                }
                etab[0].flg &= ~0200000;    /* don't save history */
            } else {
                if ( etab[0].fso )
                    fprintf(etab[0].fso, "%s%s\n", strmprint(prompt), line);
                etab[0].flg |= 0200000; /* save history */
            }
            while ( ( j + ll + 1 ) > bs ) { /* we need more memory for Ocmd */
                bs += CMD_CHUNK;
                O = Ocmd ;
                if ( ( O = (SQLCHAR *)realloc ( O, bs ) ) == (void *)NULL ) {
                        fprintf(stderr, "odb [main(%d)] - Error re-allocating memory for Ocmd: [%d] %s\n",
                            __LINE__, errno, strerror(errno));
                    j = 0;
                    break;
                }
                Ocmd = O;
            }
            if ( line == (char *) (-2) ) {  /* CTRL-X */
                etab[0].flg &= ~0200000;    /* don't save history */
                nrag = j = 0;
                rl = 0 ;
            } else {
                j += strmcat ( (char *)Ocmd, line, bs, 0);
                nrag = tokenize ( line, (int)ll, rag );
                rl = strlen ( rag[0] );
                ll = (unsigned int) j;  /* save current command length */
            }

            /* alias substitution */
            if ( l && nrag ) {  /* don't check aliases on continuation lines */
                for ( vv = thps[0].tva; vv ; vv = vv->next ) {
                    if ( vv->type == VTYPE_A &&             /* Is an ALIAS */
                         vv->nlen == rl &&                  /* Alias name length = first command line word */
                         !strmicmp(rag[0], vv->name, rl)) { /* Alias name == first command line word */
                        mrhadd((char *)Ocmd, ll);   /* save command to history now... */
                        etab[0].flg &= ~0200000;    /* ... and not after alias expansion */
                        memset(Ocmd, 0, bs);            
                        for ( j = k = 0; vv->value[k]; k++ ) { /* build new Ocmd */
                            if ( vv->value[k] == '&' && vv->value[k+1] && vv->value[k+1] > 48 && vv->value[k+1] < 58 ) {
                                j += (int)strlen(rag[vv->value[++k]-48]);
                                (void) strmcat((char *)Ocmd, rag[vv->value[k]-48], bs, 0);
                            } else {
                                Ocmd[j++] = (SQLCHAR)vv->value[k];
                            }
                        }
                        if ( ( line = mresize ( (size_t)(j + 1) ) ) == (char *)NULL ) {
                            fprintf(stderr, "odb [main(%d)] - Error re-allocating memory for line: [%d] %s\n",
                                __LINE__, errno, strerror(errno));
                        } else {
                            ll = (unsigned int)j;
                            MEMCPY ( line, Ocmd , j + 1);   /* Copy Ocmd and the terminating NULL */
                            nrag = tokenize ( line, (int)ll, rag );
                        }
                        break;              /* we've found our alias */
                    }
                }
            }

            /* Commands for odb... */
            if ( nrag && l ) {
                if ( nrag == 1 && ( !strmicmp(rag[0], "h", 0) || !strmicmp(rag[0], "help", 0) ) ) {
                printf ("All the following are case insensitive:\n"
                    "  h | help                : print this help\n"
                    "  i | info                : print database info\n"
                    "  q | quit                : exit SQL Interpreter\n"
                    "  c | connect { no | [user[/pswd][;opts;...] (re/dis)connect using previous or new user\n"
                    "  odb odb_command         : will run an odb instance using the same DSN/credentials\n"
                    "  ls -[type] [pattern]    : list objects. Type=(t)ables, (v)iews, s(y)nonyns, (s)chemas\n"
                    "                          : (c)atalogs, syst(e)m tables, (l)ocal temp, (g)lobal temp\n"
                    "                          : (m)at views, (M)mat view groups, (a)lias, (A)ll object types\n"
                    "                          : (D)table DDL, (T)table desc\n"
                    "  print <string>          : print <string>\n"
                    "  !cmd                    : execute the operating system cmd\n"
                    "  @file [&0]... [&9]      : execute the sql script in file\n"
                    "  set                     : show all settings\n"
                    "  set alias [name] [cmd|-]: show/set/change/delete aliases\n"
                    "  set chsch [cmd]         : show/set change schema command\n"
                    "  set cols [#cols]        : show/set ls number of columns\n"
                    "  set cwd [<directory>]   : show/set current working directory\n"
                    "  set drs [on|off|\"SQL\"]  : show/enable/disable/run SQL describe result set mode\n"
                    "  set fs [<char>]         : show/set file field separator\n"
                    "  set hist [#lines]       : show/set lines saved in the history file\n"
                    "  set maxfetch [#rows]    : show/set max lines to be fetched (-1 = unlimited)\n"
                    "  set nocatalog [on|off]  : show/enable/disable \"no catalog\" database mode)\n"
                    "  set nocatnull [on|off]  : show/enable/disable \"no catalog as null\" database mode)\n"
                    "  set noschema [on|off]   : show/enable/disable \"no schema\" database mode)\n"
                    "  set nullstr [<string>|-]: show/set/delete nullstring\n"
                    "  set pad [fit|full|off]  : show/set column padding\n"
                    "  set param name [value|-]: show/set/change/delete a parameter\n"
                    "  set pcn [on|off]        : show/enable/disable printing column names\n"
                    "  set plm [on|off|\"SQL\"]  : show/enable/disable/run SQL print list mode (one col/row)\n"
                    "  set prepare [on|off]    : show/enable/disable 'prepare only' mode\n"
                    "  set prompt [string]     : show/set prompt string\n"
                    "  set query_timeout [s]   : show/set query timeout in seconds (def = 0 no timeout)\n"
                    "  set quiet [cmd|res|timing|all|off] : show/enable/disable quiet mode\n"
                    "  set rowset [#]          : show/set rowset used to fetch rows\n"
                    "  set soe [on|off]        : show/enable/disable Stop On Error mode\n"
                    "  set spool [<file>|off]  : show/enable/disable spooling output on <file>\n"
                    "  set ucs2toutf8 [<file>|off]  : show/enable/disable UCS-2 to UTF-8 conversion\n"
                    "  <SQL statement>;        : everything ending with ';' is sent to the database\n");
#ifndef _WIN32
                    printf("\nmreadline keys:\n"
                    "  Control-A  : move to beginning of line      Control-P  : history Previous\n"
                    "  Control-E  : move to end of line            Up Arrow   : history Previous\n"         
                    "  Control-B  : move cursor Back               Control-N  : history Next\n"
                    "  Left Arrow : move cursor Back               Down Arrow : history Next\n"
                    "  Control-F  : move cursor Forward            Control-W  ; history List\n"
                    "  Right Arrow: move cursor Forward            Control-R  : Redraw\n"
                    "  Control-D  : input end (exit) - DEL right   Control-V  : Edit current line\n"
                    "  Control-L  : Lowercase Line                 Control-X  : Kill line\n"
                    "  Control-U  : Uppercase Line                 #Control-G : load history entry #\n");
#endif
                    j = 0;
                } else if ( !strmicmp((char *)Ocmd, chsch, ssl) ) {
                    if ( rag[nrag-1][0] == ';' )
                        k = nrag - 2;
                    else
                        k = nrag - 1;
                    i = (int) strlen(rag[k]);
                    if ( rag[k][i - 1] == ';' || rag[k][i-1] == ',')
                        rag[k][--i] = '\0';
                    if ( f & 020000000 ) {
                        strmcpy(ccn, rag[k], sizeof(ccn));
                    } else {
                        for ( ; i && rag[k][i] != '.' ; i--); 
                        strmcpy(csn, &rag[k][i+(i?1:0)], sizeof(csn));
                        if ( i )
                            strmcpy(ccn, rag[k], i);
                    }
                    var_set ( &thps[0].tva, VTYPE_I, "catalog", ccn[0] ? (char *)ccn : "none" );
                    var_set ( &thps[0].tva, VTYPE_I, "schema", csn[0] ? (char *)csn : "none" );
                } else if ( !strmicmp(rag[0], "ls", 0) ) {
                    if ( etab[0].flg & 0100000 ) {
                        otype = 'A';
                        if ( nrag > 1 ) {
                            if ( rag[1][0] == '-' )
                                otype = rag[1][1];
                            if ( rag[nrag-1][0] == '.' && rag[nrag-1][1] == '\0' )
                                Ocso[1] = (SQLCHAR *)csn;
                            else if ( rag[nrag-1][0] != '-' )
                                splitcso(rag[nrag-1], Ocso, 1);
                        }
                        if ( otype == 's' ) {       /* list schemas */
                            Olist((SQLCHAR *)"", (SQLCHAR *)SQL_ALL_SCHEMAS, (SQLCHAR *)"", 's');
                        } else if ( otype == 'c' ) {    /* list catalogs */
                            Olist((SQLCHAR *)SQL_ALL_CATALOGS, (SQLCHAR *)"", (SQLCHAR *)"", 'c');
                        } else {                        /* all other objects */
                            Olist(( f & 0100000000) ? (SQLCHAR *)"":(Ocso[0]?Ocso[0]:(SQLCHAR *)ccn),
                                  ( f & 020000000) ? (SQLCHAR *)"":(Ocso[1]?Ocso[1]:(SQLCHAR *)csn), Ocso[2], otype);
                        }
                    } else {
                        fprintf(stderr, "odb [main(%d)] - No Database Connection\n", __LINE__);
                    }
                    Ocso[0] = Ocso[1] = Ocso[2] = 0;
                    j = 0;
                } else if ( !strmicmp(rag[0], "odb", 0) ) {
                    mrhadd((char *)Ocmd, ll);   /* save command to history now... */
                    etab[0].flg &= ~0200000;    /* ... and not after alias expansion */
                    while ( ol + ll > bs ) {
                        bs += CMD_CHUNK;
                        if ( ( Ocmd = (SQLCHAR *)realloc ( Ocmd, bs ) ) == (void *)NULL ) {
                            fprintf(stderr, "odb [main(%d)] - Error re-allocating memory for Ocmd: [%d] %s\n",
                                __LINE__, errno, strerror(errno));
                        }
                    }
                    strmcpy((char *)Ocmd, odbcl, bs);
                    for ( i = 1 ; i < nrag ; i++ ) {
                        strmcat((char *)Ocmd, " ", bs, 0);
                        strmcat((char *)Ocmd, rag[i], bs, 0);
                    }
#ifdef _WIN32
                    _spawnlp(_P_WAIT, "cmd.exe", "cmd.exe", "/c", Ocmd, NULL);
#else
                    if ( system((const char *)Ocmd) < 0 )
                        fprintf(stderr, "odb [main(%d)] - Error running %s\n", __LINE__, &Ocmd[1]);
#endif
                    j = 0;
                } else if ( !strmicmp(rag[0], "set", 0) ) {
                    setan ( 0, 0, nrag, rag, 0 );
                    j = 0;
                } else if ( !strmicmp(rag[0], "i", 0) || !strmicmp(rag[0], "info", 0) ) {
                    if ( etab[0].flg & 0100000 ) {
                        Odinfo();
                        mfprintf(stdout, etab[0].fso, "\t%-45s: %s\n", "Current Catalog", ccn);
                        mfprintf(stdout, etab[0].fso, "\t%-45s: %s\n", "Current Schema", csn);
                    } else {
                        fprintf(stderr, "odb [main(%d)] - No Database Connection\n", __LINE__);
                    }
                    j = 0;
                } else if ( !strmicmp(rag[0], "c", 0) || !strmicmp(rag[0], "connect", 0) ) {
                    if ( nrag == 2 && !strmicmp(rag[1], "NO", 2) ) {
                        if (!SQL_SUCCEEDED(Oret=SQLDisconnect(thps[0].Oc)))
                            Oerr(0, 0, __LINE__, thps[0].Oc, SQL_HANDLE_DBC);
                        etab[0].flg &= ~0100000;
                        thps[0].Os = 0;
                        thps[0].Oc = 0;
                    } else {
                        if ( nrag > 1 ) {
                            if ( rag[1][0] == ';' ) {
                                clca = &rag[1][1];
                            } else {
                                for ( i = 0, o = 0; rag[1][i] && rag[1][i] != '/' ; i++, o++ )
                                    Ouser[o] = (SQLCHAR) rag[1][i];
                                Ouser[i] = '\0';
                                for ( i++, o = 0; rag[1][i] && rag[1][i] != ';' ; i++, o++ )
                                    Opwd[o] = (SQLCHAR) rag[1][i];
                                Opwd[i] = '\0';
                                if ( rag[1][i] == ';' )
                                    clca = &rag[1][i+1];
                            }
                            (void) bcs ( Oics, sizeof(Oics), Ouser, Opwd, Odsn, clca, 0 );
                        }
                        if ( !thps[0].Oc )
                            if (!SQL_SUCCEEDED(Oret=SQLAllocHandle(SQL_HANDLE_DBC, Oenv, &thps[0].Oc)))
                                Oerr(0, 0, __LINE__, Oenv, SQL_HANDLE_ENV);
                        if (!SQL_SUCCEEDED(Oret=SQLDriverConnect(thps[0].Oc, (SQLHWND) NULL,
                                Oics, SQL_NTS, Oocs, (SQLSMALLINT) sizeof(Oocs),
                                &Oocsl, (SQLUSMALLINT)SQL_DRIVER_NOPROMPT))) {
                            Oerr(0, 0, __LINE__, thps[0].Oc, SQL_HANDLE_DBC);
                        } else {
                            etab[0].flg |= 0100000;
                            if (!SQL_SUCCEEDED(Oret=SQLAllocHandle(SQL_HANDLE_STMT,
                                thps[0].Oc, &thps[0].Os))){
                                Oerr(0, 0, __LINE__, thps[0].Oc, SQL_HANDLE_DBC);
                            }
                            var_set ( &thps[0].tva, VTYPE_I, "cs", (char *)Oocs );
                        }
                        etab[0].dbt = checkdb(0, &thps[0].Oc, ccn, csn);
                        if ( ( vv = var_idx ( &thps[0].tva, VTYPE_I, "schema" ) ) ) {
                            strmcpy(csn, vv->value, sizeof(csn));
                            vv = var_idx ( &thps[0].tva, VTYPE_I, "catalog" );
                            if ( vv && strmicmp(ccn, vv->value, 0) ) {
                                strmcpy(ccn, vv->value, sizeof(ccn));
                                snprintf((char *)Obuf[0], sizeof(Obuf[0]), "%s %s.%s", chsch, ccn, csn);
                            } else {
                                snprintf((char *)Obuf[0], sizeof(Obuf[0]), "%s %s", chsch, csn);
                            }
                            printf("Resetting schema: \"%s\"\n", (char *)Obuf[0]);
                            Oexec( 0, 0, -1, 0, Obuf[0], "");
                        }
                        var_set ( &thps[0].tva, VTYPE_I, "catalog", ccn[0] ? (char *)ccn : "unknown" );
                        var_set ( &thps[0].tva, VTYPE_I, "schema", ccn[0] ? (char *)csn : "unknown" );
                    }
                    j = 0;
                } else if ( !strmicmp(rag[0], "q", 0) || !strmicmp(rag[0], "quit", 0) ||
                            !strmicmp(rag[0], "exit", 0) ) {
                    break;
                } else if ( !strmicmp(rag[0], "print", 0 ) ) {
                    switch ( nrag ) {
                    case 1:
                        fprintf(stderr, "odb [main(%d)] - Not enough arguments (print what?)\n", __LINE__);
                        break;
                    default:
                        var_exp (rag[1], 0, &thps[0].tva);
                    }
                    j = 0;
                } else if ( rag[0][0] == '@' ) {
                    if ( etab[0].flg & 0100000 ) {
                        i = pad;            /* save pad setting */
                        pad = 0;    
                        (void)runsql(0, 0, -1, &rag[0][1]);
                        pad = (char)i;          /* reset pad setting */
                    } else {
                        fprintf(stderr, "odb [main(%d)] - No Database Connection\n", __LINE__);
                    }
                    j = 0;
                } else if ( rag[0][0] == '!' ) {
#ifdef _WIN32
                    _spawnlp(_P_WAIT, "cmd.exe", "cmd.exe", "/c", &Ocmd[1], NULL);
#else
                    if ( system((const char *)&Ocmd[1]) < 0 )
                        fprintf(stderr, "odb [main(%d)] - Error running %s\n", __LINE__, &Ocmd[1]);
#endif
                    j = 0;
                } 
            }

            /* Commands ending with ';' are sent to the database */
            if ( j ) {
                for ( i = j - 1; i && isspace(Ocmd[i]) ; i--); /* skip trailing blanks */
                if ( Ocmd[i] == ';' ) {
                    Ocmd = (SQLCHAR *)var_exp ( (char *)Ocmd, &bs, &thps[0].tva);
                    if ( !(etab[0].flg & 0100000) )     /* No Connection */
                        fprintf(stderr, "odb [main(%d)] - No Database Connection\n", __LINE__);
                    else
                        Omexec( 0, 0, -1, 0, Ocmd, "", ccn, csn);
                    j = 0;
                } else {
                    (void) strmcat((char *)Ocmd, "\n", bs, 0);
                    j++;
                }
            }
            if ( !j ) {
                l = 1;
                if ( etab[0].flg & 0200000 )
                    mrhadd((char *)Ocmd, ll);
                Ocmd[0] = '\0';
            }
            /* Setting prompt string */
            memset(prompt, 0, sizeof(prompt));
            c = tprompt;
            i = 0;
            while ( (ch = *c++) ) {
                if ( ch == '%' ) {
                    switch ( ( ch = *c++ ) ) {
                    case 'D':
                        i += strmcat(prompt, (char *)Odsn, sizeof(prompt), 0);
                        break;
                    case 'U':
                        i += strmcat(prompt, (char *)Ouser, sizeof(prompt), 0);
                        break;
                    case 'S':
                        i += strmcat(prompt, csn, sizeof(prompt), 0);
                        break;
                    case 'C':
                        i += strmcat(prompt, ccn, sizeof(prompt), 0);
                        break;
                    case 'M':
                        i += 3;
                        if ( j ) { /* command continuing on the next line */
                            l = 0;
                            (void) strmcat(prompt, "...", sizeof(prompt), 0);
                        } else if ( !(etab[0].flg & 0100000) ) {
                            (void) strmcat(prompt, "NDC", sizeof(prompt), 0);
                        } else if ( etab[0].fso ) {
                            (void) strmcat(prompt, "SPO", sizeof(prompt), 0);
                        } else if ( etab[0].flg & 0010 ) {
                            (void) strmcat(prompt, "PRE", sizeof(prompt), 0);
                        } else if ( f & 0400000 ) {
                            (void) strmcat(prompt, "QUI", sizeof(prompt), 0);
                        } else {
                            (void) strmcat(prompt, "SQL", sizeof(prompt), 0);
                        }
                        break;
                    case 'T':
                        if ( time(&ptime) < 0 ) {
                            strmcpy(nows, "Time N.A.", sizeof(nows));
                        } else {
                            nowt = localtime(&ptime);
                            if (!strftime(nows, sizeof(nows), "%H:%M:%S", nowt))
                                strmcpy(nows, "Time N.A.", sizeof(nows));
                        }
                        i += strmcat(prompt, nows, sizeof(prompt), 0);
                        break;
                    }
                } else {
                    prompt[i++] = (char)ch;
                }
            }
        }
        mrhsave(hfile);         /* save history file */
        oint_exit:
        free(Ocmd);             /* Free Ocmd */
        if (etab[0].fso)        /* spool not closed */
            fclose(etab[0].fso);
        mrend();                /* End mreadline */
        printf("Bye\n");
    }

    if ( f & 00200 ) {  /* [-x / -f / -P / -S / -l / -e ] */
        if ( f & 00400 ) {  /* csv output */
            gettimeofday(&tvs, (void *)NULL);       /* register start time */
            ptime = (time_t) tvs.tv_sec; /* WIN localtime requires time_t */            
            if ( f & 01000000 ) {
                nowt = localtime(&ptime);
                if (strftime(nows, sizeof(nows), "%Y-%m-%d %H:%M:%S", nowt))
                    printf("Thread id [%s]%c", nows, fs);
                else
                    printf("Thread id [start time N.A.]%c", fs);
            } else {
                nowt = localtime(&ptime);
                strftime(nows, sizeof(nows), "%Y-%m-%d %H:%M:%S", nowt);
                fprintf(stderr, "odb [%s]: starting (%d) threads...\n", nows, tn);
                printf("Thread id%c", fs);
            }
            printf("Proc id%cThread Exec#%cScript Cmd#%c", fs, fs, fs);
            printf("File%cLabel%cCommand%cRows%cRsds%cPrepare(s)%c", fs, fs, fs, fs, fs, fs);
            printf("Exec(s)%c1st Fetch(s)%cFetch(s)%cTotal(s)%cSTimeline%cETimeline\n",
                fs, fs, fs, fs, fs);
            fflush(stdout);
        }
        if ( tn == 1 ) {    /* just one command OR serial execution: no threads */

            (void)signal(SIGINT, sigcatch);         /* Keyboard Ctrl-C */
            (void)signal(SIGTERM, sigcatch);        /* Software termination (kill) */
#ifndef _WIN32
            (void)signal(SIGALRM, sigcatch);        /* timeout alarm */
            if ( w )
                alarm ( w );
#endif
            f |= 010000000;
            (void)Oruncmd((void *)&etab[0].id);
        } else {
#ifdef _WIN32
            for ( i = 0; i < tn; i++ ){
                if ( i && d ) 
                    Sleep (d);
                if( ( thhn[i] = CreateThread(NULL, 0, Oruncmd, (void *)&thps[i].tid,
                        0, NULL)) == (HANDLE)NULL ) {
                    fprintf(stderr, "odb [main(%d)] - Error starting cmd thread %d: [%d] %s\n",
                        __LINE__, i, errno, strerror(errno));
                    exit( EX_OSERR );
                }
            }
            (void)signal(SIGINT, sigcatch);                     /* Keyboard Ctrl-C */
            (void)signal(SIGTERM, sigcatch);                    /* Software termination (kill) */
            WaitForMultipleObjects(i, thhn, TRUE, INFINITE);    /* wait threads */
            for ( i = 0 ; i < tn ; i++)                         /* close thread handles */
                CloseHandle(thhn[i]);
#else

            for ( i = 0 ; i < tn ; i++ ) {
                if ( i && d )
                    Sleep (d);
                if( ( k = pthread_create(&thid[i], NULL,
                        Oruncmd, (void *) &thps[i].tid) ) ) {
                    fprintf(stderr, "odb [main(%d)] - Error starting thread %d: [%d] %s\n", 
                        __LINE__, i, errno, strerror(errno));
                    exit( EX_OSERR );
                }
            }
            (void)signal(SIGALRM, sigcatch);                /* timeout alarm */
            (void)signal(SIGINT, sigcatch);                 /* Keyboard Ctrl-C */
            (void)signal(SIGTERM, sigcatch);                /* Software termination (kill) */
            if ( w )
                alarm ( w );
            for ( i = 0 ; i < tn ; i++) {                   /* wait threads */
                if ( ( k = pthread_join(thid[i], (void *)&thres) ) ) {
                    fprintf(stderr, "odb [main(%d)] - Error joining thread %d: [%d] %s\n",
                        __LINE__, i, errno, strerror(errno));
                    exit( EX_OSERR );
                }
            }
#endif
        }
        if ( !(f & 01000000) && ( f & 00400 ) ) {   /* -c (csv output) and not -b */
            gettimeofday(&tvn, (void *)NULL);       /* register end time */
            printf("%s statistics:\n", odbid);
            ptime = (time_t)tvi.tv_sec; /* WIN localtime() requires time_t input */
            nowt = localtime(&ptime);
            if (strftime(nows, sizeof(nows), "%Y-%m-%d %H:%M:%S", nowt))
                printf("\tInit timestamp: %s\n", nows);
            else
                printf("\tInit timestamp N.A.\n");
            ptime = (time_t)tvs.tv_sec; /* WIN localtime() requires time_t input */
            nowt = localtime(&ptime);
            if (strftime(nows, sizeof(nows), "%Y-%m-%d %H:%M:%S", nowt))
                printf("\tStart timestamp: %s\n", nows);
            else
                printf("\tStart timestamp N.A.\n");
            ptime = (time_t)tvn.tv_sec; /* WIN localtime() requires time_t input */
            nowt = localtime(&ptime);
            if (strftime(nows, sizeof(nows), "%Y-%m-%d %H:%M:%S", nowt))
                printf("\tEnd timestamp: %s\n", nows);
            else
                printf("\tEnd timestamp N.A.\n");
            printf("\tElapsed [Start->End] (s): %.3f\n",
                (double)(tvn.tv_sec-tvs.tv_sec)+(double)(tvn.tv_usec-tvs.tv_usec)/1000000.0);
            fflush(stdout);
        }
    }

    if ( thps[0].Oc && !thps[0].cr ) {
        if (!SQL_SUCCEEDED(Oret=SQLDisconnect(thps[0].Oc)))
            Oerr(0, 0, __LINE__, thps[0].Oc, SQL_HANDLE_DBC);
    }

    gettimeofday(&tvn, (void *)NULL);       /* register end time */
    ptime = (time_t)tvn.tv_sec; /* WIN localtime() requires time_t input */     
    nowt = localtime(&ptime);
    strftime(nows, sizeof(nows), "%Y-%m-%d %H:%M:%S", nowt);
    seconds = (double)(tvn.tv_sec-tvi.tv_sec+(tvn.tv_usec-tvi.tv_usec)/1000000.0);
    fprintf(stderr, "odb [%s]: exiting. Session Elapsed time %s seconds (%s)\n", nows,
        strmnum(seconds, num, sizeof(num), 3), strmtime(seconds, tim));

    gclean();
    exit( exstat );
}

/* setan: change/print settings
 *
 * eid(I): etab[] index
 * tid(I): thps[] index
 * nrag(I): number of arguments
 * rag(I): arguments array pointer
 * ql(I): qlabel pointer
 *
 * return: void
 */
static void setan ( int eid, int tid, int nrag, char *rag[], char *ql )
{
    int i = 0;              /* loop variable */
    unsigned int ul = 0;    /* used to park new nullstr length */

    if ( nrag == 1 || !strmicmp(rag[1], "alias", 0) ) {
        switch ( nrag ) {
        case 1:
        case 2:
            for ( vv = thps[tid].tva; vv ; vv = vv->next )
                if ( vv->type == VTYPE_A )
                    fprintf(stderr, "set alias \"%s\" is \"%s\"\n", vv->name, vv->value);
            break;
        case 3:
            vv = var_idx ( &thps[tid].tva, VTYPE_A, rag[2] );
            fprintf(stderr, "set alias \"%s\" is \"%s\"\n", rag[2], vv ? vv->value : "not set");
            break;
        case 4:
            if ( rag[3][0] == '-' )
                var_del ( &thps[tid].tva, VTYPE_A, rag[2] );
            else
                var_set ( &thps[tid].tva, VTYPE_A, rag[2], rag[3] );
            break;
        }
    }
    if ( nrag == 1 || !strmicmp(rag[1], "chsch", 0) ) {
        switch ( nrag ) {
        case 1:
        case 2:
            fprintf(stderr, "set chsch (change schema cmd) is %s\n", chsch);
            break;
        case 3:
            strmcpy(chsch, rag[2], sizeof(chsch));
            ssl = (int) strlen(chsch);
            break;
        }
    } 
    if ( nrag == 1 || !strmicmp(rag[1], "cwd", 0) ) {
        switch ( nrag ) {
        case 1:
        case 2:
            if ( !getcwd(cwd, sizeof(cwd)) ) 
                fprintf(stderr, "odb [main(%d)] - Cannot get cwd: [%d] %s\n",
                    __LINE__, errno, strerror(errno));
            else 
                fprintf(stderr, "set cwd is %s\n", cwd );
            break;
        case 3:
            if ( ( i = chdir(rag[2]) ) == -1 ) 
                fprintf(stderr, "odb [main(%d)] - Cannot chdir: [%d] %s\n",
                    __LINE__, errno, strerror(errno));
            break;
        }
    } 
    if ( nrag == 1 || !strmicmp(rag[1], "drs", 0) ) {
        switch ( nrag ) {
        case 1:
        case 2:
                fprintf(stderr, "set drs (describe result set) is %s\n", ( etab[eid].flg & 0400000 ) ? "on" : "off" );
            break;
        case 3:
            if ( !strmicmp(rag[2], "on", 0) ) {
                etab[eid].flg |= 0400000;
            } else if ( !strmicmp(rag[2], "off", 0) ) {
                etab[eid].flg &= ~0400000;
            } else {
                etab[eid].flg |= 0400000;
                (void)Oexec( tid, eid, -1, 0, (SQLCHAR *)rag[2], "");
                etab[eid].flg &= ~0400000;
            }
            break;
        }
    } 
    if ( nrag == 1 || !strmicmp(rag[1], "fs", 0) ) {
        switch ( nrag ) {
        case 1:
        case 2:
            fprintf(stderr, "set fs (field separator) is \"%c\"\n", etab[eid].fs);
            break;
        case 3:
            etab[eid].fs = rag[2][0];
            break;
        }
    } 
    if ( nrag == 1 || !strmicmp(rag[1], "maxfetch", 0) ) {
        switch ( nrag ) {
        case 1:
        case 2:
            fprintf(stderr, "set maxfetch (max rows fetched) is %lu%s\n", 
                etab[eid].mr, etab[eid].mr ? "" : " (unlimited)");
            break;
        case 3:
            etab[eid].mr = atoi(rag[2]);
            if ( etab[eid].r > (size_t) etab[eid].mr )
                etab[eid].r = (size_t) etab[eid].mr;
            break;
        }
    } 
    if ( nrag == 1 || !strmicmp(rag[1], "nocatalog", 0) ) {
        switch ( nrag ) {
        case 1:
        case 2:
            fprintf(stderr, "set nocatalog (\"no catalog\" mode) is %s\n",
                ( f & 0100000000 ) ? "on" : "off");
            break;
        case 3:
            if ( !strmicmp(rag[2], "on", 0) )
                f |= 0100000000;
            else if ( !strmicmp(rag[2], "off", 0) )
                f &= ~0100000000;
            break;
        }
    } 
    if ( nrag == 1 || !strmicmp(rag[1], "nocatnull", 0) ) {
        switch ( nrag ) {
        case 1:
        case 2:
            fprintf(stderr, "set nocatnull (\"no catalog as null\" mode) is %s\n",
                ( f & 04100000000 ) ? "on" : "off");
            break;
        case 3:
            if ( !strmicmp(rag[2], "on", 0) )
                f |= 04100000000;
            else if ( !strmicmp(rag[2], "off", 0) )
                f &= ~04100000000;
            break;
        }
    } 
    if ( nrag == 1 || !strmicmp(rag[1], "noschema", 0) ) {
        switch ( nrag ) {
        case 1:
        case 2:
            fprintf(stderr, "set noschema (\"no schema\" mode) is %s\n",
                ( f & 020000000 ) ? "on" : "off");
            break;
        case 3:
            if ( !strmicmp(rag[2], "on", 0) )
                f |= 020000000;
            else if ( !strmicmp(rag[2], "off", 0) )
                f &= ~020000000;
            break;
        }
    } 
    if ( nrag == 1 || !strmicmp(rag[1], "ucs2toutf8", 0) ) {
        switch ( nrag ) {
        case 1:
        case 2:
            fprintf(stderr, "set ucs2toutf8 is %s\n",
                ( f & 010000000 ) ? "on" : "off");
            break;
        case 3:
            if ( !strmicmp(rag[2], "on", 0) )
                f |= 010000000;
            else if ( !strmicmp(rag[2], "off", 0) )
                f &= ~010000000;
            break;
        }
    } 
    if ( nrag == 1 || !strmicmp(rag[1], "plm", 0) ) {
        switch ( nrag ) {
        case 1:
        case 2:
            fprintf(stderr, "set plm (print one column/row) is %s\n", ( etab[eid].flg & 040000 ) ? "on" : "off" );
            break;
        case 3:
            if ( !strmicmp(rag[2], "on", 0) ) {
                etab[eid].flg |= 040000;
            } else if ( !strmicmp(rag[2], "off", 0) ) {
                etab[eid].flg &= ~040000;
            } else {    /*run cmd in plm mode */
                etab[eid].flg |= 040000;
                (void)Oexec( tid, eid, -1, 0, (SQLCHAR *)rag[2], "");
                etab[eid].flg &= ~040000;
            }
            break;
        }
    } 
    if ( nrag == 1 || !strmicmp(rag[1], "param", 3) ) {
        switch ( nrag ) {
        case 1:
        case 2:
            for ( vv = thps[tid].tva; vv ; vv = vv->next ) {
                if ( vv->type == VTYPE_U )
                    fprintf(stderr, "set (user) param \"%s\" is \"%s\"\n", vv->name, vv->value);
                else if ( vv->type == VTYPE_I )
                    fprintf(stderr, "set (internal) param \"%s\" is \"%s\"\n", vv->name, vv->value);
            }
            break;
        case 3:
            vv = var_idx ( &thps[tid].tva, VTYPE_U, rag[2] );
            fprintf(stderr, "set (user) param \"%s\" is \"%s\"\n", rag[2], vv ? vv->value : "not set");
            if ( ( vv = var_idx ( &thps[tid].tva, VTYPE_I, rag[2] ) ) ) 
                fprintf(stderr, "set (internal) param \"%s\" is \"%s\"\n",
                    rag[2], vv ? vv->value : "not set");
            break;
        case 4:
            if ( rag[3][0] == '-' )
                var_del ( &thps[tid].tva, VTYPE_U, rag[2] );
            else
                var_set ( &thps[tid].tva, VTYPE_U, rag[2], rag[3] );
            break;
        }
    } 
    if ( nrag == 1 || !strmicmp(rag[1], "pcn", 0) ) {
        switch ( nrag ) {
        case 1:
        case 2:
            fprintf(stderr, "set pcn (print column names) is %s\n", etab[eid].flg & 0200 ? "on" : "off" );
            break;
        case 3:
            if ( !strmicmp(rag[2], "on", 0) ) {
                etab[eid].flg |= 0200;
            } else if ( !strmicmp(rag[2], "off", 0) ) {
                etab[eid].flg &= ~0200;
            }
            break;
        }
    } 
    if ( nrag == 1 || !strmicmp(rag[1], "prepare", 0) ) {
        switch ( nrag ) {
        case 1:
        case 2:
            fprintf(stderr, "set prepare is %s\n", etab[eid].flg & 0010 ? "on" : "off" );
            break;
        case 3:
            if ( !strmicmp(rag[2], "on", 0) )
                etab[eid].flg |= 0010;
            else if ( !strmicmp(rag[2], "off", 0) )
                etab[eid].flg &= ~0010;
            break;
        }
    } 
    if ( nrag == 1 || !strmicmp(rag[1], "query_timeout", 0) ) {
        switch ( nrag ) {
        case 1:
        case 2:
            fprintf(stderr, "set query_timeout is \"%ld\"\n", (long)Oqt);
            break;
        case 3:
            Oqt = (SQLULEN) atol(rag[2]);
            if (!SQL_SUCCEEDED(Oret=SQLSetStmtAttr(thps[tid].Os, SQL_ATTR_QUERY_TIMEOUT,
                    &Oqt, SQL_IS_UINTEGER))) {
                Oerr(0, 0, __LINE__, thps[tid].Os, SQL_HANDLE_STMT);
            }
            break;
        }
    } 
    if ( nrag == 1 || !strmicmp(rag[1], "quiet", 0) ) {
        switch ( nrag ) {
        case 1:
        case 2:
            if ( etab[eid].flg & 030000 && etab[eid].flg2 & 0004 ) {
                fprintf(stderr, "set quiet is all\n");
            } else if ( !(etab[eid].flg & 030000) && !(etab[eid].flg2 & 0004) ) {
                fprintf(stderr, "set quiet is off\n");
            } else {
                if ( etab[eid].flg & 010000 )
                    fprintf(stderr, "set quiet is cmd\n");
                if ( etab[eid].flg & 020000 )
                    fprintf(stderr, "set quiet is res\n");
                if ( etab[eid].flg2 & 0004 )
                    fprintf(stderr, "set quiet is timing\n");
            }
            break;
        case 3:
            if ( !strmicmp(rag[2], "off", 0) ) {
                etab[eid].flg &= ~030000;   /* switch all off */
                etab[eid].flg2 &= ~0004;    /* switch all off */
            } else if ( !strmicmp(rag[2], "cmd", 0) ) {
                etab[eid].flg |= 010000;    /* switch cmd on */
            } else if ( !strmicmp(rag[2], "res", 0) ) {
                etab[eid].flg |= 020000;    /* switch res on */
            } else if ( !strmicmp(rag[2], "timing", 0) ) {
                etab[eid].flg2 |= 0004;     /* switch timing on */
            } else if ( !strmicmp(rag[2], "all", 0) ) {
                etab[eid].flg |= 030000;    /* switch all on */
                etab[eid].flg2 |= 0004;     /* switch all on */
            }
            break;
        }
    } 
    if ( nrag == 1 || !strmicmp(rag[1], "rowset", 0) ) {
        switch ( nrag ) {
        case 1:
        case 2:
            if ( etab[eid].r )
                fprintf(stderr, "set rowset (insert/fetch rowset) is %ld\n",(long int) etab[eid].r);
            else
                fprintf(stderr, "set rowset (insert/fetch buffer size) is %ld\n",(long int) etab[eid].rbs);
            break;
        case 3:
            switch ( rag[2][0] ) {
            case 'k':
            case 'K':
                etab[eid].r = 0 ;
                etab[eid].rbs = 1024 * atoi (&rag[2][1]);
                break;
            case 'm':
            case 'M':
                etab[eid].r = 0 ;
                etab[eid].rbs = 1024 * 1024 * atoi (&rag[2][1]);
                break;
            default:
                etab[eid].r = (size_t) atoi(rag[2]);
                etab[eid].rbs = 0;
            }
            break;
        }
    } 
    if ( nrag == 1 || !strmicmp(rag[1], "soe", 0) ) {
        switch ( nrag ) {
        case 1:
        case 2:
            fprintf(stderr, "set soe (Stop On Error) is %s\n", etab[eid].flg & 0004 ? "on" : "off" );
            break;
        case 3:
            if ( !strmicmp(rag[2], "on", 0) )
                etab[eid].flg |= 0004 ;
            else if ( !strmicmp(rag[2], "off", 0) )
                etab[eid].flg &= ~0004 ;
            break;
        }
    } 
    if ( nrag == 1|| !strmicmp(rag[1], "spool", 0) ) {
        switch ( nrag ) {
        case 1:
        case 2:
            if ( etab[eid].fso )
                fprintf(stderr, "set spool is on %s\n", sfile );
            else
                fprintf(stderr, "set spool is off\n");
            break;
        case 3:
            if ( !strmicmp(rag[2], "off", 0) ) {
                fclose(etab[eid].fso);
                etab[eid].fso = 0;
            } else {
                strmcpy(sfile, rag[2], sizeof(sfile));
                if ( ( etab[eid].fso = fopen(sfile, "a") ) == (FILE *)NULL )
                    fprintf(stderr, "odb [main(%d)] - Cannot open %s: [%d] %s\n",
                        __LINE__, sfile, errno, strerror(errno));
            }
            break;
        }
    } 
    if ( nrag == 1 || !strmicmp(rag[1], "nullstr", 0) ) {
        switch ( nrag ) {
        case 1:
        case 2:
            fprintf(stderr, "set nullstr is \'%s\'\n", etab[eid].ns ? etab[eid].ns : "not set");
            break;
        case 3:
            ul = etab[eid].nl;      /* save current nullstring length */
            etab[eid].nl = (unsigned int)strlen(rag[2]);
            if ( etab[eid].nl == 1 && rag[2][0] == '-' ) {  /* clean nullstr */
                etab[eid].nl = 0;
                free ( etab[eid].ns );
                etab[eid].ns = '\0';
            } else {
                if ( ul < etab[eid].nl ) {  /* we need more memory for our nullstring */
                    if ( ( etab[eid].ns = realloc ( etab[eid].ns, etab[eid].nl + 1 ) ) == (void *)NULL ) {
                        fprintf(stderr, "[%d] odb [setan(%d)] - Error re-allocating nullstr memory: [%d] %s\n",
                            tid, __LINE__, errno, strerror(errno));
                        return;
                    }
                }
                strmcpy(etab[eid].ns, rag[2], (size_t)etab[eid].nl);
            }
            break;
        }
    }
    if ( f & 02000 ) {  /* [-I] Interpreter only flags */
        if ( nrag == 1 || !strmicmp(rag[1], "casesens", 0) ) {
            switch ( nrag ) {
            case 1:
            case 2:
                fprintf(stderr, "set casesens (Case Sensitive DB) is %s\n", f & 010000000000 ? "on" : "off" );
                break;
            case 3:
                if ( !strmicmp(rag[2], "on", 0) )
                    f |= 010000000000;
                else if ( !strmicmp(rag[2], "off", 0) )
                    f &= ~010000000000;
                break;
            }
        } 
        if ( nrag == 1 || !strmicmp(rag[1], "cols", 0) ) {
            switch ( nrag ) {
            case 1:
            case 2:
                fprintf(stderr, "set cols is %d\n", nc);
                break;
            case 3:
                nc = atoi(rag[2]);
                break;
            }
            cs = (int)(mrcol / nc);
        } 
        if ( nrag == 1 || !strmicmp(rag[1], "hist", 0) ) {
            switch ( nrag ) {
            case 1:
            case 2:
                fprintf(stderr, "set hist (lines saved in the history file) is %d\n", hist);
                break;
            case 3:
                hist = atoi(rag[2]);
                break;
            }
        } 
        if ( nrag == 1 || !strmicmp(rag[1], "pad", 0) ) {
            switch ( nrag ) {
            case 1:
            case 2:
                fprintf(stderr, "set pad (pad columns to display size) is %s\n",
                        pad == 0 ? "off" : ( pad == 1 ? "full" : "fit" ) );
                break;
            case 3:
            case 4:
                if ( !strmicmp(rag[2], "fit", 0) ) {
                    pad = 2;
                } else if ( !strmicmp(rag[2], "full", 0) ) {
                    pad = 1;
                } else if ( !strmicmp(rag[2], "off", 0) ) {
                    ul = pad ;  /* save old pad setting */
                    pad = 0;
                    if ( nrag == 4 ) {
                        (void)Oexec( tid, eid, -1, 0, (SQLCHAR *)rag[3], "");
                        pad = ul ;  /* reset pad */
                    }
                }
                break;
            }
        } 
        if ( nrag == 1 || !strmicmp(rag[1], "prompt", 0) ) {
            switch ( nrag ) {
            case 1:
            case 2:
                fprintf(stderr, "set prompt is \"%s\"\n", tprompt);
                break;
            case 3:
                strmcpy(tprompt, rag[2], sizeof(tprompt));
                break;
            }
        } 
    } else {    /* script only flags */
        if ( !strmicmp(rag[1], "ttime", 0) ) {
            thps[tid].cd = (unsigned int)strtol(rag[2], (char **)NULL, 10);
            for ( i = 0; rag[2][i] && rag[2][i] != ':'; i++);                   
            thps[tid].cd2 = (unsigned int)strtol(&rag[2][i+1], (char **)NULL, 10);
        } else if ( !strmicmp(rag[1], "qlabel", 0) ) {
            strmcpy ( ql, rag[2], QLABEL_LEN );
        }
    }
}

/* Oerr:
 *      print ODBC error stack related to a given HANDLE
 *
 *      eid: etab[] entry index
 *      tid: thread id
 *      line: line number where the Oerr() call occurred
 *      Ohandle: ODBC handle
 *      Otype: ODBC handle type
 *
 *      return: void
 */
static void Oerr(int eid, int tid, unsigned int line, SQLHANDLE Ohandle, SQLSMALLINT Otype)
{
    size_t bs=ERR_MSG_LEN;   /* Memory needed for the error message */
    SQLSMALLINT Oi=1, 
                Oln=0;
    SQLINTEGER Onat=0;
    SQLCHAR Ostate[6],
            *O,
            *Otxt;
    SQLRETURN Orv;
    time_t errtime = 0;     /* error time */
    struct timeval tverr;   /* error timeval struct */
    struct tm *errt;        /* date/time struct */
    char errts[20];         /* string with date/time in YYYY-MM-DD HH:MM:SS format*/

    /* Initial memory allocation for the error message buffer */
    if ( ( Otxt = malloc(bs) ) == (void *)NULL ) {
        fprintf(stderr, "[%d] odb [Oerr(%d)] - Error allocating err msg buffers: [%d] %s\n",
            tid, __LINE__, errno, strerror(errno));
        return;
    }

    /* Get current time */
    gettimeofday(&tverr, (void *)NULL);
    errtime = (time_t)tverr.tv_sec;
    errt = localtime(&errtime);
    strftime(errts, sizeof(errts), "%Y-%m-%d %H:%M:%S", errt);

    /* Loop through ODBC error stack */
    while ( ( Orv = SQLGetDiagRec(Otype, Ohandle, Oi++, Ostate, &Onat, Otxt,
            (SQLSMALLINT)bs, &Oln) ) != SQL_NO_DATA ) {
        if ( (size_t)Oln > bs ) {   /* error message buffer was too small */
            bs = (size_t) ( Oln + 1 );
            O = Otxt;
            if ( ( O = realloc ( O, bs ) ) == (void *)NULL ) {
                fprintf(stderr, "[%d] odb [Oerr(%d)] - Error re-allocating memory for err msg buff: [%d] %s\n",
                    tid, __LINE__, errno, strerror(errno));
                    continue;
            }
            Otxt = O;
            if ( ( Orv = SQLGetDiagRec(Otype, Ohandle, (Oi - 1) , Ostate, &Onat, Otxt,
                (SQLSMALLINT)bs, &Oln) ) != SQL_SUCCESS ) {
                fprintf(stderr, "[%d] odb [Oerr(%d)] - Error %d getting errmsg with bigger buff of size %d\n",
                    tid, __LINE__, Orv, (int)bs);
                break;
            }
        }
        mfprintf(stderr, eid < 0 ? (FILE *)NULL : etab[eid].fso, "[%d] odb(%d) [%s] - %s (State: %s Native Err: %ld)\n",
                        tid, line, errts, (char *)Otxt, (char *)Ostate, (long) Onat);
        if ( Orv < 0 ) {
            fprintf(stderr, "[%d] odb [Oerr(%d)] - Error %d getting the error message\n", tid, __LINE__, Orv);
            break;
        }
    }
    fflush(stderr);
    free(Otxt);
}

/* Omexec:
 *      Expand command looking for multi-objects and call Oexec for each object
 *      tid: Thread id;
 *      eid: Execution Table Process id (idx in the etab structure array)
 *      ten: Thread Execution no (progr # of cmd/script executed for a specific thread);
 *           (SQL Intepreter uses -1)
 *      scn: Script Command no (progr # of cmd executed in a given script);
 *      Ocmd: command to execute
 *      label: Query label 
 *      dcat: default catalog
 *      dsch: default schema
 *
 *      return: number of executed commands, (-1) if soe and errors
 */
static int Omexec(int tid, int eid, int ten, int scn, SQLCHAR *Ocmd, char *label, char *dcat, char *dsch)
{
    unsigned int i=0,           /* loop variable */
        k=0,                    /* loop variable */
        j=0;                    /* loop variable */
    int ret=0;                  /* return value */
    size_t ll, bl;              /* Ocmd length */
    SQLCHAR *Otype = 0,         /* ODBC Object Type: see struct Otypes[] definition */
            *Oncmd = 0,         /* New (expanded) command */
            Oname[MAXOBJ_LEN],  /* ODBC object name */
            Ostr[MAXOBJ_LEN],   /* ODBC string to expand */
            *Ocso[3];           /* ODBC array of catalog/schema/object pointers */
    SQLLEN Onamel;              /* ODBC object name length */
    SQLRETURN Or=0;             /* ODBC return value */
    SQLHSTMT Ostmt = 0;         /* Statement Handle */

    /* Clean Ostr */
    Ostr[0] = '\0';

    /* look for expansion string */
    bl = strlen((char *)Ocmd);
    for ( i = 1; i < ( bl - 2 ); i++) {
        if ( Ocmd[i] == '&' && Ocmd[i+2] == ':' && isspace(Ocmd[i-1]) ) {
            for ( k = 0 ; Ocmd[i+k] != ';' && Ocmd[i+k] && !isspace(Ocmd[i+k]) ; k++ )
                Ostr[k] = Ocmd[i+k];
            Ostr[k] = '\0';
            break;
        }
    }

    /* if nothing to expand */
    if ( !Ostr[0] )
        return ( Oexec(tid, eid, ten, scn, Ocmd, label) );

    /* Object type expansion */
    for ( j = 0 ; j < sizeof(Otypes)/sizeof(struct Otype) ; j++ )
        if ( Ostr[1] == Otypes[j].type )
            Otype = Otypes[j].Otype;
    if ( !Otype ) {     /* type not found */
        fprintf(stderr, "odb [Omexec(%d)] - Invalid object type >%c<\n", 
            __LINE__, (char)Ostr[1]);
        ret = -1;
        exstat = EX_USAGE ;
        goto omexec_exit;
    }

    /* Allocate statement handle */
    if (!SQL_SUCCEEDED(Or=SQLAllocHandle(SQL_HANDLE_STMT,
            thps[tid].Oc, &Ostmt))){
        Oerr(0, 0, __LINE__, thps[tid].Oc, SQL_HANDLE_DBC);
        ret = -1;
        exstat = EX_ODBCERR ;
        goto omexec_exit;
    }

    /* Allocate memory for the expanded command */
    if ( ( Oncmd = malloc ( ( ll = bl + MAXOBJ_LEN ) ) ) == (void *)NULL ) {
        fprintf(stderr, "odb [Omexec(%d)] - Error allocating memory for Oncmd\n", __LINE__);
        ret = -1;
        exstat = EX_OSERR ;
        goto omexec_exit;
    }

    /* look for catalogs/schema/object */
    splitcso ( (char *)&Ostr[3], Ocso, 1);

    /* list objects */
    if (!SQL_SUCCEEDED(Or=SQLTables(Ostmt,
        ( f & 0100000000) ? (SQLCHAR *)"": ( Ocso[0] ? Ocso[0] : (SQLCHAR *)dcat ), SQL_NTS, 
        ( f & 020000000) ? (SQLCHAR *)"": ( Ocso[1] ? Ocso[1] : (SQLCHAR *)dsch), SQL_NTS,
        Ocso[2], SQL_NTS, Otype , SQL_NTS ))) {
        Oerr(0, 0, __LINE__, Ostmt, SQL_HANDLE_STMT);
        ret = -1;
        exstat = EX_ODBCERR ;
        goto omexec_exit;
    }
        
    /* Bind object name */
    if (!SQL_SUCCEEDED(Or = SQLBindCol(Ostmt, (SQLUSMALLINT) 3,
            SQL_C_CHAR, Oname, (SQLLEN)sizeof(Oname), &Onamel))) {
        Oerr(0, 0, __LINE__, Ostmt, SQL_HANDLE_STMT);
        ret = -1;
        exstat = EX_ODBCERR ;
        goto omexec_exit;
    }
        
    /* Fetch object name and run */
    for ( j = 0; SQL_SUCCEEDED(Or=SQLFetch(Ostmt)) ; j++ ) {
        MEMCPY ( Oncmd, Ocmd, i);
        Oncmd[i] = '\0';
        if ( Ocso[0] && !(f & 0100000000) ) {
            (void) strmcat((char *)Oncmd, (char *)Ocso[0], ll, 0);
            (void) strmcat((char *)Oncmd, ".", ll, 0);
        }
        if ( Ocso[1] && !(f & 020000000) ) {
            (void) strmcat((char *)Oncmd, (char *)Ocso[1], ll, 0);
            (void) strmcat((char *)Oncmd, ".", ll, 0);
        }
        (void) strmcat((char *)Oncmd, (char *)Oname, ll, 0);
        (void) strmcat((char *)Oncmd, (char *)(Ocmd+i+k), ll, 0);
        if ( ! ( etab[eid].flg & 010000 ) )     /* if not quiet cmd on */
            mfprintf(stderr, etab[eid].fso, "[%d.%d] Command expanded to: \'%s\'\n", 
                tid, j, Oncmd);
        ret = Oexec(tid, eid, ten, scn, Oncmd, label);
        if ( etab[eid].flg & 0004 && ret < 1 ) {
            ret = -1;
            break;
        }
    }
    omexec_exit:
    if ( Ostmt )
        (void)SQLFreeHandle(SQL_HANDLE_STMT, Ostmt);
    if ( Oncmd )
        free ( Oncmd );
    return ( ret );
}

/* Oexec:
 *      prepare and eventually execute a command printing the results.
 *      tid: Thread id;
 *      eid: Execution Table Process id (idx in the etab structure array)
 *      ten: Thread Execution no (progr # of cmd/script executed for a specific thread);
 *           (SQL Intepreter uses -1)
 *      scn: Script Command no (progr # of cmd executed in a given script);
 *      Ocmd: command to execute
 *      label: Query label 
 *
 *      return: 0 if no errors, (-1) otherwise
 */
static int Oexec(int tid, int eid, int ten, int scn, SQLCHAR *Ocmd, char *label)
{
    int i=0,                    /* loop variable */
        t=0,                    /* command type: 1=select, 2=update, 3=delete, 4=insert */
        par = etab[eid].parent, /* Parent shortcut */
        gzret = 0,              /* zlib function return values */
        ret = -1;               /* function return value */
    char *ch,                   /* used to browse the field to print looking for char to escape */
         *os = 0,               /* output string pointer */
         *gzbuff = 0,           /* GZIP IO buffer */
         *obuff=0;              /* output buffer pointer */
    char *p = 0 ;               /* loop variable */
    char q = 0,                 /* quote flag */
         buff[50],              /* to build command string for csv output */
         num[32];               /* Formatted Number String */
    unsigned long l=0,          /* loop variable */
                  b=0,          /* Number of bytes written */
                  sline=0,      /* Qs start timeline in ms */
                  eline=0;      /* Qs end timeline in ms */
    unsigned int mchl=0,        /* Max Column Header Length used in print line mode */
                 osl = 0,       /* Output string length */
                 cfl=0,         /* CHAR/VARCHAR field length */
                 ncds=0,        /* Non CHAR/VARCHAR display size */
                 cfields=0,     /* number of CHAR/VARCHAR fields */
                 mrsds=0,       /* minimal result set disp size when pad=fit */
                 tfl=0,         /* total fields length */
                 d = 0,         /* loop variable */
                 j = 0 ,        /* loop variable */
                 k = 0 ,        /* loop variable */
                 otype = 1,     /* output type: 0=no print, 1=normal, 2=xml, 3=plm, 4=NOT USED, 5=pad */
                 otflg = 0017,  /* output type flag: 0001=results 0002=timing 0004=headers 0010=print rs(no binary mode) */
                 omark = 0,     /* output mark: 0=no mark, 1=normal mark, 2=mark+timeline */
                 *fl=0;         /* Field length array: max between col name or field length */
    unsigned char *fa=0,        /* Field array mask:
                                    0000 = align field left,     0010 = NOT USED            0200 = NOT USED         
                                    0001 = align field right,    0020 = NOT USED         
                                    0002 = use string qualifier  0040 = NOT USED        
                                    0004 = char/varchar field    0100 = nullable field     */
                  lfs = etab[eid].fs,   /* local field separator */
                  lrs = etab[eid].rs,   /* local record separator */
                  lsq = etab[eid].sq,   /* local string qualifier */
                  lec = etab[eid].ec;   /* local escape character */
    size_t pos=0,               /* position in the output buffer */
           nby=0,               /* number of bytes returned by fwrite */
           rsds=0,              /* Result Set Display Size */
           gzb = 0 ,            /* Number of gizipped bytes in the deflate output buffer */
           obl = 0;             /* output buffer (obuff) length */
    long  tinit=0,              /* init time in ms */
          tprep=0,              /* prepare time in ms */
          t1fetch=0,            /* first fetch time in ms */
          tfetch=0,             /* total fetch time in ms */
          telaps=0,             /* elapsed time mark */
          texec=0;              /* exec time in ms */
    SQLCHAR *Ocnames=0;         /* ODBC column name returned by select */
    SQLLEN Onrows=0;            /* ODBC no of affected rows */
    SQLRETURN Or;               /* ODBC return value */
    SQLHSTMT Ostmt=0;           /* ODBC Statement Handle */
    SQLSMALLINT Onamel,         /* ODBC column name length returned by SQLDescribeCol */
                *Odt=0,         /* ODBC data type returned by SQLDescribeCol */
                *Odd=0,         /* ODBC decimal digit returned by SQLDescribeCol */
                Onull;          /* ODBC nullable returned by SQLDescribeCol */
    SQLUSMALLINT Oncol;         /* ODBC number of columns in the result set */
    SQLPOINTER **Oresp = 0;     /* ODBC Result Array Pointer */
    SQLLEN **Olength = 0;       /* ODBC Length indicator array pointer */
    SQLLEN Oll = 0 ;            /* ODBC Local Length Indicator value loop variable */
    SQLULEN *Ors = 0;           /* ODBC Record Display Size array pointer */
    SQLULEN Orespl = 0;         /* ODBC ulen to store SQLGetInfo results */
    struct timeval tve;         /* timeval struct to define elapesd/timelines */
    unsigned int ffetch = 1 ;   /* First Fetch marker */
    Mutex *parmutex = &etab[par].pmutex ;   /* Local copy of parent mutex address */
    z_stream gzstream = { 0 };  /* Local gzstream structure */
    unsigned int ucs = 0;       /* Local ucs2toutf8 conversion flag */
    size_t mcfsl = etab[eid].mcfs ? strlen(etab[eid].mcfs) : 0;
    size_t mcrsl = etab[eid].mcrs ? strlen(etab[eid].mcrs) : 0;

    /* register start time */
    gettimeofday(&tve, (void *)NULL);       
    sline=1000*(tve.tv_sec-tvs.tv_sec)+(tve.tv_usec-tvs.tv_usec)/1000;
    tinit = 1000*(tve.tv_sec-tvi.tv_sec)+(tve.tv_usec-tvi.tv_usec)/1000;

    if ( etab[eid].flg2 & 020000000 ) {     /* Oexec to allocate its own handle */
        if ( !SQL_SUCCEEDED(Or=SQLAllocHandle(SQL_HANDLE_STMT, thps[tid].Oc, &Ostmt))){
            Oerr(eid, tid, __LINE__, thps[tid].Oc, SQL_HANDLE_DBC);
            return(-1);
        }
    } else {
        Ostmt = thps[tid].Os;
    }

    /* Initialize otype, otflg, omark and ucs flags */
    if ( etab[eid].flg & 0100000000 ) {         /* Oexec in silent mode */
        otype = 0;
    } else if ( etab[eid].flg2 & 04000000 ) {   /* xlm on */
        otype = 2;
        otflg &= ~0004 ;                        /* no headers */
    } else if ( etab[eid].flg & 040000 ) {      /* plm on */
        otype = 3;
        otflg &= ~0004 ;                        /* no headers */
    } else if ( pad ) {                         /* pad */
        otype = 5;
    }
    if ( !(etab[eid].flg & 0200) )              /* pcn is off */
        otflg &= ~0004;
    if ( etab[eid].flg & 020000 ) {             /* -q res */
        otflg &= ~0005;
        otype = 0 ;
    }
    if ( etab[eid].flg2 & 0004 )                /* -q timing */
        otflg &= ~0002;
    if ( etab[eid].type == 'e' ) {
        if ( etab[eid].flg & 0400 )             /* print simple extract mark */
            omark = 1 ;
        if ( etab[eid].flg2 & 020000 )
            omark = 2 ;
    }
    if ( etab[eid].flg & 0100 )                 /* nimary mode: no RS */
        otflg &= ~0010 ;
    if ( etab[eid].flg & 010000000 ) {          /* Convert ucs2toutf8 */
        ucs = 1 ;
    }

    /* Initialize deflate & allocate gzip io buffer */
    if ( etab[eid].flg & 0020 ) {               /* gzip output: allocate gzstream structure  */
        gzstream.zalloc = Z_NULL;
        gzstream.zfree = Z_NULL;
        gzstream.opaque = Z_NULL;
        /* Initalize deflate */
        if ( ( gzret = deflateInit2 (&gzstream, etab[eid].gzlev, Z_DEFLATED, windowBits + GZIP_ENCODING, 8, Z_DEFAULT_STRATEGY ) ) != Z_OK ) {
            fprintf(stderr, "odb [Oexec(%d)] - Error initializing zlib: [%d]\n",
                __LINE__, gzret);
            return(-1);
        }
        if ( ( gzbuff = malloc((size_t)GZBUFF_LEN) ) == (void *)NULL ) {
            fprintf(stderr, "odb [Oexec(%d)] - Error allocating gzip IO buffer: [%d] %s\n",
                __LINE__, errno, strerror(errno));
            return(-1);
        }
    }

    /* Determine command type 
        The "right" solution here would have been:
        SQLGetDiagField(SQL_HANDLE_STMT, Ostmt, 0, SQL_DIAG_DYNAMIC_FUNCTION_CODE, &Ostype,
            SQL_IS_INTEGER, (SQLSMALLINT *)NULL)
        and then switch based on Ostype. However this doesn't work with Vertica
Please note the fixed length '6' in strmicmp: SELECT/UPDATET/DELETE/INSERT have the same length */
    for ( t = (int)((sizeof(rmess)/sizeof(struct rm)) - 1);
          t > 0 && strmicmp(rmess[t].cmd, (char *)Ocmd, 6); t--); 

    /* Prepare command */
    tprep -= 1000*tve.tv_sec + tve.tv_usec/1000;
    if ((Or=SQLPrepare(Ostmt, Ocmd, SQL_NTS)) != SQL_SUCCESS) {
        Oerr(eid, tid, __LINE__, Ostmt, SQL_HANDLE_STMT);
        if ( Or != SQL_SUCCESS_WITH_INFO )
            return(-1);
    }
    gettimeofday(&tve, (void *)NULL);       /* register end time */
    tprep += 1000*tve.tv_sec + tve.tv_usec/1000;
    if ( etab[eid].flg & 0010 ) {   /* Null run: just check syntax preparing the command */
        if ( ten >= 0 )
            fprintf(stderr, "[%d.%d.%d]", tid, ten, scn);
        mfprintf (stderr, etab[eid].fso, "--- prepared in %.3fs\n", tprep/1000.0);
        return (0);
    }

    /* Execute command */
    texec -= 1000*tve.tv_sec + tve.tv_usec/1000;
    if (!SQL_SUCCEEDED(Or=SQLExecute(Ostmt)) && Or != SQL_NO_DATA) {
        Oerr(eid, tid, __LINE__, Ostmt, SQL_HANDLE_STMT);
        return(-1);
    }
    gettimeofday(&tve, (void *)NULL);       /* register end time */
    texec += 1000*tve.tv_sec + tve.tv_usec/1000;

    /* If SQLExecute executes a searched update, insert, or delete statement that does... */
    /* not affect any rows at the data source, the call to SQLExecute returns SQL_NO_DATA */
    if ( Or == SQL_NO_DATA )
        goto oexec_nocols ;

    /* Find number of returned columns */
    if (!SQL_SUCCEEDED(Or=SQLNumResultCols(Ostmt, (SQLSMALLINT *)&Oncol))) {
        Oerr(eid, tid, __LINE__, Ostmt, SQL_HANDLE_STMT);
        return(-1);
    }
    if ( Oncol == 0 )       /* No resulting Columns... goto oexec_nocols: */
        goto oexec_nocols;
    tfetch -= 1000*tve.tv_sec + tve.tv_usec/1000;

    /* Allocate memory for the record size array */
    if ( (Ors = (SQLULEN *)calloc ((size_t)Oncol, sizeof(SQLULEN))) == (void *)NULL ) {
            fprintf(stderr, "odb [Oexec(%d)] - Error allocating record size array memory: [%d] %s\n",
                __LINE__, errno, strerror(errno));
        return(-1);
    }   

    /* Allocate memory for the data type size array */
    if ( (Odt = (SQLSMALLINT *)calloc ((size_t)Oncol, sizeof(SQLSMALLINT))) == (void *)NULL ) {
            fprintf(stderr, "odb [Oexec(%d)] - Error allocating data types array memory: [%d] %s\n",
                __LINE__, errno, strerror(errno));
        exstat = EX_OSERR ;
        goto oexec_exit;
    }   

    /* Allocate memory for decimals array */
    if ( (Odd = (SQLSMALLINT *)calloc ((size_t)Oncol, sizeof(SQLSMALLINT))) == (void *)NULL ) {
            fprintf(stderr, "odb [Oexec(%d)] - Error allocating decimals array memory: [%d] %s\n",
                __LINE__, errno, strerror(errno));
        exstat = EX_OSERR ;
        goto oexec_exit;
    }   

    /* Allocate memory for the result array */
    if ( (Oresp = (SQLPOINTER **)calloc ((size_t)Oncol, sizeof(SQLPOINTER *))) == (void *)NULL ) {
            fprintf(stderr, "odb [Oexec(%d)] - Error allocating result array memory: [%d] %s\n",
                __LINE__, errno, strerror(errno));
        exstat = EX_OSERR ;
        goto oexec_exit;
    }   

    /* Allocate memory for the length indicator array */
    if ( (Olength = (SQLLEN **)calloc ((size_t)Oncol, sizeof(SQLLEN *))) == (void *)NULL ) {
            fprintf(stderr, "odb [Oexec(%d)] - Error allocating length indicator array memory: [%d] %s\n",
                __LINE__, errno, strerror(errno));
        exstat = EX_OSERR ;
        goto oexec_exit;
    }   

    /* Allocate memory for the field length array */
    if ( (fl = (unsigned int *)calloc ((size_t)Oncol, sizeof(unsigned int))) == (void *)NULL ) {
        fprintf(stderr, "odb [Oexec(%d)] - Error allocating field length array memory: [%d] %s\n",
            __LINE__, errno, strerror(errno));
        exstat = EX_OSERR ;
        goto oexec_exit;
    }   
    if ( (fa = (unsigned char *)calloc ((size_t)Oncol, 1)) == (void *)NULL ) {
        fprintf(stderr, "odb [Oexec(%d)] - Error allocating field align array memory: [%d] %s\n",
            __LINE__, errno, strerror(errno));
        exstat = EX_OSERR ;
        goto oexec_exit;
    }   

    /* Allocate header's buffer memory */
    if ( (Ocnames = (SQLCHAR *)calloc ((size_t)Oncol, MAXCOL_LEN)) == (void *)NULL){
        fprintf(stderr, "odb [Oexec(%d)] - Error allocating header buffer memory: [%d] %s\n",
            __LINE__, errno, strerror(errno));
        exstat = EX_OSERR ;
        goto oexec_exit;
    }

    /* Get Result Set columns descriptions */
    for (j = 0; j < Oncol; j++) {
        if ( !SQL_SUCCEEDED(Or=SQLDescribeCol(Ostmt, (SQLUSMALLINT)(j+1),
                &Ocnames[j*MAXCOL_LEN], (SQLSMALLINT) MAXCOL_LEN, &Onamel,
                &Odt[j], &Ors[j], &Odd[j], &Onull))) {
            Oerr(eid, tid, __LINE__, Ostmt, SQL_HANDLE_STMT);
            exstat = EX_ODBCERR ;
            goto oexec_exit;
        }
        if ( etab[eid].flg & 0400000 && t == 1 ) {  /* Describe result set */
            mfprintf(stderr, etab[eid].fso, "%s%s %s",
                j ? "\t," : "CREATE TABLE ODBC.DATA.TYPES (\n\t ",
                (char *)&Ocnames[j*MAXCOL_LEN], expandtype(Odt[j]));
            switch(Odt[j]) {
            case SQL_CHAR:
            case SQL_WCHAR:
            case SQL_VARCHAR:
            case SQL_WVARCHAR:
            case SQL_LONGVARCHAR:
            case SQL_WLONGVARCHAR:
            case SQL_BINARY:
            case SQL_VARBINARY:
            case SQL_LONGVARBINARY:
                mfprintf(stderr, etab[eid].fso, "(%u) ", (unsigned int)Ors[j]);
                break;
            case SQL_NUMERIC:
            case SQL_DECIMAL:
                mfprintf(stderr, etab[eid].fso, "(%u,%u) ", (unsigned int)Ors[j], (unsigned int)Odd[j]);
                break;
            case SQL_TIME:
            case SQL_TIMESTAMP:
            case SQL_TYPE_TIME:
            case SQL_TYPE_TIMESTAMP:
                if ( Odd[j] )
                    mfprintf(stderr, etab[eid].fso, "(%u) ", (unsigned int)Odd[j]);
                break;
            default:
                mfprintf(stderr, etab[eid].fso, " ");
                break;
            }
            mfprintf(stderr, etab[eid].fso, "%s\n", Onull ? "" : "NOT NULL");
        }
        if ( (unsigned int) Onamel > mchl ) /* Calculate max column-name length */
            mchl = (unsigned int)Onamel;
        if(!SQL_SUCCEEDED(Or=SQLColAttribute(Ostmt, (SQLUSMALLINT)(j+1),
                SQL_DESC_DISPLAY_SIZE, (SQLPOINTER) NULL, (SQLSMALLINT) 0,
                (SQLSMALLINT *) NULL, (SQLPOINTER) &Ors[j]))) {
            Oerr(eid, tid, __LINE__, thps[tid].Os, SQL_HANDLE_STMT);
            exstat = EX_ODBCERR ;
            goto oexec_exit;
        }
        if ( Onull )
            fa[j] |= 0100;  
        if ( pad && (fa[j] & 0100 ) && ( Ors[j] < etab[eid].nl ) )
            Ors[j] = etab[eid].nl;  /* space to write nullstring */ 
        if ( ( etab[eid].flg & 0200 ) || pad ) 
            fl[j] = Ors[j] > (SQLULEN)Onamel ? (unsigned int)Ors[j] : (unsigned int)Onamel;
        switch ( Odt[j] ) {
        case SQL_REAL:
        case SQL_DOUBLE:
        case SQL_FLOAT:
        case SQL_NUMERIC:
        case SQL_DECIMAL:
        case SQL_SMALLINT:
        case SQL_INTEGER:
        case SQL_TINYINT:
        case SQL_BIGINT:
            fa[j] |= 0001;      /* numeric alignment right */
            ncds += (unsigned int)Ors[j];
            break;
        case SQL_WCHAR:
            if ( etab[eid].dbt != VERTICA ) /* Vertica's CHAR field length is in bytes (not chars) */
                Ors[j] *= etab[eid].bpwc;
            /* FALLTHRU */
        case SQL_CHAR:
            if ( etab[eid].dbt != VERTICA ) /* Vertica's CHAR field length is in bytes (not chars) */
                Ors[j] *= etab[eid].bpc;
            fa[j] |= 0004;      /* This is a CHAR/VARCHAR field */
            if ( etab[eid].sq ) /* if string qualifier char is defined */
                fa[j] |= 0002;  /* enclose CHAR/VARCHAR fields in sq */
            if ( etab[eid].Omaxl && etab[eid].Omaxl < Ors[j] ) {
                fl[j] = (unsigned int)etab[eid].Omaxl;
                Ors[j] = etab[eid].Omaxl;
            }
            cfl += fl[j];
            cfields++;
            break;
        case SQL_WVARCHAR:
        case SQL_WLONGVARCHAR:
            if ( etab[eid].dbt != VERTICA ) /* Vertica's CHAR field length is in bytes (not chars) */
                Ors[j] *= etab[eid].bpwc;
            /* FALLTHRU */
        case SQL_VARCHAR:
        case SQL_LONGVARCHAR:
            if ( etab[eid].dbt != VERTICA ) /* Vertica's CHAR field length is in bytes (not chars) */
                Ors[j] *= etab[eid].bpc;
            fa[j] |= 0004;      /* This is a CHAR/VARCHAR field */
            if ( etab[eid].sq ) /* if string qualifier char is defined */
                fa[j] |= 0002;  /* enclose CHAR/VARCHAR fields in sq */
            if ( etab[eid].Omaxl && etab[eid].Omaxl < Ors[j] ) {
                fl[j] = (unsigned int)etab[eid].Omaxl;
                Ors[j] = etab[eid].Omaxl;
            }
            cfl += fl[j];
            cfields++;
            break;
        case SQL_TIME:
        case SQL_TYPE_TIME:
        case SQL_TIMESTAMP:
        case SQL_TYPE_TIMESTAMP:
            if ( etab[eid].flg & 0040 ) /* truncate decimals */
                fa[j] |= 0200;  
            ncds += (unsigned int)Ors[j];
            break;
        default:
            ncds += (unsigned int)Ors[j];
        }
        Ors[j]++;                   /* add space for NULL */
        tfl += fl[j];
        if ( fdmp && eid == par )   /* Parent to initialize ODBC dump file */
            fprintf(fdmp, "[%d] Column %d: name=%s, type=%d (%s), size=%lu, decimals=%d, nullable=%s\n",
                tid, j, &Ocnames[j*MAXCOL_LEN], (int)Odt[j], expandtype(Odt[j]),
                (unsigned long)Ors[j], (int)Odd[j], Onull ? "yes" : "no" ) ; 
        rsds += (size_t) Ors[j];
    }

    /* Close Describe Result set */
    if ( etab[eid].flg & 0400000 && t == 1 ) {   /* Describe result set */
        mfprintf(stderr, etab[eid].fso, ");\n");
        mfprintf(stderr, etab[eid].fso, "Result Set Display Size: %zu bytes (%zu bytes including terminating NULLs)\n", rsds - j, rsds);
    }

    /* reduce CHAR/VARCHAR field length to fit display size */
    if ( !(etab[eid].flg & 040000) && pad == 2 && mrcol < tfl ) {
        if ( ncds + (unsigned int)Oncol + cfields * 2 > mrcol ) {
            fprintf(stderr, "odb [Oexec(%d)] - Warning cannot fit record in %u cols\n",
                 __LINE__, mrcol);
            pad = 3;    /* temporary pad. behaves like full, reset to pad=2 at the end */
        } else {
            mrsds = tfl - cfl + Oncol - 1 ; /* space needed for non CHAR/VCHAR fields + field seps */
            for (j = 0; j < Oncol; j++) {   /* shrink fields longer than content (because of col titles) */
                if ( !(fa[j] & 0004) ) {        /* this is NOT a CHAR/VARCHAR field */
                    if ( fl[j] > (unsigned int)Ors[j] ) {
                        d = fl[j] - (unsigned int)Ors[j] + 1;
                        fl[j] -= d;
                        mrsds -= d;
                        cfl += d;
                    }
                }
            }
            for (j = 0, d = 0; j < Oncol; j++) {    /* shrink CHAR/VCHAR fields */
                if ( fa[j] & 0004 ) {   /* this is a CHAR/VARCHAR field */
                    fl[j] = (unsigned int) ( fl[j] * ( mrcol - mrsds ) * 1.0 / cfl - 1.0 ); /* assign space proportionally */
                    if ( fl[j] < 2 )    /* for short fields... */
                        fl[j] = 2;      /* ... use a min length of 2 chars */
                    Ors[j] = fl[j] + 1; /* update Ors with new field length + NULL */
                }
                d += fl[j];
            }
            /* Finally: assign remaining chars due to division roundings */
            for (j = 0, d = mrcol - d - Oncol + 1 ; d && j < Oncol ; j++) {
                if ( fa[j] & 0004 ) {   /* this is a CHAR/VARCHAR field */
                    fl[j]++;
                    Ors[j]++;
                    d--;
                }
            }
        }
    }

    /* Calculate rowset if buffer size is set */
    if ( etab[eid].rbs ) {
        etab[eid].r = etab[eid].rbs / rsds;
        if ( etab[eid].mr && etab[eid].r > etab[eid].mr )   /* if # records to fetch < rowset ... */
            etab[eid].r = etab[eid].mr;                     /* make rowset = records to fetch */
        etab[eid].r = etab[eid].r < 1 ? 1 : etab[eid].r;    /* at least one record at a time  */
    }

    /* Set output buffer length */
    obl = ( rsds + 1 ) * ( etab[eid].r + 1 ) ; 

    /* Allocate memory for the result set based on display size */
    for (j = 0; j < Oncol; j++) {
        if ( (Oresp[j] = (SQLPOINTER *)calloc (etab[eid].r, (size_t)Ors[j])) == (void *)NULL ||
             (Olength[j] = (SQLLEN *)calloc (etab[eid].r, sizeof(SQLLEN))) == (void *)NULL ) {
            fprintf(stderr, "odb [Oexec(%d)] - Error allocating memory (%d bytes) for %s: [%d] %s\n",
                __LINE__, (int) Ors[j], (char *)&Ocnames[j*MAXCOL_LEN], errno, strerror(errno));
            exstat = EX_OSERR ;
            goto oexec_exit;
        }
        /* Binding columns */
        if (!SQL_SUCCEEDED(Or = SQLBindCol(Ostmt, (SQLUSMALLINT)(j+1),
                etab[eid].flg & 0100 ? SQL_C_BINARY : SQL_C_CHAR, 
                Oresp[j], Ors[j], Olength[j]))) {
            Oerr(eid, tid, __LINE__, Ostmt, SQL_HANDLE_STMT);
            exstat = EX_OSERR ;
            goto oexec_exit;
        }
    }

    /* Set Statement attributes for Column-wise binding */
    if (!SQL_SUCCEEDED(Or=SQLSetStmtAttr(Ostmt, SQL_ATTR_ROW_BIND_TYPE,
            (SQLPOINTER)SQL_BIND_BY_COLUMN, 0))) {
        Oerr(eid, tid, __LINE__, Ostmt, SQL_HANDLE_STMT);
        exstat = EX_ODBCERR ;
        goto oexec_exit;
    }
    if (!SQL_SUCCEEDED(Or=SQLSetStmtAttr(Ostmt, SQL_ATTR_ROW_ARRAY_SIZE,
            (SQLPOINTER)(etab[eid].r), 0))) {
        Oerr(eid, tid, __LINE__, Ostmt, SQL_HANDLE_STMT);
        exstat = EX_ODBCERR ;
        goto oexec_exit;
    }
    if (!SQL_SUCCEEDED(Or=SQLSetStmtAttr(Ostmt, SQL_ATTR_ROW_STATUS_PTR,
            NULL, 0))) {
        Oerr(eid, tid, __LINE__, Ostmt, SQL_HANDLE_STMT);
        exstat = EX_ODBCERR ;
        goto oexec_exit;
    }
    if (!SQL_SUCCEEDED(Or=SQLSetStmtAttr(Ostmt, SQL_ATTR_ROWS_FETCHED_PTR, 
            &Orespl, 0))) {
        exstat = EX_ODBCERR ;
        goto oexec_exit;
    }

    /* Fetch data */
    while ( go &&
            SQL_SUCCEEDED(Or = SQLFetchScroll(Ostmt, SQL_FETCH_NEXT, 0))) {
        if ( ffetch ) {         /* first fetch */
            ffetch = 0 ;
            gettimeofday(&tve, (void *)NULL);   /* register time */
            t1fetch = tfetch + 1000*tve.tv_sec + tve.tv_usec/1000;
            if ( (obuff = malloc ( obl )) == (void *)NULL ) {           /* Allocate memory for the output buffer */
                fprintf(stderr, "odb [Oexec(%d)] - Error allocating output buffer: [%d] %s\n",
                    __LINE__, errno, strerror(errno));
                exstat = EX_OSERR ;
                goto oexec_exit;
            }   
            if ( otflg & 0004 ) {   /* print column header */
                if ( pad ) {
                    for ( i = 0 ; i < Oncol ; i++ ) {
                        for ( l = 0 ; Ocnames[i*MAXCOL_LEN+l] ; l++ ) {
                            if ( pad == 2 && l == fl[i] ) {
                                Ocnames[i*MAXCOL_LEN+l] = '\0';
                                Ocnames[i*MAXCOL_LEN+l-1] = '>';
                                break;
                            }
                        }
                        pos += snprintf(&obuff[pos], obl - pos,
                            ( fa[i] & 0001 ) ? "%*s" : "%-*s",
                            (int)( fl[i] > MAXCOL_LEN ? MAXCOL_LEN : fl[i] ),
                            (char *)&Ocnames[i*MAXCOL_LEN]);
                        if ( (i+1) < (int)Oncol)
                            obuff[pos++] = lfs;
                    }
                    obuff[pos++] = etab[eid].rs;
                    for (i = 0; i < (int) Oncol; i++) {
                        for ( l = 0; l < fl[i] && l < MAXCOL_LEN ; l++)
                            obuff[pos++] = '-';
                        if ( (i+1) < (int) Oncol)
                            obuff[pos++] = '+';
                    }
                } else {
                    for ( i = 0 ; i < Oncol ; i++ ) {
                        p = (char *)&Ocnames[i*MAXCOL_LEN] ;
                        while ( *p )
                            obuff[pos++] = *p++ ;
                        if ( (i+1) < (int)Oncol)
                            obuff[pos++] = lfs;
                    }
                }
                obuff[pos++] = lrs;
            }
        }
        Onrows += Orespl;
        if ( fdmp ) {   /* Dump ODBC buffers */
            MutexLock( parmutex ); 
            for ( i = 0 ; i < (int)Oncol ; i++ ) {
                fprintf(fdmp, "[%d] Column %d: %s (%lu values, %lu bytes/value)\n",
                    tid, i, (char *)&Ocnames[i*MAXCOL_LEN], (unsigned long) Orespl, (unsigned long)Ors[i] ) ;
                dumpbuff (fdmp, eid, (unsigned char *)Oresp[i], (size_t)(Orespl * Ors[i]) , 1 );
            }
            MutexUnlock( parmutex );
        }
        if ( ucs ) {                            /* UCS-2 to UTF-8 conversion */
            for ( l = 0 ; l < (unsigned long)Orespl ; l++ ) {
                for ( i = 0 ; i < (int)Oncol ; i++ ) {
                    switch ( etab[eid].td[i].Otype ) {
                    case SQL_WCHAR:
                    case SQL_WVARCHAR:
                    case SQL_WLONGVARCHAR:
                        if ( ucs2toutf8 ( (SQLCHAR *)Oresp[i] + (Ors[i] * l ) , &Olength[i][l], Ors[i], etab[eid].bucs2 ) ) {
                            fprintf(stderr, "odb [Oexec(%d)] - Error converting UCS-2 column %s\n",
                                __LINE__, (char *)etab[eid].td[i].Oname);
                        }
                        break;
                    default:
                        break;                          /* do nothing */
                    }
                }
            }
        }
        /* Fill output buffer */
        switch ( otype ) {
        case 0:                 /* no print */
            break;
        case 1:                 /* normal print (csv like) */
            for ( l = 0 ; l < (unsigned long)Orespl ; l++) {
                for (i = 0; i < (int) Oncol; i++) { 
                    if ( i ) {
                        if (!etab[eid].mcfs)
                            obuff[pos++] = lfs;
                        else {
                            strcpy(&obuff[pos], etab[eid].mcfs);
                            pos += mcfsl;
                        }
                    }
                    switch ( Oll = Olength[i][l] ) {
                    case 0:
                        os = etab[eid].es ;
                        osl = etab[eid].el ;
                        break;
                    case SQL_NULL_DATA:
                        os = etab[eid].ns ;
                        osl = etab[eid].nl ;
                        break;
                    default:
                        os = (char *)Oresp[i] + (Ors[i] * l) ;
                        osl = (unsigned int) Oll;
                        q = fa[i] & 0002 ;  /* quote? */
                    }
                    if ( q ) {  /* Print qualified strings with escapes */
                        obuff[pos++] = lsq;
                        for ( ch = os, j = 0; j < osl ; j++ ) {
                            if ( *ch == lsq || *ch == lec ) 
                                obuff[pos++] = lec;
                            obuff[pos++] = *ch++;
                        }
                        obuff[pos++] = lsq;
                    } else {    /* Print based on field length (long fields) */
                        MEMCPY ( &obuff[pos], os, osl ) ;
                        pos += (size_t)osl;
                    }
                }
                if ( otflg & 0010 ) {
                    if (!etab[eid].mcrs)
                        obuff[pos++] = lrs;
                    else {
                        strcpy(&obuff[pos], etab[eid].mcrs);
                        pos += mcrsl;
                    }
                }
            }
            break;
        case 2:                 /* XML print */
            for ( l = 0 ; l < (unsigned long)Orespl ; l++) {
                pos += snprintf(&obuff[pos], obl - pos, "\n  <row>");
                for (i = 0; i < (int) Oncol; i++) { 
                    pos += snprintf(&obuff[pos], obl - pos, "\n    <%s>", (char *)&Ocnames[i*MAXCOL_LEN]);
                    switch ( Oll = Olength[i][l] ) {
                    case 0:
                        os = etab[eid].es ;
                        break;
                    case SQL_NULL_DATA:
                        os = etab[eid].ns ;
                        break;
                    default:
                        os = (char *)Oresp[i] + (Ors[i] * l) ;
                    }
                    pos += snprintf(&obuff[pos], obl - pos, "%s</%s>", os, (char *)&Ocnames[i*MAXCOL_LEN]);
                }
                pos += snprintf(&obuff[pos], obl - pos, "\n  </row>");
            }
            break;
        case 3:                 /* Print Line Mode */
            for ( l = 0 ; l < (unsigned long)Orespl ; l++) {
                k = (unsigned int)snprintf(&obuff[pos], obl - pos, "[ROW %lu] ", l + 1 ) ;
                pos += (size_t) k ;
                while ( k++ < mrcol )
                    obuff[pos++] = k == mchl + 1 ? '+' : '-' ;
                obuff[pos++] = '\n';
                for (i = 0; i < (int) Oncol; i++) { 
                    switch ( Oll = Olength[i][l] ) {
                    case 0:
                        os = etab[eid].es ;
                        break;
                    case SQL_NULL_DATA:
                        os = etab[eid].ns ;
                        break;
                    default:
                        os = (char *)Oresp[i] + (Ors[i] * l) ;
                    }
                    pos += snprintf(&obuff[pos], obl - pos, "%-*s|%s\n", mchl,
                        (char *)&Ocnames[i*MAXCOL_LEN], os);
                }
            }
            break;
        case 5:                         /* pad printing */
            for ( l = 0 ; l < (unsigned long)Orespl ; l++) {
                for (i = 0; i < (int) Oncol; i++) { 
                    if ( i )
                        obuff[pos++] = lfs;
                    switch ( Oll = Olength[i][l] ) {
                    case 0:
                        os = etab[eid].es ;
                        osl = etab[eid].el ;
                        break;
                    case SQL_NULL_DATA:
                        os = etab[eid].ns ;
                        osl = etab[eid].nl ;
                        break;
                    default:
                        os = (char *)Oresp[i] + (Ors[i] * l) ;
                        osl = (unsigned int) Oll ;
                    }
                    if ( fl[i] < osl ) { /* field has been truncated */
                        os[fl[i]-1] = '>';
                        os[fl[i]] = '\0';
                    }
                    pos += snprintf(&obuff[pos], obl - pos, (fa[i]&0001)?"%*s":"%-*s",
                        (int)fl[i], os);
                }
                obuff[pos++] = lrs;
            }
            break;
        }
        /* Write output buffer */
        if ( etab[eid].flg & 0020 ) {   /* gzip output */
            gzstream.next_in = (unsigned char *)obuff ;
            gzstream.avail_in = (unsigned int) pos ;
            MutexLock( parmutex ); 
            do {
                gzstream.avail_out = GZBUFF_LEN ;
                gzstream.next_out = (unsigned char *)gzbuff ;
                if ( ( gzret = deflate (&gzstream, Z_NO_FLUSH) ) != Z_OK ) {
                    fprintf(stderr, "odb [Oexec(%d)] - Error during deflate: [%d]\n",
                        __LINE__, gzret);
                    MutexUnlock( parmutex );
                    goto oexec_exit ;
                }
                gzb = GZBUFF_LEN - gzstream.avail_out ;
#ifdef HDFS
                if ( etab[eid].fho ) {  /* write gzipped file to normal output */
                    nby = ( size_t) (*hdfswrite)(hfs, etab[eid].fho, (void *)gzbuff, gzb);
                } else {                        /* HDFS output */
                    if ( ( nby = fwrite ( gzbuff, 1, gzb, etab[eid].fo ) ) != gzb )
                        fprintf(stderr, "odb [Oexec(%d)] - Warning fwrite wrote " SIZET_SPEC " out of " SIZET_SPEC " bytes: %s",
                            __LINE__, nby, gzb, strerror(errno));
                }
#else
                if ( ( nby = fwrite ( gzbuff, 1, gzb, etab[eid].fo ) ) != gzb )
                    fprintf(stderr, "odb [Oexec(%d)] - Warning fwrite wrote " SIZET_SPEC " out of " SIZET_SPEC " bytes: %s",
                        __LINE__, nby, gzb, strerror(errno));
#endif
                b += (unsigned long)nby;
            } while ( gzstream.avail_out == 0 ) ;
            gzstream.next_in = (unsigned char *)obuff ;
            gzstream.next_out = (unsigned char *)gzbuff ;
            gzstream.avail_in = 0 ;
            gzstream.avail_out = GZBUFF_LEN ;
            if ( ( gzret = deflate (&gzstream, Z_FINISH) ) != Z_STREAM_END ) {
                fprintf(stderr, "odb [Oexec(%d)] - Error during deflate: [%d]\n",
                    __LINE__, gzret);
                goto oexec_exit ;
            }
            gzb = GZBUFF_LEN - gzstream.avail_out ;
#ifdef HDFS
            if ( etab[eid].fho ) {  /* write gzipped file to normal output */
                nby = ( size_t) (*hdfswrite)(hfs, etab[eid].fho, (void *)gzbuff, gzb);
            } else {                        /* HDFS output */
                if ( ( nby = fwrite ( gzbuff, 1, gzb, etab[eid].fo ) ) != gzb )
                    fprintf(stderr, "odb [Oexec(%d)] - Warning fwrite wrote " SIZET_SPEC " out of " SIZET_SPEC " bytes: %s",
                        __LINE__, nby, gzb, strerror(errno));
            }
#else
            if ( ( nby = fwrite ( gzbuff, 1, gzb, etab[eid].fo ) ) != gzb )
                fprintf(stderr, "odb [Oexec(%d)] - Warning fwrite wrote " SIZET_SPEC " out of " SIZET_SPEC " bytes: %s",
                    __LINE__, nby, gzb, strerror(errno));
#endif
            b += (unsigned long)nby;
            if ( ( gzret = deflateReset (&gzstream) ) != Z_OK ) {
                fprintf(stderr, "odb [Oexec(%d)] - Error during deflateReset: [%d]\n",
                    __LINE__, gzret);
            }
            MutexUnlock( parmutex );
#ifdef HDFS
        } else if ( etab[eid].fho ) {           /* non-HDFS output */
            MutexLock( parmutex );
            nby = ( size_t) (*hdfswrite)(hfs, etab[eid].fho, (void *)obuff, pos);
            MutexUnlock( parmutex );
            b += (unsigned long)nby;
#endif
        } else {                        /* HDFS output */
            if ( ( nby = fwrite ( obuff, 1, pos, etab[eid].fo ) ) != pos )
                fprintf(stderr, "odb [Oexec(%d)] - Warning fwrite wrote " SIZET_SPEC " out of " SIZET_SPEC " bytes: %s",
                    __LINE__, nby, pos, strerror(errno));
            b += (unsigned long)nby;
        }
        if ( etab[eid].fso )
            if ( ( nby = fwrite ( obuff, 1, pos, etab[eid].fso ) ) != pos )
                fprintf(stderr, "odb [Oexec(%d)] - Warning fwrite sppoled " SIZET_SPEC " out of " SIZET_SPEC " bytes: %s",
                    __LINE__, nby, pos, strerror(errno));
        pos = 0;
        /* Print mark for extracts */
        switch ( omark ) {  
        case 1:     /* simple mark */
            fprintf(stderr,"[%d] %s records extracted\n", tid, strmnum((double)Onrows,num,sizeof(num),0));
            break;
        case 2:     /* mark + timeline */
            gettimeofday(&tve, (void *)NULL);       /* register end time */
            telaps = 1000*(tve.tv_sec-tvi.tv_sec)+(tve.tv_usec-tvi.tv_usec)/1000;
            fprintf(stderr,"[%d] %s records extracted (%ld ms)\n", tid, strmnum((double)Onrows,num,sizeof(num),0), telaps);
            break;
        }
        /* Check Max fetches */
        if ( etab[eid].mr && (unsigned long)Onrows >= etab[eid].mr ) /* max fetches */
            break;
    }
    if ( Onrows && otype == 3 ) {   /* Print Line Mode: end line */
        for ( k = 0 ; k < mrcol ; k++ ) 
            obuff[pos++] = k == mchl ? '+' : '-' ;
        obuff[pos++] = '\n';
    }

    /* deflateEnd gzip */
    if ( etab[eid].flg & 0020 ) {   /* gzip output */
        if ( ( gzret = deflateEnd (&gzstream) ) != Z_OK ) {
            fprintf(stderr, "odb [Oexec(%d)] - Error during deflateEnd: [%d]\n",
                __LINE__, gzret);
        }
    } 

    /* Check fetch loop return value */
    if (Or != SQL_NO_DATA && Or != SQL_SUCCESS) {
        Oerr(eid, tid, __LINE__, Ostmt, SQL_HANDLE_STMT);
        goto oexec_exit;
    }

    /* Register end fetch time */
    gettimeofday(&tve, (void *)NULL);       /* register end fetch time */
    tfetch += 1000*tve.tv_sec + tve.tv_usec/1000;
    eline=1000*(tve.tv_sec-tvs.tv_sec)+(tve.tv_usec-tvs.tv_usec)/1000;

    oexec_nocols:       /* no columns found during exec */
    /* Find number of affected rows */
    if ( t > 1 && !SQL_SUCCEEDED(Or=SQLRowCount(Ostmt, &Onrows))) {
        Oerr(eid, tid, __LINE__, Ostmt, SQL_HANDLE_STMT);
        exstat = EX_ODBCERR ;
        goto oexec_exit;
    }

    if ( f & 00400 ) {  /* csv timing output */
        for ( i = j = 0 ; Ocmd[j] && i < 49 ; j++ )
            if ( !isspace(Ocmd[j]) )
                buff[i++] = (char)Ocmd[j];
            else if ( i && buff[i - 1] != ' ' )
                buff[i++] = ' ';
        if ( Ocmd[j] )
            buff[i++] = '>';
        buff[i] = '\0';
        printf("%d%c%d%c%d%c%d%c%s%c%s%c\"%s\"%c%ld%c" SIZET_SPEC "%c%.3f%c%.3f%c%.3f%c%.3f%c%.3f%c%lu%c%lu\n",
            tid, fs, eid, fs, ten, fs, scn, fs,
            (etab[eid].type != 'x' ? etab[eid].run : "(none)"), fs,
            (label ? label : "(none)"), fs,
            buff, fs, (long int) Onrows, fs, rsds, fs,
            tprep/1000.0,fs, texec/1000.0, fs, t1fetch/1000.0, fs, tfetch/1000.0, fs,
            (tprep+texec+tfetch)/1000.0,fs, sline, fs, eline);
        fflush(stdout);
    } else if (etab[eid].type == 'e' ) {        /* extract timing */
        etab[eid].k = (unsigned int)rsds ;      /* pass back record display size */
        etab[eid].mr = (unsigned long)tinit ;   /* pass back tinit */
        etab[eid].nrt = (unsigned long)(tprep+texec+tfetch) ;   /* pass back timig info */
        etab[eid].nbs = b ;                     /* pass back no. of read bytes */
    } else if ( otflg & 0002 ) {                /* print timing info */
        if ( ten >= 0 )                         /* not in SQL Interpreter */
            mfprintf(stderr, etab[eid].fso, "[%d.%d.%d]", tid, ten, scn);
        if ( t ) 
            mfprintf(stderr, etab[eid].fso, "--- %s row(s) %s", strmnum((double)Onrows,num,sizeof(num),0), rmess[t].mex);
        else
            mfprintf(stderr, etab[eid].fso, "--- command executed");
        mfprintf (stderr, etab[eid].fso, " in %ss ", strmnum((double)(tprep+texec+tfetch)/1000.0,num,sizeof(num),3));
        mfprintf (stderr, etab[eid].fso, "(prep %ss, ", strmnum((double)tprep/1000.0,num,sizeof(num),3));
        mfprintf (stderr, etab[eid].fso, "exec %ss, ", strmnum((double)texec/1000.0,num,sizeof(num),3));
        mfprintf (stderr, etab[eid].fso, "fetch %ss", strmnum((double)tfetch/1000.0,num,sizeof(num),3));
        mfprintf (stderr, etab[eid].fso, "/%ss)\n", strmnum((double)t1fetch/1000.0,num,sizeof(num),3));
    }
    etab[eid].nr = (unsigned long)Onrows ;      /* pass back no. of resulting rows */
    ret = t;    /* set return value */

    oexec_exit:
    if ( pad == 3 )
        pad = 2;
    /* Close & Free statement handle & Memory */
    if ( obuff )
        free ( obuff );
    if ( Oresp ) {
        for ( i = 0; i < Oncol ; i++) {
            if ( Oresp[i] )
                free (Oresp[i]);
        }
        free(Oresp);
    }
    if ( Olength ) {
        for ( i = 0; i < Oncol ; i++)
            free (Olength[i]);
        free(Olength);
    }
    if ( etab[eid].flg2 & 020000000 ) {
        (void)SQLFreeHandle(SQL_HANDLE_STMT, Ostmt);
    } else {
        (void)SQLFreeStmt(Ostmt, SQL_CLOSE);
        (void)SQLFreeStmt(Ostmt, SQL_UNBIND);
    }
    if ( Ors )
        free(Ors);
    if ( Odt )
        free(Odt);
    if ( Odd )
        free(Odd);
    if ( Ocnames )
        free(Ocnames);
    if ( fl )
        free(fl);
    if ( fa )
        free(fa);
    return(ret);    /* return type of executed command */
}

/* Oruncmd:
 *      executes commands/scripts allocated for a given thread.
 *
 *      tid: thread id
 *
 *      return: void
 */
#ifdef _WIN32
static DWORD WINAPI Oruncmd(void *tid)
#else
static void *Oruncmd(void *tid)
#endif
{
    register unsigned int i = 0;                /* loop variable */
    int eid = 0,            /* etab idx loop variable */
        n = 0,              /* command/script no for this thread */
        ret = 0,            /* return value */
        id = 0;             /* thread id */
    unsigned int lcd = 0;   /* delay to start next command within a thread */

    id = *(int *)tid;

#ifndef _WIN32
    pthread_cleanup_push(tunlock, (void *)&eid);
    pthread_cleanup_push(tclean, tid);
#endif

    for ( i = 0; go && ( ( nloop == 0 ) || ( i < nloop ) ) ; i++ ) {
        if ( ( f & 010000000 ) && i && ( f & 02000000 ) )
            etabshuffle();              /* re-shuffle etab[] for serial runs */
        if ( i && ld )                  /* loop delay */
            Sleep (ld);
        for ( eid = 0, n = 0; go && eid < no ; eid++ ) {
            if ( etab[eid].tbe != 1 )
                continue;               /* skip completed or started jobs */
            if ( f & 01000000000 ) {    /* if Dynamic Load Balancing */
                MutexLock(&dlbmutex);
                if ( etab[eid].tbe != 1 ) { /* re-check eid is not assigned already */
                    MutexUnlock(&dlbmutex);
                    continue;               /* skip completed or started jobs */
                }
                switch ( etab[eid].type ) {
                case 'C':
                case 'P':
                case 'D':
                case 'Z':
                    if ( thps[id].cr == 0 ) {   /* This thread is not connected with target */
                        MutexUnlock(&dlbmutex);
                        continue ;
                    } else {
                        etab[eid].id = id;      /* dynamically assign thread id */
                        etab[eid].tbe = 2;      /* job started */
                    }
                    break ;
                case 'c':
                case 'p':
                case 'd':
                    if ( thps[id].cr ) {    /* This thread is not connected with source */
                        MutexUnlock(&dlbmutex);
                        continue ;
                    }
                    /* FALLTHRU */
                default:
                    if ( etab[eid].tbe != 1 ) {
                        MutexUnlock(&dlbmutex);
                        continue ;
                    }
                    etab[eid].tbe = 2;      /* job started */
                    etab[eid].id = id;      /* dynamically assign thread id */
                    break ;
                }
                MutexUnlock(&dlbmutex);
                if ( f & 020000 )       /* if verbose */
                    fprintf(stderr, "odb [Oruncmd(%d)] - thread %d associated with eid=%d\n", __LINE__, id, eid);
            } else if ( etab[eid].id != id ) {  /* Static Load Balancing: skip jobs assigned to other threads */
                continue;
            } else {
                etab[eid].tbe = 2;      /* job started */
            }
            if ( thps[id].cd2 )         /* random delay between cd and cd2 */
                lcd = (unsigned int) ( thps[id].cd + rand() / ( RAND_MAX + 1.0 ) * ( thps[id].cd2 - thps[id].cd + 1 ) );
            else
                lcd = thps[id].cd;
            if ( n && lcd )
                Sleep (lcd);
            switch ( etab[eid].type ) {
            case 'x':       /* run commands */
                if ( tn > 1 && ! ( etab[eid].flg & 010000 ) )       /* if not quiet cmd on */
                    mfprintf(stderr, etab[eid].fso, "[%d.%d]%s: \'%s\'\n", 
                        id, n, ( etab[eid].flg & 0010 ? "Preparing" : "Executing" ), etab[eid].run);
                ret = Omexec ( id, eid, n++, 0, (SQLCHAR *)etab[eid].run, "", (char *)NULL, (char *)NULL);
                break;
            case 'l':       /* load (read file) thread */
#ifdef XML
                if ( etab[eid].xrt )
                    OloadX ( eid ) ;    /* XML Load */
                else 
#endif
                     if ( etab[eid].flg & 01000000 )
                    Oload ( eid ) ;     /* complex load */
                else if (etab[eid].jsonKey)
                    OloadJson ( eid ) ;
                else
                    Oload2 ( eid );     /* simple (fast) CSV load */
                break;
            case 'L':       /* load (insert to target) thread */
                (void)Oloadbuff( eid );
                break ;
            case 'C':       /* copy (insert to target) thread */
            case 'P':       /* pipe (insert to target) thread */
                do {
                    ret = Oloadbuff( eid );
                    if ( ret ) {
                        if ( f & 020000 ) 
                            fprintf(stderr, "odb [Oruncmd(%d)] - Oloadbuff[%d] returned %d\n", __LINE__, eid, ret);
                        fflush(stdout);
                        /* Disconnect */
                        if (!SQL_SUCCEEDED(Oret=SQLDisconnect(thps[etab[eid].id].Oc)))
                            Oerr(0, 0, __LINE__, thps[etab[eid].id].Oc, SQL_HANDLE_DBC);
                        etab[eid].mpre = 0 ;    /* do not re-run target mpre a second time */
                        /* Reconnect: Obuf[0] still contains the connection string built during the initial connect */
                        if (!SQL_SUCCEEDED(Oret=SQLDriverConnect(thps[etab[eid].id].Oc, (SQLHWND) NULL,
                                Obuf[0], SQL_NTS, Oocs, (SQLSMALLINT) sizeof(Oocs),
                                &Oocsl, (SQLUSMALLINT)SQL_DRIVER_NOPROMPT)))
                            Oerr(0, 0, __LINE__, thps[etab[eid].id].Oc, SQL_HANDLE_DBC);
                        /* Reallocate statement handle */
                        if (!SQL_SUCCEEDED(Oret=SQLAllocHandle(SQL_HANDLE_STMT,
                            thps[etab[eid].id].Oc, &thps[etab[eid].id].Os)))
                            Oerr(0, 0, __LINE__, thps[etab[eid].id].Oc, SQL_HANDLE_DBC);
                        if ( f & 020000 ) 
                            fprintf(stderr, "odb [Oruncmd(%d)] - Oloadbuff[%d] reconnected\n", __LINE__, ret);
                    } else {
                        break ;
                    }
                } while ( etab[eid].roe-- ) ;
                break;
            case 'c':       /* copy (read from source) thread */
            case 'p':       /* pipe (read from source) thread */
                do {
                    ret = Ocopy( eid );
                    if ( ret  && etab[eid].roe ) {
                        if ( f & 020000 ) 
                            fprintf(stderr, "odb [Oruncmd(%d)] - OCopy returned %d\n", __LINE__, ret);
                        fflush(stdout);
                        /* Disconnect */
                        if (!SQL_SUCCEEDED(Oret=SQLDisconnect(thps[etab[eid].id].Oc)))
                            Oerr(0, 0, __LINE__, thps[etab[eid].id].Oc, SQL_HANDLE_DBC);
                        /* Reset Parameters */
                        etab[eid].pre = 0 ; /* do not re-run target mpre a second time */
                        etab[eid].mpre = 0; /* do not re-run mpre on source */
                        /* Wait */
                        Sleep ( etab[eid].roedel ) ;
                        etab[eid].roedel *= 2 ;
                        /* Reconnect: Oics still contains the connection string built during the initial connect */
                        if (!SQL_SUCCEEDED(Oret=SQLDriverConnect(thps[etab[eid].id].Oc, (SQLHWND) NULL,
                                Oics, SQL_NTS, Oocs, (SQLSMALLINT) sizeof(Oocs),
                                &Oocsl, (SQLUSMALLINT)SQL_DRIVER_NOPROMPT)))
                            Oerr(0, 0, __LINE__, thps[etab[eid].id].Oc, SQL_HANDLE_DBC);
                        /* Reallocate statement handle */
                        if (!SQL_SUCCEEDED(Oret=SQLAllocHandle(SQL_HANDLE_STMT,
                            thps[etab[eid].id].Oc, &thps[etab[eid].id].Os)))
                            Oerr(0, 0, __LINE__, thps[etab[eid].id].Oc, SQL_HANDLE_DBC);
                        if ( f & 020000 ) 
                            fprintf(stderr, "odb [Oruncmd(%d)] - OCopy restarted\n", __LINE__);
                    } else {
                        break ;
                    }
                } while ( etab[eid].roe-- ) ;
                break;
            case 'e':       /* extract thread */
                Oextract( eid );
                break;
            case 'd':       /* diff (read from source) thread */
            case 'D':       /* diff (read from target) thread */
                Odiff( eid );
                break;
            case 'Z':       /* diff (compare) thread */
                Ocompare( eid );
                break;
            case 'f':       /* run SQL script thread */
            case 'F':       /* run SQL script thread (Win -S/-P) */
                ret = runsql ( id, eid, n++, etab[eid].run );
                break;
            }
            if ( f & 01000000000 ) {    /* if Dynamic Load Balancing */
                MutexLock(&dlbmutex);
                etab[eid].tbe = ( i < nloop -1 ) ? 1 : 0 ;
                MutexUnlock(&dlbmutex);
            } else {
                etab[eid].tbe = ( i < nloop -1 ) ? 1 : 0 ;
            }
            if ( ret && etab[eid].flg & 0004 ) {
                i = nloop;  /* stop loops */
                break;
            }
        }
    }
#ifdef _WIN32
    tclean(tid);
    return((DWORD)0);
#else
    pthread_cleanup_pop(1);
    pthread_cleanup_pop(2);
    if(!(f & 02000) && !( f & 010000000 )) 
        pthread_exit((void *)NULL);
    return((void *)NULL);
#endif
}

/* sigcatch:
 *      print a message with the catched signal and exit
 *
 *      sig: integer used as exit status (received signal)
 *
 *      return: void
 */
static void sigcatch(int sig)
{
    int i = 0;

    switch ( sig ) {
#ifndef _WIN32
    case SIGALRM:
        fprintf(stderr, "odb [sigcatch(%d)] - Received SIGALRM (timeout) after %us. Exiting\n", 
            __LINE__, w);
        break;
#endif
    case SIGINT:
        fprintf(stderr, "odb [sigcatch(%d)] - Received SIGINT. Exiting\n", __LINE__);
        go = 0;
        break;
    case SIGTERM:
        fprintf(stderr, "odb [sigcatch(%d)] - Received SIGTERM. Exiting\n", __LINE__);
        go = 0;
        break;
    }
#ifdef _WIN32
    gclean();
    exit ( EX_SIGNAL );
#else 
    if ( tn == 1 ) { /* single threaded */
        gclean();
    } else {
        for ( i = 0 ; i < tn ; i++ ) {
            if ( !pthread_kill(thid[i], 0) ) {  /* If this thread is alive... */
                if ( pthread_cancel(thid[i]) )  /* ... cancel it */
                    fprintf(stderr, "odb [sigcatch(%d)] - Error canceling thread %d: [%d] %s\n",
                                    __LINE__, i, errno, strerror(errno) );
            }
        }
    }
    exit(EX_SIGNAL);
#endif
}

/* tclean: Thread cleanup routine
 *
 *      tid: thread identifier
 *      return: void
 */
static void tclean(void *tid)
{
    int id = tid ? *(int *)tid : 0 ; 
    
    if ( ! ( f & 02000 ) ) {    /* if not the Interpreter ... */
        if ( f & 020000 )       /* if verbose */
            fprintf(stderr, "odb: thread %d closing connection...\n", id);
        if ( thps[id].cr <= 0 ) {       /* if cr > 0 connection handle belongs to another thread */
            if (thps[id].Oc) {          /* if connected... */
                if (!SQL_SUCCEEDED(SQLDisconnect(thps[id].Oc)))
                    Oerr(-1, id, __LINE__, thps[id].Oc, SQL_HANDLE_DBC);
                (void)SQLFreeHandle(SQL_HANDLE_DBC, thps[id].Oc); /* free conn handle */
                thps[id].Oc = 0;
            }
        }
        if ( f & 020000 )       /* if verbose */
            fprintf(stderr, "odb: thread %d is ending...\n", id);
    }
}

#ifndef _WIN32
/* tunlock: Thread Mutexes & Condition variables unlock. This function is actived
 *          when a multithreaded program using condition varables and mutexes is
 *          interrupted via SIGINT or SIGQUIT. It unlock mutexes and wakeup waiting
 *          threads
 *
 *      eid: execution table identifier
 *      return: void
 */
static void tunlock(void *eid)
{
    int id = *(int *)eid ; 
    
    if ( go == 0 ) {    /* MT instance interrupted via SIGINT/SIGQUIT */
        switch(etab[id].type) {
        case 'e':
            MutexUnlock(&etab[etab[id].parent].pmutex);
            break;
        case 'c':
        case 'd':
            MutexUnlock(&etab[id].pmutex);
            CondWakeAll(&etab[id].pcvc);
            break;
        case 'C':
        case 'D':
            MutexUnlock(&etab[etab[id].parent].pmutex);
            CondWakeAll(&etab[etab[id].parent].pcvc);
            break;
        case 'Z':
            MutexUnlock(&etab[etab[id].child].pmutex);
            CondWakeAll(&etab[etab[id].child].pcvc);
            break;
        }
    }
}
#endif

/* gclean:  Global clean routine
 *
 *      return: void
 */
static void gclean(void)
{
    int i = 0;  /* loop variable */
    int j = 0;  /* loop variable */
    struct ovar *p = 0; 

    if ( fdmp && fdmp != stdout )   /* close dump file */
        fclose ( fdmp ) ;
    MutexDestroy(&dlbmutex);
#ifndef _WIN32
    if ( thid )         /* Free thread ID array */
        free (thid);
#endif
#ifdef HDFS
    if ( hdfs_handle )
        dlclose ( hdfs_handle );
#endif
    for ( i = 0 ; i < no ; i++ )    /* Free memory */
        switch ( etab[i].type ) {
        case 'e':               /* ...(e) allocated in etabadd */
            if ( i == etab[i].parent ) { /* the "parent" thread ... */
                free( etab[i].src );    /* ... free .src */
                if ( etab[i].ps )       /* ps but not multi */
                    MutexDestroy(&etab[i].pmutex);
                if (i == 0)              /* fix jira-2029, the groupt mutex is only set in etab[0] */
                    MutexDestroy(&etab[i].gmutex);
            }
            break;
        case 'l':               /* ...(l) allocated in etabadd */
            if ( etab[i].ps ) {
                MutexDestroy(&etab[i].pmutex);
                CondDestroy ( &etab[i].pcvp );
                CondDestroy ( &etab[i].pcvc );
            }
            break;
        case 'd':               /* ...(d) allocated in etabadd */
            if ( etab[etab[i].k].em ) { /* only grand parent frees .key */
                for ( j = 0 ; j < etab[etab[i].k].pc ; j++ )
                    free(etab[etab[i].k].key[j]);
                etab[etab[i].k].em = 0 ;
                if (i == 0)              /* fix jira-2029, the groupt mutex is only set in etab[0] */
                    MutexDestroy(&etab[i].gmutex);
            }
            if ( etab[etab[i].k].src ) {    /* only grand parent frees .src */
                free(etab[etab[i].k].src);
                etab[etab[i].k].src = 0 ;
                if (i == 0)              /* fix jira-2029, the groupt mutex is only set in etab[0] */
                    MutexDestroy(&etab[i].gmutex);
            }
            break;
        case 'D':               /* ...(D) allocated in etabadd */
            MutexDestroy( &etab[i].pmutex );
            CondDestroy ( &etab[i].pcvp );
            CondDestroy ( &etab[i].pcvc );
            break;
        case 'c':               /* ...(c) allocated in etabadd */
            if ( i == (int)etab[i].k ) {        /* grand parent */
                if ( etab[i].sbl )
                    free ( etab[i].sb );
                free ( etab[i].src );
                MutexDestroy( &etab[i].pmutex );
                if (i == 0)              /* fix jira-2029, the groupt mutex is only set in etab[0] */
                    MutexDestroy(&etab[i].gmutex);
                CondDestroy ( &etab[i].pcvp );
                CondDestroy ( &etab[i].pcvc );
            }
            break;
        case 'F':               /* ...(F) allocated during -S/-P */
            free(etab[i].run);
            /* FALLTHRU */
        case 'f':
        case 'I':
            if ( etab[i].ns != ns )     /* nullstr allocated */
                free ( etab[i].ns );
            break;
        }
    if ( etab )                     /* Free etab array */
        free ( etab );
    for ( i = 0 ; i < tn ; i++ ) {  /* Free vars chains */
        for ( p = thps[i].tva ; p && p->next ; p = p->next ) ;
        while ( p ) {
            free ( p->value );
            p = p->prev;
            if ( p )
                free ( p->next );
        }
    }
    (void)SQLFreeHandle(SQL_HANDLE_ENV, Oenv);

    for (int i = 0; i < nGlobalPointers; ++i) /* free globalPointers buffer */
    {
        free(globalPointers[i]);
    }
    free(globalPointers);
}

/* cancel: executed in interactive mode when ^C is pressed:
 *      - send a SQLCancel to the statement handle thps[0].Os
 *
 *      sig: received signal
 *
 *      return: void
 */
static void cancel(int sig)
{
  printf("odb canceling statement... ");
  fflush(stdout);
  if ( thps[0].Oc )
    if (!SQL_SUCCEEDED(Oret=SQLCancel(thps[0].Os)))
      Oerr(0, 0, __LINE__, thps[0].Os, SQL_HANDLE_STMT);
  printf("%s", prompt);
  fflush(stdout);
  (void)signal(sig, cancel);  /* reset SIGINT handling to cancel() */
}

/* Otinfo:
 *      print table DDL
 *
 *      Oca: Catalog name
 *      Osc: Schema name
 *      Ota: Object name
 *      ddl: 0 = column output, 1 = ddl output, 2 = ddl output with CHAR/VARCHAR x 4
 *
 *      return: void
 */
static void Otinfo(SQLCHAR *Oca, SQLCHAR *Osc, SQLCHAR *Ota, char ddl)
{
    int mc=6,                   /* Max Column Name size */
        mt=4,                   /* Max Type_Name column size */
        md=7,                   /* Max Default Value column size */
        mi=5,                   /* Max Index column size */
        i=0,                    /* loop variable */
        j=0;                    /* loop variable */
    struct tstruct {
        char cname[MAXCOL_LEN]; /* Column name */
        char ctype[64];         /* Column type */
        char cdef[64];          /* Default Value */
        char cnull[4];          /* Nullable (YES/NO) */
        char kseq[4];           /* Index type (PRI/UNI/MUL)*/
        SQLSMALLINT Otype;      /* ODBC data type */
        struct tstruct *prev;   /* pointer to the prev structure */
        struct tstruct *next;   /* pointer to the next structure */
    } *tdef=0, *p=0, *pp=0;     /* Table Structure First/Current/Previous Element pointers */
    char head[128];             /* table header/footer */
    SQLHSTMT Ostmt;             /* Statement Handle. Otinfo may be called from Olist() within
                                   a loop based on the thps[0].Os statement handle. So we need
                                   "our own" statement handle in Otinfo() */

    /* Allocate statement handle */
    if (!SQL_SUCCEEDED(Oret=SQLAllocHandle(SQL_HANDLE_STMT,
            thps[0].Oc, &Ostmt))){
        Oerr(0, 0, __LINE__, thps[0].Oc, SQL_HANDLE_DBC);
        return;
    }

    /* Execute SQLColumns to get object list */
    if (!SQL_SUCCEEDED(Oret=SQLColumns(Ostmt, 
            Oca[0] ? Oca : f & 04000000000 ? NULL : (SQLCHAR *)"", 
            Oca[0] ? SQL_NTS : 0,
            Osc[0] ? Osc : (SQLCHAR *)"", SQL_NTS,
            Ota[0] ? Ota : (SQLCHAR *)"", SQL_NTS,
            NULL, 0))) {
        Oerr(0, 0, __LINE__, Ostmt, SQL_HANDLE_STMT);
        exstat = EX_ODBCERR ;
        goto otinfo_exit;
    }

    /* Bind result set:
        field  4: COLUMN_NAME      VARCHAR  -> Obuf[0]
        field  5: DATA_TYPE        SMALLINT -> Osio[0]
        field  6: TYPE_NAME        VARCHAR  -> Obuf[1]
        field  7: COLUMN_SIZE      INTEGER  -> Odps
        field  9: DECIMAL_DIGITS   SMALLINT -> Osio[1]
        field 11: NULLABLE         SMALLINT -> Osio[2]
        field 13: COLUMN_DEF       VARCHAR  -> Obuf[2]
     */
    if (!SQL_SUCCEEDED(Oret = SQLBindCol(Ostmt, (SQLUSMALLINT) 4, SQL_C_CHAR, 
        Obuf[0], (SQLLEN)sizeof(Obuf[0]), &Olen[0]))) {
        Oerr(0, 0, __LINE__, Ostmt, SQL_HANDLE_STMT);
        exstat = EX_ODBCERR ;
        goto otinfo_exit;
    }
    if (!SQL_SUCCEEDED(Oret = SQLBindCol(Ostmt, (SQLUSMALLINT) 5, SQL_C_SSHORT, 
        &Osio[0], (SQLLEN)sizeof(Osio[0]), &Olen[1]))) {
        Oerr(0, 0, __LINE__, Ostmt, SQL_HANDLE_STMT);
        exstat = EX_ODBCERR ;
        goto otinfo_exit;
    }
    if (!SQL_SUCCEEDED(Oret = SQLBindCol(Ostmt, (SQLUSMALLINT) 6, SQL_C_CHAR, 
        Obuf[1], (SQLLEN)sizeof(Obuf[1]), &Olen[2]))) {
        Oerr(0, 0, __LINE__, Ostmt, SQL_HANDLE_STMT);
        exstat = EX_ODBCERR ;
        goto otinfo_exit;
    }
    if (!SQL_SUCCEEDED(Oret = SQLBindCol(Ostmt, (SQLUSMALLINT) 7, SQL_C_SLONG, 
        &Odps, (SQLLEN)sizeof(Odps), &Olen[3]))) {
        Oerr(0, 0, __LINE__, Ostmt, SQL_HANDLE_STMT);
        exstat = EX_ODBCERR ;
        goto otinfo_exit;
    }
    if (!SQL_SUCCEEDED(Oret = SQLBindCol(Ostmt, (SQLUSMALLINT) 9, SQL_C_SSHORT, 
        &Osio[1], (SQLLEN)sizeof(Osio[1]), &Olen[4]))) {
        Oerr(0, 0, __LINE__, Ostmt, SQL_HANDLE_STMT);
        exstat = EX_ODBCERR ;
        goto otinfo_exit;
    }
    if (!SQL_SUCCEEDED(Oret = SQLBindCol(Ostmt, (SQLUSMALLINT) 11, SQL_C_SSHORT, 
        &Osio[2], (SQLLEN)sizeof(Osio[2]), &Olen[5]))) {
        Oerr(0, 0, __LINE__, Ostmt, SQL_HANDLE_STMT);
        exstat = EX_ODBCERR ;
        goto otinfo_exit;
    }
    if (!SQL_SUCCEEDED(Oret = SQLBindCol(Ostmt, (SQLUSMALLINT) 13, SQL_C_CHAR, 
        Obuf[2], (SQLLEN)sizeof(Obuf[2]), &Olen[6]))) {
        Oerr(0, 0, __LINE__, Ostmt, SQL_HANDLE_STMT);
        exstat = EX_ODBCERR ;
        goto otinfo_exit;
    }

    /* Fill tdef structure with basic table info */
    while (SQL_SUCCEEDED(Oret=SQLFetch(Ostmt))) {
        if ( ( pp = malloc ( sizeof(struct tstruct) ) ) == (void *)NULL ) {
            fprintf(stderr, "odb [Otinfo(%d)] - Error allocating tstruct element\n", __LINE__);
            goto otinfo_exit;
        }
        if ( i++ ) {
            pp->prev = p;
            p->next = pp;
        } else {
            tdef = pp;
            tdef->prev = (struct tstruct *)NULL;
        }
        p = pp;
        p->kseq[0] = '\0';
        p->Otype = Osio[0] ;    /* save ODBC data type */
        strmcpy(p->cname, (char *)Obuf[0], sizeof(p->cname));
        if ( (int)Olen[0] > mc )
            mc = (int)Olen[0];
        switch (Osio[0]) {
        case SQL_CHAR:
        case SQL_VARCHAR:
        case SQL_LONGVARCHAR:
        case SQL_BINARY:
        case SQL_VARBINARY:
        case SQL_LONGVARBINARY:
            if ( ddl == 2 )
                Odps *= mult ;  
            snprintf(p->ctype, sizeof(p->ctype), "%s(%d)",
                (char *)Obuf[1], (int)Odps);
            break ;
        case SQL_WCHAR:
        case SQL_WVARCHAR:
        case SQL_WLONGVARCHAR:
            if ( ddl == 2 )
                Odps *= wmult ; 
            snprintf(p->ctype, sizeof(p->ctype), "%s(%d)",
                (char *)Obuf[1], (int)Odps);
            break ;
        case SQL_FLOAT:
            snprintf(p->ctype, sizeof(p->ctype), "%s(%d)",
                (char *)Obuf[1], (int)Odps);
            break;
        case SQL_NUMERIC:
        case SQL_DECIMAL:
            snprintf(p->ctype, sizeof(p->ctype), "%s(%d,%d)",
                (char *)Obuf[1], (int)Odps, (int)Osio[1]);
            break;
        case SQL_TIME:
        case SQL_TIMESTAMP:
        case SQL_TYPE_TIME:
        case SQL_TYPE_TIMESTAMP:
            snprintf(p->ctype, sizeof(p->ctype), "%s(%d)",
                (char *)Obuf[1], (int)Osio[1]);
            break;
        default:
            strmcpy(p->ctype, (char *)Obuf[1], sizeof(p->ctype));
            break;
        }
        j = (int)strlen(p->ctype);
        if ( j > mt )
            mt = j;
        strmcpy(p->cdef, (char *)Obuf[2], sizeof(p->cdef));
        Obuf[2][0] = '\0';  /* clean Default buffer */
        if ( (int)Olen[6] > md )
            md = (int)Olen[6];
        strmcpy(p->cnull, Osio[2] ? "YES" : "NO", sizeof(p->cnull));
    }
    if ( !i ) {
        fprintf(stderr, "odb [Otinfo(%d)] - Cannot access %s.%s.%s\n", __LINE__, Oca, Osc, Ota);
        goto otinfo_exit;
    }
    p->next = (struct tstruct *)NULL;

    /* Clean statement handle */
    (void)SQLFreeStmt(Ostmt, SQL_CLOSE);
    (void)SQLFreeStmt(Ostmt, SQL_UNBIND);

    /* Execute SQLPrimaryKeys */
    if (!SQL_SUCCEEDED(Oret=SQLPrimaryKeys(Ostmt, 
            Oca[0] ? Oca : (SQLCHAR *)NULL, SQL_NTS, 
            Osc[0] ? Osc : (SQLCHAR *)NULL, SQL_NTS,
            Ota[0] ? Ota : (SQLCHAR *)NULL, SQL_NTS))) {
        Oerr(0, 0, __LINE__, Ostmt, SQL_HANDLE_STMT);
        goto otinfo_exit;
    }
    
    /* Bind result set:
        field  4: COLUMN_NAME    VARCHAR  -> Obuf[0]
        field  5: KEY_SEQ        SMALLINT -> Osio[0]
     */
    if (!SQL_SUCCEEDED(Oret = SQLBindCol(Ostmt, (SQLUSMALLINT) 4, SQL_C_CHAR, 
        Obuf[0], sizeof(Obuf[0]), &Olen[0]))) {
        Oerr(0, 0, __LINE__, Ostmt, SQL_HANDLE_STMT);
        goto otinfo_exit;
    }
    if (!SQL_SUCCEEDED(Oret = SQLBindCol(Ostmt, (SQLUSMALLINT) 5, SQL_C_SSHORT, 
        &Osio[0], sizeof(Osio[0]), &Olen[1]))) {
        Oerr(0, 0, __LINE__, Ostmt, SQL_HANDLE_STMT);
        goto otinfo_exit;
    }

    /* Fill tdef structure with PK info */
    Obuf[1][0] = '\0';
    while (SQL_SUCCEEDED(Oret=SQLFetch(Ostmt))) {
        for ( p = tdef; p ; p = p->next ) {
            if ( !strmicmp(p->cname, (char *)Obuf[0], 0) ) {
                snprintf(p->kseq, sizeof(p->kseq),"%d", (int)Osio[0]);
                if ( Obuf[1][0] )
                    strmcat ( (char *)Obuf[1], ",", sizeof(Obuf[1]), 0);
                strmcat ( (char *)Obuf[1], (char *)Obuf[0], sizeof(Obuf[1]), 0);
                break;
            }
        }
    }

    /* Print CREATE TABLE stmt if ddl = 1 */
    if ( ddl ) {
        mfprintf(etab[0].fo, etab[0].fso, "%s %s%s%s%s\"%s\" (\n", 
            islower(Ota[0]) ? "create table" : "CREATE TABLE",
            Oca[0] ? (char *)Oca : "", Oca[0] ? "." : "",
            Osc[0] ? (char *)Osc : "", Osc[0] ? "." : "", (char *)Ota);
    } else {    /* Print Table Structure Header*/
        for ( i = 1, head[0] = '+' ; i <= mc ; head[i++]='-');
        for ( j = 0, head[i++] = '+' ; j < mt ; j++, head[i++]='-');
        for ( j = 0, head[i++] = '+' ; j < 29 ; j++, head[i++]='-');
        for ( j = 0, head[i++] = '+' ; j < 4 ; j++, head[i++]='-');
        for ( j = 0, head[i++] = '+' ; j < md ; j++, head[i++]='-');
        for ( j = 0, head[i++] = '+' ; j < mi ; j++, head[i++]='-');
        head[i++] = '+';
        head[i] = '\0';
        mfprintf(etab[0].fo, etab[0].fso, "Describing: ");
        if ( Oca[0] )
            mfprintf(etab[0].fo, etab[0].fso, "%s.", Oca);
        if ( Osc[0] )
            mfprintf(etab[0].fo, etab[0].fso, "%s.", Osc);
        mfprintf(etab[0].fo, etab[0].fso, "%s\n", Ota);
    }

    for ( i = 0, p = tdef; p; p = p->next ) {
        if ( ddl ) {
            mfprintf(etab[0].fo, etab[0].fso, "\t%c%s %s%s%s%s\n",
                (i++) ? ',' : ' ', p->cname, p->ctype,
                p->cnull[0] == 'N' ? ( islower ( Ota[0] ) ? " not null" : " NOT NULL" ) : "",
                p->cdef[0] ? ( islower ( Ota[0] ) ? " default " : " DEFAULT " ) : "",
                p->cdef[0] ? p->cdef : "");
        } else {
            if ( i++ == 0 ) {
                mfprintf(etab[0].fo, etab[0].fso, "%s\n|%-*s|%-*s|%-29s|NULL|%-*s|%-*s|\n%s\n",
                    head, mc,"COLUMN", mt, "TYPE","ODBC DATA TYPE", md, "DEFAULT", mi, "PKEY", head); 
            }
            mfprintf(etab[0].fo, etab[0].fso, "|%-*s|%-*s|%-29s|%-4s|%-*s|%-*s|\n",
                mc, p->cname, mt, p->ctype, expandtype(p->Otype),p->cnull, md, p->cdef, mi, p->kseq);
        }
    }
    if ( ddl ) {
        if ( Obuf[1][0] )
            mfprintf(etab[0].fo, etab[0].fso, "\t,%s (%s)\n",
                islower ( Ota[0] ) ? "primary key" : "PRIMARY KEY", Obuf[1]);
        mfprintf(etab[0].fo, etab[0].fso, ");\n");
    } else {
        mfprintf(etab[0].fo, etab[0].fso, "%s\n\n", head);
    }

    otinfo_exit:
    /* free tdef memory */
    for ( p = tdef; p && p->next ; p = p->next ); 
    while ( p ) {
        if ( p->prev ) {
            p = p->prev;
            free ( p->next );
        } else {
            free ( p );
            break;
        }
    }

    /* Free statement handle */
    (void)SQLFreeHandle(SQL_HANDLE_STMT, Ostmt);
}

/* Olist:
 *      perform actions (default print) on schema objects
 *
 *      Oca: Catalog name | SQL_ALL_CATALOGS
 *      Osc: Schema name | SQL_ALL_SCHEMAS
 *      Ota: object name
 *      otype:  see struct Otypes[] (type field)
 *
 *      return: void
 */
static void Olist(SQLCHAR *Oca, SQLCHAR *Osc, SQLCHAR *Ota, char otype)
{
    SQLCHAR *Otype = 0,             /* ODBC Object Type */
            Ostn[3][MAXOBJ_LEN];    /* ODBC to save schema/table/type name */
    SQLLEN Ostnl[3];                /* ODBC schema/table name/type length */
    register unsigned int i=0;      /* loop variable */
    char so[MAXOBJ_LEN * 2];        /* contains schema.object */
    SQLRETURN Or=0;                 /* ODBC return value */
    SQLHSTMT Ostmt;                 /* Statement Handle */

    /* if nocatnull is set */
    if ( Oca &&  Oca[0] == '\0' && f & 04000000000 )
        Oca = NULL ;

    /* Allocate statement handle */
    if (!SQL_SUCCEEDED(Or=SQLAllocHandle(SQL_HANDLE_STMT,
            thps[0].Oc, &Ostmt))){
        Oerr(0, 0, __LINE__, thps[0].Oc, SQL_HANDLE_DBC);
        return;
    }

    /* Define the object type */
    if ( otype != 'A' ) {
        for ( i = 0 ; i < sizeof(Otypes)/sizeof(struct Otype) ; i++ )
            if ( otype == Otypes[i].type )
                Otype = Otypes[i].Otype;
        if ( !Otype ) {     /* type not found */
            fprintf(stderr, "odb [Olist(%d)] - Invalid object type >%c<\n", 
                __LINE__, otype);
            return;
        }
    }

    if ( otype == 's' || otype == 'c' ) {           /* List schemas / catalogs */
        if (!SQL_SUCCEEDED(Oret=SQLTables(Ostmt,
                Oca, SQL_NTS,
                Osc, SQL_NTS,
                (SQLCHAR *)"", 0, 
                (SQLCHAR *)"", 0))) {
            Oerr(0, 0, __LINE__, Ostmt, SQL_HANDLE_STMT);
            goto olist_exit;
        }
        if (!SQL_SUCCEEDED(Oret = SQLBindCol(Ostmt, (SQLUSMALLINT) (Oca[0]?1:2),
                SQL_C_CHAR, Ostn[0], (SQLLEN)MAXOBJ_LEN, &Ostnl[0]))) {
            Oerr(0, 0, __LINE__, Ostmt, SQL_HANDLE_STMT);
            goto olist_exit;
        }
        while (SQL_SUCCEEDED(Oret=SQLFetch(Ostmt))) {
            if ( nc > 1 ) {
                mfprintf(etab[0].fo, etab[0].fso, "%-*s", cs, (char *)Ostn[0]);
                if ( ! ( ++i % nc ) ) {
                    mfputc(etab[0].fo, etab[0].fso, '\n');
                }
            } else {
                mfprintf(etab[0].fo, etab[0].fso, "%s\n", (char *)Ostn[0]);
            }
        }
        if ( nc > 1 && i % nc ) {
            mfputc(etab[0].fo, etab[0].fso, '\n');
        }
    } else {
        if (!SQL_SUCCEEDED(Oret=SQLTables(Ostmt,
                Oca, SQL_NTS, 
                Osc, SQL_NTS,
                Ota, SQL_NTS,
                otype == 'A' ? Oaot : Otype , SQL_NTS ))) {
            Oerr(0, 0, __LINE__, Ostmt, SQL_HANDLE_STMT);
            goto olist_exit;
        }
        
        /* Bind schema name */
        if (!SQL_SUCCEEDED(Oret = SQLBindCol(Ostmt, (SQLUSMALLINT) 2,
                SQL_C_CHAR, Ostn[0], (SQLLEN)sizeof(Ostn[0]), &Ostnl[0]))) {
            Oerr(0, 0, __LINE__, Ostmt, SQL_HANDLE_STMT);
            goto olist_exit;
        }

        /* Bind object name */
        if (!SQL_SUCCEEDED(Oret = SQLBindCol(Ostmt, (SQLUSMALLINT) 3,
                SQL_C_CHAR, Ostn[1], (SQLLEN)sizeof(Ostn[1]), &Ostnl[1]))) {
            Oerr(0, 0, __LINE__, Ostmt, SQL_HANDLE_STMT);
            goto olist_exit;
        }
        
        /* Bind object type */
        if (!SQL_SUCCEEDED(Oret = SQLBindCol(Ostmt, (SQLUSMALLINT) 4,
                SQL_C_CHAR, Ostn[2], (SQLLEN)sizeof(Ostn[2]), &Ostnl[2]))) {
            Oerr(0, 0, __LINE__, Ostmt, SQL_HANDLE_STMT);
            goto olist_exit;
        }
        
        if ( otype == 'T' ) {
            while (SQL_SUCCEEDED(Oret=SQLFetch(Ostmt))){
                Otinfo(Oca?Oca:(SQLCHAR *)"", Ostn[0], Ostn[1], 0);
            }
        } else if ( otype == 'D' ) {
            while (SQL_SUCCEEDED(Oret=SQLFetch(Ostmt))){
                Otinfo(Oca?Oca:(SQLCHAR *)"", Ostn[0], Ostn[1], 1);
            }
        } else if ( otype == 'U' ) {
            while (SQL_SUCCEEDED(Oret=SQLFetch(Ostmt))){
                Otinfo(Oca?Oca:(SQLCHAR *)"", Ostn[0], Ostn[1], 2);
            }
        } else if ( otype == 'A' ) {
            while (SQL_SUCCEEDED(Oret=SQLFetch(Ostmt))) {
                if ( Ostnl[0] ) /* if we have a schema name */
                    mfprintf(etab[0].fo, etab[0].fso, "%s %s.%s\n", (char *)Ostn[2], (char *)Ostn[0], (char *)Ostn[1]);
                else
                    mfprintf(etab[0].fo, etab[0].fso, "%s %s\n", (char *)Ostn[2], (char *)Ostn[1]);
            }
        } else {
            while (SQL_SUCCEEDED(Oret=SQLFetch(Ostmt))) {
                snprintf(so, sizeof(so), "%s.%s", Ostn[0], Ostn[1]);
                if ( nc > 1 ) {
                    mfprintf(etab[0].fo, etab[0].fso, "%-*s", cs, so);
                    if ( ! ( ++i % nc ) ) {
                        mfputc(etab[0].fo, etab[0].fso, '\n');
                    }
                } else {
                    mfprintf(etab[0].fo, etab[0].fso, "%s\n", so);
                }
            }
            if ( nc > 1 && i % nc )
                fputc('\n', etab[0].fo);
        }
    }

    olist_exit:
    /* Clean statement handle */
    (void)SQLFreeHandle(SQL_HANDLE_STMT, Ostmt);
}

/* Odinfo:
 *      print odb version, data source, driver and database information 
 *
 *      return: void
 */
static void Odinfo()
{
    int k = 0;                  /* loop variable */
    SQLUINTEGER Ops = 0;        /* Connection Packet Size */

    mfprintf (etab[0].fo, etab[0].fso, "\t[%s]\n\tBuild: %s [%s %s]\n", odbid, ODBBLD, __DATE__, __TIME__);
    for (k=0; k< (int)(sizeof(Oinfo)/sizeof(struct info)); k++) {
        if (SQL_SUCCEEDED(Oret=SQLGetInfo(thps[0].Oc, Oinfo[k].Oid, 
                (SQLPOINTER)Obuf[0], (SQLSMALLINT)sizeof(Obuf[0]), NULL))) {
            mfprintf(etab[0].fo, etab[0].fso, "\t%-45s: %s\n", Oinfo[k].desc, (char *)Obuf[0]);
            if(Oret == SQL_SUCCESS_WITH_INFO)
                fprintf(stderr, "odb [Odinfo(%d)] - Warning data truncation\n", __LINE__);
        }
        if(!SQL_SUCCEEDED(Oret))
            Oerr(0, 0, __LINE__, thps[0].Oc, SQL_HANDLE_DBC);
    }
    if (SQL_SUCCEEDED(Oret=SQLGetConnectAttr(thps[0].Oc, SQL_ATTR_PACKET_SIZE, 
            (SQLPOINTER)&Ops, SQL_IS_UINTEGER, 0))) {
        mfprintf(etab[0].fo, etab[0].fso, "\t%-45s: %u\n", "Connection Packet Size (SQL_ATTR_PACKET_SIZE)", (unsigned int)Ops);
    }
}

/* etabadd:
 *      add a new entry in the etab[] array allocating space in block of
 *      ETAB_CHUNK if needed.
 *
 *      type (I): field to add to etab[].type;
 *      run (I): field to add to etab[].run;
 *      id (I): field to add to etab[].id;
 *
 *      return: void
 */
static void etabadd(char type, char *run, int id)
{
    size_t ll = 0,  /* run line length */
           lsql = 0,/* SQL file (@) length */
           len = 0; /* fread size */
    unsigned int j = 0, /* loop variable */
        l = 0,      /* loop variable */
        n = 0,      /* loop variable */
        o = 0,      /* loop variable */
        i = 0,      /* loop variable */
        ldbt = 0,   /* local dbt */
        bl = 0,     /* file name length befor writing multi file no */
        np = 0,     /* no parse via SQLTable() flag: 0=parse, 1=no_parse */
        ret = 0;    /* parseopt return value */
    int or = no;    /* Original etab[] entry (used fo replications) */
    SQLHSTMT Os=0,  /* ODBC Statement handle to analyze source table(s) */
             Os1=0; /* ODBC statement handle to get table partitions */
    SQLCHAR Ostn[3][MAXOBJ_LEN];    /* ODBC to save schema/table/type name */
    SQLLEN Ostnl[4];/* ODBC schema/table name/type & other lengths */
    SQLCHAR Omn[64];/* ODBC to store min splitby as string */
    SQLCHAR Omx[64];/* ODBC to store max splitby as string */
    long sbmin=0;   /* splitby min value */
    long sbmax=0;   /* splitby max value */
    long d = 0;     /* splitby delta */
    char buff[256]; /* generic buffer */
    char buff2[16]; /* generic buffer */
    char tabn[512]; /* buffer containing table name to expand */
    struct tm *dt;  /* used to generate date and time strings */
    time_t pt;      /* used to generate date and time strings */
    FILE *fl = 0;   /* List of tables file pointer */
    FILE *fsql = 0; /* to read file containing custom SQL */
    struct tdesc to = { 0 };    /* used to sort etab[].td[] structure array based on order-by cols */
    struct execute edef = { 0 };/* default value structure for input file based etab entries */

    etabnew ( -1 );
    etab[no].type = type;
    etab[no].id = id;
    
    strcpy(etab[no].loadcmd, "IN\0");
    if (type == 'l' || type == 'e' || type == 'c' || type == 'd' || type == 'p') {
        etab[no].flg |= 016000000400;   /* print mark every rowset, print diff ICD */
        etab[no].tbe = 1;
        if ( !etab[no].buffsz )         /* set default buffer size */
            etab[no].buffsz = RWBUFF_LEN ;
        etab[no].flg2 |= 0200000;       /* bind = auto */
        edef = etab[no];                /* save default values */

        /* Parse command line parameters */
        if ( ( ret = parseopt ( type, run, (unsigned int)strlen(run), 0 ) ) == 1 ) {
            goto etabadd_exit;
        }

        /* Check for mandatory arguments */
        switch ( etab[no].type ) {
        case 'l':
            if ( etab[no].flg2 & 0020 ) {   /* "show" mode */
                etab[no].r = 1;             /* rowset = 1 */
                etab[no].ps = 0;            /* parallel streams = 0 */
                etab[no].flg &= ~0002;      /* truncate off */
                etab[no].flg &= ~0200000;   /* ifempty off */
            }
            if ( !etab[no].src  ) {
                fprintf(stderr, "odb [etabadd(%d)] - Error: missing \'src\' operator\n", __LINE__ );
                goto etabadd_exit;
            }
            if ( !etab[no].tgt ) {
                fprintf(stderr, "odb [etabadd(%d)] - Error: missing \'tgt\' operator\n", __LINE__ );
                goto etabadd_exit;
            }
            else
            {
                /* Allocate new buf to save Catalog/Schema/Table */
                char *ptrCST = (char *)malloc(strlen(etab[no].tgt) + 1);
                strcpy(ptrCST, etab[no].tgt);
                addGlobalPointer(ptrCST);

                /* Split Catalog/Schema/Table */
                splitcso(ptrCST, etab[no].Ocso, 1);
                if (f & 04000000000)  /* no catalog as null */
                    etab[no].Ocso[0] = '\0';
            }
            if (etab[no].k && (etab[no].mcfs || etab[no].mcrs)) {
                fprintf(stderr, "odb [etabadd(%d)] - Error: neither 'mcfs' nor 'mcrs' support skip lines\n", __LINE__);
                goto etabadd_exit;
            }

            if ( etab[no].flg2 & 0400000000 ) { /* user defined iobuff */
                if ( !etab[no].iobuff ) {       /* iobuff set to zero */
                    fprintf(stderr, "odb [etabadd(%d)] - Error: cannot set iobuff to zero for loads\n", __LINE__ );
                    goto etabadd_exit;
                }
            }
            break;
        case 'e':
            if ( !etab[no].src && !etab[no].sql ) {
                fprintf(stderr, "odb [etabadd(%d)] - Error: missing \'src\' or \'sql\' operators\n", __LINE__);
                goto etabadd_exit;
            }
            if ( !etab[no].tgt ) {
                fprintf(stderr, "odb [etabadd(%d)] - Error: missing \'tgt\' operator\n", __LINE__ );
                goto etabadd_exit;
            }
            if ( ( etab[no].flg2 ) & 04000000 && ( etab[no].flg & 01000 ) ) {
                fprintf(stderr, "odb [etabadd(%d)] - Error: \'xml\' and \'multi\' operators are not compatible\n", __LINE__ );
                goto etabadd_exit;
            }
            if ( ( etab[no].flg & 010000000 ) && ( etab[no].sql || etab[no].cols ) ) {
                fprintf(stderr, "odb [etabadd(%d)] - Error: \'ucs2toutf8\' is available only for full table extracts\n", __LINE__ );
                goto etabadd_exit;
            }
            break;
        case 'c':
            if ( etab[no].flg & 01000000000 ) { /* if cast... */
                etab[no].flg2 &= ~0200000;      /* ...auto-bind off */
                etab[no].flg2 |= 04000;         /* ...char-bind on */
            }
            if ( !etab[no].tgt ) {
                fprintf(stderr, "odb [etabadd(%d)] - Error: missing \'tgt\' operator\n", __LINE__ );
                goto etabadd_exit;
            }
            if ( etab[no].roe ) {
                if ( etab[no].flg & 0004 ) {
                    fprintf(stderr, "odb [etabadd(%d)] - Error: you cannot use both \'soe\' and \'roe\' operators\n", __LINE__ );
                    goto etabadd_exit;
                } else if ( etab[no].cmt != (-1) ) {
                    fprintf(stderr, "odb [etabadd(%d)] - Error: you can use \'roe\' operator only with \'commit=end\'\n", __LINE__ );
                    goto etabadd_exit;
                } else if ( etab[no].nl != 1 ) {
                    fprintf(stderr, "odb [etabadd(%d)] - Error: you can use \'roe\' operator only with \'loaders=1\'\n", __LINE__ );
                    goto etabadd_exit;
                }
            }
            if ( ( etab[no].flg & 010000000 ) && ( etab[no].sql || etab[no].cols ) ) {
                fprintf(stderr, "odb [etabadd(%d)] - Error: \'ucs2toutf8\' is available only for full table copies\n", __LINE__ );
                goto etabadd_exit;
            }
            break;
        case 'p':
            if ( !etab[no].ns ) {
                fprintf(stderr, "odb [etabadd(%d)] - Error: missing \'tgtsql\' operator\n", __LINE__ );
                goto etabadd_exit;
            }
            break;
        case 'd':
            if ( !etab[no].src  ) {
                fprintf(stderr, "odb [etabadd(%d)] - Error: missing \'src\' operator\n", __LINE__ );
                goto etabadd_exit;
            }
            if ( !etab[no].tgt ) {
                fprintf(stderr, "odb [etabadd(%d)] - Error: missing \'tgt\' operator\n", __LINE__ );
                goto etabadd_exit;
            }
            if ( etab[no].flg2 & 0010 ) /* quick mode */
                etab[no].flg &= ~010000000000;  /* do not consider changes */
            if ( etab[no].sb ) {
                if ( etab[no].ps <= 1 )
                    fprintf(stderr, "odb [etabadd(%d)] - Warning: \'splitby\' won't be used'\n", __LINE__ );
            } else {
                if ( etab[no].ps > 1 ) {
                    fprintf(stderr, "odb [etabadd(%d)] - Error: missing \'splitby\' operator with \'parallel\'\n", __LINE__ );
                    goto etabadd_exit;
                }
            }
        }

#ifdef HDFS
        /* Load libhdfs, get function pointers and connect to HDFS */
        if ( etab[no].flg2 & 0100 ) {
            if ( etab[no].flg2 & 01000000000 ) {
                if ( f & 020000 )       /* if verbose */
                    fprintf(stderr, "odb [etabadd(%d)]: Loading libMapRClient functions... ", __LINE__);
                if ( (hdfs_handle = dlopen("libMapRClient.so", RTLD_LAZY )) == (void *)NULL ) {
                    fprintf(stderr, "odb [etabadd(%d)] - Error loading libMapRClient.so: %s\n",
                        __LINE__, dlerror());
                    goto etabadd_exit;
                }
            } else {
                if ( f & 020000 )       /* if verbose */
                    fprintf(stderr, "odb [etabadd(%d)]: Loading libhdfs functions... ", __LINE__);
                if ( (hdfs_handle = dlopen("libhdfs.so", RTLD_NOW )) == (void *)NULL ) {
                    fprintf(stderr, "odb [etabadd(%d)] - Error loading libhdfs.so: %s\n",
                        __LINE__, dlerror());
                    goto etabadd_exit;
                }
            }
            if ( ( *(void **) (&hdfsconn) = dlsym(hdfs_handle, "hdfsConnectAsUser" ) ) == NULL ) {
                fprintf(stderr, "odb [etabadd(%d)] - Error loading hdfsConnectAsUser symbol: %s\n",
                    __LINE__, dlerror());
                goto etabadd_exit;
            }
            if ( ( *(void **) (&hdfsopen) = dlsym(hdfs_handle, "hdfsOpenFile" ) ) == NULL ) {
                fprintf(stderr, "odb [etabadd(%d)] - Error loading hdfsOpenFile symbol: %s\n",
                    __LINE__, dlerror());
                goto etabadd_exit;
            }
            if ( ( *(void **) (&hdfsclose) = dlsym(hdfs_handle, "hdfsCloseFile" ) ) == NULL ) {
                fprintf(stderr, "odb [etabadd(%d)] - Error loading hdfsCloseFile symbol: %s\n",
                    __LINE__, dlerror());
                goto etabadd_exit;
            }
            if ( ( *(void **) (&hdfsseek) = dlsym(hdfs_handle, "hdfsSeek" ) ) == NULL ) {
                fprintf(stderr, "odb [etabadd(%d)] - Error loading hdfsSeek symbol: %s\n",
                    __LINE__, dlerror());
                goto etabadd_exit;
            }
            if ( ( *(void **) (&hdfswrite) = dlsym(hdfs_handle, "hdfsWrite" ) ) == NULL ) {
                fprintf(stderr, "odb [etabadd(%d)] - Error loading hdfsWrite symbol: %s\n",
                    __LINE__, dlerror());
                goto etabadd_exit;
            }
            if ( ( *(void **) (&hdfsread) = dlsym(hdfs_handle, "hdfsRead" ) ) == NULL ) {
                fprintf(stderr, "odb [etabadd(%d)] - Error loading hdfsRead symbol: %s\n",
                    __LINE__, dlerror());
                goto etabadd_exit;
            }
            if ( f & 020000 )       /* if verbose */
                fprintf(stderr, "done.\n");
            /* hdfsConnectAsUser parameters:
               - host A string containing either a host name, or an ip address of the namenode of a hdfs cluster.s
                'host' should be passed as NULL if you want to connect to local filesystem.
                'host' should be passed as 'default' (and port as 0) to used the 'configured' filesystem (core-site/core-default.xml).  
            */
            if ( ( hfs = (*hdfsconn)(
                    etab[no].hdfshost ? etab[no].hdfshost : "default",
                    etab[no].hdfsport ? etab[no].hdfsport : 0,
                    etab[no].hdfsuser ? etab[no].hdfsuser : NULL) ) == (hdfsFS)NULL ) {
                fprintf(stderr, "odb [etabadd(%d)] - Error connecting to HDFS (host=%s port=%u, user=%s)\n",
                    __LINE__,
                    etab[no].hdfshost ? etab[no].hdfshost : "default",
                    etab[no].hdfsport ? (unsigned int)etab[no].hdfsport : 0,
                    etab[no].hdfsuser ? etab[no].hdfsuser : "NULL" );
                goto etabadd_exit;
            }
        }
#endif

#ifdef XML
        /* Load libxml2 and get function pointers */
        if ( etab[no].xrt ) {
            if ( f & 020000 )       /* if verbose */
                fprintf(stderr, "odb [etabadd(%d)]: Loading libxml2 functions... ", __LINE__);
            if ( (xml_handle = dlopen(XML2LIBNAME, RTLD_NOW )) == (void *)NULL ) {
                fprintf(stderr, "odb [etabadd(%d)] - Error loading libxml2.so: %s\n",
                    __LINE__, dlerror());
                goto etabadd_exit;
            }
            if ( ( *(void **) (&xmlfile) = dlsym(xml_handle, "xmlNewTextReaderFilename" ) ) == NULL ) {
                fprintf(stderr, "odb [etabadd(%d)] - Error loading xmlNewTextReaderFilename symbol: %s\n",
                    __LINE__, dlerror());
                goto etabadd_exit;
            }
            if ( ( *(void **) (&xmlread) = dlsym(xml_handle, "xmlTextReaderRead" ) ) == NULL ) {
                fprintf(stderr, "odb [etabadd(%d)] - Error loading xmlTextReaderRead symbol: %s\n",
                    __LINE__, dlerror());
                goto etabadd_exit;
            }
            if ( ( *(void **) (&xmltype) = dlsym(xml_handle, "xmlTextReaderNodeType" ) ) == NULL ) {
                fprintf(stderr, "odb [etabadd(%d)] - Error loading xmlTextReaderNodeType symbol: %s\n",
                    __LINE__, dlerror());
                goto etabadd_exit;
            }
            if ( ( *(void **) (&xmldepth) = dlsym(xml_handle, "xmlTextReaderDepth" ) ) == NULL ) {
                fprintf(stderr, "odb [etabadd(%d)] - Error loading xmlTextReaderDepth symbol: %s\n",
                    __LINE__, dlerror());
                goto etabadd_exit;
            }
            if ( ( *(void **) (&xmllname) = dlsym(xml_handle, "xmlTextReaderLocalName" ) ) == NULL ) {
                fprintf(stderr, "odb [etabadd(%d)] - Error loading xmlTextReaderLocalName symbol: %s\n",
                    __LINE__, dlerror());
                goto etabadd_exit;
            }
            if ( ( *(void **) (&xmlnextattr) = dlsym(xml_handle, "xmlTextReaderMoveToNextAttribute" ) ) == NULL ) {
                fprintf(stderr, "odb [etabadd(%d)] - Error loading xmlTextReaderMoveToNextAttribute symbol: %s\n",
                    __LINE__, dlerror());
                goto etabadd_exit;
            }
            if ( ( *(void **) (&xmlvalue) = dlsym(xml_handle, "xmlTextReaderValue" ) ) == NULL ) {
                fprintf(stderr, "odb [etabadd(%d)] - Error loading xmlTextReaderValue symbol: %s\n",
                    __LINE__, dlerror());
                goto etabadd_exit;
            }
            if ( ( *(void **) (&xmlfree) = dlsym(xml_handle, "xmlFreeTextReader" ) ) == NULL ) {
                fprintf(stderr, "odb [etabadd(%d)] - Error loading xmlFreeTextReader symbol: %s\n",
                    __LINE__, dlerror());
                goto etabadd_exit;
            }
            if ( f & 020000 )       /* if verbose */
                fprintf(stderr, "done.\n");
        }
#endif

        /* Max fetches vs rowset check */
        if ( etab[no].mr && etab[no].r > etab[no].mr )  /* if # records to fetch < rowset ... */
            etab[no].r = etab[no].mr;                   /* make rowset = records to fetch */

        /* Check src database */
        if ( type != 'l' ) {
            if ( !Oc ) {    /* First Time Connection */
                if ( ( f & 00006 ) && ( Odsn[0] || clca ) ) {
                    if (!SQL_SUCCEEDED(Oret=SQLAllocHandle(SQL_HANDLE_DBC, Oenv, &Oc))){
                        Oerr(-1, -1, __LINE__, Oenv, SQL_HANDLE_ENV);
                        goto etabadd_exit;
                    }
                    (void) bcs ( Oics, sizeof(Oics), Ouser, Opwd, Odsn, clca, 0 );
                    if ( Onps && !SQL_SUCCEEDED(Oret=SQLSetConnectAttr(Oc, 
                            (SQLINTEGER)SQL_ATTR_PACKET_SIZE, (SQLPOINTER)&Onps, SQL_IS_UINTEGER ) ) )
                        Oerr(-1, -1, __LINE__, Oenv, SQL_HANDLE_ENV);
                    if (!SQL_SUCCEEDED(Oret=SQLDriverConnect(Oc, (SQLHWND) NULL,
                            Oics, SQL_NTS, Oocs, (SQLSMALLINT) sizeof(Oocs),
                            &Oocsl, (SQLUSMALLINT)SQL_DRIVER_NOPROMPT))) {
                        Oerr(-1, -1, __LINE__, Oc, SQL_HANDLE_DBC);
                        goto etabadd_exit;
                    }
                } else {
                    fprintf(stderr, "odb [etabadd(%d)] - Connection parameters not available\n", __LINE__);
                    goto etabadd_exit;
                }
                ldbt = checkdb ( -1, &Oc, NULL, NULL ); /* check DB type */
            }
            etab[no].dbt = ldbt ;
        }

        /* Check if options are consistent with source database type */
        if ( type == 'd' ) {
            if ( ( etab[no].flg2 & 01000000 ) && etab[no].dbt != ORACLE ) {
                fprintf(stderr, "odb [etabadd(%d)] - Error: odad is set but the source database is not Oracle\n", __LINE__ );
                goto etabadd_exit;
            }
        }

        /* Manage Options */
        buff[0] = '\0';                         /* Initialize buff */
        cimutex(&etab[no].gmutex);              /* Create & Initialize Group mutex */
        if ( etab[no].sql ) {                   /* source is SQL so... no catalog/schema/table */
            if ( etab[no].sql[0] == '-' ) {     /* Read list of custom SQL from file */
                edef = etab[no];                /* save defaults */
                edef.sql = 0;                   /* sql cannot be defaulted */
                if ( ( fl = fopen(&etab[no].sql[1], "r") ) == (FILE *)NULL ) {
                    fprintf(stderr, "odb [etabadd(%d)] - Cannot open %s: [%d] %s\n",
                        __LINE__, &etab[no].sql[1], errno, strerror(errno));
                    goto etabadd_exit;
                }
                (void) fseek(fl, 0L, SEEK_END);     /* goto EOF */
                ll = ftell ( fl );                  /* get file size */
                (void) fseek(fl, 0L, SEEK_SET);     /* rewind */
                /* Allocate memory to contain the whole input file */
                if ( esrc ) {
                    fprintf(stderr, "odb [etabadd(%d)] - you can use only one src input file per odb session\n", 
                        __LINE__);
                    fclose(fl);
                    goto etabadd_exit;

                }
                if ( ( esrc = malloc ( ll + 1 ) ) == (void *)NULL) {
                    fprintf(stderr, "odb [etabadd(%d)] - error allocating memory to read %s\n", 
                        __LINE__, etab[no].src);
                    fclose(fl);
                    goto etabadd_exit;
                }
                /* read input file */
#ifdef _WIN32
                len = fread ( esrc, 1, ll, fl) ;
#else
                if ( ( len = fread ( esrc, 1, ll, fl) ) != ll ) {
                    fprintf(stderr, "odb [etabadd(%d)] - error reading %s (got " SIZET_SPEC " bytes, expected " SIZET_SPEC "): [%d] %s\n",
                        __LINE__, &etab[no].src[1], len, ll, errno, strerror(errno) );
                    fclose(fl);
                    goto etabadd_exit;
                }
#endif
                fclose ( fl ) ;
                esrc[len] = '\0';
                tabn[0] = '\0';                     /* this to run parseopt in the next while loop */
            }
            do {
                if ( fl ) {                         /* input file with list of SQLs */
                    etab[no] = edef;
                    if ( ( ret = parseopt ( type, esrc, (unsigned int)len, 1 ) ) == 2 ) {   /* no more options to parse */
                        break;
                    } else if ( ret == 1 ) {
                        goto etabadd_exit;
                    }
                    etab[no].parent = no;
                    etab[no].id = no;
                }
                if ( etab[no].ps )
                    etab[no].mr /= etab[no].ps; /* each thread will get a portion of the max record to fetch */
                if ( etab[no].sql[0] == '@' ) {         /* Read SQL from file */
                    etab[no].cols = &etab[no].sql[1] ;  /* save original file name */
                    if ( ( fsql = fopen(&etab[no].sql[1], "r") ) == (FILE *)NULL ) {
                        fprintf(stderr, "odb [etabadd(%d)] - Cannot open %s: [%d] %s\n",
                            __LINE__, &etab[no].sql[1], errno, strerror(errno));
                        goto etabadd_exit;
                    }
                    (void) fseek(fsql, 0L, SEEK_END);       /* goto EOF */
                    ll = ftell ( fsql );                    /* get file size */
                    (void) fseek(fsql, 0L, SEEK_SET);       /* rewind */
                    /* Allocate memory to contain the whole input file */
                    if ( ( etab[no].sql = malloc ( ll + 1 ) ) == (void *)NULL) {
                        fprintf(stderr, "odb [etabadd(%d)] - error allocating memory to read %s\n", 
                            __LINE__, etab[no].sql);
                        fclose(fsql);
                        goto etabadd_exit;
                    }
                    /* read input file */
#ifdef _WIN32
                    lsql = fread ( etab[no].sql, 1, ll, fsql) ;
#else
                    if ( ( lsql = fread ( etab[no].sql, 1, ll, fsql) ) != ll ) {
                        fprintf(stderr, "odb [etabadd(%d)] - error reading %s (got " SIZET_SPEC " bytes, expected " SIZET_SPEC "): [%d] %s\n",
                            __LINE__, &etab[no].sql[1], lsql, ll, errno, strerror(errno) );
                        fclose(fsql);
                        goto etabadd_exit;
                    }
#endif
                    fclose ( fsql ) ;
                    etab[no].sql[lsql] = '\0';
                    etab[no].flg2 |= 0100000 ;
                }
                if ( etab[no].ns ) {        /* target custom SQL */
                    if ( etab[no].ns[0] == '@' ) {  /* tgt SQL from file */
                        if ( ( fsql = fopen(&etab[no].ns[1], "r") ) == (FILE *)NULL ) {
                            fprintf(stderr, "odb [etabadd(%d)] - Cannot open %s: [%d] %s\n",
                                __LINE__, &etab[no].sql[1], errno, strerror(errno));
                            goto etabadd_exit;
                        }
                        (void) fseek(fsql, 0L, SEEK_END);       /* goto EOF */
                        ll = ftell ( fsql );                    /* get file size */
                        (void) fseek(fsql, 0L, SEEK_SET);       /* rewind */
                        /* Allocate memory to contain the whole input file */
                        if ( ( etab[no].es = malloc ( ll + 1 ) ) == (void *)NULL) {
                            fprintf(stderr, "odb [etabadd(%d)] - error allocating memory to read %s\n", 
                                __LINE__, etab[no].ns);
                            fclose(fsql);
                            goto etabadd_exit;
                        }
                        /* read input file */
#ifdef _WIN32
                        lsql = fread ( etab[no].es, 1, ll, fsql) ;
#else
                        if ( ( lsql = fread ( etab[no].es, 1, ll, fsql) ) != ll ) {
                            fprintf(stderr, "odb [etabadd(%d)] - error reading %s (got " SIZET_SPEC " bytes, expected " SIZET_SPEC "): [%d] %s\n",
                                __LINE__, &etab[no].ns[1], lsql, ll, errno, strerror(errno) );
                            fclose(fsql);
                            goto etabadd_exit;
                        }
#endif
                        fclose ( fsql ) ;
                        etab[no].es[lsql] = '\0';
                    } else {
                        etab[no].es = etab[no].ns ;
                    }
                    etab[no].fsl = strlen(etab[no].es);
                    if ( ( etab[no].tgtsql = malloc ( etab[no].fsl + 1 ) ) == (void *)NULL) {
                        fprintf(stderr, "odb [etabadd(%d)] - error allocating memory for tgtsql\n", __LINE__);
                        goto etabadd_exit;
                    }
                    /* First pass to count parameters and write tgtsql */
                    for ( ll = 0 ; ll < etab[no].fsl ; ll++ ) {
                        if ( etab[no].es[ll] == '?' && isdigit((int)etab[no].es[ll+1]) ) {
                            while ( isdigit((int)etab[no].es[++ll] ) );
                            etab[no].tgtsql[j++] = '?' ;
                            etab[no].ep++;
                            ll--;
                        } else {
                            etab[no].tgtsql[j++] = etab[no].es[ll] ;
                        }
                    }
                    etab[no].tgtsql[j] = '\0' ;
                    if ( etab[no].ep == 0 ) {
                        fprintf(stderr, "odb [etabadd(%d)] - no parameters in tgtsql\n", __LINE__);
                        goto etabadd_exit;
                    } 
                    if ( ( etab[no].tgtp = calloc ( (size_t)etab[no].ep, sizeof(unsigned int) ) ) == (void *)NULL) {
                        fprintf(stderr, "odb [etabadd(%d)] - error allocating memory for tgtsql parameters\n", __LINE__);
                        goto etabadd_exit;
                    }
                    /* Second pass to get parameter positions */
                    for ( ll = 0, i = 0 ; ll < etab[no].fsl ; ll++ ) {
                        if ( etab[no].es[ll] == '?' && isdigit((int)etab[no].es[ll+1]) ) {
                            etab[no].tgtp[i++] = (unsigned int)strtol(&etab[no].es[ll+1], NULL, 10);
                        }
                    }
                    if ( etab[no].ns )                  /* target custom SQL */
                        if ( etab[no].ns[0] == '@' )    /* tgt SQL from file */
                            free ( etab[no].es ) ;      /* free etab[no].es, SQL already copied to etab[no].tgtsql */
                }
                if ( strstr(etab[no].sql, "&cds") ) {   /* custom splitby */
                    etab[no].flg2 |= 040000 ;   /* custom SQL */
                } else if ( etab[no].ps ) {
                    if ( etab[no].dbt != ORACLE &&
                         etab[no].dbt != SQLSERVER ) {
                        fprintf(stderr, "odb [etabadd(%d)] - Error: \"parallel\" without \"&cds/&tds\" is"
                                        " only supported by Oracle and SQL Server databases \n",
                                        __LINE__);
                        goto etabadd_exit;
                    }
                }
                if ( etab[no].type == 'e' ) { /* name & create output file */
                    for ( i = j = 0; i < sizeof(buff) && etab[no].tgt[i]; i++ ) {
                        switch ( etab[no].tgt[i] ) {
                        case '%':
                            switch ( etab[no].tgt[++i] ) {
                            case 'd':   /* date (YYYYMMDD) */
                                pt = (time_t)tvi.tv_sec;
                                dt = localtime(&pt);
                                strftime(buff2, sizeof(buff2), "%Y%m%d", dt);
                                j += strmcat (buff, buff2, sizeof(buff), 0 ); 
                                break;
                            case 'D':   /* date (YYYY-MM-DD) */
                                pt = (time_t)tvi.tv_sec;
                                dt = localtime(&pt);
                                strftime(buff2, sizeof(buff2), "%Y-%m-%d", dt);
                                j += strmcat (buff, buff2, sizeof(buff), 0 ); 
                                break;
                            case 'm':   /* time (hhmmss) */
                                pt = (time_t)tvi.tv_sec;
                                dt = localtime(&pt);
                                strftime(buff2, sizeof(buff2), "%H%M%S", dt);
                                j += strmcat (buff, buff2, sizeof(buff), 0 ); 
                                break;
                            case 'M':   /* time (hhmmss) */
                                pt = (time_t)tvi.tv_sec;
                                dt = localtime(&pt);
                                strftime(buff2, sizeof(buff2), "%H:%M:%S", dt);
                                j += strmcat (buff, buff2, sizeof(buff), 0 ); 
                                break;
                            default:
                                fprintf(stderr, "odb [etabadd(%d)] - Invalid %%%c in %s\n",
                                    __LINE__, etab[no].tgt[i], etab[no].tgt );
                                goto etabadd_exit;
                            }
                            break;
                        default:
                            buff[j++] = etab[no].tgt[i];
                            break;
                        }
                    }
                    buff[j] = '\0';
                    if ( etabout ( no , buff ) ) {
                        goto etabadd_exit;
                    }
                    l = no;
                } else if ( etab[no].type == 'c' || etab[no].type == 'p' ) {
                    l = etab[no].parent;
                    etab[l].ps = etab[no].ps;
                    etab[no].lstat = 4;     /* other threads to wait while gpar check tgt dbt */
                    if ( etab[no].nloader == 0 )
                        etab[no].nloader = etab[no].type == 'c' ? 2 : 1; /* initialize default number of loaders */
                    etab[no].nltotal = etab[no].ps * etab[no].nloader ;
                    etab[no].k = no;        /* save grand-parent copy thread (first 'c' thread for this pool) */
                    etab[no].run = etab[no].tgt ;
                    cimutex(&etab[no].pmutex);
                    cicondvar(&etab[no].pcvp);
                    cicondvar(&etab[no].pcvc);
                    for ( i = 0, j = no; i < etab[j].nloader ; i++ ) {
                        etabnew ( no++ );   /* Create new etab entry based on original 'c'/'p' struct */
                        etab[no].type = etab[l].type == 'c' ? 'C' : 'P';
                        if ( etab[no].type == 'C' ) 
                            etab[no].mpre = tmpre ;
                        etab[no].id = no;
                        etab[no].parent = j;
                        etab[no - 1].child = no;
                    }
                }
                cimutex(&etab[no].pmutex);
                for ( no++, j = 1 ; j < etab[l].ps ; j++ ) {
                    etabnew ( l );
                    etab[no].id = no;
                    etab[no].parent = l;
                    etab[no].sbmin = j;
                    if ( etab[no].flg & 01000 ) {   /* if multi open the output file now */
                        buff[bl] = '\0';
                        snprintf(buff2, sizeof(buff2), ".%04d", j+1);
                        strmcat (buff, buff2, sizeof(buff), 0 ); 
                        if ( etabout ( no , buff ) ) {
                            goto etabadd_exit;
                        }
                    }
                    etab[no].tbe = 1;
                    if ( etab[no].type == 'c' || etab[no].type == 'p' ) {
                        etab[no].parent = no;
                        etab[no].iobuff = etab[no].ps * etab[no].nloader ;   /* total number of loaders */
                        cimutex(&etab[no].pmutex);
                        cicondvar(&etab[no].pcvp);
                        cicondvar(&etab[no].pcvc);
                        for ( i = 0, n = no; i < etab[j].nloader ; i++ ) {
                            etabnew ( no++ );           /* Create new etab entry based on original 'c'/'p' struct */
                            etab[no].type = etab[l].type == 'c' ? 'C' : 'P';
                            if ( etab[no].type == 'C' ) 
                                etab[no].mpre = tmpre ;
                            etab[no].id = no;
                            etab[no].parent = n;
                            etab[no - 1].child = no;
                        }
                    }
                    no++;
                }
            } while ( fl ) ;
        } else {                /* Source is a table pattern or a file containing table patterns */
            if ( etab[no].src[0] == '-' ) {     /* etab[no].src is a file containing list of tables */
                edef = etab[no];                /* save defaults */
                edef.src = 0;                   /* src cannot be defaulted */
                if ( ( fl = fopen(&etab[no].src[1], "r") ) == (FILE *)NULL ) {
                    fprintf(stderr, "odb [etabadd(%d)] - Cannot open %s: [%d] %s\n",
                        __LINE__, &etab[no].src[1], errno, strerror(errno));
                    goto etabadd_exit;
                }
                (void) fseek(fl, 0L, SEEK_END);     /* goto EOF */
                ll = ftell ( fl );                  /* get file size */
                (void) fseek(fl, 0L, SEEK_SET);     /* rewind */
                /* Allocate memory to contain the whole input file */
                if ( esrc ) {
                    fprintf(stderr, "odb [etabadd(%d)] - you can use only one src input file per odb session\n", 
                        __LINE__);
                    fclose(fl);
                    goto etabadd_exit;

                }
                if ( ( esrc = malloc ( ll + 1 ) ) == (void *)NULL) {
                    fprintf(stderr, "odb [etabadd(%d)] - error allocating memory to read %s\n", 
                        __LINE__, etab[no].src);
                    fclose(fl);
                    goto etabadd_exit;
                }
                /* read input file */
#ifdef _WIN32
                len = fread ( esrc, 1, ll, fl) ;
#else
                if ( ( len = fread ( esrc, 1, ll, fl) ) != ll ) {
                    fprintf(stderr, "odb [etabadd(%d)] - error reading %s (got " SIZET_SPEC " bytes, expected " SIZET_SPEC "): [%d] %s\n",
                        __LINE__, &etab[no].src[1], len, ll, errno, strerror(errno) );
                    fclose(fl);
                    goto etabadd_exit;
                }
#endif
                fclose ( fl ) ;
                esrc[ll] = '\0';
                tabn[0] = '\0';                     /* this to run parseopt in the next while loop */
            } else {
                strmcpy(tabn, etab[no].src, sizeof(tabn));
            }
            do {
                if ( fl ) {                         /* input file with list of tables */
                    etabnew(-1);
                    etab[no] = edef;
                    if ( etab[no].seqp ) {          /* :seq option at command line level: set global sequence */
                        f2 |= 0002 ;                /* switch global sequence flag on */
                        gseq = etab[no].seq ;       /* save seq start value */
                    }
                    if ( ( ret = parseopt ( type, esrc, (unsigned int)len, 1 ) ) == 2 ) {   /* no more options to parse */
                        break;
                    } else if ( ret == 1 ) {
                        goto etabadd_exit;
                    }
                    etab[no].parent = no;
                    etab[no].id = no;
                }

                if (etab[no].bad) {        /* bad file */
                    if (etabout(no, etab[no].bad))
                        goto etabadd_exit;
                }

                if ( etab[no].type == 'l' ) {   /* load job */
                    if ( etab[no].ps ) {
                        cimutex(&etab[no].pmutex);
                        cicondvar(&etab[no].pcvp);
                        cicondvar(&etab[no].pcvc);
                        for ( l = no++, j = 0 ; j < etab[l].ps ; j++ ) {
                            etabnew ( l );
                            etab[no].type = 'L';
                            etab[no].id = no;
                            etab[no++].parent = l;
                        }
                    } else {
                        etab[no++].tbe = 1;
                    }
                } else {                            /* not a load job */
                    etab[no].k = no;                /* record grandparent for copy/diff ops */
                    if (etab[no].mr && etab[no].ps > 1 ){
                        etab[no].TotalMaxRecords = etab[no].mr;
                        etab[no].mr /= etab[no].ps; /* each thread will get a portion of the max record to fetch */
                        if (etab[0].r > etab[0].mr) /* rowset size should not exceeds the max size */
                            etab[0].r = etab[0].mr;
                    }
                    strmcpy(tabn, etab[no].src, sizeof(tabn));
                    if ( !SQL_SUCCEEDED(Oret=SQLAllocHandle(SQL_HANDLE_STMT, Oc, &Os))){
                        Oerr(-1, -1, __LINE__, Oc, SQL_HANDLE_DBC);
                        goto etabadd_exit;
                    }
                    if ( !SQL_SUCCEEDED(Oret=SQLAllocHandle(SQL_HANDLE_STMT, Oc, &Os1))){
                        Oerr(-1, -1, __LINE__, Oc, SQL_HANDLE_DBC);
                        goto etabadd_exit;
                    }
                    if ( ( strchr ( tabn , '%' ) ) == ( char * ) NULL ) {   /* Nothing to expand in the source string */
                        /* Note: strchr() to be executed before splitcso() inserts NULL in tabn */
                        splitcso ( tabn, etab[no].Ocso, 1);
                        strmcpy((char *)Ostn[0], (char *)etab[no].Ocso[0], sizeof(Ostn[0]));
                        strmcpy((char *)Ostn[1], (char *)etab[no].Ocso[1], sizeof(Ostn[1]));
                        strmcpy((char *)Ostn[2], (char *)etab[no].Ocso[2], sizeof(Ostn[2]));
                        Ostnl[0] = (SQLLEN)strlen((char *)Ostn[0]);
                        Ostnl[1] = (SQLLEN)strlen((char *)Ostn[1]);
                        Ostnl[2] = (SQLLEN)strlen((char *)Ostn[2]);
                        np = 1 ;
                    } else {
                        splitcso ( tabn, etab[no].Ocso, 1);
                        if (!SQL_SUCCEEDED(Oret=SQLTables(Os,
                                etab[no].Ocso[0], SQL_NTS, 
                                etab[no].Ocso[1], SQL_NTS,
                                etab[no].Ocso[2], SQL_NTS,
                                (SQLCHAR *)"TABLE" , SQL_NTS ))) {
                            Oerr(-1, -1, __LINE__, Os, SQL_HANDLE_STMT);
                            goto etabadd_exit;
                        }
                        if (!SQL_SUCCEEDED(Oret = SQLBindCol(Os, (SQLUSMALLINT) 1,
                                SQL_C_CHAR, Ostn[0], (SQLLEN)sizeof(Ostn[0]), &Ostnl[0]))) {
                            Oerr(-1, -1, __LINE__, Os, SQL_HANDLE_STMT);
                            goto etabadd_exit;
                        }
                        if (!SQL_SUCCEEDED(Oret = SQLBindCol(Os, (SQLUSMALLINT) 2,
                                SQL_C_CHAR, Ostn[1], (SQLLEN)sizeof(Ostn[1]), &Ostnl[1]))) {
                            Oerr(-1, -1, __LINE__, Os, SQL_HANDLE_STMT);
                            goto etabadd_exit;
                        }
                        if (!SQL_SUCCEEDED(Oret = SQLBindCol(Os, (SQLUSMALLINT) 3,
                                SQL_C_CHAR, Ostn[2], (SQLLEN)sizeof(Ostn[2]), &Ostnl[2]))) {
                            Oerr(-1, -1, __LINE__, Os, SQL_HANDLE_STMT);
                            goto etabadd_exit;
                        }
                    }
                    for ( o = 0 ; np ||  SQL_SUCCEEDED(Oret=SQLFetch(Os)); o++) {
                        np = 0 ;    /* reset no parse flag */
                        if ( o ) {
                            etabnew ( -1 );
                            etab[no] = etab[or];
                            etab[no].parent = no;
                            etab[no].id = no;
                        }
                        if ( Ostnl[0] < 0 )
                            Ostnl[0] = 0;
                        if ( Ostnl[1] < 0 )
                            Ostnl[1] = 0;
                        ll = (size_t)(4 + Ostnl[0] + Ostnl[1] + Ostnl[2]);
                        if ( ( etab[no].src = malloc ( ll ) ) == (void *)NULL ) {
                            fprintf(stderr,"odb [etabadd(%d)] - Error allocating memory for table name: [%d] %s\n",
                                    __LINE__, errno, strerror(errno) );
                            goto etabadd_exit;
                        }
                        etab[no].src[0] = '\0';
                        if ( Ostnl[0] ) {
                            strmcat ( etab[no].src , (char *)Ostn[0] , ll , 0 );
                            strmcat ( etab[no].src , "." , ll , 0 );
                        }
                        if ( Ostnl[1] ) {
                            strmcat ( etab[no].src , (char *)Ostn[1] , ll , 0 );
                            strmcat ( etab[no].src , "." , ll , 0 );
                        }
                        strmcat ( etab[no].src , (char *)Ostn[2] , ll , 0 );
                        etab[no].Ocso[2] = &Ostn[2][0]; /* update table name pointer with expanded tab name */
                        memset ( buff, 0, sizeof(buff));
                        for ( i = j = 0; i < sizeof(buff) && etab[no].tgt[i]; i++ ) {
                            switch ( etab[no].tgt[i] ) {
                            case '%':
                                switch ( etab[no].tgt[++i] ) {
                                case 'd':   /* date (YYYYMMDD) */
                                    pt = (time_t)tvi.tv_sec;
                                    dt = localtime(&pt);
                                    strftime(buff2, sizeof(buff2), "%Y%m%d", dt);
                                    j += strmcat (buff, buff2, sizeof(buff), 0 ); 
                                    break;
                                case 'D':   /* date (YYYY-MM-DD) */
                                    pt = (time_t)tvi.tv_sec;
                                    dt = localtime(&pt);
                                    strftime(buff2, sizeof(buff2), "%Y-%m-%d", dt);
                                    j += strmcat (buff, buff2, sizeof(buff), 0 ); 
                                    break;
                                case 'm':   /* time (hhmmss) */
                                    pt = (time_t)tvi.tv_sec;
                                    dt = localtime(&pt);
                                    strftime(buff2, sizeof(buff2), "%H%M%S", dt);
                                    j += strmcat (buff, buff2, sizeof(buff), 0 ); 
                                    break;
                                case 'M':   /* time (hhmmss) */
                                    pt = (time_t)tvi.tv_sec;
                                    dt = localtime(&pt);
                                    strftime(buff2, sizeof(buff2), "%H:%M:%S", dt);
                                    j += strmcat (buff, buff2, sizeof(buff), 0 ); 
                                    break;
                                case 'c':   /* LC catalog name */
                                    j += strmcat (buff, (char *)Ostn[0], sizeof(buff), 2 ); 
                                    break;
                                case 'C':   /* UC catalog name */
                                    j += strmcat (buff, (char *)Ostn[0], sizeof(buff), 1 ); 
                                    break;
                                case 's':   /* LC schema name */
                                    j += strmcat (buff, (char *)Ostn[1], sizeof(buff), 2 ); 
                                    break;
                                case 'S':   /* UC schema name */
                                    j += strmcat (buff, (char *)Ostn[1], sizeof(buff), 1 ); 
                                    break;
                                case 't':   /* LC table name */
                                    j += strmcat (buff, (char *)Ostn[2], sizeof(buff), 2 ); 
                                    break;
                                case 'T':   /* UC table name */
                                    j += strmcat (buff, (char *)Ostn[2], sizeof(buff), 1 ); 
                                    break;
                                default:
                                    fprintf(stderr, "odb [etabadd(%d)] - Invalid %%%c in %s\n",
                                        __LINE__, etab[no].tgt[i], etab[no].tgt );
                                    goto etabadd_exit;
                                }
                                break;
                            default:
                                buff[j++] = etab[no].tgt[i];
                                break;
                            }
                        }
                        buff[j] = '\0';
                        bl = j;
                        if ( f & 020000 )   /* if verbose */
                            fprintf(stderr, "odb: Now Analyzing %s:", etab[no].src);
                        if ( etab[no].type == 'e' || etab[no].type == 'd' ) {
                            if ( etab[no].flg & 01000 )         /* if multi open the output file now */
                                strmcat(buff, ".0001", sizeof(buff), 0);
                            if ( etab[no].flg & 01416000000 ||  /* extract with cast and/or trim and/or ucs2toutf8: analyze source table */
                                 etab[no].flg2 & 0040 ||        /* extract with exclude columns: analyze source table */
                                 etab[no].type == 'd' ) {       /* diff: analyze source table */
                                if ( etab[no].type == 'd' && !etab[no].key[0] ) {   /* diff without key... use PK */
                                    /* Get Primary key */
                                    if (!SQL_SUCCEEDED(Oret=SQLPrimaryKeys(Os1, 
                                            ( f & 04000000000 ) || etab[no].Ocso[0] ? etab[no].Ocso[0] : (SQLCHAR *)"",
                                            etab[no].Ocso[0] ? (SQLSMALLINT) strlen((char *)etab[no].Ocso[0]) : 0,
                                            etab[no].Ocso[1] ? etab[no].Ocso[1] : (SQLCHAR *)"",
                                            etab[no].Ocso[1] ? (SQLSMALLINT) strlen((char *)etab[no].Ocso[1]) : 0,
                                            etab[no].Ocso[2], (SQLSMALLINT) strlen((char *)etab[no].Ocso[2])))) {
                                        Oerr( -1, -1, __LINE__, Os1, SQL_HANDLE_STMT);
                                        goto etabadd_exit;
                                    }
                                    /* Bind result set:
                                        field  4: COLUMN_NAME      VARCHAR  -> Ostn[0]  -> etab[no].key[]
                                    */
                                    if (!SQL_SUCCEEDED(Oret = SQLBindCol(Os1, (SQLUSMALLINT) 4, SQL_C_CHAR, 
                                        &Ostn[0], (SQLLEN)MAXCOL_LEN, &Ostnl[0]))) {
                                        Oerr(-1, -1, __LINE__, Os1, SQL_HANDLE_STMT);
                                        goto etabadd_exit;
                                    }
                                    for ( i = 0 ; SQL_SUCCEEDED(Oret=SQLFetch(Os1)) && i < MAX_PK_COLS; i++) {
                                        if ( ( etab[no].key[i] = malloc ( (size_t)(Ostnl[0]+1) ) ) == (void *)NULL ) {
                                            fprintf(stderr, "odb [etabadd(%d)] - Error allocating memory for etab[%d].key[%u]\n",
                                                __LINE__, no, i);
                                            goto etabadd_exit;
                                        }
                                        strmcpy(etab[no].key[i], (char *)Ostn[0], (size_t)Ostnl[0]);
                                    }
                                    if ( i == 0 ) {     /* No PK elements found */
                                        fprintf(stderr, "odb [etabadd(%d)] - You didn't provide \"key\" columns and no PK was found\n",
                                            __LINE__);
                                        goto etabadd_exit;
                                    }
                                    etab[no].pc = (unsigned char)i;
                                    etab[no].em = 1;    /* free etab[].key[] buffers */
                                    (void)SQLFreeStmt(Os1, SQL_CLOSE);
                                    (void)SQLFreeStmt(Os1, SQL_UNBIND);
                                }

                                /* Analyze source table */
                                if ( ( i = Otcol(no, &Oc) ) <= 0 ) {
                                    fprintf(stderr, "odb [etabadd(%d)] - Error analyzing source table %s.%s.%s\n", 
                                        __LINE__, etab[no].Ocso[0], etab[no].Ocso[1], etab[no].Ocso[2]);
                                    goto etabadd_exit;
                                }
                                etab[no].cmt = i;   /* save the number of source table columns */
                                if ( etab[no].type == 'e' && etab[no].flg2 & 0040 )     /* extract with exclude columns */
                                    excol(no, &Oc);
                                if ( etab[no].type == 'd' )
                                    while ( i-- ) 
                                        etab[no].td[i].Osize++ ;

                                /* Sort etab[no].td[] elements based on key column order */
                                for ( i = 0 ; i < etab[no].pc ; i++ ) {
                                    for ( j = 0 ; j < (unsigned int)etab[no].cmt ; j++ ) {
                                        if ( !strmicmp((char *)etab[no].td[j].Oname, etab[no].key[i], strlen(etab[no].key[i])) ) {
                                            if ( i != j ) {
                                                to = etab[no].td[i];
                                                etab[no].td[i] = etab[no].td[j];
                                                etab[no].td[j] = to;
                                            }
                                        }
                                    }
                                }
                                /* mark order-by columns in etab[].td[] */
                                for ( i = 0 ; i < etab[no].pc ; i++ )
                                    etab[no].td[i].dl = 1;
                                if ( etab[no].flg2 & 0010 )     /* quick diff */
                                    etab[no].cmt = etab[no].pc; /* limit columns to order-by cols */
                            }
                            if ( etabout ( no , etab[no].type == 'd' ? etab[no].run : buff ) ) {
                                goto etabadd_exit;
                            }
                            if ( etab[no].type == 'd' ) {
                                etab[no].k = no;            /* save grand-parent copy thread (first 'd' thread for this pool) */
                                cimutex(&etab[no].pmutex);
                                cicondvar(&etab[no].pcvp);
                                cicondvar(&etab[no].pcvc);
                                etabnew ( no++ );           /* Helper 'D' thread */
                                etab[no].type = 'D';
                                etab[no].id = no;
                                etab[no].parent = no - 1;
                                etab[no].cr = 1 ;           /* first 'D' thread */
                                etabnew ( no++ );           /* Helper 'Z' thread */
                                etab[no].type = 'Z';
                                etab[no].id = no;
                                etab[no].lstat |= 0003 ;    /* Initial buffers status: src & tgt write available */
                                etab[no].parent = no - 2;   /* 'Z' threads have two parents (type 'd' and type 'D' threads) */
                                etab[no].child = no - 1;    /* 'd' parent eid is saved in .parent, 'D' parent id in .child */
                                etab[no - 2].child = no;    /* d (father) --> Z (child) */
                                etab[no - 1].child = no;    /* D (father) --> Z (child) */
                                if ( etab[no].flg & 020000000000 ) {    /* 's' thread entry */
                                    etabnew ( no++ );           /* Helper 'Z' thread */
                                    etab[no].type = 's';
                                    etab[no].id = no;
                                    etab[no].parent = no - 1;   /* 's' threads parents are 'Z' (compare) threads */
                                }
                            }
                        } else if ( etab[no].type == 'c' ) {
                            etab[no].lstat = 4;             /* other threads to wait while gpar check tgt dbt */
                            if ( etab[no].nloader == 0 )
                                etab[no].nloader = NUMLOADERS;   /* initialize number of loaders to its default */
                            etab[no].nltotal = etab[no].ps * etab[no].nloader ;
                            etab[no].k = no;                /* save grand-parent copy thread (first 'c' thread for this pool) */
                            ll = strlen ( buff ) ;
                            if ( ( etab[no].run = malloc ( ll + 1 ) ) == (void *)NULL ) {
                                fprintf(stderr,"odb [etabadd(%d)] - Error allocating memory for target table name: [%d] %s\n",
                                        __LINE__, errno, strerror(errno) );
                                goto etabadd_exit;
                            }
                            strmcpy ( etab[no].run, buff, ll );
                            cimutex(&etab[no].pmutex);
                            cicondvar(&etab[no].pcvp);
                            cicondvar(&etab[no].pcvc);
                            if ( etab[no].flg2 & 0040  ||       /* exclude columns */
                                 etab[no].flg & 01406000000 )   /* cast and/or [r]trim columns */
                                excol(no, &Oc);
                            /* ucs2toutf8 is set and we didn't analyze source table yet... */
                            if ( ( etab[no].flg & 010000000 ) && !etab[no].td ) {
                                if ( ( i = Otcol(no, &Oc) ) <= 0 ) {
                                    fprintf(stderr, "odb [etabadd(%d)] - Error analyzing source table %s\n", 
                                        __LINE__, etab[no].src);
                                    goto etabadd_exit;
                                }
                            }
                            for ( i = 0, j = no; i < etab[j].nloader ; i++ ) {
                                etabnew ( no++ );           /* Create new etab entry based on original 'c' struct */
                                etab[no].type = 'C';
                                etab[no].mpre = tmpre ;
                                etab[no].id = no;
                                etab[no].parent = j;
                                etab[no - 1].child = no;
                            }
                        } 
                        if ( etab[no].ps ) { /* Multi-stream table analysis */
                            if ( etab[no].sb ) {    /* split by */
                                if (etab[no].map) { /* if we get a pwhere condition, we apply it to improve perfomance */
                                    snprintf((char *)Obuf[0], sizeof(Obuf[0]),
                                        "SELECT MIN(%s), MAX(%s) FROM %s WHERE %s", etab[no].sb, etab[no].sb, etab[no].src, etab[no].map);
                                }
                                else {
                                    snprintf((char *)Obuf[0], sizeof(Obuf[0]),
                                        "SELECT MIN(%s), MAX(%s) FROM %s", etab[no].sb, etab[no].sb, etab[no].src);
                                }
                                if (!SQL_SUCCEEDED(Oret=SQLExecDirect (Os1, Obuf[0], SQL_NTS))) {
                                    Oerr(-1, -1, __LINE__, Os1, SQL_HANDLE_STMT);
                                    goto etabadd_exit;
                                }
                                if (!SQL_SUCCEEDED(Oret = SQLBindCol(Os1, (SQLUSMALLINT) 1, SQL_C_CHAR, 
                                    &Omn, (SQLLEN)sizeof(Omn), &Olen[0]))) {
                                    Oerr(-1, -1, __LINE__, Os1, SQL_HANDLE_STMT);
                                    goto etabadd_exit;
                                }
                                if (!SQL_SUCCEEDED(Oret = SQLBindCol(Os1, (SQLUSMALLINT) 2, SQL_C_CHAR, 
                                    &Omx, (SQLLEN)sizeof(Omx), &Olen[1]))) {
                                    Oerr(-1, -1, __LINE__, Os1, SQL_HANDLE_STMT);
                                    goto etabadd_exit;
                                }
                                if (!SQL_SUCCEEDED(Oret=SQLFetch(Os1))) {
                                    Oerr(-1, -1, __LINE__, Os1, SQL_HANDLE_STMT);
                                    goto etabadd_exit;
                                }
                                Omn[(int)Olen[0]] = '\0';
                                Omx[(int)Olen[1]] = '\0';
                                sbmin = strtol((char *)Omn, NULL, 10);
                                sbmax = strtol((char *)Omx, NULL, 10);
                                d = sbmax - sbmin;
                                if ( (unsigned int)d < etab[no].ps ) {
                                    fprintf(stderr,"odb [etabadd(%d)] - Warning: you cannot use %u threads because max(%s)-min(%s)=%ld. Reducing parallelism\n",
                                        __LINE__, etab[no].ps, etab[no].sb, etab[no].sb, d);
                                    etab[no].ps = d ? (unsigned int)d : 1 ;
                                }
                                d /= etab[no].ps;
                                etab[no].sbmin = sbmin;
                                etab[no].sbmax = d ? ( etab[no].sbmin + d ) : ( etab[no].sbmin + 1 ) ;
                                if ( etab[no].type == 'C' ) {
                                    etab[no].mpre = tmpre ;
                                    l = etab[no].parent;
                                    etab[l].ps = etab[no].ps;
                                    etab[l].sbmin = etab[no].sbmin;
                                    etab[l].sbmax = etab[no].sbmax;
                                } else if ( etab[no].type == 'Z' ) {
                                    l = etab[no].parent;
                                    etab[no-1].ps = etab[no-2].ps = etab[no].ps;
                                    etab[no-1].sbmin = etab[no-2].sbmin = etab[no].sbmin;
                                    etab[no-1].sbmax = etab[no-2].sbmax = etab[no].sbmax;
                                } else {
                                    l = no;
                                }
                                cimutex(&etab[no].pmutex);
                                for ( no++, j = 1 ; j < etab[l].ps ; j++ ) {
                                    etabnew ( l );
                                    etab[no].id = no;
                                    etab[no].parent = l;
                                    etab[no].sbmin = etab[no-1].sbmax ;
                                    etab[no].sbmax = ( j + 1 ) == etab[l].ps ? (sbmax + 1 ) : etab[no].sbmin + d ;
                                    if ( etab[no].flg & 01000 ) {   /* if multi open the output file now */
                                        buff[bl] = '\0';
                                        snprintf(buff2, sizeof(buff2), ".%04d", j+1);
                                        strmcat (buff, buff2, sizeof(buff), 0 ); 
                                        if ( etabout ( no , buff ) ) {
                                            goto etabadd_exit;
                                        }
                                    }
                                    etab[no].tbe = 1;
                                    if ( etab[no].type == 'c' ) {
                                        etab[no].parent = no;
                                        etab[no].iobuff = etab[no].ps * etab[no].nloader ;   /* total number of loaders */
                                        cimutex(&etab[no].pmutex);
                                        cicondvar(&etab[no].pcvp);
                                        cicondvar(&etab[no].pcvc);
                                        for ( i = 0, n = no; i < etab[j].nloader ; i++ ) {
                                            etabnew ( no++ );           /* Create new etab entry based on original 'c' struct */
                                            etab[no].type = 'C';
                                            etab[no].mpre = tmpre ;
                                            etab[no].id = no;
                                            etab[no].parent = n;
                                            etab[no - 1].child = no;
                                        }
                                    } else if ( etab[no].type == 'd' ) {
                                        etab[no].parent = no;
                                        cimutex(&etab[no].pmutex);
                                        cicondvar(&etab[no].pcvp);
                                        cicondvar(&etab[no].pcvc);
                                        etabnew ( no++ );       /* Helper 'D' thread (extract from target) */
                                        etab[no].type = 'D';
                                        etab[no].id = no;
                                        if ( ( j + 1 ) == etab[l].ps )
                                            etab[no].cr = 2;
                                        etab[no].parent = no - 1;
                                        etabnew ( no++ );       /* Helper 'D' thread (extract from target) */
                                        etab[no].type = 'Z';
                                        etab[no].id = no;
                                        etab[no].lstat |= 0003 ;    /* Initial buffers status: src & tgt write available */
                                        etab[no].parent = no - 2;   /* 'Z' threads have two parents (type 'd' and type 'D' threads) */
                                        etab[no].child = no - 1;    /* 'd' parent eid is saved in .parent, 'D' parent id in .child */
                                        etab[no - 2].child = no;    /* d (father) --> Z (child) */
                                        etab[no - 1].child = no;    /* D (father) --> Z (child) */
                                        if ( etab[no].flg & 020000000000 ) {    /* 's' thread entry */
                                            etabnew ( no++ );           /* Helper 'Z' thread */
                                            etab[no].type = 's';
                                            etab[no].id = no;
                                            etab[no].parent = no - 1;   /* 's' threads parents are 'Z' (compare) threads */
                                        }
                                    }
                                    no++;
                                }
                            } else if ( etab[no].dbt == ORACLE || etab[no].dbt == SQLSERVER ) {
                                /* Parallel option without splitby and source=ORACLE */
                                /* replicate 'ps' etab entries */
                                if ( etab[no].type == 'C' ) {
                                    l = etab[no].parent;
                                    etab[no].mpre = tmpre ;
                                    etab[l].ps = etab[no].ps;
                                } else {
                                    l = no ;
                                }
                                cimutex(&etab[no].pmutex);
                                
                                for ( no++, j = 1 ; j < etab[l].ps ; j++ ) {
                                    etabnew ( l );
                                    etab[no].id = no;
                                    etab[no].parent = l;
                                    etab[no].sbmin = j; /* default current thread number */
                                    if ( etab[no].flg & 01000 ) {   /* if multi open the output file now */
                                        buff[bl] = '\0';
                                        snprintf(buff2, sizeof(buff2), ".%04d", j+1);
                                        strmcat (buff, buff2, sizeof(buff), 0 ); 
                                        if ( etabout ( no , buff ) ) {
                                            goto etabadd_exit;
                                        }
                                    }
                                    etab[no].tbe = 1;
                                    if ( etab[no].type == 'c' ) {
                                        etab[no].parent = no;
                                        cimutex(&etab[no].pmutex);
                                        cicondvar(&etab[no].pcvp);
                                        cicondvar(&etab[no].pcvc);
                                        for ( i = 0, n = no; i < etab[j].nloader ; i++ ) {
                                        etabnew ( no++ );           /* Create new etab entry based on original 'c' struct */
                                            etab[no].type = 'C';
                                            etab[no].mpre = tmpre ;
                                            etab[no].id = no;
                                            etab[no].parent = n;
                                            etab[no - 1].child = no;
                                        }
                                    }
                                    no++;
                                }
                            } 
                            else {
                                /* Parallel option without splitby with unsopported DB */
                                fprintf(stderr, "odb [etabadd(%d)] - Error: \"parallel\" without \"splitby\" is"
                                                " only supported by Oracle and SQL Server databases \n",
                                                __LINE__);
                                goto etabadd_exit;
                            }

                            /* append the remainder to original etab */
                            if (etab[or].TotalMaxRecords)
                                etab[or].mr += etab[or].TotalMaxRecords%etab[or].ps;

                            (void)SQLFreeStmt(Os1, SQL_CLOSE);
                            (void)SQLFreeStmt(Os1, SQL_UNBIND);
                        } else {
                            no++;
                        }
                        if ( f & 020000 )   /* if verbose */
                            fprintf(stderr, "output to %s\n", buff);
                    }
                    tabn[0] = '\0';
                    (void)SQLFreeStmt(Os, SQL_CLOSE);
                    (void)SQLFreeStmt(Os, SQL_UNBIND);
                }
            } while ( fl ) ;
            if ( Os ) 
                (void)SQLFreeHandle(SQL_HANDLE_STMT, Os);
            if ( Os1 ) 
                (void)SQLFreeHandle(SQL_HANDLE_STMT, Os1);
        }
        if ( no == or ) {
            fprintf(stderr, "odb [etabadd(%d)] - No table(s) to extract for \'src=%s\'\n",
                __LINE__, etab[no].src);
            goto etabadd_exit;
        }
    } else {
        etab[no].run = run;
        etab[no++].tbe = 1;
    }
    if ( tn ) { /* tpar option activated */
        if ( !fl ) {
            fprintf(stderr, "odb [etabadd(%d)] - No table list: tpar ignored\n", __LINE__);
            tn = 0 ;
        } else {
            switch ( type ) {
            case 'l':
                tn = tn * ( etab[no].ps + 1 ) ;
                break;
            case 'c':
            case 'p':
                if ( etab[no].ps )
                    tn = tn * etab[no].ps * ( etab[etab[no].parent].nl + 1 ) ;
                else
                    tn = tn * ( etab[etab[no].parent].nl + 1 ) ;
                break;
            case 'd':
                if ( etab[no].ps )
                    tn = tn * etab[no].ps * 3 ;
                else
                    tn = tn * 3 ;
                break;
            case 'e':
                if ( etab[no].ps )
                    tn = tn * etab[no].ps * 2 ;
                else
                    tn = tn * 2 ;
                break;
            }
        }
    }
    return;
    etabadd_exit:   /* something was wrong.... clean and return */
    if ( esrc )
        free ( esrc ) ;
    no = tn = 0;    
    return;
}

/* etabnew:
 *      initialize - and allocate if needed - a new etab[] structure element.
 *      etab entries are allocated in chunks of ETAB_CHUNK.
 *
 *      n(I): existing structure id to copy into the new one (-1 start from scratch)
 *
 *      return: void
 */
static void etabnew(int ix)
{
    struct execute zero = { 0 } ;

    if ( no >= ( nae * ETAB_CHUNK - 2 ) ) { /* need new memory */
        if ((etab=realloc(etab, ++nae*ETAB_CHUNK*sizeof(struct execute)))==(void *)NULL) {
            fprintf(stderr, "odb [etabnew(%d)] - Error allocating %dnth block of etab[] memory: [%d] %s\n",
                __LINE__, nae, errno, strerror(errno));
            exit( EX_OSERR );
        }
    }

    if ( ix < 0 ) {
        etab[no] = zero;
        etab[no].r = r;
        etab[no].fo = stdout;
        etab[no].parent = no;
        etab[no].fsl = 1;
        etab[no].bpwc = BYTESPWCHAR ;
        etab[no].bpc = BYTESPCHAR ;
        etab[no].buffsz = RWBUFF_LEN ;
    } else {
        etab[no] = etab[ix];
    }
}

/* etabshuffle:
 *      randomize etab[] array .run elements. This function SHOULD NOT be used
 *      in a multi-threaded environment. Is it safe to use it with:
 *      - with serial runs: always
 *      - with multi-thread tests to shuffle etab[] BEFORE threads are created
 *
 *      return: void
 */
static void etabshuffle()
{
    int i=0,                    /* loop variable */
        j=0;                    /* loop variable */
    char *save;                 /* used as a temp buffer */
    
    srand((unsigned int)time(0));
    if ( no ) {                 /* more than one element in etab[] */
        for ( i = 0; i < no - 1 ; i++ ) {
            if ( etab[i].type != 'q' ) {    /* etab[] type 'q' entries are linked to qtab[] */
                j = (int) ( i + rand() / ( RAND_MAX + 1.0 ) * ( no - i ) );
                save = etab[j].run;
                etab[j].run = etab[i].run;
                etab[i].run = save;
            }
        }
    }
}

/* etabout:
 *      Open execution output files
 *
 *      eid(I): Execution ID
 *
 *      return: 0 if everything was ok ; -1 in case of errors
 */
static int etabout ( int eid , char *file )
{
    if ( !(etab[eid].flg2 & 0100) ) {       /* NOT HDFS output file */ 
        if ( !file || !strcmp( file, "stdout" ) ) {
            if ( etab[no].flg & 01000 ) {
                fprintf(stderr, "odb [etabout(%d)] - Error setting stdout if multi is enabled\n", __LINE__);
                return ( -1 );
            }
            etab[eid].fo = stdout;
        } else {
            if ((etab[eid].fo=fopen((file[0]=='+'?&file[1]:file), (file[0]=='+'?"a":"w")))==(FILE *)NULL) {
                fprintf(stderr,"odb [etabout(%d)] - Error opening out file %s: [%d] %s\n",
                    __LINE__, file, errno, strerror(errno) );
                return (-1);
            }
            if ( etab[eid].flg2 & 0400000000 ) {    /* user defined iobuff */
                if ( !(etab[eid].flg2 & 0100) ) {   /* no HDFS output file */
                    if ( setvbuf( etab[eid].fo, (char *)NULL, _IOFBF, (size_t)etab[eid].iobuff) )   /* setting IO buff size */
                        fprintf(stderr,"odb [etabout(%d)] - Error setting IO buff size to %zu : [%d] %s\n",
                            __LINE__, etab[eid].iobuff, errno, strerror(errno) );
                }
            }
        }
#ifdef HDFS
    } else {                                /* HDFS output file */ 
        if ( ( etab[no].fho = (*hdfsopen)(hfs, file, O_WRONLY|O_CREAT, (int)etab[no].iobuff, 0, etab[no].hblock) ) == (hdfsFile) NULL ) {
            fprintf(stderr, "odb [etabout(%d)] - Error opening hdfs file %s\n",
                __LINE__, file);
            return (-1);
        }
#endif
    }
    return ( 0 );
}

/* cimutex:
 *      Create and Initialize a mutex object
 *      - mutexp: pointer to a mutex object
 *
 *      return: void
 */
static void cimutex(Mutex *mutexp)
{
#ifdef _WIN32
    InitializeCriticalSection(mutexp);
#else
    if ( pthread_mutex_init(mutexp, NULL) ) {
        fprintf(stderr, "odb [cimutex(%d)] - Error initializing mutex object: [%d] %s\n",
            __LINE__, errno, strerror(errno));
        return;
    }
#endif
}

/* cicondvar:
 *      Create and Initialize a Condition Variable object
 *      - condvarp: pointer to a condition variable object
 *
 *      return: void
 */
static void cicondvar(CondVar *condvarp)
{
#ifdef _WIN32
    InitializeConditionVariable(condvarp);
#else
    if ( pthread_cond_init(condvarp, NULL) ) {
        fprintf(stderr, "odb [cicondvar(%d)] - Error initializing cond var object: [%d] %s\n",
            __LINE__, errno, strerror(errno));
        return;
    }
#endif
}

/* Oload:
 *      load files using parameters in etab[eid]
 *
 *      eid (I): etab entry ID to run
 *
 *      return: void
 */
static void Oload(int eid)
{
    int tid = etab[eid].id;     /* Thread ID */
    SQLCHAR *Oins = 0;          /* INSERT Statement */
    SQLCHAR *Odel = 0;          /* DELETE Statement */
    SQLCHAR *O = 0;             /* ODBC temp variable for memory realloc */
    SQLRETURN Or=0;             /* ODBC return value */
    unsigned long nw=0,         /* no of wait cycles */
                  nt=0,         /* No of total write cycles */
                  nrf=0;        /* no of read records */
    long telaps = 0,            /* Elapsed time in ms */
         tinit = 0,             /* Initialization time in ms */
         fsize = 0;             /* embedded file size */
    size_t bs = TD_CHUNK,       /* td[] buffer size */
           nb = 0,              /* number of bytes read from input file */
           mfl = 0,             /* max field length */
           cl = CMD_CHUNK,      /* INSERT command buffer length */
           cmdl = 0,            /* INSERT command length */
           dell = CMD_CHUNK,    /* DELETE command length */
           len = 0;             /* used to scroll the IO buffer */
    int ch = 0,                 /* char read from data file */                  
         z = 0,                 /* loop variable */
         t = 0,                 /* loop variable */
         gzret = 0,             /* zlib function return values */
         ifl=0;                 /* input field length */
    unsigned int nldr=0,        /* number of loaders */
                 p=0,           /* current position in the IO buffer */
                 k=0,           /* input file field number */
                 n=0,           /* input file row number */
                 m = 0,         /* rowset array record number */
                 mi = 1,        /* rowset array record number increment */
                 c=0,           /* current column in the record read from file */
                 mff=0,         /* max file field number mapped to a table column */
                 nmf=0,         /* number of mapped fields */
                 i=0,           /* loop variable */
                 ffstart=0,     /* fixed field start position */
                 ffend=0,       /* fixed field end position */
                 rnd=0,         /* random number */
                 l=0,           /* destination table fields */
                 o=0,           /* loop variable */
                 j=0,           /* loop variable */
                 lts = etab[eid].k ,    /* lines to skip */
                 isgz=0;        /* input file is gzipped: 0=no , 1=yes */
    int mcfsl = etab[eid].mcfs ? (int)strlen(etab[eid].mcfs) : 0; /* multi character field separator length */
    int mcrsl = etab[eid].mcrs ? (int)strlen(etab[eid].mcrs) : 0; /* multi character record separator length */
    unsigned char fg = 0,       /* Oload flags:
                                    0001 = in a quoted string   0004 = fixed fields      0020 = field ready
                                    0002 = nofile               0010 = delimited fields  0040 = record ready
                                    0100 = escape flag          0200 = embed file read  */
                  ccl = 0,      /* Continue cleaning to RS in the next buffer */
                  pstats = 0,   /* Error flag: 0 = print stats, 1 = don't print stats */
                  lfs = etab[eid].fs,                                 /* local field separator */
                  lrs = etab[eid].rs,                                 /* local record separator */
                  lsq = etab[eid].sq ? etab[eid].sq : '"',            /* local string qualifier, deafult is '"' */
                  lem = etab[eid].em,                                 /* local embed character */
                  lec = etab[eid].ec;                                 /* local escape character */
    int *ldrs=0,                /* pointer to array containing loaders EIDs */
        *rmap=0;                /* Input File Fields Map (reverse map):
                                    >=0 maps the correspoding Table Column Number
                                    -1 field to be ignored */
    FILE *fl=0,                 /* data to load file pointer */
         *fm=0,                 /* loadmap file pointer */
         *fe=0;                 /* embed file pointer */
#ifdef HDFS
    hdfsFile fhl = 0;           /* Hadoop Input File Handle */
#endif
    char *buff = 0,             /* IO buffer */
         *gzbuff = 0,           /* GZIP IO buffer */
         *bp = 0,               /* used to browse line read from mapfile */
         *str = 0,              /* field buffer */
         *sp = 0;               /* string pointer loop variable */
    struct m {                  /* Table Column Map */
        int min;                /* min value */
        int max;                /* max value */
        int idx;                /* target table col number:
                                    >=0 maps the corresponding Input File Field number
                                    -1 const/$var   -2 seq       -3 irand    -4 drand
                                    -5 tmrand       -6 tsrand    -7 crand    -8 emrand
                                    -9 null        -10 fixed    -11 cdate   -12 ctime
                                   -13 ctstamp     -14 dsrand   -15 txtrand -16 nrand
                                   -17 lstrand */
        size_t cl;              /* Constant length */
        char *c;                /* pointer to constant */
        char trf[16],           /* translit from array */
             trt[16];           /* translit to array */         
        char op;                /* 1=substr, 2=dconv, 3=tconv, 4=tsconv, 5=replace,
                                   6=toupper, 7=tolower, 8=firstup, 9=csubstr, 10=translit,
                                   11=comp, 12=comp3, 13=zoned, 14=emptyasconst, 15=emptyasempty,
                                   16=div, 17 trim */
        char **el;              /* dataset element array pointer */
        unsigned int prec;      /* COMP3/ZONED Precision */
        unsigned int scale;     /* COMP3/ZONED Scale */
        unsigned int *eln;      /* dataset element length array pointer */
    } *map = 0;
    struct tm *dt;              /* tm struct for random date/timestamp generation */
    time_t trnd;                /* to generate random time/timestamp(s) */
    char num[32];               /* Formatted Number String */
    char tim[15];               /* Formatted Time String */
    struct timeval tve;         /* timeval struct to define elapesd/timelines */
    SQLCHAR *Odp = 0;           /* rowset buffer data pointer */
    double seconds = 0;         /* seconds used for timings */
    z_stream gzstream = { 0 } ; /* zlib structure for gziped files */
                                
    /* Check if we have to use another ODBC connection */
    if ( thps[tid].cr > 0 ) {
        thps[tid].Oc = thps[thps[tid].cr].Oc;
        thps[tid].Os = thps[thps[tid].cr].Os;
    }

    /* Set "tgt" variable */
    var_set ( &thps[tid].tva, VTYPE_I, "tgt", etab[eid].tgt );

    /* Run "pre" SQL */
    if ( etab[eid].pre ) {
        etab[eid].flg2 |= 020000000 ;           /* Oexec to allocate/use its own handle */
        if ( etab[eid].pre[0] == '@' )          /* run a sql script */
            z = runsql(tid, eid, 0, (etab[eid].pre + 1 ));
        else                                    /* Run single SQL command */
            z = Oexec(tid, eid, 0, 0, (SQLCHAR *)etab[eid].pre, "");
        etab[eid].flg2 &= ~020000000 ;          /* reset Oexec to allocate/use its own handle */
        if ( z && etab[eid].flg & 0004 )
            goto oload_exit;
        etab[eid].nr = 0;   /* reset number of resulting rows */
        (void)SQLFreeHandle(SQL_HANDLE_STMT, thps[tid].Os);
        if ( !SQL_SUCCEEDED(Or=SQLAllocHandle(SQL_HANDLE_STMT, thps[tid].Oc, &thps[tid].Os))){
            Oerr(eid, tid, __LINE__, Oc, SQL_HANDLE_DBC);
            goto oload_exit;
        }
    }

    /* Check database type */
    etab[eid].dbt = checkdb( eid, &thps[tid].Oc, NULL, NULL);

    /* Check if truncate */
    if ( etab[eid].flg & 0002 ) {           
        etab[eid].flg &= ~0200000;      /* if truncate... unset ifempty */
        if ( ( Odel = malloc(dell) ) == (void *)NULL ) {
            fprintf(stderr, "odb [Oload(%d)] - Error allocating Odel memory: [%d] %s\n",
                __LINE__, errno, strerror(errno));
            goto oload_exit;
        }
        strmcpy((char *)Odel, dbscmds[etab[eid].dbt].trunc, dell);
        Odel = (SQLCHAR *)var_exp((char *)Odel, &dell, &thps[tid].tva);
    }
        
    /* check ifempty */
    if ( etab[eid].flg & 0200000 ) {    /* ifempty is set */
        if ( ifempty( eid, etab[eid].tgt ) ) {
            fprintf(stderr, "odb [Oload(%d)] - Target table %s is not empty\n",
                    __LINE__, etab[eid].tgt);
            etab[eid].post = 0; /* prevent post SQL execution */
            goto oload_exit;
        }
    }
    
    /* Initialize gzstream structure */
    gzstream.zalloc = Z_NULL;
    gzstream.zfree = Z_NULL;
    gzstream.opaque = Z_NULL;
    gzstream.next_in = (unsigned char *)gzbuff;
    gzstream.avail_in = 0;
    gzstream.next_out = (unsigned char *)buff;
        
    /* Initialize INSERT statement */
    cl += strlen(etab[eid].tgt);
    if ( ( Oins = malloc ( cl ) ) == (void *) NULL ) {
        fprintf(stderr, "odb [Oload(%d)] - Error allocating Oins memory\n", __LINE__);
        goto oload_exit;
    }
    if (!strcasecmp(etab[eid].loadcmd, "UL"))
    {
        cmdl += snprintf((char *)Oins, cl, "UPSERT USING LOAD %s%sINTO %s(\"",
            etab[eid].flg & 040000000 ? "/*+ DIRECT */ " : "",
            etab[eid].flg2 & 0002 ? "WITH NO ROLLBACK " : "",
            etab[eid].tgt);
    }
    else if (!strcasecmp(etab[eid].loadcmd, "UP"))
    {
        cmdl += snprintf((char *)Oins, cl, "UPSERT %s%sINTO %s(\"",
            etab[eid].flg & 040000000 ? "/*+ DIRECT */ " : "",
            etab[eid].flg2 & 0002 ? "WITH NO ROLLBACK " : "",
            etab[eid].tgt);
    }
    else if (!strcasecmp(etab[eid].loadcmd, "IN"))
    {
        cmdl += snprintf((char *)Oins, cl, "INSERT %s%sINTO %s(\"",
            etab[eid].flg & 040000000 ? "/*+ DIRECT */ " : "",
            etab[eid].flg2 & 0002 ? "WITH NO ROLLBACK " : "",
            etab[eid].tgt);
    }

    /* Allocate io buffer */
    if ( ( buff = malloc((size_t)etab[eid].buffsz) ) == (void *)NULL ) {
        fprintf(stderr, "odb [Oload(%d)] - Error allocating IO buffer: [%d] %s\n",
            __LINE__, errno, strerror(errno));
        goto oload_exit;
    }
    memset ( buff, 0, (size_t)etab[eid].buffsz);

    /* Open input file */
    if ( !strcmp ( etab[eid].src , "stdin" ) ) {
        fl = stdin ;
    } else if ( !strmicmp ( etab[eid].src, "nofile", 6) ) {
        if ( ! ( len = (size_t) etab[eid].mr ) )    /* if max record is not set */
            len = 100;                              /* set it to 100 */
        fg |= 0002;                                 /* set nofile flag */
    } else {
        for ( i = j = 0; i < etab[eid].buffsz && etab[eid].src[i]; i++ ) {
            switch ( etab[eid].src[i] ) {
            case '%':
                switch ( etab[eid].src[++i] ) {
                case 't':
                    j += strmcat (buff, (char *)etab[eid].Ocso[2], (size_t)etab[eid].buffsz, 2 ); 
                    break;
                case 'T':
                    j += strmcat (buff, (char *)etab[eid].Ocso[2], (size_t)etab[eid].buffsz, 1 ); 
                    break;
                case 's':
                    j += strmcat (buff, (char *)etab[eid].Ocso[1], (size_t)etab[eid].buffsz, 2 ); 
                    break;
                case 'S':
                    j += strmcat (buff, (char *)etab[eid].Ocso[1], (size_t)etab[eid].buffsz, 1 ); 
                    break;
                case 'c':
                    j += strmcat (buff, (char *)etab[eid].Ocso[0], (size_t)etab[eid].buffsz, 2 ); 
                    break;
                case 'C':
                    j += strmcat (buff, (char *)etab[eid].Ocso[0], (size_t)etab[eid].buffsz, 1 ); 
                    break;
                default:
                    fprintf(stderr, "odb [Oload(%d)] - Invalid expansion %%%c in %s\n",
                        __LINE__, etab[eid].src[i], etab[eid].src );
                    goto oload_exit;
                }
                break;
            default:
                buff[j++] = etab[eid].src[i];
                break;
            }
        }
        buff[j] = '\0';
        if ( !(etab[eid].flg2 & 0100) ) {   /* No HDFS input file */
            if ( ( fl = fopen(buff, "r") ) == (FILE *) NULL ) {
                fprintf(stderr, "odb [Oload(%d)] - Error opening input file %s: [%d] %s\n",
                    __LINE__, buff, errno, strerror(errno) );
                goto oload_exit;
            }
            if ( etab[eid].flg2 & 0400000000 ) {    /* user defined iobuff */
                if ( setvbuf( etab[eid].fo, (char *)NULL, _IOFBF, (size_t)etab[eid].iobuff) )   /* setting IO buff size */
                    fprintf(stderr,"odb [Oload(%d)] - Error setting IO buff size to %zu : [%d] %s\n",
                        __LINE__, etab[eid].iobuff, errno, strerror(errno) );
            }
        }
    }

    /* Open map file */
    if ( etab[eid].map && ( fm = fopen(etab[eid].map, "r") ) == (FILE *) NULL ) {
        fprintf(stderr, "odb [Oload(%d)] - Error opening map file %s: [%d] %s\n",
                __LINE__, etab[eid].map, errno, strerror(errno) );
        goto oload_exit;
    }

#ifdef HDFS
    /* Open HDFS input file */
    if ( etab[eid].flg2 & 0100 ) {  /* HDFS input file */ 
        if ( ( fhl = (*hdfsopen)(hfs, buff, O_RDONLY, (int)etab[eid].iobuff, 0, 0) ) == (hdfsFile) NULL ) {
            fprintf(stderr, "odb [Oload(%d)] - Error opening hdfs file %s\n",
                __LINE__, buff);
            goto oload_exit;
        }
    }
#endif

    /* Analyze target table */
    z = Otcol(eid, &thps[tid].Oc) ;
    if ( z <= 0 ) {
        fprintf(stderr, "odb [Oload(%d)] - Error analyzing target table %s.%s.%s\n", 
            __LINE__, etab[eid].Ocso[0], etab[eid].Ocso[1], etab[eid].Ocso[2]);
        goto oload_exit;
    } else {
        l = (unsigned int)z ;
    }

    /* Initilize ODBC dump */
    if ( fdmp ) {
        for ( j = 0 ; j < l ; j++ ) {
            fprintf(fdmp, "[%d] Column %d: name=%s, type=%d (%s), size=%lu, decimals=%d, nullable=%s\n",
                tid, j, (char *)etab[eid].td[j].Oname, (int)etab[eid].td[j].Otype,
                expandtype(etab[eid].td[j].Otype), (unsigned long)etab[eid].td[j].Osize,
                (int)etab[eid].td[j].Odec, etab[eid].td[j].Onull ? "yes" : "no" ) ; 
        }
    }

    /* Adjust Osize for WCHAR field (up to 4 bytes/char) */
    if ( etab[eid].dbt != VERTICA ) {       /* Vertica's CHAR field length is in bytes (not chars) */
        for ( j = 0 ; j < l ; j++ ) {
            switch ( etab[eid].td[j].Otype ) {
            case SQL_WCHAR:
            case SQL_WVARCHAR:
            case SQL_WLONGVARCHAR:
                etab[eid].td[j].Osize *= etab[eid].bpwc;    /* Space for UTF-8 conversion */
                break;
            case SQL_CHAR:
            case SQL_VARCHAR:
            case SQL_LONGVARCHAR:
                etab[eid].td[j].Osize *= etab[eid].bpc;/* Space for UTF-8 conversion */
                break;
            }
        }
    }

    /* Allocate map structure */
    if ( ( map = (struct m *)calloc ( l , sizeof(struct m) ) ) == (void *)NULL ) {
        fprintf(stderr, "odb [Oload(%d)] - Error allocating %u memory elements for map[]\n", 
            __LINE__, l);
        goto oload_exit;
    }

    /* Fill map structure */
    if ( fm ) {             /* if a loadmap file exists */
        while ( fgets(buff, (int)etab[eid].buffsz, fm ) ) {
            if ( buff[0] == '#' )   /* Comment: do nothing */
                continue;
            buff[strlen(buff) - 1] = '\0';
            if (strlen(buff) <= 1)
                continue;
            for ( i = 0 ; buff[i] && buff[i] != ':' ; i++ );
            for ( j = 0 ; j < l ; j++ ) {
                if ( (len=strlen((char *)etab[eid].td[j].Oname))==(size_t)i )
                    if ( !strncmp(buff, (char *)etab[eid].td[j].Oname, (size_t)i) )
                        break;
            }
            if ( j == l ) {
                fprintf(stderr, "odb [Oload(%d)] - Error table column not found for:\n%s\n", 
                        __LINE__, buff);
                fclose(fm);
                goto oload_exit;
            } else {
                etab[eid].td[j].dl = 1;
                nmf++;
            }
            bp = buff + i + 1 ;
            map[j].op = 0;
            if (!strmicmp ( "const", bp, 5 ) ) {
                map[j].idx = (-1);
                while ( *bp && *bp++ != ':' );
                map[j].cl = strlen ( bp );
                if ((map[j].c = (char *)malloc ( map[j].cl + 1 )) == (void *)NULL) {
                    fprintf(stderr, "odb [Oload(%d)] - CONST error allocating memory for %s\n", 
                        __LINE__, (char *)etab[eid].td[j].Oname);
                    goto oload_exit;
                } else {
                    strmcpy(map[j].c, bp, map[j].cl);
                }
            } else if (!strmicmp ( "seq", bp, 3 ) ) {
                map[j].idx = (-2);
                while ( *bp && *bp++ != ':' );
                map[j].min = (int) strtol ( bp, NULL, 10);
            } else if (!strmicmp ( "irand", bp, 5 ) ) {
                map[j].idx = (-3);
                while ( *bp && *bp++ != ':' );
                map[j].min = (int) strtol ( bp, NULL, 10);
                while ( *bp && *bp++ != ':' );
                map[j].max = (int) strtol ( bp, NULL, 10);
                if ( map[j].min > map[j].max ) {
                    fprintf(stderr, "odb [Oload(%d)] - IRAND error: min length (%d) greater than max (%d)\n",
                        __LINE__, map[j].min, map[j].max );
                    goto oload_exit;
                }
            } else if (!strmicmp ( "drand", bp, 5 ) ) {
                map[j].idx = (-4);
                while ( *bp && *bp++ != ':' );
                map[j].min = (int) strtol ( bp, NULL, 10);
                while ( *bp && *bp++ != ':' );
                map[j].max = (int) strtol ( bp, NULL, 10);
                if ( map[j].min > map[j].max ) {
                    fprintf(stderr, "odb [Oload(%d)] - DRAND error: min value (%d) greater than max (%d)\n",
                        __LINE__, map[j].min, map[j].max );
                    goto oload_exit;
                }
            } else if (!strmicmp ( "tmrand", bp, 6 ) ) {
                map[j].idx = (-5);
            } else if (!strmicmp ( "tsrand", bp, 6 ) ) {
                map[j].max = (int) time(0);
                map[j].idx = (-6);
            } else if (!strmicmp ( "crand", bp, 5 ) ) {
                map[j].idx = (-7);
                while ( *bp && *bp++ != ':' );
                map[j].cl = (size_t) strtol ( bp, NULL, 10);
            } else if (!strmicmp ( "emrand", bp, 6 ) ) {
                map[j].idx = (-8);
                while ( *bp && *bp++ != ':' );
                map[j].min = (int) strtol ( bp, NULL, 10);  /* username min length */
                while ( *bp && *bp++ != ':' );
                map[j].max = (int) strtol ( bp, NULL, 10);  /* username max length */
                if ( map[j].min > map[j].max ) {
                    fprintf(stderr, "odb [Oload(%d)] - EMRAND error: min userlength (%d) greater than max (%d)\n",
                        __LINE__, map[j].min, map[j].max );
                    goto oload_exit;
                }
                while ( *bp && *bp++ != ':' );
                map[j].prec = (int) strtol ( bp, NULL, 10); /* domain min length */
                while ( *bp && *bp++ != ':' );
                map[j].scale = (int) strtol ( bp, NULL, 10);/* domain max length */
                if ( map[j].prec > map[j].scale ) {
                    fprintf(stderr, "odb [Oload(%d)] - EMRAND error: min domainlength (%d) greater than max (%d)\n",
                        __LINE__, map[j].min, map[j].max );
                    goto oload_exit;
                }
                while ( *bp && *bp++ != ':' );
                map[j].cl = strlen(bp);
                /* Allocate memory to contain dataset and copy it*/
                if ( ( map[j].c = (char *)malloc ( (map[j].cl + 1 ) ) ) == (void *)NULL) {
                    fprintf(stderr, "odb [Oload(%d)] - EMRAND error allocating memory for %s\n", 
                        __LINE__, (char *)etab[eid].td[j].Oname);
                    goto oload_exit;
                }
                MEMCPY(map[j].c, bp , map[j].cl);
                map[j].c[map[j].cl] = '\0';
                /* count dataset elements (lines) */
                for ( map[j].cl = 1 , sp = map[j].c ; *sp ; sp++ )
                    if ( *sp == ',' )
                        map[j].cl++;
                /* allocate memory for dataset element array */
                if ( ( map[j].el = calloc ( map[j].cl, sizeof(char *) ) ) == (void *)NULL) {
                    fprintf(stderr, "odb [Oload(%d)] - EMRAND error allocating element array for %s\n", 
                        __LINE__, (char *)etab[eid].td[j].Oname);
                    goto oload_exit;
                }
                /* allocate memory for dataset element length array */
                if ( ( map[j].eln = calloc ( map[j].cl, sizeof(unsigned int) ) ) == (void *)NULL) {
                    fprintf(stderr, "odb [Oload(%d)] - EMRAND error allocating element length array for %s\n", 
                        __LINE__, (char *)etab[eid].td[j].Oname);
                    goto oload_exit;
                }
                /* tokenize dataset and fill dataset element array */
                for ( z = 0, sp = map[j].c, map[j].el[0] = map[j].c, o = 0 ; *sp ; sp++ ) {
                    if ( *sp == ',' ) {
                        *sp = '\0';
                        if ( map[j].eln[z] > o )
                            o = map[j].eln[z];  /* record max dataset element length */
                        if ( ++z < (int)map[j].cl )
                            map[j].el[z] = sp + 1 ;
                    } else {
                        map[j].eln[z]++;
                    }
                }
                if ( ( o + map[j].max + map[j].scale ) > (unsigned int)etab[eid].td[j].Osize ) {
                    fprintf(stderr, "odb [Oload(%d)] - EMRAND error: total email length greater than \'%s\' target column length (%d)\n", 
                        __LINE__, (char *)etab[eid].td[j].Oname, (int)etab[eid].td[j].Osize);
                    goto oload_exit;
                }
            } else if (!strmicmp ( "null", bp, 4 ) ) {
                map[j].idx = (-9);
            } else if (!strmicmp ( "cdate", bp, 5 ) ) {
                map[j].idx = (-11);
            } else if (!strmicmp ( "ctime", bp, 5 ) ) {
                map[j].idx = (-12);
            } else if (!strmicmp ( "ctstamp", bp, 7 ) ) {
                map[j].idx = (-13);
            } else if (!strmicmp ( "dsrand", bp, 6 ) ) {
                map[j].idx = (-14);
                while ( *bp && *bp++ != ':' );
                if ( ( fe = fopen(bp, "rb") ) == (FILE *) NULL ) {
                    fprintf(stderr, "odb [Oload(%d)] - DSRAND error opening %s: [%d] %s\n",
                        __LINE__, bp, errno, strerror(errno) );
                    goto oload_exit;
                }
                (void) fseek(fe, 0L, SEEK_END);     /* goto EOF */
                fsize = ftell ( fe );               /* get file size */
                (void) fseek(fe, 0L, SEEK_SET);     /* rewind */
                /* Allocate memory to contain the whole dataset */
                if ( ( map[j].c = (char *)malloc ( (size_t)(fsize + 1 ) ) ) == (void *)NULL) {
                    fprintf(stderr, "odb [Oload(%d)] - DSRAND error allocating memory for %s\n", 
                        __LINE__, (char *)etab[eid].td[j].Oname);
                    goto oload_exit;
                }
                /* read the dataset from disk and close the input file */
                if ( ( len = fread ( map[j].c, 1, (size_t)fsize, fe) ) != (size_t)fsize ) {
                    fprintf(stderr, "odb [Oload(%d)] - DSRAND error reading from %s (got " SIZET_SPEC " bytes, expected " SIZET_SPEC "): [%d] %s\n",
                        __LINE__, bp, len, (size_t)fsize, errno, strerror(errno) );
                    goto oload_exit;
                }
                *(map[j].c + len) = '\0';   /* write a NULL at the end */
                fclose ( fe ) ;
                /* count dataset elements (lines) */
                for ( map[j].cl = 0 , sp = map[j].c ; *sp ; sp++ )
                    if ( *sp == '\n' || *sp == '\r' )
                    {
                        map[j].cl++;
                        if( *(sp + 1) == '\n' ) /* windows linefeed use \r\n */
                            sp++;
                    }
                /* allocate memory for dataset element array */
                if ( ( map[j].el = calloc ( map[j].cl, sizeof(char *) ) ) == (void *)NULL) {
                    fprintf(stderr, "odb [Oload(%d)] - DSRAND error allocating element array for %s\n", 
                        __LINE__, bp);
                    goto oload_exit;
                }
                /* allocate memory for dataset element length array */
                if ( ( map[j].eln = calloc ( map[j].cl, sizeof(unsigned int) ) ) == (void *)NULL) {
                    fprintf(stderr, "odb [Oload(%d)] - DSRAND error allocating element length array for %s\n", 
                        __LINE__, bp);
                    goto oload_exit;
                }
                /* tokenize dataset and fill dataset element array */
                for ( z = 0, sp = map[j].c, map[j].el[0] = map[j].c ; *sp ; sp++ ) {
                    if ( *sp == '\n' || *sp == '\r' ) {
                        *sp = '\0';
                        if ( *sp == '\r' && *(sp + 1) == '\n' )
                        {
                            *(sp + 1) = '\0';
                            sp++;
                        }
                        if ( map[j].eln[z] > (unsigned int)etab[eid].td[j].Osize ) {
                            fprintf(stderr, "odb [Oload(%d)] - DSRAND warning: dataset element \'%s\' will be truncated to fit \'%s\' column length (%d)\n", 
                                __LINE__, map[j].el[z], (char *)etab[eid].td[j].Oname, (int)etab[eid].td[j].Osize);
                            map[j].eln[z] = (int)etab[eid].td[j].Osize;
                        }
                        if ( ++z < (int)map[j].cl )
                            map[j].el[z] = sp + 1 ;
                    } else {
                        map[j].eln[z]++;
                    }
                }
            } else if (!strmicmp ( "txtrand", bp, 7 ) ) {
                map[j].idx = (-15);
                while ( *bp && *bp++ != ':' );
                map[j].min = (int) strtol ( bp, NULL, 10);
                while ( *bp && *bp++ != ':' );
                map[j].max = (int) strtol ( bp, NULL, 10);
                while ( *bp && *bp++ != ':' );
                if ( ( fe = fopen(bp, "rb") ) == (FILE *) NULL ) {
                    fprintf(stderr, "odb [Oload(%d)] - Error opening %s: [%d] %s\n",
                        __LINE__, bp, errno, strerror(errno) );
                    goto oload_exit;
                }
                (void) fseek(fe, 0L, SEEK_END);     /* goto EOF */
                map[j].cl = (size_t)ftell ( fe );               /* get file size */
                (void) fseek(fe, 0L, SEEK_SET);     /* rewind */
                /* Allocate memory to contain the whole dataset */
                if ( ( map[j].c = (char *)malloc ( (map[j].cl + 1 ) ) ) == (void *)NULL) {
                    fprintf(stderr, "odb [Oload(%d)] - TXTRAND error allocating memory for %s\n", 
                        __LINE__, (char *)etab[eid].td[j].Oname);
                    goto oload_exit;
                }
                /* read the dataset from disk and close the input file */
                if ( ( len = fread ( map[j].c, 1, map[j].cl, fe) ) != map[j].cl ) {
                    fprintf(stderr, "odb [Oload(%d)] - TXTRAND error reading from %s (got " SIZET_SPEC " bytes, expected " SIZET_SPEC "): [%d] %s\n",
                        __LINE__, bp, len, map[j].cl, errno, strerror(errno) );
                    goto oload_exit;
                }
                *(map[j].c + len) = '\0';   /* write a NULL at the end */
                fclose ( fe ) ;
                if ( map[j].min > map[j].max ) {
                    fprintf(stderr, "odb [Oload(%d)] - TXTRAND error: min length (%d) greater than max (%d)\n",
                        __LINE__, map[j].min, map[j].max );
                    goto oload_exit;
                }
                if ( (size_t)map[j].max > map[j].cl ) {
                    fprintf(stderr, "odb [Oload(%d)] - TXTRAND error: max length (%d) exceed file length (" SIZET_SPEC ")\n",
                        __LINE__, map[j].max, map[j].cl );
                    goto oload_exit;
                }
            } else if (!strmicmp ( "nrand", bp, 5 ) ) {
                map[j].idx = (-16);
                while ( *bp && *bp++ != ':' );
                map[j].max = (int) strtol ( bp, NULL, 10);  /* precision */
                while ( *bp && *bp++ != ':' );
                map[j].min = (int) strtol ( bp, NULL, 10);  /* scale */
                if ( map[j].min > map[j].max ) {
                    fprintf(stderr, "odb [Oload(%d)] - NRAND error: scale (%d) greater than precision (%d)\n",
                        __LINE__, map[j].min, map[j].max );
                    goto oload_exit;
                }
                map[j].cl = (size_t)(map[j].max + 1);       /* total string length */
                map[j].max -= map[j].min;                   /* digits before dec sep */
            } else if (!strmicmp ( "lstrand", bp, 7 ) ) {
                map[j].idx = (-17);
                while ( *bp && *bp++ != ':' );
                map[j].cl = strlen(bp);
                /* Allocate memory to contain dataset and copy it*/
                if ( ( map[j].c = (char *)malloc ( (map[j].cl + 1 ) ) ) == (void *)NULL) {
                    fprintf(stderr, "odb [Oload(%d)] - LSTRAND error allocating memory for %s\n", 
                        __LINE__, (char *)etab[eid].td[j].Oname);
                    goto oload_exit;
                }
                MEMCPY(map[j].c, bp , map[j].cl);
                map[j].c[map[j].cl] = '\0';
                /* count dataset elements (lines) */
                for ( map[j].cl = 1 , sp = map[j].c ; *sp ; sp++ )
                    if ( *sp == ',' )
                        map[j].cl++;
                /* allocate memory for dataset element array */
                if ( ( map[j].el = calloc ( map[j].cl, sizeof(char *) ) ) == (void *)NULL) {
                    fprintf(stderr, "odb [Oload(%d)] - LSTRAND error allocating element array memory for %s\n", 
                        __LINE__, (char *)etab[eid].td[j].Oname);
                    goto oload_exit;
                }
                /* allocate memory for dataset element length array */
                if ( ( map[j].eln = calloc ( map[j].cl, sizeof(unsigned int) ) ) == (void *)NULL) {
                    fprintf(stderr, "odb [Oload(%d)] - LSTRAND error allocating element length array memory for %s\n", 
                        __LINE__, (char *)etab[eid].td[j].Oname);
                    goto oload_exit;
                }
                /* tokenize dataset and fill dataset element array */
                for ( z = 0, sp = map[j].c, map[j].el[0] = map[j].c ; *sp ; sp++ ) {
                    if ( *sp == ',' ) {
                        *sp = '\0';
                        if ( map[j].eln[z] > (unsigned int)etab[eid].td[j].Osize ) {
                            fprintf(stderr, "odb [Oload(%d)] - LSTRAND warning: dataset element \'%s\' will be truncated to fit target column %s of length %d\n", 
                                __LINE__, map[j].el[z], (char *)etab[eid].td[j].Oname, (int)etab[eid].td[j].Osize);
                            map[j].eln[z] = (int)etab[eid].td[j].Osize;
                        }
                        if ( ++z < (int)map[j].cl )
                            map[j].el[z] = sp + 1 ;
                    } else {
                        map[j].eln[z]++;
                    }
                }
            } else if ( bp[0] == '$' ) {
                map[j].idx = (-1);
                map[j].c = getenv(bp + 1);
                map[j].cl = strlen ( map[j].c );
            } else {
                if (!strmicmp ( "fixed", bp, 5 ) ) {
                    map[j].idx = j; /* this will be adjusted afterwards */
                    fg |= 0004;     /* fixed format flag */
                    while ( *bp && *bp++ != ':' );
                    map[j].min = (int) strtol ( bp, NULL, 10);
                    while ( *bp && *bp++ != ':' );
                    map[j].max = (int) strtol ( bp, NULL, 10);
                } else {
                    fg |= 0010; /* delimited format flag */
                    map[j].idx = (int)strtol( bp, NULL, 10) - 1;
                    if (map[j].idx < 0) {
                        fprintf(stderr, "odb [Oload(%d)] - Error: Field index should should start from 1\n", __LINE__);
                        goto oload_exit;
                    }
                }
                while ( *bp && *bp++ != ':' );
                if ( *bp ) {
                    if ( !strmicmp ( "substr", bp, 6 ) ) {
                        map[j].op = 1;
                        while ( *bp && *bp++ != ':' );
                        map[j].min = (int) strtol ( bp, NULL, 10);
                        while ( *bp && *bp++ != ':' );
                        map[j].max = (int) strtol ( bp, NULL, 10);
                        if ( map[j].min >= map[j].max ) {
                            fprintf(stderr, "odb [Oload(%d)] - SUBSTR error: min value (%d) is >= max value (%d) for %s\n",
                                __LINE__, map[j].min, map[j].max, (char *)etab[eid].td[j].Oname);
                            goto oload_exit;
                        }
                    } else if ( !strmicmp ( "dconv", bp, 5 ) ) {
                        map[j].cl = strlen ( bp+6 );
                        map[j].op = 2;
                        if ((map[j].c = (char *)malloc ( map[j].cl + 1 )) == (void *)NULL) {
                            fprintf(stderr, "odb [Oload(%d)] - DCONV error allocating format string memory for %s\n", 
                                __LINE__, (char *)etab[eid].td[j].Oname);
                            goto oload_exit;
                        } else {
                            strmcpy(map[j].c, bp+6, map[j].cl);
                        }
                    } else if ( !strmicmp ( "tconv", bp, 5 ) ) {
                        map[j].cl = strlen ( bp+6 );
                        map[j].op = 3;
                        if ((map[j].c = (char *)malloc ( map[j].cl + 1 )) == (void *)NULL) {
                            fprintf(stderr, "odb [Oload(%d)] - TCONV error allocating format string memory for %s\n", 
                                __LINE__, (char *)etab[eid].td[j].Oname);
                            goto oload_exit;
                        } else {
                            strmcpy(map[j].c, bp+6, map[j].cl);
                        }
                    } else if ( !strmicmp ( "tsconv", bp, 6 ) ) {
                        map[j].cl = strlen ( bp+7 );
                        map[j].op = 4;
                        if ((map[j].c = (char *)malloc ( map[j].cl + 1 )) == (void *)NULL) {
                            fprintf(stderr, "odb [Oload(%d)] - TSCONV error allocating format string memory for %s\n", 
                                __LINE__, (char *)etab[eid].td[j].Oname);
                            goto oload_exit;
                        } else {
                            strmcpy(map[j].c, bp+7, map[j].cl);
                        }
                    } else if ( !strmicmp ( "replace", bp, 7 ) ) {
                        map[j].cl = strlen ( bp+8 );
                        map[j].op = 5;
                        if ((map[j].c = (char *)malloc ( map[j].cl + 1 )) == (void *)NULL) {
                            fprintf(stderr, "odb [Oload(%d)] - REPLACE error allocating memory for %s\n", 
                                __LINE__, (char *)etab[eid].td[j].Oname);
                            goto oload_exit;
                        } else {
                            strmcpy(map[j].c, bp+8, map[j].cl);
                        }
                        for ( i = 0 ; map[j].c[i] && map[j].c[i] != ':' ; i++ );
                        map[j].max = (int) map[j].cl - i - 1;
                        if ( map[j].max > (int)etab[eid].td[j].Osize )
                            map[j].max = (int)etab[eid].td[j].Osize;
                        map[j].cl = i + 1;
                        map[j].c[i] = '\0';
                    } else if ( !strmicmp ( "toupper", bp, 7 ) ) {
                        map[j].op = 6;
                    } else if ( !strmicmp ( "tolower", bp, 7 ) ) {
                        map[j].op = 7;
                    } else if ( !strmicmp ( "firstup", bp, 7 ) ) {
                        map[j].op = 8;
                    } else if ( !strmicmp ( "csubstr", bp, 7 ) ) {
                        map[j].op = 9;
                        while ( *bp && *bp++ != ':' );
                        map[j].min = (int) mchar ( bp );
                        while ( *bp && *bp++ != ':' );
                        map[j].max = (int) mchar ( bp );
                    } else if ( !strmicmp ( "translit", bp, 8 ) ) {
                        map[j].op = 10;
                        while ( *bp && *bp++ != ':' );
                        for ( z = 0 ; *bp && *bp != ':' && z < 16 ; z++, bp++ ) {
                            if ( *bp == '\\' ) {
                                switch ( *++bp ) {
                                case 't':
                                    map[j].trf[z] = 9;
                                    break;
                                case 'n':
                                    map[j].trf[z] = 10;
                                    break;
                                case 'v':
                                    map[j].trf[z] = 11;
                                    break;
                                case 'f':
                                    map[j].trf[z] = 12;
                                    break;
                                case 'r':
                                    map[j].trf[z] = 13;
                                    break;
                                case 'e':
                                    map[j].trf[z] = 27;
                                    break;
                                default:
                                    fprintf(stderr, "odb [Oload(%d)] - TRANSLIT error: invalid escape \\%c\n", 
                                        __LINE__, *bp);
                                    goto oload_exit;
                                }
                            } else {
                                map[j].trf[z] = *bp;
                            }
                        }
                        for ( t = 0, bp++ ; *bp && *bp != ':' && t < 16 ; t++, bp++ ) {
                            if ( *bp == '\\' ) {
                                switch ( *++bp ) {
                                case 'd':
                                    map[j].trt[t] = 0;
                                    break;
                                case 't':
                                    map[j].trt[t] = 9;
                                    break;
                                case 'n':
                                    map[j].trt[t] = 10;
                                    break;
                                case 'v':
                                    map[j].trt[t] = 11;
                                    break;
                                case 'f':
                                    map[j].trt[t] = 12;
                                    break;
                                case 'r':
                                    map[j].trt[t] = 13;
                                    break;
                                case 'e':
                                    map[j].trt[t] = 27;
                                    break;
                                default:
                                    fprintf(stderr, "odb [Oload(%d)] - TRANSLIT error: invalid escape \\%c\n", 
                                        __LINE__, *bp);
                                    goto oload_exit;
                                }
                            } else {
                                map[j].trt[t] = *bp;
                            }
                        }
                        if ( z != t ) {
                            fprintf(stderr, "odb [Oload(%d)] - TRANSLIT error: number of \'translit-from\' (%d) and \'translit-to\' (%d) characters differ\n", 
                                __LINE__, z, t);
                            goto oload_exit;
                        } else {
                            map[j].min = z;
                        }
                    } else if ( !strmicmp ( "comp3", bp, 5 ) ) {
                        map[j].op = 12;
                        while ( *bp && *bp++ != ':' );
                        map[j].prec = (int) mchar ( bp );   /* read precision */
                        while ( *bp && *bp++ != ':' );
                        map[j].scale = (int) mchar ( bp );  /* read scale */
                    } else if ( !strmicmp ( "comp", bp, 4 ) ) {
                        map[j].op = 11;
                    } else if ( !strmicmp ( "zoned", bp, 5 ) ) {
                        map[j].op = 13;
                        while ( *bp && *bp++ != ':' );
                        map[j].prec = (int) mchar ( bp );   /* read precision */
                        while ( *bp && *bp++ != ':' );
                        map[j].scale = (int) mchar ( bp );  /* read scale */
                    } else if ( !strmicmp ( "emptyasconst", bp, 10 ) ) {
                        while ( *bp && *bp++ != ':' );
                        map[j].cl = strlen ( bp );
                        if ((map[j].c = (char *)malloc ( map[j].cl + 1 )) == (void *)NULL) {
                            fprintf(stderr, "odb [Oload(%d)] - EMPTYASCONST error allocating memory for %s\n", 
                                __LINE__, (char *)etab[eid].td[j].Oname);
                            goto oload_exit;
                        } else {
                            strmcpy(map[j].c, bp, map[j].cl);
                        }
                        map[j].op = 14;
                    } else if ( !strmicmp ( "emptyasempty", bp, 11 ) ) {
                        map[j].op = 15;
                    } else if (!strmicmp("div", bp, 3)) {
                        map[j].op = 16;
                        while (*bp && *bp++ != ':');
                        map[j].scale = strtol(bp, NULL, 10);
                        if (map[j].scale == 0) {
                            fprintf(stderr, "odb [Oload(%d)] - DIV error for %s\n", __LINE__, (char *)etab[eid].td[j].Oname);
                            goto oload_exit;
                        }
                    } else if (!strmicmp("trim", bp, 4)) {
                        map[j].op = 17;
                    }
                    else {
                        map[j].op = 0;
                    }
                }
            }
        }
        fclose(fm);
        /* Determine max mapped field number */
        for ( i = 0, mff = 0 ; i < l ; i++ ) {
            if ( map[i].idx > 0 && (unsigned int)map[i].idx > mff ) 
                mff = map[i].idx;
        }
        mff++;
        /* Adjust map[].idx for fixed formats */
        if ( fg & 0004 ) {
            for ( i = 0 ; i < mff ; i++ ) {
                unsigned int rnk = 0;
                for ( j = 0 ; j < mff ; j++ ) {
                    if ( map[j].min < map[i].min ) {
                        rnk++;
                    }
                }
                if ( map[i].idx > 0 )
                    map[i].idx = rnk;
            } 
        }
    } else {
        for ( i = 0 ; i < l ; i++ )
            etab[eid].td[i].dl = 1;
        nmf = mff = l;
    }
    if ( ( fg & 0010 ) && ( fg & 0004 ) ) {
        fprintf(stderr, "odb [Oload(%d)] - You cannot mix fixed/delimited fields \n", __LINE__);
        goto oload_exit;
    }

    /* Allocate & Initialize Reverse Map */
    if ( mff ) {
        if ( ( rmap = (int *)calloc ( mff , sizeof(int) ) ) == (void *)NULL ) {
            fprintf(stderr, "odb [Oload(%d)] - Error allocating memory for rmap\n", __LINE__);
            goto oload_exit;
        }
        if ( fm ) {
            for ( i = 0 ; i < mff ; i++ ) /* Initialize to ignore */
                rmap[i] = -1;
            for ( i = 0 ; i < l ; i++ ) 
                if ( etab[eid].td[i].dl && map[i].idx >= 0 )
                    rmap[map[i].idx] = i;
        } else {
            for ( i = 0 ; i < mff ; i++ )
                rmap[i] = i;
        }
    }

    /* Truncate target table */
    if ( etab[eid].flg & 0002 ) {   /* truncate target table */
        if ( f & 020000 )   /* if verbose */
            fprintf(stderr, "odb: Now truncating target table (%s)... ", (char *)Odel);
        etab[eid].flg2 |= 020000000 ;           /* Oexec to allocate/use its own handle */
        z = Oexec(tid, eid, 0, 0, Odel, "");    /* Run Truncate */
        etab[eid].flg2 &= ~020000000 ;          /* reset Oexec to allocate/use its own handle */
        if ( j && etab[eid].flg & 0004 ) {
            fprintf(stderr, "odb [Oload(%d)] - Error truncating Target table. Exiting because Stop On Error was set\n", 
                __LINE__);
            goto oload_exit;
        }
        if ( etab[eid].cmt ) {  /* Autocommit off: have to commit truncate */
            if (!SQL_SUCCEEDED(Or=SQLEndTran(SQL_HANDLE_DBC, thps[tid].Oc, SQL_COMMIT))) {
                Oerr(eid, tid, __LINE__, thps[tid].Oc, SQL_HANDLE_DBC);
                goto oload_exit;
            }
        }
        etab[eid].nr = 0;   /* reset number of resulting rows */
        if ( f & 020000 )   /* if verbose */
            fprintf(stderr, "done\n");
    }

    /* Continue INSERT statement building and calculate buffer size */
    for ( i = z = 0, etab[eid].s = 0 ; i < l ; i++ ) {
        if ( !etab[eid].td[i].dl )  /* skip this column */
            continue;
        if ( z )                    /* not the first column */
            cl += strmcat((char *)Oins, "\",\"", cl, 0);
        else
            z = 1;
        cl += strmcat((char *)Oins, (char *)etab[eid].td[i].Oname, cl, 0);
        etab[eid].td[i].start = etab[eid].s;
        #ifdef __hpux
            if ( etab[eid].td[i].Osize % WORDSZ )
                etab[eid].td[i].pad = WORDSZ - etab[eid].td[i].Osize % WORDSZ ;
        #endif
        etab[eid].s += ( etab[eid].td[i].Osize + etab[eid].td[i].pad + sizeof(SQLLEN) );    /* space for length indicator */
        if ( cmdl + CMD_CHUNK < cl ) {  /* increase Oins buffer */
            cl += CMD_CHUNK;
            O = Oins;
            if ( ( O = realloc ( O, cl ) ) == (void *)NULL ) {
                fprintf(stderr, "odb [Oload(%d)] - Error re-allocating memory for Ocmd: [%d] %s\n",
                    __LINE__, errno, strerror(errno));
                goto oload_exit;
            }
            Oins = O;
        }
    }

    /* Calculate rowset if buffer size is set */
    if ( etab[eid].rbs ) {
        etab[eid].r = etab[eid].rbs / etab[eid].s;
        if ( etab[eid].mr && etab[eid].r > etab[eid].mr )   /* if # records to fetch < rowset ... */
            etab[eid].r = etab[eid].mr;                     /* make rowset = records to fetch */
        etab[eid].r = etab[eid].r < 1 ? 1 : etab[eid].r;    /* at least one record at a time  */
    }
    if ( etab[eid].flg2 & 0001 )    /* commit as multiplier */
        etab[eid].cmt *= (int)etab[eid].r ;

    /* Determine max target column length */
    for ( i = 0 ; i < l ; i++ )
        if ( (size_t)etab[eid].td[i].Osize > mfl )
            mfl = (size_t) etab[eid].td[i].Osize;

    /* Allocate field buffer */
    if ( (str = (char *)calloc (1, etab[eid].buffsz + 1)) == (void *)NULL ) {
        fprintf(stderr, "odb [Oload(%d)] - Error allocating field buffer: [%d] %s\n",
            __LINE__, errno, strerror(errno));
        goto oload_exit;
    }

    /* Complete INSERT statement */
    cl += strmcat ( (char *) Oins, "\") VALUES(", cl, 0);
    for ( i = 0; i < nmf ; i++ ) {
        if ( i )
            cl += strmcat ( (char *) Oins, ",", cl, 0);
        cl += strmcat ( (char *) Oins, "?", cl, 0);
        if ( cmdl + CMD_CHUNK < cl ) {  /* increase Oins buffer */
            cl += CMD_CHUNK;
            O = Oins ;
            if ( ( O = realloc ( O, cl ) ) == (void *)NULL ) {
                fprintf(stderr, "odb [Oload(%d)] - Error re-allocating memory for Ocmd: [%d] %s\n",
                    __LINE__, errno, strerror(errno));
                goto oload_exit;
            }
            Oins = O ;
        }
    }
    (void) strmcat ( (char *) Oins, ")", cl, 0);
    if ( f & 020000 )   /* if verbose */
        fprintf(stderr, "odb [Oload(%d)] - INSERT statement: %s\n", __LINE__, (char *)Oins);

    /* Allocate loader eids array */
    nldr = etab[eid].ps ? etab[eid].ps : 1;
    if ( ( ldrs = (int *)calloc ( nldr, sizeof ( int ) ) ) == (void *)NULL ) {
        fprintf(stderr, "odb [Oload(%d)] - Error allocating loader eids array: [%d] %s\n",
            __LINE__, errno, strerror(errno));
        goto oload_exit;
    }

    /* Initialize loader eids array */
    if ( etab[eid].ps ) {
        for ( i = 0, j = 0 ; i < (unsigned int)no && j < etab[eid].ps ; i++ ){
            if ( etab[i].type == 'L' && etab[i].parent == eid ){
                etab[i].r = etab[eid].r;    /* adjust rowset when rbs is specified */
                etab[i].cmt = etab[eid].cmt;/* adjust rowset when cmt is a multiplier */
                etab[i].lstat = 0;          /* reset buffer status to available */
                etab[i].td = etab[eid].td;  /* copy table structure pointer */
                ldrs[j++] = i;
            }
        }
    } else {
        ldrs[0] = eid;
    }

    /* Allocate rowset & status memory */
    if ( (etab[eid].Orowsetl = (SQLCHAR *)calloc (etab[eid].r, etab[eid].s)) == (void *)NULL ||
         (etab[eid].Ostatusl = (SQLUSMALLINT *)calloc (etab[eid].r, sizeof(SQLUSMALLINT))) == (void *)NULL ) {
            fprintf(stderr, "odb [Oload(%d)] - Error allocating rowset memory: [%d] %s\n",
                __LINE__, errno, strerror(errno));
        goto oload_exit;
    }
    if ( etab[eid].ps ) {
        for ( j = 0 ; j < nldr ; j++ ) {    /* for all loading threads... */
            if ( (etab[ldrs[j]].Orowsetl = (SQLCHAR *)calloc (etab[eid].r, etab[eid].s)) == (void *)NULL ||
                (etab[ldrs[j]].Ostatusl = (SQLUSMALLINT *)calloc (etab[eid].r, sizeof(SQLUSMALLINT))) == (void *)NULL ) {
                    fprintf(stderr, "odb [Oload(%d)] - Error allocating rowset memory: [%d] %s\n",
                        __LINE__, errno, strerror(errno));
                goto oload_exit;
            }
        }
    }

    /* Set rowset size, bind parameters and prepare INSERT */
    for ( j = 0 ; j < nldr ; j++ ) {
        /* Set max commit mode */
        if ( etab[eid].cmt ) {
            if (!SQL_SUCCEEDED(Or=SQLSetConnectAttr(thps[etab[ldrs[j]].id].Oc,
                    SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_OFF, SQL_IS_UINTEGER))) {
                Oerr(eid, tid, __LINE__, thps[etab[ldrs[j]].id].Oc, SQL_HANDLE_DBC);
                goto oload_exit;
            }
        } else {
            if (!SQL_SUCCEEDED(Or=SQLSetConnectAttr(thps[etab[ldrs[j]].id].Oc,
                    SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_ON, SQL_IS_UINTEGER))) {
                Oerr(eid, tid, __LINE__, thps[etab[ldrs[j]].id].Oc, SQL_HANDLE_DBC);
                goto oload_exit;
            }
        }
        /* Set max char/varchar/binary column length */
        if (!SQL_SUCCEEDED(Oret=SQLSetStmtAttr(thps[etab[ldrs[j]].id].Os, SQL_ATTR_MAX_LENGTH,
            (SQLPOINTER)etab[eid].Omaxl, SQL_IS_UINTEGER))) {
            Oerr(eid, tid, __LINE__, thps[etab[ldrs[j]].id].Os, SQL_HANDLE_STMT);
        }
        /* Bind parameters */
        if (!SQL_SUCCEEDED(Or=SQLSetStmtAttr(thps[etab[ldrs[j]].id].Os, SQL_ATTR_PARAM_BIND_TYPE,
                (SQLPOINTER)(etab[eid].s), 0))) {
            Oerr(eid, tid, __LINE__, thps[etab[ldrs[j]].id].Os, SQL_HANDLE_STMT);
            goto oload_exit;
        }
        if (!SQL_SUCCEEDED(Or=SQLSetStmtAttr(thps[etab[ldrs[j]].id].Os, SQL_ATTR_PARAMSET_SIZE,
                (SQLPOINTER)(etab[eid].r), 0))) {
            Oerr(eid, tid, __LINE__, thps[etab[ldrs[j]].id].Os, SQL_HANDLE_STMT);
            goto oload_exit;
        }
        if (!SQL_SUCCEEDED(Or=SQLSetStmtAttr(thps[etab[ldrs[j]].id].Os, SQL_ATTR_PARAM_STATUS_PTR,
                etab[ldrs[j]].Ostatusl, 0))) {
            Oerr(eid, tid, __LINE__, thps[etab[ldrs[j]].id].Os, SQL_HANDLE_STMT);
            goto oload_exit;
        }
        if (!SQL_SUCCEEDED(Or=SQLSetStmtAttr(thps[etab[ldrs[j]].id].Os, SQL_ATTR_PARAMS_PROCESSED_PTR, 
                &etab[ldrs[j]].Oresl, 0))) {
            Oerr(eid, tid, __LINE__, thps[etab[ldrs[j]].id].Os, SQL_HANDLE_STMT);
            goto oload_exit;
        }
        for ( i = 0, z = 1; i < l; i++ ) {
            if ( etab[eid].td[i].dl ) {
                if (!SQL_SUCCEEDED(Or=SQLBindParameter(thps[etab[ldrs[j]].id].Os, (SQLUSMALLINT)z++,
                        SQL_PARAM_INPUT, SQL_C_CHAR, etab[eid].td[i].Otype, (SQLULEN)etab[eid].td[i].Osize,
                        etab[eid].td[i].Odec, &etab[ldrs[j]].Orowsetl[0+etab[eid].td[i].start], etab[eid].td[i].Osize,
                        (SQLLEN *)&etab[ldrs[j]].Orowsetl[0+etab[eid].td[i].start+etab[eid].td[i].Osize+etab[eid].td[i].pad]))) {
                    Oerr(eid, tid, __LINE__, thps[etab[ldrs[j]].id].Os, SQL_HANDLE_STMT);
                    goto oload_exit;
                }
            }
        }
        if (!SQL_SUCCEEDED(Or=SQLPrepare(thps[etab[ldrs[j]].id].Os, Oins, SQL_NTS))) {
            Oerr(eid, tid, __LINE__, thps[etab[ldrs[j]].id].Os, SQL_HANDLE_STMT);
            goto oload_exit;
        }
        etab[ldrs[j]].sp = l;   /* save number of target table fields for prec */
    }

    /* Register Load Start Time */
    gettimeofday(&tve, (void *)NULL);
    tinit = 1000*(tve.tv_sec-tvi.tv_sec)+(tve.tv_usec-tvi.tv_usec)/1000;

    /* Set initial fixed fields start/end */
    if ( rmap[k] >= 0 ) {       /* rmap[k]=-1 for ignored fields */
        ffstart = map[rmap[k]].min;
        ffend = map[rmap[k]].max;
    }

    /* Determine if the input file is gzipped by checking the first 2 chars */
    if ( fl ) {
        if ( ( fl != stdin ) && ( len = fread ( buff, 1, 2, fl) ) == 2 ) {
            if ( (unsigned char)buff[0] == 037 &&
                 (unsigned char)buff[1] == 0213 ) { /* gzipped file */
                 isgz = 1 ;
            }
            (void) fseek(fl, 0L, SEEK_SET);     /* rewind */
        }
#ifdef HDFS
    } else if ( fhl ) {
        if ( ( len = (size_t)(*hdfsread)(hfs, fhl, (void *)buff, 2) ) == 2 ) {
            if ( (unsigned char)buff[0] == 037 &&
                 (unsigned char)buff[1] == 0213 ) { /* gzipped file */
                 isgz = 1 ;
            }
            (void)(*hdfsseek)(hfs, fhl, 0);     /* rewind */
        }
#endif
    }

    /* Allocate buffer for gzip read and initialize inflate */
    if ( isgz ) {
        /* Allocate gzip io buffer */
        if ( ( gzbuff = malloc((size_t)etab[eid].buffsz) ) == (void *)NULL ) {
            fprintf(stderr, "odb [Oload(%d)] - Error allocating gzip IO buffer: [%d] %s\n",
                __LINE__, errno, strerror(errno));
            goto oload_exit;
        }
        memset ( gzbuff, 0, (size_t)etab[eid].buffsz);
        /* Initalize inflate */
        if ( ( gzret = inflateInit2 (&gzstream, windowBits | ENABLE_ZLIB_GZIP) ) != Z_OK ) {
            fprintf(stderr, "odb [Oload(%d)] - Error initializing zlib: [%d]\n",
                __LINE__, gzret);
            goto oload_exit;
        }
    }

    /* Reading input file */
    pstats = 1;                         /* From now on print stats on exit */
    Odp = &etab[eid].Orowsetl[m*etab[eid].s];
    while ( go ) {
        if ( isgz ) {       /* is a gzipped file */
            if ( gzstream.avail_in ) {          /* continue inflating previous gzbuff into buff */
                gzstream.avail_out = (unsigned int) etab[eid].buffsz ;
                gzstream.next_out = (unsigned char *)buff ;
                gzret = inflate (&gzstream, Z_NO_FLUSH) ;
                switch ( gzret ) {
                case Z_OK:
                    break ;     /* everything is ok - continue */
                case Z_STREAM_END:
                    inflateReset ( &gzstream ) ;
                    break;
                default:
                    fprintf(stderr, "odb [Oload(%d)] - Error during deflate: [%d]\n",
                        __LINE__, gzret);
                    goto oload_exit ;
                    break;
                }
                len = etab[eid].buffsz - gzstream.avail_out ;
            } else {
                if ( fl ) {                     /* read new data from normal file-system into gzbuff */
                    len = fread ( gzbuff, 1, (size_t)etab[eid].buffsz, fl);
#ifdef HDFS
                } else if ( fhl ) {             /* read new data from HDFS into gzbuff */
                    len = (size_t)(*hdfsread)(hfs, fhl, (void *)buff, etab[eid].buffsz);
#endif
                }
                gzstream.avail_in = (unsigned int)len ;
                gzstream.next_in = (unsigned char *)gzbuff ;
                if ( len )
                    continue ;
            }
        } else if ( fl ) {
            len = fread ( buff, 1, (size_t)etab[eid].buffsz, fl);
#ifdef HDFS
        }  else if ( fhl ) {
            len = (size_t)(*hdfsread)(hfs, fhl, (void *)buff, etab[eid].buffsz);
#endif
        }
        if ( len == 0 ) {                                       /* EOF */
            if ( ( k + 1 ) == mff && !(fg & 0004) ) {           /* complete last row & insert */
                ch = -1;                                        /* insert this block */
                goto oload_lastrow;
            } else if ( m ) {                                   /* rows to be inserted */ 
                goto oload_insert;
            } else {                                            /* exit loop */
                break;
            }
        }
        nb += len;                                              /* update bytes read from file */
        p = 0;                                                  /* reset buffer index */
        if (!etab[eid].mcrs) {
            while (lts && p < len) {                                /* skip initial lines */
                if (buff[p++] == lrs) {
                    --lts;
                }
            }
        }

        if (!etab[eid].mcrs) {
            if ( ccl ) {                                            /* continue cleaning rest of line */
                while ( p < len && buff[p] != lrs )                 /* ... skip the rest of the line */
                    p++;
                if (buff[p] == lrs) { /* if a record separator has been found */
                    ccl = 0;            /* switch the continue cleaning flag off */
                    p++;                /* skip the record separator */
                }
            }
        }

        for ( ; p < len ; p++ ) {
            ch = buff[p];
            if ( fg & 0004 ) {                                  /* fixed file format */
                if ( c < ffstart ) {                            /* before field start */
                    c++;                                        /* do nothing (skip it) */
                } else if ( ch == lrs ) {                       /* if record sep... */
                    if ( c )
                        fg |= 0040;                             /* set record complete flag on */
                } else {                                        /* add new character to field buffer */
                    str[ifl++] = ch;
                    c++;
                    if ( ifl == (int)ffend ) {                      /* fixed file format: end of field */
                        while ( ifl && str[ifl-1] == etab[eid].pc ) /* "remove" trailing pad chars */
                            ifl--;
                        fg |= 0020;
                    }
                }
            } else {                                            /* delimited file format */
                if ( ch == lec && !(fg & 0100)) {               /* if non-escaped escape char... */
                    fg |= 0100;                                 /* set Escape flag on */
                } else if (!etab[eid].mcfs && (ch == lfs) && !(fg & 0001)) {        /* if field sep... */
                    fg |= 0020;                                 /* set field complete flag on */
                } else if (!etab[eid].mcrs && (ch == lrs) && !(fg & 0001)) {        /* if record sep... */
                    fg |= 0040;                                 /* set record complete flag on */
                } else if ( ch == lsq && !(fg & 0100)) {        /* if string qualifier char... */
                    fg ^= 0001;                                 /* flip quoted string flag */
                } else if ( ch == lem && !(fg & 0100)) {        /* if embedded file char */
                    fg |= 0200;                                 /* embed file reading mode */
                } else {
                    str[ifl++] = ch;                            /* add new character to field buffer */
                    fg &= ~0100;                                /* set escape flag off */

                    if (etab[eid].mcfs && (ifl >= mcfsl)
                        && !strncmp(etab[eid].mcfs, &str[ifl-mcfsl], mcfsl)) { /* if field sep... */
                        fg |= 0020;
                        ifl -= mcfsl;
                    } else if (etab[eid].mcrs && (ifl >= mcrsl)
                               && !strncmp(etab[eid].mcrs, &str[ifl-mcrsl], mcrsl)) { /* if reco sep... */
                        fg |= 0040;
                        ifl -= mcrsl;
                    }
                }
            }
            if ( fg & 0062 ) {                                  /* field/record ready or nofile */
                oload_lastrow:
                str[ifl] = '\0';
                if ( rmap && rmap[k] >= 0 ) {
                    Odp = &etab[eid].Orowsetl[m*etab[eid].s + etab[eid].td[rmap[k]].start];
                    if ( fg & 0200 ) {                          /* embed file reading mode */
                        fg &= ~0200 ;                           /* reset embed file flag */
                        str[ifl] = '\0';                        /* close filename string */
                        if ( ( fe = fopen(str, "rb") ) == (FILE *) NULL ) {
                            fprintf(stderr, "odb [Oload(%d)] - Line skipped. Error opening %s: [%d] %s\n",
                                __LINE__, str, errno, strerror(errno) );
                            mi = 0;
                        } else {
                            (void) fseek(fe, 0L, SEEK_END);     /* goto EOF */
                            fsize = ftell ( fe );               /* get file size */
                            (void) fseek(fe, 0L, SEEK_SET);     /* rewind */
                            if ( fsize > (long)etab[eid].td[rmap[k]].Osize ) {  /* prevent Orowsetl[] overflow */
                                fprintf(stderr, "odb [Oload(%d)] - Line skipped. %s size (%ld) exceed target field length (%lu)\n",
                                    __LINE__, str, fsize, (unsigned long)etab[eid].td[rmap[k]].Osize );
                                mi = 0;
                            } else if ( ( ifl = (int)fread ( Odp, 1, (size_t)fsize, fe) ) != (int)fsize ) {
                                fprintf(stderr, "odb [Oload(%d)] - Line skipped. Error reading %s: [%d] %s\n",
                                    __LINE__, str, errno, strerror(errno) );
                                mi = 0;
                            }
                            fclose ( fe ) ;
                        }
                    } else if ( etab[eid].ns && !strncmp(etab[eid].ns, str, (size_t)ifl) ) {
                        ifl = SQL_NULL_DATA;
                    }
                    else if ( (ifl == 0) && (map[rmap[k]].op != 14) && (map[rmap[k]].op != 15) ) {
                        if (!etab[eid].ns) {
                            ifl = SQL_NULL_DATA;
                        }
                    } else {
                        switch ( map[rmap[k]].op ) {    /* manipulate str if needed */
                        case 1:         /* substr */
                            ifl = map[rmap[k]].max - map[rmap[k]].min;
                            memmove((void *)str, (void *) (str + map[rmap[k]].min), (size_t)ifl);
                            break;
                        case 2:         /* date conversion */
                            if ( ( ifl = dconv ( map[rmap[k]].c, str, str, bs, 0) ) == 0 )
                                fprintf ( stderr, "odb [Oload(%d)] - Error converting date row %u col %u\n"
                                                  "Input string: >%s<\nFormat string: >%s<\n",
                                                  __LINE__, n, k, str, map[rmap[k]].c );
                            else if ( ifl == 1 )
                                fprintf ( stderr, "odb [Oload(%d)] - Error converting date row %u col %u"
                                                  " (Bad Format String)\nInput string: >%s<\nFormat string: >%s<\n",
                                                  __LINE__, n, k, str, map[rmap[k]].c );
                            break;
                        case 3:         /* time conversion */
                            if ( ( ifl = dconv ( map[rmap[k]].c, str, str, bs, 1) ) == 0 ) 
                                fprintf ( stderr, "odb [Oload(%d)] - Error converting time row %u col %u\n"
                                                  "Input string: >%s<\nFormat string: >%s<\n",
                                                  __LINE__, n, k, str, map[rmap[k]].c );
                            else if ( ifl == 1 )
                                fprintf ( stderr, "odb [Oload(%d)] - Error converting time row %u col %u"
                                                  " (Bad Format String)\nInput string: >%s<\nFormat string: >%s<\n",
                                                  __LINE__, n, k, str, map[rmap[k]].c );
                            break;
                        case 4:         /* timestamp conversion */
                            if ( ( ifl = dconv ( map[rmap[k]].c, str, str, bs, 2) ) == 0 )
                                fprintf ( stderr, "odb [Oload(%d)] - Error converting timestamp row %u col %u\n"
                                                  "Input string: >%s<\nFormat string: >%s<\n",
                                                  __LINE__, n, k, str, map[rmap[k]].c );
                            else if ( ifl == 1 )
                                fprintf ( stderr, "odb [Oload(%d)] - Error converting timestamp row %u col %u"
                                                  " (Bad Format String)\nInput string: >%s<\nFormat string: >%s<\n",
                                                  __LINE__, n, k, str, map[rmap[k]].c );
                            break;
                        case 5:         /* replace */
                            if ( !strncmp(map[rmap[k]].c, str, (size_t)ifl) ){
                                ifl = map[rmap[k]].max;
                                MEMCPY(str, ( map[rmap[k]].c + map[rmap[k]].cl ) , map[rmap[k]].max);
                            }
                            break;
                        case 6:         /* toupper */
                            str = strup(str);
                            break;
                        case 7:         /* tolower */
                            str = strlo(str);
                            break;
                        case 8:         /* firstup */
                            str = strlo(str);
                            *str = (unsigned char) toupper ( (int)*str );
                            break;
                        case 9:         /* csubstr */
                            for ( z = 0 ; z < (int)ifl && str[z] != map[rmap[k]].max ; z++ );
                            if ( map[rmap[k]].min == map[rmap[k]].max && z < (int)ifl )
                                for ( ++z ; z < (int)ifl && str[z] != map[rmap[k]].max ; z++ );
                            for ( t = z - 1 ; t >= 0 && str[t] != map[rmap[k]].min ; t-- );
                            if ( ( ifl = (z - t - 1 ) ) <= 0 )
                                ifl = 0;
                            else if ( t >= 0 )
                                memmove((void *)str, (void *) (str + t + 1), (size_t)ifl);
                            break;
                        case 10:        /* translit */
                            for ( i = 0, z = 0 ; i < (unsigned int)ifl ; i++ ) {
                                for ( t = 0 ; t <= map[rmap[k]].min ; t++ ) {
                                    if ( str[i] == map[rmap[k]].trf[t] ) {
                                        if ( map[rmap[k]].trt[t] )
                                            str[z++] = map[rmap[k]].trt[t] ;
                                        t = -1;
                                        break ;
                                    }
                                }
                                if ( t >=0 )
                                    str[z++] = str[i];
                            }
                            ifl = (unsigned int)z;
                            break ;
                        case 11:    /* comp */
                            z = comp2asc(str, num, ifl, (unsigned int) etab[eid].td[rmap[k]].Osize);
                            if ( z < 0 ) {
                                fprintf ( stderr, "odb [Oload(%d)] - COMP field conversion error: row %d col %d. "
                                                "This row won't be loaded\n", __LINE__, n+1, k+1);
                                mi = 0; /* SKIP THIS ROW */
                            } else {
                                strmcpy(str, num, z);
                                ifl = z;
                            }
                            break;
                        case 12:    /* comp3 */
                            z = comp32asc(str, num, ifl, (unsigned int) etab[eid].td[rmap[k]].Osize, map[rmap[k]].prec, map[rmap[k]].scale);
                            if ( z < 0 ) {
                                fprintf ( stderr, "odb [Oload(%d)] - COMP3 field conversion error: row %d col %d. "
                                                "This row won't be loaded\n", __LINE__, n+1, k+1);
                                mi = 0; /* SKIP THIS ROW */
                            } else {
                                strmcpy(str, num, z);
                                ifl = z;
                            }
                            break;
                        case 13:    /* zoned */
                            z = zoned2asc(str, num, ifl, (unsigned int) etab[eid].td[rmap[k]].Osize, map[rmap[k]].prec, map[rmap[k]].scale);
                            if ( z < 0 ) {
                                fprintf ( stderr, "odb [Oload(%d)] - ZONED field conversion error: row %d col %d. "
                                                "This row won't be loaded\n", __LINE__, n+1, k+1);
                                mi = 0; /* SKIP THIS ROW */
                            } else {
                                strmcpy(str, num, z);
                                ifl = z;
                            }
                            break;
                        case 14:    /* emptyasconst */
                            if ( ifl == 0 ) {
                                ifl = (int)map[rmap[k]].cl;
                                strmcpy(str, map[rmap[k]].c, map[rmap[k]].cl);
                            }
                            break;
                        case 15:    /* emptyasempty */
                            if ( ifl == 0 )
                                ifl = EMPTY ;
                            break;
                        case 16:
                        {
                            if (ifl > 0) {
                                double dv = 0;

                                str[ifl] = '\0';
                                if (!(is_valid_numeric(str, strlen(str)) && sscanf(str, "%lf", &dv))) {
                                    fprintf(stderr, "odb [Oload(%d)] - DIV field conversion error: row %d col %d. "
                                        "%s is not valid numeric, This row won't be loaded\n", __LINE__, n + 1, k + 1, str);
                                    mi = 0; /* SKIP THIS ROW */
                                }
                                dv = dv / map[rmap[k]].scale;
                                ifl = sprintf(str, "%.*lf", etab[eid].td[rmap[k]].Odec, dv);
                            }
                        }
                            break;
                        case 17: // trim
                            str = strtrim(str);
                            ifl = strlen(str);
                            break;
                        }
                        if ( ifl > (int)etab[eid].td[rmap[k]].Osize ) { /* prevent Orowsetl[] overflow */
                            str[ifl]='\0';
                            switch ( etab[eid].fldtr ) {
                            case 0: /* truncate, warn, load if text */
                            case 1: /* truncate, load if text */
                                switch ( etab[eid].td[rmap[k]].Otype ) {
                                case SQL_CHAR:
                                case SQL_WCHAR:
                                case SQL_VARCHAR:
                                case SQL_WVARCHAR:
                                case SQL_LONGVARCHAR:
                                case SQL_WLONGVARCHAR:
                                    if ( !etab[eid].fldtr )
                                        fprintf ( stderr, "odb [Oload(%d)] - Warning: row %d col %d field truncation. Input "
                                            "string: >%s< of length %d. Only the first %lu char(s) will be loaded\n", __LINE__,
                                            n+1, k+1, str, ifl, (long unsigned)etab[eid].td[rmap[k]].Osize );
                                    break;
                                default:
                                    if ( !etab[eid].fldtr )
                                        fprintf ( stderr, "odb [Oload(%d)] - Error: row %d col %d field truncation. Input "
                                            "string: >%s< of length %d exceeds %lu. This row won't be loaded\n", __LINE__,
                                            n+1, k+1, str, ifl, (long unsigned)etab[eid].td[rmap[k]].Osize );
                                    mi = 0; /* SKIP THIS ROW */
                                    break;
                                }
                                break;
                            case 2: /* warn, skip */
                                fprintf ( stderr, "odb [Oload(%d)] - Error: row %d col %d field truncation. Input "
                                    "string: >%s< of length %d. This row won't be loaded\n", __LINE__, n+1, k+1, str, ifl);
                                mi = 0;     /* SKIP THIS ROW */
                                break;
                            case 3: /* truncate, warn, load */
                                fprintf ( stderr, "odb [Oload(%d)] - Warning: row %d col %d field truncation. Input "
                                    "string: >%s< of length %d. First %lu char will be loaded\n", __LINE__,
                                    n+1, k+1, str, ifl, (long unsigned)etab[eid].td[rmap[k]].Osize );
                                break;
                            case 4: /* truncate, load */
                            case 5: /* do not truncate */
                                break;
                            }
                            if ( etab[eid].fldtr != 5 )
                                ifl = (int)etab[eid].td[rmap[k]].Osize;
                        }
                        if ( ifl > 0 ) {
                            MEMCPY(Odp, str, ifl);
                        } else if ( ifl == 0 ) {
                            if (!etab[eid].ns) {
                                ifl = SQL_NULL_DATA;
                            }
                        } else if ( ifl == EMPTY ) {
                            ifl = 0 ;
                        }
                        else {
                            fprintf(stderr, "odb [Oload(%d)] - Error: get unexpected string length(%d) while parsing row %d col %d field.\n", __LINE__,
                                ifl, n + 1, k + 1);
                        }
                    }
                    *((SQLLEN *)(Odp + etab[eid].td[rmap[k]].Osize + etab[eid].td[rmap[k]].pad)) = (SQLLEN)ifl ;
                }
                if ( ( ++k == mff ) || ( fg & 0002 ) ) {                /* we read all fields from file or nofile */
                    if ( fg & 0002 ) {                                  /* nofile */
                        c = 1;                                          /* reset current column to 1 to return here */
                        p = 0;
                    } else {                                            /* real file */
                        c = 0;                                          /* reset column number */
                        if (!etab[eid].mcrs) {
                            while ( p < len && buff[p] != lrs )         /* ... skip the rest of the line */
                                p++;
                            if ( p == len && buff[p-1] != lrs )         /* Continue cleaning rest of line in the next buffsz */
                                ccl = 1;
                        }
                    }
                    for ( j = 0 ; j < l ; j++ ) {                       /* now generate "artificial" fields for this record */
                        Odp = &etab[eid].Orowsetl[m*etab[eid].s + etab[eid].td[j].start];
                        switch ( map[j].idx ) {
                        case (-1):  /* const */
                            MEMCPY(Odp, map[j].c, map[j].cl);
                            *((SQLLEN *)(Odp + etab[eid].td[j].Osize + etab[eid].td[j].pad)) = map[j].cl;
                            break;
                        case (-2):  /* seq */
                            map[j].cl = snprintf((char *)Odp, etab[eid].td[j].Osize, "%d", map[j].min++);
                            *((SQLLEN *)(Odp + etab[eid].td[j].Osize + etab[eid].td[j].pad)) = map[j].cl;
                            break;
                        case (-3):  /* irand */
                            map[j].cl = snprintf((char *)Odp, etab[eid].td[j].Osize, "%d",
                                (int)RAND(map[j].min, map[j].max));
                            *((SQLLEN *)(Odp + etab[eid].td[j].Osize + etab[eid].td[j].pad)) = map[j].cl;
                            break;
                        case (-4):  /* drand */
                            rnd = (unsigned int)RAND(map[j].min, map[j].max);   /* year */
                            i = (unsigned int)RAND(1, 12);                      /* month */
                            switch ( i ) {  /* set date range based on month number */
                            case 1:     /* January */
                            case 3:     /* March */
                            case 5:     /* May */
                            case 7:     /* July */
                            case 8:     /* August */
                            case 10:    /* October */
                            case 12:    /* December */
                                o = 31;
                                break;
                            case 4:     /* April */
                            case 6:     /* June */
                            case 9:     /* September */
                            case 11:    /* November */
                                o = 30;
                                break;
                            case 2:     /* February */
                                if ( ( rnd % 400 ) == 0 ) {
                                    o = 29;
                                } else if ( ( rnd % 100 ) == 0 ) {
                                    o = 28;
                                } else if ( ( rnd % 4 ) == 0 ) {
                                    o = 29;
                                } else {
                                    o = 28;
                                }
                                break;
                            }
                            map[j].cl = snprintf((char *)Odp, etab[eid].td[j].Osize + 1,
                                "%04u-%02u-%02d", rnd, i, (int)RAND(1, o));
                            *((SQLLEN *)(Odp + etab[eid].td[j].Osize + etab[eid].td[j].pad)) = 10;
                            break;
                        case (-5):  /* tmrand */
                            rnd = (unsigned int)RAND(0, 85399);
                            map[j].cl = snprintf((char *)Odp, etab[eid].td[j].Osize + 1,
                                "%02d:%02d:%02d", rnd/3600, ( rnd % 3600 ) / 60, rnd % 60);
                            *((SQLLEN *)(Odp + etab[eid].td[j].Osize + etab[eid].td[j].pad)) = map[j].cl;
                            break;
                        case (-6):  /* tsrand */
                            trnd = (time_t)RAND(0, map[j].max);
                            dt = localtime(&trnd);
                            map[j].cl = snprintf((char *)Odp, etab[eid].td[j].Osize + 1, "%04d-%02d-%02d %02d:%02d:%02d",
                                dt->tm_year+1900, dt->tm_mon+1, dt->tm_mday, dt->tm_hour, dt->tm_min, dt->tm_sec);
                            *((SQLLEN *)(Odp + etab[eid].td[j].Osize + etab[eid].td[j].pad)) = map[j].cl;
                            break;
                        case (-7):  /* crand */
                            for ( i = 0; i < map[j].cl ; i++ ) {
                                rnd = rand() % ( sizeof(alnum) - 1 ) ;
                                *(Odp + i) = (SQLCHAR)alnum[rnd] ;
                            }
                            *((SQLLEN *)(Odp + etab[eid].td[j].Osize + etab[eid].td[j].pad)) = map[j].cl;
                            break;
                        case (-8):  /* emrand */
                            for ( i = 0 ; i < (unsigned int)RAND(map[j].min, map[j].max) ; i++) {
                                rnd = rand() % ( sizeof(alnum) - 1 ) ;
                                *(Odp+i) = (SQLCHAR)alnum[rnd] ;
                            }
                            *(Odp + i++ ) = '@';
                            for ( o = 0; o < (unsigned int)RAND(map[j].prec, map[j].scale) ; o++, i++) {
                                rnd = rand() % ( sizeof(alnum) - 1 ) ;
                                *(Odp+i) = (SQLCHAR)alnum[rnd] ;
                            }
                            *(Odp + i++ ) = '.';
                            rnd = (unsigned int)RAND(0, (int)(map[j].cl - 1));
                            for ( o = 0; o < map[j].eln[rnd] ; o++, i++)
                                *(Odp + i) = map[j].el[rnd][o];
                            *((SQLLEN *)(Odp + etab[eid].td[j].Osize + etab[eid].td[j].pad)) = i;
                            break;
                        case (-9):  /* null */
                            *((SQLLEN *)(Odp + etab[eid].td[j].Osize + etab[eid].td[j].pad)) = SQL_NULL_DATA;
                            break;
                        case (-11): /* cdate */
                            trnd = time(NULL);
                            dt = localtime(&trnd);
                            map[j].cl = snprintf((char *)Odp, etab[eid].td[j].Osize + 1,
                                "%04d-%02d-%02d", dt->tm_year+1900, dt->tm_mon+1, dt->tm_mday);
                            *((SQLLEN *)(Odp + etab[eid].td[j].Osize + etab[eid].td[j].pad)) = map[j].cl;
                            break;
                        case (-12): /* ctime */
                            trnd = time(NULL);
                            dt = localtime(&trnd);
                            map[j].cl = snprintf((char *)Odp, etab[eid].td[j].Osize + 1,
                                "%02d:%02d:%02d", dt->tm_hour, dt->tm_min, dt->tm_sec);
                            *((SQLLEN *)(Odp + etab[eid].td[j].Osize + etab[eid].td[j].pad)) = map[j].cl;
                            break;
                        case (-13): /* ctstamp */
                            trnd = time(NULL);
                            dt = localtime(&trnd);
                            map[j].cl = snprintf((char *)Odp, etab[eid].td[j].Osize + 1, "%04d-%02d-%02d %02d:%02d:%02d",
                                dt->tm_year+1900, dt->tm_mon+1, dt->tm_mday, dt->tm_hour, dt->tm_min, dt->tm_sec);
                            *((SQLLEN *)(Odp + etab[eid].td[j].Osize + etab[eid].td[j].pad)) = map[j].cl;
                            break;
                        case (-14): /* dsrand */
                        case (-17): /* lstrand */
                            i = (unsigned int)RAND(0, (int)(map[j].cl - 1));
                            MEMCPY(Odp, map[j].el[i], map[j].eln[i]);
                            *((SQLLEN *)(Odp + etab[eid].td[j].Osize + etab[eid].td[j].pad)) = map[j].eln[i];
                            break;
                        case (-15): /* txtrand */
                            i = (int)RAND(map[j].min, map[j].max);
                            MEMCPY(Odp, (map[j].c+(int)RAND(0, (int)map[j].cl-i)), i);
                            *((SQLLEN *)(Odp + etab[eid].td[j].Osize + etab[eid].td[j].pad)) = i;
                            break;
                        case (-16): /* nrand */
                            for ( i = 0; i < (unsigned int)map[j].max ; i++ ) 
                                *(Odp + i) = (SQLCHAR)RAND(48, 57) ;
                            *(Odp + i++) = (SQLCHAR)'.' ;
                            for ( ; i < (unsigned int)map[j].cl ; i++ ) 
                                *(Odp + i) = (SQLCHAR)RAND(48, 57) ;
                            *((SQLLEN *)(Odp + etab[eid].td[j].Osize + etab[eid].td[j].pad)) = map[j].cl;
                            break;
                        }
                    }
                    n++;
                    m += mi;
                    mi = 1;
                    nrf++;
                    k = 0;
                    if ( m == etab[eid].r || ( etab[eid].mr && nrf >= etab[eid].mr ) || ch == ( -1 ) ) {    /* Insert the rowset */
                    oload_insert:
                        nt++;
                        while ( go ) {
                            if ( etab[eid].ps ) {                           /* Find a buffer loader thread available */
                                MutexLock(&etab[eid].pmutex);
                                while ( go ) {
                                    for ( i = 0 ; i < nldr ; i++ ) {        /* Look for write_available buffers */
                                        if ( etab[ldrs[i]].lstat == 0 ) {
                                            etab[ldrs[i]].lstat = 2;        /* write_busy */
                                            break;
                                        }
                                    }
                                    if ( etab[eid].flg & 04000 ) {          /* Oloadbuff set error flag */
                                        break;
                                    } else if ( i >= nldr ) {                           /* nothing available... wait */
                                        nw++;
                                        CondWait(&etab[eid].pcvp, &etab[eid].pmutex);
                                    } else {
                                        break ;
                                    }
                                }
                                MutexUnlock(&etab[eid].pmutex);
                                if ( etab[eid].flg & 04000 )                /* Oloadbuff set error flag */
                                    break;
                                memcpy(etab[ldrs[i]].Orowsetl, etab[eid].Orowsetl, etab[eid].r * etab[eid].s);
                                etab[ldrs[i]].nbs = nrf - (unsigned long)m ;/* pass the input file base row for this rowset */
                                etab[ldrs[i]].lstat = 1;                    /* read_available */
                                etab[ldrs[i]].ar = m;
                                CondWakeAll(&etab[eid].pcvc);
                            } else if ( etab[eid].flg2 & 0020 ) {           /* "show" mode */
                                pstats = 0;
                                printf("Record number %lu:\n", nrf);
                                printf("%4s %-30s %-20s %9s %3s %4s %7s %9s\n",
                                    "COL#", "COLUMN_NAME", "ODBC_DATA_TYPE", "DISP_SZ", "DEC", "NULL", "FLD_LEN", "[FLD_VALUE]");
                                for ( i = 0 ; i < l ; i++ ) {
                                    if ( !etab[eid].td[i].dl )
                                        ifl = (-2);     /* column value not loaded */
                                    else
                                        ifl = (int) *((SQLLEN *)(etab[eid].Orowsetl + etab[eid].td[i].start + etab[eid].td[i].Osize + etab[eid].td[i].pad)); 
                                    printf("%4d %-30s %-20s %9u %3d %4s", i,
                                        (char *) etab[eid].td[i].Oname,
                                        expandtype(etab[eid].td[i].Otype),
                                        (unsigned int)etab[eid].td[i].Osize,
                                        (int) etab[eid].td[i].Odec,
                                        etab[eid].td[i].Onull == SQL_NULLABLE ? "YES" : "NO");
                                    switch ( ifl ) {
                                    case -2:            /* not loaded */
                                        printf("       ? [(not loaded)]\n");
                                        break;
                                    case -1:            /* SQL_NULL_DATA */
                                        printf("       - [(null)]\n");
                                        break;
                                    case 0:
                                        printf("       0 [(empty)]\n");
                                        break;
                                    default:
                                        printf(" %7d [", ifl);
                                        for ( sp = (char *)(etab[eid].Orowsetl + etab[eid].td[i].start) ; ifl ; sp++, ifl-- )
                                            fputc( *sp , stdout );
                                        fputc( ']' , stdout );
                                        fputc( '\n' , stdout );
                                    }
                                }
                            } else {
                                etab[ldrs[0]].nbs = nrf - (unsigned long)m ;    /* pass the input file base row for this rowset */
                                etab[ldrs[0]].ar = m;
                                Oloadbuff(eid);
                            }
                            break;
                        }
                        if ( etab[eid].flg & 04000 )                        /* Oloadbuff set error flag */
                            goto oload_exit;
                        m = 0;
                    }
                    if ( ( etab[eid].mr && nrf >= etab[eid].mr ) || len == 0 )
                        goto oload_exit;
                }
                ifl = 0;
                fg &= ~0060;                                                /* reset field/record ready */
                if ( rmap[k] >= 0 ) {                                       /* rmap[k]=-1 for ignored fields */
                    ffstart = map[rmap[k]].min;                             /* set fixed field start */
                    ffend = map[rmap[k]].max;                               /* set fixed field end */
                }
            }
        }
        if ( fg & 0002 )
            break;
    }
    oload_exit:
    etab[eid].flg |= 02000;                 /* mark EOF */
    if ( etab[eid].ps ) {   /* wait all loaders to complete */
        CondWakeAll(&etab[eid].pcvc);
        MutexLock(&etab[eid].pmutex);
        while ( go ) {
            for ( i = 0, k=0 ; i < nldr ; i++ ) {
                k += etab[ldrs[i]].lstat;
            }
            if ( k != nldr * 10 )
                CondWait(&etab[eid].pcvp, &etab[eid].pmutex);
            else 
                break;
        }
        MutexUnlock(&etab[eid].pmutex);
    }
    telaps -= 1000*tve.tv_sec + tve.tv_usec/1000;
    gettimeofday(&tve, (void *)NULL);       /* register end time */
    telaps += 1000*tve.tv_sec + tve.tv_usec/1000;
    
    for ( i = 0 ; i < nldr ; i++ )
        etab[eid].nrt += etab[ldrs[i]].nr;

    /* Print results */
    if ( pstats ) {
        fprintf(stderr, "[%d] %s Load statistics:\n\t[%d] Target table: %s.%s.%s\n",
            tid, odbid, tid, etab[eid].Ocso[0], etab[eid].Ocso[1], etab[eid].Ocso[2]);
        if ( etab[eid].flg2 & 01000000000 ) {
            fprintf(stderr, "\t[%d] Source: [MAPR] %s\n", tid, etab[eid].src);
        } else if ( etab[eid].flg2 & 0100 ) {
            fprintf(stderr, "\t[%d] Source: [HDFS] %s\n", tid, etab[eid].src);
        } else {
            fprintf(stderr, "\t[%d] Source: %s\n", tid, etab[eid].src);
        }
        seconds = (double)tinit/1000.0 ;
        fprintf(stderr, "\t[%d] Pre-loading time: %s s (%s)\n", tid,
            strmnum(seconds, num, sizeof(num), 3), strmtime(seconds, tim));
        seconds = (double)telaps/1000.0 ;
        fprintf(stderr, "\t[%d] Loading time: %s s(%s)\n", tid,
            strmnum(seconds, num, sizeof(num), 3), strmtime(seconds, tim));
        fprintf(stderr, "\t[%d] Total records read: %s\n", tid, strmnum((double)nrf,num,sizeof(num),0));
        fprintf(stderr, "\t[%d] Total records inserted: %s\n", tid, strmnum((double)etab[eid].nrt,num,sizeof(num),0));
        fprintf(stderr, "\t[%d] Total number of columns: %s\n", tid, strmnum((double)l,num,sizeof(num),0));
        fprintf(stderr, "\t[%d] Total bytes read: %s\n", tid, strmnum((double)nb,num,sizeof(num),0));
        fprintf(stderr, "\t[%d] Average input row size: %s B\n", tid, strmnum((double)nb*1.0/n,num,sizeof(num),1));
        fprintf(stderr, "\t[%d] ODBC row size: %s B (data)", tid, strmnum((double)(etab[eid].s-l*sizeof(SQLLEN)),num,sizeof(num),0));
        fprintf(stderr, " + %s B (len ind)\n", strmnum((double)(l*sizeof(SQLLEN)), num,sizeof(num),0));
        fprintf(stderr, "\t[%d] Rowset size: %s\n", tid, strmnum((double)etab[eid].r,num,sizeof(num),0));
        fprintf(stderr, "\t[%d] Rowset buffer size: %s KiB\n", tid, strmnum((double)etab[eid].s/1024.0*etab[eid].r,num,sizeof(num),2));
        fprintf(stderr, "\t[%d] Load throughput (real data): %s KiB/s\n", tid, strmnum((double)nb/1.024/telaps,num,sizeof(num),3));
        fprintf(stderr, "\t[%d] Load throughput (ODBC): %s KiB/s\n", tid,
            strmnum((double)((etab[eid].s-l*sizeof(SQLLEN))*etab[eid].nrt)/1.024/telaps,num,sizeof(num),3));
        if ( etab[eid].ps ){
            fprintf(stderr, "\t[%d] Reader Total/Wait Cycles: %s", tid, strmnum((double)nt,num,sizeof(num),0));
            fprintf(stderr, "/%s\n", strmnum((double)nw,num,sizeof(num),0));
        }
    }

    /* Unbind parameters (if parallel unbind is executed in Oloadbuff) */
    if ( !etab[eid].ps )
        (void)SQLFreeStmt(thps[tid].Os, SQL_RESET_PARAMS);

    /* Close input file */
    if ( isgz ) 
        inflateEnd ( &gzstream ) ;
    if (fl && fl != stdin)
        fclose ( fl );
#ifdef HDFS
    else if ( fhl )
        (*hdfsclose)(hfs, fhl);
#endif

    /* Free Memory */
    if ( Odel ) 
        free ( Odel );
    if ( Oins )
        free ( Oins );
    if ( buff )
        free ( buff );
    if ( gzbuff )
        free ( gzbuff );
    if ( str )
        free ( str );
    if ( ldrs )     /* Free ldrs */
        free(ldrs); 
    if ( etab[eid].Orowsetl )       /* Free Orowsetl if allocated */
        free(etab[eid].Orowsetl);   
    if ( etab[eid].Ostatusl )       /* Free Ostatusl if allocated */
        free(etab[eid].Ostatusl);       
    if ( etab[eid].td ) {       
        for ( i = 0; i < l ; i++)
            free ( etab[eid].td[i].Oname );
        free(etab[eid].td);
    }
    if ( map ) {
        for ( i = 0 ; i < l ; i++ ) {
            if ( map[i].c )
                free ( map[i].c );
            if ( map[i].el )
                free ( map[i].el );
            if ( map[i].eln )
                free ( map[i].eln );
        }
        free ( map );
    }
    if ( rmap )
        free (rmap );

    /* Close bad file */
    if ( etab[eid].fo != stderr )
        fclose ( etab[eid].fo );

    /* Run "post" SQL */
    if ( etab[eid].post ) {
        var_set ( &thps[tid].tva, VTYPE_I, "tgt", etab[eid].tgt );
        etab[eid].flg2 |= 020000000 ;           /* Oexec to allocate/use its own handle */
        if ( etab[eid].post[0] == '@' )         /* run a sql script */
            (void)runsql(tid, eid, 0, (etab[eid].post + 1 ));
        else                                    /* Run single SQL command */
            (void)Oexec(tid, eid, 0, 0, (SQLCHAR *)etab[eid].post, "");
        etab[eid].flg2 &= ~020000000 ;          /* reset Oexec to allocate/use its own handle */
    }
    return;
}

/* Oload2:
 *      load CSV files using parameters in etab[eid] (no map, no escapes, no string qualifiers
 *
 *      eid (I): etab entry ID to run
 *
 *      return: void
 */
static void Oload2(int eid)
{
    int tid = etab[eid].id;     /* Thread ID */
    SQLCHAR *Oins = 0;          /* INSERT Statement */
    SQLCHAR *Odel = 0;          /* DELETE Statement */
    SQLCHAR *O = 0;             /* ODBC temp variable for memory realloc */
    SQLRETURN Or=0;             /* ODBC return value */
    unsigned long nw=0,         /* no of wait cycles */
                  nt=0,         /* No of total write cycles */
                  nrf=0;        /* no of read records */
    long telaps = 0,            /* Elapsed time in ms */
         tinit = 0;             /* Initialization time in ms */
    size_t nb = 0,              /* number of bytes read from input file */
           cl = CMD_CHUNK,      /* INSERT command buffer length */
           cmdl = 0,            /* INSERT command length */
           dell = CMD_CHUNK,    /* DELETE command length */
           ifl = 0,             /* input field length */
           rl = 0,              /* residual length */
           len = 0;             /* used to scroll the IO buffer */
    int gzret = 0,              /* zlib function return values */
        z = 0;                  /* loop variable */
    unsigned int nldr=0,        /* number of loaders */
                 k=0,           /* input file field number */
                 m = 0,         /* rowset array record number */
                 i=0,           /* loop variable */
                 pstats = 0,    /* Error flag: 0 = print stats, 1 = don't print stats */
                 l=0,           /* destination table fields */
                 j=0,           /* loop variable */
                 lts = etab[eid].k ,    /* lines to skip */
                 isgz=0;        /* input file is gzipped: 0=no , 1=yes */
    int *ldrs=0;                /* pointer to array containing loaders EIDs */
    FILE *fl=0;                 /* data to load file pointer */
#ifdef HDFS
    hdfsFile fhl = 0;           /* Hadoop Input File Handle */
#endif
    char *buff = 0,             /* IO buffer */
         *buff_save = 0,        /* save original IO buffer pointer */
         *gzbuff = 0,           /* GZIP IO buffer */
         *str = 0,              /* field buffer */
         *bc = 0;               /* current field buffer size pointer */
    char num[32];               /* Formatted Number String */
    char tim[15];               /* Formatted Time String */
    struct timeval tve;         /* timeval struct to define elapesd/timelines */
    SQLCHAR *Odp = 0;           /* rowset buffer data pointer */
    double seconds = 0;         /* seconds used for timings */
    z_stream gzstream = { 0 } ; /* zlib structure for gziped files */
    unsigned char fg = 0,       /* Oload flags:
                                0001 = in a quoted string   0020 = field ready
                                0010 = delimited fields     0040 = record ready
                                0100 = escape flag */
        lfs = etab[eid].fs,                                 /* local field separator */
        lrs = etab[eid].rs,                                 /* local record separator */
        lsq = etab[eid].sq ? etab[eid].sq : '"';            /* local string qualifier, deafult is '"' */
                                
    /* Check if we have to use another ODBC connection */
    if ( thps[tid].cr > 0 ) {
        thps[tid].Oc = thps[thps[tid].cr].Oc;
        thps[tid].Os = thps[thps[tid].cr].Os;
    }

    /* Set "tgt" variable */
    var_set ( &thps[tid].tva, VTYPE_I, "tgt", etab[eid].tgt );

    /* Run "pre" SQL */
    if ( etab[eid].pre ) {
        etab[eid].flg2 |= 020000000 ;           /* Oexec to allocate/use its own handle */
        if ( etab[eid].pre[0] == '@' )          /* run a sql script */
            z = runsql(tid, eid, 0, (etab[eid].pre + 1 ));
        else                                    /* Run single SQL command */
            z = Oexec(tid, eid, 0, 0, (SQLCHAR *)etab[eid].pre, "");
        etab[eid].flg2 &= ~020000000 ;          /* reset Oexec to allocate/use its own handle */
        if ( z && etab[eid].flg & 0004 )
            goto oload2_exit;
        etab[eid].nr = 0;   /* reset number of resulting rows */
        (void)SQLFreeHandle(SQL_HANDLE_STMT, thps[tid].Os);
        if ( !SQL_SUCCEEDED(Or=SQLAllocHandle(SQL_HANDLE_STMT, thps[tid].Oc, &thps[tid].Os))){
            Oerr(eid, tid, __LINE__, Oc, SQL_HANDLE_DBC);
            goto oload2_exit;
        }
    }

    /* Check database type */
    etab[eid].dbt = checkdb( eid, &thps[tid].Oc, NULL, NULL);

    /* Check if truncate */
    if ( etab[eid].flg & 0002 ) {           
        etab[eid].flg &= ~0200000;      /* if truncate... unset ifempty */
        if ( ( Odel = malloc(dell) ) == (void *)NULL ) {
            fprintf(stderr, "odb [Oload2(%d)] - Error allocating Odel memory: [%d] %s\n",
                __LINE__, errno, strerror(errno));
            goto oload2_exit;
        }
        strmcpy((char *)Odel, dbscmds[etab[eid].dbt].trunc, dell);
        Odel = (SQLCHAR *)var_exp((char *)Odel, &dell, &thps[tid].tva);
    }
        
    /* check ifempty */
    if ( etab[eid].flg & 0200000 ) {    /* ifempty is set */
        if ( ifempty( eid, etab[eid].tgt ) ) {
            fprintf(stderr, "odb [Oload2(%d)] - Target table %s is not empty\n",
                    __LINE__, etab[eid].tgt);
            etab[eid].post = 0; /* prevent post SQL execution */
            goto oload2_exit;
        }
    }

    /* Allocate io buffer */
    if ( ( buff = malloc((size_t)etab[eid].buffsz) ) == (void *)NULL ) {
        fprintf(stderr, "odb [Oload2(%d)] - Error allocating IO buffer: [%d] %s\n",
            __LINE__, errno, strerror(errno));
        goto oload2_exit;
    }
    buff_save = buff ;
    memset ( buff, 0, (size_t)etab[eid].buffsz);

    /* Open input file */
    if ( !strcmp ( etab[eid].src , "stdin" ) ) {
        fl = stdin ;
    } else {
        for ( i = j = 0; i < etab[eid].buffsz && etab[eid].src[i]; i++ ) {
            switch ( etab[eid].src[i] ) {
            case '%':
                switch ( etab[eid].src[++i] ) {
                case 't':
                    j += strmcat (buff, (char *)etab[eid].Ocso[2], (size_t)etab[eid].buffsz, 2 ); 
                    break;
                case 'T':
                    j += strmcat (buff, (char *)etab[eid].Ocso[2], (size_t)etab[eid].buffsz, 1 ); 
                    break;
                case 's':
                    j += strmcat (buff, (char *)etab[eid].Ocso[1], (size_t)etab[eid].buffsz, 2 ); 
                    break;
                case 'S':
                    j += strmcat (buff, (char *)etab[eid].Ocso[1], (size_t)etab[eid].buffsz, 1 ); 
                    break;
                case 'c':
                    j += strmcat (buff, (char *)etab[eid].Ocso[0], (size_t)etab[eid].buffsz, 2 ); 
                    break;
                case 'C':
                    j += strmcat (buff, (char *)etab[eid].Ocso[0], (size_t)etab[eid].buffsz, 1 ); 
                    break;
                default:
                    fprintf(stderr, "odb [Oload2(%d)] - Invalid expansion %%%c in %s\n",
                        __LINE__, etab[eid].src[i], etab[eid].src );
                    goto oload2_exit;
                }
                break;
            default:
                buff[j++] = etab[eid].src[i];
                break;
            }
        }
        buff[j] = '\0';
        if ( !(etab[eid].flg2 & 0100) && ( fl = fopen(buff, "r") ) == (FILE *) NULL ) {
            fprintf(stderr, "odb [Oload2(%d)] - Error opening input file %s: [%d] %s\n",
                __LINE__, buff, errno, strerror(errno) );
            goto oload2_exit;
        }
    }

#ifdef HDFS
    /* Open HDFS input file */
    if ( etab[eid].flg2 & 0100 ) {  /* HDFS input file */ 
        if ( ( fhl = (*hdfsopen)(hfs, buff, O_RDONLY, 0, 0, 0) ) == (hdfsFile) NULL ) {
            fprintf(stderr, "odb [Oload2(%d)] - Error opening hdfs file %s\n",
                __LINE__, buff);
            goto oload2_exit;
        }
    }
#endif

    /* Analyze target table */
    z = Otcol(eid, &thps[tid].Oc) ;
    if ( z <= 0 ) {
        fprintf(stderr, "odb [Oload2(%d)] - Error analyzing target table %s.%s.%s\n", 
            __LINE__, etab[eid].Ocso[0], etab[eid].Ocso[1], etab[eid].Ocso[2]);
        goto oload2_exit;
    } else {
        l = (unsigned int)z ;
    }

    /* Adjust Osize for WCHAR field (up to 4 bytes/char). Set buffer size */
    if ( etab[eid].dbt != VERTICA ) {       /* Vertica's CHAR field length is in bytes (not chars) */
        for ( j = 0 ; j < l ; j++ ) {
            switch ( etab[eid].td[j].Otype ) {
            case SQL_WCHAR:
            case SQL_WVARCHAR:
            case SQL_WLONGVARCHAR:
                etab[eid].td[j].Osize *= etab[eid].bpwc;    /* Space for UTF-8 conversion */
                break;
            case SQL_CHAR:
            case SQL_VARCHAR:
            case SQL_LONGVARCHAR:
                etab[eid].td[j].Osize *= etab[eid].bpc;/* Space for UTF-8 conversion */
                break;
            }
        }
    }

    /* Determine buffer start positions and size */
    for ( j = 0 ; j < l ; j++ ) {
        etab[eid].td[j].start = etab[eid].s;
        #ifdef __hpux
            if ( etab[eid].td[j].Osize % WORDSZ )
                etab[eid].td[j].pad = WORDSZ - etab[eid].td[j].Osize % WORDSZ ;
        #endif
        etab[eid].s += ( etab[eid].td[j].Osize + etab[eid].td[j].pad + sizeof(SQLLEN) );    /* space for length indicator */
    }

    /* Allocate field buffer */
    if ((str = calloc(1, etab[eid].buffsz + 1)) == (void *)NULL) {
        fprintf(stderr, "odb [Oload2(%d)] - Error allocating field buffer: [%d] %s\n",
            __LINE__, errno, strerror(errno));
        goto oload2_exit;
    }

    /* Truncate target table */
    if ( etab[eid].flg & 0002 ) {   /* truncate target table */
        if ( f & 020000 )   /* if verbose */
            fprintf(stderr, "odb: Now truncating target table (%s)... ", (char *)Odel);
        etab[eid].flg2 |= 020000000 ;           /* Oexec to allocate/use its own handle */
        z = Oexec(tid, eid, 0, 0, Odel, "");    /* Run Truncate */
        etab[eid].flg2 &= ~020000000 ;          /* reset Oexec to allocate/use its own handle */
        if ( j && etab[eid].flg & 0004 ) {
            fprintf(stderr, "odb [Oload2(%d)] - Error truncating Target table. Exiting because Stop On Error was set\n", 
                __LINE__);
            goto oload2_exit;
        }
        if ( etab[eid].cmt ) {  /* Autocommit off: have to commit truncate */
            if (!SQL_SUCCEEDED(Or=SQLEndTran(SQL_HANDLE_DBC, thps[tid].Oc, SQL_COMMIT))) {
                Oerr(eid, tid, __LINE__, thps[tid].Oc, SQL_HANDLE_DBC);
                goto oload2_exit;
            }
        }
        etab[eid].nr = 0;   /* reset number of resulting rows */
        if ( f & 020000 )   /* if verbose */
            fprintf(stderr, "done\n");
    }

    /* Calculate rowset if buffer size is set */
    if ( etab[eid].rbs ) {
        etab[eid].r = etab[eid].rbs / etab[eid].s;
        etab[eid].r = etab[eid].r < 1 ? 1 : etab[eid].r;    /* at least one record at a time  */
    }
    if ( etab[eid].flg2 & 0001 )    /* commit as multiplier */
        etab[eid].cmt *= (int)etab[eid].r ;

    /* Build INSERT statement */
    if ( ( Oins = malloc ( cl ) ) == (void *) NULL ) {
        fprintf(stderr, "odb [Oload2(%d)] - Error allocating Oins memory\n", __LINE__);
        goto oload2_exit;
    }

    if (!strcasecmp(etab[eid].loadcmd, "UL"))
    {
        cmdl += snprintf((char *)Oins, cl, "UPSERT USING LOAD %s%sINTO %s%c%s%c%s VALUES(",
            etab[eid].flg & 040000000 ? "/*+ DIRECT */ " : "",
            etab[eid].flg2 & 0002 ? "WITH NO ROLLBACK " : "",
            etab[eid].Ocso[0] ? (char *)etab[eid].Ocso[0] : " ",
            etab[eid].Ocso[0] ? '.' : ' ',
            etab[eid].Ocso[1] ? (char *)etab[eid].Ocso[1] : " ",
            etab[eid].Ocso[1] ? '.' : ' ', (char *)etab[eid].Ocso[2]);
    }
    else if (!strcasecmp(etab[eid].loadcmd, "IN"))
    {
        cmdl += snprintf((char *)Oins, cl, "INSERT %s%sINTO %s%c%s%c%s VALUES(",
            etab[eid].flg & 040000000 ? "/*+ DIRECT */ " : "",
            etab[eid].flg2 & 0002 ? "WITH NO ROLLBACK " : "",
            etab[eid].Ocso[0] ? (char *)etab[eid].Ocso[0] : " ",
            etab[eid].Ocso[0] ? '.' : ' ',
            etab[eid].Ocso[1] ? (char *)etab[eid].Ocso[1] : " ",
            etab[eid].Ocso[1] ? '.' : ' ', (char *)etab[eid].Ocso[2]);
    }
    else if (!strcasecmp(etab[eid].loadcmd, "UP"))
    {
        cmdl += snprintf((char *)Oins, cl, "UPSERT %s%sINTO %s%c%s%c%s VALUES(",
            etab[eid].flg & 040000000 ? "/*+ DIRECT */ " : "",
            etab[eid].flg2 & 0002 ? "WITH NO ROLLBACK " : "",
            etab[eid].Ocso[0] ? (char *)etab[eid].Ocso[0] : " ",
            etab[eid].Ocso[0] ? '.' : ' ',
            etab[eid].Ocso[1] ? (char *)etab[eid].Ocso[1] : " ",
            etab[eid].Ocso[1] ? '.' : ' ', (char *)etab[eid].Ocso[2]);
    }

    for (i = 0; i < l; i++) {
        if ( i )
            cl += strmcat ( (char *) Oins, ",", cl, 0);
        cl += strmcat ( (char *) Oins, "?", cl, 0);
        if ( cmdl + CMD_CHUNK < cl ) {  /* increase Oins buffer */
            cl += CMD_CHUNK;
            O = Oins ;
            if ( ( O = realloc ( O, cl ) ) == (void *)NULL ) {
                fprintf(stderr, "odb [Oload2(%d)] - Error re-allocating memory for Ocmd: [%d] %s\n",
                    __LINE__, errno, strerror(errno));
                goto oload2_exit;
            }
            Oins = O ;
        }
    }
    (void) strmcat ( (char *) Oins, ")", cl, 0);
    if ( f & 020000 )   /* if verbose */
        fprintf(stderr, "odb [Oload2(%d)] - INSERT statement: %s\n", __LINE__, (char *)Oins);

    /* Allocate loader eids array */
    nldr = etab[eid].ps ? etab[eid].ps : 1;
    if ( ( ldrs = (int *)calloc ( nldr, sizeof ( int ) ) ) == (void *)NULL ) {
        fprintf(stderr, "odb [Oload2(%d)] - Error allocating loader eids array: [%d] %s\n",
            __LINE__, errno, strerror(errno));
        goto oload2_exit;
    }

    /* Initialize loader eids array */
    if ( etab[eid].ps ) {
        for ( i = 0, j = 0 ; i < (unsigned int)no && j < etab[eid].ps ; i++ ){
            if ( etab[i].type == 'L' && etab[i].parent == eid ){
                etab[i].r = etab[eid].r;    /* adjust rowset when rbs is specified */
                etab[i].cmt = etab[eid].cmt;/* adjust rowset when cmt is a multiplier */
                etab[i].lstat = 0;          /* reset buffer status to available */
                etab[i].td = etab[eid].td;  /* copy table structure pointer */
                ldrs[j++] = i;
            }
        }
    } else {
        ldrs[0] = eid;
    }

    /* Allocate rowset & status memory */
    if ( (Odp = etab[eid].Orowsetl = (SQLCHAR *)calloc (etab[eid].r, etab[eid].s)) == (void *)NULL ||
         (etab[eid].Ostatusl = (SQLUSMALLINT *)calloc (etab[eid].r, sizeof(SQLUSMALLINT))) == (void *)NULL ) {
            fprintf(stderr, "odb [Oload2(%d)] - Error allocating rowset memory: [%d] %s\n",
                __LINE__, errno, strerror(errno));
        goto oload2_exit;
    }
    if ( etab[eid].ps ) {
        for ( j = 0 ; j < nldr ; j++ ) {    /* for all loading threads... */
            if ( (etab[ldrs[j]].Orowsetl = (SQLCHAR *)calloc (etab[eid].r, etab[eid].s)) == (void *)NULL ||
                (etab[ldrs[j]].Ostatusl = (SQLUSMALLINT *)calloc (etab[eid].r, sizeof(SQLUSMALLINT))) == (void *)NULL ) {
                    fprintf(stderr, "odb [Oload2(%d)] - Error allocating rowset memory: [%d] %s\n",
                        __LINE__, errno, strerror(errno));
                goto oload2_exit;
            }
        }
    }

    /* Set rowset size, bind parameters and prepare INSERT */
    for ( j = 0 ; j < nldr ; j++ ) {
        /* Set max commit mode */
        if ( etab[eid].cmt ) {
            if (!SQL_SUCCEEDED(Or=SQLSetConnectAttr(thps[etab[ldrs[j]].id].Oc,
                    SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_OFF, SQL_IS_UINTEGER))) {
                Oerr(eid, tid, __LINE__, thps[etab[ldrs[j]].id].Oc, SQL_HANDLE_DBC);
                goto oload2_exit;
            }
        } else {
            if (!SQL_SUCCEEDED(Or=SQLSetConnectAttr(thps[etab[ldrs[j]].id].Oc,
                    SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_ON, SQL_IS_UINTEGER))) {
                Oerr(eid, tid, __LINE__, thps[etab[ldrs[j]].id].Oc, SQL_HANDLE_DBC);
                goto oload2_exit;
            }
        }
        /* Set max char/varchar/binary column length */
        if (!SQL_SUCCEEDED(Oret=SQLSetStmtAttr(thps[etab[ldrs[j]].id].Os, SQL_ATTR_MAX_LENGTH,
            (SQLPOINTER)etab[eid].Omaxl, SQL_IS_UINTEGER))) {
            Oerr(eid, tid, __LINE__, thps[etab[ldrs[j]].id].Os, SQL_HANDLE_STMT);
        }
        /* Bind parameters */
        if (!SQL_SUCCEEDED(Or=SQLSetStmtAttr(thps[etab[ldrs[j]].id].Os, SQL_ATTR_PARAM_BIND_TYPE,
                (SQLPOINTER)(etab[eid].s), 0))) {
            Oerr(eid, tid, __LINE__, thps[etab[ldrs[j]].id].Os, SQL_HANDLE_STMT);
            goto oload2_exit;
        }
        if (!SQL_SUCCEEDED(Or=SQLSetStmtAttr(thps[etab[ldrs[j]].id].Os, SQL_ATTR_PARAMSET_SIZE,
                (SQLPOINTER)(etab[eid].r), 0))) {
            Oerr(eid, tid, __LINE__, thps[etab[ldrs[j]].id].Os, SQL_HANDLE_STMT);
            goto oload2_exit;
        }
        if (!SQL_SUCCEEDED(Or=SQLSetStmtAttr(thps[etab[ldrs[j]].id].Os, SQL_ATTR_PARAM_STATUS_PTR,
                etab[ldrs[j]].Ostatusl, 0))) {
            Oerr(eid, tid, __LINE__, thps[etab[ldrs[j]].id].Os, SQL_HANDLE_STMT);
            goto oload2_exit;
        }
        if (!SQL_SUCCEEDED(Or=SQLSetStmtAttr(thps[etab[ldrs[j]].id].Os, SQL_ATTR_PARAMS_PROCESSED_PTR, 
                &etab[ldrs[j]].Oresl, 0))) {
            Oerr(eid, tid, __LINE__, thps[etab[ldrs[j]].id].Os, SQL_HANDLE_STMT);
            goto oload2_exit;
        }
        for ( i = 0, z = 1; i < l; i++ ) {
            if (!SQL_SUCCEEDED(Or=SQLBindParameter(thps[etab[ldrs[j]].id].Os, (SQLUSMALLINT)z++,
                    SQL_PARAM_INPUT, SQL_C_CHAR, etab[eid].td[i].Otype, (SQLULEN)etab[eid].td[i].Osize,
                    etab[eid].td[i].Odec, &etab[ldrs[j]].Orowsetl[0+etab[eid].td[i].start], etab[eid].td[i].Osize,
                    (SQLLEN *)&etab[ldrs[j]].Orowsetl[0+etab[eid].td[i].start+etab[eid].td[i].Osize+etab[eid].td[i].pad]))) {
                Oerr(eid, tid, __LINE__, thps[etab[ldrs[j]].id].Os, SQL_HANDLE_STMT);
                goto oload2_exit;
            }
        }
        if (!SQL_SUCCEEDED(Or=SQLPrepare(thps[etab[ldrs[j]].id].Os, Oins, SQL_NTS))) {
            Oerr(eid, tid, __LINE__, thps[etab[ldrs[j]].id].Os, SQL_HANDLE_STMT);
            goto oload2_exit;
        }
        etab[ldrs[j]].sp = l;   /* save number of target table fields for prec */
    }

    /* Register Load Start Time */
    gettimeofday(&tve, (void *)NULL);
    tinit = 1000*(tve.tv_sec-tvi.tv_sec)+(tve.tv_usec-tvi.tv_usec)/1000;

    /* Determine if the input file is gzipped by checking the first 2 chars */
    if ( fl ) {
        if ( ( fl != stdin ) && ( len = fread ( buff, 1, 2, fl) ) == 2 ) {
            if ( (unsigned char)buff[0] == 037 &&
                 (unsigned char)buff[1] == 0213 ) { /* gzipped file */
                 isgz = 1 ;
            }
            (void) fseek(fl, 0L, SEEK_SET);     /* rewind */
        }
#ifdef HDFS
    } else if ( fhl ) {
        if ( ( len = (size_t)(*hdfsread)(hfs, fhl, (void *)buff, 2) ) == 2 ) {
            if ( (unsigned char)buff[0] == 037 &&
                 (unsigned char)buff[1] == 0213 ) { /* gzipped file */
                 isgz = 1 ;
            }
            (void)(*hdfsseek)(hfs, fhl, 0);     /* rewind */
        }
#endif
    }

    /* Allocate buffer for gzip read and initialize inflate */
    if ( isgz ) {
        /* Allocate gzip io buffer */
        if ( ( gzbuff = malloc((size_t)etab[eid].buffsz) ) == (void *)NULL ) {
            fprintf(stderr, "odb [Oload2(%d)] - Error allocating gzip IO buffer: [%d] %s\n",
                __LINE__, errno, strerror(errno));
            goto oload2_exit;
        }
        memset ( gzbuff, 0, (size_t)etab[eid].buffsz);
        /* Initalize inflate */
        if ( ( gzret = inflateInit2 (&gzstream, windowBits | ENABLE_ZLIB_GZIP) ) != Z_OK ) {
            fprintf(stderr, "odb [Oload2(%d)] - Error initializing zlib: [%d]\n",
                __LINE__, gzret);
            goto oload2_exit;
        }
    }

    /* Read input file */
    pstats = 1 ;    /* From now on print stats on exit */
    while ( go ) {
        if ( isgz ) {       /* is a gzipped file */
            if ( gzstream.avail_in ) {          /* continue inflating previous gzbuff into buff */
                gzstream.avail_out = (unsigned int)etab[eid].buffsz ;
                gzstream.next_out = (unsigned char *)buff ;
                gzret = inflate (&gzstream, Z_NO_FLUSH) ;
                switch ( gzret ) {
                case Z_OK:
                    break ;     /* everything is ok - continue */
                case Z_STREAM_END:
                    inflateReset ( &gzstream ) ;
                    break;
                default:
                    fprintf(stderr, "odb [Oload2(%d)] - Error during deflate: [%d]\n",
                        __LINE__, gzret);
                    goto oload2_exit ;
                    break;
                }
                len = etab[eid].buffsz - gzstream.avail_out ;
            } else {
                if ( fl ) {                     /* read new data from normal file-system into gzbuff */
                    len = fread ( gzbuff, 1, (size_t)etab[eid].buffsz, fl);
#ifdef HDFS
                } else if ( fhl ) {             /* read new data from HDFS into gzbuff */
                    len = (size_t)(*hdfsread)(hfs, fhl, (void *)gzbuff, etab[eid].buffsz);
#endif
                }
                gzstream.avail_in = (unsigned int) len ;
                gzstream.next_in = (unsigned char *)gzbuff ;
                if ( len )
                    continue ;
            }
        } else if ( fl ) {
            len = fread ( buff, 1, (size_t)etab[eid].buffsz, fl);
#ifdef HDFS
        }  else if ( fhl ) {
            len = (size_t)(*hdfsread)(hfs, fhl, (void *)buff, etab[eid].buffsz);
#endif
        }
        if ( len == 0 ) {                                       /* EOF */
            if ( m )    {                                       /* rows to be inserted */ 
                if ( k == ( l - 1 ) ) {                         /* flush last row not terminated by RS */
                    Odp += etab[eid].td[k].Osize + etab[eid].td[k].pad - rl ;
                    *((SQLLEN *)(Odp)) = (SQLLEN)rl ;
                    m++ ;
                    nrf++ ;
                }
                goto oload2_insert;
            } else {                                            /* exit loop */
                break;
            }
        }
        nb += len;                                              /* update bytes read from file */
        while ( lts && len ) {                                         /* skip initial lines */
            if ( *buff++ == lrs ) {
                lts-- ;
            }
            len-- ;
        }
        bc = buff ;

        for (size_t currentFieldPos = 0; currentFieldPos < len;) {
            ifl = 0;
            for (; currentFieldPos < len; ++currentFieldPos) {
                if ((fg & 0001) && (fg & 0100) && (bc[currentFieldPos] == lfs || bc[currentFieldPos] == lrs)) { /* treat string qualifier before as end quote */
                    fg &= ~0101;
                }
                if ((bc[currentFieldPos] == lfs) && !(fg & 0001)) {             /* if field sep... */
                    fg |= 0020;                                 /* set field complete flag on */
                    ++currentFieldPos;
                    break;
                }
                else if ((bc[currentFieldPos] == lrs) && !(fg & 0001)) {        /* if record sep... */
                    fg |= 0040;                                 /* set record complete flag on */
                    ++currentFieldPos;
                    break;
                }
                else if ((bc[currentFieldPos] == lsq) && (fg & 0001) && !(fg & 0100)) {  /* if string qualifier char and in string qualifier */
                    fg |= 0100;
                }
                else if ((bc[currentFieldPos] == lsq) && !(fg & 0100)) {
                    fg ^= 0001;                                 /* flip quoted string flag */
                }
                else {                                          /* add new character to field buffer */
                    str[ifl++] = bc[currentFieldPos];
                    fg &= ~0100;                                /* set escape flag off */
                }
            }

            if (rl + ifl > (size_t)(etab[eid].td[k].Osize)) { /* prevent Orowsetl[] overflow */
                char *tmpbuf = (char*)malloc(rl + ifl + 1);
                strncpy(tmpbuf, (const char*)(Odp - rl), rl);
                strncpy(tmpbuf + rl, str, ifl);
                tmpbuf[rl + ifl] = '\0';
                fprintf(stderr, "odb [Oload2(%d)] - Error: row %lu col %u field truncation. Input "
                    "string: >%s< of length %lu.\n", __LINE__, nrf + 1, k + 1, tmpbuf, ifl);
                free(tmpbuf);
                goto oload2_exit;
            }

            if ( fg & 0060 ) {                                  /* field complete */
                fg &= 0;                                        /* reset flags */
                if ( ifl ) {
                    MEMCPY(Odp, str, ifl);
                    Odp += etab[eid].td[k].Osize + etab[eid].td[k].pad - rl ;
                    *((SQLLEN *)(Odp)) = (SQLLEN)(ifl+rl) ;
                } else if ( rl ) {
                    Odp += etab[eid].td[k].Osize + etab[eid].td[k].pad - rl ;
                    *((SQLLEN *)(Odp)) = (SQLLEN)rl ;
                } else {
                    Odp += etab[eid].td[k].Osize + etab[eid].td[k].pad - rl ;
                    *((SQLLEN *)(Odp)) = (SQLLEN)SQL_NULL_DATA ;
                }
                rl = 0 ;
                Odp += sizeof(SQLLEN) ;
                if ( ++k == l ) {                               /* row completed */
                    k = 0 ;
                    m++ ;
                    nrf++ ;
                    Odp = &etab[eid].Orowsetl[m*etab[eid].s];
                }
            } else {                                            /* field incomplete */
                rl += ifl;                                      /* = change to +=, for = may cause potential bug */
                if ( ifl )
                    MEMCPY(Odp, str, ifl);
                memset(str, '\0', ifl);
                Odp += ifl ;
                break;
            }
            if ( m == etab[eid].r ) {                           /* Insert rowset */
                oload2_insert:
                nt++;
                while ( go ) {
                    if ( etab[eid].ps ) {                       /* Find a buffer loader thread available */
                        MutexLock(&etab[eid].pmutex);
                        while ( go ) {
                            for ( i = 0 ; i < nldr ; i++ ) {    /* Look for write_available buffers */
                                if ( etab[ldrs[i]].lstat == 0 ) {
                                    etab[ldrs[i]].lstat = 2;    /* write_busy */
                                    break;
                                }
                            }
                            if ( etab[eid].flg & 04000 ) {      /* Oloadbuff set error flag */
                                break;
                            } else if ( i >= nldr ) {           /* nothing available... wait */
                                nw++;
                                CondWait(&etab[eid].pcvp, &etab[eid].pmutex);
                            } else {
                                break ;
                            }
                        }
                        MutexUnlock(&etab[eid].pmutex);
                        if ( etab[eid].flg & 04000 )            /* Oloadbuff set error flag */
                            break;
                        memcpy(etab[ldrs[i]].Orowsetl, etab[eid].Orowsetl, m * etab[eid].s);
                        etab[ldrs[i]].nbs = nrf - (unsigned long)m ;    /* pass the input file base row for this rowset */
                        etab[ldrs[i]].lstat = 1;                /* read_available */
                        etab[ldrs[i]].ar = m;
                        CondWakeAll(&etab[eid].pcvc);
                    } else {
                        etab[ldrs[0]].nbs = nrf - (unsigned long)m ;    /* pass the input file base row for this rowset */
                        etab[ldrs[0]].ar = m;
                        Oloadbuff(eid);
                    }
                    break;
                }
                if ( etab[eid].flg & 04000 )                    /* Oloadbuff set error flag */
                    goto oload2_exit;
                m = 0;
                k = 0;
                Odp = etab[eid].Orowsetl ;
            }
        }
    }
    oload2_exit:
    etab[eid].flg |= 02000;                 /* mark EOF */
    if ( etab[eid].ps ) {                   /* wait all loaders to complete */
        CondWakeAll(&etab[eid].pcvc);
        MutexLock(&etab[eid].pmutex);
        while ( go ) {
            for ( i = 0, k=0 ; i < nldr ; i++ ) {
                k += etab[ldrs[i]].lstat;
            }
            if ( k != nldr * 10 )
                CondWait(&etab[eid].pcvp, &etab[eid].pmutex);
            else 
                break;
        }
        MutexUnlock(&etab[eid].pmutex);
    }
    telaps -= 1000*tve.tv_sec + tve.tv_usec/1000;
    gettimeofday(&tve, (void *)NULL);       /* register end time */
    telaps += 1000*tve.tv_sec + tve.tv_usec/1000;
    
    for ( i = 0 ; i < nldr ; i++ )
        etab[eid].nrt += etab[ldrs[i]].nr;

    /* Print results */
    if ( pstats ) {
        fprintf(stderr, "[%d] %s Load(2) statistics:\n\t[%d] Target table: %s.%s.%s\n",
            tid, odbid, tid, etab[eid].Ocso[0], etab[eid].Ocso[1], etab[eid].Ocso[2]);
        if ( etab[eid].flg2 & 01000000000 ) {
            fprintf(stderr, "\t[%d] Source: [MAPR] %s\n", tid, etab[eid].src);
        } else if ( etab[eid].flg2 & 0100 ) {
            fprintf(stderr, "\t[%d] Source: [HDFS] %s\n", tid, etab[eid].src);
        } else {
            fprintf(stderr, "\t[%d] Source: %s\n", tid, etab[eid].src);
        }
        seconds = (double)tinit/1000.0 ;
        fprintf(stderr, "\t[%d] Pre-loading time: %s s (%s)\n", tid,
            strmnum(seconds, num, sizeof(num), 3), strmtime(seconds, tim));
        seconds = (double)telaps/1000.0 ;
        fprintf(stderr, "\t[%d] Loading time: %s s(%s)\n", tid,
            strmnum(seconds, num, sizeof(num), 3), strmtime(seconds, tim));
        fprintf(stderr, "\t[%d] Total records read: %s\n", tid, strmnum((double)nrf,num,sizeof(num),0));
        fprintf(stderr, "\t[%d] Total records inserted: %s\n", tid, strmnum((double)etab[eid].nrt,num,sizeof(num),0));
        fprintf(stderr, "\t[%d] Total number of columns: %s\n", tid, strmnum((double)l,num,sizeof(num),0));
        fprintf(stderr, "\t[%d] Total bytes read: %s\n", tid, strmnum((double)nb,num,sizeof(num),0));
        fprintf(stderr, "\t[%d] Average input row size: %s B\n", tid, strmnum((double)nb*1.0/nrf,num,sizeof(num),1));
        fprintf(stderr, "\t[%d] ODBC row size: %s B (data)", tid, strmnum((double)(etab[eid].s-l*sizeof(SQLLEN)),num,sizeof(num),0));
        fprintf(stderr, " + %s B (len ind)\n", strmnum((double)(l*sizeof(SQLLEN)), num,sizeof(num),0));
        fprintf(stderr, "\t[%d] Rowset size: %s\n", tid, strmnum((double)etab[eid].r,num,sizeof(num),0));
        fprintf(stderr, "\t[%d] Rowset buffer size: %s KiB\n", tid, strmnum((double)etab[eid].s/1024.0*etab[eid].r,num,sizeof(num),2));
        fprintf(stderr, "\t[%d] Load throughput (real data): %s KiB/s\n", tid, strmnum((double)nb/1.024/telaps,num,sizeof(num),3));
        fprintf(stderr, "\t[%d] Load throughput (ODBC): %s KiB/s\n", tid,
            strmnum((double)((etab[eid].s-l*sizeof(SQLLEN))*etab[eid].nrt)/1.024/telaps,num,sizeof(num),3));
        if ( etab[eid].ps ){
            fprintf(stderr, "\t[%d] Reader Total/Wait Cycles: %s", tid, strmnum((double)nt,num,sizeof(num),0));
            fprintf(stderr, "/%s\n", strmnum((double)nw,num,sizeof(num),0));
        }
    }

    /* Unbind parameters (if parallel unbind is executed in Oloadbuff) */
    if ( !etab[eid].ps )
        (void)SQLFreeStmt(thps[tid].Os, SQL_RESET_PARAMS);

    /* Close input file */
    if ( isgz ) 
        inflateEnd ( &gzstream );
    if (fl && fl != stdin)
        fclose ( fl );
#ifdef HDFS
    else if ( fhl )
        (*hdfsclose)(hfs, fhl);
#endif

    /* Free Memory */
    if ( Odel ) 
        free ( Odel );
    if ( Oins )
        free ( Oins );
    if ( gzbuff )
        free ( gzbuff );
    if ( buff_save )
        free ( buff_save );
    if ( ldrs )     /* Free ldrs */
        free(ldrs); 
    if ( etab[eid].Orowsetl )       /* Free Orowsetl if allocated */
        free(etab[eid].Orowsetl);   
    if ( etab[eid].Ostatusl )       /* Free Ostatusl if allocated */
        free(etab[eid].Ostatusl);       
    if ( etab[eid].td ) {       
        for ( i = 0; i < l ; i++)
            free ( etab[eid].td[i].Oname );
        free(etab[eid].td);
    }
    if (str) {
        free(str);
    }

    /* Close bad file */
    if ( etab[eid].fo != stderr )
        fclose ( etab[eid].fo );

    /* Run "post" SQL */
    if ( etab[eid].post ) {
        var_set ( &thps[tid].tva, VTYPE_I, "tgt", etab[eid].tgt );
        etab[eid].flg2 |= 020000000 ;           /* Oexec to allocate/use its own handle */
        if ( etab[eid].post[0] == '@' )         /* run a sql script */
            (void)runsql(tid, eid, 0, (etab[eid].post + 1 ));
        else                                    /* Run single SQL command */
            (void)Oexec(tid, eid, 0, 0, (SQLCHAR *)etab[eid].post, "");
        etab[eid].flg2 &= ~020000000 ;          /* reset Oexec to allocate/use its own handle */
    }
    return;
}

#ifdef XML
/* OloadX:
 *      load XML files using parameters in etab[eid]
 *
 *      eid (I): etab entry ID to run
 *
 *      return: void
 */
static void OloadX(int eid)
{
    int tid = etab[eid].id;     /* Thread ID */
    SQLCHAR *Oins = 0;          /* INSERT Statement */
    SQLCHAR *Odel = 0;          /* DELETE Statement */
    SQLCHAR *O = 0;             /* ODBC temp variable for memory realloc */
    SQLRETURN Or=0;             /* ODBC return value */
    unsigned long nw=0,         /* no of wait cycles */
                  nt=0,         /* No of total write cycles */
                  nrf=0;        /* no of read records */
    long telaps = 0,            /* Elapsed time in ms */
         tinit = 0;             /* Initialization time in ms */
    size_t nb = 0,              /* number of bytes read from input file */
           cl = CMD_CHUNK,      /* INSERT command buffer length */
           cmdl = 0,            /* INSERT command length */
           dell = CMD_CHUNK,    /* DELETE command length */
           ifl = 0;             /* input field length */
    int z = 0;                  /* loop variable */
    unsigned int nldr=0,        /* number of loaders */
                 k=0,           /* input file field number */
                 m = 0,         /* rowset array record index */
                 i=0,           /* loop variable */
                 pstats = 0,    /* Error flag: 0 = print stats, 1 = don't print stats */
                 l=0,           /* destination table fields */
                 j=0;           /* loop variable */
    int *ldrs=0;                /* pointer to array containing loaders EIDs */
    char buff[128];             /* buffer to build output file name */
    char num[32];               /* Formatted Number String */
    char tim[15];               /* Formatted Time String */
    struct timeval tve;         /* timeval struct to define elapesd/timelines */
    SQLCHAR *Odp = 0;           /* rowset buffer data pointer */
    double seconds = 0;         /* seconds used for timings */
    xmlTextReaderPtr xread = 0; /* XML reader */
    int xnt = 0 ;               /* xml node type */
    int xnd = 0 ;               /* xml node depth */
    int xrtnd = 0 ;             /* xml row tag node depth */
    unsigned int xpflag = 1 ;   /* xml process node flag: 0=no, 1=yes */
    unsigned int xdump = 0 ;    /* xml dump flag: 0=no, 1=yes */
    unsigned int xttype = 0 ;   /* xml tag type : 0=text, 1=attr */
    char *xvalue = 0 ;          /* xml return value */
    char *xname = 0 ;           /* xml node name */
                                
    /* Check if we have to use another ODBC connection */
    if ( thps[tid].cr > 0 ) {
        thps[tid].Oc = thps[thps[tid].cr].Oc;
        thps[tid].Os = thps[thps[tid].cr].Os;
    }

    /* Set "tgt" variable */
    var_set ( &thps[tid].tva, VTYPE_I, "tgt", etab[eid].tgt );

    /* Run "pre" SQL */
    if ( etab[eid].pre ) {
        etab[eid].flg2 |= 020000000 ;           /* Oexec to allocate/use its own handle */
        if ( etab[eid].pre[0] == '@' )          /* run a sql script */
            z = runsql(tid, eid, 0, (etab[eid].pre + 1 ));
        else                                    /* Run single SQL command */
            z = Oexec(tid, eid, 0, 0, (SQLCHAR *)etab[eid].pre, "");
        etab[eid].flg2 &= ~020000000 ;          /* reset Oexec to allocate/use its own handle */
        if ( z && etab[eid].flg & 0004 )
            goto oloadX_exit;
        etab[eid].nr = 0;   /* reset number of resulting rows */
        (void)SQLFreeHandle(SQL_HANDLE_STMT, thps[tid].Os);
        if ( !SQL_SUCCEEDED(Or=SQLAllocHandle(SQL_HANDLE_STMT, thps[tid].Oc, &thps[tid].Os))){
            Oerr(eid, tid, __LINE__, Oc, SQL_HANDLE_DBC);
            goto oloadX_exit;
        }
    }

    /* Check database type */
    etab[eid].dbt = checkdb( eid, &thps[tid].Oc, NULL, NULL);

    /* Check if truncate */
    if ( etab[eid].flg & 0002 ) {           
        etab[eid].flg &= ~0200000;      /* if truncate... unset ifempty */
        if ( ( Odel = malloc(dell) ) == (void *)NULL ) {
            fprintf(stderr, "odb [OloadX(%d)] - Error allocating Odel memory: [%d] %s\n",
                __LINE__, errno, strerror(errno));
            goto oloadX_exit;
        }
        strmcpy((char *)Odel, dbscmds[etab[eid].dbt].trunc, dell);
        Odel = (SQLCHAR *)var_exp((char *)Odel, &dell, &thps[tid].tva);
    }
        
    /* check ifempty */
    if ( etab[eid].flg & 0200000 ) {    /* ifempty is set */
        if ( ifempty( eid, etab[eid].tgt ) ) {
            fprintf(stderr, "odb [OloadX(%d)] - Target table %s is not empty\n",
                    __LINE__, etab[eid].tgt);
            etab[eid].post = 0; /* prevent post SQL execution */
            goto oloadX_exit;
        }
    }
        
    /* Initialize xttype & xdump */
    if ( etab[eid].flg2 & 02000000 )
        xttype = 1 ;
    if ( etab[eid].flg2 & 0200000000 )
        xdump = 1 ;

    /* Open input file */
    for ( i = j = 0; i < sizeof(buff) && etab[eid].src[i]; i++ ) {
        switch ( etab[eid].src[i] ) {
        case '%':
            switch ( etab[eid].src[++i] ) {
            case 't':
                j += strmcat (buff, (char *)etab[eid].Ocso[2], (size_t)etab[eid].buffsz, 2 ); 
                break;
            case 'T':
                j += strmcat (buff, (char *)etab[eid].Ocso[2], (size_t)etab[eid].buffsz, 1 ); 
                break;
            case 's':
                j += strmcat (buff, (char *)etab[eid].Ocso[1], (size_t)etab[eid].buffsz, 2 ); 
                break;
            case 'S':
                j += strmcat (buff, (char *)etab[eid].Ocso[1], (size_t)etab[eid].buffsz, 1 ); 
                break;
            case 'c':
                j += strmcat (buff, (char *)etab[eid].Ocso[0], (size_t)etab[eid].buffsz, 2 ); 
                break;
            case 'C':
                j += strmcat (buff, (char *)etab[eid].Ocso[0], (size_t)etab[eid].buffsz, 1 ); 
                break;
            default:
                fprintf(stderr, "odb [OloadX(%d)] - Invalid expansion %%%c in %s\n",
                    __LINE__, etab[eid].src[i], etab[eid].src );
                goto oloadX_exit;
            }
            break;
        default:
            buff[j++] = etab[eid].src[i];
            break;
        }
    }
    buff[j] = '\0';

    /* Open input file */
    if ( etab[eid].xrt ) {
        if ( ( xread = (*xmlfile)( buff ) ) == (xmlTextReaderPtr) NULL ) {
            fprintf(stderr, "odb [OloadX(%d} - Error opening XML input file %s\n", __LINE__, etab[eid].src);
            goto oloadX_exit;
        }
    }

    /* Analyze target table */
    z = Otcol(eid, &thps[tid].Oc) ;
    if ( z <= 0 ) {
        fprintf(stderr, "odb [OloadX(%d)] - Error analyzing target table %s.%s.%s\n", 
            __LINE__, etab[eid].Ocso[0], etab[eid].Ocso[1], etab[eid].Ocso[2]);
        goto oloadX_exit;
    } else {
        l = (unsigned int)z ;
    }

    /* Adjust Osize for WCHAR field (up to 4 bytes/char). Set buffer size */
    if ( etab[eid].dbt != VERTICA ) {       /* Vertica's CHAR field length is in bytes (not chars) */
        for ( j = 0 ; j < l ; j++ ) {
            switch ( etab[eid].td[j].Otype ) {
            case SQL_WCHAR:
            case SQL_WVARCHAR:
            case SQL_WLONGVARCHAR:
                etab[eid].td[j].Osize *= etab[eid].bpwc;    /* Space for UTF-8 conversion */
                break;
            case SQL_CHAR:
            case SQL_VARCHAR:
            case SQL_LONGVARCHAR:
                etab[eid].td[j].Osize *= etab[eid].bpc;/* Space for UTF-8 conversion */
                break;
            }
        }
    }

    /* Determine buffer start positions and size */
    for ( j = 0 ; j < l ; j++ ) {
        etab[eid].td[j].start = etab[eid].s;
        #ifdef __hpux
            if ( etab[eid].td[j].Osize % WORDSZ )
                etab[eid].td[j].pad = WORDSZ - etab[eid].td[j].Osize % WORDSZ ;
        #endif
        etab[eid].s += ( etab[eid].td[j].Osize + etab[eid].td[j].pad + sizeof(SQLLEN) );    /* space for length indicator */
    }

    /* Truncate target table */
    if ( etab[eid].flg & 0002 ) {   /* truncate target table */
        if ( f & 020000 )   /* if verbose */
            fprintf(stderr, "odb: Now truncating target table (%s)... ", (char *)Odel);
        etab[eid].flg2 |= 020000000 ;           /* Oexec to allocate/use its own handle */
        z = Oexec(tid, eid, 0, 0, Odel, "");    /* Run Truncate */
        etab[eid].flg2 &= ~020000000 ;          /* reset Oexec to allocate/use its own handle */
        if ( j && etab[eid].flg & 0004 ) {
            fprintf(stderr, "odb [OloadX(%d)] - Error truncating Target table. Exiting because Stop On Error was set\n", 
                __LINE__);
            goto oloadX_exit;
        }
        if ( etab[eid].cmt ) {  /* Autocommit off: have to commit truncate */
            if (!SQL_SUCCEEDED(Or=SQLEndTran(SQL_HANDLE_DBC, thps[tid].Oc, SQL_COMMIT))) {
                Oerr(eid, tid, __LINE__, thps[tid].Oc, SQL_HANDLE_DBC);
                goto oloadX_exit;
            }
        }
        etab[eid].nr = 0;   /* reset number of resulting rows */
        if ( f & 020000 )   /* if verbose */
            fprintf(stderr, "done\n");
    }

    /* Calculate rowset if buffer size is set */
    if ( etab[eid].rbs ) {
        etab[eid].r = etab[eid].rbs / etab[eid].s;
        etab[eid].r = etab[eid].r < 1 ? 1 : etab[eid].r;    /* at least one record at a time  */
    }
    if ( etab[eid].flg2 & 0001 )    /* commit as multiplier */
        etab[eid].cmt *= (int)etab[eid].r ;

    /* Build INSERT statement */
    if ( ( Oins = malloc ( cl ) ) == (void *) NULL ) {
        fprintf(stderr, "odb [OloadX(%d)] - Error allocating Oins memory\n", __LINE__);
        goto oloadX_exit;
    }

    if (!strcasecmp(etab[eid].loadcmd, "UL"))
    {
        cmdl += snprintf((char *)Oins, cl, "UPSERT USING LOAD %s%sINTO %s%c%s%c%s VALUES(",
            etab[eid].flg & 040000000 ? "/*+ DIRECT */ " : "",
            etab[eid].flg2 & 0002 ? "WITH NO ROLLBACK " : "",
            etab[eid].Ocso[0] ? (char *)etab[eid].Ocso[0] : " ",
            etab[eid].Ocso[0] ? '.' : ' ',
            etab[eid].Ocso[1] ? (char *)etab[eid].Ocso[1] : " ",
            etab[eid].Ocso[1] ? '.' : ' ', (char *)etab[eid].Ocso[2]);
    }
    else if (!strcasecmp(etab[eid].loadcmd, "UP"))
    {
        cmdl += snprintf((char *)Oins, cl, "UPSERT %s%sINTO %s%c%s%c%s VALUES(",
            etab[eid].flg & 040000000 ? "/*+ DIRECT */ " : "",
            etab[eid].flg2 & 0002 ? "WITH NO ROLLBACK " : "",
            etab[eid].Ocso[0] ? (char *)etab[eid].Ocso[0] : " ",
            etab[eid].Ocso[0] ? '.' : ' ',
            etab[eid].Ocso[1] ? (char *)etab[eid].Ocso[1] : " ",
            etab[eid].Ocso[1] ? '.' : ' ', (char *)etab[eid].Ocso[2]);
    }
    else if (!strcasecmp(etab[eid].loadcmd, "IN"))
    {
        cmdl += snprintf((char *)Oins, cl, "INSERT %s%sINTO %s%c%s%c%s VALUES(",
            etab[eid].flg & 040000000 ? "/*+ DIRECT */ " : "",
            etab[eid].flg2 & 0002 ? "WITH NO ROLLBACK " : "",
            etab[eid].Ocso[0] ? (char *)etab[eid].Ocso[0] : " ",
            etab[eid].Ocso[0] ? '.' : ' ',
            etab[eid].Ocso[1] ? (char *)etab[eid].Ocso[1] : " ",
            etab[eid].Ocso[1] ? '.' : ' ', (char *)etab[eid].Ocso[2]);
    }

    for (i = 0; i < l; i++) {
        if ( i )
            cl += strmcat ( (char *) Oins, ",", cl, 0);
        cl += strmcat ( (char *) Oins, "?", cl, 0);
        if ( cmdl + CMD_CHUNK < cl ) {  /* increase Oins buffer */
            cl += CMD_CHUNK;
            O = Oins ;
            if ( ( O = realloc ( O, cl ) ) == (void *)NULL ) {
                fprintf(stderr, "odb [OloadX(%d)] - Error re-allocating memory for Ocmd: [%d] %s\n",
                    __LINE__, errno, strerror(errno));
                goto oloadX_exit;
            }
            Oins = O ;
        }
    }
    (void) strmcat ( (char *) Oins, ")", cl, 0);
    if ( f & 020000 )   /* if verbose */
        fprintf(stderr, "odb [OloadX(%d)] - INSERT statement: %s\n", __LINE__, (char *)Oins);

    /* Allocate loader eids array */
    nldr = etab[eid].ps ? etab[eid].ps : 1;
    if ( ( ldrs = (int *)calloc ( nldr, sizeof ( int ) ) ) == (void *)NULL ) {
        fprintf(stderr, "odb [OloadX(%d)] - Error allocating loader eids array: [%d] %s\n",
            __LINE__, errno, strerror(errno));
        goto oloadX_exit;
    }

    /* Initialize loader eids array */
    if ( etab[eid].ps ) {
        for ( i = 0, j = 0 ; i < (unsigned int)no && j < etab[eid].ps ; i++ ){
            if ( etab[i].type == 'L' && etab[i].parent == eid ){
                etab[i].r = etab[eid].r;    /* adjust rowset when rbs is specified */
                etab[i].cmt = etab[eid].cmt;/* adjust rowset when cmt is a multiplier */
                etab[i].lstat = 0;          /* reset buffer status to available */
                etab[i].td = etab[eid].td;  /* copy table structure pointer */
                ldrs[j++] = i;
            }
        }
    } else {
        ldrs[0] = eid;
    }

    /* Allocate rowset & status memory */
    if ( (Odp = etab[eid].Orowsetl = (SQLCHAR *)calloc (etab[eid].r, etab[eid].s)) == (void *)NULL ||
         (etab[eid].Ostatusl = (SQLUSMALLINT *)calloc (etab[eid].r, sizeof(SQLUSMALLINT))) == (void *)NULL ) {
            fprintf(stderr, "odb [OloadX(%d)] - Error allocating rowset memory: [%d] %s\n",
                __LINE__, errno, strerror(errno));
        goto oloadX_exit;
    }
    if ( etab[eid].ps ) {
        for ( j = 0 ; j < nldr ; j++ ) {    /* for all loading threads... */
            if ( (etab[ldrs[j]].Orowsetl = (SQLCHAR *)calloc (etab[eid].r, etab[eid].s)) == (void *)NULL ||
                (etab[ldrs[j]].Ostatusl = (SQLUSMALLINT *)calloc (etab[eid].r, sizeof(SQLUSMALLINT))) == (void *)NULL ) {
                    fprintf(stderr, "odb [OloadX(%d)] - Error allocating rowset memory: [%d] %s\n",
                        __LINE__, errno, strerror(errno));
                goto oloadX_exit;
            }
        }
    }

    /* Set rowset size, bind parameters and prepare INSERT */
    for ( j = 0 ; j < nldr ; j++ ) {
        /* Set max commit mode */
        if ( etab[eid].cmt ) {
            if (!SQL_SUCCEEDED(Or=SQLSetConnectAttr(thps[etab[ldrs[j]].id].Oc,
                    SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_OFF, SQL_IS_UINTEGER))) {
                Oerr(eid, tid, __LINE__, thps[etab[ldrs[j]].id].Oc, SQL_HANDLE_DBC);
                goto oloadX_exit;
            }
        } else {
            if (!SQL_SUCCEEDED(Or=SQLSetConnectAttr(thps[etab[ldrs[j]].id].Oc,
                    SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_ON, SQL_IS_UINTEGER))) {
                Oerr(eid, tid, __LINE__, thps[etab[ldrs[j]].id].Oc, SQL_HANDLE_DBC);
                goto oloadX_exit;
            }
        }
        /* Set max char/varchar/binary column length */
        if (!SQL_SUCCEEDED(Oret=SQLSetStmtAttr(thps[etab[ldrs[j]].id].Os, SQL_ATTR_MAX_LENGTH,
            (SQLPOINTER)etab[eid].Omaxl, SQL_IS_UINTEGER))) {
            Oerr(eid, tid, __LINE__, thps[etab[ldrs[j]].id].Os, SQL_HANDLE_STMT);
        }
        /* Bind parameters */
        if (!SQL_SUCCEEDED(Or=SQLSetStmtAttr(thps[etab[ldrs[j]].id].Os, SQL_ATTR_PARAM_BIND_TYPE,
                (SQLPOINTER)(etab[eid].s), 0))) {
            Oerr(eid, tid, __LINE__, thps[etab[ldrs[j]].id].Os, SQL_HANDLE_STMT);
            goto oloadX_exit;
        }
        if (!SQL_SUCCEEDED(Or=SQLSetStmtAttr(thps[etab[ldrs[j]].id].Os, SQL_ATTR_PARAMSET_SIZE,
                (SQLPOINTER)(etab[eid].r), 0))) {
            Oerr(eid, tid, __LINE__, thps[etab[ldrs[j]].id].Os, SQL_HANDLE_STMT);
            goto oloadX_exit;
        }
        if (!SQL_SUCCEEDED(Or=SQLSetStmtAttr(thps[etab[ldrs[j]].id].Os, SQL_ATTR_PARAM_STATUS_PTR,
                etab[ldrs[j]].Ostatusl, 0))) {
            Oerr(eid, tid, __LINE__, thps[etab[ldrs[j]].id].Os, SQL_HANDLE_STMT);
            goto oloadX_exit;
        }
        if (!SQL_SUCCEEDED(Or=SQLSetStmtAttr(thps[etab[ldrs[j]].id].Os, SQL_ATTR_PARAMS_PROCESSED_PTR, 
                &etab[ldrs[j]].Oresl, 0))) {
            Oerr(eid, tid, __LINE__, thps[etab[ldrs[j]].id].Os, SQL_HANDLE_STMT);
            goto oloadX_exit;
        }
        for ( i = 0, z = 1; i < l; i++ ) {
            if (!SQL_SUCCEEDED(Or=SQLBindParameter(thps[etab[ldrs[j]].id].Os, (SQLUSMALLINT)z++,
                    SQL_PARAM_INPUT, SQL_C_CHAR, etab[eid].td[i].Otype, (SQLULEN)etab[eid].td[i].Osize,
                    etab[eid].td[i].Odec, &etab[ldrs[j]].Orowsetl[0+etab[eid].td[i].start], etab[eid].td[i].Osize,
                    (SQLLEN *)&etab[ldrs[j]].Orowsetl[0+etab[eid].td[i].start+etab[eid].td[i].Osize+etab[eid].td[i].pad]))) {
                Oerr(eid, tid, __LINE__, thps[etab[ldrs[j]].id].Os, SQL_HANDLE_STMT);
                goto oloadX_exit;
            }
        }
        if (!SQL_SUCCEEDED(Or=SQLPrepare(thps[etab[ldrs[j]].id].Os, Oins, SQL_NTS))) {
            Oerr(eid, tid, __LINE__, thps[etab[ldrs[j]].id].Os, SQL_HANDLE_STMT);
            goto oloadX_exit;
        }
        etab[ldrs[j]].sp = l;   /* save number of target table fields for prec */
    }

    /* Register Load Start Time */
    gettimeofday(&tve, (void *)NULL);
    tinit = 1000*(tve.tv_sec-tvi.tv_sec)+(tve.tv_usec-tvi.tv_usec)/1000;

    /* XML load */
    pstats = 1 ;    /* from now on print stats on exit */
    while ( ( z = (*xmlread)(xread) ) == 1 ) {
        xnt = (*xmltype)(xread) ;   /* save node type */
        xnd = (*xmldepth)(xread) ;  /* save node depth */
        if ( !strcmp(etab[eid].xrt, (char *)(*xmllname)(xread)) ) {
            if ( xnt == 1 ) {       /* rowtag start */
                if ( xttype ) {     /* process attributes */
                    k = 0 ;         /* reset field number counter */
                    while ( (*xmlnextattr)(xread) ) {
                        xname = (char *)(*xmllname)(xread) ;
                        xvalue = (char *)(*xmlvalue)(xread) ; 
                        if ( xdump ) {  /* XML dump: print key/value */
                            printf("%s: %s\n", xname, xvalue);
                        } else {        /* XML Load */
                            ifl = strlen(xvalue) ;
                            if ( etab[eid].flg2 & 010000000 ) { /* xmlord is set */
                                Odp = etab[eid].Orowsetl + m*etab[eid].s + etab[eid].td[k].start ;
                                MEMCPY(Odp, xvalue, ifl);
                                Odp += etab[eid].td[k].Osize + etab[eid].td[k].pad ;
                                *((SQLLEN *)(Odp)) = (SQLLEN)(ifl) ;
                                if ( ++k == l )
                                    break ;
                            } else {    /* xmlord not set: use attr names */
                                for ( k = 0 ; k < l ; k++ ) {
                                    if ( !strmicmp((char *)etab[eid].td[k].Oname, xname, strlen(xname) ) ) {    /* name matches */
                                        Odp = etab[eid].Orowsetl + m*etab[eid].s + etab[eid].td[k].start ;
                                        MEMCPY(Odp, xvalue, ifl);
                                        Odp += etab[eid].td[k].Osize + etab[eid].td[k].pad ;
                                        *((SQLLEN *)(Odp)) = (SQLLEN)(ifl) ;
                                        break;
                                    }
                                }
                            }
                        }
                    }
                } else {            /* process nodes */
                    xpflag = 1 ;
                    xrtnd = xnd ;
                }
            } else if ( xnt == 15 ) {   /* rowtag end */
                m++ ;       /* increase rowset index */
                nrf++ ;     /* increase "read rows" counter */
                k = 0 ;     /* reset field number counter to zero */
                xpflag = 0; /* set process flag to zero */
            }
        } else if ( xnd > xrtnd + 2 ){
            xpflag = 0 ;    /* too deep: set process flag to zero */
        }
        /* process sub-rowtag nodes of depth 1+1 */
        if ( xpflag ) {
            switch ( xnt ) {
            case 1:                     /* node start */
                if ( ( xnd - xrtnd ) == 1 ) {
                    xname = (char *)(*xmllname)(xread);
                    xvalue = 0 ;
                }
                break;
            case 3:                     /* text node */
                if ( ( xnd - xrtnd ) == 2 ) {
                    xvalue = (char *)(*xmlvalue)(xread);
                    ifl = strlen(xvalue) ;
                    if (ifl > etab[eid].td[k].Osize) { // prevent Orowsetl[] overflow
                        fprintf(stderr, "odb [OloadX(%d)] - Error: row %lu col %u field truncation. Input "
                            "string: >%s< of length %lu.\n", __LINE__, nrf + 1, k + 1, xvalue, ifl);
                        goto oloadX_exit;
                    }
                    if ( xdump ) {
                        printf("%s: %s\n", xname, xvalue);
                    } else {
                        if ( etab[eid].flg2 & 010000000 ) { /* xmlord is set */
                            Odp = etab[eid].Orowsetl + m*etab[eid].s + etab[eid].td[k].start ;
                            MEMCPY(Odp, xvalue, ifl);
                            Odp += etab[eid].td[k].Osize + etab[eid].td[k].pad ;
                            *((SQLLEN *)(Odp)) = (SQLLEN)(ifl) ;
                        } else {    /* xmlord not set: use attr names */
                            for ( k = 0 ; k < l ; k++ ) {
                                if ( !strmicmp((char *)etab[eid].td[k].Oname, xname, strlen(xname) ) ) {    /* name matches */
                                    Odp = etab[eid].Orowsetl + m*etab[eid].s + etab[eid].td[k].start ;
                                    MEMCPY(Odp, xvalue, ifl);
                                    Odp += etab[eid].td[k].Osize + etab[eid].td[k].pad ;
                                    *((SQLLEN *)(Odp)) = (SQLLEN)(ifl) ;
                                    break;
                                }
                            }
                        }
                    }
                }
                break;
            case 15:                    /* node end */
                if ( ( xnd - xrtnd ) == 1 ) {
                    if ( xvalue == 0 ) {/* text node was empty */
                        if ( xdump ) {
                            printf("%s: %s\n", xname, xvalue);
                        } else {
                            if ( etab[eid].flg2 & 010000000 ) { /* xmlord is set */
                                Odp = etab[eid].Orowsetl + m*etab[eid].s + etab[eid].td[k].start +
                                      etab[eid].td[k].Osize + etab[eid].td[k].pad ;
                                *((SQLLEN *)(Odp)) = (SQLLEN)SQL_NULL_DATA ;
                                if ( ++k == l )
                                    break ;
                            } else {    /* xmlord not set: use attr names */
                                for ( k = 0 ; k < l ; k++ ) {
                                    if ( !strmicmp((char *)etab[eid].td[k].Oname, xname, strlen(xname) ) ) {    /* name matches */
                                        Odp = etab[eid].Orowsetl + m*etab[eid].s + etab[eid].td[k].start +
                                              etab[eid].td[k].Osize + etab[eid].td[k].pad ;
                                        *((SQLLEN *)(Odp)) = (SQLLEN)(SQL_NULL_DATA) ;
                                        break;
                                    }
                                }
                            }
                        }
                    }
                }
                if ( ++k > l )
                    k = 0 ;
                break;
            }
        }
        if ( m == etab[eid].r ) {                           /* Insert rowset */
            nt++;
            while ( go ) {
                if ( etab[eid].ps ) {                       /* Find a buffer loader thread available */
                    MutexLock(&etab[eid].pmutex);
                    while ( go ) {
                        for ( i = 0 ; i < nldr ; i++ ) {    /* Look for write_available buffers */
                            if ( etab[ldrs[i]].lstat == 0 ) {
                                etab[ldrs[i]].lstat = 2;    /* write_busy */
                                break;
                            }
                        }
                        if ( etab[eid].flg & 04000 ) {      /* Oloadbuff set error flag */
                            break;
                        } else if ( i >= nldr ) {           /* nothing available... wait */
                            nw++;
                            CondWait(&etab[eid].pcvp, &etab[eid].pmutex);
                        } else {
                            break ;
                        }
                    }
                    MutexUnlock(&etab[eid].pmutex);
                    if ( etab[eid].flg & 04000 )            /* Oloadbuff set error flag */
                        break;
                    memcpy(etab[ldrs[i]].Orowsetl, etab[eid].Orowsetl, m * etab[eid].s);
                    etab[ldrs[i]].nbs = nrf - (unsigned long)m ;    /* pass the input file base row for this rowset */
                    etab[ldrs[i]].lstat = 1;                /* read_available */
                    etab[ldrs[i]].ar = m;
                    CondWakeAll(&etab[eid].pcvc);
                } else {
                    etab[ldrs[0]].nbs = nrf - (unsigned long)m ;    /* pass the input file base row for this rowset */
                    etab[ldrs[0]].ar = m;
                    Oloadbuff(eid);
                }
                break;
            }
            if ( etab[eid].flg & 04000 )                    /* Oloadbuff set error flag */
                break ;
            m = 0;
            k = 0;
        }
    }
    (*xmlfree)(xread);
    /* load trailing rows */
    if ( m ) {                          /* Insert rowset */
        nt++;
        while ( go ) {
            if ( etab[eid].ps ) {                       /* Find a buffer loader thread available */
                MutexLock(&etab[eid].pmutex);
                while ( go ) {
                    for ( i = 0 ; i < nldr ; i++ ) {    /* Look for write_available buffers */
                        if ( etab[ldrs[i]].lstat == 0 ) {
                            etab[ldrs[i]].lstat = 2;    /* write_busy */
                            break;
                        }
                    }
                    if ( etab[eid].flg & 04000 ) {      /* Oloadbuff set error flag */
                        break;
                    } else if ( i >= nldr ) {           /* nothing available... wait */
                        nw++;
                        CondWait(&etab[eid].pcvp, &etab[eid].pmutex);
                    } else {
                        break ;
                    }
                }
                MutexUnlock(&etab[eid].pmutex);
                if ( etab[eid].flg & 04000 )            /* Oloadbuff set error flag */
                    break;
                memcpy(etab[ldrs[i]].Orowsetl, etab[eid].Orowsetl, m * etab[eid].s);
                etab[ldrs[i]].nbs = nrf - (unsigned long)m ;    /* pass the input file base row for this rowset */
                etab[ldrs[i]].lstat = 1;                /* read_available */
                etab[ldrs[i]].ar = m;
                CondWakeAll(&etab[eid].pcvc);
            } else {
                etab[ldrs[0]].nbs = nrf - (unsigned long)m ;    /* pass the input file base row for this rowset */
                etab[ldrs[0]].ar = m;
                Oloadbuff(eid);
            }
            break;
        }
    }
    oloadX_exit:
    etab[eid].flg |= 02000;                 /* mark EOF */
    if ( etab[eid].ps ) {                   /* wait all loaders to complete */
        CondWakeAll(&etab[eid].pcvc);
        MutexLock(&etab[eid].pmutex);
        while ( go ) {
            for ( i = 0, k=0 ; i < nldr ; i++ ) {
                k += etab[ldrs[i]].lstat;
            }
            if ( k != nldr * 10 )
                CondWait(&etab[eid].pcvp, &etab[eid].pmutex);
            else 
                break;
        }
        MutexUnlock(&etab[eid].pmutex);
    }
    telaps -= 1000*tve.tv_sec + tve.tv_usec/1000;
    gettimeofday(&tve, (void *)NULL);       /* register end time */
    telaps += 1000*tve.tv_sec + tve.tv_usec/1000;
    
    for ( i = 0 ; i < nldr ; i++ )
        etab[eid].nrt += etab[ldrs[i]].nr;

    /* Print results */
    if ( pstats ) {
        fprintf(stderr, "[%d] %s Load(X) statistics:\n\t[%d] Target table: %s.%s.%s\n",
            tid, odbid, tid, etab[eid].Ocso[0], etab[eid].Ocso[1], etab[eid].Ocso[2]);
        fprintf(stderr, "\t[%d] Source: %s\n", tid, etab[eid].src);
        seconds = (double)tinit/1000.0 ;
        fprintf(stderr, "\t[%d] Pre-loading time: %s s (%s)\n", tid,
            strmnum(seconds, num, sizeof(num), 3), strmtime(seconds, tim));
        seconds = (double)telaps/1000.0 ;
        fprintf(stderr, "\t[%d] Loading time: %s s(%s)\n", tid,
            strmnum(seconds, num, sizeof(num), 3), strmtime(seconds, tim));
        fprintf(stderr, "\t[%d] Total records read: %s\n", tid, strmnum((double)nrf,num,sizeof(num),0));
        fprintf(stderr, "\t[%d] Total records inserted: %s\n", tid, strmnum((double)etab[eid].nrt,num,sizeof(num),0));
        fprintf(stderr, "\t[%d] Total number of columns: %s\n", tid, strmnum((double)l,num,sizeof(num),0));
        fprintf(stderr, "\t[%d] Average input row size: %s B\n", tid, strmnum((double)nb*1.0/nrf,num,sizeof(num),1));
        fprintf(stderr, "\t[%d] ODBC row size: %s B (data)", tid, strmnum((double)(etab[eid].s-l*sizeof(SQLLEN)),num,sizeof(num),0));
        fprintf(stderr, " + %s B (len ind)\n", strmnum((double)(l*sizeof(SQLLEN)), num,sizeof(num),0));
        fprintf(stderr, "\t[%d] Rowset size: %s\n", tid, strmnum((double)etab[eid].r,num,sizeof(num),0));
        fprintf(stderr, "\t[%d] Rowset buffer size: %s KiB\n", tid, strmnum((double)etab[eid].s/1024.0*etab[eid].r,num,sizeof(num),2));
        fprintf(stderr, "\t[%d] Load throughput (real data): %s KiB/s\n", tid, strmnum((double)nb/1.024/telaps,num,sizeof(num),3));
        fprintf(stderr, "\t[%d] Load throughput (ODBC): %s KiB/s\n", tid,
            strmnum((double)((etab[eid].s-l*sizeof(SQLLEN))*etab[eid].nrt)/1.024/telaps,num,sizeof(num),3));
        if ( etab[eid].ps ){
            fprintf(stderr, "\t[%d] Reader Total/Wait Cycles: %s", tid, strmnum((double)nt,num,sizeof(num),0));
            fprintf(stderr, "/%s\n", strmnum((double)nw,num,sizeof(num),0));
        }
    }

    /* Unbind parameters (if parallel unbind is executed in Oloadbuff) */
    if ( !etab[eid].ps )
        (void)SQLFreeStmt(thps[tid].Os, SQL_RESET_PARAMS);

    /* Free Memory */
    if ( Odel ) 
        free ( Odel );
    if ( Oins )
        free ( Oins );
    if ( ldrs )     /* Free ldrs */
        free(ldrs); 
    if ( etab[eid].Orowsetl )       /* Free Orowsetl if allocated */
        free(etab[eid].Orowsetl);   
    if ( etab[eid].Ostatusl )       /* Free Ostatusl if allocated */
        free(etab[eid].Ostatusl);       
    if ( etab[eid].td ) {       
        for ( i = 0; i < l ; i++)
            free ( etab[eid].td[i].Oname );
        free(etab[eid].td);
    }

    /* Close bad file */
    if ( etab[eid].fo != stderr )
        fclose ( etab[eid].fo );

    /* Run "post" SQL */
    if ( etab[eid].post ) {
        var_set ( &thps[tid].tva, VTYPE_I, "tgt", etab[eid].tgt );
        etab[eid].flg2 |= 020000000 ;           /* Oexec to allocate/use its own handle */
        if ( etab[eid].post[0] == '@' )         /* run a sql script */
            (void)runsql(tid, eid, 0, (etab[eid].post + 1 ));
        else                                    /* Run single SQL command */
            (void)Oexec(tid, eid, 0, 0, (SQLCHAR *)etab[eid].post, "");
        etab[eid].flg2 &= ~020000000 ;          /* reset Oexec to allocate/use its own handle */
    }
    return;
}
#endif

/* OloadJson:
*      load json files using parameters in etab[eid]
*
*      eid (I): etab entry ID to run
*
*      return: void
*/
static void OloadJson(int eid)
{
    int tid = etab[eid].id;     /* Thread ID */
    SQLCHAR *Oins = 0;          /* INSERT Statement */
    SQLCHAR *Odel = 0;          /* DELETE Statement */
    SQLCHAR *O = 0;             /* ODBC temp variable for memory realloc */
    SQLRETURN Or = 0;             /* ODBC return value */
    unsigned long nw = 0,         /* no of wait cycles */
        nt = 0,         /* No of total write cycles */
        nrf = 0;        /* no of read records */
    long telaps = 0,            /* Elapsed time in ms */
        tinit = 0;             /* Initialization time in ms */
    size_t nb = 0,              /* number of bytes read from input file */
        cl = CMD_CHUNK,      /* INSERT command buffer length */
        cmdl = 0,            /* INSERT command length */
        dell = CMD_CHUNK,    /* DELETE command length */
        ifl = 0;             /* input field length */
    int z = 0;                  /* loop variable */
    unsigned int nldr = 0,        /* number of loaders */
        k = 0,           /* input file field number */
        m = 0,         /* rowset array record index */
        i = 0,           /* loop variable */
        pstats = 0,    /* Error flag: 0 = print stats, 1 = don't print stats */
        l = 0,           /* destination table fields */
        j = 0;           /* loop variable */
    int *ldrs = 0;                /* pointer to array containing loaders EIDs */
    char buff[128];             /* buffer to build output file name */
    char num[32];               /* Formatted Number String */
    char tim[15];               /* Formatted Time String */
    struct timeval tve;         /* timeval struct to define elapesd/timelines */
    SQLCHAR *Odp = 0;           /* rowset buffer data pointer */
    double seconds = 0;         /* seconds used for timings */
    JsonReader *pJsonReader = 0; /* XML reader */
    int readState = 0; /* 0: look for key, 1: look for array value */
    char keybuf[128];
    char *valuebuf;
    /* Check if we have to use another ODBC connection */
    if (thps[tid].cr > 0) {
        thps[tid].Oc = thps[thps[tid].cr].Oc;
        thps[tid].Os = thps[thps[tid].cr].Os;
    }

    /* Set "tgt" variable */
    var_set(&thps[tid].tva, VTYPE_I, "tgt", etab[eid].tgt);

    /* Run "pre" SQL */
    if (etab[eid].pre) {
        etab[eid].flg2 |= 020000000;           /* Oexec to allocate/use its own handle */
        if (etab[eid].pre[0] == '@')          /* run a sql script */
            z = runsql(tid, eid, 0, (etab[eid].pre + 1));
        else                                    /* Run single SQL command */
            z = Oexec(tid, eid, 0, 0, (SQLCHAR *)etab[eid].pre, "");
        etab[eid].flg2 &= ~020000000;          /* reset Oexec to allocate/use its own handle */
        if (z && etab[eid].flg & 0004)
            goto oloadJson_exit;
        etab[eid].nr = 0;   /* reset number of resulting rows */
        (void)SQLFreeHandle(SQL_HANDLE_STMT, thps[tid].Os);
        if (!SQL_SUCCEEDED(Or = SQLAllocHandle(SQL_HANDLE_STMT, thps[tid].Oc, &thps[tid].Os))) {
            Oerr(eid, tid, __LINE__, Oc, SQL_HANDLE_DBC);
            goto oloadJson_exit;
        }
    }

    /* Check database type */
    etab[eid].dbt = checkdb(eid, &thps[tid].Oc, NULL, NULL);

    /* Check if truncate */
    if (etab[eid].flg & 0002) {
        etab[eid].flg &= ~0200000;      /* if truncate... unset ifempty */
        if ((Odel = malloc(dell)) == (void *)NULL) {
            fprintf(stderr, "odb [OloadJson(%d)] - Error allocating Odel memory: [%d] %s\n",
                __LINE__, errno, strerror(errno));
            goto oloadJson_exit;
        }
        strmcpy((char *)Odel, dbscmds[etab[eid].dbt].trunc, dell);
        Odel = (SQLCHAR *)var_exp((char *)Odel, &dell, &thps[tid].tva);
    }

    /* check ifempty */
    if (etab[eid].flg & 0200000) {    /* ifempty is set */
        if (ifempty(eid, etab[eid].tgt)) {
            fprintf(stderr, "odb [OloadJson(%d)] - Target table %s is not empty\n",
                __LINE__, etab[eid].tgt);
            etab[eid].post = 0; /* prevent post SQL execution */
            goto oloadJson_exit;
        }
    }

    /* alocate valuebuf */
    if ((valuebuf = calloc(1, etab[eid].buffsz + 1)) == (void *)NULL) {
        fprintf(stderr, "odb [OloadJson(%d)] - Error allocating field buffer: [%d] %s\n",
            __LINE__, errno, strerror(errno));
        goto oloadJson_exit;
    }

    /* Open input file */
    for (i = j = 0; i < sizeof(buff) && etab[eid].src[i]; i++) {
        switch (etab[eid].src[i]) {
        case '%':
            switch (etab[eid].src[++i]) {
            case 't':
                j += strmcat(buff, (char *)etab[eid].Ocso[2], (size_t)etab[eid].buffsz, 2);
                break;
            case 'T':
                j += strmcat(buff, (char *)etab[eid].Ocso[2], (size_t)etab[eid].buffsz, 1);
                break;
            case 's':
                j += strmcat(buff, (char *)etab[eid].Ocso[1], (size_t)etab[eid].buffsz, 2);
                break;
            case 'S':
                j += strmcat(buff, (char *)etab[eid].Ocso[1], (size_t)etab[eid].buffsz, 1);
                break;
            case 'c':
                j += strmcat(buff, (char *)etab[eid].Ocso[0], (size_t)etab[eid].buffsz, 2);
                break;
            case 'C':
                j += strmcat(buff, (char *)etab[eid].Ocso[0], (size_t)etab[eid].buffsz, 1);
                break;
            default:
                fprintf(stderr, "odb [OloadJson(%d)] - Invalid expansion %%%c in %s\n",
                    __LINE__, etab[eid].src[i], etab[eid].src);
                goto oloadJson_exit;
            }
            break;
        default:
            buff[j++] = etab[eid].src[i];
            break;
        }
    }
    buff[j] = '\0';

    /* Open input file */
    if (etab[eid].jsonKey) {
        if ((pJsonReader = jsonReaderNew(buff)) == NULL) {
            fprintf(stderr, "odb [OloadJson(%d} - Error opening json input file %s\n", __LINE__, etab[eid].src);
            goto oloadJson_exit;
        }
    }

    /* Analyze target table */
    z = Otcol(eid, &thps[tid].Oc);
    if (z <= 0) {
        fprintf(stderr, "odb [OloadJson(%d)] - Error analyzing target table %s.%s.%s\n",
            __LINE__, etab[eid].Ocso[0], etab[eid].Ocso[1], etab[eid].Ocso[2]);
        goto oloadJson_exit;
    }
    else {
        l = (unsigned int)z;
    }

    /* Adjust Osize for WCHAR field (up to 4 bytes/char). Set buffer size */
    if (etab[eid].dbt != VERTICA) {       /* Vertica's CHAR field length is in bytes (not chars) */
        for (j = 0; j < l; j++) {
            switch (etab[eid].td[j].Otype) {
            case SQL_WCHAR:
            case SQL_WVARCHAR:
            case SQL_WLONGVARCHAR:
                etab[eid].td[j].Osize *= etab[eid].bpwc;    /* Space for UTF-8 conversion */
                break;
            case SQL_CHAR:
            case SQL_VARCHAR:
            case SQL_LONGVARCHAR:
                etab[eid].td[j].Osize *= etab[eid].bpc;/* Space for UTF-8 conversion */
                break;
            }
        }
    }

    /* Determine buffer start positions and size */
    for (j = 0; j < l; j++) {
        etab[eid].td[j].start = etab[eid].s;
#ifdef __hpux
        if (etab[eid].td[j].Osize % WORDSZ)
            etab[eid].td[j].pad = WORDSZ - etab[eid].td[j].Osize % WORDSZ;
#endif
        etab[eid].s += (etab[eid].td[j].Osize + etab[eid].td[j].pad + sizeof(SQLLEN));    /* space for length indicator */
    }

    /* Truncate target table */
    if (etab[eid].flg & 0002) {   /* truncate target table */
        if (f & 020000)   /* if verbose */
            fprintf(stderr, "odb: Now truncating target table (%s)... ", (char *)Odel);
        etab[eid].flg2 |= 020000000;           /* Oexec to allocate/use its own handle */
        z = Oexec(tid, eid, 0, 0, Odel, "");    /* Run Truncate */
        etab[eid].flg2 &= ~020000000;          /* reset Oexec to allocate/use its own handle */
        if (j && etab[eid].flg & 0004) {
            fprintf(stderr, "odb [OloadJson(%d)] - Error truncating Target table. Exiting because Stop On Error was set\n",
                __LINE__);
            goto oloadJson_exit;
        }
        if (etab[eid].cmt) {  /* Autocommit off: have to commit truncate */
            if (!SQL_SUCCEEDED(Or = SQLEndTran(SQL_HANDLE_DBC, thps[tid].Oc, SQL_COMMIT))) {
                Oerr(eid, tid, __LINE__, thps[tid].Oc, SQL_HANDLE_DBC);
                goto oloadJson_exit;
            }
        }
        etab[eid].nr = 0;   /* reset number of resulting rows */
        if (f & 020000)   /* if verbose */
            fprintf(stderr, "done\n");
    }

    /* Calculate rowset if buffer size is set */
    if (etab[eid].rbs) {
        etab[eid].r = etab[eid].rbs / etab[eid].s;
        etab[eid].r = etab[eid].r < 1 ? 1 : etab[eid].r;    /* at least one record at a time  */
    }
    if (etab[eid].flg2 & 0001)    /* commit as multiplier */
        etab[eid].cmt *= (int)etab[eid].r;

    /* Build INSERT statement */
    if ((Oins = malloc(cl)) == (void *)NULL) {
        fprintf(stderr, "odb [OloadJson(%d)] - Error allocating Oins memory\n", __LINE__);
        goto oloadJson_exit;
    }

    if (!strcasecmp(etab[eid].loadcmd, "UL"))
    {
        cmdl += snprintf((char *)Oins, cl, "UPSERT USING LOAD %s%sINTO %s%c%s%c%s VALUES(",
            etab[eid].flg & 040000000 ? "/*+ DIRECT */ " : "",
            etab[eid].flg2 & 0002 ? "WITH NO ROLLBACK " : "",
            etab[eid].Ocso[0] ? (char *)etab[eid].Ocso[0] : " ",
            etab[eid].Ocso[0] ? '.' : ' ',
            etab[eid].Ocso[1] ? (char *)etab[eid].Ocso[1] : " ",
            etab[eid].Ocso[1] ? '.' : ' ', (char *)etab[eid].Ocso[2]);
    }
    else if (!strcasecmp(etab[eid].loadcmd, "UP"))
    {
        cmdl += snprintf((char *)Oins, cl, "UPSERT %s%sINTO %s%c%s%c%s VALUES(",
            etab[eid].flg & 040000000 ? "/*+ DIRECT */ " : "",
            etab[eid].flg2 & 0002 ? "WITH NO ROLLBACK " : "",
            etab[eid].Ocso[0] ? (char *)etab[eid].Ocso[0] : " ",
            etab[eid].Ocso[0] ? '.' : ' ',
            etab[eid].Ocso[1] ? (char *)etab[eid].Ocso[1] : " ",
            etab[eid].Ocso[1] ? '.' : ' ', (char *)etab[eid].Ocso[2]);
    }
    else if (!strcasecmp(etab[eid].loadcmd, "IN"))
    {
        cmdl += snprintf((char *)Oins, cl, "INSERT %s%sINTO %s%c%s%c%s VALUES(",
            etab[eid].flg & 040000000 ? "/*+ DIRECT */ " : "",
            etab[eid].flg2 & 0002 ? "WITH NO ROLLBACK " : "",
            etab[eid].Ocso[0] ? (char *)etab[eid].Ocso[0] : " ",
            etab[eid].Ocso[0] ? '.' : ' ',
            etab[eid].Ocso[1] ? (char *)etab[eid].Ocso[1] : " ",
            etab[eid].Ocso[1] ? '.' : ' ', (char *)etab[eid].Ocso[2]);
    }

    for (i = 0; i < l; i++) {
        if (i)
            cl += strmcat((char *)Oins, ",", cl, 0);
        cl += strmcat((char *)Oins, "?", cl, 0);
        if (cmdl + CMD_CHUNK < cl) {  /* increase Oins buffer */
            cl += CMD_CHUNK;
            O = Oins;
            if ((O = realloc(O, cl)) == (void *)NULL) {
                fprintf(stderr, "odb [OloadJson(%d)] - Error re-allocating memory for Ocmd: [%d] %s\n",
                    __LINE__, errno, strerror(errno));
                goto oloadJson_exit;
            }
            Oins = O;
        }
    }
    (void)strmcat((char *)Oins, ")", cl, 0);
    if (f & 020000)   /* if verbose */
        fprintf(stderr, "odb [OloadJson(%d)] - INSERT statement: %s\n", __LINE__, (char *)Oins);

    /* Allocate loader eids array */
    nldr = etab[eid].ps ? etab[eid].ps : 1;
    if ((ldrs = (int *)calloc(nldr, sizeof(int))) == (void *)NULL) {
        fprintf(stderr, "odb [OloadJson(%d)] - Error allocating loader eids array: [%d] %s\n",
            __LINE__, errno, strerror(errno));
        goto oloadJson_exit;
    }

    /* Initialize loader eids array */
    if (etab[eid].ps) {
        for (i = 0, j = 0; i < (unsigned int)no && j < etab[eid].ps; i++) {
            if (etab[i].type == 'L' && etab[i].parent == eid) {
                etab[i].r = etab[eid].r;    /* adjust rowset when rbs is specified */
                etab[i].cmt = etab[eid].cmt;/* adjust rowset when cmt is a multiplier */
                etab[i].lstat = 0;          /* reset buffer status to available */
                etab[i].td = etab[eid].td;  /* copy table structure pointer */
                ldrs[j++] = i;
            }
        }
    }
    else {
        ldrs[0] = eid;
    }

    /* Allocate rowset & status memory */
    if ((Odp = etab[eid].Orowsetl = (SQLCHAR *)calloc(etab[eid].r, etab[eid].s)) == (void *)NULL ||
        (etab[eid].Ostatusl = (SQLUSMALLINT *)calloc(etab[eid].r, sizeof(SQLUSMALLINT))) == (void *)NULL) {
        fprintf(stderr, "odb [OloadJson(%d)] - Error allocating rowset memory: [%d] %s\n",
            __LINE__, errno, strerror(errno));
        goto oloadJson_exit;
    }
    if (etab[eid].ps) {
        for (j = 0; j < nldr; j++) {    /* for all loading threads... */
            if ((etab[ldrs[j]].Orowsetl = (SQLCHAR *)calloc(etab[eid].r, etab[eid].s)) == (void *)NULL ||
                (etab[ldrs[j]].Ostatusl = (SQLUSMALLINT *)calloc(etab[eid].r, sizeof(SQLUSMALLINT))) == (void *)NULL) {
                fprintf(stderr, "odb [OloadJson(%d)] - Error allocating rowset memory: [%d] %s\n",
                    __LINE__, errno, strerror(errno));
                goto oloadJson_exit;
            }
        }
    }

    /* Set rowset size, bind parameters and prepare INSERT */
    for (j = 0; j < nldr; j++) {
        /* Set max commit mode */
        if (etab[eid].cmt) {
            if (!SQL_SUCCEEDED(Or = SQLSetConnectAttr(thps[etab[ldrs[j]].id].Oc,
                SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_OFF, SQL_IS_UINTEGER))) {
                Oerr(eid, tid, __LINE__, thps[etab[ldrs[j]].id].Oc, SQL_HANDLE_DBC);
                goto oloadJson_exit;
            }
        }
        else {
            if (!SQL_SUCCEEDED(Or = SQLSetConnectAttr(thps[etab[ldrs[j]].id].Oc,
                SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_ON, SQL_IS_UINTEGER))) {
                Oerr(eid, tid, __LINE__, thps[etab[ldrs[j]].id].Oc, SQL_HANDLE_DBC);
                goto oloadJson_exit;
            }
        }
        /* Set max char/varchar/binary column length */
        if (!SQL_SUCCEEDED(Oret = SQLSetStmtAttr(thps[etab[ldrs[j]].id].Os, SQL_ATTR_MAX_LENGTH,
            (SQLPOINTER)etab[eid].Omaxl, SQL_IS_UINTEGER))) {
            Oerr(eid, tid, __LINE__, thps[etab[ldrs[j]].id].Os, SQL_HANDLE_STMT);
        }
        /* Bind parameters */
        if (!SQL_SUCCEEDED(Or = SQLSetStmtAttr(thps[etab[ldrs[j]].id].Os, SQL_ATTR_PARAM_BIND_TYPE,
            (SQLPOINTER)(etab[eid].s), 0))) {
            Oerr(eid, tid, __LINE__, thps[etab[ldrs[j]].id].Os, SQL_HANDLE_STMT);
            goto oloadJson_exit;
        }
        if (!SQL_SUCCEEDED(Or = SQLSetStmtAttr(thps[etab[ldrs[j]].id].Os, SQL_ATTR_PARAMSET_SIZE,
            (SQLPOINTER)(etab[eid].r), 0))) {
            Oerr(eid, tid, __LINE__, thps[etab[ldrs[j]].id].Os, SQL_HANDLE_STMT);
            goto oloadJson_exit;
        }
        if (!SQL_SUCCEEDED(Or = SQLSetStmtAttr(thps[etab[ldrs[j]].id].Os, SQL_ATTR_PARAM_STATUS_PTR,
            etab[ldrs[j]].Ostatusl, 0))) {
            Oerr(eid, tid, __LINE__, thps[etab[ldrs[j]].id].Os, SQL_HANDLE_STMT);
            goto oloadJson_exit;
        }
        if (!SQL_SUCCEEDED(Or = SQLSetStmtAttr(thps[etab[ldrs[j]].id].Os, SQL_ATTR_PARAMS_PROCESSED_PTR,
            &etab[ldrs[j]].Oresl, 0))) {
            Oerr(eid, tid, __LINE__, thps[etab[ldrs[j]].id].Os, SQL_HANDLE_STMT);
            goto oloadJson_exit;
        }
        for (i = 0, z = 1; i < l; i++) {
            if (!SQL_SUCCEEDED(Or = SQLBindParameter(thps[etab[ldrs[j]].id].Os, (SQLUSMALLINT)z++,
                SQL_PARAM_INPUT, SQL_C_CHAR, etab[eid].td[i].Otype, (SQLULEN)etab[eid].td[i].Osize,
                etab[eid].td[i].Odec, &etab[ldrs[j]].Orowsetl[0 + etab[eid].td[i].start], etab[eid].td[i].Osize,
                (SQLLEN *)&etab[ldrs[j]].Orowsetl[0 + etab[eid].td[i].start + etab[eid].td[i].Osize + etab[eid].td[i].pad]))) {
                Oerr(eid, tid, __LINE__, thps[etab[ldrs[j]].id].Os, SQL_HANDLE_STMT);
                goto oloadJson_exit;
            }
        }
        if (!SQL_SUCCEEDED(Or = SQLPrepare(thps[etab[ldrs[j]].id].Os, Oins, SQL_NTS))) {
            Oerr(eid, tid, __LINE__, thps[etab[ldrs[j]].id].Os, SQL_HANDLE_STMT);
            goto oloadJson_exit;
        }
        etab[ldrs[j]].sp = l;   /* save number of target table fields for prec */
    }

    /* Register Load Start Time */
    gettimeofday(&tve, (void *)NULL);
    tinit = 1000 * (tve.tv_sec - tvi.tv_sec) + (tve.tv_usec - tvi.tv_usec) / 1000;

    /* Json load */
    pstats = 1;    /* from now on print stats on exit */
    while (pJsonReader->errorCode == JSON_SUCCESS) {
        // read start
        if (readState == 0) {
            while (pJsonReader->errorCode == JSON_SUCCESS) {
                if (pJsonReader->state == JSON_STATE_MEMBER_KEY) {
                    jsonReadKey(pJsonReader, keybuf, sizeof(keybuf));
                    if (pJsonReader->errorCode == JSON_SUCCESS && !strmicmp((char *)etab[eid].td[k].Oname, keybuf, strlen(keybuf))) {
                        readState = 1;
                        break;
                    }
                }
                else {
                    jsonRead(pJsonReader);
                }
            }
        }
        else {
            // read rows
            while (pJsonReader->errorCode == JSON_SUCCESS) {
                if (pJsonReader->state == JSON_STATE_MEMBER_KEY) {
                    jsonReadKey(pJsonReader, keybuf, sizeof(keybuf));
                }
                else if (pJsonReader->state == JSON_STATE_MEMBER_VALUE) {
                    jsonReadMemberValue(pJsonReader, valuebuf, etab[eid].buffsz);
                    ifl = strlen(valuebuf);

                    for (k = 0; k < l; k++) {
                        if (!strmicmp((char *)etab[eid].td[k].Oname, keybuf, strlen(keybuf))) {    /* name matches */
                            Odp = etab[eid].Orowsetl + m*etab[eid].s + etab[eid].td[k].start;
                            if (ifl > etab[eid].td[k].Osize) { // prevent Orowsetl[] overflow
                                fprintf(stderr, "odb [OloadJson(%d)] - Error: row %lu col %u field truncation. Input "
                                    "string: >%s< of length %lu.\n", __LINE__, nrf + 1, k + 1, valuebuf, ifl);
                                goto oloadJson_exit;
                            }
                            MEMCPY(Odp, valuebuf, ifl);
                            Odp += etab[eid].td[k].Osize + etab[eid].td[k].pad;
                            *((SQLLEN *)(Odp)) = (SQLLEN)(ifl);
                            break;
                        }
                    }
                }
                else if (pJsonReader->state == JSON_STATE_OBJECT_FINISH) {
                    ++m;
                    jsonRead(pJsonReader);
                }
                else if (pJsonReader->state == JSON_STATE_ARRAY_FINISH) {
                    readState = 0;
                    break;
                }
                else {
                    jsonRead(pJsonReader);
                }
            }
        }

        if (m == etab[eid].r) {                           /* Insert rowset */
            nt++;
            while (go) {
                if (etab[eid].ps) {                       /* Find a buffer loader thread available */
                    MutexLock(&etab[eid].pmutex);
                    while (go) {
                        for (i = 0; i < nldr; i++) {    /* Look for write_available buffers */
                            if (etab[ldrs[i]].lstat == 0) {
                                etab[ldrs[i]].lstat = 2;    /* write_busy */
                                break;
                            }
                        }
                        if (etab[eid].flg & 04000) {      /* Oloadbuff set error flag */
                            break;
                        }
                        else if (i >= nldr) {           /* nothing available... wait */
                            nw++;
                            CondWait(&etab[eid].pcvp, &etab[eid].pmutex);
                        }
                        else {
                            break;
                        }
                    }
                    MutexUnlock(&etab[eid].pmutex);
                    if (etab[eid].flg & 04000)            /* Oloadbuff set error flag */
                        break;
                    memcpy(etab[ldrs[i]].Orowsetl, etab[eid].Orowsetl, m * etab[eid].s);
                    etab[ldrs[i]].nbs = nrf - (unsigned long)m;    /* pass the input file base row for this rowset */
                    etab[ldrs[i]].lstat = 1;                /* read_available */
                    etab[ldrs[i]].ar = m;
                    CondWakeAll(&etab[eid].pcvc);
                }
                else {
                    etab[ldrs[0]].nbs = nrf - (unsigned long)m;    /* pass the input file base row for this rowset */
                    etab[ldrs[0]].ar = m;
                    Oloadbuff(eid);
                }
                break;
            }
            if (etab[eid].flg & 04000)                    /* Oloadbuff set error flag */
                break;
            m = 0;
            k = 0;
        }
    }

    if (pJsonReader->errorCode != JSON_ERROR_PARSE_EOF) {
        fprintf(stderr, "odb [OloadJson(%d)] - Error parse json file encountered error:%s\n", __LINE__, jsonReaderErrorMessage(pJsonReader));
    }

    jsonReaderFree(pJsonReader);
    /* load trailing rows */
    if (m) {                          /* Insert rowset */
        nt++;
        while (go) {
            if (etab[eid].ps) {                       /* Find a buffer loader thread available */
                MutexLock(&etab[eid].pmutex);
                while (go) {
                    for (i = 0; i < nldr; i++) {    /* Look for write_available buffers */
                        if (etab[ldrs[i]].lstat == 0) {
                            etab[ldrs[i]].lstat = 2;    /* write_busy */
                            break;
                        }
                    }
                    if (etab[eid].flg & 04000) {      /* Oloadbuff set error flag */
                        break;
                    }
                    else if (i >= nldr) {           /* nothing available... wait */
                        nw++;
                        CondWait(&etab[eid].pcvp, &etab[eid].pmutex);
                    }
                    else {
                        break;
                    }
                }
                MutexUnlock(&etab[eid].pmutex);
                if (etab[eid].flg & 04000)            /* Oloadbuff set error flag */
                    break;
                memcpy(etab[ldrs[i]].Orowsetl, etab[eid].Orowsetl, m * etab[eid].s);
                etab[ldrs[i]].nbs = nrf - (unsigned long)m;    /* pass the input file base row for this rowset */
                etab[ldrs[i]].lstat = 1;                /* read_available */
                etab[ldrs[i]].ar = m;
                CondWakeAll(&etab[eid].pcvc);
            }
            else {
                etab[ldrs[0]].nbs = nrf - (unsigned long)m;    /* pass the input file base row for this rowset */
                etab[ldrs[0]].ar = m;
                Oloadbuff(eid);
            }
            break;
        }
    }
oloadJson_exit:
    etab[eid].flg |= 02000;                 /* mark EOF */
    if (etab[eid].ps) {                   /* wait all loaders to complete */
        CondWakeAll(&etab[eid].pcvc);
        MutexLock(&etab[eid].pmutex);
        while (go) {
            for (i = 0, k = 0; i < nldr; i++) {
                k += etab[ldrs[i]].lstat;
            }
            if (k != nldr * 10)
                CondWait(&etab[eid].pcvp, &etab[eid].pmutex);
            else
                break;
        }
        MutexUnlock(&etab[eid].pmutex);
    }
    telaps -= 1000 * tve.tv_sec + tve.tv_usec / 1000;
    gettimeofday(&tve, (void *)NULL);       /* register end time */
    telaps += 1000 * tve.tv_sec + tve.tv_usec / 1000;

    for (i = 0; i < nldr; i++)
        etab[eid].nrt += etab[ldrs[i]].nr;

    /* Print results */
    if (pstats) {
        fprintf(stderr, "[%d] %s Load(X) statistics:\n\t[%d] Target table: %s.%s.%s\n",
            tid, odbid, tid, etab[eid].Ocso[0], etab[eid].Ocso[1], etab[eid].Ocso[2]);
        fprintf(stderr, "\t[%d] Source: %s\n", tid, etab[eid].src);
        seconds = (double)tinit / 1000.0;
        fprintf(stderr, "\t[%d] Pre-loading time: %s s (%s)\n", tid,
            strmnum(seconds, num, sizeof(num), 3), strmtime(seconds, tim));
        seconds = (double)telaps / 1000.0;
        fprintf(stderr, "\t[%d] Loading time: %s s(%s)\n", tid,
            strmnum(seconds, num, sizeof(num), 3), strmtime(seconds, tim));
        fprintf(stderr, "\t[%d] Total records read: %s\n", tid, strmnum((double)nrf, num, sizeof(num), 0));
        fprintf(stderr, "\t[%d] Total records inserted: %s\n", tid, strmnum((double)etab[eid].nrt, num, sizeof(num), 0));
        fprintf(stderr, "\t[%d] Total number of columns: %s\n", tid, strmnum((double)l, num, sizeof(num), 0));
        fprintf(stderr, "\t[%d] Average input row size: %s B\n", tid, strmnum((double)nb*1.0 / nrf, num, sizeof(num), 1));
        fprintf(stderr, "\t[%d] ODBC row size: %s B (data)", tid, strmnum((double)(etab[eid].s - l * sizeof(SQLLEN)), num, sizeof(num), 0));
        fprintf(stderr, " + %s B (len ind)\n", strmnum((double)(l * sizeof(SQLLEN)), num, sizeof(num), 0));
        fprintf(stderr, "\t[%d] Rowset size: %s\n", tid, strmnum((double)etab[eid].r, num, sizeof(num), 0));
        fprintf(stderr, "\t[%d] Rowset buffer size: %s KiB\n", tid, strmnum((double)etab[eid].s / 1024.0*etab[eid].r, num, sizeof(num), 2));
        fprintf(stderr, "\t[%d] Load throughput (real data): %s KiB/s\n", tid, strmnum((double)nb / 1.024 / telaps, num, sizeof(num), 3));
        fprintf(stderr, "\t[%d] Load throughput (ODBC): %s KiB/s\n", tid,
            strmnum((double)((etab[eid].s - l * sizeof(SQLLEN))*etab[eid].nrt) / 1.024 / telaps, num, sizeof(num), 3));
        if (etab[eid].ps) {
            fprintf(stderr, "\t[%d] Reader Total/Wait Cycles: %s", tid, strmnum((double)nt, num, sizeof(num), 0));
            fprintf(stderr, "/%s\n", strmnum((double)nw, num, sizeof(num), 0));
        }
    }

    /* Unbind parameters (if parallel unbind is executed in Oloadbuff) */
    if (!etab[eid].ps)
        (void)SQLFreeStmt(thps[tid].Os, SQL_RESET_PARAMS);

    /* Free Memory */
    if (Odel)
        free(Odel);
    if (Oins)
        free(Oins);
    if (ldrs)     /* Free ldrs */
        free(ldrs);
    if (etab[eid].Orowsetl)       /* Free Orowsetl if allocated */
        free(etab[eid].Orowsetl);
    if (etab[eid].Ostatusl)       /* Free Ostatusl if allocated */
        free(etab[eid].Ostatusl);
    if (etab[eid].td) {
        for (i = 0; i < l; i++)
            free(etab[eid].td[i].Oname);
        free(etab[eid].td);
    }

    /* Close bad file */
    if (etab[eid].fo != stderr)
        fclose(etab[eid].fo);

    /* Run "post" SQL */
    if (etab[eid].post) {
        var_set(&thps[tid].tva, VTYPE_I, "tgt", etab[eid].tgt);
        etab[eid].flg2 |= 020000000;           /* Oexec to allocate/use its own handle */
        if (etab[eid].post[0] == '@')         /* run a sql script */
            (void)runsql(tid, eid, 0, (etab[eid].post + 1));
        else                                    /* Run single SQL command */
            (void)Oexec(tid, eid, 0, 0, (SQLCHAR *)etab[eid].post, "");
        etab[eid].flg2 &= ~020000000;          /* reset Oexec to allocate/use its own handle */
    }
    return;
}

/* Oloadbuff:
 *      load buffer pointed by etab[eid].Orowsetl 
 *
 *      eid (I): etab entry ID
 *  
 *      return: 0=OK, 1=Fetch Error, 2=Load Error
 */
static int Oloadbuff(int eid)
{
    size_t embs=ERR_MSG_LEN;     /* Error message buffer size */
    SQLRETURN Or=0;             /* ODBC return value */
    SQLCHAR Ostate[6];          /* ODBC state */
    SQLSMALLINT Oi=1;           /* ODBC error index */
    SQLSMALLINT Oln=0;          /* ODBC error message length */
    SQLLEN Onrows=0;            /* ODBC no of affected rows */
    SQLCHAR *Otxt=0;            /* ODBC error message buffer pointer */
    SQLINTEGER Onative=0;       /* ODBC native erroro code */
    char num[32];               /* Formatted Number String */
    int tid = etab[eid].id;     /* Thread ID */
    int par = etab[eid].parent; /* Parent shortcut */
    int gpar = etab[eid].k;     /* grand parent eid */
    struct timeval tve;         /* timeval struct to define elapesd/timelines */
    unsigned int umx = 0;       /* Local use mutexes flag: 0=no, 1=yes */
    unsigned int pmk = 0;       /* Local print mark flag: 0=no, 1=yes (simple), 2=yes (timeline) */
    char type = etab[eid].type; /* Local thread type */
    unsigned int i = 0;         /* loop variable */
    SQLLEN Orown = 0;           /* Row number with errors in rowset */
    long telaps = 0;            /* Register elapsed time */
    struct timeval tvel;        /* timeval struct to define elapesd/timelines */
    int mtype = 0;              /* olbmex message index */
    unsigned long lnr = etab[eid].nr ;  /* Local copy of etab[eid].nr */
    unsigned int lcr = etab[eid].cr ;   /* Local copy of etab[eid].cr */
    Mutex *parmutex = &etab[par].pmutex;/* Local copy of parent mutex address */
    int fexst = 0 ;             /* Function Exit Status: 0=OK, 1=Fetch Error, 2=Load Error */
#ifdef ODB_PROFILE
    struct timespec tsp1, tsp2 ;/* Profiling function */
    unsigned long ti = 0 ;      /* Insert profiled time in nanoseconds */
    unsigned long ts = 0 ;      /* Thread synch profiled time in nanoseconds */
#endif

    if ( etab[eid].fo == stdout )
        etab[eid].fo = stderr ;
    switch ( type ) {
    case 'L':
        mtype = 0;
        gpar = par;
        break;
    case 'C':
    case 'P':
        if ( type == 'C' )
            mtype = 1;
        else
            mtype = 2;
        umx = 1;
        /* Run mpre-SQL on target (tmpre) */
        if ( etab[eid].mpre ) {
            etab[eid].flg2 |= 020000000 ;           /* Oexec to allocate/use its own handle */
            if ( etab[eid].mpre[0] == '@' ) {       /* run a sql script */
                etab[eid].type = 'f';               /* set execution type to file during runsql() */
                i = runsql(tid, eid, 0, (etab[eid].mpre + 1 ));
            } else {                                /* Run single SQL command */
                etab[eid].type = 'x';               /* set execution type to file during runsql() */
                i = Oexec(tid, eid, 0, 0, (SQLCHAR *)etab[eid].mpre, "");
            }
            etab[eid].flg2 &= ~020000000 ;          /* reset Oexec to allocate/use its own handle */
            if ( i && etab[eid].flg & 0004 ) {
                fprintf(stderr, "odb [Oloadbuff(%d)] - Error running mpre-SQL on tgt. Exiting because Stop On Error is set\n", 
                    __LINE__);
                etab[gpar].flg |= 04000;            /* set error flag */
                return(fexst);
            }
            etab[eid].nr = 0;                       /* reset number of resulting rows */
            etab[eid].type = type;                  /* reset thread type */
        }
        break;
    }
    if ( etab[eid].ps )
        umx = 1;
    if ( etab[eid].flg & 0400 )
        pmk = 1;
    if ( etab[eid].flg2 & 020000 )
        pmk = 2;

    /* Initial memory allocation for the error message buffer */
    if ( ( Otxt = malloc(embs) ) == (void *)NULL ) {
        fprintf(stderr, "[%d] odb [loadbuff(%d)] - Error allocating err msg buffers: [%d] %s\n",
            tid, __LINE__, errno, strerror(errno));
        return(fexst);
    }

    /* Main loop */
    while ( go ) {
        if ( umx ) {
#ifdef ODB_PROFILE
            clock_gettime(CLOCK_MONOTONIC, &tsp1);
#endif
            MutexLock( parmutex );
            while ( go ) {
                if ( etab[eid].lstat == 1 ) {
                    etab[eid].lstat = 3;                /* read_busy */
                    break ;
                } else if ( etab[par].flg & 02000 &&    /* EOF */
                            etab[eid].lstat != 10 ) {   /* Job not restarted */
                    goto oloadbuff_exit ;               /* no more data */
                } else {
                    CondWait(&etab[par].pcvc, parmutex);
                }
            }
            MutexUnlock(parmutex);
#ifdef ODB_PROFILE
            clock_gettime(CLOCK_MONOTONIC, &tsp2);
            ts += tspdiff ( &tsp1 , &tsp2 ) ;
#endif
        }

        if ( etab[eid].ar < etab[eid].r )       /* Reset Rowset size */ 
            if (!SQL_SUCCEEDED(Or=SQLSetStmtAttr(thps[tid].Os,
                SQL_ATTR_PARAMSET_SIZE, (SQLPOINTER)(etab[eid].ar), 0)))
                Oerr(eid, tid, __LINE__, thps[tid].Os, SQL_HANDLE_STMT);
#ifdef ODB_PROFILE
        clock_gettime(CLOCK_MONOTONIC, &tsp1);
#endif
        Or = SQLExecute(thps[tid].Os) ;         /* Execute INSERT (load/copy) or tgt command */
        SQLLEN tLastRow = -1;           /* remember last bad row to ensure that a bad row will be printed only once. */
#ifdef ODB_PROFILE
        clock_gettime(CLOCK_MONOTONIC, &tsp2);
        ti += tspdiff ( &tsp1 , &tsp2 ) ;
#endif
        switch ( Or ) {                         /* Evaluate SQLExecute Result */
        case SQL_SUCCESS:
            lnr += (unsigned long)etab[eid].Oresl;
            if ( etab[par].cmt )
                lcr += (unsigned long)etab[eid].Oresl;
            break;
        default:
            Oi = 1; //the record number need to be initialize for next loop
            /* Loop through the ODBC error stack for this statement handle. */
            while ( ( Or = SQLGetDiagRec(SQL_HANDLE_STMT, thps[tid].Os, Oi, Ostate, &Onative, Otxt,
                (SQLSMALLINT)embs, &Oln) ) != SQL_NO_DATA ) {
                /* Error management process is made of three steps:
                    1. print the error messages and - when possible - the 'wrong' row/record
                       close to the error message and preceeded by ">>> " so you can
                       easily sed/grep "bad rows". This depends on ODBC driver
                       SQL_DIAG_ROW_NUMBER availability
                    2. understand how many rows have been loaded using SQLRowCount if possible
                       - SQL_PARAM_ARRAY_ROW_COUNTS = SQL_PARC_NO_BATCH - or browing the
                       status array
                    3. manage max-error conditions. Please note, from 'odb point of view'
                       errors are rows or records that were not loaded or copied. For example
                       if you have a 100 rows rowset and row nomber 13 is not loaded because
                       of a PK violation, the ODBC error stack contains a single error due to
                       the PK violation. Now, depending on ODBC driver bevaviour:
                       - if the driver inserts the remaining 99 rows (if valid) odb will 
                       increase error count by 1 (because only one row was not loaded)
                       - if the driver loads the 12 records "before" the error and then
                       stops when it found the PK violation, odb will increase the error
                       counter by 88 because only the first 12 rows out of 100 have been
                       loaded.
                       - if the driver other driver will discard the whole rowset odb will
                       increase the error counter by 100 */
                /* STEP 1: we have to understand which row this error message belongs to */ 
                if ( etab[eid].r == 1 ) {   
                    /* this is easy: rowset is made of a single row... */
                    fprintf(stderr, "[%d] odb [Oloadbuff(%d)] - Error loading row %lu (State: %s, Native %ld)\n%s\n",
                        tid, __LINE__, etab[eid].nbs + 1, (char *)Ostate, (long)Onative, (char *)Otxt);
                    if ( type == 'C' ) {        /* 'C' thread */
                        if ( etab[eid].fdmp ) { /* dump ODBC buffer */
                            MutexLock(&etab[gpar].pmutex);
                            fprintf(etab[eid].fdmp, "[%d] odb [Oloadbuff(%d)] - Error loading row %lu (State: %s, Native %ld)\n%s\n",
                                tid, __LINE__, etab[eid].nbs + 1, (char *)Ostate, (long)Onative, (char *)Otxt);
                            fprintf(etab[eid].fdmp, "[%d] Dumping failing row. ODBC row length = %zu\n", tid, etab[par].s ) ;
                            dumpbuff(etab[eid].fdmp, tid, (unsigned char *)etab[eid].Orowsetl, etab[par].s , 0 );
                            MutexUnlock(&etab[gpar].pmutex);
                        }
                    } else {                /* either multi ('L') or single ('l') threaded loaders */
                        prec('L', (unsigned char *)etab[eid].Orowsetl, eid);
                    }
                } else if ( SQL_SUCCEEDED(Or=SQLGetDiagField(SQL_HANDLE_STMT, thps[tid].Os, Oi, SQL_DIAG_ROW_NUMBER,
                        (SQLPOINTER)&Orown, 0, NULL))) {
                    if ( (size_t)Oln > embs ) {         /* error message buffer was too small */
                        embs = (size_t) ( Oln + 1 );    /* increase buffer size */
                        if ( ( Otxt = realloc ( Otxt, embs ) ) == (void *)NULL )    /* Re-alloc message buffer */
                            fprintf(stderr, "[%d] odb [Oloadbuff(%d)] - Error re-allocating err msg buff: [%d] %s\n",
                                tid, __LINE__, errno, strerror(errno));
                    }
                    switch ( (int)Orown ) {
                    case SQL_NO_ROW_NUMBER:         /* error not associated to specific row number */
                    case SQL_ROW_NUMBER_UNKNOWN:    /* driver doesn't know row number for this error */
                         /* Just print the error message but no failing data */
                        fprintf(stderr, "[%d] odb [Oloadbuff(%d)] - Error (State: %s, Native %ld)\n%s\n",
                            tid, __LINE__, (char *)Ostate, (long)Onative, (char *)Otxt);
                        if ( type == 'C' ) {        /* 'C' thread */
                            if ( etab[eid].fdmp ) { /* dump ODBC buffer */
                                MutexLock(&etab[gpar].pmutex);
                                fprintf(etab[eid].fdmp, "[%d] odb [Oloadbuff(%d)] - Error (State: %s, Native %ld)\n%s\n",
                                    tid, __LINE__, (char *)Ostate, (long)Onative, (char *)Otxt);
                                fprintf(etab[eid].fdmp, "[%d] Dumping whole rowset of %zu rows. ODBC row length = %zu\n",
                                    tid, etab[eid].ar, etab[par].s ) ;
                                dumpbuff(etab[eid].fdmp, tid, (unsigned char *)etab[eid].Orowsetl, etab[eid].ar * etab[par].s , 0 );
                                MutexUnlock(&etab[gpar].pmutex);
                            }
                        }
                        break;
                    default:
                        /* Ok, now we have an error message (Otxt), a five char SQLState (Ostate), 
                         * a native error code (Onative) and the rowset row number (Orown). Let's
                         * print everything to stderr. */
                        fprintf(stderr, "[%d] odb [Oloadbuff(%d)] - Error loading row %lu (State: %s, Native %ld)\n%s\n",
                            tid, __LINE__, (unsigned long)Orown + etab[eid].nbs, (char *)Ostate, (long)Onative, (char *)Otxt);
                        if (type == 'C') {        /* 'C' thread */
                            if (etab[eid].fdmp) { /* dump ODBC buffer */
                                MutexLock(&etab[gpar].pmutex);
                                fprintf(etab[eid].fdmp, "[%d] odb [Oloadbuff(%d)] - Error loading row %lu (State: %s, Native %ld)\n%s\n",
                                    tid, __LINE__, (unsigned long)Orown + etab[eid].nbs, (char *)Ostate, (long)Onative, (char *)Otxt);
                                fprintf(etab[eid].fdmp, "[%d] Dumping row %lu in a block of %zu rows. ODBC row length = %zu\n",
                                    tid, (unsigned long)Orown, etab[eid].ar, etab[par].s);
                                dumpbuff(etab[eid].fdmp, tid, (unsigned char *)(etab[eid].Orowsetl + (Orown - 1) * etab[par].s), etab[par].s, 0);
                                MutexUnlock(&etab[gpar].pmutex);
                            }
                        }
                        if (tLastRow != Orown) { /* ensure no duplicated rows in bad file */
                            char type = etab[eid].type;
                            tLastRow = Orown;
                            if (type == 'l') type = 'L';
                            prec(type, (unsigned char *)(etab[eid].Orowsetl + etab[par].s*(Orown - 1)), eid);
                        }
                        break;
                    }
                } else {
                    fprintf(stderr, "[%d] odb [Oloadbuff(%d)] - Unable to get SQL_DIAG_ROW_NUMBER. Printing the whole ODBC error stack\n",
                        tid, __LINE__);
                    Oerr(eid, tid, __LINE__, thps[tid].Os, SQL_HANDLE_STMT);
                    break;
                }
                Oi++;
            }
            /* STEP 2: determine how many rows have been loaded */
            if ( etab[eid].flg2 & 01000 ) {     /* tgt SQL_PARAM_ARRAY_ROW_COUNTS set to SQL_PARC_NO_BATCH */
                if (!SQL_SUCCEEDED(Or=SQLRowCount(thps[tid].Os, &Onrows))) {
                    fprintf(stderr, "[%d] odb [Oloadbuff(%d)] - Unable to get row counts from driver\n", tid, __LINE__);
                    Oerr(eid, tid, __LINE__, thps[tid].Os, SQL_HANDLE_STMT);
                }
            } else if ( etab[eid].dbt == ORACLE ) {
                /* Oracle does not insert anything if one of the rows in the rowset fails.
                 * Unfortunately it still sets all status array elements to SQL_SUCCESS !! */
                Onrows = 0;
            } else {
                /* etab[eid].Oresl contains the number of 'processed' rows.
                 * However the number of 'processed' rows could be different from
                 * the number of 'inserted' rows. Here we analyze the status array
                 * to determine how many 'processed' rows have been 'inserted'. */
                for ( i = 0, Onrows = 0 ; i < (unsigned int) etab[eid].Oresl ; i++ ) {
                    switch ( etab[eid].Ostatusl[i] ) {
                    case SQL_PARAM_SUCCESS_WITH_INFO:
                    case SQL_PARAM_SUCCESS:
                        Onrows++;
                        break;
                    }
                }
            }
            /* STEP 3 - Finally we have to manage max error conditions.
             * I do consider as 'errors' any 'non-loaded/copied' rows. */
            if ( etab[gpar].mer ) {
                MutexLock(&etab[gpar].pmutex);
                etab[gpar].mer -= (int) ( etab[eid].ar - (size_t)Onrows ) ;
                if ( etab[gpar].mer <= 0 ) {
                    etab[gpar].flg |= 04000 ;
                    if ( f & 020000 )   /* if verbose */
                        fprintf(stderr, "[%d] odb [Oloadbuff(%d)] - Max number of error reached\n",
                            tid, __LINE__);
                }
                MutexUnlock(&etab[gpar].pmutex);
            }
            lnr += Onrows < 0 ? 0 : (unsigned long)Onrows;
            if ( ( etab[eid].flg & 0004 ) || etab[eid].roe  ) {     /* Stop or Restart on Error */
                etab[gpar].flg |= 04000;
                fexst = 2 ;     /* Function Exit Status = Load Error */
            }
            break;
        }
#ifndef _WIN32
        pthread_testcancel();
#endif
        if ( etab[par].cmt > 0 && lcr >= (unsigned int)etab[par].cmt ) {
            if (!SQL_SUCCEEDED(Or=SQLEndTran(SQL_HANDLE_DBC, thps[tid].Oc, SQL_COMMIT)))
                Oerr(eid, tid, __LINE__, thps[tid].Oc, SQL_HANDLE_DBC);
            lcr = 0;
        }
        switch ( pmk ) {
        case 1:             /* simple mark */
            fprintf(stderr, "[%d] %s records %s %s\n",
                tid, strmnum((double)lnr,num,sizeof(num),0), olbmex[mtype], lcr ? "" : "[commit]");
            break;
        case 2:             /* timeline mark */
            gettimeofday(&tvel, (void *)NULL);  /* register end time */
            telaps = 1000*(tvel.tv_sec-tvi.tv_sec)+(tvel.tv_usec-tvi.tv_usec)/1000;
            fprintf(stderr, "[%d] %s records %s %s (%ld ms)\n",
                tid, strmnum((double)lnr,num,sizeof(num),0), olbmex[mtype], lcr ? "" : "[commit]", telaps);
            break;
        }
        if ( fdmp ) {                           /* dump ODBC buffer */
            MutexLock(&etab[gpar].pmutex);
            fprintf(fdmp, "[%d] Block of %zu rows. Row length = %zu\n", tid, etab[eid].ar, etab[eid].s ) ;
            dumpbuff(fdmp, tid, (unsigned char *)etab[eid].Orowsetl, etab[eid].ar * etab[par].s, 1 );
            MutexUnlock(&etab[gpar].pmutex);
        }
        if ( umx ) {
#ifdef ODB_PROFILE
            clock_gettime(CLOCK_MONOTONIC, &tsp1);
#endif
            MutexLock(parmutex);
            etab[eid].lstat = 0;                /* write_available */
            MutexUnlock(parmutex);
            CondWake( &etab[par].pcvp );
#ifdef ODB_PROFILE
            clock_gettime(CLOCK_MONOTONIC, &tsp2);
            ts += tspdiff ( &tsp1 , &tsp2 ) ;
#endif
        } else {
            break;
        }
    }
    oloadbuff_exit:
#ifdef ODB_PROFILE
    fprintf(stderr, "[%d] (odb profile Oloadbuff) Insert time (ns): %s\n", tid, strmnum((double)ti,num,sizeof(num),0));
    fprintf(stderr, "[%d] (odb profile Oloadbuff) Thread synch time (ns): %s\n", tid, strmnum((double)ts,num,sizeof(num),0));
#endif
    etab[eid].nr = lnr ;                        /* save lnr */
    if ( Otxt )
        free ( Otxt ) ;
    if ( etab[par].cmt ) {                      /* Rolling back */
        if (!SQL_SUCCEEDED(SQLEndTran(SQL_HANDLE_DBC, thps[tid].Oc, SQL_COMMIT)))
            Oerr(eid, tid, __LINE__, thps[tid].Oc, SQL_HANDLE_DBC);
    }
    if ( umx ) {
        (void)SQLFreeStmt(thps[tid].Os, SQL_RESET_PARAMS);
        etab[eid].lstat = 10;                   /* Set lstat available */
        MutexUnlock(parmutex);
        CondWake( &etab[par].pcvp );
        gettimeofday(&tve, (void *)NULL);       /* save thread-end time */
        etab[eid].nrt = 1000*tve.tv_sec + tve.tv_usec/1000 - etab[par].nbs ;
        if ( etab[eid].Orowsetl )               /* Free Olrowsetl if allocated */
            free(etab[eid].Orowsetl);   
        if ( etab[eid].Ostatusl )               /* Free Olstatusl if allocated */
            free(etab[eid].Ostatusl);
    }
    if ( fexst ) {
        return ( fexst ) ;
    }
    if ( ( type == 'C' || type == 'P' ) && etab[gpar].post ) {
        MutexLock(&etab[gpar].pmutex);
        etab[gpar].nltotal--;                            /* decrease number of active loaders */
        MutexUnlock(&etab[gpar].pmutex);
        if ( etab[gpar].nltotal == 0 ) {                 /* no more active loaders: run post-SQL */
            var_set ( &thps[tid].tva, VTYPE_I, "tgt", etab[eid].tgt );
            etab[eid].flg2 |= 020000000 ;           /* Oexec to allocate/use its own handle */
            if ( etab[eid].post[0] == '@' )         /* run a sql script */
                (void)runsql(tid, eid, 0, (etab[gpar].post + 1 ));
            else                                    /* Run single SQL command */
                (void)Oexec(tid, eid, 0, 0, (SQLCHAR *)etab[gpar].post, "");
            etab[eid].flg2 &= ~020000000 ;          /* reset Oexec to allocate/use its own handle */
        }
    }
    return (fexst);
}

/* Oextract:
 *      extract to output file
 *
 *      eid (I): etab entry ID to run
 *
 *      return: void
 */
static void Oextract(int eid)
{
    int tid = etab[eid].id;     /* Thread ID */
    char num[32];               /* to build formatted numbers */
    char tim[15];               /* formatted time string */
    char *xbuff = 0;            /* buffer allocated for XML output */
    size_t xbuffl = 33 + MAXOBJ_LEN * 3 ;   /* xmlbuff length */
    SQLCHAR *Ocmd = 0;          /* ODBC Command buffer */
    SQLCHAR *O = 0;             /* ODBC temp variable for realloc */
    SQLRETURN Or=0;             /* ODBC return value */
    size_t cl=CMD_CHUNK;        /* ODBC Command Length */
    int par = etab[eid].parent; /* Parent shortcut */
    int ptid = etab[par].id;    /* Parent thead ID */
    int i = 0;                  /* loop variable */
    int cmdl = 0;               /* Ocmd length */
    int gpar = etab[eid].k;     /* grand parent eid */
    unsigned long b = 0;        /* total number of bytes written by "pool" */
    unsigned long nr = 0;       /* total number of recs written by "pool" */
    double seconds = 0;         /* seconds used for timings */

    /* Allocate XML buffer */
    if ( etab[eid].flg2 & 04000000 ) {  /* initialize XML output buffer */
        if ( ( xbuff = malloc ( xbuffl ) ) == (void *) NULL ) {
            fprintf(stderr, "odb [Oextract(%d)] - Error allocating xbuff memory\n", __LINE__);
            etab[gpar].lstat |= 0010 ;  /* mark pre-xml error */    
            return;
        }
    }

    /* Allocate Ocmd memory */
    cl += etab[eid].sql ? strlen(etab[eid].sql) : 0;
    cl += etab[eid].src ? strlen(etab[eid].src) : 0;
    cl += etab[eid].map ? strlen(etab[eid].map) : 0;
    cl += etab[eid].cols ? strlen(etab[eid].cols) : 0;
    if ( ( Ocmd = malloc ( cl ) ) == (void *) NULL ) {
        fprintf(stderr, "odb [Ocopy(%d)] - Error allocating Ocmd memory\n", __LINE__);
        return ;
    }
    Ocmd[0] = '\0' ;    /* ready for strmcat */

    /* First thread to run "pre-extract" work - others to wait */
    MutexLock(&etab[gpar].gmutex);  /* lock shared mutex */
    if ( ( ( etab[gpar].lstat & 0002 ) &&       /* pre-SQL ran by another thread with errors and... */
           ( etab[eid].flg & 0004 ) )  ||       /* ... soe is set or... */  
         ( etab[gpar].lstat & 0010 ) ) {        /* ... XML init errors */
        MutexUnlock(&etab[gpar].gmutex);        /* unlock mutex */
        return;
    }
    if ( etab[gpar].lstat & 0001 ) {    /* XML output to initialize */
        xbuffl = snprintf(xbuff, xbuffl, "<?xml version=\"1.0\"?>\n<%s>",
            etab[eid].src ? etab[eid].src : "select" );
#ifdef HDFS
        if ( etab[eid].fho ) {
            (*hdfswrite)(hfs, etab[eid].fho, (void *)xbuff, xbuffl);
        } else if ( etab[eid].fo ) {
            (void)fwrite ( xbuff, 1, xbuffl, etab[eid].fo );
        }
#else
        (void)fwrite ( xbuff, 1, xbuffl, etab[eid].fo );
#endif
        etab[gpar].lstat &= ~0001 ;     /* mark XML initialization done */
    }
    if ( etab[gpar].lstat & 0004 ) {    /* pre SQL to run */
        etab[eid].fso = etab[eid].fo;   /* save output file */
        etab[eid].fo = stdout;          /* set output to stdout */
        var_set ( &thps[tid].tva, VTYPE_I, "src", etab[eid].src );
        etab[eid].flg2 |= 020000000 ;       /* Oexec to allocate/use its own handle */
        if ( etab[eid].pre[0] == '@' ) {    /* run a sql script */
            etab[eid].type = 'f';           /* set execution type to file during runsql() */
            i = runsql(tid, eid, 0, (etab[eid].pre + 1 ));
        } else {                                /* Run single SQL command */
            etab[eid].type = 'x';           /* set execution type to run single SQL */
            i = Oexec(tid, eid, 0, 0, (SQLCHAR *)etab[eid].pre, "");
        }
        etab[eid].flg2 &= ~020000000 ;          /* reset Oexec to allocate/use its own handle */
        etab[gpar].lstat &= ~0004;      /* mark pre-SQL done */
        if ( i ) {
            etab[gpar].lstat |= 0002 ;  /* mark pre-sql error */    
            if ( etab[eid].flg & 0004 ) {
                fprintf(stderr, "odb [Oextract(%d)] - Error running pre-SQL. Exiting because Stop On Error is set\n", 
                    __LINE__);
                MutexUnlock(&etab[gpar].gmutex);/* unlock mutex */
                return;
            }
        }
        etab[eid].type = 'e';           /* reset execution type */
        etab[eid].fo = etab[eid].fso;   /* reset output file */
        etab[eid].fso = (FILE *)NULL;   /* reset spool file */
        etab[eid].nr = 0;               /* reset number of resulting rows */
    }
    MutexUnlock(&etab[gpar].gmutex);/* unlock mutex */

    /* Every extraction thread to run "mpre" SQL script */
    if ( etab[eid].mpre ) {
        etab[eid].fso = etab[eid].fo;   /* save output file */
        etab[eid].fo = stdout;          /* set output to stdout */
        etab[eid].flg2 |= 020000000 ;       /* Oexec to allocate/use its own handle */
        if ( etab[eid].mpre[0] == '@' ) {   /* run a sql script */
            etab[eid].type = 'f';           /* set execution type to file during runsql() */
            i = runsql(tid, eid, 0, (etab[eid].mpre + 1 ));
        } else {                            /* Run single SQL command */
            etab[eid].type = 'x';           /* set execution type to run single SQL */
            i = Oexec(tid, eid, 0, 0, (SQLCHAR *)etab[eid].mpre, "");
        }
        etab[eid].flg2 &= ~020000000 ;      /* reset Oexec to allocate/use its own handle */
        if ( i && etab[eid].flg & 0004 ) {
            fprintf(stderr, "odb [Oextract(%d)] - Error running mpre-SQL. Exiting because Stop On Error is set\n", 
                __LINE__);
            return;
        }
        etab[eid].type = 'e';           /* reset execution type */
        etab[eid].fo = etab[eid].fso;   /* reset output file */
        etab[eid].fso = (FILE *)NULL;   /* reset spool file */
        etab[eid].nr = 0;               /* reset number of resulting rows */
    }

    /* Set connection attribute Read Only */
    if (!SQL_SUCCEEDED(Or=SQLSetConnectAttr(thps[tid].Oc,
            SQL_ATTR_ACCESS_MODE, (SQLPOINTER)SQL_MODE_READ_ONLY, SQL_IS_UINTEGER))) {
        Oerr(eid, tid, __LINE__, thps[tid].Oc, SQL_HANDLE_DBC);
        return;
    }

    /* Set max char/varchar/binary column length */
    if (!SQL_SUCCEEDED(Or=SQLSetStmtAttr(thps[tid].Os, SQL_ATTR_MAX_LENGTH,
            (SQLPOINTER)etab[eid].Omaxl, SQL_IS_UINTEGER))) {
        Oerr(eid, tid, __LINE__, thps[tid].Os, SQL_HANDLE_STMT);
    }

    /* Build & run extract statement */
    if ( etab[eid].sql ) { 
        if ( etab[eid].flg2 & 040000 ) {    /* custom parallel SQL */
            /* Set initial Ocmd */
            strmcpy((char *)Ocmd, etab[eid].sql, cl);
            /* Set tds and cds variables */
            (void)snprintf(num, sizeof(num), "%u", etab[eid].ps);
            var_set ( &thps[tid].tva, VTYPE_I, "tds", num );
            (void)snprintf(num, sizeof(num), "%ld", etab[eid].sbmin);
            var_set ( &thps[tid].tva, VTYPE_I, "cds", num );
            /* Expand Ocmd */
            Ocmd = (SQLCHAR *)var_exp((char *)Ocmd, &cl, &thps[tid].tva);
            if ( f & 020000 )   /* if verbose */
                fprintf(stderr, "[%d] odb [Oextract(%d)] - EXTRACT statement: %s\n",
                    tid, __LINE__, (char *)Ocmd);
            Oexec ( tid, eid, 0, 0, Ocmd, "");
        } else {
            Oexec ( tid, eid, 0, 0, (SQLCHAR *)etab[eid].sql, "");
        }
    } else {
        cmdl = strmcat((char *)Ocmd, "SELECT ", cl, 0);
        if ( etab[eid].flg & 01406000000 ) {    /* cast and/or trim: we have to analyze source table */
            for ( i = 0 ; i < etab[eid].cmt ; i++ ) {
                if ( i )
                    cmdl += strmcat((char *)Ocmd, ",", cl, 0);
                switch ( etab[eid].td[i].Otype ) {
                case SQL_CHAR:
                case SQL_WCHAR:
                    if ( etab[eid].flg & 0400000000 ) { /* trim */
                        cmdl += strmcat ( (char *) Ocmd, "TRIM(", cl, 0);
                        cmdl += strmcat ( (char *) Ocmd, (char *) etab[eid].td[i].Oname, cl, 0);
                        cmdl += strmcat ( (char *) Ocmd, ")", cl, 0);
                        break;
                    } else if ( etab[eid].flg & 02000000 ) { /* rtrim */
                        cmdl += strmcat ( (char *) Ocmd, "RTRIM(", cl, 0);
                        cmdl += strmcat ( (char *) Ocmd, (char *) etab[eid].td[i].Oname, cl, 0);
                        cmdl += strmcat ( (char *) Ocmd, ")", cl, 0);
                        break;
                    }
                    /* FALLTHRU */
                case SQL_VARCHAR:
                case SQL_WVARCHAR:
                case SQL_LONGVARCHAR:
                case SQL_WLONGVARCHAR:
                    if ( etab[eid].flg2 & 040000000 ) { /* trim+ */
                        cmdl += strmcat ( (char *) Ocmd, "TRIM(", cl, 0);
                        cmdl += strmcat ( (char *) Ocmd, (char *) etab[eid].td[i].Oname, cl, 0);
                        cmdl += strmcat ( (char *) Ocmd, ")", cl, 0);
                    } else if ( etab[eid].flg & 04000000 ) { /* rtrim+ */
                        cmdl += strmcat ( (char *) Ocmd, "RTRIM(", cl, 0);
                        cmdl += strmcat ( (char *) Ocmd, (char *) etab[eid].td[i].Oname, cl, 0);
                        cmdl += strmcat ( (char *) Ocmd, ")", cl, 0);
                    } else {
                        cmdl += strmcat ( (char *) Ocmd, (char *) etab[eid].td[i].Oname, cl, 0);
                    }
                    break;
                default:
                    if ( etab[eid].flg & 01000000000 ) { /* cast */
                        cmdl += snprintf((char *)(Ocmd + cmdl), cl, "CAST(%s AS VARCHAR(%d))",
                            (char *)etab[eid].td[i].Oname, (int)etab[eid].td[i].Osize);
                    } else {
                        cmdl += strmcat ( (char *) Ocmd, (char *) etab[eid].td[i].Oname, cl, 0);
                    }
                }
                if ( cl - cmdl < CMD_CHUNK ) {
                    cl += CMD_CHUNK;
                    O = Ocmd ;
                    if ( ( O = (SQLCHAR *)realloc ( O, cl ) ) == (void *)NULL ) {
                        fprintf(stderr, "odb [Oextract(%d)] - Error re-allocating memory for Ocmd: [%d] %s\n",
                            __LINE__, errno, strerror(errno));
                        return;
                    }
                    Ocmd = O ;
                }
            }
        } else if ( etab[eid].cols ) {
            cmdl += strmcat((char *)Ocmd, etab[eid].cols, cl, 0);
        } else {
            cmdl += strmcat((char *)Ocmd, "*", cl, 0);
        }
        if ( etab[eid].ps ) {
            if ( etab[eid].sb ) {   /* split by */
                snprintf((char *)(Ocmd +cmdl), cl,
                    " FROM %s WHERE %s >= %ld AND %s < %ld %s%s %s",
                    etab[eid].src, etab[eid].sb, etab[eid].sbmin, etab[eid].sb, etab[eid].sbmax,
                    etab[eid].map ? "AND " : "", etab[eid].map ? etab[eid].map : "",
                    etab[eid].flg & 0001 ? dbscmds[etab[eid].dbt].uncomm : "");
            } else if ( etab[eid].dbt == ORACLE ) {
                snprintf((char *)(Ocmd +cmdl), cl,
                    " FROM %s WHERE MOD(ORA_HASH(ROWID), %u) = %ld%s%s",
                    etab[eid].src, etab[eid].ps, etab[eid].sbmin,
                    etab[eid].map ? " AND " : "", etab[eid].map ? etab[eid].map : "");
            } else if ( etab[eid].dbt == SQLSERVER ) {
                snprintf((char *)(Ocmd +cmdl), cl,
                    /* The following will provide a better distribution but is (much) slower:
                       " FROM %s WHERE ABS(HASHBYTES('MD5',%%%%PHYSLOC%%%%) %% %u) = %ld%s%s" */
                    " FROM %s WHERE (%%%%PHYSLOC%%%% / 147) %% %u = %ld%s%s",
                    etab[eid].src, etab[eid].ps, etab[eid].sbmin,
                    etab[eid].map ? " AND " : "", etab[eid].map ? etab[eid].map : "");
            } else {
                if (!SQL_SUCCEEDED(SQLExecDirect (thps[tid].Os,
                        (SQLCHAR *)"CONTROL QUERY DEFAULT DBTR_PROCESS 'ON'", SQL_NTS))) {
                    Oerr(eid, tid, __LINE__, thps[tid].Os, SQL_HANDLE_STMT);
                    return;
                }
                snprintf((char *)(Ocmd + cmdl), cl,
                    " FROM TABLE(TABLE %s, PARTITION NUMBER FROM %u TO %u) %s%s %s",
                    etab[eid].src, etab[eid].sp, etab[eid].ep,
                    etab[eid].map ? "WHERE " : "", etab[eid].map ? etab[eid].map : "",
                    etab[eid].flg & 0001 ? dbscmds[etab[eid].dbt].uncomm : " ");
            }
        } else {
            snprintf((char *)(Ocmd + cmdl), cl, " FROM %s %s%s %s", etab[eid].src,
                etab[eid].map ? "WHERE " : "", etab[eid].map ? etab[eid].map : "",
                etab[eid].flg & 0001 ? dbscmds[etab[eid].dbt].uncomm : " ");
        }
        if ( f & 020000 )   /* if verbose */
            fprintf(stderr, "[%d] odb [Oextract(%d)] - EXTRACT statement: %s\n",
                tid, __LINE__, (char *)Ocmd);
        Oexec ( tid, eid, 0, 0, Ocmd, "");
    }

    /* Close output file & print stats */
    if ( (etab[eid].flg & 01000) ) {    /* multi-parted output: each thread closes its file */
        if ( etab[eid].fo && etab[eid].fo != stdout ) 
            fclose ( etab[eid].fo );
#ifdef HDFS
        else if ( etab[eid].fho )
            (*hdfsclose)(hfs, etab[eid].fho);
#endif
    }

    /* decrease "still active" parallel streams counter */
    MutexLock( &etab[par].gmutex ); 
    if ( etab[par].ps ) { 
        etab[par].ps--;
        if ( etab[par].ps )             /* Not the last thread */
            goto oextract_exit;
    }
    if ( !etab[par].ps ) {              /* Last thread */
        MutexUnlock( &etab[par].gmutex );
        /* sum bytes & rows written */
        for ( i = 0 ; i < no ; i++ ) {
            if ( etab[i].parent == par ) {
                b += etab[i].nbs;
                nr += etab[i].nr;
            }
        }

        if ( etab[eid].flg2 & 04000000 ) {  /* initialize XML output buffer */
            xbuffl = snprintf(xbuff, xbuffl, "\n</%s>\n",
                etab[eid].src ? etab[eid].src : "select" );
#ifdef HDFS
            if ( etab[eid].fho ) {
                (*hdfswrite)(hfs, etab[eid].fho, (void *)xbuff, xbuffl);
            } else {
                if ( etab[eid].fo ) {
                    (void)fwrite ( xbuff, 1, xbuffl, etab[eid].fo );
                }
            }
#else
            (void)fwrite ( xbuff, 1, xbuffl, etab[eid].fo );
#endif
        }

        /* close single outut file */
        if (!(etab[eid].flg & 01000)) {   /* non multiparted output */
#ifdef HDFS
            if (etab[eid].fho)
                (*hdfsclose)(hfs, etab[eid].fho);
#endif
            if (etab[eid].fo && etab[eid].fo != stdout)
                fclose(etab[eid].fo);
        }

        /* Print stats */
        fprintf(stderr,"[%d] %s Extract statistics:\n", ptid, odbid);
        if ( etab[eid].flg2 & 0100000 )     /* print custom SQL file name */
            fprintf(stderr,"\t[%d] Source (SQL file): %s\n", ptid, etab[eid].cols);
        else if ( etab[eid].flg2 & 040000 ) /* print custom SQL file name */
            fprintf(stderr,"\t[%d] Source (SQL): %s\n", ptid, etab[eid].sql);
        else                                /* print input table name */
            fprintf(stderr,"\t[%d] Source: %s\n", ptid, etab[eid].src);
        if ( etab[eid].flg2 & 01000000000 ) {
            fprintf(stderr,"\t[%d] Target: [MAPR] %s\n", ptid, etab[par].tgt);
        } else if ( etab[eid].flg2 & 0100 ) {
            fprintf(stderr,"\t[%d] Target: [HDFS] %s\n", ptid, etab[par].tgt);
        } else {
            fprintf(stderr,"\t[%d] Target: %s\n", ptid, etab[par].tgt);
        }
        fprintf(stderr,"\t[%d] Record buffer size: %s bytes\n", ptid, strmnum((double)etab[par].k,num,sizeof(num),0));
        fprintf(stderr,"\t[%d] Rowset size: %s\n", ptid, strmnum((double)etab[par].r,num,sizeof(num),0));
        fprintf(stderr,"\t[%d] Rowset buffer size: %s KiB\n", ptid, strmnum((double)etab[par].k/1024.0*etab[par].r,num,sizeof(num),2));
        seconds = (double)etab[eid].mr/1000.0 ;
        fprintf(stderr,"\t[%d] Pre-extract time: %s s (%s)\n", ptid,
            strmnum(seconds,num,sizeof(num),3), strmtime(seconds, tim));
        seconds = (double)etab[eid].nrt/1000.0 ;
        fprintf(stderr,"\t[%d] Extract time: %s s (%s)\n", ptid,
            strmnum(seconds,num,sizeof(num),3), strmtime(seconds, tim));
        fprintf(stderr,"\t[%d] Total records extracted: %s ", ptid, strmnum((double)nr,num,sizeof(num),0));
            fprintf(stderr,"(%s krec/s)\n", strmnum((double)nr*1.0/etab[eid].nrt,num,sizeof(num),3));
        fprintf(stderr,"\t[%d] Total data bytes written: %s ", ptid, strmnum((double)b,num,sizeof(num),0));
            fprintf(stderr,"(%s KiB/s)\n", strmnum((double)b/1.024/etab[eid].nrt,num,sizeof(num),3));
        if ( i > 1 ) {  /* more than one extract stream... */
            for ( i = 0 ; i < no ; i++ ) {
                if ( etab[i].parent == par ) {
                    fprintf(stderr,"\t\t[%d] %s records extracted", i, strmnum((double)etab[i].nr,num,sizeof(num),0));
                    fprintf(stderr," (%s bytes)", strmnum((double)etab[i].nbs,num,sizeof(num),0));
                    seconds = (double)etab[i].nrt/1000.0 ;
                    fprintf(stderr," in %s s (%s)\n",
                        strmnum(seconds,num,sizeof(num),3), strmtime(seconds, tim));
                }
            }
        }
        if ( etab[eid].td ) {   /* last thread frees table desc structure memory */
            for ( i = 0 ; i < etab[eid].cmt ; i++ )
                free ( etab[eid].td[i].Oname );
            free(etab[eid].td);
        }
        if ( etab[eid].post ) { /* run post-SQL script */
            etab[eid].type = 'f';   /* set execution type to file during runsql() */
            etab[eid].fo = stdout;  /* set output to stdout */
            var_set ( &thps[tid].tva, VTYPE_I, "src", etab[eid].src );
            etab[eid].flg2 |= 020000000 ;           /* Oexec to allocate/use its own handle */
            if ( etab[eid].post[0] == '@' )         /* run a sql script */
                (void)runsql(tid, eid, 0, (etab[eid].post + 1 ));
            else                                    /* Run single SQL command */
                (void)Oexec(tid, eid, 0, 0, (SQLCHAR *)etab[eid].post, "");
            etab[eid].flg2 &= ~020000000 ;          /* reset Oexec to allocate/use its own handle */
        }
        if ( etab[eid].flg2 & 0040 )
            free ( etab[eid].cols ) ;
        if ( etab[eid].flg2 & 0100000 )
            free ( etab[eid].sql ) ;
    }
    oextract_exit:
    MutexUnlock( &etab[par].gmutex );

    /* Free memory */
    if ( Ocmd )
        free ( Ocmd );
    if ( xbuff )
        free ( xbuff ) ;
}

/* Ocopy:
 *      copy between ODBC connections
 *
 *      eid (I): etab entry ID to run
 *
 *      return: 0=OK, 1=Fetch Error, 2=Load Error
 */
static int Ocopy(int eid)
{
    SQLCHAR *Ocmd = 0;      /* ODBC Command buffer */
    SQLCHAR *O = 0;         /* ODBC temp variable for realloc */
    SQLCHAR *Osq = 0;       /* ODBC first row sequence buffer address */
    SQLRETURN Or=0;         /* ODBC return value */
    size_t cl=CMD_CHUNK;    /* ODBC Command Length */
    size_t rbl = 0;         /* rowset buffer length */
    size_t sqo = 0;         /* sequence offset */
    int j = 0,              /* loop variable */
        k = 0,              /* loop variable */
        i = 0,              /* loop variable */
        np = 0;             /* Number of parameters */
    long max = (long) etab[eid].mr ;    /* max record to copy flag */
    unsigned long nw=0,     /* no of wait cycles */
                  nt=0;     /* No of total write cycles */
    int tid = etab[eid].id; /* Thread ID */
    int echild = etab[eid].child;   /* first child process eid */ 
    int tchild = etab[echild].id;   /* first child process tid */ 
    int nchild = (int)etab[eid].nloader; /* number of children: each 'c'/'p' (extraction) thread
                            has 'nchild' children 'C'/'P' (loading) threads having etab[] 
                            index from 'echild' to 'echild + nchild'*/
    int gpar = etab[eid].k;     /* grand parent eid */
    int gchild = gpar+1;        /* grand parent first child eid */
    int gptid = etab[gpar].id;  /* grand parent tid */
    SQLSMALLINT Oncol;      /* ODBC number of columns in the result set */
    unsigned int ucs = 0;   /* Local ucs2toutf8 conversion flag */
    SQLCHAR *Op = 0 ;       /* Orowsetl buffer pointer used during UCS-2/UTF-8 conversion */
    SQLCHAR Oname[MAXOBJ_LEN];  /* ODBC Column Name */
    SQLUSMALLINT Otf = 0 ;  /* ODBC target field number */
    struct timeval tve;     /* timeval struct to define elapesd/timelines */
    long telaps = 0,        /* Elapsed time in ms */
         tinit = 0;         /* Initialization time in ms */
    char num[32];           /* formatted numbers */
    char tim[15];           /* formatted time string */
    char type = etab[eid].type ;    /* save thread type */
    SQLULEN Orespl = 0;     /* ODBC ulen to store # of fetched rows */
    unsigned long nr = 0;   /* total number of records copied by pool */
    SQLHSTMT Ostmt=thps[tid].Os;    /* Local ODBC Statement handle */
    double seconds = 0;     /* seconds used for timings */
    static int tdbteid = 0; /* EID with target dbt */
    uint64_t lseq = 0 ;     /* local sequence value */
    unsigned long ul = 0 ;  /* sequence loop variable */
    int fexst = 0 ;         /* function exist status: 0=OK, 1=Fetch Error, 2=Load Error */
#ifdef ODB_PROFILE
    struct timespec tsp1, tsp2 ;    /* Profiling function */
    unsigned long tf = 0 ;          /* Fetch profiled time in nanoseconds */
    unsigned long ts = 0 ;          /* Thread synch time in nanoseconds */
    unsigned long tm = 0 ;          /* memcpy profiled time in nanoseconds */
#endif

    /* Allocate Ocmd memory */
    cl += etab[eid].sql ? strlen(etab[eid].sql) : 0;
    cl += etab[eid].src ? strlen(etab[eid].src) : 0;
    cl += etab[eid].map ? strlen(etab[eid].map) : 0;
    cl += etab[eid].cols ? strlen(etab[eid].cols) : 0;
    if ( ( Ocmd = malloc ( cl ) ) == (void *) NULL ) {
        fprintf(stderr, "odb [Ocopy(%d)] - Error allocating Ocmd memory\n", __LINE__);
        goto ocopy_exit;
    }

    if ( etab[eid].flg & 010000000 )        /* Convert ucs2toutf8 */
        ucs = 1 ;

    /* First thread to run: truncate/ifempty check/pre-SQL on target (using child connection) */
    MutexLock(&etab[gpar].gmutex);  /* lock shared mutex */
    if ( etab[gpar].lstat == 4 ) {  /* pre-copy work to do */
        if ( tdbteid ) {            /* target dbt was already found */
            etab[gchild].dbt = etab[tdbteid].dbt ;
        } else {
            etab[gchild].dbt = checkdb ( echild, &thps[tchild].Oc, NULL, NULL );    /* check tgt DB type */
            tdbteid = gchild ;
        }
        if ( ( etab[eid].flg2 & 0200000 ) &&                        /* if bind=auto */
             ( ( etab[eid].dbt != etab[gchild].dbt )  ||            /* different SRC/TGT */
               ( etab[eid].dbt == 0 && etab[gchild].dbt == 0 ) ) )  /* or both SRC & TGT are generic */
                etab[gpar].flg2 |= 04000 ;                          /* set gpar bind=char */
        if ( !etab[eid].tgtsql )
            var_set ( &thps[tchild].tva, VTYPE_I, "tgt", etab[eid].run );
        if ( etab[eid].pre ) {                  /* run pre SQL */
            etab[echild].flg2 |= 020000000 ;    /* Oexec to allocate/use its own handle */
            if ( etab[eid].pre[0] == '@' ) {    /* run a sql script */
                etab[eid].type = 'f';           /* set execution type to file during runsql() */
                j = runsql(tchild, echild, 0, (etab[eid].pre + 1 ));
            } else {                            /* Run single SQL command */
                etab[eid].type = 'x';           /* set execution type to file during runsql() */
                j = Oexec(tchild, echild, 0, 0, (SQLCHAR *)etab[eid].pre, "");
            }
            etab[echild].flg2 &= ~020000000;    /* reset Oexec to allocate/use its own handle */
            etab[echild].nr = 0;                /* reset number of resulting rows */
            etab[eid].type = type ;
            etab[gpar].lstat = j ? 5 : 0;       /* set "SQL" done (with or without errors)*/
            if ( j && etab[eid].flg & 0004 ) {
                fprintf(stderr, "odb [Ocopy(%d)] - Error running pre-SQL. Exiting because Stop On Error has set\n", 
                    __LINE__);
                goto ocopy_exit;
            }
        }
        if ( etab[eid].flg & 0002 ) {           /* truncate target table */
            strmcpy((char *)Ocmd, dbscmds[etab[gchild].dbt].trunc, cl);
            Ocmd = (SQLCHAR *)var_exp((char *)Ocmd, &cl, &thps[tchild].tva);
            if ( f & 020000 )                   /* if verbose */
                fprintf(stderr, "odb: Now truncating target table (%s)\n", (char *)Ocmd);
            etab[echild].flg2 |= 020000000 ;            /* Oexec to allocate/use its own handle */
            j = Oexec(tchild, echild, 0, 0, Ocmd, "");  /* Run Truncate */
            etab[echild].flg2 &= ~020000000 ;           /* reset Oexec to allocate/use its own handle */
            if ( j && etab[eid].flg & 0004 ) {
                fprintf(stderr, "odb [Ocopy(%d)] - Error truncating Target table. Exiting because Stop On Error was set\n", 
                    __LINE__);
                goto ocopy_exit;
            }
            if ( etab[echild].cmt ) {           /* Autocommit off: have to commit truncate */
                if ( f & 020000 )                   /* if verbose */
                    fprintf(stderr, "odb: Autocommit off. Now committing truncate target table (%s)\n", (char *)Ocmd);
                if (!SQL_SUCCEEDED(Or=SQLEndTran(SQL_HANDLE_DBC, thps[tchild].Oc, SQL_COMMIT))) {
                    Oerr(echild, tchild, __LINE__, thps[tchild].Oc, SQL_HANDLE_DBC);
                    goto ocopy_exit;
                }
            }
            etab[echild].nr = 0;            /* reset number of resulting rows */
            etab[gpar].lstat = 0;           /* set "truncate" done */
        } 
        if ( etab[eid].flg & 0200000 ) {    /* check ifempty */
            j = (int)ifempty(echild, etab[eid].run);
            if ( j ) {
                etab[gpar].lstat = j ? 6 : 0;   /* set "ifempty" done (with or without errors)*/
                fprintf(stderr, "odb [Ocopy(%d)] - Target table %s is not empty\n",
                        __LINE__, etab[eid].run);
                etab[gpar].post = 0;        /* do not run post SQL */
                goto ocopy_exit;
            }
        }
        etab[gpar].lstat = 0 ;          /* set lstat write available */
    }
    etab[eid].lstat = 0;                /* set "SQL" done (with or without errors)*/
    MutexUnlock(&etab[gpar].gmutex);    /* unlock shared mutex */
    if ( etab[gpar].lstat == 6 ||                               /* tgt not empty & ifempty is set */
         ( etab[gpar].lstat == 5 && etab[eid].flg & 0004 ) )    /* pre-SQL errors & soe */
        goto ocopy_exit;

    /* Run mpre-SQL on source */
    if ( etab[eid].mpre ) {
        etab[eid].flg2 |= 020000000 ;       /* Oexec to allocate/use its own handle */
        if ( etab[eid].mpre[0] == '@' ) {   /* run a sql script */
            etab[eid].type = 'f';           /* set execution type to file during runsql() */
            j = runsql(tid, eid, 0, (etab[eid].mpre + 1 ));
        } else{                             /* Run single SQL command */
            etab[eid].type = 'x';           /* set execution type to file during runsql() */
            j = Oexec(tid, eid, 0, 0, (SQLCHAR *)etab[eid].mpre, "");
        }
        etab[eid].flg2 &= ~020000000 ;      /* reset Oexec to allocate/use its own handle */
        if ( j && etab[eid].flg & 0004 ) {
            fprintf(stderr, "odb [Ocopy(%d)] - Error running mpre-SQL. Exiting because Stop On Error is set\n", 
                __LINE__);
            goto ocopy_exit;
        }
        etab[echild].nr = 0;                /* reset number of resulting rows */
        etab[eid].type = type ;
    }

    /* Set connection attribute Read Only (on source) */
    if (!SQL_SUCCEEDED(Or=SQLSetConnectAttr(thps[tid].Oc,
            SQL_ATTR_ACCESS_MODE, (SQLPOINTER)SQL_MODE_READ_ONLY, SQL_IS_UINTEGER))) {
        Oerr(eid, tid, __LINE__, thps[tid].Oc, SQL_HANDLE_DBC);
        return(fexst);
    }

    /* Build Extract Command */
    if ( etab[eid].ps > 1 ) {
        if ( etab[eid].flg2 & 040000 ) {    /* custom parallel SQL */
            /* Set initial Ocmd */
            strmcpy((char *)Ocmd, etab[eid].sql, cl);
            /* Set tds and cds variables */
            (void)snprintf(num, sizeof(num), "%u", etab[echild].ps);
            /* I'm using echild here above because gpar could be decremented by other threads
               before being used by thread with eid=gpar */
            var_set ( &thps[tid].tva, VTYPE_I, "tds", num );
            (void)snprintf(num, sizeof(num), "%ld", etab[eid].sbmin);
            var_set ( &thps[tid].tva, VTYPE_I, "cds", num );
            /* Expand Ocmd */
            Ocmd = (SQLCHAR *)var_exp((char *)Ocmd, &cl, &thps[tid].tva);
        } else if ( etab[eid].sb ) {
            snprintf((char *) Ocmd, cl,
                "SELECT %s FROM %s WHERE %s >= %ld AND %s < %ld %s%s %s",
                etab[eid].cols ? etab[eid].cols : "*",
                etab[eid].src, etab[eid].sb, etab[eid].sbmin, etab[eid].sb, etab[eid].sbmax,
                etab[eid].map ? "AND " : "", etab[eid].map ? etab[eid].map : "",
                etab[eid].flg & 0001 ? dbscmds[etab[eid].dbt].uncomm : "");
        } else if ( etab[eid].dbt == ORACLE ) {
            snprintf((char *) Ocmd, cl,
                "SELECT %s FROM %s WHERE MOD(ORA_HASH(ROWID), %u) = %ld%s%s",
                etab[eid].cols ? etab[eid].cols : "*",
                etab[eid].src, etab[eid].ps, etab[eid].sbmin,
                etab[eid].map ? " AND " : "", etab[eid].map ? etab[eid].map : "");
        } else if ( etab[eid].dbt == SQLSERVER ) {
            snprintf((char *)Ocmd, cl,
                /* the following will provide a better distribution but is (much) slower:
                 "SELECT %s FROM %s WHERE ABS(HASHBYTES('MD5',%%%%PHYSLOC%%%%) %% %u) = %ld%s%s" */
                "SELECT %s FROM %s WHERE (%%%%PHYSLOC%%%% / 147) %% %u = %ld%s%s",
                etab[eid].cols ? etab[eid].cols : "*",
                etab[eid].src, etab[eid].ps, etab[eid].sbmin,
                etab[eid].map ? " AND " : "", etab[eid].map ? etab[eid].map : "");
        } else {
            if (!SQL_SUCCEEDED(Or=SQLExecDirect (Ostmt,
                    (SQLCHAR *)"CONTROL QUERY DEFAULT DBTR_PROCESS 'ON'", SQL_NTS))) {
                Oerr(eid, tid, __LINE__, Ostmt, SQL_HANDLE_STMT);
                goto ocopy_exit;
            }
            snprintf((char *)Ocmd, cl, 
                "SELECT %s FROM TABLE(TABLE %s, PARTITION NUMBER FROM %u TO %u) %s%s %s",
                etab[eid].cols ? etab[eid].cols : "*",
                etab[eid].src, etab[eid].sp, etab[eid].ep,
                etab[eid].map ? "WHERE " : "", etab[eid].map ? etab[eid].map : "",
                etab[eid].flg & 0001 ? dbscmds[etab[eid].dbt].uncomm : "");
        }
    } else if ( etab[eid].sql ) {           /* no parallel custom SQL */
        strmcpy((char *)Ocmd, etab[eid].sql, cl);
    } else {                                /* no parallel SQL copy */
        snprintf((char *)Ocmd, cl, "SELECT %s FROM %s%s%s %s", 
                etab[eid].cols ? etab[eid].cols : "*", etab[eid].src,
                etab[eid].map ? " WHERE " : "", etab[eid].map ? etab[eid].map : "",
                etab[eid].flg & 0001 ? dbscmds[etab[eid].dbt].uncomm : "");
    }
    if ( f & 020000 )   /* if verbose */
        fprintf(stderr, "[%d] odb [Ocopy(%d)] - SOURCE statement: %s\n",
            tid, __LINE__, (char *)Ocmd);

    /* Prepare command */
    if ((Or=SQLPrepare(Ostmt, Ocmd, SQL_NTS)) != SQL_SUCCESS) {
        Oerr(eid, tid, __LINE__, Ostmt, SQL_HANDLE_STMT);
        if ( Or != SQL_SUCCESS_WITH_INFO )
            goto ocopy_exit;
    }

    /* Execute command */
    if (!SQL_SUCCEEDED(Or=SQLExecute(Ostmt))) {
        Oerr(eid, tid, __LINE__, Ostmt, SQL_HANDLE_STMT);
        goto ocopy_exit;
    }

    /* Find number of returned columns */
    if (!SQL_SUCCEEDED(Or=SQLNumResultCols(Ostmt, &Oncol))) {
        Oerr(eid, tid, __LINE__, Ostmt, SQL_HANDLE_STMT);
        goto ocopy_exit;
    }
    etab[eid].sp = (unsigned int)Oncol; /* save number of src/tgt table fields for prec */

    /* Allocate memory for the tdesc array */
    if ( (etab[eid].td = (struct tdesc *)calloc ((size_t)Oncol, sizeof(struct tdesc))) == (void *)NULL ) {
            fprintf(stderr, "odb [Ocopy(%d)] - Error allocating tdesc array memory: [%d] %s\n",
                __LINE__, errno, strerror(errno));
        goto ocopy_exit;
    }   

    /* Get Result Set columns descriptions */
    for (j = 0; j < Oncol; j++) {
        if ( !SQL_SUCCEEDED(Or=SQLDescribeCol(Ostmt, (SQLUSMALLINT)(j+1),
                NULL, 0, &etab[eid].td[j].OnameLen, &etab[eid].td[j].Otype, &etab[eid].td[j].Osize,
                &etab[eid].td[j].Odec, &etab[eid].td[j].Onull))) {
            Oerr(eid, tid, __LINE__, Ostmt, SQL_HANDLE_STMT);
            goto ocopy_exit;
        }

        if ((etab[eid].td[j].Oname = (SQLCHAR*)malloc(etab[eid].td[j].OnameLen + 1)) == (void *)NULL) {
            fprintf(stderr, "odb [Ocopy(%d)] - Error allocating etab[%d].td[%d].Oname array memory: [%d] %s\n",
                __LINE__, eid, j, errno, strerror(errno));
            goto ocopy_exit;
        }

        if (!SQL_SUCCEEDED(Or = SQLColAttribute(Ostmt, (SQLUSMALLINT)(j + 1), SQL_DESC_NAME, etab[eid].td[j].Oname,
                etab[eid].td[j].OnameLen + 1, &etab[eid].td[j].OnameLen, NULL))) {
            Oerr(eid, tid, __LINE__, Ostmt, SQL_HANDLE_STMT);
            goto ocopy_exit;
        }

        if ( etab[gpar].flg2 & 04000 ) {        /* Force Char Binding */
            if(!SQL_SUCCEEDED(Or=SQLColAttribute(Ostmt, (SQLUSMALLINT)(j+1), SQL_DESC_DISPLAY_SIZE,
                (SQLPOINTER) NULL, (SQLSMALLINT) 0, (SQLSMALLINT *) NULL, (SQLPOINTER)&etab[eid].td[j].OdisplaySize))) {
                Oerr(eid, tid, __LINE__, Ostmt, SQL_HANDLE_STMT);
                goto ocopy_exit;
            }
            if ( etab[eid].dbt != VERTICA ) { /* Vertica's CHAR field length is in bytes (not chars) */
                if ( etab[eid].dbt == ORACLE && etab[eid].td[j].Otype == SQL_TYPE_TIMESTAMP )
                /* The precision of Oracle's Timestamp is always 9 */
                {
                    etab[eid].td[j].OdisplaySize = SQL_TIMESTAMP_LEN + 10; /* Display Size of TIMESTAMP(9) */
                }
                switch (etab[eid].td[j].Otype) {
                case SQL_WCHAR:
                case SQL_WVARCHAR:
                case SQL_WLONGVARCHAR:
                    etab[eid].td[j].OdisplaySize *= etab[eid].bpwc;   /* Space for UTF-8 conversion */
                    break;
                case SQL_CHAR:
                case SQL_VARCHAR:
                case SQL_LONGVARCHAR:
                    etab[eid].td[j].OdisplaySize *= etab[eid].bpc;
                    break;
                }
            }
            etab[eid].td[j].Octype = SQL_C_CHAR ;          /* Bind to char */
        } else {    /* Try to use "C Default" data types to minimize conversions and reduce buffer size */
            switch ( etab[eid].td[j].Otype ) {
            case SQL_INTEGER:
            case SQL_SMALLINT:
            case SQL_BIGINT:
                etab[eid].td[j].Osize = sizeof (SQLBIGINT) ;
                etab[eid].td[j].Octype = SQL_C_SBIGINT ;
                break;
            case SQL_FLOAT:
            case SQL_DOUBLE:
                etab[eid].td[j].Osize = sizeof (SQLDOUBLE) ;
                etab[eid].td[j].Octype = SQL_C_DOUBLE ;
                break;
            case SQL_TYPE_DATE:
                etab[eid].td[j].Osize = sizeof (SQL_DATE_STRUCT) ;
                etab[eid].td[j].Octype = SQL_C_TYPE_DATE ;
                break;
            case SQL_TYPE_TIMESTAMP:
                etab[eid].td[j].Osize = sizeof (SQL_TIMESTAMP_STRUCT) ;
                etab[eid].td[j].Octype = SQL_C_TYPE_TIMESTAMP ;
                break;
            case SQL_BINARY:
            case SQL_VARBINARY:
            case SQL_LONGVARBINARY:
                etab[eid].td[j].Octype = SQL_C_BINARY ;
                break;
            default:
                etab[eid].td[j].Octype = SQL_C_CHAR ;
                if(!SQL_SUCCEEDED(Or=SQLColAttribute(Ostmt, (SQLUSMALLINT)(j+1), SQL_DESC_DISPLAY_SIZE,
                    (SQLPOINTER) NULL, (SQLSMALLINT) 0, (SQLSMALLINT *) NULL, (SQLPOINTER) &etab[eid].td[j].OdisplaySize))) {
                    Oerr(eid, tid, __LINE__, Ostmt, SQL_HANDLE_STMT);
                    goto ocopy_exit;
                }
                if ( etab[eid].dbt != VERTICA ) {   /* Vertica's CHAR field length is in bytes (not chars) */
                    switch ( etab[eid].td[j].Otype ) {
                    case SQL_WCHAR:
                    case SQL_WVARCHAR:
                    case SQL_WLONGVARCHAR:
                        etab[eid].td[j].OdisplaySize *= etab[eid].bpwc;   /* Space for UTF-8 conversion */
                        break;
                    case SQL_CHAR:
                    case SQL_VARCHAR:
                    case SQL_LONGVARCHAR:
                        etab[eid].td[j].OdisplaySize *= etab[eid].bpc;
                        break;
                    }
                }
                break;
            }
        }
        ++etab[eid].td[j].OdisplaySize;   /* buffer should contain space for NULL termination in order to avoid truncation */
        #ifdef __hpux
            if (Ors[j] % WORDSZ )
                padl[j] = WORDSZ - (unsigned int)etab[eid].td[j].OdisplaySize % WORDSZ ;
        #endif
        if ( ( j + 1 ) == (int)etab[eid].seqp ) {   /* Allocate extra space for sequence and length indicator */
            sqo = etab[eid].s ; /* record sequence offset */
            etab[eid].s += (size_t) ( sizeof(SQLBIGINT) + sizeof(SQLLEN) ) ;
        }
        etab[eid].td[j].start = (int)etab[eid].s;
        etab[eid].td[j].Ocdatabufl = etab[eid].td[j].Octype == SQL_C_CHAR ? etab[eid].td[j].OdisplaySize : (SQLLEN)etab[eid].td[j].Osize;
        etab[eid].s += (size_t)(etab[eid].td[j].Ocdatabufl + etab[eid].td[j].pad + sizeof(SQLLEN));
        if ( etab[eid].flg & 0400000 && eid == gpar )   /* Grand Parent to Describe result set */
            fprintf(stderr, "--- col #%d, name=%s type=%d (%s) size=%lu, dec=%d, nullable=%s\n",
                j, (char *)Oname, etab[eid].td[j].Otype, expandtype(etab[eid].td[j].Otype), (unsigned long)etab[eid].td[j].Osize,
                (int) etab[eid].td[j].Odec, etab[eid].td[j].Onull ? "yes" : "no");
        if ( fdmp && eid == gpar )  /* Grand Parent to initialize ODBC dump */ 
            fprintf(fdmp, "[%d] Column %d: name=%s, type=%d (%s), size=%lu, decimals=%d, nullable=%s\n",
                tid, j, (char *)etab[eid].td[j].Oname, (int)etab[eid].td[j].Otype, expandtype(etab[eid].td[j].Otype),
                (unsigned long)etab[eid].td[j].Ocdatabufl, (int)etab[eid].td[j].Odec, etab[eid].td[j].Onull ? "yes" : "no" ) ;
    }

    /* Calculate rowset if buffer size is set */
    if ( etab[eid].rbs ) {
        etab[eid].r = etab[eid].rbs / etab[eid].s;
        etab[eid].r = etab[eid].r < 1 ? 1 : etab[eid].r;    /* at least one record at a time  */
        if ( etab[eid].mr && etab[eid].r > etab[eid].mr )   /* if # records to fetch < rowset ... */
            etab[eid].r = etab[eid].mr;                     /* make rowset = records to fetch */
        for ( i = echild ; i < echild + nchild ; i++ )
            etab[i].r = etab[eid].r;
    }
    /* Adjust commit if multiplier */
    if ( etab[eid].flg2 & 0001 ) {  /* commit as multiplier */
        etab[eid].cmt *= (int)etab[eid].r ;
        for ( i = echild ; i < echild + nchild ; i++ )
            etab[i].cmt = etab[eid].cmt;
    }

    /* Allocate memory for result set from source */
    if ( (etab[eid].Orowsetl = (SQLCHAR *)calloc (etab[eid].r, etab[eid].s)) == (void *)NULL ) { 
            fprintf(stderr, "odb [Ocopy(%d)] - Error allocating source rowset memory: [%d] %s\n",
                __LINE__, errno, strerror(errno));
        goto ocopy_exit;
    }

    /* Allocate memory for children rowset and status array and pass td to children */
    for ( i = echild ; i < echild + nchild ; i++ ) {
        if ( (etab[i].Orowsetl = (SQLCHAR *)calloc (etab[eid].r, etab[eid].s)) == (void *)NULL ||
            (etab[i].Ostatusl = (SQLUSMALLINT *)calloc (etab[eid].r, sizeof(SQLUSMALLINT))) == (void *)NULL ) {
                fprintf(stderr, "odb [Ocopy(%d)] - Error allocating target rowset memory for child etab[%d]: [%d] %s\n",
                    __LINE__, i, errno, strerror(errno));
            goto ocopy_exit;
        }
        etab[i].td = etab[eid].td;
        etab[i].sp = etab[eid].sp;
    }
    rbl = etab[eid].r * etab[eid].s ;

    /* Set Children Commit */
    if ( etab[eid].cmt ) {
        for ( i = echild ; i < echild + nchild ; i++ ) {
            if (!SQL_SUCCEEDED(Or=SQLSetConnectAttr(thps[etab[i].id].Oc,
                    SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_OFF, SQL_IS_UINTEGER))) {
                Oerr(i, etab[i].id, __LINE__, thps[etab[i].id].Oc, SQL_HANDLE_DBC);
            }
        }
    } else {
        for ( i = echild ; i < echild + nchild ; i++ ) {
            if (!SQL_SUCCEEDED(Or=SQLSetConnectAttr(thps[etab[i].id].Oc,
                    SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_ON, SQL_IS_UINTEGER))) {
                Oerr(i, etab[i].id, __LINE__, thps[etab[i].id].Oc, SQL_HANDLE_DBC);
            }
        }
    }

    /* Reset gpar error flag */
    etab[gpar].flg &= ~04000 ;

    /* Set row-wise binding for source */
    if (!SQL_SUCCEEDED(Or=SQLSetStmtAttr(Ostmt, SQL_ATTR_ROW_BIND_TYPE,
            (SQLPOINTER)(etab[eid].s), 0))) {
        Oerr(eid, tid, __LINE__, Ostmt, SQL_HANDLE_STMT);
        goto ocopy_exit;
    }
    if (!SQL_SUCCEEDED(Or=SQLSetStmtAttr(Ostmt, SQL_ATTR_ROW_ARRAY_SIZE,
            (SQLPOINTER)(etab[eid].r), 0))) {
        Oerr(eid, tid, __LINE__, Ostmt, SQL_HANDLE_STMT);
        goto ocopy_exit;
    }
    if (!SQL_SUCCEEDED(Or=SQLSetStmtAttr(Ostmt, SQL_ATTR_ROW_STATUS_PTR,
            NULL, 0))) {
        Oerr(eid, tid, __LINE__, Ostmt, SQL_HANDLE_STMT);
        goto ocopy_exit;
    }
    if (!SQL_SUCCEEDED(Or=SQLSetStmtAttr(Ostmt, SQL_ATTR_ROWS_FETCHED_PTR, 
            &Orespl, 0))) {
        Oerr(eid, tid, __LINE__, Ostmt, SQL_HANDLE_STMT);
        goto ocopy_exit;
    }

    /* Set row-wise binding for target */
    for ( i = echild ; i < echild + nchild ; i++ ) {
        if (!SQL_SUCCEEDED(Or=SQLSetStmtAttr(thps[etab[i].id].Os, SQL_ATTR_PARAM_BIND_TYPE,
                (SQLPOINTER)(etab[eid].s), 0))) {
            Oerr(i, etab[i].id, __LINE__, thps[etab[i].id].Os, SQL_HANDLE_STMT);
            goto ocopy_exit;
        }
        if (!SQL_SUCCEEDED(Or=SQLSetStmtAttr(thps[etab[i].id].Os, SQL_ATTR_PARAMSET_SIZE,
                (SQLPOINTER)(etab[eid].r), 0))) {
            Oerr(i, etab[i].id, __LINE__, thps[etab[i].id].Os, SQL_HANDLE_STMT);
            goto ocopy_exit;
        }
        if (!SQL_SUCCEEDED(Or=SQLSetStmtAttr(thps[etab[i].id].Os, SQL_ATTR_PARAM_STATUS_PTR,
                etab[i].Ostatusl, 0))) {
            Oerr(i, etab[i].id, __LINE__, thps[etab[i].id].Os, SQL_HANDLE_STMT);
            goto ocopy_exit;
        }
        if (!SQL_SUCCEEDED(Or=SQLSetStmtAttr(thps[etab[i].id].Os, SQL_ATTR_PARAMS_PROCESSED_PTR, 
                &etab[i].Oresl, 0))) {
            Oerr(i, etab[i].id, __LINE__, thps[etab[i].id].Os, SQL_HANDLE_STMT);
            goto ocopy_exit;
        }
    }

    /* Bind Source */
    for (j = 0; j < Oncol; j++) {
        if (!SQL_SUCCEEDED(Or=SQLBindCol(Ostmt, (SQLUSMALLINT)j + 1, etab[eid].td[j].Octype,
                &etab[eid].Orowsetl[etab[eid].td[j].start], etab[eid].td[j].Ocdatabufl,
            (SQLLEN *)&etab[eid].Orowsetl[etab[eid].td[j].start + etab[eid].td[j].Ocdatabufl + etab[eid].td[j].pad]))) {
            Oerr(eid, tid, __LINE__, Ostmt, SQL_HANDLE_STMT);
            goto ocopy_exit;
        }
    }

    /* Bind Target */
    np = etab[eid].tgtsql ? (int) etab[eid].ep : (int) Oncol;
    for (j = 0; j < np; j++) {
        k = etab[eid].tgtsql ? (int)etab[eid].tgtp[j] - 1 : j ;
        Otf = (SQLUSMALLINT)(j+1); /* SQLBindParameter target field number */
        if ( etab[eid].seqp && etab[eid].seqp <= (unsigned int)(j+1) )
            Otf++;
        for ( i = echild ; i < echild + nchild ; i++ ) {
            if (!SQL_SUCCEEDED(Or=SQLBindParameter(thps[etab[i].id].Os, Otf,
                    SQL_PARAM_INPUT, etab[eid].td[k].Octype, etab[eid].td[k].Otype, etab[eid].td[k].Osize,
                    etab[eid].td[k].Odec, &etab[i].Orowsetl[etab[eid].td[k].start], etab[eid].td[k].Ocdatabufl,
                    (SQLLEN *)&etab[i].Orowsetl[etab[eid].td[k].start + etab[eid].td[k].Ocdatabufl + etab[eid].td[k].pad]))) {
                Oerr(i, etab[i].id, __LINE__, thps[etab[i].id].Os, SQL_HANDLE_STMT);
                goto ocopy_exit;
            }
        }
    }

    /* Bind sequence to target */
    if ( etab[eid].seqp ) {
        for ( i = echild ; i < echild + nchild ; i++ ) {
            if (!SQL_SUCCEEDED(Or=SQLBindParameter(thps[etab[i].id].Os, (SQLUSMALLINT)etab[eid].seqp,
                    SQL_PARAM_INPUT, SQL_C_DEFAULT, SQL_BIGINT, (SQLULEN)sizeof(SQLBIGINT), 0,
                    &etab[i].Orowsetl[sqo], 0, (SQLLEN *)&etab[i].Orowsetl[sqo+sizeof(SQLUBIGINT)]))) {
                Oerr(i, etab[i].id, __LINE__, thps[etab[i].id].Os, SQL_HANDLE_STMT);
                goto ocopy_exit;
            }
        }
    }

    /* Build child's statement */
    if ( etab[eid].tgtsql ) {   /* custom tgt sql */
        if ( Ocmd )             /* free previously allocated Ocmd */
            free ( Ocmd ) ;
        Ocmd = (SQLCHAR *)etab[eid].tgtsql ;
    } else {
        if ( cl < (size_t)( 172 + Oncol * 2 ) ) {
            cl = 172 + Oncol * 2 ;
            O = Ocmd ;
            if ( ( O = (SQLCHAR *)realloc ( O, cl ) ) == (void *)NULL ) {
                    fprintf(stderr, "odb [Ocopy(%d)] - Error re-allocating memory for Ocmd: [%d] %s\n",
                        __LINE__, errno, strerror(errno));
                goto ocopy_exit;
            }
            Ocmd = O ;
        }
        if (!strcasecmp(etab[eid].loadcmd, "IN"))
        {
            snprintf((char *)Ocmd, cl, "INSERT %s%sINTO %s VALUES (?",
                etab[eid].flg & 040000000 ? "/*+ DIRECT */ " : "",
                etab[eid].flg2 & 0002 ? "WITH NO ROLLBACK " : "", etab[eid].run);
        }
        else if (!strcasecmp(etab[eid].loadcmd, "UP"))
        {
            snprintf((char *)Ocmd, cl, "UPSERT %s%sINTO %s VALUES (?",
                etab[eid].flg & 040000000 ? "/*+ DIRECT */ " : "",
                etab[eid].flg2 & 0002 ? "WITH NO ROLLBACK " : "", etab[eid].run);
        }
        else if (!strcasecmp(etab[eid].loadcmd, "UL"))
        {
            snprintf((char *)Ocmd, cl, "UPSERT USING LOAD %s%sINTO %s VALUES (?",
                etab[eid].flg & 040000000 ? "/*+ DIRECT */ " : "",
                etab[eid].flg2 & 0002 ? "WITH NO ROLLBACK " : "", etab[eid].run);
        }
        for (j = 1; j < Oncol; j++)
            (void) strmcat ( (char *) Ocmd, ",?", cl, 0);
        if ( etab[eid].seqp )
            (void) strmcat ( (char *) Ocmd, ",?", cl, 0);
        (void) strmcat ( (char *) Ocmd, ")", cl, 0);
    }
    if ( f & 020000 )   /* if verbose */
        fprintf(stderr, "[%d] odb [Ocopy(%d)] - TARGET statement: %s\n",
            tid, __LINE__, (char *)Ocmd);

    /* Prepare INSERT for children */
    for ( i = echild ; i < echild + nchild ; i++ ) {
        if (!SQL_SUCCEEDED(Or=SQLPrepare(thps[etab[i].id].Os, Ocmd, SQL_NTS))) {
            Oerr(i, etab[i].id, __LINE__, thps[etab[i].id].Os, SQL_HANDLE_STMT);
            goto ocopy_exit;
        }
    }

    /* Reset children statuses to write available */
    for ( i = echild ; i < echild + nchild ; i++ )
        etab[i].lstat = 0;

    /* Register Copy Start Time */
    gettimeofday(&tve, (void *)NULL);
    tinit = 1000*(tve.tv_sec-tvi.tv_sec)+(tve.tv_usec-tvi.tv_usec)/1000;
    etab[eid].nbs = (unsigned long) (1000*tve.tv_sec+tve.tv_usec/1000);

    /* Fetch data */
#ifdef ODB_PROFILE
    clock_gettime(CLOCK_MONOTONIC, &tsp1);
#endif
    while ( go &&
            SQL_SUCCEEDED(Or = SQLFetchScroll(Ostmt, SQL_FETCH_NEXT, 0))) {
#ifdef ODB_PROFILE
    clock_gettime(CLOCK_MONOTONIC, &tsp2);
    tf += tspdiff ( &tsp1 , &tsp2 ) ;
#endif
        nt++;                                   /* increase total number of cycles counter */
        while ( go ) {
#ifdef ODB_PROFILE
            clock_gettime(CLOCK_MONOTONIC, &tsp1);
#endif
            MutexLock(&etab[eid].pmutex);       /* lock shared mutex */
            while ( go ) {
                if ( etab[gpar].flg & 04000 ) { /* Oloadbuff error flag is set */
                    fexst = 2 ;                 /* Function Exit Status = Load Error */
                    break;
                }
                for ( i = echild ; i < echild + nchild ; i++ ) {
                    if ( etab[i].lstat == 0 ) { /* if loader available */
                        etab[i].lstat = 2;      /* mark loader buffer write_busy */
                        break ;
                    }
                }
                if ( i >= (echild + nchild) ) { /* no loaders available */
                    nw++ ;                      /* increase waiting cycles counter */
                    CondWait(&etab[eid].pcvp, &etab[eid].pmutex);   /* wait */
                } else {
                    break;
                }
            }
            MutexUnlock(&etab[eid].pmutex);     /* unlock mutex */
#ifdef ODB_PROFILE
            clock_gettime(CLOCK_MONOTONIC, &tsp2);
            ts += tspdiff ( &tsp1 , &tsp2 ) ;
#endif
            if ( etab[gpar].flg & 04000 ) {     /* Oloadbuff error flag is set */
                fexst = 2 ;                     /* Function Exit Status = Load Error */
                break;
            }
            if ( go == 0 ) /* In case we got a break after SQLFetch() */
                break ;
#ifdef ODB_PROFILE
            clock_gettime(CLOCK_MONOTONIC, &tsp1);
#endif
            if ( ucs ) {                            /* UCS-2 to UTF-8 conversion */
                for ( j = 0, Op = etab[eid].Orowsetl ; j < (int)Orespl ; j++, Op += etab[eid].s ) {
                    for ( k = 0 ; k < Oncol ; k++ ) {
                        switch ( etab[eid].td[k].Otype ) {
                        case SQL_WCHAR:
                        case SQL_WVARCHAR:
                        case SQL_WLONGVARCHAR:
                            if ( ucs2toutf8 ( Op + etab[eid].td[k].start ,
                                        (SQLLEN *)(Op + etab[eid].td[k].start + etab[eid].td[k].Ocdatabufl + etab[eid].td[k].pad),
                                    etab[eid].td[k].Ocdatabufl, etab[eid].bucs2 ) ) {
                                    fprintf(stderr, "odb [Ocopy(%d)] - Error converting UCS-2 column %s\n",
                                        __LINE__, (char *)etab[eid].td[k].Oname);
                            }
                            break;
                        default:
                            break;                          /* do nothing */
                        }
                    }
                }
            }
            memcpy(etab[i].Orowsetl, etab[eid].Orowsetl, rbl);
#ifdef ODB_PROFILE
            clock_gettime(CLOCK_MONOTONIC, &tsp2);
            tm += tspdiff ( &tsp1 , &tsp2 ) ;
#endif
            if ( etab[eid].seqp ) {             /* manage sequences */
                MutexLock(&dlbmutex);           /* lock global mutex */
                if ( f2 & 0002 ) {              /* if global sequence flag */
                    lseq = gseq ;               /* get current global seq value */
                    gseq += (uint64_t)Orespl ;  /* update global sequence value */
                } else {
                    lseq = etab[gpar].seq ;     /* get current sequence value */
                    etab[gpar].seq += (uint64_t)Orespl ;    /* update grand parent sequence value */
                }
                MutexUnlock(&dlbmutex);         /* unlock global mutex */
                ul = (unsigned long) Orespl ;   
                Osq = &etab[i].Orowsetl[sqo] ;  /* first row sequence address */
                for ( ; ul ; ul--, Osq += etab[eid].s ) {
                    *((SQLUBIGINT *)Osq) = lseq++ ; 
                    *((SQLLEN *)(Osq+sizeof(SQLUBIGINT))) = (SQLLEN)sizeof(SQLUBIGINT) ; 
                }
            }
#ifdef ODB_PROFILE
            clock_gettime(CLOCK_MONOTONIC, &tsp1);
#endif
            MutexLock(&etab[eid].pmutex);       /* lock shared mutex */
            etab[i].lstat = 1;                  /* mark loader buffer read_available */
            MutexUnlock(&etab[eid].pmutex);     /* lock shared mutex */
            etab[i].ar = Orespl;                /* inform loader about #rec to insert */
            CondWakeAll(&etab[eid].pcvc);       /* wake-up sleeping loader threads */
#ifdef ODB_PROFILE
            clock_gettime(CLOCK_MONOTONIC, &tsp2);
            ts += tspdiff ( &tsp1 , &tsp2 ) ;
#endif
            break;                              /* exit this loop and... */
        }
        if ( etab[gpar].flg & 04000 ) {         /* Oloadbuff error flag is set */
            fexst = 2 ;                         /* Function Exit Status = Load Error */
            break;
        }
        if ( max ) {                            /* max record to copy limit exists */
            max -= (long)Orespl;                /* decrese max record counter */
            if ( max <= 0 ) {                   /* max limit has been reached */
                break ;
            } else if (max < (long)etab[eid].r) {     /* we do not need to fetch full rowsets any more */
                if (!SQL_SUCCEEDED(Or = SQLSetStmtAttr(Ostmt, SQL_ATTR_ROW_ARRAY_SIZE,
                    (SQLPOINTER)(max), 0))) {
                    Oerr(eid, tid, __LINE__, Ostmt, SQL_HANDLE_STMT);
                    goto ocopy_exit;
                }
            }
        }
#ifdef ODB_PROFILE
            clock_gettime(CLOCK_MONOTONIC, &tsp1);
#endif
    }
    if ( !SQL_SUCCEEDED ( Or ) && Or != SQL_NO_DATA ) {
        fexst = 1 ;                         /* Function Exit Status = Fetch Error */
        Oerr(eid, tid, __LINE__, Ostmt, SQL_HANDLE_STMT);
    }
    etab[eid].flg |= 02000;                     /* mark EOF */
    CondWakeAll(&etab[eid].pcvc);

    /* Wait children thread */
#ifdef ODB_PROFILE
    clock_gettime(CLOCK_MONOTONIC, &tsp1);
#endif
    MutexLock(&etab[eid].pmutex);
    while ( go ) {          
        for ( j = 0 , i = echild ; i < echild + nchild ; i++ )
            j += etab[i].lstat ;
        if ( j != 10 * nchild ) {               /* one or more child still working */
            CondWait(&etab[eid].pcvp, &etab[eid].pmutex);
        } else {
            break;
        }
    }
    MutexUnlock(&etab[eid].pmutex);
#ifdef ODB_PROFILE
    clock_gettime(CLOCK_MONOTONIC, &tsp2);
    ts += tspdiff ( &tsp1 , &tsp2 ) ;
#endif

    /* Check Function Exit Status */
    if ( etab[gpar].flg & 04000 )
        fexst = 2 ;
    if ( fexst )
        goto ocopy_exit ;

    /* Mark End Copy Timestamp */
    telaps -= 1000*tve.tv_sec + tve.tv_usec/1000;
    gettimeofday(&tve, (void *)NULL);       /* register end time */
    telaps += 1000*tve.tv_sec + tve.tv_usec/1000;
    
    /* Save Total/Wait cycles counter into global variables */
    etab[eid].nt = nt ;
    etab[eid].nw = nw ;

#ifdef ODB_PROFILE
    fprintf(stderr, "[%d] (odb profile Ocopy) Fetch time (ns): %s\n", tid, strmnum((double)tf,num,sizeof(num),0));
    fprintf(stderr, "[%d] (odb profile Ocopy) Thread synch time (ns): %s\n", tid, strmnum((double)ts,num,sizeof(num),0));
    fprintf(stderr, "[%d] (odb profile Ocopy) Memory copy time (ns): %s\n", tid, strmnum((double)tm,num,sizeof(num),0));
#endif
    /* print sequence info */
    if ( etab[eid].seqp && f & 020000 )     /* if verbose */
        fprintf(stderr, "[%d] odb [Ocopy(%d)] - Max sequence value: %llu\n",
            tid, __LINE__, (unsigned long long)(lseq - 1) );

    /* Decrease "still active" parallel streams counter. Last thread to print stats */
    MutexLock( &etab[gpar].gmutex ); 
    if ( etab[gpar].ps ) {
        etab[gpar].ps--;
        if ( etab[gpar].ps )            /* this is not the last thread */
            goto ocopy_exit;
    }
    if ( !etab[gpar].ps ) {             /* Last thread to print stats */
        MutexUnlock( &etab[gpar].gmutex );
        for ( j = 0 ; j < no ; j++ ) {
            if ( ( etab[j].type == 'C' || etab[j].type == 'P' ) && etab[j].k == (unsigned int)gpar ) {
                nr += etab[j].nr;
            }
        }
        fprintf(stderr,"[%d] %s %s statistics:\n", tid, odbid, etab[eid].tgtsql ? "Pipe" : "Copy");
        if ( etab[eid].flg2 & 0100000 )     /* print custom SQL file name */
            fprintf(stderr,"\t[%d] Source (SQL file): %s\n", gptid, etab[eid].cols);
        else if ( etab[eid].sql )           /* print custom SQL cmd */
            fprintf(stderr,"\t[%d] Source (SQL): %s\n", gptid, etab[eid].sql);
        else                                /* print input table name */
            fprintf(stderr,"\t[%d] Source: %s\n", gptid, etab[eid].src);
        if ( etab[eid].tgtsql && etab[eid].ns[0] == '@' )   /* print custom tgt sql file name */
            fprintf(stderr,"\t[%d] Target (SQL file): %s\n", gptid, &etab[eid].ns[1]);
        else if ( etab[eid].tgtsql )
            fprintf(stderr,"\t[%d] Target (SQL): %s\n", gptid, etab[eid].tgtsql);
        else
            fprintf(stderr,"\t[%d] Target: %s\n", gptid, etab[eid].run);
        fprintf(stderr, "\t[%d] Total number of columns: %s\n", gptid, strmnum((double)Oncol,num,sizeof(num),0));
        fprintf(stderr, "\t[%d] ODBC row size: %s B (data)", gptid, strmnum((double)(etab[eid].s-Oncol*sizeof(SQLLEN)),num,sizeof(num),0));
        fprintf(stderr, " + %s B (len ind)\n", strmnum((double)(Oncol*sizeof(SQLLEN)), num,sizeof(num),0));
        fprintf(stderr,"\t[%d] Rowset size: %s\n", gptid, strmnum((double)etab[eid].r,num,sizeof(num),0));
        fprintf(stderr,"\t[%d] Rowset buffer size: %s KiB\n", gptid, strmnum((double)rbl/1024.0,num,sizeof(num),2));
        seconds = (double)tinit/1000.0 ;
        fprintf(stderr,"\t[%d] Pre-%s time: %s s (%s)\n", gptid, etab[eid].tgtsql ? "pipe" : "copy",
            strmnum(seconds,num,sizeof(num),3), strmtime(seconds, tim));
        seconds = (double)telaps/1000.0 ;
        fprintf(stderr,"\t[%d] %s time: %s s (%s)\n", gptid, etab[eid].tgtsql ? "Pipe" : "Copy",
            strmnum(seconds,num,sizeof(num),3), strmtime(seconds, tim));
        fprintf(stderr,"\t[%d] Total records %s: %s ", gptid, etab[eid].tgtsql ? "piped" : "copied",
            strmnum((double)nr,num,sizeof(num),0));
            fprintf(stderr,"(%s krec/s)\n", strmnum((double)nr*1.0/(telaps),num,sizeof(num),3));
        fprintf(stderr, "\t[%d] %s throughput (ODBC): %s MiB/s ", gptid, etab[eid].tgtsql ? "Pipe" : "Copy",
            strmnum((double)((etab[eid].s-Oncol*sizeof(SQLLEN))*nr)/1.024/1024.0/telaps,num,sizeof(num),3));
        fprintf(stderr, " (%s GiB/h)\n", 
            strmnum((double)((etab[eid].s-Oncol*sizeof(SQLLEN))*nr)*3600.0/1.024/1024.0/1024.0/telaps,num,sizeof(num),3));
        if ( j > 1 ) {  /* more than one extract stream... */
            for ( j = 0 ; j < no ; j++ ) {
                if ( ( etab[j].type == 'c' || etab[j].type == 'p' ) && etab[j].k == (unsigned int)gpar ) {
                    fprintf(stderr,"\t\t[%d] Total/Wait cycles: %s/", etab[j].id, strmnum((double)etab[j].nt,num,sizeof(num),0));
                    fprintf(stderr,"%s\n", strmnum((double)etab[j].nw,num,sizeof(num),0));
                    for ( i = etab[j].child ; i < etab[j].child + nchild ; i++ ) {
                        fprintf(stderr,"\t\t\t[%d>%d] %s records %s", etab[j].id, etab[i].id,
                            strmnum((double)etab[i].nr,num,sizeof(num),0), etab[eid].tgtsql ? "piped" : "copied");
                        seconds = (double)etab[i].nrt/1000.0 ;
                        fprintf(stderr," in %s (%s s)\n",
                            strmnum(seconds,num,sizeof(num),3), strmtime(seconds, tim));
                    }
                }
            }
        }
        if ( ( etab[eid].flg2 & 0040 ) && etab[eid].cols )
            free ( etab[eid].cols ) ;
        if ( etab[eid].flg2 & 0100000 )
            free ( etab[eid].sql ) ;
        if ( etab[eid].tgtp )
            free ( etab[eid].tgtp ) ;
        if ( etab[eid].tgtsql )
            free ( etab[eid].tgtsql ) ;
        if ( etab[eid].fdmp )               /* close errdmp file */
            fclose(etab[eid].fdmp);
        if ( etab[eid].type == 'c' )        /* it's a copy thread */
            if ( etab[eid].src )            /* table (not SQL) copy */
                if ( etab[eid].run )
                    free ( etab[eid].run ) ;
    }

    /* Clean & exit */
    ocopy_exit:
    MutexUnlock( &etab[gpar].gmutex );
    etab[eid].flg |= 02000;             /* mark EOF */
    CondWakeAll(&etab[eid].pcvc);
    (void)SQLFreeStmt(Ostmt, SQL_CLOSE);
    (void)SQLFreeStmt(Ostmt, SQL_UNBIND);
    if (Ocmd) 
        if (!etab[eid].tgtsql)
            free ( Ocmd );

    if (etab[eid].td) {
        for (j = 0; j < (int)etab[eid].sp; ++j) {
            if (etab[eid].td[j].Oname)
                free(etab[eid].td[j].Oname);
        }
        free(etab[eid].td);
    }

    if ( etab[eid].Orowsetl )
        free (etab[eid].Orowsetl);

    return(fexst);
}

/* Odiff:
 *      Extract tables to compare them
 *
 *      eid (I): etab entry ID to run
 *
 *      return: void
 */
static void Odiff(int eid)
{
    SQLCHAR *Ocmd = 0;              /* ODBC Command buffer */
    SQLCHAR *O = 0;                 /* ODBC temp variable for realloc */
    SQLRETURN Or=0;                 /* ODBC return value */
    size_t cl=CMD_CHUNK;            /* ODBC Command Length */
    size_t rbl = 0;                 /* rowset buffer length */
    int j = 0;                      /* loop variable */
    unsigned int i = 0;             /* loop variable */
    int tid = etab[eid].id;         /* Thread ID */
    int zchild = etab[eid].child;   /* child Zthread process eid */ 
    int dchild = etab[zchild].child;/* child Dthread process eid */ 
    int epars = eid;                /* parent process eid */ 
    int gpar = etab[eid].k;         /* grand parent eid */
    int fchild = etab[eid].k + 1;   /* grand parent child eid */
    int gptid = etab[gpar].id;      /* grand parent tid */
    struct timeval tve;             /* timeval struct to define elapesd/timelines */
    long telaps = 0,                /* Elapsed time in ms */
         tinit = 0;                 /* Initialization time in ms */
    char num[64];                   /* formatted numbers */
    char tim[15];                   /* formatted time string */
    SQLULEN Orespl = 0;             /* ODBC ulen to store # of fetched rows */
    unsigned char wava = 0001;      /* used to check buffer write_available */
    unsigned char wbsy = 0020;      /* used to mark buffer write_busy */
    unsigned char rava = 0004;      /* used to mark buffer read_available */
    unsigned int cmdl = 0;          /* Ocmd length */
    SQLHSTMT Ostmt=thps[tid].Os;    /* Local ODBC Statement handle */
    double seconds = 0;         /* seconds used for timings */

    /* Run pre-SQL on target (using child connection) */
    if ( etab[gpar].flg2 & 02000 )      /* pre-SQL executed with errors & soe */
        goto odiff_exit;
    MutexLock(&etab[gpar].gmutex);      /* lock shared mutex */
    if ( !(etab[gpar].flg2 & 0400) ) {  /* if checkdb/pre is not done */
        etab[fchild].dbt = checkdb ( fchild, &thps[etab[fchild].id].Oc, NULL, NULL );   /* check tgt DB type */
        if ( etab[eid].pre ) {          /* Pre-SQL to run */
            var_set ( &thps[etab[dchild].id].tva, VTYPE_I, "tgt", etab[eid].tgt );
            etab[dchild].flg2 |= 020000000;/* Oexec to allocate/use its own handle */
            if ( etab[eid].pre[0] == '@' )          /* run a sql script */
                j = runsql(etab[dchild].id, dchild, 0, (etab[eid].pre + 1 ));
            else                                    /* Run single SQL command */
                j = Oexec(etab[dchild].id, dchild, 0, 0, (SQLCHAR *)etab[eid].pre, "");
            etab[dchild].flg2 &= ~020000000;/* Oexec to allocate/use its own handle */
            etab[dchild].nr = 0;        /* reset numer of resulting rows */
            if ( j && etab[eid].flg & 0004 ) {
                etab[eid].flg2 |= 02000 ;   /* set pre-sql error & dtop on error flag */
                fprintf(stderr, "odb [Odiff(%d)] - Error running pre-SQL. Exiting because Stop On Error is set\n", 
                    __LINE__);
                MutexUnlock(&etab[gpar].gmutex);    /* unlock shared mutex */
                goto odiff_exit;
            }
        }
        etab[eid].flg2 |= 0400 ;        /* set checkdb/pre flag done */
    } 
    MutexUnlock(&etab[gpar].gmutex);    /* lock shared mutex */

    /* Set connection attribute Read Only */
    if (!SQL_SUCCEEDED(Or=SQLSetConnectAttr(thps[tid].Oc,
            SQL_ATTR_ACCESS_MODE, (SQLPOINTER)SQL_MODE_READ_ONLY, SQL_IS_UINTEGER))) {
        Oerr(eid, tid, __LINE__, thps[tid].Oc, SQL_HANDLE_DBC);
        return;
    }

    /* Allocate initial Ocmd memory */
    if ( ( Ocmd = malloc ( cl * 2 ) ) == (void *) NULL ) {
        fprintf(stderr, "odb [Ocopy(%d)] - Error allocating Ocmd memory\n", __LINE__);
        goto odiff_exit;
    }
    Ocmd[0] = '\0';
    cmdl = strmcat((char *)Ocmd, "SELECT ", cl, 0);

    /* Build Extraction Command */
    for ( j = 0 ; j < etab[eid].cmt ; j++ ) {
        if ( j )
            cmdl += strmcat ((char *)Ocmd, ",", cl, 0);
        if ( ( ( etab[eid].type == 'd' && etab[eid].dbt == ORACLE ) ||      
               ( etab[eid].type == 'D' && etab[fchild].dbt == ORACLE ) ) &&
               ( etab[eid].td[j].Otype == SQL_DECIMAL && etab[eid].td[j].Odec > 0 ) ) {
            /* Oracle - differently from MySQL, PostgreSQL... export NUMBER(x,y) fields without trailing
             * zeroes and this creates "false differences" comparing - for example - 12.30 with 12.3. So I cast all
             * SQL_DECIMAL (Oracle number) to char using the "right" number of digits before/after the decimal separator.
             * Be aware, odb uses the decimal separator defined through -dsep command line option */
            for ( i = 0 ; i < etab[eid].td[j].Osize - etab[eid].td[j].Odec - 4 ; i++ )
                num[i] = '9' ;
            num[i++] = '0' ;
            num[i++] = (char)dsep ;
            for ( ; i < etab[eid].td[j].Osize - 2 ; i++ )
                num[i] = '0' ;
            num[i] = '\0';
            cmdl += snprintf((char *)(Ocmd+cmdl), cl - cmdl, "TRIM(TO_CHAR(%s,\'%s\'))", etab[eid].td[j].Oname, num);
        } else if ( ( etab[eid].type == 'd' && etab[eid].dbt == ORACLE ) &&
                    ( etab[eid].flg2 & 01000000 ) && etab[eid].td[j].Otype == SQL_TYPE_TIMESTAMP ) {
            /* Oracle ODBC data type for dates is SQL_TIMESTAMP. If odad (Oracle Dates As Dates) option is set we
             * use TO_CHAR to select the Oracle dates as real dates in YYYY-MM-DD format */
            cmdl += snprintf((char *)(Ocmd+cmdl), cl - cmdl, "TO_CHAR(%s,\'YYYY-MM-DD\')", etab[eid].td[j].Oname);
        } else if ( ( ( etab[eid].type == 'd' && etab[eid].dbt == VERTICA ) ||      
                      ( etab[eid].type == 'D' && etab[fchild].dbt == VERTICA ) ) &&
                    ( etab[eid].td[j].Otype == SQL_DECIMAL || etab[eid].td[j].Otype == SQL_NUMERIC ) &&
                    etab[eid].td[j].Odec > 0 ) {
            /* Vertica - differently from MySQL, PostgreSQL... export NUMBER(x,y) fields without trailing
             * zeroes and this creates "false differences" comparing - for example - 12.30 with 12.3. So I cast all
             * SQL_DECIMAL (Oracle number) to char using the "right" number of digits before/after the decimal separator.
             * Be aware, odb uses the decimal separator defined through -dsep command line option */
            cmdl += snprintf((char *)(Ocmd+cmdl), cl - cmdl, "TRIM(TO_CHAR(%s))", etab[eid].td[j].Oname);
        } else if ( ( etab[eid].flg & 0400000000 ) &&
                    ( etab[eid].td[j].Otype == SQL_CHAR || etab[eid].td[j].Otype == SQL_WCHAR ) ) {
            cmdl += snprintf((char *)(Ocmd+cmdl), cl - cmdl, "TRIM(%s)", etab[eid].td[j].Oname);
        } else if ( ( etab[eid].flg2 & 040000000 ) &&
                    ( etab[eid].td[j].Otype == SQL_VARCHAR || etab[eid].td[j].Otype == SQL_WVARCHAR ) ) {
            cmdl += snprintf((char *)(Ocmd+cmdl), cl - cmdl, "TRIM(%s)", etab[eid].td[j].Oname);
        } else if ( ( etab[eid].flg & 02000000 ) &&
                    ( etab[eid].td[j].Otype == SQL_CHAR || etab[eid].td[j].Otype == SQL_WCHAR ) ) {
            cmdl += snprintf((char *)(Ocmd+cmdl), cl - cmdl, "RTRIM(%s)", etab[eid].td[j].Oname);
        } else if ( ( etab[eid].flg & 04000000 ) &&
                    ( etab[eid].td[j].Otype == SQL_VARCHAR || etab[eid].td[j].Otype == SQL_WVARCHAR ) ) {
            cmdl += snprintf((char *)(Ocmd+cmdl), cl - cmdl, "RTRIM(%s)", etab[eid].td[j].Oname);
        } else {
            cmdl += strmcat ( (char *) Ocmd, (char *) etab[eid].td[j].Oname, cl, 0);
        }
        if ( cl - cmdl < CMD_CHUNK ) {
            cl += CMD_CHUNK;
            O = Ocmd ;
            if ( ( O = (SQLCHAR *)realloc ( O, cl ) ) == (void *)NULL ) {
                fprintf(stderr, "odb [Odiff(%d)] - Error re-allocating memory for Ocmd: [%d] %s\n",
                    __LINE__, errno, strerror(errno));
                return;
            }
            Ocmd = O ;
        }
        #ifdef __hpux
            /* length indicators have to be memory aligned to avoid bus errors */
            if ( etab[eid].td[j].Osize % WORDSZ )
                etab[eid].td[j].pad = WORDSZ - etab[eid].td[j].Osize % WORDSZ ;
        #endif
        if ( !etab[eid].td[j].dl && j && etab[eid].td[j-1].dl ) {   /* no more order-by columns */ 
            if ( etab[eid].s % WORDSZ )             /* Round-up (order-by only) rowset to wordsize */
                etab[eid].s += WORDSZ - etab[eid].s % WORDSZ ;
            etab[zchild].r = etab[eid].s ;              /* and save it in the zchild structure */
        }
        etab[eid].td[j].start = (int)etab[eid].s;
        etab[eid].s += (size_t)(etab[eid].td[j].Osize+etab[eid].td[j].pad+sizeof(SQLLEN));
    }

    /* Round-up rowset to wordsize to increase memcmp efficiency */
    if ( etab[eid].s % WORDSZ )
        etab[eid].s += WORDSZ - etab[eid].s % WORDSZ ;
    etab[zchild].s = etab[eid].s ;
    if ( etab[eid].flg2 & 0010 )    /* quick mode */
        etab[zchild].r = etab[eid].s ; 
    /* Adjust count for single column tables */
    if ( etab[eid].cmt == 1 )
        etab[zchild].r = etab[zchild].s;

    /* Build Extraction Command: add FROM/WHERE clause */
    if ( etab[eid].ps ) {
        if ( etab[eid].cr == 1 ) {
            cmdl += snprintf((char *)(Ocmd+cmdl), cl - cmdl,
                " FROM %s WHERE %s < %ld",
                etab[eid].tgt, etab[eid].sb, etab[eid].sbmax);
        } else if ( etab[eid].cr == 2 ) {
            cmdl += snprintf((char *)(Ocmd+cmdl), cl - cmdl,
                " FROM %s WHERE %s >= %ld",
                etab[eid].tgt, etab[eid].sb, etab[eid].sbmin);
        } else {
            cmdl += snprintf((char *)(Ocmd+cmdl), cl - cmdl,
                " FROM %s WHERE %s >= %ld AND %s < %ld",
                etab[eid].type == 'd' ? etab[eid].src : etab[eid].tgt,
                etab[eid].sb, etab[eid].sbmin, etab[eid].sb, etab[eid].sbmax);
        }
        if ( etab[eid].map )
            cmdl += snprintf((char *)(Ocmd+cmdl), cl - cmdl,
            " AND %s", etab[eid].map);
    } else {
        cmdl += snprintf((char *)(Ocmd + cmdl), cl - cmdl, " FROM %s",
            etab[eid].type == 'd' ? etab[eid].src : etab[eid].tgt);
        if ( etab[eid].map )
            cmdl += snprintf((char *)(Ocmd+cmdl), cl - cmdl,
            " WHERE %s", etab[eid].map);
    }

    /* Build Extraction Command: add ORDER BY columns */
    for ( j = 0 ; etab[eid].td[j].dl && j < etab[eid].cmt ; j++ ) {
        if ( j )
            cmdl += strmcat ((char *)Ocmd, ",", cl, 0);
        else
            cmdl += strmcat ((char *)Ocmd, " ORDER BY ", cl, 0);
        cmdl += strmcat ( (char *) Ocmd, (char *) etab[eid].td[j].Oname, cl, 0);
        if ( cl - cmdl < CMD_CHUNK ) {
            cl += CMD_CHUNK;
            O = Ocmd ;
            if ( ( O = (SQLCHAR *)realloc ( O, cl ) ) == (void *)NULL ) {
                fprintf(stderr, "odb [Odiff(%d)] - Error re-allocating memory for Ocmd: [%d] %s\n",
                    __LINE__, errno, strerror(errno));
                return;
            }
            Ocmd = O ;
        }
    }

    /* Print extract command if verbose */
    if ( f & 020000 )
        fprintf(stderr, "[%d] odb [Odiff(%d)] - %s EXTRACT statement: %s\n",
            tid, __LINE__, etab[eid].type == 'd' ? "src" : "tgt", (char *)Ocmd);

    /* Prepare command */
    if ((Or=SQLPrepare(Ostmt, Ocmd, SQL_NTS)) != SQL_SUCCESS) {
        Oerr(eid, tid, __LINE__, Ostmt, SQL_HANDLE_STMT);
        if ( Or != SQL_SUCCESS_WITH_INFO )
            goto odiff_exit;
    }

    /* Execute command */
    if (!SQL_SUCCEEDED(Or=SQLExecute(Ostmt))) {
        Oerr(eid, tid, __LINE__, Ostmt, SQL_HANDLE_STMT);
        goto odiff_exit;
    }

    /* Calculate rowset if buffer size is set */
    if ( etab[eid].rbs )
    {
        etab[eid].r = etab[eid].rbs / etab[eid].s;
        etab[eid].r = etab[eid].r < 1 ? 1 : etab[eid].r; // at least one record at a time
    }

    /* Allocate memory for result set from source */
    rbl = etab[eid].r * etab[eid].s ;
    if ( (etab[eid].Orowsetl = (SQLCHAR *) calloc ( etab[eid].r, etab[eid].s )) == (void *)NULL ) { 
            fprintf(stderr, "odb [Odiff(%d)] - Error allocating source rowset memory: [%d] %s\n",
                __LINE__, errno, strerror(errno));
        goto odiff_exit;
    } 

    /* Allocate memory for child rowset(s) */
    if ( etab[eid].type == 'd' ) {
        if ( (etab[zchild].Orowsetl = (SQLCHAR *) malloc (rbl)) == (void *)NULL ) {
                fprintf(stderr, "odb [Odiff(%d)] - Error allocating target rowset memory: [%d] %s\n",
                    __LINE__, errno, strerror(errno));
            goto odiff_exit;
        }
    } else {    /* type 'D' thread */
        wava = 0002;    /* used to check buffer write_available */
        wbsy = 0040;    /* used to mark buffer write_busy */
        rava = 0010;    /* used to mark buffer read_available */
        epars = etab[eid].parent;   /* used to point mutex and cond var */
        if ( (etab[zchild].Orowsetl2 = (SQLCHAR *) malloc (rbl)) == (void *)NULL ) {
                fprintf(stderr, "odb [Odiff(%d)] - Error allocating target rowset memory2: [%d] %s\n",
                    __LINE__, errno, strerror(errno));
            goto odiff_exit;
        }
    }

    /* Initilize ODBC dump */
    if ( fdmp ) {
        for ( j = 0 ; j < etab[eid].cmt ; j++ ) {
            fprintf(fdmp, "[%d] Column %d: name=%s, type=%d (%s), size=%lu, decimals=%d, nullable=%s\n",
                tid, j, (char *)etab[eid].td[j].Oname, (int)etab[eid].td[j].Otype,
                expandtype(etab[eid].td[j].Otype), (unsigned long)etab[eid].td[j].Osize,
                (int)etab[eid].td[j].Odec, etab[eid].td[j].Onull ? "yes" : "no" ) ; 
        }
    }

    /* Set row-wise binding for source */
    if (!SQL_SUCCEEDED(Or=SQLSetStmtAttr(Ostmt, SQL_ATTR_ROW_BIND_TYPE,
            (SQLPOINTER)(etab[eid].s), 0))) {
        Oerr(eid, tid, __LINE__, Ostmt, SQL_HANDLE_STMT);
        goto odiff_exit;
    }
    if (!SQL_SUCCEEDED(Or=SQLSetStmtAttr(Ostmt, SQL_ATTR_ROW_ARRAY_SIZE,
            (SQLPOINTER)(etab[eid].r), 0))) {
        Oerr(eid, tid, __LINE__, Ostmt, SQL_HANDLE_STMT);
        goto odiff_exit;
    }
    if (!SQL_SUCCEEDED(Or=SQLSetStmtAttr(Ostmt, SQL_ATTR_ROW_STATUS_PTR,
            NULL, 0))) {
        Oerr(eid, tid, __LINE__, Ostmt, SQL_HANDLE_STMT);
        goto odiff_exit;
    }
    if (!SQL_SUCCEEDED(Or=SQLSetStmtAttr(Ostmt, SQL_ATTR_ROWS_FETCHED_PTR, 
            &Orespl, 0))) {
        Oerr(eid, tid, __LINE__, Ostmt, SQL_HANDLE_STMT);
        goto odiff_exit;
    }

    /* Bind Columns */
    for (j = 0; j < etab[eid].cmt; j++) {
        if (!SQL_SUCCEEDED(Or=SQLBindCol(Ostmt, (SQLUSMALLINT)j + 1, SQL_C_CHAR,
                &etab[eid].Orowsetl[etab[eid].td[j].start], etab[eid].td[j].Osize,
                (SQLLEN *)&etab[eid].Orowsetl[etab[eid].td[j].start+etab[eid].td[j].Osize+etab[eid].td[j].pad]))) {
            Oerr(eid, tid, __LINE__, Ostmt, SQL_HANDLE_STMT);
            goto odiff_exit;
        }
    }

    /* Register Diff Start Time */
    gettimeofday(&tve, (void *)NULL);
    tinit = 1000*(tve.tv_sec-tvi.tv_sec)+(tve.tv_usec-tvi.tv_usec)/1000;

    /* Fetch data */
    while ( go &&
            SQL_SUCCEEDED(Or = SQLFetchScroll(Ostmt, SQL_FETCH_NEXT, 0))) {
        while ( go ) {
            MutexLock(&etab[epars].pmutex);     /* lock shared mutex */
            while ( go ) {
                if ( etab[zchild].lstat & wava ) {  /* if write available */
                    etab[zchild].lstat &= ~wava;    /* reset write availability */
                    etab[zchild].lstat |= wbsy;     /* set buffer write_busy */
                    MutexUnlock(&etab[epars].pmutex);   /* unlock mutex */
                    break;
                } else {                            /* if comp not available */
                    CondWait(&etab[epars].pcvp, &etab[epars].pmutex);   /* wait */
                }
            }
            MutexUnlock(&etab[epars].pmutex);   /* unlock mutex */
            if ( etab[eid].type == 'd' ) {      /* if src reader... */
                MEMCPY(etab[zchild].Orowsetl, etab[eid].Orowsetl, rbl);
                etab[zchild].ar = (size_t)Orespl;       /* inform loader about #rec to insert */
            } else{                             /* if tgt reader ... */
                MEMCPY(etab[zchild].Orowsetl2, etab[eid].Orowsetl, rbl);
                etab[zchild].rbs = (size_t)Orespl;      /* inform loader about #rec to insert */
            }
            MutexLock(&etab[epars].pmutex);     /* lock shared mutex */
            etab[zchild].lstat &= ~wbsy;        /* unset buffer write_busy */
            etab[zchild].lstat |= rava;         /* set buffer read_available */
            MutexUnlock(&etab[epars].pmutex);   /* unlock mutex */
            CondWake(&etab[epars].pcvc);        /* wake-up loader thread */
            break;                              /* exit this loop and... */
        }
        if ( etab[eid].mr && (unsigned long) ( etab[zchild].nr + Orespl ) >= etab[eid].mr )
            break;
        memset ( etab[eid].Orowsetl, 0, rbl );
    }
    if ( !SQL_SUCCEEDED ( Or ) )
        Oerr(eid, tid, __LINE__, Ostmt, SQL_HANDLE_STMT);
    MutexLock(&etab[epars].pmutex);
    etab[eid].flg |= ( etab[eid].type == 'd' ) ? 02000 : 0200000000 ;   /* mark End Of Data */
    MutexUnlock(&etab[epars].pmutex);
    CondWake(&etab[epars].pcvc);
    MutexLock(&etab[epars].pmutex);
    while ( go ) {                      /* Wait child Z-thread */
        if ( etab[zchild].lstat )
            CondWait(&etab[epars].pcvp, &etab[epars].pmutex);
        else
            break;
    }
    MutexUnlock(&etab[epars].pmutex);
    telaps -= 1000*tve.tv_sec + tve.tv_usec/1000;
    gettimeofday(&tve, (void *)NULL);       /* register end time */
    telaps += 1000*tve.tv_sec + tve.tv_usec/1000;
    
    /* End thread activities */
    if ( etab[eid].type == 'd' ) {
        MutexLock( &etab[gpar].gmutex ); 
        if ( etab[gpar].ps ) {                  /* this is not the last "stream" */
            etab[gpar].ps--;                    /* Decrease d-thread counter */
            if ( etab[gpar].ps )                /* this is not the last thread */
                goto odiff_exit;
        }
        if ( !etab[gpar].ps ) {                 /* this is the last thread */
            MutexUnlock( &etab[gpar].gmutex );
            for ( j = 0 ; j < no ; j++ ) {
                if ( etab[j].type == 'Z' && etab[j].k == (unsigned int)gpar ) {
                    etab[eid].nr += etab[j].nr;     /* records compared */
                    etab[eid].mr += etab[j].mr;     /* records 'I' (no src) */
                    etab[eid].nrt += etab[j].nrt;   /* records 'D' (no tgt) */
                    etab[eid].nbs += etab[j].nbs;   /* records 'C' (same order-by - different content) */
                }
            }
            if ( etab[eid].fo && etab[eid].fo != stdout )   /* close output file */
                fclose ( etab[eid].fo );
            fprintf(stderr,"[%d] %s Diff statistics:\n", tid, odbid);
            fprintf(stderr,"\t[%d] Source: %s\n", gptid, etab[eid].src);
            fprintf(stderr,"\t[%d] Target: %s\n", gptid, etab[eid].tgt);
            fprintf(stderr,"\t[%d] Quick Diff: %s\n", gptid, ( etab[eid].flg2 & 0010 ) ? "Yes": "No" );
            fprintf(stderr,"\t[%d] Record buffer size: %s bytes\n", gptid, strmnum((double)etab[eid].s,num,sizeof(num),0));
            fprintf(stderr,"\t[%d] Rowset size: %s\n", gptid, strmnum((double)etab[eid].r,num,sizeof(num),0));
            fprintf(stderr,"\t[%d] Rowset buffer size: %s KiB\n", gptid, strmnum((double)rbl/1024.0,num,sizeof(num),2));
            seconds = (double)tinit/1000.0 ;
            fprintf(stderr,"\t[%d] Pre-diff time: %s (%s s)\n", gptid,
                strmnum(seconds,num,sizeof(num),3), strmtime(seconds, tim));
            seconds = (double)telaps/1000.0 ;
            fprintf(stderr,"\t[%d] Diff time: %s (%s s)\n", gptid,
                strmnum(seconds,num,sizeof(num),3), strmtime(seconds, tim));
            fprintf(stderr,"\t[%d] Total comparisons: %s ", gptid, strmnum((double)etab[eid].nr,num,sizeof(num),0));
                fprintf(stderr,"(%s kcomp/s)\n", strmnum((double)etab[eid].nr*1.0/telaps,num,sizeof(num),3));
            if ( etab[eid].flg & 02000000000 )  /* print 'I' rows */
                fprintf(stderr,"\t[%d] Total new records on tgt (I): %s\n", gptid, strmnum((double)etab[eid].mr,num,sizeof(num),0));
            if ( etab[eid].flg & 04000000000 )  /* print 'D' rows */
                fprintf(stderr,"\t[%d] Total missing records on tgt (D): %s\n", gptid, strmnum((double)etab[eid].nrt,num,sizeof(num),0));
            if ( etab[eid].flg & 010000000000 ) /* print 'C' rows */
                fprintf(stderr,"\t[%d] Total records modified (C): %s\n", gptid, strmnum((double)etab[eid].nbs,num,sizeof(num),0));
            if ( j > 1 ) {  /* more than one extract stream... */
                for ( j = 0 ; j < no ; j++ ) {
                    if ( etab[j].type == 'Z' && etab[j].k == (unsigned int)gpar ) {
                        fprintf(stderr,"\t\t[%d:%d>%d] %s records compared: ", 
                            j-2, j-1, j, strmnum((double)etab[j].nr,num,sizeof(num),0));
                        if ( etab[eid].flg & 02000000000 ) /* print 'I' rows */
                            fprintf(stderr,"I=%s ", strmnum((double)etab[j].mr,num,sizeof(num),0));
                        if ( etab[eid].flg & 04000000000 ) /* print 'D' rows */
                            fprintf(stderr,"D=%s ", strmnum((double)etab[j].nrt,num,sizeof(num),0));
                        if ( etab[eid].flg & 010000000000 ) /* print 'C' rows */
                            fprintf(stderr,"C=%s ", strmnum((double)etab[j].nbs,num,sizeof(num),0));
                        fputc('\n', stderr);
                    }
                }
            }
            if ( etab[eid].td ) {   /* last thread frees table desc structure memory */
                for ( j = 0 ; j < etab[eid].cmt ; j++ )
                    free ( etab[eid].td[j].Oname );
                free(etab[eid].td);
            }
        }
    } else {    /* this is a D-thread */
        if ( etab[eid].post ) {             /* SQL-POST cmd to run */
            MutexLock( &etab[gpar+1].pmutex ); 
            if ( etab[gpar+1].ps )          /* this is not the last D-thread */
                etab[gpar+1].ps--;          /* Decrease D-thread counter */
            if ( !etab[gpar+1].ps ) {       /* last D-thread */
                MutexUnlock( &etab[gpar+1].pmutex );
                (void)SQLFreeStmt(Ostmt, SQL_CLOSE);    /* Close cursor */
                (void)SQLFreeStmt(Ostmt, SQL_UNBIND);   /* Unbind params */
                var_set ( &thps[tid].tva, VTYPE_I, "tgt", etab[eid].tgt );
                etab[eid].flg2 |= 020000000 ;           /* Oexec to allocate/use its own handle */
                if ( etab[eid].pre[0] == '@' )          /* run a sql script */
                    (void)runsql(tid, eid, 0, (etab[eid].post + 1 ));
                else                                    /* Run single SQL command */
                    (void)Oexec(tid, eid, 0, 0, (SQLCHAR *)etab[eid].post, "");
                etab[eid].flg2 &= ~020000000 ;          /* reset Oexec to allocate/use its own handle */
            } else {
                MutexUnlock( &etab[gpar+1].pmutex );
            }
        }
    }

    /* Clean & exit */
    odiff_exit:
    MutexUnlock( &etab[gpar].gmutex );
    etab[eid].flg |= etab[eid].type == 'd' ? 02000 : 0200000000 ;   /* mark End Of Data */
    CondWake(&etab[epars].pcvc);
    (void)SQLFreeStmt(Ostmt, SQL_CLOSE);
    (void)SQLFreeStmt(Ostmt, SQL_UNBIND);
    if (Ocmd) 
        free ( Ocmd );
    if ( etab[eid].Orowsetl )
        free (etab[eid].Orowsetl);
}

/* Ocompare:
 *      Compares two buffers received from src/tgt
 *
 *      eid (I): etab entry ID to run
 *
 *      return: void
 */
static void Ocompare(int eid)
{
    unsigned int i = 0;             /* loop variable */
    int epars = etab[eid].parent;   /* Parent 'd' (src) etab id shortcut */
    int epart = etab[eid].child;    /* Parent 'D' (tgt) etab id shortcut */
    int ret = 0;                    /* memcmp ret value */
    unsigned int csrn = 0;          /* Current Source Record Number */
    unsigned int ctrn = 0;          /* Current Target Record Number */
    unsigned int hsrn = 0;          /* High Target Source Number (last row in src buffer)*/
    unsigned int htrn = 0;          /* High Target Record Number (last row in tgt buffer)*/
    unsigned char cont = 0003 ;     /* Main loop continue flag: 0001=src_cont 0002=tgt_cont */
    unsigned char *ps = 0 ;         /* Source buffer pointer */
    unsigned char *pt = 0 ;         /* Target buffer pointer */
    char num[32];                   /* used to build formatted numbers */
    SQLLEN Old = 0;                 /* src-tgt length indicator difference */

    etab[eid].mr = 0;
    while ( go ) {
        if ( ( cont & 0001 ) && ( csrn == hsrn ) ) {/* more data from src is needed and available */
            MutexLock(&etab[epars].pmutex);
            while ( go ) {
                if ( etab[eid].lstat & 0004 ) {     /* src buffer available */
                    etab[eid].lstat &= ~0004;       /* unset src buffer ready status */
                    etab[eid].lstat |= 0100;        /* set src buffer read busy */
                    break;
                } else if ( etab[epars].flg & 02000 ){/* src EOD */
                    cont &= ~0001;                  /* no more data available from source */
                    break;
                } else {
                    CondWait(&etab[epars].pcvc, &etab[epars].pmutex);
                }
            }
            MutexUnlock(&etab[epars].pmutex);
            hsrn += (unsigned int)etab[eid].ar ;    /* set high source record number */
            ps = (unsigned char *)etab[eid].Orowsetl;   /* set src pointer */
        }
        if ( ( cont & 0002 ) && ( ctrn == htrn ) ) {/* more data from tgt is needed and available */
            MutexLock(&etab[epars].pmutex);
            while ( go ) {
                if ( etab[eid].lstat & 0010 ) {     /* tgt buffer available */
                    etab[eid].lstat &= ~0010;       /* reset tgt buffer ready status */
                    etab[eid].lstat |= 0200;        /* set src buffer read busy */
                    break;
                } else if ( etab[epart].flg & 0200000000 ){/* tgt EOD */
                    cont &= ~0002;                  /* no more data available from target */
                    break;
                } else {
                    CondWait(&etab[epars].pcvc, &etab[epars].pmutex);
                }
            }
            MutexUnlock(&etab[epars].pmutex);
            htrn += (unsigned int)etab[eid].rbs ;   /* set high source record number */
            pt = (unsigned char *)etab[eid].Orowsetl2;  /* set tgt pointer */
        }
        if ( !cont ) {                              /* neither src nor tgt data available */
            break;                                  /* exit loop */
        } else if ( ! ( cont & 0001 ) ) {           /* no more data on src... */
            for ( ; ctrn < htrn ; ctrn++ ) {        /* all tgt rows were inserted */
                if ( etab[eid].flg & 02000000000 )  /* print 'I' rows */
                    prec('I', pt, eid);
                etab[eid].mr++;
                pt += etab[eid].s;
            }
            hsrn = csrn ;
        } else if ( ! ( cont & 0002 ) ) {           /* no more data on tgt... */
            for ( ; csrn < hsrn ; csrn++ ) {        /* all src rows were deleted */
                if ( etab[eid].flg & 04000000000 )  /* print 'D' rows */
                    prec('D', ps, eid);
                etab[eid].nrt++;
                ps += etab[eid].s;
            }
            htrn = ctrn ;
        } else {                                    /* data available on both src ad tgt */
            while ( ( csrn < hsrn ) && ( ctrn < htrn ) ) {  /* comparison loop */
                etab[eid].nr++;
                /* First compare via memcmp() the initial 'key' rowset component made
                   of both real data in SQL_C_CHAR format and length indicators */  
                if ( ( ret = memcmp ( ps, pt, etab[eid].r ) ) == 0 ) {
                    /* If the src/tgt key rowset component are identical (ret=0) and we have to
                       look for 'C'hanged rows, compare the remaining part of rowset via memcmp() */
                    if ( etab[eid].flg & 010000000000 ) {   /* print 'C' rows */
                        if ( memcmp ( ps + etab[eid].r, pt + etab[eid].r, etab[eid].s - etab[eid].r ) ) {
                            prec('S', ps, eid);
                            prec('T', pt, eid);
                            etab[eid].nbs++;
                        }
                    }
                    csrn++;
                    ctrn++;
                    pt += etab[eid].s;
                    ps += etab[eid].s;
                } else {
                    /* We're here if the src/tgt 'key' rowset component are different. In order to
                       understand if this is a new or deleted record we will:
                       - first compare the length indicators for the key columns
                       - if there are no differences we will use the "alphabetic' comparison
                         returned by the first memcmp() here above */
                    for ( i = 0 ; i < etab[eid].pc ; i++ ) {
                        Old = *((SQLLEN *)(ps + etab[eid].td[i].start + etab[eid].td[i].Osize + etab[eid].td[i].pad)) -
                              *((SQLLEN *)(pt + etab[eid].td[i].start + etab[eid].td[i].Osize + etab[eid].td[i].pad)) ;
                        if ( Old ) {    /* src key component longer than tgt */
                            ret = (int)Old ;
                            break;
                        }
                    }
                    if ( ret > 0 ) {
                        if ( etab[eid].flg & 02000000000 )
                            prec('I', pt, eid);
                        etab[eid].mr++;
                        ctrn++;
                        pt += etab[eid].s;
                    } else if ( ret < 0 ) {
                        if ( etab[eid].flg & 04000000000 )
                            prec('D', ps, eid);
                        etab[eid].nrt++;
                        csrn++;
                        ps += etab[eid].s;
                    }
                }
            }
            if ( etab[eid].flg & 0400 )     /* print mark */
                fprintf(stderr, "[%d] %s comparisons (%lu I, %lu D, %lu C)\n",
                    etab[eid].id, strmnum((double)etab[eid].nr,num,sizeof(num),0),
                    etab[eid].mr, etab[eid].nrt, etab[eid].nbs);
        }
        if ( csrn == hsrn ) {               /* mark src buffer write ready */
            MutexLock(&etab[epars].pmutex); /* lock mutex */
            etab[eid].lstat &= ~0100;       /* unset src buffer read busy */
            etab[eid].lstat |= 0001;        /* set src buffer write available */
            MutexUnlock(&etab[epars].pmutex);/* unlock mutex */
            CondWakeAll(&etab[epars].pcvp); /* wake up waiting thread */
        }
        if ( ctrn == htrn ) {           /* mark tgt buffer write ready */
            MutexLock(&etab[epars].pmutex); /* lock mutex */
            etab[eid].lstat &= ~0200;   /* unset tgt buffer read busy */
            etab[eid].lstat |= 0002;    /* set tgt buffer write available */
            MutexUnlock(&etab[epars].pmutex);/* unlock mutex */
            CondWakeAll(&etab[epars].pcvp); /* wake up waiting thread */
        }
    }
    MutexLock(&etab[epars].pmutex); /* lock mutex */
    etab[eid].lstat = 0;            /* set lstat to zero */
    MutexUnlock(&etab[epars].pmutex);/* unlock mutex */
    CondWakeAll(&etab[epars].pcvp); /* wake up waiting thread */

    /* clean and exit */
    if ( etab[eid].Orowsetl )
        free ( etab[eid].Orowsetl );
    if ( etab[eid].Orowsetl2 )
        free ( etab[eid].Orowsetl2 );
}

/* decode_buf
 * decode contents in buf to string and save in buf
 *
 * ibuf: input buffer
 * type: input buffer C type
 * obuf: output buffer
 * size: output buffer dispaly size
 * return obuf
 */
static char* decode_buf(char *ibuf, SQLSMALLINT type, char *obuf, SQLLEN size) {
    switch (type)
    {
    case SQL_C_SHORT:
    case SQL_C_SSHORT:
        snprintf(obuf, size, "%hd", *((unsigned short*)ibuf));
        return obuf;
    case SQL_C_USHORT:
        snprintf(obuf, size, "%hu", *((unsigned short*)ibuf));
        return obuf;
    case SQL_C_LONG:
        snprintf(obuf, size, "%ld", *((long*)ibuf));
        return obuf;
    case SQL_C_ULONG:
        snprintf(obuf, size, "%lu", *((unsigned long*)ibuf));
        return obuf;
    case SQL_C_FLOAT:
        snprintf(obuf, size, "%f", *((float*)ibuf));
        return obuf;
    case SQL_C_DOUBLE:
        snprintf(obuf, size, "%lf", *((double*)ibuf));
        return obuf;
    case SQL_C_STINYINT:
        snprintf(obuf, size, "%hhd", *((char*)ibuf));
        return obuf;
    case SQL_C_UTINYINT:
        snprintf(obuf, size, "%hhu", *((unsigned char*)ibuf));
        return obuf;
    case SQL_C_SBIGINT:
        snprintf(obuf, size, "%lld", *((long long*)ibuf));
        return obuf;
    case SQL_C_UBIGINT:
        snprintf(obuf, size, "%llu", *((long long*)ibuf));
        return obuf;
    default:
        strncpy(obuf, ibuf, size);
        return obuf;
    }
}

/* prec:
 * print rows 
 *
 * type: 'L' (from loads) and 'I', 'D', 'S' or 'T' (from diff)
 * podbc: pointer to the ODBC (row-wise bound) buffer
 * eid: etab entry id
 *
 * return: void
 */
static void prec(char type, unsigned char *podbc, int eid)
{
    unsigned int i = 0;         /* loop variable */
    int gpar = etab[eid].k;     /* GrandParent 'd' (src) etab id shortcut */
    unsigned int nfields = (unsigned int)etab[eid].cmt; /* Initialize number of fields */
    unsigned char *p = 0;       /* pointer to string to print */
    SQLLEN Ofl = 0;             /* Field Length */
    SQLLEN bufl = 20;
    SQLCHAR *tbuf = (SQLCHAR*)malloc(bufl);
    if (!tbuf) {
        fprintf(stderr, "prec(%d): alloc memory for tbuf failed\n", __LINE__);
    }

    if ( type == 'L' || type == 'C' ) {
        gpar = etab[eid].parent;
        /* nfields = (unsigned int)etab[gpar].sp ; */
        /* fix jira 2034, write to bad_record if load with parameter 'bad', etab[gpar].sp equals 0 always */
        nfields = (unsigned int)etab[eid].sp ;
    }
    MutexLock(&etab[gpar].pmutex);      /* lock mutex */
    if ( !(etab[gpar].flg2 & 0200) ) {
        if ( type != 'L' && type != 'C' ) {
            fputs(islower(etab[eid].td[0].Oname[0]) ? "dtype" : "DTYPE", etab[eid].fo);
            for ( i = 0 ; i < nfields ; i++ ) {
                fputc(etab[eid].fs, etab[eid].fo);
                fputs((char *)etab[eid].td[i].Oname, etab[eid].fo);
            }
            fputc(etab[eid].rs, etab[eid].fo);
            etab[gpar].flg2 |= 0200 ;
        }
    }
    if ( type == 'L' || type == 'C' ) {
        if ( etab[eid].fo == stderr ) {
            fputs(">>> ", stderr);
        }
    } else {
        fputc(type, etab[eid].fo);
        fputc(etab[eid].fs, etab[eid].fo);
    }
    for ( i = 0 ; i < nfields ; i++ ) {
        SQLULEN cdatal;
        if (type == 'C') {
            cdatal = etab[eid].td[i].Ocdatabufl;
        }
        else {
            cdatal = etab[eid].td[i].Osize;
        }
        if ( i )
            fputc(etab[eid].fs, etab[eid].fo);
        if ( *((char *)podbc + etab[eid].td[i].start + cdatal + etab[eid].td[i].pad ) == SQL_NULL_DATA ) {
            fputs(etab[eid].ns ? etab[eid].ns : "", etab[eid].fo);
        } else if ( *(SQLLEN *)(podbc + etab[eid].td[i].start + cdatal + etab[eid].td[i].pad ) == 0 ) {
            fputs(etab[eid].es ? etab[eid].es : "", etab[eid].fo);
        } else {
            p = podbc + etab[eid].td[i].start;
            Ofl = *(SQLLEN *)(p + cdatal + etab[eid].td[i].pad);

            if (type == 'C') {
                if (bufl < etab[eid].td[i].OdisplaySize) {
                    bufl = etab[eid].td[i].OdisplaySize + 1;
                    tbuf = (SQLCHAR*)realloc(tbuf, bufl);
                    if (!tbuf) {
                        fprintf(stderr, "prec(%d): alloc memory for tbuf failed\n", __LINE__);
                    }
                }

                Ofl = etab[eid].td[i].OdisplaySize;
                p = (unsigned char*)decode_buf((char*)p, (SQLSMALLINT)etab[eid].td[i].Octype, (char*)tbuf, (SQLLEN)Ofl);
            }

            for ( ; *p && Ofl ; p++, Ofl-- )
                fputc( *p , etab[eid].fo );
        }
    }
    fputc(etab[eid].rs, etab[eid].fo);
    MutexUnlock(&etab[gpar].pmutex);    /* unlock mutex */
}

/* strmcpy:
 * copies source "src" into target "tgt" up to "len" chars in "tgt"
 *
 * tgt: pointer to the target string;
 * src: pointer to the source string;
 * len: max number of chars in t;
 *
 * return: void
 */
static void strmcpy(char *tgt, char *src, size_t len)
{
    char *t = tgt;
    char *s = src;

    if ( s )
        while ( len-- && ( *t++ = *s++ ) );
    *t = '\0';
}

/* strmcat:
 * concat source "src" to target "tgt" up to "len" chars in "tgt"
 *
 * tgt: pointer to the target string;
 * src: pointer to the source string;
 * len: max number of chars in tgt;
 * cas: 0 = don't convert case, 1 = convert to upper cases, 2 convert to lower cases
 *
 * return: number of concatenated chars
 */
static unsigned int strmcat(char *tgt, char *src, size_t len, char cas)
{
    char *t = tgt;
    char *s = src;
    unsigned int n = 0;

    if ( s ) {
        while ( *t && len-- ) 
            t++;
        switch ( cas ) {
        case 1:
            while ( len-- && ( *t++ = toupper((unsigned char)*s++ ) ) )
                n++; 
            break;
        case 2:
            while ( len-- && ( *t++ = tolower((unsigned char)*s++ ) ) )
                n++; 
            break;
        default:
            while ( len-- && ( *t++ = *s++ ) )
                n++; 
            break;
        }
    }
    *t = '\0';
    return ( n );
}

/* strmicmp:
 * compare source and target string ignoring case up to len chars
 *
 * tgt: pointer to the target string;
 * src: pointer to the source string;
 * len: max number of chars to compare: O means the whole string
 *
 * return: 0 if comp is ok, 1 otherwise
 */
static unsigned int strmicmp(char *tgt, char *src, size_t len)
{
    unsigned char *t = (unsigned char *) tgt;
    unsigned char *s = (unsigned char *) src;

    do {
        if ( toupper(*t) != toupper(*s) )
            return (1);
    } while ( --len && *s++ && *t++ );
    return (0);
}

/* strmprint:
 * return a string in input excluding portion included in ^A - ^A
 * and non printable characters. Max length is 128 chars.
 * NOTE: this function is not thread safe and should be used only for
 * SQL Intepreter
 *
 * s(I): pointer to the string to b converted
 *
 * return: converted string
 */
static char *strmprint(char *s)
{
    static char str[128];
    char *t = &str[0];
    register unsigned int c = f = 1;

    while ( *s ) {
        if ( *s == 1 )      /* ^A in input */
            f = f ? 0 : 1;
        else if ( isprint((unsigned char)*s) && f && c++ < sizeof(str))
            *t++ = *s;
        s++;
    }
    *t = '\0';

    return(&str[0]);
}

/* strmnum: convert a double to a formatted string with decimal/thousands separators
 * d = number to convert
 * s = pointer string hosting the formatted number
 * l = length of the hosting string
 * dd = decimal digits
 * ksep = thousand separator (global variable)
 * dsep = decimal separator (global variable)
 *
 * return pointer to the formatted string 
 */
static char *strmnum(double d, char *s, size_t l, unsigned int dd)
{
    unsigned int i=0;   /* loop variable */
    double round = 5;   /* add .5 after last decimal digit */
    char *p = &s[l-dd-1];
    unsigned long n = 0;

    for ( i = 0 ; i < dd + 1 ; i++ )
        round /= 10;
    d += round ;
    n = (unsigned long) d;

    if ( isnan ( d ) )
        return ( "NaN" );
    if ( isinf ( d ) )
        return ( "Inf" );
    s[l-1] = '\0';
    if ( dd ) 
        for ( *--p = dsep, d = ( d - (double) n ) * 10 ; dd ; dd--, d *= 10 )
            s[l-dd-1] = '0' + (unsigned int)d % 10;
    i = 0 ;
    do {
        *--p = '0' + n % 10 ;
        n /= 10;
        if ( n && !(++i%3) )
            *--p = ksep;
    } while ( n );

    return ( p );
}

/* splitcso:
 * split input string using '.' a string separator
 *
 * s: string to split
 * p: array of pointers to cat/sch/obj (to be defined as char *p[3];
 *      p[0] = catalog name or:
 *             NULL if 's' is NULL
 *             NULL if 's' contains 1 element and 'a' is true
 *             NULL if 's' contains 2 elements and 'no schema' is false and 'a' is true
 *      p[1] = schema name or:
 *             NULL if 's' is NULL
 *             NULL if 's' contains 1 element
 *             NULL if 's' contains 2 elements and no schema is true
 *      p[2] = object name or:
 *             NULL if 's' is NULL
 *             NULL if 's' contains 2 elements and 'no schema' is false and 'a' is false
 * a: align elements to left (0) or right (1) when 's' has less than 3 elements:
 *            (a=0) left <-- catalog.schema.object --> right (a=1)
 *                            p[0]    p[1]   p]2]
 * return: void
 */
static void splitcso(char *s, SQLCHAR **p, int a)
{
    int i=0;
    unsigned char *st = (unsigned char *)s;
    char q = 0; /* Quoted text flag. Quoted text is not UPPERCASEd */

    p[0] = p[1] = p[2] = (unsigned char *)NULL;
    if ( !st || !*st )
        return;
    do {
        if ( *st == '\"' )
            q = q ? 0 : 1 ;
        if ( !q && !(f & 010000000000)) 
            *st = toupper(*st);
    } while ( *st++ );
    st = (unsigned char *)s;
    if ( *st )
        p[i++] = st;
    do {
        if ( *st == '.' ) {
            *st = '\0';
            p[i++] = ++st;
        }
    } while ( *st++ && i < 3 );
    switch (i) {
    case 3:
        if ( f & 0100000000 )
            p[0] = (unsigned char *)NULL;
        break;
    case 2:
        if ( f & 020000000 ) {
            p[2] = p[1];
            p[1] = (unsigned char *)NULL;
        } else if ( a || ( f & 0100000000 ) ) {
            p[2] = p[1];
            p[1] = p[0];
            p[0] = (unsigned char *)NULL;
        }
        break;
    case 1:
        if ( a ) {
            p[2] = p[0];
            p[0] = (unsigned char *)NULL;
        }
        break;
    }
}

/* var_set:
 * set a variable to a given array;
 *
 * chain: pointers to vars chain where the new variable should be added
 * type: var type: VTYPE_A, VTYPE_U or VTYPE_I
 * next: name of the variable to be added
 * value: value of the variable to be added
 *
 * return: 
 *  0 = success
 *  -1 = error allocating new struct memory
 *  -2 = error allocating memory for var value
 */
static int var_set(struct ovar **chain, char type, char *name, char *value)
{
    struct ovar *v,
                *p,
                *n;
    size_t len = 0;

    if ( ( v = var_idx ( chain, type, name ) ) ) { 
        if ( ( len = strlen ( value ) ) > v->vlen ) { 
            if ( ( v->value = realloc(v->value, len)) == (void *)NULL ) {
                fprintf(stderr, "odb [var_set(%d)] - Error re-allocating value memory for var %s: [%d] %s\n",
                    __LINE__, name, errno, strerror(errno));
                    return ( -2 );
            }
        }
        strmcpy(v->value, value, len);
        v->vlen = len;
    } else {
        if ( (v = calloc (1, sizeof(struct ovar))) == (void *)NULL ) {
            fprintf(stderr, "odb [var_set(%d)] - Error allocating struct var memory for %s: [%d] %s\n",
                __LINE__, name, errno, strerror(errno));
            return ( -1 );
        }
        len = strlen ( value ) ;
        if ( (v->value = calloc (1, len + 1)) == (void *)NULL ) {
            fprintf(stderr, "odb [var_set(%d)] - Error allocating value memory for var %s: [%d] %s\n",
                __LINE__, name, errno, strerror(errno));
            return ( -2 );
        }
        for ( p = n = *chain ; n ; p = n, n = n->next ) ;
        n = v;
        n->type = type;
        strmcpy(n->name, name, sizeof(n->name));
        strmcpy(n->value, value, len);
        n->vlen = len;
        n->nlen = strlen(name);
        n->next = (struct ovar *) NULL;
        n->prev = p;
        if ( *chain )
            p->next = n;
        else
            *chain = n;
    }
    return ( 0 );
}

/* var_idx:
 * return a pointer to a "struct var" if the given variable exists or NULL
 * if the variable does not exists in the given chain
 *
 * chain: pointers to vars chain where the new variable should be added
 * type: var type: VTYPE_A, VTYPE_U, VTYPE_I or VTYPE_UI
 * value: value of the variable to be added
 *
 * return: 
 *  - pointer to the variable value
 *  - NULL if the given variable does not exists
 */
static struct ovar *var_idx(struct ovar **chain, char type, char *name)
{
    struct ovar *p;

    for ( p = *chain ; p ; p = p->next )
        if ( p->type & type && !strcmp( name, p->name ) )
            return ( p );
    return ((struct ovar *)NULL);
}

/* var_del:
 * delete a given entry from a var chain
 *
 * chain: pointers to vars chain where the new variable should be added
 * type: var type: VTYPE_A, VTYPE_U, VTYPE_I or VTYPE_UI
 * value: value of the variable to be added
 *
 * return: 
 *  0 = success
 *  -1 = entry not found
 */
static int var_del(struct ovar **chain, char type, char *name)
{
    struct ovar *p;

    for ( p = *chain ; p ; p = p->next ) 
        if ( p->type == type && !strcmp( name, p->name ) ) {
            if ( p->next )
                p->next->prev = p->prev;
            if ( p->prev )
                p->prev->next = p->next;
            if ( type != VTYPE_I )
                free ( p->value );
            free ( p );
            return ( 0 );
        }
    return ( -1 );
}

/* var_exp:
 * expand one or more variables in a string: 
 *  - variables identified by '&' will be expanded with 'u'ser defined parameters
 *  - variables identified by '$' will be expanded with environment variables
 *  - to avoid expansion '&' and '$' have to be escaped by the "ec" character
 *
 * str: string to be expanded
 * lp: pointer to a size_t var containing str length. When NULL string will be
 * written to stderr.
 * chain: chain of variables
 *
 * return: 
 *  pointer to the expanded str
 *  NULL error expanding str
 */
static char *var_exp(char *str, size_t *lp, struct ovar **chain)
{
    int i = 0, /* loop variable */
        k = 0,
        j = 0;
    char vn[MAX_VNLEN];
    char flag=0;
    char *es = 0;
    char *new = 0;
    char *n = 0 ;   /* temp realloc var */
    size_t len = 0;
    struct ovar *v=0;

    if ( lp && (new = calloc ( 1, *lp ) ) == (void *)NULL ) {
            fprintf(stderr, "odb [var_exp(%d)] - Error allocating new str in var_exp: [%d] %s\n",
                __LINE__, errno, strerror(errno));
        return ( (char *) NULL);
    }
    for ( i = j = 0 ; str[i] ; i++, j++ ) {
        if ( ( str[i] == '&' || str[i] == '$' ) &&
             ( ( i && str[i-1] != ec ) || !i ) ) {
            flag = str[i++];
            for ( k = 0 ; str[i] && ( str[i] == '_' || isalnum((unsigned char)str[i]) ) ; i++, k++ )
                vn[k] = str[i];
            vn[k] = '\0';
            if ( lp ) {
                if ( flag == '&' && ( v = var_idx ( chain, VTYPE_UI, vn ) ) ) 
                    len = v->vlen;
                else if ( flag == '$' && ( es = getenv ( vn ) ) )
                    len = strlen ( es );
                if ( *lp < ( j + len + 1 ) ) {
                    *lp += CMD_CHUNK;
                    n = new ;
                    if ( ( n = realloc ( n, *lp ) ) == (void *)NULL ) {
                        fprintf(stderr, "odb [var_exp(%d)] - Error re-allocating memory for new str: [%d] %s\n",
                            __LINE__, errno, strerror(errno));
                        return ( (char *) NULL );
                    }
                    new = n ;
                }
                if ( flag == '&' && v ) {
                    j += strmcat ( new, v->value , *lp, 0);
                } else if ( flag == '$' && es ) {
                    j += strmcat ( new, es , *lp, 0);
                } else {
                    while ( j < i ) {
                        new[j] = str[j];
                        j++;
                    }
                }
            } else {
                if ( flag == '&' && ( v = var_idx ( chain, VTYPE_UI, vn ) ) ) 
                    fputs (v->value, stderr);
                else if ( flag == '$' && ( es = getenv ( vn ) ) )
                    fputs (es, stderr);
                else
                    while ( j < i ) {
                        fputc(str[j++], stderr);
                    }
            }
        }
        if ( lp )
            new[j] = str[i];
        else{
            fputc (str[i], stderr);
        }
        if ( !str[i] )
            i--;
    }
    if ( lp ) {
        free ( str );
        return ( new );
    } else {
        fputc ('\n', stderr);
        return ( (char *)NULL );
    }
}

/* dconv:
 * convert input sring into an ISO8601 date/time/timestamp according to the
 * format_string
 *
 * fmt: input format string made of the following characters:
 *      b = abbreviated month name
 *      B = full month name
 *      d = day of the month
 *      H = hour (24 hour format)
 *      m = month number
 *      M = minute
 *      S = second
 *      D = decimals
 *      y = year (four digits)
 *      . = ignore single char
 *      _ = ignore up to the next digit
 * str: string to convert
 * out: pointer to string where to write the output (*)
 * t: conversion type (0=date, 1=timestamp(0), 2=time(0)
 *
 * (*) dconv can safely use in output the input string (if big enough)
 * return:
 *  >1 = output string length 
 *  0 = conversion error
 *  1 = invalid format string
 */
static unsigned int dconv ( char *fmt, char *str, char *out, size_t len, char t )
{
    unsigned int year = 0,  /* year */
                 mon = 0,   /* month */
                 day = 0,   /* day */
                 hour = 0,  /* hours */
                 min = 0,   /* minutes */
                 sec = 0,   /* seconds */
                 dec = 0,   /* decimals */
                 df = 0,    /* decimal flag */
                 i = 0;     /* loop variable */
    char *p = str,
         *ft = fmt;
    char buf[10];           /* buffer */
    static char *amn[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
                           "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" } ;
    static char *fmn[] = { "January", "February", "March", "April",
                           "May", "June", "July", "August", "September",
                           "October", "November", "December" } ;

    while ( *p && *ft ) {
        switch ( *ft++ ) {
        case '.':   /* skip single char */
            p++;
            break;
        case '_':   /* skip up to the next digit */
            while ( *p < '0' || *p > '9' )
                p++;
            break;
        case 'B':
            for ( i = 0 ; ( buf[i] = *p++ ) && *p != ' ' && *p != '\t'; i++ );
            buf[i] = '\0';
            for ( i = 0 ; i < 12 && strcmp(buf, fmn[i]) ; i++ )
            if ( i == 12 )
                return (0);
            else
                mon = i+1;
            break;
        case 'b':
            for ( i = 0 ; i < 3 && ( buf[i] = *p++ ) ; i++ );
            buf[i] = '\0';
            for ( i = 0 ; i < 12 && strcmp(buf, amn[i]) ; i++ );
            if ( i == 12 )
                return (0);
            else
                mon = i+1;
            break;
        case 'd':
            for ( i = 0 ; i < 2 && *p >= '0' && *p <= '9' ; i++ )
                buf[i] = *p++;
            buf[i] = '\0';
            day = (unsigned int)strtol(buf, (char **)NULL, 10);
            if ( !i || day > 31 )
                return (0);
            break;  
        case 'D':
            df = 1;
            for ( i = 0 ; i < (unsigned int)(*ft + 48) && *p >= '0' && *p <= '9' ; i++ )
                buf[i] = *p++;
            ft++;
            buf[i] = '\0';
            dec = (unsigned int)strtol(buf, (char **)NULL, 10);
            break;  
        case 'H':
            for ( i = 0 ; i < 2 && *p >= '0' && *p <= '9' ; i++ )
                buf[i] = *p++;
            buf[i] = '\0';
            hour = (unsigned int)strtol(buf, (char **)NULL, 10);
            if ( !i || hour > 23 )
                return (0);
            break;  
        case 'm':
            for ( i = 0 ; i < 2 && *p >= '0' && *p <= '9' ; i++ )
                buf[i] = *p++;
            buf[i] = '\0';
            mon = (unsigned int)strtol(buf, (char **)NULL, 10);
            if ( !i || mon > 12 )
                return (0);
            break;  
        case 'M':
            for ( i = 0 ; i < 2 && *p >= '0' && *p <= '9' ; i++ )
                buf[i] = *p++;
            buf[i] = '\0';
            min = (unsigned int)strtol(buf, (char **)NULL, 10);
            if ( !i || min > 59 )
                return (0);
            break;  
        case 'S':
            for ( i = 0 ; i < 2 && *p >= '0' && *p <= '9' ; i++ )
                buf[i] = *p++;
            buf[i] = '\0';
            sec = (unsigned int)strtol(buf, (char **)NULL, 10);
            if ( !i || sec > 59 )
                return (0);
            break;  
        case 'y':
            for ( i = 0 ; i < 4 && *p >= '0' && *p <= '9' ; i++ )
                buf[i] = *p++;
            buf[i] = '\0';
            year = (unsigned int)strtol(buf, (char **)NULL, 10);
            if ( i < 4 )
                return (0);
            break;  
        default:
            return ( 1 );
        }
    }
    if ( df ) {
        switch ( t ) {
        case 0:     /* return a date */
            return ( snprintf (out, len, "%04u-%02u-%02u.%u", year, mon, day, dec ) );
        case 1:     /* return a time */
            return ( snprintf (out, len, "%02u:%02u:%02u.%u", hour, min, sec, dec ) );
        case 2:     /* return a timestamp */
            return ( snprintf (out, len, "%04u-%02u-%02u %02u:%02u:%02u.%u",
                year, mon, day, hour, min, sec, dec ) );
        }
    } else {
        switch ( t ) {
        case 0:     /* return a date */
            return ( snprintf (out, len, "%04u-%02u-%02u", year, mon, day ) );
        case 1:     /* return a time */
            return ( snprintf (out, len, "%02u:%02u:%02u", hour, min, sec ) );
        case 2:     /* return a timestamp */
            return ( snprintf (out, len, "%04u-%02u-%02u %02u:%02u:%02u",
                year, mon, day, hour, min, sec ) );
        }
    }
    return ( 0 ) ;
}

/* tokenize:
 * given a string in input returns an array of pointers to the substring 
 * separated by whitespaces (blank, tab, new lines). It manages quoted
 * text and escape characters. The substrings are stored exactly in the
 * same memory as the input string so there's no need for new memory.
 *
 * Exambles:
 *   Input >one two three<          Ouput >one< >two< >three<
 *   Input >one "two three"<        Ouput >one< >two three<
 *   Input >one 'two three'<        Ouput >one< >two three<
 *   Input >one "two 'three"<       Ouput >one< >two 'three<
 *   Input >one "two \"three"<      Ouput >one< >two "three<
 *
 * s: string to tokenize
 * l: length of string s
 * ss: array of char* containing the pointers to the tokenized substrings
 *
 * return:
 *  number of tokenized substrings
 */
static int tokenize(char *s, int l, char *ss[])
{
    int i,
        j,
        n = 0;
    char qs = 0,    /* single quote flag */
         qd = 0;    /* double quote flag */

    for ( i = 0 ; i < l ; i++ ) {
        switch ( s[i] ) {
        case ' ':       /* space */
        case '\t':      /* tab */
        case '\n':      /* tab */
            if ( !qs && !qd )
                s[i] = '\0';
            break;
        case '"':       /* double quote */
            if ( i && !qs && s[i-1] != ec ) {
                qd = qd ? 0 : 1;
                s[i]= '\0';
            }
            break;
        case '\'':      /* single quote */
            if ( i && !qd && s[i-1] != ec ) {
                qs = qs ? 0 : 1;
                s[i]= '\0';
            }
            break;
        default:        /* any other char */
            if ( s[i] == ec && s[i+1] != ec ) {
                for ( j = i, l-- ; j < l; j++) {
                    s[j] = s[j+1];
                }
                s[l] = '\0';    
            }
            if ( !i || !s[i-1] )
                ss[n++] = &s[i]; 
            if ( n == MAX_ARGS )
                return ( n );
        }
    }
    return ( n );
}

/* bcs:
 * Build connection string for SQLDriverConnect
 *
 * Ocs: pointer to the connection string to fill
 * csl: connection string length
 * Ou: User Name
 * Op Password
 * Od: (local) DSN
 * ca: extra connection attributes
 * ctn: if >0 will transform <dsn> into <dsn><number>
 *
 * return:
 *  void
 */
static void bcs(SQLCHAR *Ocs, size_t csl, SQLCHAR *Ou, SQLCHAR *Op, SQLCHAR *Od, char *ca, int ctn)
{
    char buff[8]; 

    Ocs[0] = '\0';
    if ( ca ) {
        (void) strmcat((char *)Ocs, ca, csl, 0);
    }
    if ( Od[0] ) {
        if (Ocs[0])
            (void) strmcat((char *)Ocs, ";", csl, 0);
        (void) strmcat((char *)Ocs, "DSN=", csl, 0);
        (void) strmcat((char *)Ocs, (char *)Od, csl, 0);
        if ( ctn && ndsn ) {
            if ( f & 02000000000 ) 
                snprintf(buff, sizeof(buff), "%d", 1 + ( ctn - 1 ) * ndsn / no );
            else
                snprintf(buff, sizeof(buff), "%d", ( ctn % ndsn ) ? ctn % ndsn : ndsn );
            (void) strmcat((char *)Ocs, buff, csl, 0);
        }
    }
    if ( Ou[0] ) {
        if (Ocs[0])
            (void) strmcat((char *)Ocs, ";", csl, 0);
        (void) strmcat((char *)Ocs, "UID=", csl, 0);
        (void) strmcat((char *)Ocs, (char *)Ou, csl, 0);
    }
    if ( Op[0] ) {
        if (Ocs[0])
            (void) strmcat((char *)Ocs, ";", csl, 0);
        (void) strmcat((char *)Ocs, "PWD=", csl, 0);
        (void) strmcat((char *)Ocs, (char *)Op, csl, 0);
    }
    return;
}

/* mfprintf:
 * Multiple fprintf: write on two file pointers
 *
 * fl1: file pointer to the first output file
 * fl2: file pointer to the second output file
 * fmt: vfprintf() format string
 *
 * return:
 *  the number of written characters on fl1/fl2 or -1 if they differ
 */
static int mfprintf( FILE *fl1, FILE *fl2, char *fmt, ... )
{
    int nc1 = 0,
        nc2 = 0;
    va_list al;

    va_start ( al, fmt );
    if ( fl1 ) 
        nc2 = nc1 = vfprintf ( fl1, fmt, al );
    va_start ( al, fmt );
    if ( fl2 ) {
        nc2 = vfprintf ( fl2, fmt, al );
        fflush ( fl2 );
    }
    va_end ( al );

    return ( nc1 == nc2 ? nc1 : -1 );
}

/* mfputc:
 * Multiple fputc: write on two file pointers
 *
 * fl1: file pointer to the first output file
 * fl2: file pointer to the second output file
 * ch: char to write
 *
 * return:
 *  the character written on fl1/fl2 or -1 if they differ
 */
static int mfputc( FILE *fl1, FILE *fl2, int ch )
{

    int nc1 = 0,
        nc2 = 0;

    if ( fl1 ) 
        nc1 = fputc ( ch, fl1 );
    if ( fl2 ) 
        nc2 = fputc ( ch, fl2 );

    return ( nc1 == nc2 ? nc1 : -1 );
}

/* checkdb:
 * check target database returns database type and fills:
 *
 * eid: etab idx
 * Ocn: connection handle pointer
 * c: pointer to string used to return initial catalog if not NULL
 * s: pointer to string used to return the initial schema if not NULL
 *
 * return:
 *  dbt: database type: 0=unknown/generic, 1=PostgreSQL, 2=Vertica,
 *                      3=Microsoft SQL Server, 4=Teradata, 5=NonStop SQL/MX,
 *                      6=Oracle, 7=MySQL, 8=Trafodion, 255=Error
 */
static unsigned int checkdb(int eid, SQLHDBC *Ocn, char *c, char *s)
{
    char *p = 0;            /* strstr output pointer */
    unsigned int i = 0;     /* loop variable */
    unsigned int dbt = 0;   /* returned Database ID */
    SQLHSTMT Ostmt=0;       /* ODBC Statement handle */
    int tid = eid < 0 ? -1 : etab[eid].id; /* Thread ID */
    SQLCHAR Obuff[128];     /* ODBC buffer */
    SQLINTEGER Obatch = 0;  /* ODBC to get SQL_PARAM_ARRAY_ROW_COUNTS value */

    if ( !SQL_SUCCEEDED(Oret=SQLAllocHandle(SQL_HANDLE_STMT, *Ocn, &Ostmt))){
        Oerr(eid, tid, __LINE__, *Ocn, SQL_HANDLE_DBC);
        return(255);
    }

    if (SQL_SUCCEEDED(Oret=SQLGetInfo(*Ocn, SQL_DBMS_NAME, 
        (SQLPOINTER)Obuff, (SQLSMALLINT)sizeof(Obuff), NULL))) {
        if(Oret == SQL_SUCCESS_WITH_INFO)
            fprintf(stderr, "odb [checkdb(%d)] - Warning data truncation\n", __LINE__);
    }
    if(!SQL_SUCCEEDED(Oret))
        Oerr(eid, tid, __LINE__, *Ocn, SQL_HANDLE_DBC);
    if ( ! ( f & 00040 ) )
        fprintf(stderr, "Connected to %s\n", (char *)Obuff);
    if ( !strcmp((char *)Obuff, dbscmds[1].dbname) ) {          /* PostgreSQL */
        dbt = 1;
        f |= 010000000000;  /* Postgres uses lowercase table names if not quoted */
        if ( c )
            if (!SQL_SUCCEEDED(Oret=SQLGetInfo(*Ocn, SQL_DATABASE_NAME, 
                (SQLPOINTER)c, (SQLSMALLINT)MAXOBJ_LEN, NULL)))
                    Oerr(eid, tid, __LINE__, *Ocn, SQL_HANDLE_DBC);
        if ( s ) {
            if (!SQL_SUCCEEDED(Oret=SQLExecDirect (Ostmt, 
                    (SQLCHAR *)"SELECT CURRENT_SCHEMA()", SQL_NTS)))
                Oerr(eid, tid, __LINE__, Ostmt, SQL_HANDLE_STMT);
            if (!SQL_SUCCEEDED(Oret = SQLBindCol(Ostmt, (SQLUSMALLINT) 1,
                SQL_C_CHAR, (SQLCHAR *)s, (SQLLEN)MAXOBJ_LEN, NULL)))
                Oerr(eid, tid, __LINE__, Ostmt, SQL_HANDLE_STMT);
            if (!SQL_SUCCEEDED(Oret=SQLFetch(Ostmt)))
                Oerr(eid, tid, __LINE__, Ostmt, SQL_HANDLE_STMT);
        }
    } else if ( !strcmp((char *)Obuff, dbscmds[2].dbname) ) {   /* Vertica */
        dbt = 2;
        if ( c ) {  /* Read Calatog from Output Connection String */
            if (!SQL_SUCCEEDED(Oret=SQLExecDirect (Ostmt, 
                (SQLCHAR *)"SELECT CURRENT_DATABASE()", SQL_NTS)))
                Oerr(eid, tid, __LINE__, Ostmt, SQL_HANDLE_STMT);
            if (!SQL_SUCCEEDED(Oret = SQLBindCol(Ostmt, (SQLUSMALLINT) 1,
                SQL_C_CHAR, (SQLCHAR *)c, (SQLLEN)MAXOBJ_LEN, NULL)))
                Oerr(eid, tid, __LINE__, Ostmt, SQL_HANDLE_STMT);
            if (!SQL_SUCCEEDED(Oret=SQLFetch(Ostmt)))
                Oerr(eid, tid, __LINE__, Ostmt, SQL_HANDLE_STMT);
            (void)SQLFreeStmt(Ostmt, SQL_CLOSE);
            (void)SQLFreeStmt(Ostmt, SQL_UNBIND);
        }
        if ( s ) {
            if (!SQL_SUCCEEDED(Oret=SQLExecDirect (Ostmt, 
                (SQLCHAR *)"SELECT CURRENT_SCHEMA()", SQL_NTS)))
                Oerr(eid, tid, __LINE__, Ostmt, SQL_HANDLE_STMT);
            if (!SQL_SUCCEEDED(Oret = SQLBindCol(Ostmt, (SQLUSMALLINT) 1,
                SQL_C_CHAR, (SQLCHAR *)s, (SQLLEN)MAXOBJ_LEN, NULL)))
                Oerr(eid, tid, __LINE__, Ostmt, SQL_HANDLE_STMT);
            if (!SQL_SUCCEEDED(Oret=SQLFetch(Ostmt)))
                Oerr(eid, tid, __LINE__, Ostmt, SQL_HANDLE_STMT);
        }
    } else if ( !strcmp((char *)Obuff, dbscmds[3].dbname) ) {   /* Microsoft SQL Server */
        dbt = 3;
        if ( c )
            if (!SQL_SUCCEEDED(Oret=SQLGetInfo(*Ocn, SQL_DATABASE_NAME, 
                (SQLPOINTER)c, (SQLSMALLINT)MAXOBJ_LEN, NULL)))
                    Oerr(eid, tid, __LINE__, *Ocn, SQL_HANDLE_DBC);
        if ( s ) {
            if (!SQL_SUCCEEDED(Oret=SQLExecDirect (Ostmt, 
                    (SQLCHAR *)"SELECT SCHEMA_NAME()", SQL_NTS)))
                Oerr(eid, tid, __LINE__, Ostmt, SQL_HANDLE_STMT);
            if (!SQL_SUCCEEDED(Oret = SQLBindCol(Ostmt, (SQLUSMALLINT) 1,
                SQL_C_CHAR, (SQLCHAR *)s, (SQLLEN)MAXOBJ_LEN, NULL)))
                Oerr(eid, tid, __LINE__, Ostmt, SQL_HANDLE_STMT);
            if (!SQL_SUCCEEDED(Oret=SQLFetch(Ostmt)))
                Oerr(eid, tid, __LINE__, Ostmt, SQL_HANDLE_STMT);
        }
    } else if ( !strcmp((char *)Obuff, dbscmds[4].dbname) ) {   /* Teradata */
        dbt = 4;
        if ( c )
            *c = '\0';  /* empty string */
        if ( s )
            if (!SQL_SUCCEEDED(Oret=SQLGetInfo(*Ocn, SQL_DATABASE_NAME, 
                (SQLPOINTER)s, (SQLSMALLINT)MAXOBJ_LEN, NULL)))
                    Oerr(eid, tid, __LINE__, *Ocn, SQL_HANDLE_DBC);
    } else if ( !strcmp((char *)Obuff, dbscmds[5].dbname) ) {   // NonStop SQL/MX 
        dbt = 5;
        if ( Oocs[0] ) {
            if ( c ) {  // Read Calatog from Output Connection String 
                for ( i = 0, p = strstr((char *)Oocs, "CATALOG=") + 8 ; *p != ';' && i < sizeof(Oocs) ; p++, i++ )
                    c[i] = *p;
                c[i] = '\0';
            }
            if ( s ) {  // Read Schema from Output Connection String 
                for ( i = 0, p = strstr((char *)Oocs, "SCHEMA=") + 7 ; *p != ';' && i < sizeof(Oocs) ; p++, i++ )
                    s[i] = *p;
                s[i] = '\0';
            }
        }
    } else if ( !strcmp((char *)Obuff, dbscmds[6].dbname) ) {   /* Oracle */
        dbt = 6;
        f |= 04000000000 ;  /* Catalog as NULL */
        if ( c ) 
            *c = '\0';
        if ( s ) {  
            if (!SQL_SUCCEEDED(Oret=SQLExecDirect (Ostmt, 
                    (SQLCHAR *)"SELECT SYS_CONTEXT('userenv','current_schema') x FROM DUAL", SQL_NTS)))
                Oerr(eid, tid, __LINE__, Ostmt, SQL_HANDLE_STMT);
            if (!SQL_SUCCEEDED(Oret = SQLBindCol(Ostmt, (SQLUSMALLINT) 1,
                SQL_C_CHAR, (SQLCHAR *)s, (SQLLEN)MAXOBJ_LEN, NULL)))
                Oerr(eid, tid, __LINE__, Ostmt, SQL_HANDLE_STMT);
            if (!SQL_SUCCEEDED(Oret=SQLFetch(Ostmt)))
                Oerr(eid, tid, __LINE__, Ostmt, SQL_HANDLE_STMT);
        }
    } else if ( !strcmp((char *)Obuff, dbscmds[7].dbname ) ) {  /* MySQL */
        dbt = 7;
        if ( c ) 
            *c = '\0';
        if ( s ) {
            if (!SQL_SUCCEEDED(Oret=SQLExecDirect (Ostmt, 
                    (SQLCHAR *)"SELECT SCHEMA() FROM DUAL", SQL_NTS)))
                Oerr(eid, tid, __LINE__, Ostmt, SQL_HANDLE_STMT);
            if (!SQL_SUCCEEDED(Oret = SQLBindCol(Ostmt, (SQLUSMALLINT) 1,
                SQL_C_CHAR, (SQLCHAR *)s, (SQLLEN)MAXOBJ_LEN, NULL)))
                Oerr(eid, tid, __LINE__, Ostmt, SQL_HANDLE_STMT);
            if (!SQL_SUCCEEDED(Oret=SQLFetch(Ostmt)))
                Oerr(eid, tid, __LINE__, Ostmt, SQL_HANDLE_STMT);
	} 
    } else if ( !strcmp((char *)Obuff, dbscmds[8].dbname) ) 
    {   /* Trafodion */
        dbt = 8;
        if ( Oocs[0] ) 
        {
            if ( c ) 
            {   /* Read Calatog from Output Connection String */
                for ( i = 0, p = strstr((char *)Oocs, "CATALOG=") + 8 ; *p != ';' && i < sizeof(Oocs) ; p++, i++ )
                    c[i] = *p;
                c[i] = '\0';
            }
            if ( s ) 
            {   /* Read Schema from Output Connection String */
                for ( i = 0, p = strstr((char *)Oocs, "SCHEMA=") + 7 ; *p != ';' && i < sizeof(Oocs) ; p++, i++ )
                    s[i] = *p;
                s[i] = '\0';
            }
        }
    }

    /* If this is the Interpreter get the object types list */
    if ( eid >= 0 && etab[eid].type == 'I' ) {
        (void)SQLFreeStmt(Ostmt, SQL_CLOSE);
        (void)SQLFreeStmt(Ostmt, SQL_UNBIND);
        if (!SQL_SUCCEEDED(Oret=SQLTables(Ostmt,
                (SQLCHAR *)"", 0,(SQLCHAR *)"", 0, (SQLCHAR *)"", 0,
                (SQLCHAR *)SQL_ALL_TABLE_TYPES, (SQLSMALLINT)1))) {
            Oerr(eid, tid, __LINE__, Ostmt, SQL_HANDLE_STMT);
        } else {
            if (!SQL_SUCCEEDED(Oret = SQLBindCol(Ostmt, (SQLUSMALLINT) 4,
                SQL_C_CHAR, Obuff, (SQLLEN)sizeof(Obuff), &Olen[0]))) {
                Oerr(eid, tid, __LINE__, Ostmt, SQL_HANDLE_STMT);
            }
            while (SQL_SUCCEEDED(Oret=SQLFetch(Ostmt))) {
                if ( Oaot[0] )
                    strmcat ( (char *) Oaot, ",", sizeof(Oaot), 0);
                strmcat((char *) Oaot,(char *) Obuff, sizeof(Oaot), 0);
            }
        }
    }
    (void)SQLFreeHandle(SQL_HANDLE_STMT, Ostmt);

    /* Next find if row counts are available for parameterized executions
     * Alias: can we use SQLRowCount() after an array INSERT/DELETE/UDATE? */
    if ( eid > 0 ) {
        if (SQL_SUCCEEDED(Oret=SQLGetInfo(*Ocn, SQL_PARAM_ARRAY_ROW_COUNTS,
            (SQLPOINTER)&Obatch, 0, NULL))) {
            switch ( Obatch ) {
            case SQL_PARC_BATCH:
                etab[eid].flg2 &= ~01000 ;
                break;
            case SQL_PARC_NO_BATCH:
                etab[eid].flg2 |= 01000 ;
                break;
            default:
                fprintf(stderr, "odb [checkdb(%d)] - Invalid SQL_PARAM_ARRAY_ROW_COUNTS\n", __LINE__);
            }
        } else {
            Oerr(eid, tid, __LINE__, *Ocn, SQL_HANDLE_DBC);
        }
    }

    return ( dbt );
}

/* mchar:
 * return char from strings, decimal/hex/octal numbers. Example:
 *  "," "44" "x2c" "054" all return comma
 * str: input string
 * return: char
 */
static unsigned char mchar ( char *s )
{
    long l;

    while( *s == ' ' || *s == '\t' )    /* skip leading white spaces */
        s++;
        
    if ( *s == '0' ) {
        l=strtol(++s, (char **)NULL, 8);
    } else if ( *s == 'x' || *s == 'X' ) {
        l=strtol(++s, (char **)NULL, 16);
    } else if ( *s > '0' && *s <= '9' ) {
        l=strtol( s, (char **)NULL, 10);
    } else {
        return ( *s );
    }
    return( (unsigned char)l );
}

/* hextobin:
 * convert c from hexadecimal char to binary
 */
char hextobin(char c) {
    switch(c) {
    default: return c - '0';
    case 'a': case 'A': return '\10';
    case 'b': case 'B': return '\11';
    case 'c': case 'C': return '\12';
    case 'd': case 'D': return '\13';
    case 'e': case 'E': return '\14';
    case 'f': case 'F': return '\15';
    }
}

/* replace_escapes:
 * replace escape sequence like "\r" "\n" "\t" "\xA0"
 * str: input string
 * return: str
 */
static char* replace_escapes ( char *str ) {
    char *p = str;
    char *q = str;

    while (*q) {
        if (*q == '\\' && *(q + 1)) {
            switch (*++q) {
            case 'n': *p++ = '\n'; break;
            case 'r': *p++ = '\r'; break;
            case 't': *p++ = '\t'; break;
            case 'x': case 'X':
                if (!isxdigit(q[1]))
                    goto not_escape;
                *p = hextobin(*++q);
                if (isxdigit(q[1]))
                    *p = *p * 16 + hextobin(*++q);
                ++p;
                break;
            default: // no need to be converted
            not_escape:
                *p++ = '\\';
                *p++ = *q;
                break; // just escape
            }
            ++q;
        }
        else {
            *p++ = *q++;
        }
    }
    *p = '\0';

    return str;
}

/* strup:
 *      convert a source string (s) to uppercase
 *
 *      s: null terminated string to uppercase
 *
 *      return: the conveterd string
 */
static char *strup ( char *s )
{
    char *save = s;

    while ( *s ) {
        *s = toupper((unsigned char) *s);
        s++;
    }
    return(save);
}

/* strlo:
 *      convert a source string (s) to lowercase
 *
 *      s: null terminated string to lowercase
 *
 *      return: the conveterd string
 */
static char *strlo ( char *s )
{
    char *save = s;

    while ( *s ) {
        *s = tolower((unsigned char) *s);
        s++;
    }
    return(save);
}

static char *strtrim(char *str)
{
    // trim tailing space
    size_t i = strlen(str) - 1;
    while (str[i] == ' ') --i;
    str[i + 1] = '\0';

    // trim heading space
    for (i = 0; str[i] == ' '; ++i);
    if (i > 0)
        strcpy(str, str + i);

    return str;
}

/* expandtype:
 *      return SQL type string associated with a given data type
 *
 *      Odt: SQL data type code
 *      return: pointer to SQL type string
 */
static char *expandtype( SQLSMALLINT Odt )
{
    switch ( Odt ) {
    case SQL_SMALLINT: return("SQL_SMALLINT");
    case SQL_INTEGER: return("SQL_INTEGER");
    case SQL_TINYINT: return("SQL_TINYINT");
    case SQL_BIGINT: return("SQL_BIGINT");
    case SQL_REAL: return("SQL_REAL");
    case SQL_DOUBLE: return("SQL_DOUBLE");
    case SQL_FLOAT: return("SQL_FLOAT");
    case SQL_NUMERIC: return("SQL_NUMERIC");
    case SQL_DECIMAL: return("SQL_DECIMAL");
    case SQL_CHAR: return("SQL_CHAR");
    case SQL_WCHAR: return("SQL_WCHAR");
    case SQL_VARCHAR: return("SQL_VARCHAR");
    case SQL_WVARCHAR: return("SQL_WVARCHAR");
    case SQL_LONGVARCHAR: return("SQL_LONGVARCHAR");
    case SQL_WLONGVARCHAR: return("SQL_WLONGVARCHAR");
    case SQL_TIME: return("SQL_TIME");
    case SQL_TYPE_TIME: return("SQL_TYPE_TIME");
    case SQL_TYPE_DATE: return("SQL_TYPE_DATE");
    case SQL_TIMESTAMP: return("SQL_TIMESTAMP");
    case SQL_TYPE_TIMESTAMP: return("SQL_TYPE_TIMESTAMP");
    case SQL_BIT: return("SQL_BIT");
    case SQL_BINARY: return("SQL_BINARY");
    case SQL_VARBINARY: return("SQL_VARBINARY");
    case SQL_LONGVARBINARY: return("SQL_LONGVARBINARY");
    case SQL_INTERVAL_MONTH: return("SQL_INTERVAL_MONTH");
    case SQL_INTERVAL_YEAR: return("SQL_INTERVAL_YEAR");
    case SQL_INTERVAL_YEAR_TO_MONTH: return("SQL_INTERVAL_YEAR_TO_MONTH");
    case SQL_INTERVAL_DAY: return("SQL_INTERVAL_DAY");
    case SQL_INTERVAL_HOUR: return("SQL_INTERVAL_HOUR");
    case SQL_INTERVAL_MINUTE: return("SQL_INTERVAL_MINUTE");
    case SQL_INTERVAL_SECOND: return("SQL_INTERVAL_SECOND");
    case SQL_INTERVAL_DAY_TO_HOUR: return("SQL_INTERVAL_DAY_TO_HOUR");
    case SQL_INTERVAL_DAY_TO_MINUTE: return("SQL_INTERVAL_DAY_TO_MINUTE");
    case SQL_INTERVAL_DAY_TO_SECOND: return("SQL_INTERVAL_DAY_TO_SECOND");
    case SQL_INTERVAL_HOUR_TO_MINUTE: return("SQL_INTERVAL_HOUR_TO_MINUTE");
    case SQL_INTERVAL_HOUR_TO_SECOND: return("SQL_INTERVAL_HOUR_TO_SECOND");
    case SQL_INTERVAL_MINUTE_TO_SECOND: return("SQL_INTERVAL_MINUTE_TO_SECOND");
    case SQL_GUID: return("SQL_GUID");
    default: return("UNKNOWN");
    }
}

/* runsql:
 *      executes SQL scripts for a given thread.
 *
 *      tid: thread id
 *      eid: execution number id
 *      ten: thread execution number
 *      script: script to run
 *
 *      return: 0 OK, 1 runsql() error, 2 Omexec() error and SOE is set
 */
static int runsql(int tid, int eid, int ten, char *script)
{
    register unsigned int j = 0;    /* loop variable */
    int n = 0,              /* command/script no for this thread */
        k = 1,              /* command no in a given script */
        q = 0,              /* flag for quoted string */
        ret = 0,            /* return value from Oexec */
        rv = 0,             /* return value from this function */
        ch = 0,             /* char read from file */
        cp = 0,             /* previous char read from file */
        nrag=0;             /* Number of rag[] arguments */
    unsigned int lcd = 0;   /* delay to start next command within a thread */
    FILE *fr = 0;           /* FILE pointer */
    SQLCHAR *Ocmd = 0;      /* Ocmd buffer */
    SQLCHAR *O = 0;         /* ODBC temp variable for realloc */
    size_t bs = CMD_CHUNK;  /* Ocmd buffer size */
    size_t bs1 = CMD_CHUNK; /* line buffer size */
    char *line = 0,         /* line to tokenize */
         *l = 0 ,           /* temp realloc var */
         *rag[MAX_ARGS];    /* arguments in a row */
    char ql[QLABEL_LEN];    /* Query label */
    unsigned long smr = etab[eid].mr;   /* save max records to be fetched */
    size_t sr = etab[eid].r;            /* save rowset */
    char stype = etab[eid].type;        /* save execution type */
    char *sns = etab[eid].ns;           /* save original null string */
    unsigned int sflg = etab[eid].flg;  /* save eid flg */

    /* Initialize Query Label */
    ql[0] = '\0';

    /* Initial Ocmd allocation */
    if ( (Ocmd = malloc ( bs ) ) == (void *)NULL ||
         (line = malloc ( bs1 ) ) == (void *)NULL ) {
        fprintf(stderr, "odb [runsql(%d)] - Error allocating memory buffers: [%d] %s\n",
            __LINE__, errno, strerror(errno));
        rv = 1;
        goto runsql_exit;
    }

    /* read from file and run commands */
    if ( ! strcmp ( script, "-" ) ) {
        fr = stdin;
    } else if ( (fr = fopen (script, "r")) == (FILE *) NULL ) {
        fprintf(stderr, "odb [runsql(%d)] - Cannot open %s: [%d] %s\n",
             __LINE__, script, errno, strerror(errno));
        rv = 1;
        goto runsql_exit;
    }
    for ( ch = cp = j = k = 0; ( ch = getc(fr) ) != EOF ; cp = ch ) {
        if ( !(f2 & 0001) && ch == 42 && cp == 47 && q == 0 ) { /* remove C style comments comments */
            do {
                cp = ch ;
                ch = getc ( fr ) ;
            } while ( ch != 47 || cp != 42 );
            j--;
        } else if ( ch == '-' && cp == '-' && q == 0 ) { /* remove douple hyphen comments */
            while ( ( ch = getc(fr) ) != EOF && ch != '\n');
            j--;    /* first '-' is already in Ocmd */
        } else if ( ch == ';' ) {       /* command end */
            Ocmd[j++] = (SQLCHAR)ch;
            Ocmd[j] = (SQLCHAR)'\0';
            Ocmd = (SQLCHAR *)var_exp ( (char *)Ocmd, &bs, &thps[tid].tva);
            if ( ! ( etab[eid].flg & 010000 ) )     /* if not quiet cmd on */
                mfprintf(stderr, etab[eid].fso , "[%d.%d.%d]%s: \'%s\'\n", 
                    tid, n, k, ( etab[eid].flg & 0010 ? "Preparing": "Executing" ), (char *)Ocmd);
            if ( thps[tid].cd2 )                        /* random delay between cd and cd2 */
                lcd = (unsigned int) ( thps[tid].cd + rand() / ( RAND_MAX + 1.0 ) * ( thps[tid].cd2 - thps[tid].cd + 1 ) );
            else
                lcd = thps[tid].cd;
            if ( k && lcd )
                Sleep (lcd);
            if ( ( ret = Omexec( tid, eid, ten, k++, Ocmd, ql, (char *)NULL, (char *)NULL) ) < 0 ) {
                if ( etab[eid].flg & 0004 ) {
                    rv = 2; /* error during execution & exit on error */
                    goto runsql_exit;
                } else {
                    fprintf ( stderr,
                        "odb [runsql(%d)] - [%d.%d.%d] Error received while executing %s\n",
                        __LINE__, tid, n, k, script);
                    fprintf ( stderr, "Offending Command: >%s<\n", Ocmd);
                }
            } 
            j = 0;      /* reset command position */
            ql[0]='\0'; /* clean query label */
        } else if ( ch == '\n' || ch == '\r' ) {
            if ( j ) {
                if ( (size_t) j >= bs1 ) {  /* line requires more memory */ 
                    bs1 = (size_t)j + 1; 
                    l = line ;
                    if ( ( l = realloc ( l, bs1 ) ) == (void *)NULL ) {
                            fprintf(stderr, "odb [runsql(%d)] - Error re-allocating memory for line: [%d] %s\n",
                                __LINE__, errno, strerror(errno));
                        rv = 1;
                        goto runsql_exit;
                    }
                    line = l ;
                }
                strmcpy ( line, (char *)Ocmd, j );
                // eliminate left spaces
                unsigned int alphaIndex = 0;
                while (alphaIndex < j && (line[alphaIndex] == ' ' || line[alphaIndex] == '\t')) ++alphaIndex;

                if (alphaIndex != j) { // skip blank line
                    if (!strmicmp(line, "odb ", 4)) {
                        snprintf((char *)Ocmd, bs, "%s -u %s -p %s %s%s %s%s %s",
                            odbcmd, (char *)Ouser, (char *)Opwd, Odsn[0] ? "-d " : "", Odsn[0] ? (char *)Odsn : "",
                            clca ? "-ca" : "", clca ? clca : "", line + 4);
#ifdef _WIN32
                        _spawnlp(_P_WAIT, "cmd.exe", "cmd.exe", "/c", var_exp((char *)Ocmd, &bs, &thps[tid].tva), NULL);
#else
                        if (system((const char *)var_exp((char *)Ocmd, &bs, &thps[tid].tva)) < 0)
                            fprintf(stderr, "odb [runsql(%d)] - Error running %s\n", __LINE__, &Ocmd[1]);
#endif
                        j = 0;
                    }
                    else {
                        nrag = tokenize(line, j, rag);
                        if (nrag && !strmicmp(rag[0], "set", 0)) {
                            setan(eid, tid, nrag, rag, ql);
                            j = 0;
                        }
                        else if (!strmicmp(rag[0], "print", 0)) {
                            var_exp(nrag ? rag[1] : "", 0, &thps[0].tva);
                            j = 0;
                        }
                        else {
                            Ocmd[j++] = (SQLCHAR)ch;
                        }
                    }
                }
            }
        } else {    /* ch is not a space or new line char */
            Ocmd[j++] = (SQLCHAR)ch;
            if ( ( ch == 34 || ch == 39 ) && cp != ec )
                q = q ? 0 : 1;
        }
        if ( ( j + 1 ) >= bs ) {    /* we need more memory for Ocmd */
            bs += CMD_CHUNK;
            O = Ocmd ;
            if ( ( O = realloc ( O, bs ) ) == (void *)NULL ) {
                    fprintf(stderr, "odb [runsql(%d)] - Error re-allocating memory for Ocmd: [%d] %s\n",
                        __LINE__, errno, strerror(errno));
                break;
            }
            Ocmd = O ;
        }
    }

    runsql_exit:
    if ( line )
        free ( line ) ;
    if ( Ocmd ) 
        free ( Ocmd ) ;
    if ( fr && fr != stdin )
        fclose(fr);

    /* restore original eid values */
    etab[eid].mr = smr;                 /* reset max fetch records */
    etab[eid].r = sr;                   /* reset rowset */
    etab[eid].flg = sflg;               /* reset command flag */
    etab[eid].type = stype;             /* reset execution type */
    if ( sns != etab[eid].ns ) {        /* if nullstring was changed */
        free ( etab[eid].ns );          /* free \"new\" allocated memory */
        etab[eid].ns = sns;             /* restore original nullstring */
    }
    return(rv);
}

/* ifempty:
 *      check if the table is empty
 *
 *      eid: etab idx
 *      table: table name
 *      return: 0 (table is empty), 1 (table is not empty)
 */
static unsigned int ifempty(int eid, char *table)
{
    SQLCHAR Ocmd[CMD_CHUNK];            /* ODBC Command buffer */
    unsigned int sflg = etab[eid].flg;  /* save eid flg */
    unsigned long smr = etab[eid].mr;   /* save max records to be fetched */
    size_t sr = etab[eid].r;            /* save rowset */
    int tid = etab[eid].id;             /* thread ID */

    Ocmd[0] = '\0';                     /* initialize Ocmd */

    /* build select command */
    (void) strmcat ( (char *) Ocmd, "SELECT * FROM ", sizeof(Ocmd), 0);
    (void) strmcat ( (char *) Ocmd, table, sizeof(Ocmd), 0);

    /* alter eid values */
    etab[eid].mr = (unsigned long)1 ;   /* fetch one row */
    etab[eid].r = (size_t)1;            /* use single row rowset */
    etab[eid].flg |= 0100000000;        /* set silent mode */

    /* run command */
    etab[eid].flg2 |= 020000000 ;           /* Oexec to allocate/use its own handle */
    (void) Oexec(tid, eid, 0, 0, Ocmd, "");
    etab[eid].flg2 &= ~020000000 ;          /* reset Oexec to allocate/use its own handle */

    /* reset eid values */
    etab[eid].mr = smr ;                /* set original maxfetch */
    etab[eid].r = sr;                   /* set original rowset */
    etab[eid].flg = sflg ;              /* set original flg */

    /* return */
    return(etab[eid].nr ? 1 : 0 );
}

static int comp2asc(char *srcfld, char *tgtfld,unsigned int srclen, unsigned int maxlen)
{
    unsigned int i = 0;
    unsigned int j = 0;
    unsigned int k = 0;
    unsigned int tgtlen = 0;
    unsigned char tmpx = 0;
    unsigned char tmpy = 0;
    unsigned long long result = 0;

    /* First we need to calculate target size */
    switch (srclen) {
    case (2):
        tgtlen = 5 + 1; /* num chars + sign */
        break;
    case (4):
        tgtlen = 10 + 1; /* num chars + sign */
        break;
    case (8):
        tgtlen = 19 + 1; /* num chars + sign */
        break;
    default:
        fprintf(stderr, "odb [comp2asc(%d)] - Invalid COMP field size (%u). Suitable size 2, 4 or 8 bytes\n",
            __LINE__, srclen);
        return (-2);
    }
    if (tgtlen > maxlen)
        return (-1);

    /* Now the real conversion */
    for ( i = srclen ; i > 0 ; i-- ) {
        for ( k = 8, tmpy = 1 ; k > 0 ; k--, j++, tmpy <<= 1 ) {
            tmpx = (unsigned char)(srcfld[i-1] & tmpy);
            if ( i == 1 && k == 1 ) /* sign always is the most significant bit */
                break;
            if (tmpx != 0)          /* it is not necessary to sum */
#ifdef _WIN32
                result += 1i64<<j;
#else
                result += 1<<j;
#endif
        }
    }
    if ( srclen == 8 ) {
#ifdef _WIN32
        result += 1i64<<j;
#else
        result += 1<<j;
#endif
        result >>= 1;
    }

    /* Fill output buffer & return */
    snprintf(tgtfld, tgtlen+1, "%c%0*llu", tmpx ? '-' : '+', tgtlen - 1 , result ); /* it is tgtlen +1 because of the last \0 */
    return ((int)tgtlen);
}

static int comp32asc(char * srcfld, char *tgtfld, unsigned int srclen, unsigned int maxlen, int precision, int scale)
{
    unsigned int i = 0;
    int numdec = 0;
    char sign = 0;
    unsigned char tmpc = 0;
    unsigned int tgtlen = 0;

    /* default values precision 5 and scale 0*/
    if (scale < 0)
        scale = 0;
    if (precision < 0)
        precision = 5;

    tgtlen = precision + 1 + 1; /* sign + decimal separator */
    if (tgtlen % 2 == 0 )       /* if odd number always plus 1 */
        tgtlen++;
    if (scale == 0) /* if there are no decimals remove "." char */
        tgtlen = tgtlen - 1;

    if (tgtlen > maxlen)
        return(-1);

    for (i=srclen; i>0; i--) {
        if (i==srclen) { /* first iteration. sign + less valuable digit */
            tmpc = (unsigned char)(srcfld[i-1] & 15 ); /* 15: 0000 FFFF */
            switch (tmpc) {
            case 10:
            case 12:
            case 14:
            case 15:
                sign = '+';
                break;
            case 11:
            case 13:
                sign = '-';
                break;
            default:
                fprintf(stderr, "odb [comp32asc(%d)] - Invalid COMP3 sign format b[%x]i[%d]\n",
                    __LINE__, srcfld[i-1], i-1 );
                return (-2);
            }
            tmpc = (unsigned char)(srcfld[i-1] & 240);  /*240: FFFF 0000 */
            tmpc = (unsigned char)(tmpc >> 4); 
            tgtfld[tgtlen-numdec-1] = tmpc + 48;
        } else {
            if (numdec == scale) {
                tgtfld[tgtlen-numdec-1] = '.';
                numdec++;
            }
            /* low */
            tmpc = 15;
            tmpc = (unsigned char)(srcfld[i-1] & tmpc);
            tgtfld[tgtlen-numdec-1] = tmpc + 48;
            numdec++;
            if (numdec == scale) {
                tgtfld[tgtlen-numdec-1] = '.';
                numdec++;
            }
            /* high */
            tmpc = 240;
            tmpc = (unsigned char)(srcfld[i-1] & tmpc);
            tmpc = (unsigned char)(tmpc >> 4);
            tgtfld[tgtlen-numdec-1] = tmpc + 48;
        }
        numdec++;
    }
    tgtfld[0] = sign;
    tgtfld[tgtlen] = 0;
    return(tgtlen);
}

static int zoned2asc(char * srcfld, char *tgtfld, unsigned int srclen, unsigned int maxlen, int precision, int scale)
{
    unsigned int i = 0;
    unsigned int tgtlen = 0;
    unsigned char tmpc = 128;
    unsigned int count = 0;

    if (precision < 0)
        precision = (int)srclen;
    if (scale <0)
        scale = 0;

    if (srclen != (unsigned int)precision) {
        fprintf(stderr, "odb [zoned2asc(%d)] - Error: ZONED field length different from precision\n",
            __LINE__);
        return -2;
    }

    tgtlen = (int)srclen + 1;
    if (scale > 0)
        tgtlen++;

    if (tgtlen > maxlen)
        return -1;

    /* Nonstop implementation highest bit on the last byte = sign */
    /* sign */
    tmpc = (char)(tmpc & srcfld[srclen-1]);
    if (tmpc > 0) {
        tgtfld[count++]='-'; /* F000 0000 */
    } else {
        tgtfld[count++]='+'; /* 0000 0000 */
    }

    for (i=0;i<srclen;i++) {
        if (count == (unsigned int)(precision - scale + 1))
        tgtfld[count++] = '.';
        if (i == srclen-1 ) {
            tgtfld[count++] = (char)(srcfld[i] & 127); /* 0FFF FFFF */
        } else {
            if (srcfld[i] < 48 || srcfld[i] > 57) {
                fprintf(stderr, "odb [zoned2asc(%d)] - Error: invalid ZONED character (%d)\n",
                    __LINE__, srcfld[i]);
                return -2;
            }
            tgtfld[count++] = srcfld[i];
        }
    }
    tgtfld[tgtlen] = 0;
    return((int)tgtlen);
}

/* Otcol:
 *      analyze table in etab[eid].Ocso[0-2] and fills etab[eid].td[]
 *
 *      eid: etab[] index
 *
 *      return: number of columns in table or -1 if errors
 */
static int Otcol(int eid, SQLHDBC *Ocn)
{
    SQLCHAR Osel[512];          /* Select statement buffer */
    SQLHSTMT Ostmt=0;           /* ODBC Statement Handle */
    SQLCHAR Ocname[MAXCOL_LEN]; /* Temporary Column Name pointer */
    SQLSMALLINT Onamel;         /* ODBC column name length returned by SQLDescribeCol */
    SQLUSMALLINT Oncol;         /* ODBC number of columns in the result set */
    unsigned int i = 0;         /* loop variable */
    SQLRETURN Or;               /* ODBC return value */
    int tid = eid < 0 ? -1 : etab[eid].id; /* Thread ID */
    int ret = 0;                /* return value */

    /* Build select statement */
    (void)snprintf((char *)Osel, sizeof(Osel), "SELECT * FROM %s%s%s%s%s WHERE 1 = 0",
        etab[eid].Ocso[0] ? (char *)etab[eid].Ocso[0] : "", (char *)etab[eid].Ocso[0] ? "." : "",
        etab[eid].Ocso[1] ? (char *)etab[eid].Ocso[1] : "", (char *)etab[eid].Ocso[1] ? "." : "",
        (char *)etab[eid].Ocso[2] );

    /* Allocate statement handle */
    if (!SQL_SUCCEEDED(Or=SQLAllocHandle(SQL_HANDLE_STMT, *Ocn, &Ostmt))){
        Oerr(0, 0, __LINE__, *Ocn, SQL_HANDLE_DBC);
        ret = -1;
        goto otcol_exit;
    }

    /* Prepare dummy select */
    if ((Or=SQLPrepare(Ostmt, Osel, SQL_NTS)) != SQL_SUCCESS) {
        Oerr(eid, tid, __LINE__, Ostmt, SQL_HANDLE_STMT);
        if ( Or != SQL_SUCCESS_WITH_INFO ) {
            ret = -1;
            goto otcol_exit;
        }
    }

    /* Get number of resulting cols */
    if (!SQL_SUCCEEDED(Or=SQLNumResultCols(Ostmt, (SQLSMALLINT *)&Oncol))) {
        Oerr(eid, tid, __LINE__, Ostmt, SQL_HANDLE_STMT);
        ret = -1;
        goto otcol_exit;
    }

    /* Allocate struct tdesc array */
    if ( ( etab[eid].td = calloc ( (size_t)Oncol, sizeof(struct tdesc) ) ) == (void *)NULL ) {
        fprintf(stderr, "odb [Otcol(%d)] - Error allocating %u memory elements for struct tdesc\n",
            __LINE__, (unsigned int)Oncol);
        ret = -1;
        goto otcol_exit;
    }

    /* Get column attributes and fill td structure */
    for ( i = 0 ; i < (unsigned int)Oncol ; i++ ) {
        if ( !SQL_SUCCEEDED(Or=SQLDescribeCol(Ostmt, (SQLUSMALLINT)(i+1),
                Ocname, (SQLSMALLINT)MAXCOL_LEN, &Onamel,
                &etab[eid].td[i].Otype, &etab[eid].td[i].Osize,
                &etab[eid].td[i].Odec, &etab[eid].td[i].Onull)) ) {
            Oerr(eid, tid, __LINE__, Ostmt, SQL_HANDLE_STMT);
            ret = -1;
            goto otcol_exit;
        }
        if(!SQL_SUCCEEDED(Or=SQLColAttribute(Ostmt, (SQLUSMALLINT)(i+1),
                SQL_DESC_DISPLAY_SIZE, (SQLPOINTER) NULL, (SQLSMALLINT) 0,
                (SQLSMALLINT *) NULL, (SQLPOINTER) &etab[eid].td[i].Osize))) {
            Oerr(eid, tid, __LINE__, Ostmt, SQL_HANDLE_STMT);
            ret = -1;
            goto otcol_exit;
        }
        switch ( etab[eid].td[i].Otype ) {
        case SQL_TYPE_TIME:
            break;
        case SQL_WCHAR:
        case SQL_WVARCHAR:
        case SQL_WLONGVARCHAR:
        case SQL_CHAR:
        case SQL_VARCHAR:
        case SQL_LONGVARCHAR:
            if ( etab[eid].Omaxl && etab[eid].Omaxl < etab[eid].td[i].Osize )
                etab[eid].td[i].Osize = etab[eid].Omaxl;
            break;
        }
        if ( ( etab[eid].td[i].Oname = (SQLCHAR *)malloc((size_t)Onamel+1) ) == (void *)NULL ) {
            fprintf(stderr, "odb [Otcol(%d)] - Error allocating %d bytes as column name buffer for %s: [%d] %s\n",
                __LINE__, (int)Onamel+1, (char *)Ocname, errno, strerror(errno));
            goto otcol_exit;
        }
        strmcpy((char *)etab[eid].td[i].Oname, (char *)Ocname, (size_t)Onamel);
    }
    ret = (int)Oncol;

    /* Free handle and exit */
    otcol_exit:
    (void)SQLFreeHandle(SQL_HANDLE_STMT, Ostmt);
    return ( ret );
}

/* parseopt:
 *      parse options for copy/extract/load/diff and fill etab[no] structure
 *
 *      type(I): type of action
 *      str(I): string to parse
 *      len(I): string length
 *      rp(I): remember position (0=no, 1=yes)
 *
 *      return: 0=parse ok, 1=unknown parameter, 2=no more options to parse
 */
 static unsigned int parseopt(char type, char *str, unsigned int len, unsigned int rp)
 {
    unsigned int j = 0,     /* loop variable */
                 l = 0,     /* loop variable */
                 n = 0,     /* loop variable */
                 i = 0,     /* loop variable */
                 o = 0,     /* loop variable */
                 k = 0,     /* loop variable */
                 eol = 0;   /* end of line indicator */
    char q = 0;             /* quoted pwhere: [...] */
    char *p = 0;            /* loop variable */
    static unsigned int pos = 0;    /* next string start when parsing file with lines */

    while (isspace((int)*(str+pos)) || *(str+pos)=='#') {
        while (isspace((int)*(str+pos)))/* skip spaces and empty lines */
            pos++;
        while ( *(str+pos) == '#' ) {   /* skip comments */
            while ( *(str + pos) && *(str + (++pos)) != '\n' ) ;
            if ( *(str+pos) == '\n' )
                pos++;
        }
    }
    if ( pos >= len )               /* no more options to parse */
        return ( 2 ) ;

    tmpre = 0 ;                     /* reset tmpre */
    for ( q = l = n = o = 0, j=pos; j < len ; j++) { /* n points param name, l points value */
        switch ( o ) {
            case 0: /* looking for param name */
                for ( n = j ; str[j] && str[j] != '=' && str[j] != ':' && str[j] != '\n'; j++);
                if ( str[j] == ':' ) {          /* param with no value */
                    str[j]='\0';
                    o = 2;
                } else if ( str[j] == '\n' ) {  /* param with no value && EOL */
                    str[j]='\0';
                    eol = 1;
                    o = 2;
                    for ( k = n ; k < j ; k++ ) {   /* remove trailing spaces/comments */
                        if ( isspace((int)str[k]) || str[k]=='#' ) {
                            str[k] = '\0';
                            break;
                        }
                    }
                } else {                        /* param with value */
                    o = 1;
                }
                /* FALLTHRU */
            case 1: /* looking for value */
                if ( str[j+1] == '[' ) {
                    j++; 
                    q = 1;
                }
#ifdef _WIN32
                for (l = j; (str[j] && str[j] != '\n') && (q || str[j] != ':') || str[j + 1] == '/'; j++) {
#else
                for (l = j; (str[j] && str[j] != '\n') && (q || str[j] != ':'); j++) {
#endif
                    if ( q ) {
                        if ( str[j] == ']' && ( str[j+1] == ':' || str[j+1] == 0 || isspace((int)str[j+1]) ) ) {
                            q = 0;
                            str[j]= '\0';
                        }
                    } else {
                        if ( isspace((int)str[j]) || str[j]=='#' )
                            str[j] = '\0';
                    }
                }
                if ( str[j] == ':' ) {
                    str[j]='\0';
                } else if ( str[j] =='\n' ) {
                    eol = 1;
                    str[j]='\0';
                }
                o = 2;
                /* FALLTHRU */
            case 2: /* fill etab structure */
                if ( str[l] == '=' ) {
                    str[l++] = '\0';
                } else if ( str[l] == '[' && str[l-1] == '=' ) {
                    str[l-1] = '\0';
                    str[l++] = '\0';
                }
                if ( !strcmp(&str[n], "src") ) {
                    if ( !strncmp(&str[l], "hdfs", 4) || !strncmp(&str[l], "mapr", 4) ) {
#ifdef HDFS
                        if ( str[l+4] == '@' )
                            etab[no].src = parsehdfs(no, &str[l] + 5) ;
                        else
                            etab[no].src = &str[l] + 5;
                        etab[no].flg2 |= 0100;
                        if ( !strncmp(&str[l], "mapr", 4 ) )
                            etab[no].flg2 |= 01000000000 ;
#else
                        fprintf(stderr, "odb [parseopt(%d)] - HDFS not supported by this odb executable\n", __LINE__);
                        return ( 1 ) ;
#endif
                    } else {
                        etab[no].src = &str[l];
                    }
                } else if ( !strcmp(&str[n], "tgt") ) {
                    if ( !strncmp(&str[l], "hdfs", 4) || !strncmp(&str[l], "mapr", 4) ) {
#ifdef HDFS
                        if ( str[l+4] == '@' )
                            etab[no].tgt = parsehdfs(no, &str[l] + 5) ;
                        else
                            etab[no].tgt = &str[l] + 5;
                        etab[no].flg2 |= 0100;
                        if ( !strncmp(&str[l], "mapr", 4 ) )
                            etab[no].flg2 |= 01000000000 ;
#else
                        fprintf(stderr, "odb [parseopt(%d)] - HDFS not supported on this odb executable\n", __LINE__);
                        return ( 1 ) ;
#endif
                    } else {
                        etab[no].tgt = &str[l];
                    }
                } else if ( !strcmp(&str[n], "pre") ) {
                    etab[no].pre = &str[l];
                    etab[no].lstat |= 0004;     /* this works for both octal mask & decimal lstat interpretation */
                } else if ( !strcmp(&str[n], "post") ) {
                    etab[no].post = &str[l];
                } else if ( !strcmp(&str[n], "time") ) {
                    etab[no].flg2 |= 020000 ;
                } else if ( ( type == 'e' || type == 'c' || type == 'p' ) && !strcmp(&str[n], "mpre") ) {
                    etab[no].mpre = &str[l];
                } else if ( ( type == 'c' || type == 'p' ) && !strcmp(&str[n], "tmpre") ) {
                    tmpre = &str[l];
                } else if ( ( type == 'e' || type =='c' || type == 'p' ) &&  !strcmp(&str[n], "sql") ) {
                    etab[no].sql = &str[l];
                } else if ( type == 'l' && !strcmp(&str[n], "map") ) {
                    etab[no].map = &str[l];
                    etab[no].flg |= 01000000 ;  /* complex load. Use Oload */
                } else if ( (type == 'l' || type == 'c') && !strcmp(&str[n], "bad") ) {
                    etab[no].bad = &str[l];
                } else if ( type == 'c' && !strcmp(&str[n], "errdmp") ) {
                    if ( !(strcmp(&str[l], "stdout") ) ) {
                        etab[no].fdmp = stdout ;
                    } else if ( !(strcmp(&str[l], "stderr") ) ) {
                        etab[no].fdmp = stderr ;
                    } else {
                        if ( ( etab[no].fdmp = fopen(&str[l], "w") ) == (FILE *)NULL )
                            fprintf(stderr, "odb [parseopt(%d)] - Cannot open %s: [%d] %s\n",
                                __LINE__, &str[l], errno, strerror(errno));
                    }
                } else if ( type != 'c' && !strcmp(&str[n], "fs") ) {
                    etab[no].fs = mchar(&str[l]);
                } else if ( type != 'c' && !strcmp(&str[n], "mcfs") ) {
                    etab[no].mcfs = replace_escapes(&str[l]);
                    etab[no].flg |= 01000000; /* complex load. Use Oload */
                } else if ( type != 'c' && !strcmp(&str[n], "rs") ) {
                    etab[no].rs = mchar(&str[l]);
                } else if ( type != 'c' && !strcmp(&str[n], "mcrs") ) {
                    etab[no].mcrs = replace_escapes(&str[l]);
                    etab[no].flg |= 01000000; /* complex load. Use Oload */
                } else if ( ( type == 'e' || type == 'l' ) && !strcmp(&str[n], "ec") ) {
                    etab[no].ec = mchar(&str[l]);
                    etab[no].flg |= 01000000 ;  /* complex load. Use Oload */
                } else if ( type == 'l' && !strcmp(&str[n], "pc") ) {
                    etab[no].pc = mchar(&str[l]);
                    etab[no].flg |= 01000000 ;  /* complex load. Use Oload */
                } else if ( type == 'l' && !strcmp(&str[n], "em") ) {
                    etab[no].em = mchar(&str[l]);
                    etab[no].flg |= 01000000 ;  /* complex load. Use Oload */
                } else if ( ( type == 'e' || type == 'l' ) && !strcmp(&str[n], "sq") ) {
                    etab[no].sq = mchar(&str[l]);
                    etab[no].flg |= 01000000 ;  /* complex load. Use Oload */
                } else if ( !strcmp(&str[n], "soe") ) {
                    etab[no].flg |= 0004;
                } else if ( type == 'c' && !strcmp(&str[n], "roe") ) {
                    if (str[l]) {
                        etab[no].roe = atoi(&str[l]);
                    } else {
                        etab[no].roe = 3 ;
                    }
                } else if ( type == 'c' && !strcmp(&str[n], "roedel") ) {
                    etab[no].roedel = atoi(&str[l]);
                } else if ( !strcmp(&str[n], "nomark") ) {
                    etab[no].flg &= ~0400;
                } else if ( !strcmp(&str[n], "max") ) {
                    etab[no].mr = strtol(&str[l], (char **)NULL, 10);
                    etab[no].flg |= 01000000 ;  /* complex load. Use Oload */
                } else if ( !strcmp(&str[n], "errmax") ) {
                    etab[no].mer = atoi(&str[l]);
                } else if ( !strcmp(&str[n], "tpar") ) {
                    if ( !rp )
                        tn = atoi(&str[l]);
                } else if ( !strcmp(&str[n], "parallel") ) {
                    if ( !rp )  /* this is a command-line only option */
                        etab[no].ps = (unsigned int) atoi(&str[l]);
                } else if ( ( type == 'c' || type == 'p' ) && !strcmp(&str[n], "loaders") ) {
                    if ( !rp )  /* this is a command-line only option */
                        etab[no].nloader = (unsigned int) atoi(&str[l]);
                } else if ( !strcmp(&str[n], "bpwc") ) {
                    etab[no].bpwc = (unsigned int) atoi(&str[l]);
                } else if ( !strcmp(&str[n], "bpc") ) {
                    etab[no].bpc = (unsigned int) atoi(&str[l]);
                } else if ( type == 'e' && !strcmp(&str[n], "multi") ) {
                    etab[no].flg |= 01000;
                } else if ( type == 'l' && !strcmp(&str[n], "skip") ) {
                    etab[no].k = atoi(&str[l]);
                } else if ( type == 'l' && !strcmp(&str[n], "fieldtrunc") ) {
                    etab[no].fldtr = (char)atoi(&str[l]);
                    if ( etab[no].fldtr > 5 ) {
                        etab[no].fldtr = 0;
                        fprintf(stderr, "odb [parseopt(%d)] - Invalid fieldtrunc (%s) reset to 0\n",
                                        __LINE__, &str[l] );
                    }
                    etab[no].flg |= 01000000 ;  /* complex load. Use Oload */
                } else if (!strcmp(&str[n], "ns") ) {
                    etab[no].ns = &str[l];
                    etab[no].nl = (unsigned int ) strlen( etab[no].ns );
                    etab[no].flg |= 01000000 ;  /* complex load. Use Oload */
                } else if ( ( type == 'e' || type == 'd' ) && !strcmp(&str[n], "es") ) {
                    etab[no].es = &str[l];
                    etab[no].el = (unsigned int ) strlen( etab[no].es );
                } else if ( ( type == 'l' || type == 'c' ) && !strcmp(&str[n], "norb") ) {
                    etab[no].flg2 |= 0002;
                } else if ( ( type == 'c' || type == 'l' ) && !strcmp(&str[n], "truncate") ) {
                    etab[no].flg |= 0002;
                    etab[no].lstat = 4;
                } else if ( ( type == 'c' || type == 'l' ) && !strcmp(&str[n], "ifempty") ) {
                    etab[no].flg |= 0200000;
                    etab[no].lstat = 4;
                } else if ( type == 'l' &&  !strcmp(&str[n], "full") ) {
                    etab[no].flg |= 01000000 ;  /* complex load. Use Oload */
                } else if ( type != 'l' && !strcmp(&str[n], "uncommitted") ) {
                    etab[no].flg |= 0001;
                } else if ( type == 'e' && !strcmp(&str[n], "gzip") ) {
                    etab[no].flg |= 0020;
                    if ( str[l] ) {
                        if ( str[l] >= '0' && str[l] <= '9' ) {
                            etab[no].gzlev = (int) ( str[l] - '0' ) ;
                        } else {
                            fprintf(stderr, "odb [parseopt(%d)] - Invalid gzip level (%s) reset to default\n",
                                        __LINE__, &str[l] );
                            etab[no].gzlev = Z_DEFAULT_COMPRESSION ;
                        }
                    } else {
                        etab[no].gzlev = Z_DEFAULT_COMPRESSION ;
                    }
                } else if ( ( type == 'c' || type == 'e' || type == 'd' ) && !strncmp(&str[n], "trim", 4) ) {
                    etab[no].flg |= 0400000000;
                    if ( str[n+4] == '+' )
                        etab[no].flg2 |= 040000000 ;
                } else if ( ( type == 'c' || type == 'e' || type == 'd' ) && !strncmp(&str[n], "rtrim", 5) ) {
                    etab[no].flg |= 02000000 ;
                    if ( str[n+5] == '+' ) {
                        etab[no].flg |= 04000000 ;
                    }
                } else if ( ( type == 'e' || type == 'c' ) && !strcmp(&str[n], "cast") ) {
                    etab[no].flg |= 01000000000;
                } else if ( type == 'e' && !strcmp(&str[n], "binary") ) {
                    etab[no].flg |= 0100;
                } else if ( type != 'l' && !strcmp(&str[n], "splitby") ) {
                    etab[no].sb = &str[l];
                } else if ( type == 'd' && !strcmp(&str[n], "quick") ) {
                    etab[no].flg2 |= 0010;
                } else if ( ( type == 'c' || type == 'p' ) && !strcmp(&str[n], "bind") ) {
                    if ( !strcmp(&str[l], "auto") ) {
                        etab[no].flg2 |= 0200000;
                    } else if ( !strcmp(&str[l], "char") ) {
                        etab[no].flg2 &= ~0200000;  /* auto off */
                        etab[no].flg2 |= 04000;     /* char on */
                    } else if ( !strcmp(&str[l], "cdef") ) {
                        etab[no].flg2 &= ~0200000;  /* auto off */
                        etab[no].flg2 &= ~04000;    /* char off */
                    } else {
                        fprintf(stderr, "odb [parseopt(%d)] - Invalid bind (%s) reset to auto\n",
                                        __LINE__, &str[l] );
                        etab[no].flg2 |= 0200000;
                    }
                } else if ( type == 'd' && !strcmp(&str[n], "key") ) {
                    etab[no].pc = 1 ;
                    p = &str[l];
                    while ( *p++ )
                        if ( *p == ',' )
                            etab[no].pc++;
                    etab[no].key[0] = p = &str[l] ; 
                    for ( i = 1 ; *p && i < MAX_PK_COLS; p++ ) {
                        if ( *p == ',' && *(p+1) != ':' ) {
                             *p++ = '\0';
                             etab[no].key[i++] = p;
                        }
                    }
                } else if ( type == 'd' && !strcmp(&str[n], "output") ) {
                    etab[no].run = &str[l];
                } else if ( type == 'd' && !strcmp(&str[n], "print") ) {
                    etab[no].flg &= ~016000000000;  /* reset default flags (IDC) */
                    p = &str[l];
                    if ( *p && l < j ) {
                        do {
                            switch ( *p ) {
                                case 'c':           /* Print 'C'hanged records */
                                case 'C':
                                    etab[no].flg |= 010000000000;
                                    break;
                                case 'd':           /* Print 'D'eleted source records */
                                case 'D':           
                                    etab[no].flg |= 04000000000;
                                    break;
                                case 'i':           /* Print 'I'nserted target records */
                                case 'I':
                                    etab[no].flg |= 02000000000;
                                    break;
                                default:            /* invalid character */
                                    fprintf(stderr, "odb [parseopt(%d)] - Invalid print diff option \'%c\'\n",
                                        __LINE__, *p );
                            }
                        } while ( *++p ) ;
                    }
                } else if ( type != 'l' && !strcmp(&str[n], "pwhere") ) {
                    etab[no].map = &str[l];
                } else if ( type == 'e' && !strcmp(&str[n], "gzpar") ) {
                    etab[no].gzp = &str[l];
                } else if ( type == 'e' && !strcmp(&str[n], "pcn") ) {
                    etab[no].flg |= 0200;
                } else if ( ( type == 'l' || type == 'e' ) && !strcmp(&str[n], "iobuff") ) {
                    etab[no].flg2 |= 0400000000 ;   /* iobuff has been set */
                    switch(str[l]){
                        case 'k':
                        case 'K':
                            etab[no].iobuff = (size_t)( 1024 * strtol(&str[l+1], (char **)NULL, 10) );
                            break;
                        case 'm':
                        case 'M':
                            etab[no].iobuff = (size_t)( 1024 * 1024 * strtol(&str[l+1], (char **)NULL, 10) );
                            break;
                        case '0':
                        case '1':
                        case '2':
                        case '3':
                        case '4':
                        case '5':
                        case '6':
                        case '7':
                        case '8':
                        case '9':
                            etab[no].iobuff = (size_t)strtol(&str[l], (char **)NULL, 10);
                            break;
                        default:
                            fprintf(stderr, "odb [parseopt(%d)] - Invalid iobuff \'%s\'\n",
                                __LINE__, &str[l] );
                            no = tn = 0;        /* reset tn */
                            return(1);
                    }
                } else if ( !strcmp(&str[n], "rows") ) {
                    switch(str[l]){
                        case 'k':
                        case 'K':
                            etab[no].rbs = (size_t)( 1024 * strtol(&str[l+1], (char **)NULL, 10) );
                            break;
                        case 'm':
                        case 'M':
                            etab[no].rbs = (size_t)( 1024 * 1024 * strtol(&str[l+1], (char **)NULL, 10) );
                            break;
                        case '0':
                        case '1':
                        case '2':
                        case '3':
                        case '4':
                        case '5':
                        case '6':
                        case '7':
                        case '8':
                        case '9':
                            etab[no].r = (size_t)strtol(&str[l], (char **)NULL, 10);
                            break;
                        default:
                            fprintf(stderr, "odb [parseopt(%d)] - Invalid rows \'%s\'\n",
                                __LINE__, &str[l] );
                            no = tn = 0;        /* reset tn */
                            return(1);
                    }
                } else if ( ( type == 'l' || type == 'e' ) && !strcmp(&str[n], "buffsz") ) {
                    switch(str[l]){
                        case 'k':
                        case 'K':
                            etab[no].buffsz = (size_t)( 1024 * strtol(&str[l+1], (char **)NULL, 10) );
                            break;
                        case 'm':
                        case 'M':
                            etab[no].buffsz = (size_t)( 1024 * 1024 * strtol(&str[l+1], (char **)NULL, 10) );
                            break;
                        case '0':
                        case '1':
                        case '2':
                        case '3':
                        case '4':
                        case '5':
                        case '6':
                        case '7':
                        case '8':
                        case '9':
                            etab[no].buffsz = strtol(&str[l], (char **)NULL, 10) ;
                            break;
                        default:
                            fprintf(stderr, "odb [parseopt(%d)] - Invalid buffsz \'%s\'\n",
                                __LINE__, &str[l] );
                            no = tn = 0;        /* reset tn */
                            return(1);
                    }
#ifdef HDFS
                } else if ( !strcmp(&str[n], "hblock") ) {
                    switch(str[l]){
                        case 'k':
                        case 'K':
                            etab[no].hblock = (tSize)( 1024 * strtol(&str[l+1], (char **)NULL, 10) );
                            break;
                        case 'm':
                        case 'M':
                            etab[no].hblock = (tSize)( 1024 * 1024 * strtol(&str[l+1], (char **)NULL, 10) );
                            break;
                        case '0':
                        case '1':
                        case '2':
                        case '3':
                        case '4':
                        case '5':
                        case '6':
                        case '7':
                        case '8':
                        case '9':
                            etab[no].hblock = (tSize)strtol(&str[l], (char **)NULL, 10) ;
                            break;
                        default:
                            fprintf(stderr, "odb [parseopt(%d)] - Invalid hblock \'%s\'\n",
                                __LINE__, &str[l] );
                            no = tn = 0;        /* reset tn */
                            return(1);
                    }
                    if ( etab[no].r <=0 && etab[no].rbs <=0 ) {
                        fprintf(stderr, "odb [parseopt(%d)] - Warning: invalid parameter \"%s\"; rows set to 100\n",
                             __LINE__, &str[n]);
                        etab[no].r = 100;
                    }
#endif
                } else if ( (type == 'e' || type == 'l') && !strcmp(&str[n], "maxlen") ) {
                    etab[no].Omaxl = (SQLULEN)atoi(&str[l]);
                } else if ( ( type == 'c' || type == 'l' || type == 'p' ) && !strcmp(&str[n], "commit") ) {
                    if ( !strcmp(&str[l], "auto") ) {
                        etab[no].cmt = 0;
                    } else if ( !strcmp(&str[l], "end") ) {
                        etab[no].cmt = -1;
                    } else if ( str[l] == 'x' && ( str[l+1] >= '0' && str[l+1] <= '9' ) ) {
                        etab[no].flg2 |= 0001 ;
                        etab[no].cmt = atoi(&str[l+1]);
                    } else {
                        etab[no].cmt = atoi(&str[l]);
                    }
                } else if ( ( type == 'c' || type == 'l' ) && !strcmp(&str[n], "direct") ) {
                    etab[no].flg |= 040000000;
                } else if ( ( type == 'c' || type == 'e' ) && !strcmp(&str[n], "ucs2toutf8") ) {
                    f |= 0400000000;            /* set global flag ucs2toutf8 */
                    if ( !strcmp(&str[l], "skip") ) {
                        etab[no].bucs2 = 0;
                    } else if ( !strcmp(&str[l], "force") ) {
                        etab[no].bucs2 = 1;
                    } else if ( !strcmp(&str[l], "cpucs2") ) {
                        etab[no].bucs2 = 2;
                    } else if ( !strcmp(&str[l], "qmark") ) {
                        etab[no].bucs2 = 3;
                    } else if ( str[l] ) {
                    } else if ( str[l] ) {
                        fprintf(stderr, "odb [parseopt(%d)] - Warning: invalid ucs2toutf8 parameter \"%s\"; reset to \"skip\"\n",
                            __LINE__, &str[l] );
                    }
                    etab[no].flg |= 010000000;
                    etab[no].flg2 |= 010000;    /* forcing wide char */
                } else if ( type == 'l' && !strcmp(&str[n], "show") ) {
                    etab[no].flg2 |= 0020;
                    etab[no].flg |= 01000000 ;  /* complex load. Use Oload */
                } else if ( ( type == 'e' || type == 'c' ) && !strcmp(&str[n], "cols") ) {
                    if ( str[l] == '-' ) {
                        etab[no].flg2 |= 0040 ;
                        etab[no].cols = &str[l+1];
                    } else {
                        etab[no].cols = &str[l];
                    }
                } else if ( type == 'p' && !strcmp(&str[n], "tgtsql") ) {
                    etab[no].ns = &str[l];
                } else if ( type == 'd' && !strcmp(&str[n], "odad") ) {
                    etab[no].flg2 |= 01000000 ;
#ifdef XML
                } else if ( type == 'l' && !strcmp(&str[n], "xmltag") ) {
                    if ( str[l] == '+' ) {  /* xml fields as attributes */
                        etab[no].flg2 |= 02000000 ;
                        etab[no].xrt = &str[l+1] ;
                    } else {                /* xml fields as text elements */
                        etab[no].xrt = &str[l] ;
                    }
                } else if ( type == 'l' && !strcmp(&str[n], "xmlord") ) {
                    etab[no].flg2 |= 010000000 ;
                } else if ( type == 'l' && !strcmp(&str[n], "xmldump") ) {
                    etab[no].flg2 |= 0200000000 ;
#endif
                } else if ( type == 'e' && !strcmp(&str[n], "xml") ) {
                    etab[no].flg2 |= 04000000 ;
                    etab[no].lstat |= 0001 ;
                } else if ( ( type == 'e' || type == 'l' ) && !strcmp(&str[n], "xmltype") ) {
                    if ( !strcmp(&str[l], "attr") ) {
                        etab[no].flg2 |= 02000000;
                    } else if ( !strcmp(&str[l], "text") ) {
                        etab[no].flg2 &= ~02000000;
                    } else {
                        fprintf(stderr, "odb [parseopt(%d)] - Warning: invalid xmltype \"%s\"; reset to \"text\"\n",
                             __LINE__, &str[l]);
                        etab[no].flg2 &= ~02000000;
                    }
                } else if ( type == 'c' && !strcmp(&str[n], "seq") ) {
                    p = &str[l];
                    etab[no].seqp = (unsigned int)strtol(p, (char **)NULL, 10);
                    while ( *p && *p != ',' && *p != ':' ) p++ ;
                    if ( p ) 
                        etab[no].seq = (unsigned long)strtol(++p, (char **)NULL, 10);
                } else if ( type == 'c' && !strcmp(&str[n], "seqstart") ) {
                    etab[no].seq = (unsigned long long int)strtol(&str[l], (char **)NULL, 10);
                } else if ( (type == 'l' || type == 'c') && !strcmp(&str[n], "loadcmd") ) {
                    //etab[no].loadcmd = &str[l];
                    strncpy(etab[no].loadcmd, &str[l], 2);
                    etab[no].loadcmd[2] = '\0';
                } else if (type == 'l' && !strcmp(&str[n], "jsonkey")) {
                    fprintf(stderr, "odb [parseopt(%d)] - Warning: this function is not fully tested \"%s\"\n", __LINE__, &str[n]);
                    etab[no].jsonKey = &str[n];
                }
                else {
                    fprintf(stderr, "odb [parseopt(%d)] - Error: unknown parameter \"%s\"\n",
                        __LINE__, &str[n]);
                    no = tn = 0;        /* reset tn */
                    return (1);
                }
                o = 0;  /* now look for a param name */
        }
        if ( eol ) 
            break;
    }
    if ( rp )           /* if "remember position" is set */
        pos = j + eol;  /* save string position */
    return(0);
}

#ifdef HDFS
/* parsehdfs:
 *      parse HDFS src/tgt string "host,port,user.file" and fill variables hdfshost, hdfsport,hdfsuser
 *      in the execution table returning the pointer to "file"
 *
 *      eid(I): execution table index
 *      hdfs(I): string to parse
 *      return: pointer to file name
 */
static char *parsehdfs(int eid, char *hdfs)
{
    unsigned int i = 0 ;    /* loop variable */
    char *s = hdfs ;        /* bookmark */
    unsigned int uf = 0;    /* user string flag: 1=present, 0=not present */

    /* look for host */
    for ( i = 0 ; hdfs[i] && hdfs[i] != ',' ; i++ ) ;
    if ( hdfs[i] == ',' ) {
        hdfs[i] = '\0' ;        /* replace ',' with NULL to terminate hdfshost */
        etab[eid].hdfshost = s; /* save hdfshost */
    } else {
        return ( 0 ) ;
    }

    /* look for port */
    for ( s = &hdfs[++i] ; hdfs[i] && hdfs[i] != ',' && hdfs[i] != '.' ; i++ ) ;
    switch ( hdfs[i] ) {
    case ',' :
        uf = 1 ;    /* optional user string present */
        /* FALLTHRU */
    case '.':
        hdfs[i] = '\0' ;        /* replace ',' with NULL to terminate hdfsport */
        etab[eid].hdfsport = (tPort)atoi(s);    /* save hdfsport */
        break;
    default:
        return ( 0 ) ;
    }

    /* look for user */
    if ( uf ) {
        for ( s = &hdfs[++i] ; hdfs[i] && hdfs[i] != '.' ; i++ ) ;
        if ( hdfs[i] == '.' ) {
            hdfs[i] = '\0' ;        /* replace '.' with NULL to terminate hdfsuser */
            etab[eid].hdfsuser = s; /* save hdfsuser */
        } else {
            return ( 0 ) ;
        }
    }
    return(&hdfs[++i]);
}
#endif

/* excol:
 *      expand column table column names for :cast, :cols and :trim options
 *
 *      return: pointer to the string containing the columns separated by comma
 */
static void excol(int eid, SQLHDBC *Ocn)
{
    char *cols = 0;         /* pointer to the exclude column */
    char *p = 0;            /* pointer to single exclude col name in col */
    size_t sz = LINE_CHUNK; /* size of the memory allocated for the included column */ 
    size_t used = 0 ;       /* space used for the included columns */
    size_t clen = 0;        /* single exclkuded column name length */
    unsigned int flg = 0,   /* 1 if a tdesc structure is allocated */
                 i = 0,     /* loop variable */
                 j = 0;     /* loop variable */

    if ( !etab[eid].td ) {  /* get tdesc structure for this eid if not available */
        if ( ( i = Otcol(eid, Ocn) ) <= 0 ) {
            fprintf(stderr, "odb [excol(%d)] - Error analyzing source table %s\n", 
                __LINE__, etab[eid].src);
            goto excol_exit;
        }
        flg = 1 ;
    } else {
        i = etab[no].cmt ;
    }

    if ( ( cols = malloc ( sz ) ) == (void *) NULL ) {
        fprintf(stderr, "odb [excol(%d)] - Error Allocating LINE_CHUNK memory for colnames: [%d] %s\n",
            __LINE__, errno, strerror(errno));
        goto excol_exit;
    }
    cols[0] = '\0' ;    /* clean cols */
    for ( j = 0 ; j < i ; j++ ) {
        if ( ( sz - used ) < MAXCOL_LEN ) { /* We do need more memory */
            sz += LINE_CHUNK;
            if ( ( cols = realloc ( cols, sz ) ) == (void *)NULL ) {
                fprintf(stderr, "odb [Oload(%d)] - Error re-allocating memory for colnames: [%d] %s\n",
                    __LINE__, errno, strerror(errno));
                goto excol_exit;
            }
        }
        if ( etab[eid].flg & 01406000000 ) {    /* cast or trim */
            if ( j )
                used += strmcat ( cols, ",", sz, 0);
            switch ( etab[eid].td[j].Otype ) {
            case SQL_WVARCHAR:
            case SQL_WLONGVARCHAR:
            case SQL_VARCHAR:
            case SQL_LONGVARCHAR:
            case SQL_CHAR:
            case SQL_WCHAR:
                if ( etab[eid].flg2 & 040000000 ) {
                    used += snprintf(cols + used, sz, "TRIM(%s)", (char *)etab[eid].td[j].Oname);
                } else if ( etab[eid].flg & 04000000 ) {
                    used += snprintf(cols + used, sz, "RTRIM(%s)", (char *)etab[eid].td[j].Oname);
                } else {
                    used += strmcat ( cols, (char *) etab[eid].td[j].Oname, sz, 0);
                }
                break;
            default:
                if ( etab[eid].flg & 01000000000 ) {    /* cast */
                    used += snprintf( ( cols + used ), sz, "CAST(%s AS VARCHAR(%u))",
                        (char *)etab[eid].td[j].Oname, (unsigned int)etab[eid].td[j].Osize);
                } else {
                    used += strmcat ( cols, (char *) etab[eid].td[j].Oname, sz, 0);
                }
                break;
            }
        } else {
            clen = strlen((char *)etab[eid].td[j].Oname) ;
            if ( ( p = strstr ( etab[eid].cols, (char *)etab[eid].td[j].Oname ) ) == ( char * ) NULL ) {
                    /* column included: cat its name */
                if ( used )
                    strmcat(cols, ",", sz, 0 ); 
                used += strmcat(cols, (char *)etab[eid].td[j].Oname, sz, 0 ) + 1; 
            } else {    /* Colname found */
                if ( p == etab[eid].cols && ( *(p + clen) == ',' || !*(p + clen) ) ) {
                    /* column excluded: do nothing */
                } else if ( *( p - 1) == ',' && ( *(p + clen) == ',' || !*(p + clen) ) ) {
                    /* column excluded: do nothing */
                } else {
                    /* column included: cat its name */
                    if ( used )
                        strmcat(cols, ",", sz, 0 ); 
                    used += strmcat(cols, (char *)etab[eid].td[j].Oname, sz, 0 ) + 1; 
                }
            }
        }
        if ( flg )
            free ( etab[eid].td[j].Oname );
    }
    excol_exit:
    if ( flg )
        free(etab[eid].td);
    etab[eid].cols = cols ;
}

/* strmtime:
 *      format time in HH:MM:SS.000 starting from SSS.000
 *
 *      input: secs seconds (double)
 *      input: str string to write to (14 bytes)
 *      return: formatted string
 */
static char *strmtime ( double secs, char *str )
{
    unsigned long ts;
    unsigned int ms;        
    unsigned char flg = 0;

    if ( secs < 0) {
        flg = 1;
        secs *= -1;
    }

    ts = (unsigned long)((secs+0.0005) * 1000) ;
    ms = ts % 1000 ;
    ts /= 1000 ;
    sprintf(str, "%s%02lu:%02lu:%02lu.%03u",
        flg ? "-" : "", ts / 3600, ts % 3600 / 60, ts % 60, ms);

  return(str);
}

/* dumpbuff:
 *      dump ODBC (or any other) buffer in ASCII/HEX format
 *
 *      input: tid thread id
 *      input: buff pointer to buffer 
 *      input: buffl buffer length
 *      input: v verbosity ( 0 / 1 )
 *
 *      return: void
 */
static void dumpbuff ( FILE * fd, int eid, unsigned char *buff, size_t buffl, unsigned int v )
{
    size_t i = 0;               /* loop variable */
    unsigned int j = 0;         /* loop variable */

    if ( v )
        fprintf(fd, "[%d] Dumping %zu bytes\n", etab[eid].id, buffl ) ;
    for ( i = 0 ; i < buffl ; i += 16 ) {
        fprintf ( fd,  "%06zu: " , i );
        for ( j = 0 ; j < 16 ; j++ ) { 
            if ( ( i + j ) < buffl )
                fprintf ( fd, "%02x ", buff[i+j]);
            else
                fputs ( "   " , fd ) ;
        }
        fputc ( ' ' , fd );
        for ( j = 0 ; j < 16 ; j++ ) 
        if ( ( i + j ) < buffl )
            fputc ( isprint ( buff[i+j] ) ? buff[i+j] : '.', fd );
        fputc ('\n' , fd );
    }
}

#ifdef ODB_PROFILE
/* tspdiff:
 *      returns nanoseconds between two struct timespec(s). Used to profile odb execution
 *
 *      input: start struct timespac pointer
 *      input: end struct timespac pointer
 *
 *      return: unsigned long nanoseconds
 */
unsigned long tspdiff ( struct timespec *start, struct timespec *end ) 
{
    return ((((unsigned long)end->tv_sec * 1000000000) + end->tv_nsec) -
            (((unsigned long)start->tv_sec * 1000000000) + start->tv_nsec));

}
#endif

/* ucs2toutf8:
 *      Convert UCS-2 to UTF-8 encoding: both UCS-2 string and its length will be overwritten. Buffers should be big enough
 *
 *      input: 'str': pointer to UCS-2 encoded string
 *      input: 'len': pointer to UCS-2 encoded string length
 *      input: 'bsz': buffer size
 *      input: 'bu2': bad ucs2 conversion management
 *
 *      return: 0=conversion OK, 1=conversion with errors
 */
static int ucs2toutf8 ( SQLCHAR *str, SQLLEN *len, SQLULEN bs, int bu2 ) 
{
    uint16_t u2 = 0 ;       /* UCS-2 character container */
    SQLLEN l2 = *len ;      /* UCS-2 string length */
    SQLLEN l8 = 0 ;         /* UTF-8 string length */
    SQLCHAR *u8 = 0 ;       /* UTF-8 string buffer */
    SQLCHAR buff[256];      /* avoid using malloc/free for short fields */
    SQLLEN i = 0;           /* loop variable */
    unsigned int fb = 0 ;   /* free buffer flag */
    int ret = 0 ;           /* return value */

    
    if ( l2 == 0 || l2 ==SQL_NULL_DATA ) 
        return (ret);                               /* nothing to convert for empty or NULL strings */

    if ( bs > 256 ) {
        if ( ( u8 = malloc ( (size_t)bs ) ) == (void *) NULL ) {
            fprintf(stderr, "odb [ucs2toutf8(%d)] - Error Allocating %zu bytes of memory\n", __LINE__, (size_t)bs);
            return(1);
        }
        fb = 1 ;
    } else {
        u8 = &buff[0] ;
    }

    for ( i = 0 ; i <= ( l2 - 2 ) ; i += 2 ) {
        u2 = str[i+1] << 8 | str[i] ;               /* get and swap next two bytes from UCS-2 string */
        if ( u2 < 0x80 ) {                          /* single byte UTF-8 (ASCII 7 bits) */
            u8[l8++] = (SQLCHAR)u2 ;
        } else if ( u2 >= 0x80 && u2 < 0x800 ) {    /* double byte UTF-8 */
            u8[l8++] = ( u2 >> 6 ) | 0xC0 ;
            u8[l8++] = ( u2 & 0x3F ) | 0x80 ;
        } else if (u2 >= 0x800 && u2 < 0xFFFF) {    /* three bytes UTF-8 */
            if (u2 >= 0xD800 && u2 <= 0xDFFF) {     /* Bad UCS-2 encoding: surrogate pair */
                ret = 1;                            /* set return value */
                fprintf(stderr, "odb [ucs2toutf8(%d)] - Bad UCS-2 character 0x%x found in position %ld\n", __LINE__, u2, (long)i);
                switch ( bu2 ) {
                case 0:     /* skip */
                    continue ;
                case 1:     /* force */
                    break;
                case 2:     /* cpucs2 */
                    if ( fb )
                        free ( u8 ) ;
                    return(ret);
                case 3:     /* qmark */
                    u8[l8++] = '?' ;
                    continue ;
                }
            }
            u8[l8++] = ( u2 >> 12) | 0xE0;
            u8[l8++] = ( ( u2 >> 6 ) & 0x3F) | 0x80;
            u8[l8++] = ( u2 & 0x3F) | 0x80;
        /*} else if (u2 >= 0x10000 && u2 < 0x10FFFF) { four bytes UTF-8 
            u8[l8++] = 0xF0 | ( u2 >> 18 );
            u8[l8++] = 0x80 | ( ( u2 >> 12 ) & 0x3F ) ;
            u8[l8++] = 0x80 | ( ( u2 >> 6) & 0x3F ) ;
            u8[l8++] = 0x80 | ( u2 & 0x3F ) ;*/
        }
    }
    MEMCPY ( str , u8 , (size_t)l8 ) ;              /* overwrite UCS-2 buffer with UTF-8 encoded string */
    *len  = l8 ;                                    /* update string length */
    if ( fb )
        free ( u8 ) ;                               /* free memory if allocated */
    return(ret);
}

/* addGlobalPointer:
*      add ptr globalPointers buffer
*
*      return: no return, exit on error
*/
static void addGlobalPointer(void *ptr)
{
    if (nGlobalPointers >= (naGlobalPointers * ETAB_CHUNK - 2)) { /* need new memory */
        if ((globalPointers = realloc(globalPointers, ++naGlobalPointers*ETAB_CHUNK * sizeof(void *))) == (void *)NULL) {
            fprintf(stderr, "%s [etabnew(%d)] - Error allocating %dth block of etab[] memory: [%d] %s\n",
                __FILE__, __LINE__, naGlobalPointers, errno, strerror(errno));
            exit(EX_OSERR);
        }
    }
    globalPointers[nGlobalPointers++] = ptr;
}

/* is_valid_numeric:
*      check if the string is valid numeric
*
*      Input: str: string to be validate.
*      Input: n: length of str
*
*      return: if str is valid numeric return 1 else return 0
*/
static int is_valid_numeric(const char* str, size_t n) {
    int s = 1;
    for (size_t i = 0; i < n; ++i) {
        switch (s) {
        case 1: // expect a sign or digit
            if (str[i] == '+' || str[i] == '-') s = 2;
            else if (isdigit(str[i])) s = 3;
            else return 0;
            break;
        case 2: // expect a digit
            if (!isdigit(str[i])) return 0;
            s = 3;
            break;
        case 3: // expect option digit or dot or 'e/E'
            if (str[i] == '.') s = 4;
            else if (str[i] == 'e' || str[i] == 'E') s = 6;
            else if (!isdigit(str[i])) return 0;
            break;
        case 4: // expect digit after a dot
            if (!isdigit(str[i])) return 0;
            s = 5;
            break;
        case 5: // now expect optional digit or 'e/E'
            if (str[i] == 'e' || str[i] == 'E') s = 6;
            else if (!isdigit(str[i])) return 0;
            break;
        case 6: // expect a sign or digit after 'e/E'
            if (str[i] == '+' || str[i] == '-') s = 7;
            else if (isdigit(str[i])) s = 8;
            else return 0;
            break;
        case 7: // expect a digit after a sign after 'e/E'
            if (!isdigit(str[i])) return 0;
            s = 8;
            break;
        case 8: // expect optional digit
            if (!isdigit(str[i])) return 0;
            break;
        default:
            return 0;
        }
    }
    if (s == 3 || s == 4 || s == 5 || s == 8) return 1;
    return 0;
}

/* usagexit:
 *      print usage message and exit
 *
 *      return: void
 */
static void usagexit()
{
      /* Ruler       1         2         3         4         5         6         7         8*/
    fprintf(stderr, "%s\nBuild: %s [%s %s]\n", odbid, ODBBLD, __DATE__, __TIME__);
    fprintf(stderr, "   -h: print this help\n"
        "   -version: print odb version and exit\n"
        "   -lsdrv: list available drivers @ Driver Manager level\n"
        "   -lsdsn: list available Data Sources\n"
        "Connection related options. You can connect using either:\n"
        "   -u User: (default $ODB_USER variable)\n"
        "   -p Password: (default $ODB_PWD variable)\n"
        "   -d Data_Source_Name: (default $ODB_DSN variable)\n"
        "   -ca Connection_Attributes (normally used instead of -d DSN)\n"
        "   -U sets SQL_TXN_READ_UNCOMMITTED isolation level\n"
        "   -ndsn [+]<number>: adds 1 to <number> to DSN\n"
        "   -nps <nbytes>[:<nbytes>]: specify source[:target] network packet size\n"
        "SQL interpreter options:\n"
        "   -I [$ODB_INI SECTION]: interactive mode shell\n"
        "   -noconnect: do not connect on startup\n"
        "General options:\n"
        "   -q [cmd|res|all|off]: do not print commands/results/both\n"
        "   -i [TYPE[MULT,WIDE_MULT]:CATALOG.SCHEMA[.TABLE]]: lists following object types:\n"
        "      (t)ables, (v)iews, s(y)nonyns, (s)chemas, (c)atalogs, syst(e)m tables\n"
        "      (l)ocal temp, (g)lobal temp, (m)at views, (M)mat view groups, (a)lias\n"
        "      (A)ll object types, (T)table desc, (D)table DDL, (U) table DDL with multipliers\n"
        "   -r #rowset: rowset to be used insert/selects (default 100)\n"
        "   -soe: Stop On Error (script execution/loading task)\n"
        "   -N : Null run. Doesn't SQLExecute statements\n"
        "   -v : be verbose\n"
        "   -vv : Print execution table\n"
        "   -noschema : do not use schemas: CAT.OBJ instead of CAT.SCH.OBJ\n"
        "   -nocatalog : do not use catalogs: SCH.OBJ instead of CAT.SCH.OBJ\n"
        "   -nocatnull : like -nocatalog but uses NULL instead of empty CAT strings\n"
        "   -ucs2toutf8 : set UCS-2 to UTF-8 conversion in odb\n"
        "   -var var_name var_value: set user defined variables\n"
        "   -ksep char/code: Thousands Separator Character (default \',\')\n"
        "   -dsep char/code: Decimal Separator Character (default \'.\')\n"
        "SQL execution options [connection required]:\n"
        "   -x [#inst:]'command': runs #inst (default 1) command instances\n"
        "   -f [#inst:]'script': runs #inst (default 1) script instances\n"
        "   -P script_path_regexp: runs in parallel scripts_path_regexp\n"
        "      if script_path_regexp ends with / all files in that dir\n"
        "   -S script_path_regexp: runs serially scripts_path_regexp\n"
        "      if script_path_regexp ends with / all files in that dir\n"
        "   -L #loops: runs everything #loops times\n"
        "   -T max_threads: max number of execution threads\n"
        "   -dlb: use Dynamic Load Balancing\n"
        "   -timeout #seconds: stops everything after #seconds (no Win32)\n"
        "   -delay #ms: delay (ms) before starting next thread\n"
        "   -ldelay #ms: delay (ms) before starting next loop in a thread\n"
        "   -ttime #ms[:ms]: delay (ms) before starting next command in a thread\n"
        "          random delay if a [min:max] range is specified\n"
        "   -F #records: max rows to fetch\n"
        "   -c : output in csv format\n"
        "   -b : print start time in the headers when CSV output\n"
        "   -pcn: Print Column Names\n"
        "   -plm: Print Line Mode\n"
        "   -fs char/code: Field Sep <char> ASCII_dec> 0<ASCII_OCT> X<ASCII_HEX>\n"
        "   -rs char/code: Rec Sep <char> ASCII_dec> 0<ASCII_OCT> X<ASCII_HEX>\n"
        "   -sq char/code: String Qualifier (default none)\n"
        "   -ec char/code: Escape Character (default \'\\\')\n"
        "   -ns nullstring: print nullstring when a field is NULL\n"
        "   -trim: Trim leading/trailing white spaces from txt cols\n"
        "   -drs: describe result set (#cols, data types...) for each Q)\n"
        "   -hint: do not remove C style comments (treat them as hints)\n"
        "   -casesens: set case sensitive DB\n"
        "   -Z : shuffle the execution table randomizing Qs start order\n");
    fprintf(stderr, "Data loading options [connection required]:\n"
        "   -l src=[-]file:tgt=table[:map=mapfile][:fs=fieldsep|:mcfs=fieldsep][:rs=recsep|:mcrs=recsep]\n"
        "      [:soe][:skip=linestoskip][:ns=nullstring][:ec=eschar][:sq=stringqualifier]\n"
        "      [:pc=padchar][:em=embedchar][:errmax=#max_err][:commit=auto|end|#rows|x#rs]\n"
        "      [:rows=#rowset][:norb][:full][:max=#max_rec][:truncate][:show][:bpc=#][:bpwc=#]\n"
        "      [:nomark][:parallel=number][:iobuff=#size][:buffsz=#size]][:fieldtrunc={0-4}]\n"
        "      [:pre={@sqlfile|sqlcmd}][:post={@sqlfile|sqlcmd}][:ifempty]\n"
        "      [:direct][:bad=[+]badfile][:tpar=#tables][:maxlen=#bytes][:time][:loadcmd=IN|UP|UL]\n"
#ifdef XML
        "      [:xmltag=[+]element][:xmlord][:xmldump]\n"
#endif
        "      Defaults/notes:\n"
        "      * src file: local file or {hdfs,mapr}[@host,port[,huser]].<HDFS_PATH>\n"
        "      * fs: default ','. Also <ASCII_dec> 0<ASCII_OCT> X<ASCII_HEX>\n"
        "      * mcfs: default use fs as field separator. If mcfs was set then use mcfs as field\n"
        "              separator. mcfs means multi characters separator and support escape sequence:\n"
        "              \\n: new line\n"
        "              \\r: return\n"
        "              \\t: tab\n"
        "              \\xhh: ascii code in hex, example: \\x41 is 'A'\n"
        "      * rs: default '\\n'. Also <ASCII_dec> 0<ASCII_OCT> X<ASCII_HEX>\n"
        "      * mcrs: default use rs as record separator. If mcrs was set then use mcrs as record\n"
        "              separator. Please refer to mcfs"
        "      * ec: default '\\'. Also <ASCII_dec> 0<ASCII_OCT> X<ASCII_HEX>\n"
        "      * pc: no default. Also <ASCII_dec> 0<ASCII_OCT> X<ASCII_HEX>\n"
        "      * direct: only for Vertica databases\n"
        "      * bpc: default 1,bpwc: default 4 \n"
        "      * loadcmd: default IN. only for Trafodion databases\n"
        "Data extraction options [connection required]:\n"
        "   -e {src={table|-file}|sql=<custom sql>}:tgt=[+]file[:pwhere=where_cond]\n"
        "      [:fs=fieldsep|:mcfs=fieldsep][:rs=recsep|:mcrs=recsep][:sq=stringqualifier]\n"
       "      [:ec=escape_char][:soe][:ns=nullstring][:es=emptystring][:rows=#rowset][:nomark]\n"
       "      [:binary][:bpc=#][:bpwc=#][:max=#max_rec][:[r]trim[+]][:cast][:multi][:parallel=number]\n"
       "      [:gzip[=lev]][:splitby=column][:uncommitted][:iobuff=#size][:hblock=#size][:ucs2toutf8]\n"
       "      [:pre={@sqlfile|sqlcmd}[:mpre={@sqlfile|sqlcmd}[:post={@sqlfile|sqlcmd}]\n"
       "      [:tpar=#tables][:time][:cols=[-]columns]][:maxlen=#bytes][:xml]\n"
        "      Defaults/notes:\n"
        "      * tgt file: local file or {hdfs,mapr}.[@host,port[,huser]].<HDFS_PATH>\n"  
        "      * fs: default ','. Also <ASCII_dec> 0<ASCII_OCT> X<ASCII_HEX>\n"
        "      * mcfs: default use fs as field separator. If mcfs was set then use mcfs as field\n"
        "              separator. mcfs means multi characters separator and support escape sequence:\n"
        "              \\n: new line\n"
        "              \\r: return\n"
        "              \\t: tab\n"
        "              \\xhh: ascii code in hex, example: \\x41 is 'A'\n"
        "      * rs: default '\\n'. Also <ASCII_dec> 0<ASCII_OCT> X<ASCII_HEX>\n"
        "      * mcrs: default use rs as record separator. If mcrs was set then use mcrs as record\n"
        "              separator. Please refer to mcfs"
        "      * ec: default '\\'. Also <ASCII_dec> 0<ASCII_OCT> X<ASCII_HEX>\n" 
        "      * sq: no default. Also <ASCII_dec> 0<ASCII_OCT> X<ASCII_HEX>\n" 
        "      * gzip compression level between 0 and 9\n" 
        "      * bpc: default 1,bpwc: default 4 \n");
    fprintf(stderr, "Data copy options [connection required]:\n"
        "   -cp src={table|-file:tgt=schema[.table][pwhere=where_cond][:soe][:roe=#][:roedel=#ms]\n"
        "      [:truncate][:rows=#rowset][:nomark][:max=#max_rec][:bpc=#][:bpwc=#][:[r]trim[+]]\n"
        "      [:parallel=number][:errmax=#max_err][:commit=auto|end|#rows|x#rs][:time][:cast]\n"
        "      [:direct][:uncommitted][:norb][:splitby=column][:pre={@sqlfile|sqlcmd}]\n"
	"      [:post={@sqlfile|sqlcmd}][:mpre={@sqlfile|sqlcmd}][:ifempty]\n"
        "      [:loaders=#loaders][:tpar=#tables][:cols=[-]columns][:errdmp=file]\n"
        "      [:sql={sqlcmd|@sqlfile|-file}[:bind=auto|char|cdef][:seq=field#[,start]]\n"
        "      [:tmpre={@sqlfile|sqlcmd}][:ucs2toutf8=[skip,force,cpucs2,qmark]][:loadcmd=IN|UP|UL]\n"
        "      Defaults/notes:\n"
        "      * loaders: default 2 load threads for each 'extractor'\n"
        "      * direct: only work if target database is Vertica\n"
        "      * ucs2toutf8: default is 'skip'\n"
        "      * roe: default 3 if no arguments\n"
        "      * bpc: default 1,bpwc: default 4 \n"
        "Data pipe options [connection required]:\n"
        "   -pipe sql={sqlcmd|@sqlscript|-file}:tgtsql={@sqlfile|sqlcmd}[:soe]\n"
        "      [:rows=#rowset][:nomark][:max=#max_rec][:bpc=#][:bpwc=#][:errdmp=file]\n"
        "      [:parallel=number][:errmax=#max_err][:commit=auto|end|#rows|x#rs][:time]\n"
        "      [:pre={@sqlfile|sqlcmd}][:post={@sqlfile|sqlcmd}]\n"
        "      [:mpre={@sqlfile|sqlcmd}][:tmpre={@sqlfile|sqlcmd}]\n"
        "      [:loaders=#loaders][:tpar=#tables][:bind=auto|char|cdef]\n"
        "      Defaults/notes:\n"
        "      * loaders: default 1 load threads for each extraction thread\n"
        "      * bpc: default 1,bpwc: default 4 \n"
        "Table diff options [connection required]:\n"
        "   -diff src={table|-file}:tgt=table[:key=columns][:output=[+]file][:pwhere=where_cond]\n"
        "      [:pwhere=where_cond][:nomark][:rows=#rowset][:odad][:fs=fieldsep][:time][:trim[+]]\n"
        "      [:rs=recsep][:quick][:splitby=column][:parallel=number][:max=#max_rec]\n"
        "      [:print=[I][D][C]][:ns=nullstring][:es=emptystring][:bpc=#][:bpwc=#][:uncommitted]\n"
        "      [:pre={@sqlfile|sqlcmd}][:post={@sqlfile|sqlcmd}][:tpar=#tables]\n"
        "      Defaults/notes:\n"
        "      * bpc: default 1,bpwc: default 4 \n"
        "      * print: default is Inserted Deleted Changed\n");
      /* Ruler       1         2         3         4         5         6         7         8*/
    exit( EX_OK );
}
