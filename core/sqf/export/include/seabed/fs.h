//------------------------------------------------------------------
//
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

//
// File-system module
//
#ifndef __SB_FS_H_
#define __SB_FS_H_

#include "int/conv.h"
#include "int/exp.h"
#include "int/diag.h"
#include "int/opts.h"
#include "int/types.h"

#include "cc.h"
#include "excep.h"
#include "fserr.h"
#include "mslimits.h"
#include "pctlcom.h"

#ifndef USE_EVENT_REG
//#define USE_EVENT_REG*/
#endif


enum {
    XZSYS_VAL_RCV_IOTYPE_WRITE     = 1,
    XZSYS_VAL_RCV_IOTYPE_READ      = 2,
    XZSYS_VAL_RCV_IOTYPE_WRITEREAD = 3
};

enum {
    XZSYS_VAL_SMSG_CPUDOWN         = -2,
    XZSYS_VAL_SMSG_CPUUP           = -3,
    XZSYS_VAL_SMSG_TIMESIGNAL      = -22,
    XZSYS_VAL_SMSG_PROCDEATH       = -101,
    XZSYS_VAL_SMSG_OPEN            = -103,
    XZSYS_VAL_SMSG_CLOSE           = -104,
    XZSYS_VAL_SMSG_UNKNOWN         = -1000,  // made up for seaquest
    XZSYS_VAL_SMSG_CHANGE          = -1001,  // made up for seaquest
    XZSYS_VAL_SMSG_SHUTDOWN        = -1002   // made up for seaquest
};

enum {
    XZSYS_VAL_LEN_FILENAME         = 32      // max file-name length
};

enum {
    FS_MAX_CONCUR_NOWAIT_OPENS     = 10      // max concurrent nowait-opens
};


typedef struct xzsys_ddl_ssid_def {
   char                            z_owner[8];
   short                           z_number;
   unsigned short                  z_version;
} xzsys_ddl_ssid_def;

typedef struct xzsys_ddl_phandle_def {
   union {
      struct {
         signed char                     ztype;
         char                            filler_0[63];
      } z_data;
      short                           z_word[32];
      char                            z_byte[64];
#ifdef USE_SB_DDL_PHDL_ALIGN
  #if __WORDSIZE == 64
      long                            z_64[8];
  #else
      long long                       z_64[8];
  #endif
#endif
   } u_z_data;
} xzsys_ddl_phandle_def;

typedef struct xzsys_ddl_receiveinformation {
   short                           z_iotype;
#ifdef USE_SB_NEW_RI
   int                             z_maxreplycount;
#else
   short                           z_maxreplycount;
#endif
   short                           z_messagetag;
   short                           z_filenum;
   long                            z_syncid;
   xzsys_ddl_phandle_def           z_sender;
   short                           z_openlabel;
   int                             z_userid;
} xzsys_ddl_receiveinformation_def;

typedef struct xzsys_ddl_smsg_change_def {
   short                           z_msgnumber;
   short                           z_grouptype;
   char                            z_groupname[MS_MON_MAX_KEY_NAME];
   char                            z_keyname[MS_MON_MAX_KEY_NAME];
   char                            z_value[MS_MON_MAX_VALUE_SIZE];
} xzsys_ddl_smsg_change_def;

typedef struct xzsys_ddl_smsg_close_def {
   short                           z_msgnumber;
   short                           z_tapedisposition;
} xzsys_ddl_smsg_close_def;

typedef struct xzsys_ddl_smsg_cpudown_def {
   short                           z_msgnumber;
   short                           z_cpunumber;
   int                             z_takeover; // SQ (NOT NSK)
} xzsys_ddl_smsg_cpudown_def;

typedef struct xzsys_ddl_smsg_cpuup_def {
   short                           z_msgnumber;
   short                           z_cpunumber;
   int                             z_takeover; // SQ (NOT NSK)
} xzsys_ddl_smsg_cpuup_def;

typedef struct xzsys_ddl_smsg_open_def {
   union {
      short                           z_msgnumber;
      char                            z_base[2];
   } u_z_msgnumber;
   short                           z_accessmode;
   short                           z_exclusionmode;
   short                           z_nowait;
   short                           z_syncdepth;
   short                           z_options;
   short                           z_paid;
   short                           z_flags;
   xzsys_ddl_phandle_def           z_primary;
   short                           z_qualifier_len;
   struct {
      short                           zoffset;
      short                           zlen;
   } z_opener_name;
   short                           z_primary_fnum;
   short                           z_craid;
   struct {
      short                           zoffset;
      short                           zlen;
   } z_hometerm_name;
   short                           z_reserved[5];
   union {
      struct {
         signed char                     filler_0[102];
      } z_data;
      char                            z_qualifier[102];
   } u_z_data;
} xzsys_ddl_smsg_open_def;

