//------------------------------------------------------------------
//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2006-2015 Hewlett-Packard Development Company, L.P.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//
// @@@ END COPYRIGHT @@@

#ifndef __SB_MSIX_H_
#define __SB_MSIX_H_

/* -- FROM t9050/ktdmtyph -- */

#ifndef __uchar_DEFINED
  #define __uchar_DEFINED
  typedef unsigned char uchar;
#endif
#ifndef __int16_DEFINED
  #define __int16_DEFINED
  typedef short int16;
#endif
#ifndef __uint16_DEFINED
  #define __uint16_DEFINED
  typedef unsigned short uint16;
#endif
#ifndef __int32_DEFINED
  #define __int32_DEFINED
  typedef int int32;
#endif
#ifndef __uint32_DEFINED
  #define __uint32_DEFINED
  typedef unsigned int uint32;
#endif
#ifndef   __int64_DEFINED
  #define __int64_DEFINED
  #ifdef _WIN32
    typedef __int64 int64;
  #else
    typedef long long int64;
  #endif
#endif

/* -- FROM t9055/drosxlth -- */
#define  FILEID_DEF  FileId_t
#define  OPENID_DEF  OpenId_t

/* -- FROM t9055/ddialech -- */

/*
**  The first fields in a request
*/
#define REQUEST_HEADER_FIELDS                                                 \
   uint16 dialect_type;                    /* a dialect num from above list*/ \
   uint16 request_type;                    /* defined by each dialect */      \
   uint16 request_version;                 /* actual version of request */    \
   uint16 minimum_interpretation_version;  /* server must handle this level*/

/*
**  The first fields in a reply
*/
#define REPLY_HEADER_FIELDS                                                 \
   uint16 dialect_type;                    /* a dialect num from above list*/ \
   uint16 reply_type;                      /* defined by each dialect */      \
   uint16 reply_version;                   /* actual version of reply */      \
   uint16 error;                           /* the FE code giving basic rslt*/

#define DIALECT_FS_FS            5   /* File sys to file sys */

typedef struct
{
    int64 id[4];
} Tcbref_t;
#ifndef TCBREF_DEF        /* For compatibility with original definition */
  #define TCBREF_DEF Tcbref_t
#endif

typedef struct VarString
{
  uint16    offset;   /* Byte offset to data.  It is the offset from the    */
                      /*    outermost structure that the varstring is       */
                      /*    contained in (usually the base of the message). */
  uint16    length;   /* Byte length of data, 0 = not present.              */
} VarString;

typedef struct phandle_template {
  uint16                first_word;
  uchar                 filler[62];
} phandle_template;

/* -- FROM t9055/dstats0h -- */
typedef struct crt_template {
  union {
    int16         first_word; /* Tal word reference */
    int16         name[3];
    uchar         name_s;     /* Byte access */
  } name;
} crt_template; /* struct crt_template */

typedef struct crtpid_template {
  union {
    int16        first_word;            /* Tal word reference */
    crt_template  crt;
  } crt;
  union {
    int16         pid;                   /* Process Id */
    struct {
      unsigned        processor:8;           /* Cpu */
      unsigned        pin:8;                 /* Index (-1) for generic */
    } bits;
  } pid;
                                         /*  in a process pair. */
} crtpid_template; /* struct crtpid_template */

typedef int64 OpenId_t;
typedef int64 FileId_t;

/* -- FROM t9055/dfsfsh -- */
enum {FS_FS_OPEN               = 2};
enum {FS_FS_OPEN_REPLY         = 3};

enum {FS_FS_READ               = 4};
enum {FS_FS_READ_REPLY         = 5};

enum {FS_FS_WRITE              = 6};
enum {FS_FS_WRITE_REPLY        = 7};

enum {FS_FS_WRITEREAD          = 8};
enum {FS_FS_WRITEREAD_REPLY    = 9};

enum {FS_FS_CLOSE              = 10};
enum {FS_FS_SIMPLE_REPLY       = 11};

enum {FS_FS_VERSION_JUN88      = 1}; /* Released in D00 */
enum {FS_FS_VERSION_MAR97      = 5}; /* Released in D45 */