typedef struct xzsys_ddl_smsg_procdeath_def {
   union {
      short                           z_msgnumber;
      char                            z_base[2];
   } u_z_msgnumber;
   xzsys_ddl_phandle_def           z_phandle;
   long long                       z_cputime;
   short                           z_jobid;
   short                           z_completion_code;
   union {
      short                           z_termination_code;
      short                           z_killer_craid;
   } u_z_termination_code;
   xzsys_ddl_ssid_def              z_subsystem;
   xzsys_ddl_phandle_def           z_killer;
   short                           z_termtext_len;
   struct {
      short                           zoffset;
      short                           zlen;
   } z_procname;
   short                           z_flags;
   int                             z_osspid;
   short                           z_reserved;
   union {
      struct {
         signed char                     filler_0[112];
      } z_data;
      char                            z_termtext[112];
   } u_z_data;
} xzsys_ddl_smsg_procdeath_def;
typedef struct xzsys_ddl_smsg_shutdown_def {
   short                           z_msgnumber;
   short                           z_shutdownlevel;
} xzsys_ddl_smsg_shutdown_def;

typedef struct xzsys_ddl_smsg_timesignal_def {
   short                           z_msgnumber;
   short                           z_parm1;
   long                            z_parm2;
} xzsys_ddl_smsg_timesignal_def;

typedef struct xzsys_ddl_smsg_def {
   union {
      struct {
         char                            zbase[250];
      } z_msg;
      short                           z_msgnumber[125];
      xzsys_ddl_smsg_cpudown_def      z_cpudown;
      xzsys_ddl_smsg_cpuup_def        z_cpuup;
      xzsys_ddl_smsg_timesignal_def   z_timesignal;
      xzsys_ddl_smsg_procdeath_def    z_procdeath;
      xzsys_ddl_smsg_open_def         z_open;
      xzsys_ddl_smsg_close_def        z_close;
      xzsys_ddl_smsg_change_def       z_change;
      xzsys_ddl_smsg_shutdown_def     z_shutdown;
   } u_z_msg;
} xzsys_ddl_smsg_def;


#include "int/da.h"

typedef struct FS_Receiveinfo_Type {
    short           io_type;
#ifdef USE_SB_NEW_RI
    int             max_reply_count;
#else
    short           max_reply_count;
#endif
    short           message_tag;
    short           file_number;
    int             sync_id;
    SB_Phandle_Type sender;
    short           open_label;
    SB_Uid_Type     user_id;
} FS_Receiveinfo_Type;

// Assert State
typedef struct File_AS_Type {
    int assert1;
    int assert2;
} File_AS_Type;

enum {
    FS_BUF_OPTION_COPY = 0x00000001,
    FS_BUF_OPTION_ALL  = 0x00000001
};
typedef void       *(*FS_Buf_Alloc_Cb_Type)(size_t len);
typedef void        (*FS_Buf_Free_Cb_Type)(void *buf);
SB_Export short       file_buf_options(int options)
SB_DIAG_UNUSED;
SB_Export _bcc_status file_buf_readupdatex(short         filenum,
                                           char        **buffer,
                                           int          *SB_DA(count_read,NULL),
                                           SB_Tag_Type   SB_DA(tag,BOMITTAG))
SB_DIAG_UNUSED;
SB_Export short       file_buf_register(FS_Buf_Alloc_Cb_Type callback_alloc,
                                        FS_Buf_Free_Cb_Type  callback_free)
SB_DIAG_UNUSED;

SB_Export void  file_debug_hook(const char *who, const char *fname);

SB_Export int   file_enable_open_cleanup();
#ifdef USE_EVENT_REG
SB_Export int   file_event_deregister(short event)
SB_DIAG_UNUSED;
SB_Export void  file_event_disable_abort();
SB_Export int   file_event_register(short event)
SB_DIAG_UNUSED;
#endif

SB_Export int   file_init(int *argc, char ***argv)
SB_THROWS_FATAL SB_DIAG_UNUSED;
SB_Export int   file_init_attach(int    *argc,
                                 char ***argv,
                                 int     forkexec,
                                 char   *name)
SB_THROWS_FATAL SB_DIAG_UNUSED;

SB_Export int   file_mon_process_close()
SB_DIAG_UNUSED;
SB_Export int   file_mon_process_shutdown()
SB_DIAG_UNUSED;
SB_Export void  file_mon_process_shutdown_now();
SB_Export int   file_mon_process_startup(int sysmsgs)
SB_THROWS_FATAL SB_DIAG_UNUSED;
SB_Export int   file_mon_process_startup2(int sysmsgs, int pipeio, bool stderr_remap=true) // remap std_err to monitor
SB_THROWS_FATAL SB_DIAG_UNUSED;

SB_Export void  file_test_assert_disable(File_AS_Type *state);
SB_Export void  file_test_assert_enable(File_AS_Type *state);
SB_Export int   file_test_init(int *argc, char ***argv, int mpi_init)
SB_DIAG_UNUSED;


SB_Export _bcc_status BACTIVATERECEIVETRANSID(short msg_num)
SB_DIAG_UNUSED;
SB_Export _bcc_status BAWAITIOX(short             *filenum,
                                void             **SB_DA(bufptr,NULL),
                                int               *SB_DA(xfercount,NULL),
                                SB_Tag_Type       *SB_DA(tag,0),
                                int                SB_DA(timeout,BOMITINT),
                                short             *SB_DA(segid,NULL))
SB_DIAG_UNUSED;
SB_Export _bcc_status BAWAITIOXTS(short             *filenum,
                                  void             **SB_DA(bufptr,NULL),
                                  int               *SB_DA(xfercount,NULL),
                                  SB_Tag_Type       *SB_DA(tag,0),
                                  int                SB_DA(timeout,BOMITINT),
                                  short             *SB_DA(segid,NULL))
SB_DIAG_UNUSED;
SB_Export _bcc_status BCANCEL(short                filenum)
SB_DIAG_UNUSED;
SB_Export _bcc_status BCANCELREQ(short             filenum,
                                 SB_Tag_Type       SB_DA(tag,BOMITTAG))
SB_DIAG_UNUSED;
SB_Export short BFILE_CLOSE_(short                 filenum,
                             short                 SB_DA(tape_disposition,
                                                         BOMITSHORT))
SB_DIAG_UNUSED;
SB_Export short BFILE_GETINFO_(short               filenum,
                               short              *SB_DA(lasterror,NULL),
                               char               *SB_DA(filename,NULL),
                               short               SB_DA(length_a,BOMITSHORT),
                               short              *SB_DA(filename_len,NULL),
                               short              *SB_DA(typeinfo,NULL),
                               short              *SB_DA(flags,NULL))
SB_DIAG_UNUSED;
SB_Export short BFILE_GETRECEIVEINFO_(FS_Receiveinfo_Type *receiveinfo,
                                      short               *SB_DA(reserved,
                                                                 NULL))
SB_DIAG_UNUSED;
SB_Export short BFILE_OPEN_(char                  *filename,
                            short                  length,
                            short                 *filenum,
                            short                  SB_DA(access,BOMITSHORT),
                            short                  SB_DA(exclusion,BOMITSHORT),
                            short                  SB_DA(nowait_depth,
                                                         BOMITSHORT),
                            short                  SB_DA(sync_or_receive_depth,
                                                         BOMITSHORT),
                            bfat_16                SB_DA(options,BOMITFAT_16),
                            short                  SB_DA(seq_block_buffer_id,
                                                         BOMITSHORT),
                            short                  SB_DA(seq_block_buffer_length,
                                                         BOMITSHORT),
                            SB_Phandle_Type       *SB_DA(primary_processhandle,
                                                         NULL))
SB_DIAG_UNUSED;
SB_Export short BFILE_OPEN_SELF_(short                 *filenum,
                                 short                  SB_DA(access,BOMITSHORT),
                                 short                  SB_DA(exclusion,BOMITSHORT),
                                 short                  SB_DA(nowait_depth,
                                                              BOMITSHORT),
                                 short                  SB_DA(sync_or_receive_depth,
                                                              BOMITSHORT),
                                 bfat_16                SB_DA(options,BOMITFAT_16),
                                 short                  SB_DA(seq_block_buffer_id,
                                                              BOMITSHORT),
                                 short                  SB_DA(seq_block_buffer_length,
                                                              BOMITSHORT),
                                 SB_Phandle_Type       *SB_DA(primary_processhandle,
                                                              NULL))
SB_DIAG_UNUSED;
SB_Export _bcc_status BREADX(short                 filenum,
                             char                 *buffer,
                             int                   read_count,
                             int                  *SB_DA(count_read,NULL),
                             SB_Tag_Type           SB_DA(tag,BOMITTAG))