#pragma pack(push,2)
typedef struct fs_fs_standardopeninfo /* Standard Open file information */
{
  union {
    int16          first_word; /* (generic word for Tal substruct access) */
    /*
       The first four items are in order to copy to a ReceiveInformation_
        template in the Receive section in the acb_:
         Acb_Receive.Acb_Receiveinfo.FileNumber ':='
           Msg.Write.Sender for ReceiveCopy_Length Bytes;
    */
    int16          filenum;    /* Sender's filenumber */
  } first_word;
  int32            syncid;     /* Sender's syncid */
  phandle_template  phandle;    /* The sender's procid */
  int16            user_openid;/* User supplied openid. */
  union {
    OPENID_DEF      openid;     /* Openid returned by sender */
                                /* For D00 we put the receiver's crtpid her */
    FILEID_DEF      fileid;     /* For open's we use this as the dup checker*/
  } id;
} fs_fs_standardopeninfo; /* struct fs_fs_standardopeninfo */
#pragma pack(pop)

typedef struct file_open_options_template {
  union {
    int16 initialize0;                     /* For initializing to 0. */
        struct {
      unsigned  unstraccess:1;             /* Unstructured access */
      unsigned  nowaitopen:1;              /* Nowait open request */
      unsigned  keepopentime:1;            /* Do not update last open */
                                           /*  time (disk only) */
      unsigned  any_filenum_backupopen:1;  /* Use next filenum for bu open */
      unsigned  sql_open:1;                /* Primary_Phandle is axadr of */
                                           /*   sql open struct (priv only */
      unsigned  wasnotpriv:1;              /* Caller (to Open) was not pri */
      unsigned  linkmonopen:1;             /* forwarding-mode LinkMon open */
      unsigned  callbyopen:1;              /* File_Open was called by Open */
      unsigned  setcorrupt:1;              /* Set corrupt flag in file */
      unsigned  open_posix:1;              /* Open Posix file by Guardian */
                                           /*  name (must be priv or super) */
      unsigned  posix_pathname:1;          /* Filename is Posix pathname */
      unsigned  reserved:1;                /* Reserved */
      unsigned  notransactions:1;          /* $receive to ignore xactions */
      unsigned  localesupport:1;           /* $receive to handle LIDs */
      /* TreatAsDevType0 = LocalSupport;       (see below) */
      unsigned  oldformatmsgs:1;           /* Use old format messages */
      unsigned  nosystemmsgs:1;            /* On $receive present no */
                                           /*  system messages */
    } bits;
  } flags;
} file_open_options_template; /* struct file_open_options_template */

typedef struct fs_fs_open {
  REQUEST_HEADER_FIELDS
  VarString               security;         /* Security information */
  int16                   access;           /* Access mode */
  int16                   exclusion;        /* Exclusion mode */
  int16                   nowait;           /* Nowait Depth */
  int16                   syncdepth;        /* Opener's syncdepth */
                                            /* File_Open_ options parm */
  file_open_options_template     options;
  /* paid in security block */
  fs_fs_standardopeninfo  sender;           /* Opener's info */
  crtpid_template         sender_crtpid;
                                            /* For input to old LastReceive */

  union {                                   /* added in version SEP98 */
    uint32                flags0d;          /* to init all flags at once */
    struct {
      unsigned            backupopen:1;     /* This is backup open */
      unsigned            gmom_valid:1;     /* Gmom field valid */
                                            /* (define also present in data */
      unsigned            fatnamed:1;       /* Sender is fat named */
      unsigned            open_used_privaccessonly:1; /* Opener used priv acc*/
      unsigned            filler:28;
    } bits;
  } flags;                                  /* end of flags union */
  struct {                                  /* Valid if gmom_valid bit set */
    phandle_template      phandle;          /* Gmom's phandle */
    int16                 jobid;            /* Jobid of Gmom */
                                   /* Define Tsal in data buffer if present */
  } gmom;
  struct {                                  /* Valid if backupopen is set */
                                            /* Information on primary */
    phandle_template      phandle;          /*   primary's identifier */
    int16                 filenum;          /*   primary's filenumber */
    OPENID_DEF            openid;           /*   primary's ocb */
  } primary;
  VarString               qualifier;        /* Filename qualifier to be */
                                            /*  opened in  external format */
  VarString               opener_filename;  /* Opener's filename if */
                                            /*  named process */
  uchar                   data[1];          /* start of variable data */
} fs_fs_open;

typedef struct fs_fs_open_reply {
  REPLY_HEADER_FIELDS
  OPENID_DEF              openid;           /* To be used for following req */
  int16                   user_openid;      /* User supplied open id. */
                                            /*  -1 = not supplied or don't */
                                            /*  care. */
  int16                   server_version;
                                            /* Highest Dialect Version */
                                            /*  supported by server */
                                            /* Added in Version_MAY94: */
  union {                                   /* added in version SEP98 */
    uint32                flags0d;          /* to init all flags at once */
    struct {
      unsigned            notrans:1;
                                            /* receiver does not want trans */
      unsigned            sendlids:1;       /* Locale IDs are wanted */
      unsigned            filler:30;
    } bits;
  } flags;                                  /* end of flags union */
} fs_fs_open_reply;