SB_DIAG_UNUSED;
SB_Export _bcc_status BREADUPDATEX(short           filenum,
                                   char           *buffer,
                                   int             read_count,
                                   int            *SB_DA(count_read,NULL),
                                   SB_Tag_Type     SB_DA(tag,BOMITTAG))
SB_DIAG_UNUSED;
SB_Export _bcc_status BREPLYX(char                *SB_DA(buffer,NULL),
                              int                  SB_DA(write_count, BOMITINT),
                              int                 *SB_DA(count_written,NULL),
                              short                SB_DA(replynum,BOMITSHORT),
                              short                SB_DA(errret,BOMITSHORT))
SB_DIAG_UNUSED;
SB_Export _bcc_status BSETMODE(short               filenum,
                               short               modenum,
                               bfat_16             SB_DA(parm1,BOMITFAT_16),
                               bfat_16             SB_DA(parm2,BOMITFAT_16),
                               short              *SB_DA(oldval,NULL))
SB_DIAG_UNUSED;
SB_Export _bcc_status BWRITEX(short                filenum,
                              char                *buffer,
                              int                  write_count,
                              int                 *SB_DA(count_written,NULL),
                              SB_Tag_Type          SB_DA(tag,BOMITTAG),
                              SB_Uid_Type          SB_DA(userid,BOMITUID))
SB_DIAG_UNUSED;
SB_Export _bcc_status BWRITEREADX(short            filenum,
                                  char            *buffer,
                                  int              write_count,
                                  int              read_count,
                                  int             *SB_DA(count_read,NULL),
                                  SB_Tag_Type      SB_DA(tag,BOMITTAG),
                                  SB_Uid_Type      SB_DA(userid,BOMITUID))
SB_DIAG_UNUSED;
SB_Export _bcc_status BWRITEREADX2(short           filenum,
                                   char           *wbuffer,
                                   int             write_count,
                                   char           *rbuffer,
                                   int             read_count,
                                   int            *SB_DA(count_read,NULL),
                                   SB_Tag_Type     SB_DA(tag,BOMITTAG),
                                   SB_Uid_Type     SB_DA(userid,BOMITUID))
SB_DIAG_UNUSED;

SB_Export _xcc_status XACTIVATERECEIVETRANSID(short msg_num)
SB_DIAG_UNUSED;
SB_Export _xcc_status XAWAITIOX(short             *filenum,
                                void             **SB_DA(bufptr,NULL),
                                unsigned short    *SB_DA(xfercount,NULL),
                                SB_Tag_Type       *SB_DA(tag,0),
                                int                SB_DA(timeout,XOMITINT),
                                short             *SB_DA(segid,NULL))
SB_DIAG_UNUSED;
SB_Export _xcc_status XAWAITIOXTS(short             *filenum,
                                  void             **SB_DA(bufptr,NULL),
                                  unsigned short    *SB_DA(xfercount,NULL),
                                  SB_Tag_Type       *SB_DA(tag,0),
                                  int                SB_DA(timeout,XOMITINT),
                                  short             *SB_DA(segid,NULL))
SB_DIAG_UNUSED;
SB_Export _xcc_status XCANCEL(short                filenum)
SB_DIAG_UNUSED;
SB_Export _xcc_status XCANCELREQ(short             filenum,
                                 SB_Tag_Type       SB_DA(tag,XOMITTAG))
SB_DIAG_UNUSED;
SB_Export short XFILE_CLOSE_(short                 filenum,
                             short                 SB_DA(tape_disposition,
                                                         XOMITSHORT))
SB_DIAG_UNUSED;
SB_Export short XFILE_GETINFO_(short               filenum,
                               short              *SB_DA(lasterror,NULL),
                               char               *SB_DA(filename,NULL),
                               short               SB_DA(length_a,XOMITSHORT),
                               short              *SB_DA(filename_len,NULL),
                               short              *SB_DA(typeinfo,NULL),
                               short              *SB_DA(flags,NULL))
SB_DIAG_UNUSED;
SB_Export short XFILE_GETRECEIVEINFO_(FS_Receiveinfo_Type *receiveinfo,
                                      short               *SB_DA(reserved,
                                                                 NULL))
SB_DIAG_UNUSED;
SB_Export short XFILE_OPEN_(char                  *filename,
                            short                  length,
                            short                 *filenum,
                            short                  SB_DA(access,XOMITSHORT),
                            short                  SB_DA(exclusion,XOMITSHORT),
                            short                  SB_DA(nowait_depth,
                                                         XOMITSHORT),
                            short                  SB_DA(sync_or_receive_depth,
                                                         XOMITSHORT),
                            xfat_16                SB_DA(options,XOMITFAT_16),
                            short                  SB_DA(seq_block_buffer_id,
                                                         XOMITSHORT),
                            short                  SB_DA(seq_block_buffer_length,
                                                         XOMITSHORT),
                            SB_Phandle_Type       *SB_DA(primary_processhandle,
                                                         NULL))