#pragma pack(push,2)
typedef struct linkmon_sendflags_template {
  union {
    int16 initialize0;               /* For initializing to 0. */
        struct {
      unsigned  filler:9;
      unsigned  dialogabortall:1;    /* abort all dialogs associated  */
                                     /*   with this requester */
      unsigned  broadcast:1;         /* goes to all in server class */
      unsigned  noserver:1;          /* reject send if simple SERVER */
      unsigned  dialogcontrol:2;     /* type of send; see below */
      unsigned  transcanchange:1;    /* trans not constant for dialog */
      unsigned  nowait:1;            /* true if nowait */
    } bits;
  } flags;
} linkmon_sendflags_template;
#pragma pack(pop)

#pragma pack(push,2)
typedef struct fs_fs_write {
  REQUEST_HEADER_FIELDS
  fs_fs_standardopeninfo  sender;           /* Sender's Id */

  union {                                   /* added in version SEP98 */
    uint32                flags0d;          /* to init all flags at once */
    struct {
      unsigned            tcbref_valid:1;   /* Tcbref field valid */
      unsigned            lid_valid:1;
                                            /* Lid present and length nonzero*/
                                            /*   (Added in Version_MAY94) */
      unsigned            filler:30;
    } bits;
  } flags;                                  /* end of flags union */
                                            /* note this covers SendFlags */
  TCBREF_DEF              tcbref;           /* TMF transaction id */
  int64                   startid;          /* start id */
  int32                   userid;           /* userid for SQ */
  VarString               lid;              /* Lid, if not default */
                                            /*   (Added in Version_MAY94) */
                                            /* may be absent unless LID_VALID*/
  uchar                   data[1];          /* start of variable data */
  /* WRITE length is msg request data size */
} fs_fs_write;
#pragma pack(pop)

typedef struct fs_fs_write_reply {
  REPLY_HEADER_FIELDS
#if 0 /* NSK size */
  int16                             countwritten; /* Actual bytes written */
#else /* linux size */
  int32                             countwritten; /* Actual bytes written */
#endif
  } fs_fs_write_reply;

#pragma pack(push,2)
typedef struct fs_fs_writeread {
  REQUEST_HEADER_FIELDS
  fs_fs_standardopeninfo  sender;           /* Sender's Id */

  union {                                   /* added in version SEP98 */
    uint16                flags0d;          /* to init all flags at once */
    struct {
      unsigned            tcbref_valid:1;   /* Tcbref field valid */
      unsigned            lid_valid:1;
                                            /* Lid present and length nonzero*/
                                            /*   (Added in Version_MAY94) */
      unsigned            filler:14;
    } bits;
  } flags;                                  /* end of flags union */
                                            /* note this covers SendFlags */
  linkmon_sendflags_template sendflags;
  TCBREF_DEF              tcbref;           /* TMF transaction id */
  int64                   startid;          /* start id */
  int32                   userid;           /* userid for SQ */
  VarString               lid;              /* Lid, if not default */
                                            /*   (Added in Version_MAY94) */
                                            /* may be absent unless LID_VALID*/
  uchar                   data[1];          /* start of variable data */
  /* read length is msg reply data max size */
  /* WRITE length is msg request data size */
} fs_fs_writeread;
#pragma pack(pop)

typedef struct fs_fs_writeread_reply {
  REPLY_HEADER_FIELDS
#if 0 /* NSK size */
  int16                             countwritten; /* Actual bytes written */
#else /* linux size */
  int32                             countwritten; /* Actual bytes written */
#endif
  /* count read is msg reply data size */
  } fs_fs_writeread_reply;

typedef struct fs_fs_close {
  REQUEST_HEADER_FIELDS
  fs_fs_standardopeninfo  sender;           /* Sender's Id */
  int16                   disposition;      /* Tape disposition */
  } fs_fs_close;

typedef struct fs_fs_simple_reply {
  REPLY_HEADER_FIELDS
  } fs_fs_simple_reply;


/* -- FROM t9055/wdialect -- */
enum {
MINIMUM_VERSION_FS_FS          = FS_FS_VERSION_JUN88,
CURRENT_VERSION_FS_FS          = FS_FS_VERSION_MAR97
};


#endif // !__SB_MSIX_H_