SB_DIAG_UNUSED;
SB_Export short XFILE_OPEN_SELF_(short                 *filenum,
                                 short                  SB_DA(access,XOMITSHORT),
                                 short                  SB_DA(exclusion,XOMITSHORT),
                                 short                  SB_DA(nowait_depth,
                                                              XOMITSHORT),
                                 short                  SB_DA(sync_or_receive_depth,
                                                              XOMITSHORT),
                                 xfat_16                SB_DA(options,XOMITFAT_16),
                                 short                  SB_DA(seq_block_buffer_id,
                                                              XOMITSHORT),
                                 short                  SB_DA(seq_block_buffer_length,
                                                              XOMITSHORT),
                                 SB_Phandle_Type       *SB_DA(primary_processhandle,
                                                              NULL))
SB_DIAG_UNUSED;
SB_Export _xcc_status XREADX(short                 filenum,
                             char                 *buffer,
                             unsigned short        read_count,
                             unsigned short       *SB_DA(count_read,NULL),
                             SB_Tag_Type           SB_DA(tag,XOMITTAG))
SB_DIAG_UNUSED;
SB_Export _xcc_status XREADUPDATEX(short           filenum,
                                   char           *buffer,
                                   unsigned short  read_count,
                                   unsigned short *SB_DA(count_read,NULL),
                                   SB_Tag_Type     SB_DA(tag,XOMITTAG))
SB_DIAG_UNUSED;
SB_Export _xcc_status XREPLYX(char                *SB_DA(buffer,NULL),
                              unsigned short       SB_DA(write_count,
                                                         XOMITUSHORT),
                              unsigned short      *SB_DA(count_written,NULL),
                              short                SB_DA(replynum,XOMITSHORT),
                              short                SB_DA(errret,XOMITSHORT))
SB_DIAG_UNUSED;
SB_Export _xcc_status XSETMODE(short               filenum,
                               short               modenum,
                               xfat_16             SB_DA(parm1,XOMITFAT_16),
                               xfat_16             SB_DA(parm2,XOMITFAT_16),
                               short              *SB_DA(oldval,NULL))
SB_DIAG_UNUSED;
SB_Export _xcc_status XWRITEX(short                filenum,
                              char                *buffer,
                              unsigned short       write_count,
                              unsigned short      *SB_DA(count_written,NULL),
                              SB_Tag_Type          SB_DA(tag,XOMITTAG),
                              SB_Uid_Type          SB_DA(userid,XOMITUID))
SB_DIAG_UNUSED;
SB_Export _xcc_status XWRITEREADX(short            filenum,
                                  char            *buffer,
                                  unsigned short   write_count,
                                  unsigned short   read_count,
                                  unsigned short  *SB_DA(count_read,NULL),
                                  SB_Tag_Type      SB_DA(tag,XOMITTAG),
                                  SB_Uid_Type      SB_DA(userid,XOMITUID))
SB_DIAG_UNUSED;
SB_Export _xcc_status XWRITEREADX2(short            filenum,
                                   char            *wbuffer,
                                   unsigned short   write_count,
                                   char            *rbuffer,
                                   unsigned short   read_count,
                                   unsigned short  *SB_DA(count_read,NULL),
                                   SB_Tag_Type      SB_DA(tag,XOMITTAG),
                                   SB_Uid_Type      SB_DA(userid,XOMITUID))
SB_DIAG_UNUSED;

//
// filename functions
//
SB_Export short XFILENAME_TO_PROCESSHANDLE_(const char      *filename,
                                            short            length,
                                            SB_Phandle_Type *processhandle)
SB_DIAG_UNUSED;

//
// process functions
//
// XPROCESS_GETPAIRINFO_() - see pctlcom.h

//
// process handle functions
//
SB_Export short XPROCESSHANDLE_COMPARE_(SB_Phandle_Type *processhandle1,
                                        SB_Phandle_Type *processhandle2)
SB_DIAG_UNUSED;
// XPROCESSHANDLE_DECOMPSE_() - see pctlcom.h
SB_Export short XPROCESSHANDLE_GETMINE_(SB_Phandle_Type *processhandle)
SB_DIAG_UNUSED;
SB_Export short XPROCESSHANDLE_NULLIT_(SB_Phandle_Type *processhandle)
SB_DIAG_UNUSED;

#endif // !__SB_FS_H_
