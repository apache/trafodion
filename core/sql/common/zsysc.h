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
**********************************************************************/
/* GUARDIAN 90  T9050D40 (01NOV95) */
#include "Platform.h"

#include "Int64.h"
// #pragma section zsys_ddl_extaddr
typedef long                            zsys_ddl_extaddr_def;
typedef long                            zsys_ddl_char_extaddr_def;
// #pragma section zsys_ddl_ssid
#pragma fieldalign shared2 __zsys_ddl_ssid
typedef struct __zsys_ddl_ssid
{
   char                            z_owner[8];
   short                           z_number;
   unsigned short                  z_version;
} zsys_ddl_ssid_def;
// #pragma section phandle_constant
#define ZSYS_VAL_PHANDLE_WTYPE_NULL -1
#define ZSYS_VAL_PHANDLE_TYPE_NULL 255
#define ZSYS_VAL_PHANDLE_WLEN 10
// #pragma section zsys_ddl_phandle
#pragma fieldalign shared2 __zsys_ddl_phandle
typedef struct __zsys_ddl_phandle
{
   union
   {
      struct
      {
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
} zsys_ddl_phandle_def;
// #pragma section filename_constant
#define ZSYS_VAL_LEN_FILENAME 47
#define ZSYS_VAL_LEN_FILENAME_D00 47
#define ZSYS_VAL_LEN_FILENAME_C00 34
#define ZSYS_VAL_LEN_DEFINENAME_C00 24
#define ZSYS_VAL_LEN_DEFINENAME 24
#define ZSYS_VAL_LEN_FNAMEPATTERN 67
#define ZSYS_VAL_LEN_FNAMEPATTERN_D00 67
#define ZSYS_VAL_LEN_SYSTEMNAME 8
#define ZSYS_VAL_LEN_DESTINATION 24
#define ZSYS_VAL_LEN_DEVICENAME 8
#define ZSYS_VAL_LEN_VOLUMENAME 8
#define ZSYS_VAL_LEN_PROCESSNAME 6
#define ZSYS_VAL_LEN_UNIQUEPROCESSNAME 24
#define ZSYS_VAL_LEN_QUALIFIER 8
#define ZSYS_VAL_LEN_PROCESSDESCR 33
// #pragma section oss_constant
#define ZSYS_VAL_LEN_PATHNAME 1024
// #pragma section filesystem_constant
#define ZSYS_VAL_OPENACC_READWRITE 0
#define ZSYS_VAL_OPENACC_READONLY 1
#define ZSYS_VAL_OPENACC_WRITEONLY 2
#define ZSYS_VAL_OPENACC_EXTEND 3
#define ZSYS_VAL_OPENEXCL_SHARED 0
#define ZSYS_VAL_OPENEXCL_EXCLUSIVE 1
#define ZSYS_VAL_OPENEXCL_PROTECTED 3
#define ZSYS_VAL_AWAITIOTAG_OPEN -30L
#define ZSYS_VAL_AWAITIOTAG_CHECK -29L
#define ZSYS_VAL_DISKOBJ_SUBVOL -1
#define ZSYS_VAL_DISKOBJ_FILE 0
#define ZSYS_VAL_DISKOBJ_TABLE 2
#define ZSYS_VAL_DISKOBJ_INDEX 4
#define ZSYS_VAL_DISKOBJ_PROTECTVIEW 5
#define ZSYS_VAL_DISKOBJ_SHORTHANDVIEW 7
#define ZSYS_VAL_FILETYPE_UNSTRUCTURED 0
#define ZSYS_VAL_FILETYPE_RELATIVE 1
#define ZSYS_VAL_FILETYPE_ENTRYSEQ 2
#define ZSYS_VAL_FILETYPE_KEYSEQ 3
#define ZSYS_VAL_RCV_IOTYPE_SYSTEMMSG 0
#define ZSYS_VAL_RCV_IOTYPE_WRITE 1
#define ZSYS_VAL_RCV_IOTYPE_READ 2
#define ZSYS_VAL_RCV_IOTYPE_WRITEREAD 3
#define ZSYS_VAL_FNAME_KIND_ENTITY 0
#define ZSYS_VAL_FNAME_KIND_PATTERN 1
#define ZSYS_VAL_FNAME_KIND_DEFINE 2
#define ZSYS_VAL_FNAME_LEVEL_SYSTEM -1
#define ZSYS_VAL_FNAME_LEVEL_DEVICE 0
#define ZSYS_VAL_FNAME_LEVEL_SUBVOL 1
#define ZSYS_VAL_FNAME_LEVEL_FILE 2
#define ZSYS_VAL_FNAME_SUBPART_DEST 0
#define ZSYS_VAL_FNAME_SUBPART_CPU 1
#define ZSYS_VAL_FNAME_SUBPART_PIN 2
#define ZSYS_VAL_FNAME_SUBPART_SEQNO 3
#define ZSYS_VAL_FNAME_SUBPART_NAME 4
#define ZSYS_VAL_PROCESSKIND_ASTERISK 0
#define ZSYS_VAL_PROCESSKIND_CPU 1
#define ZSYS_VAL_PROCESSKIND_CPUPIN 2
#define ZSYS_VAL_PROCESSKIND_NAME 3
// #pragma section filesystem_options
#define ZSYS_VAL_OPENOPT_UNSTRUCTACC -32768
#define ZSYS_VAL_OPENOPT_NOWAITOPEN 16384
#define ZSYS_VAL_OPENOPT_NOOPENTIME 8192
#define ZSYS_VAL_OPENOPT_BACKUPFNUMANY 4096
#define ZSYS_VAL_OPENOPT_OSSPATHNAME 32
#define ZSYS_VAL_OPENOPT_NOTRANS 8
#define ZSYS_VAL_OPENOPT_LOCALESUPPORT 4
#define ZSYS_VAL_OPENOPT_OLDFMTSYSMSG 2
#define ZSYS_VAL_OPENOPT_NOSYSTEMMSGS 1
#define ZSYS_VAL_CREATEOPT_REFRESH 32
#define ZSYS_VAL_CREATEOPT_INDEXCOMPR 16
#define ZSYS_VAL_CREATEOPT_DATACOMPR 8
#define ZSYS_VAL_CREATEOPT_AUDITCOMPR 4
#define ZSYS_VAL_CREATEOPT_AUDIT 2
#define ZSYS_VAL_CREATEOPT_ODDUNSTRUCT 1
#define ZSYS_VAL_FEDITOPT_REPLPREFIX 2
#define ZSYS_VAL_FEDITOPT_REPLSUFFIX 1
#define ZSYS_VAL_OSSOPEN_RDONLY 0L
#define ZSYS_VAL_OSSOPEN_WRONLY 1L
#define ZSYS_VAL_OSSOPEN_RDWR 2L
#define ZSYS_VAL_OSSOPEN_ACCMODE 3L
#define ZSYS_VAL_OSSOPEN_APPEND 4L
#define ZSYS_VAL_OSSOPEN_CREAT 8L
#define ZSYS_VAL_OSSOPEN_TRUNC 16L
#define ZSYS_VAL_OSSOPEN_EXCL 32L
#define ZSYS_VAL_OSSOPEN_NONBLOCK 16384L
#define ZSYS_VAL_OSSOPEN_NOCTTY 32768L
#define ZSYS_VAL_OSSOPEN_SYNC 65536L
// #pragma section filesystem_itemcodes
#define ZSYS_VAL_FCREAT_FILETYPE 41
#define ZSYS_VAL_FCREAT_FILECODE 42
#define ZSYS_VAL_FCREAT_LOGICALRECLEN 43
#define ZSYS_VAL_FCREAT_BLOCKLEN 44
#define ZSYS_VAL_FCREAT_KEYOFFSET 45
#define ZSYS_VAL_FCREAT_KEYLEN 46
#define ZSYS_VAL_FCREAT_PRIMEXTENTSIZE 50
#define ZSYS_VAL_FCREAT_SCNDEXTENTSIZE 51
#define ZSYS_VAL_FCREAT_MAXIMUMEXTENTS 52
#define ZSYS_VAL_FCREAT_EXPIRATIONDATE 57
#define ZSYS_VAL_FCREAT_ODDUNSTRUCT 65
#define ZSYS_VAL_FCREAT_AUDIT 66
#define ZSYS_VAL_FCREAT_AUDITCOMPRESS 67
#define ZSYS_VAL_FCREAT_DATACOMPRESS 68
#define ZSYS_VAL_FCREAT_INDEXCOMPRESS 69
#define ZSYS_VAL_FCREAT_REFRESH 70
#define ZSYS_VAL_FCREAT_CREATEOPTS 71
#define ZSYS_VAL_FCREAT_WRITETHRU 72
#define ZSYS_VAL_FCREAT_VERIFYWRITES 73
#define ZSYS_VAL_FCREAT_SERIALWRITES 74
#define ZSYS_VAL_FCREAT_NUMPRTNS 90
#define ZSYS_VAL_FCREAT_PRTNDESC 91
#define ZSYS_VAL_FCREAT_PRTNVOLLEN 92
#define ZSYS_VAL_FCREAT_PRTNVOLNAMES 93
#define ZSYS_VAL_FCREAT_PRTNPARTKEYLEN 94
#define ZSYS_VAL_FCREAT_PRTNPARTKEYVAL 95
#define ZSYS_VAL_FCREAT_NUMALTKEYS 100
#define ZSYS_VAL_FCREAT_ALTKEYDESC 101
#define ZSYS_VAL_FCREAT_NUMALTKEYFILES 102
#define ZSYS_VAL_FCREAT_ALTFILELEN 103
#define ZSYS_VAL_FCREAT_ALTFILENAMES 104
#define ZSYS_VAL_FALT_FILECODE 42
#define ZSYS_VAL_FALT_EXPIRATIONDATE 57
#define ZSYS_VAL_FALT_ODDUNSTRUCTURED 65
#define ZSYS_VAL_FALT_AUDIT 66
#define ZSYS_VAL_FALT_REFRESH 70
#define ZSYS_VAL_FALT_BROKEN 78
#define ZSYS_VAL_FALT_NUMPRTNS 90
#define ZSYS_VAL_FALT_PRTNDESC 91
#define ZSYS_VAL_FALT_PRTNVOLLEN 92
#define ZSYS_VAL_FALT_PRTNVOLNAMES 93
#define ZSYS_VAL_FALT_NUMALTKEYS 100
#define ZSYS_VAL_FALT_ALTKEYDESC 101
#define ZSYS_VAL_FALT_NUMALTKEYFILES 102
#define ZSYS_VAL_FALT_ALTFILELEN 103
#define ZSYS_VAL_FALT_ALTFILENAMES 104
#define ZSYS_VAL_FINF_FILENAMELEN 1
#define ZSYS_VAL_FINF_FILENAME 2
#define ZSYS_VAL_FINF_CURRFILENAMELEN 3
#define ZSYS_VAL_FINF_CURRFILENAME 4
#define ZSYS_VAL_FINF_DEFINENAMELEN 5
#define ZSYS_VAL_FINF_DEFINENAME 6
#define ZSYS_VAL_FINF_LASTERROR 7
#define ZSYS_VAL_FINF_LASTERRORDETAIL 8
#define ZSYS_VAL_FINF_PRTNINERROR 9
#define ZSYS_VAL_FINF_KEYINERROR 10
#define ZSYS_VAL_FINF_NEXTRECPOINTER 11
#define ZSYS_VAL_FINF_CURRRECPOINTER 12
#define ZSYS_VAL_FINF_CURRKEYSPECIFIER 13
#define ZSYS_VAL_FINF_CURRKEYLEN 14
#define ZSYS_VAL_FINF_CURRKEYVALUE 15
#define ZSYS_VAL_FINF_CURRPRIMKEYLEN 16
#define ZSYS_VAL_FINF_CURRPRIMKEYVALUE 17
#define ZSYS_VAL_FINF_TAPEVOL 18
#define ZSYS_VAL_FINF_HIGHOPENFILENUM 19
#define ZSYS_VAL_FINF_NEXTOPENFILENUM 20
#define ZSYS_VAL_FINF_OPENACCESSMODE 21
#define ZSYS_VAL_FINF_OPENEXCLUSMODE 22
#define ZSYS_VAL_FINF_OPENNOWAITDEPTH 23
#define ZSYS_VAL_FINF_OPENSYNCDEPTH 24
#define ZSYS_VAL_FINF_OPENOPTS 25
#define ZSYS_VAL_FINF_OPERATIONINFO 26
#define ZSYS_VAL_FINF_DEVICETYPE 30
#define ZSYS_VAL_FINF_DEVICESUBTYPE 31
#define ZSYS_VAL_FINF_DEMOUNTABLEDISK 32
#define ZSYS_VAL_FINF_AUDITEDDISK 33
#define ZSYS_VAL_FINF_PHYSICALRECLEN 34
#define ZSYS_VAL_FINF_LOGICALDEVICENUM 35
#define ZSYS_VAL_FINF_SUBDEVICENUM 36
#define ZSYS_VAL_FINF_SQLTYPE 40
#define ZSYS_VAL_FINF_FILETYPE 41
#define ZSYS_VAL_FINF_FILECODE 42
#define ZSYS_VAL_FINF_LOGICALRECLEN 43
#define ZSYS_VAL_FINF_BLOCKLEN 44
#define ZSYS_VAL_FINF_KEYOFFSET 45
#define ZSYS_VAL_FINF_KEYLEN 46
#define ZSYS_VAL_FINF_LOCKKEYLEN 47
#define ZSYS_VAL_FINF_PRIMEXTENTSIZE 50
#define ZSYS_VAL_FINF_SECNDEXTENTSIZE 51
#define ZSYS_VAL_FINF_MAXIMUMEXTENTS 52
#define ZSYS_VAL_FINF_ALLOCATEDEXTENTS 53
#define ZSYS_VAL_FINF_CREATIONDATE 54
#define ZSYS_VAL_FINF_LASTOPENDATE 56
#define ZSYS_VAL_FINF_EXPIRATIONDATE 57
#define ZSYS_VAL_FINF_FILEOWNER 58
#define ZSYS_VAL_FINF_SAFEGUARDSECUR 59
#define ZSYS_VAL_FINF_PROGIDSECURITY 60
#define ZSYS_VAL_FINF_CLEARONPURGE 61
#define ZSYS_VAL_FINF_GUARDIANSECURITY 62
#define ZSYS_VAL_FINF_LICENSED 63
#define ZSYS_VAL_FINF_ODDUNSTRUCTURED 65
#define ZSYS_VAL_FINF_AUDIT 66
#define ZSYS_VAL_FINF_AUDITCOMPRESS 67
#define ZSYS_VAL_FINF_DATACOMPRESS 68
#define ZSYS_VAL_FINF_INDEXCOMPRESS 69
#define ZSYS_VAL_FINF_REFRESH 70
#define ZSYS_VAL_FINF_CREATEOPTS 71
#define ZSYS_VAL_FINF_WRITETHRU 72
#define ZSYS_VAL_FINF_VERIFYWRITES 73
#define ZSYS_VAL_FINF_SERIALWRITES 74
#define ZSYS_VAL_FINF_FILEISOPEN 75
#define ZSYS_VAL_FINF_CRASHOPEN 76
#define ZSYS_VAL_FINF_ROLLFWDNEEDED 77
#define ZSYS_VAL_FINF_BROKEN 78
#define ZSYS_VAL_FINF_CORRUPT 79
#define ZSYS_VAL_FINF_SECNDPRTN 80
#define ZSYS_VAL_FINF_INDEXLEVELS 81
#define ZSYS_VAL_FINF_SQLPROGRAM 82
#define ZSYS_VAL_FINF_SQLVALID 83
#define ZSYS_VAL_FINF_SQLCATLOGNAMELEN 84
#define ZSYS_VAL_FINF_SQLCATLOGNAME 85
#define ZSYS_VAL_FINF_NUMPRTNS 90
#define ZSYS_VAL_FINF_PRTNDESC 91
#define ZSYS_VAL_FINF_PRTNVOLLEN 92
#define ZSYS_VAL_FINF_PRTNVOLNAMES 93
#define ZSYS_VAL_FINF_PRTNPARTKEYLEN 94
#define ZSYS_VAL_FINF_PRTNPARTKEYVAL 95
#define ZSYS_VAL_FINF_PRTNTOTALNAMELEN 96
#define ZSYS_VAL_FINF_NUMALTKEYS 100
#define ZSYS_VAL_FINF_ALTKEYDESC 101
#define ZSYS_VAL_FINF_NUMALTKEYFILES 102
#define ZSYS_VAL_FINF_ALTFILELEN 103
#define ZSYS_VAL_FINF_ALTFILENAMES 104
#define ZSYS_VAL_FINF_ALTTOTALNAMELEN 105
#define ZSYS_VAL_FINF_VOLCAPACITY 110
#define ZSYS_VAL_FINF_VOLFREESPACE 111
#define ZSYS_VAL_FINF_VOLFRAGS 112
#define ZSYS_VAL_FINF_VOLLARGESTFRAG 113
#define ZSYS_VAL_FINF_DISKDRIVETYPES 114
#define ZSYS_VAL_FINF_DISKDRIVECAP 115
#define ZSYS_VAL_FINF_SEQBLOCKBUFFER 116
#define ZSYS_VAL_FINF_LASTOPEN_LCT 117
#define ZSYS_VAL_FINF_EXPIRE_LCT 118
#define ZSYS_VAL_FINF_CREATION_LCT 119
#define ZSYS_VAL_FINF_PRTNENDOFFILE 136
#define ZSYS_VAL_FINF_PRTNMAXIMUMSIZE 137
#define ZSYS_VAL_FINF_PRTNMODIFY 140
#define ZSYS_VAL_FINF_PRTNMODIFY_LCT 141
#define ZSYS_VAL_FINF_AGGRENDOFFILE 142
#define ZSYS_VAL_FINF_AGGRMAXIMUMSIZE 143
#define ZSYS_VAL_FINF_AGGRMODIFY 144
#define ZSYS_VAL_FINF_AGGRMODIFY_LCT 145
#define ZSYS_VAL_FINF_OSSFILE 161
#define ZSYS_VAL_FINF_TMFREDONEEDED 162
#define ZSYS_VAL_FINF_TMFUNDONEEDED 163
#define ZSYS_VAL_FINF_GID 164
#define ZSYS_VAL_FINF_OSSMODE 165
#define ZSYS_VAL_FINF_ISOSSOPEN 166
#define ZSYS_VAL_FINF_UID 167
#define ZSYS_VAL_FINF_OSSNUMBEROFLINKS 168
#define ZSYS_VAL_FINF_SECURITYTYPE 169
#define ZSYS_VAL_FINF_AGGRENDOFFILE64 191
#define ZSYS_VAL_FINF_AGGRMAXSIZE64 192
#define ZSYS_VAL_FINF_PRTNENDOFFILE64 193
#define ZSYS_VAL_FINF_PRTNMAXSIZE64 194
#define ZSYS_VAL_FINF_FILEFORMAT 195
#define ZSYS_VAL_FINF_RECORDLEN32 196
#define ZSYS_VAL_FINF_BLOCKLEN32 197
#define ZSYS_VAL_FINF_KEYOFFSET32 198
#define ZSYS_VAL_FINF_PRIMEXTENT32 199
#define ZSYS_VAL_FINF_SCNDEXTENT32 200
#define ZSYS_VAL_FINF_NEXTRECPOINTER64 201
#define ZSYS_VAL_FINF_CURRRECPOINTER64 202
#define ZSYS_VAL_FINF_CURRKEYLEN64 203
#define ZSYS_VAL_FINF_CURRKEYVALUE64 204
#define ZSYS_VAL_FINF_CURRPRIMKEYLEN64 205
#define ZSYS_VAL_FINF_CURRPRIMKEYVAL64 206
#define ZSYS_VAL_FINF_BLKXSUMMING 212
#define ZSYS_VAL_FINF_PFSSIZE 216
#define ZSYS_VAL_FINF_PFSCURRUSAGE 217
#define ZSYS_VAL_FINF_PFSMAXIMUMUSAGE 218
#define ZSYS_VAL_FINF_PRTNMXEXTENTS 221
#define ZSYS_VAL_FINF_ENHANCEDCKSUMDSK 223
#define ZSYS_VAL_FINF_NOUNSTRUCCKSUM 224
#define ZSYS_VAL_FINF_ISSQLMXOBJ 225
#define ZSYS_VAL_FINF_MXPHYSICALOBJ 226
#define ZSYS_VAL_FINF_MXPRTNMETHOD 227
#define ZSYS_VAL_FINF_ANSINAMELEN 228
#define ZSYS_VAL_FINF_ANSINAME 229
#define ZSYS_VAL_FINF_ANSINAMESPACE 230
#define ZSYS_VAL_FINF_NUMLOBTABLES 231
#define ZSYS_VAL_FINF_LOBTABLESLEN 232
#define ZSYS_VAL_FINF_LOBTABLES 233
#define ZSYS_VAL_FINF_FILETRUSTME 235
#define ZSYS_VAL_FINF_DISKDRIVETYPES2 236
// #pragma section altkey_flag_bits
#define ZSYS_MSK_ALTKEY_NONULLUPD -32768
#define ZSYS_SFT_ALTKEY_NONULLUPD 15
#define ZSYS_MSK_ALTKEY_UNIQUE 16384
#define ZSYS_SFT_ALTKEY_UNIQUE 14
#define ZSYS_MSK_ALTKEY_NOUPDATE 8192
#define ZSYS_SFT_ALTKEY_NOUPDATE 13
#define ZSYS_MSK_ALTKEY_INSRTORDR 4096
#define ZSYS_SFT_ALTKEY_INSRTORDR 12
// #pragma section zsys_ddl_altkey
#pragma fieldalign shared2 __zsys_ddl_altkey
typedef struct __zsys_ddl_altkey
{
   short                           z_specifier;
   short                           z_len;
   short                           z_offset;
   short                           z_altfile;
   short                           z_nullvalue;
   short                           z_flags;
} zsys_ddl_altkey_def;
// #pragma section zsys_ddl_partition
#pragma fieldalign shared2 __zsys_ddl_partition
typedef struct __zsys_ddl_partition
{
   short                           z_primextentsize;
   short                           z_secndextentsize;
} zsys_ddl_partition_def;
// #pragma section zsys_ddl_typeinformation
#pragma fieldalign shared2 __zsys_ddl_typeinformation
typedef struct __zsys_ddl_typeinformation
{
   short                           z_devicetype;
   short                           z_devicesubtype;
   union
   {
      struct
      {
         short                           zobjtype;
         short                           zfiletype;
         short                           zfilecode;
      } z_disk;
      short                           z_devinfo[3];
   } u_z_disk;
} zsys_ddl_typeinformation_def;
// #pragma section zsys_ddl_receiveinformation
#pragma fieldalign shared2 __zsys_ddl_receiveinformation
typedef struct __zsys_ddl_receiveinformation
{
   short                           z_iotype;
#ifndef USE_SB_NEW_RI
   short                           z_maxreplycount;
#else
   Int32                           z_maxreplycount;
#endif
   short                           z_messagetag;
   short                           z_filenum;
   long                            z_syncid;
   zsys_ddl_phandle_def            z_sender;
   short                           z_openlabel;
   int			           user_id; // SB_Uid_Type
} zsys_ddl_receiveinformation_def;
// #pragma section lock_flag_bits
#define ZSYS_MSK_LOCK_GENERICLOCK -32768
#define ZSYS_SFT_LOCK_GENERICLOCK 15
// #pragma section zsys_ddl_lock
#pragma fieldalign shared2 __zsys_ddl_lock
typedef struct __zsys_ddl_lock
{
   short                           z_locktype;
   short                           z_flags;
   short                           z_numofparticipants;
   long                            z_recid;
   short                           z_keylen;
   char                            z_keyvalue[256];
} zsys_ddl_lock_def;
// #pragma section lockparticipant_flag_bits
#define ZSYS_MSK_LOCKPART0_ISPH -32768
#define ZSYS_SFT_LOCKPART0_ISPH 15
#define ZSYS_MSK_LOCKPART0_GRNTST 16384
#define ZSYS_SFT_LOCKPART0_GRNTST 14
#define ZSYS_MSK_LOCKPART0_INTENT 8192
#define ZSYS_SFT_LOCKPART0_INTENT 13
// #pragma section zsys_ddl_lockparticipant
#pragma fieldalign shared2 __zsys_ddl_lockparticipant
typedef struct __zsys_ddl_lockparticipant
{
   union
   {
      long                            z_allflags;
      short                           z_flags[2];
   } u_z_allflags;
   union
   {
      zsys_ddl_phandle_def            z_phandle;
      struct
      {
         short                           zword[6];
      } z_transid;
   } u_z_phandle;
} zsys_ddl_lockparticipant_def;
// #pragma section process_constant
#define ZSYS_VAL_PCREATOPT_NONAME 0
#define ZSYS_VAL_PCREATOPT_NAMEINCALL 1
#define ZSYS_VAL_PCREATOPT_NAMEDBYSYS 2
#define ZSYS_VAL_PCREATOPT_CALLERSNAME 3
#define ZSYS_VAL_PCREATOPT_NAMEDBYSYS5 4
#define ZSYS_VAL_PCREATOPT_ANYANCESTOR 64
#define ZSYS_VAL_PCREATOPT_FRCLOWOVER 32
#define ZSYS_VAL_PCREATOPT_ALLDEFINES 16
#define ZSYS_VAL_PCREATOPT_DEFINELIST 8
#define ZSYS_VAL_PCREATOPT_DEFOVERRIDE 4
#define ZSYS_VAL_PCREATOPT_DEFENABLED 2
#define ZSYS_VAL_PCREATOPT_LOWPIN 1
#define ZSYS_VAL_PCREATOPT_DEFAULT 0
#define ZSYS_VAL_PCREATOPT_RUND 8
#define ZSYS_VAL_PCREATOPT_SAVEABEND 4
#define ZSYS_VAL_PCREATOPT_DBGOVERRIDE 2
#define ZSYS_VAL_PCREATOPT_INSPECT 1
#define ZSYS_VAL_PSPAWNOPT_OSSDEFAULT 0
#define ZSYS_VAL_SPAWN_SETGROUP 1
#define ZSYS_VAL_SPAWN_SETSIGMASK 2
#define ZSYS_VAL_SPAWN_SETSIGDEF 4
#define ZSYS_VAL_SPAWN_NOTDEFD -8
#define ZSYS_VAL_SPAWN_NEWPGGROUP -1
// #pragma section zsys_ddl_fdentry
#pragma fieldalign shared2 __zsys_ddl_fdentry
typedef struct __zsys_ddl_fdentry
{
   long                            z_fd;
   long                            z_dupfd;
   zsys_ddl_char_extaddr_def       z_name;
   long                            z_oflag;
   long                            z_mode;
} zsys_ddl_fdentry_def;
// #pragma section zsys_ddl_fdinfo
#pragma fieldalign shared2 __zsys_ddl_fdinfo
typedef struct __zsys_ddl_fdinfo
{
   long                            z_len;
   long                            z_timeout;
   long                            z_umask;
   zsys_ddl_char_extaddr_def       z_cwd;
   long                            z_fdcount;
   zsys_ddl_fdentry_def            z_fdentry;
} zsys_ddl_fdinfo_def;
// #pragma section zsys_ddl_inheritance
#pragma fieldalign shared2 __zsys_ddl_inheritance
typedef struct __zsys_ddl_inheritance
{
   short                           z_flags;
   short                           z_filler;
   long                            z_pgroup;
   long                            z_sigmask;
   long                            z_sigdefault;
} zsys_ddl_inheritance_def;
// #pragma section zsys_ddl_sigset
#pragma fieldalign shared2 __zsys_ddl_sigset
typedef struct __zsys_ddl_sigset

	{
	Int64                           z_item64[2];
} zsys_ddl_sigset_def;
// #pragma section zsys_ddl_inheritance_native
#pragma fieldalign shared2 __zsys_ddl_inheritance_native
typedef struct __zsys_ddl_inheritance_native
{
   short                           z_flags;
   short                           z_filler;
   long                            z_pgroup;
   zsys_ddl_sigset_def             z_sigmask;
   zsys_ddl_sigset_def             z_sigdefault;
} zsys_ddl_inheritance_native_def;
// #pragma section zsys_ddl_processextension
#pragma fieldalign shared2 __zsys_ddl_processextension
typedef struct __zsys_ddl_processextension
{
   long                            z_len;
   zsys_ddl_char_extaddr_def       z_libraryname;
   zsys_ddl_char_extaddr_def       z_swapfilename;
   zsys_ddl_char_extaddr_def       z_extswapfilename;
   short                           z_priority;
   short                           z_cpu;
   unsigned short                  z_nameoptions;
   short                           z_filler;
   zsys_ddl_char_extaddr_def       z_processname;
   zsys_ddl_char_extaddr_def       z_hometerm;
   short                           z_memorypages;
   short                           z_jobid;
   unsigned short                  z_createoptions;
   short                           z_filler1;
   zsys_ddl_extaddr_def            z_defines;
   unsigned short                  z_defineslen;
   unsigned short                  z_debugoptions;
   long                            z_pfssize;
   unsigned short                  z_ossoptions;
   short                           z_filler2;
   long                            z_mainstackmax;
   long                            z_heapmax;
   long                            z_spaceguarantee;
} zsys_ddl_processextension_def;
// #pragma section zsys_ddl_processresults
#pragma fieldalign shared2 __zsys_ddl_processresults
typedef struct __zsys_ddl_processresults
{
   long                            z_len;
   zsys_ddl_phandle_def            z_phandle;
   long                            z_pid;
   long                            z_errno;
   short                           z_tpcerror;
   short                           z_tpcdetail;
} zsys_ddl_processresults_def;
// #pragma section process_itemcodes
#define ZSYS_VAL_PINF_FIND_EXACT 0
#define ZSYS_VAL_PINF_FIND_FIRST 1
#define ZSYS_VAL_PINF_FIND_MANY 2
#define ZSYS_VAL_PINF_FIND_OSS_PID 3
#define ZSYS_VAL_PINF_FIND_BY_VADDR 120
#define ZSYS_VAL_PINF_CREATOR_AID 1
#define ZSYS_VAL_PINF_PROCESS_AID 2
#define ZSYS_VAL_PINF_MAX_PRIORITY 3
#define ZSYS_VAL_PINF_PROGRAM_FILE 4
#define ZSYS_VAL_PINF_HOMETERM 5
#define ZSYS_VAL_PINF_GMOM_PHANDLE 6
#define ZSYS_VAL_PINF_JOBID 7
#define ZSYS_VAL_PINF_SUBDEVICE_TYPE 8
#define ZSYS_VAL_PINF_MIN_PRIORITY 9
#define ZSYS_VAL_PINF_MIN_CREATETIME 12
#define ZSYS_VAL_PINF_MAX_CREATETIME 13
#define ZSYS_VAL_PINF_LOWERED_PRIORITY 14
#define ZSYS_VAL_PINF_REAL_GROUPID 21
#define ZSYS_VAL_PINF_REAL_USERID 22
#define ZSYS_VAL_PINF_EFF_USERID 23
#define ZSYS_VAL_PINF_SESSIONID 26
#define ZSYS_VAL_PINF_CTTY 27
#define ZSYS_VAL_PINF_PROCESS_TYPE 28
#define ZSYS_VAL_PINF_PROCESS_TIME 30
#define ZSYS_VAL_PINF_WAIT_STATE 31
#define ZSYS_VAL_PINF_PROCESS_STATE 32
#define ZSYS_VAL_PINF_LIBRARY_FILE 33
#define ZSYS_VAL_PINF_SWAP_FILE 34
#define ZSYS_VAL_PINF_CONTEXT_CHANGES 35
#define ZSYS_VAL_PINF_DEFINE_MODE 36
#define ZSYS_VAL_PINF_LICENSES 37
#define ZSYS_VAL_PINF_PIN 38
#define ZSYS_VAL_PINF_PROCESS_DESCR 39
#define ZSYS_VAL_PINF_MOM_PHANDLE 40
#define ZSYS_VAL_PINF_FILE_SECURITY 41
#define ZSYS_VAL_PINF_CURRENT_PRIORITY 42
#define ZSYS_VAL_PINF_INITIAL_PRIORITY 43
#define ZSYS_VAL_PINF_REMOTE_CREATOR 44
#define ZSYS_VAL_PINF_LOGON_STATE 45
#define ZSYS_VAL_PINF_EXT_SWAP_FILE 46
#define ZSYS_VAL_PINF_PRIMARY 47
#define ZSYS_VAL_PINF_PHANDLE 48
#define ZSYS_VAL_PINF_QUAL_INFO_AVAIL 49
#define ZSYS_VAL_PINF_TSN_LOGON 50
#define ZSYS_VAL_PINF_FORCED_LOW 51
#define ZSYS_VAL_PINF_CREATION_TIME 53
#define ZSYS_VAL_PINF_PAGES 54
#define ZSYS_VAL_PINF_MSGS_SENT 55
#define ZSYS_VAL_PINF_MSGS_RCVD 56
#define ZSYS_VAL_PINF_RCVQ_LEN 57
#define ZSYS_VAL_PINF_RCVQ_MAX_LEN 58
#define ZSYS_VAL_PINF_PAGE_FAULTS 59
#define ZSYS_VAL_PINF_TNS_EMUL_TIME 60
#define ZSYS_VAL_PINF_TNS_TRAP_COUNT 61
#define ZSYS_VAL_PINF_IS_NAMED 62
#define ZSYS_VAL_PINF_MOMS_NAME 65
#define ZSYS_VAL_PINF_GMOMS_NAME 66
#define ZSYS_VAL_PINF_TSN_LOGOFF 67
#define ZSYS_VAL_PINF_LOGON_INHERIT 68
#define ZSYS_VAL_PINF_LOGOFF_STOP 69
#define ZSYS_VAL_PINF_PROP_LOGON 70
#define ZSYS_VAL_PINF_PROP_LOGOFF_STOP 71
#define ZSYS_VAL_PINF_LOGON_FLAGS 72
#define ZSYS_VAL_PINF_ATTRIBUTE_FLAG 73
#define ZSYS_VAL_PINF_EFF_GROUPID 80
#define ZSYS_VAL_PINF_SAVED_GROUPID 81
#define ZSYS_VAL_PINF_USERNAME 82
#define ZSYS_VAL_PINF_SUPP_GROUPS 83
#define ZSYS_VAL_PINF_SAVED_USERID 84
#define ZSYS_VAL_PINF_EFF_USERNAME 88
#define ZSYS_VAL_PINF_EFF_ALIAS 89
#define ZSYS_VAL_PINF_OSS_PID 90
#define ZSYS_VAL_PINF_COMMAND 91
#define ZSYS_VAL_PINF_ARGUMENTS 92
#define ZSYS_VAL_PINF_OSS_PATHNAME 93
#define ZSYS_VAL_PINF_OSS_PARENT_PID 94
#define ZSYS_VAL_PINF_OSS_ELAPSETIME 95
#define ZSYS_VAL_PINF_OSS_CPUTIME 96
#define ZSYS_VAL_PINF_OSS_STARTTIME 97
#define ZSYS_VAL_PINF_PROCESS_GROUPID 98
#define ZSYS_VAL_PINF_OSS_FLAG 99
#define ZSYS_VAL_PINF_PFS_SIZE 100
#define ZSYS_VAL_PINF_SERVERCLASS_NAME 101
#define ZSYS_VAL_PINF_MAINSTACK_ORG 102
#define ZSYS_VAL_PINF_MAINSTACK_SIZE 103
#define ZSYS_VAL_PINF_MAINSTACK_MAX 104
#define ZSYS_VAL_PINF_PRIVSTACK_ORG 105
#define ZSYS_VAL_PINF_PRIVSTACK_SIZE 106
#define ZSYS_VAL_PINF_PRIVSTACK_MAX 107
#define ZSYS_VAL_PINF_GLOBALS_ORG 108
#define ZSYS_VAL_PINF_GLOBALS_SIZE 109
#define ZSYS_VAL_PINF_HEAP_ORG 110
#define ZSYS_VAL_PINF_HEAP_SIZE 111
#define ZSYS_VAL_PINF_HEAP_MAX 112
#define ZSYS_VAL_PINF_SPACE_GUARANTEE 113
#define ZSYS_VAL_PINF_NATIVE_FLAG 119
// #pragma section pool_constant
#define ZSYS_VAL_POOL_OK 0
#define ZSYS_VAL_POOL_PARAMETER 2
#define ZSYS_VAL_POOL_BOUNDS 3
#define ZSYS_VAL_POOL_INVALIDSIZE 4
#define ZSYS_VAL_POOL_POINTERALIGN 5
#define ZSYS_VAL_POOL_INVALIDALIGN 6
#define ZSYS_VAL_POOL_NOMEM 7
#define ZSYS_VAL_POOL_BADHEADER 9
#define ZSYS_VAL_POOL_NOSPACE 10
#define ZSYS_VAL_POOL_BADSPACE 11
#define ZSYS_VAL_POOL_BADFREE 12
#define ZSYS_VAL_POOL_CANTSHRINK 13
// #pragma section segment_constant
#define ZSYS_VAL_SEGALLOCTYPE_DEFAULT 0
#define ZSYS_VAL_SEGALLOCTYPE_EXTENSBL 1
#define ZSYS_VAL_SEGALLOCTYPE_DEFFNAME 2
#define ZSYS_VAL_SEGALLOCTYPE_EXTFNAME 3
#define ZSYS_VAL_SEGALLOCTYPE_WINHIBIT 4
#define ZSYS_VAL_SEGALLOCTYPE_WIFNAME 6
// #pragma section invalid_address_constant
#define ZSYS_VAL_NULL_ADDRESS -262144L
#define ZSYS_VAL_NIL_ADDRESS -262144L
// #pragma section system_messages_constant
#define ZSYS_VAL_SMSG_LEN 250
#define ZSYS_VAL_SMSG_WLEN 125
#define ZSYS_VAL_SMSG_CPUDOWN -2
#define ZSYS_VAL_SMSG_CPUUP -3
#define ZSYS_VAL_SMSG_SETTIME -10
#define ZSYS_VAL_SMSG_POWERON -11
#define ZSYS_VAL_SMSG_MSGMISSED -13
#define ZSYS_VAL_SMSG_3270 -21
#define ZSYS_VAL_SMSG_TIMESIGNAL -22
#define ZSYS_VAL_SMSG_LOCKMEM -23
#define ZSYS_VAL_SMSG_LOCKMEMFAIL -24
#define ZSYS_VAL_SMSG_PROCTIMESIG -26
#define ZSYS_VAL_SMSG_CONTROL -32
#define ZSYS_VAL_SMSG_SETMODE -33
#define ZSYS_VAL_SMSG_RESETSYNC -34
#define ZSYS_VAL_SMSG_CONTROLBUF -35
#define ZSYS_VAL_SMSG_SETPARAM -37
#define ZSYS_VAL_SMSG_QMSGCANCELLED -38
#define ZSYS_VAL_SMSG_DEVINFOCOMP -41
#define ZSYS_VAL_SMSG_REMOTECPUDOWN -100
#define ZSYS_VAL_SMSG_PROCDEATH -101
#define ZSYS_VAL_SMSG_PROCCREATE -102
#define ZSYS_VAL_SMSG_OPEN -103
#define ZSYS_VAL_SMSG_CLOSE -104
#define ZSYS_VAL_SMSG_BREAK -105
#define ZSYS_VAL_SMSG_DEVINFO -106
#define ZSYS_VAL_SMSG_SUBNAME -107
#define ZSYS_VAL_SMSG_FILEINFO -108
#define ZSYS_VAL_SMSG_FILENAMENEXT -109
#define ZSYS_VAL_SMSG_NODEDOWN -110
#define ZSYS_VAL_SMSG_NODEUP -111
#define ZSYS_VAL_SMSG_GMOMNOTIFY -112
#define ZSYS_VAL_SMSG_REMOTECPUUP -113
#define ZSYS_VAL_SMSG_PROCSPAWN -141
#define ZSYS_VAL_SETTIME_INITIAL 0
#define ZSYS_VAL_SETTIME_CORRECTION 1
#define ZSYS_VAL_SETTIME_DAYLTSAV 2
#define ZSYS_VAL_PROCDEATH_STOP 0
#define ZSYS_VAL_PROCDEATH_ABEND 5
#define ZSYS_VAL_PROCDEATH_EXTERNAL 6
#define ZSYS_VAL_PROCDEATH_SIGNAL 9
#define ZSYS_VAL_PROCDEATH_TRAP -1
#define ZSYS_VAL_PROCDEATH_NORESOURCE -2
#define ZSYS_VAL_PROCDEATH_BADPARAMS -3
#define ZSYS_VAL_PROCDEATH_CPUDOWN -4
#define ZSYS_VAL_PROCDEATH_COMMFAILURE -5
#define ZSYS_VAL_PROCDEATH_HWEXCEPTION -6
#define ZSYS_VAL_PROCDEATH_STATEINVAL -7
#define ZSYS_VAL_PROCDEATH_STACKOVFLOW -8
#define ZSYS_VAL_PROCDEATH_NOPRIVSTACK -9
#define ZSYS_VAL_PROCDEATH_NOSIGRESRCE -10
#define ZSYS_VAL_PROCDEATH_CANTRESUME -11
#define ZSYS_VAL_PROCDEATH_CHILDEXEC -12
#define ZSYS_VAL_PROCDEATH_PSPAWNFSERR -13
#define ZSYS_MSK_SETMD_PARM1_VALID 4
#define ZSYS_SFT_SETMD_PARM1_VALID 2
#define ZSYS_MSK_SETMD_PARM2_VALID 2
#define ZSYS_SFT_SETMD_PARM2_VALID 1
#define ZSYS_MSK_SETMD_LPARM_VALID 1
#define ZSYS_SFT_SETMD_LPARM_VALID 0
#define ZSYS_MSK_SETPM_PARM_VALID 2
#define ZSYS_SFT_SETPM_PARM_VALID 1
#define ZSYS_MSK_SETPM_LPARM_VALID 1
#define ZSYS_SFT_SETPM_LPARM_VALID 0
#define ZSYS_MSK_PDEATH_SYSTYPE 2
#define ZSYS_SFT_PDEATH_SYSTYPE 1
#define ZSYS_MSK_PDEATH_ABENDED 1
#define ZSYS_SFT_PDEATH_ABENDED 0
#define ZSYS_MSK_OPEN_IDNOT_VERIF 4
#define ZSYS_SFT_OPEN_IDNOT_VERIF 2
#define ZSYS_MSK_OPEN_DIFF_NODE 2
#define ZSYS_SFT_OPEN_DIFF_NODE 1
#define ZSYS_MSK_OPEN_BACKUPOPEN 1
#define ZSYS_SFT_OPEN_BACKUPOPEN 0
#define ZSYS_MSK_SUBNAM_SKIPNAME -32768
#define ZSYS_SFT_SUBNAM_SKIPNAME 15
// #pragma section zsys_ddl_smsg_cpudown
#pragma fieldalign shared2 __zsys_ddl_smsg_cpudown
typedef struct __zsys_ddl_smsg_cpudown
{
   short                           z_msgnumber;
   short                           z_cpunumber;
} zsys_ddl_smsg_cpudown_def;
// #pragma section zsys_ddl_smsg_cpuup
#pragma fieldalign shared2 __zsys_ddl_smsg_cpuup
typedef struct __zsys_ddl_smsg_cpuup
{
   short                           z_msgnumber;
   short                           z_cpunumber;
} zsys_ddl_smsg_cpuup_def;
// #pragma section zsys_ddl_smsg_settime
#pragma fieldalign shared2 __zsys_ddl_smsg_settime
typedef struct __zsys_ddl_smsg_settime
{
   short                           z_msgnumber;
   short                           z_cpunumber;
	Int64                          z_timedelta;
   short                           z_reasoncode;
} zsys_ddl_smsg_settime_def;
// #pragma section zsys_ddl_smsg_poweron
#pragma fieldalign shared2 __zsys_ddl_smsg_poweron
typedef struct __zsys_ddl_smsg_poweron
{
   short                           z_msgnumber;
   short                           z_cpunumber;
} zsys_ddl_smsg_poweron_def;
// #pragma section zsys_ddl_smsg_msgmissed
#pragma fieldalign shared2 __zsys_ddl_smsg_msgmissed
typedef struct __zsys_ddl_smsg_msgmissed
{
   short                           z_msgnumber;
} zsys_ddl_smsg_msgmissed_def;
// #pragma section zsys_ddl_smsg_3270
#pragma fieldalign shared2 __zsys_ddl_smsg_3270
typedef struct __zsys_ddl_smsg_3270
{
   short                           z_msgnumber;
   short                           z_responseid;
   union
   {
      short                           z_status0;
      struct
      {
         signed char                     zsense;
         signed char                     zstatus;
      } z_status;
   } u_z_status0;
   short                           z_translated_status;
} zsys_ddl_smsg_3270_def;
// #pragma section zsys_ddl_smsg_timesignal
#pragma fieldalign shared2 __zsys_ddl_smsg_timesignal
typedef struct __zsys_ddl_smsg_timesignal
{
   short                           z_msgnumber;
   short                           z_parm1;
   long                            z_parm2;
} zsys_ddl_smsg_timesignal_def;
// #pragma section zsys_ddl_smsg_lockmem
#pragma fieldalign shared2 __zsys_ddl_smsg_lockmem
typedef struct __zsys_ddl_smsg_lockmem
{
   short                           z_msgnumber;
   short                           z_parm1;
   long                            z_parm2;
} zsys_ddl_smsg_lockmem_def;
// #pragma section zsys_ddl_smsg_lockmemfail
#pragma fieldalign shared2 __zsys_ddl_smsg_lockmemfail
typedef struct __zsys_ddl_smsg_lockmemfail
{
   short                           z_msgnumber;
   short                           z_parm1;
   long                            z_parm2;
} zsys_ddl_smsg_lockmemfail_def;
// #pragma section zsys_ddl_smsg_proctimesig
#pragma fieldalign shared2 __zsys_ddl_smsg_proctimesig
typedef struct __zsys_ddl_smsg_proctimesig
{
   short                           z_msgnumber;
   short                           z_parm1;
   long                            z_parm2;
} zsys_ddl_smsg_proctimesig_def;
// #pragma section zsys_ddl_smsg_control
#pragma fieldalign shared2 __zsys_ddl_smsg_control
typedef struct __zsys_ddl_smsg_control
{
   short                           z_msgnumber;
   short                           z_operation;
   short                           z_param;
} zsys_ddl_smsg_control_def;
// #pragma section zsys_ddl_smsg_setmode
#pragma fieldalign shared2 __zsys_ddl_smsg_setmode
typedef struct __zsys_ddl_smsg_setmode
{
   short                           z_msgnumber;
   short                           z_function;
   short                           z_param1;
   short                           z_param2;
   short                           z_flags;
} zsys_ddl_smsg_setmode_def;
// #pragma section zsys_ddl_smsg_setmode_reply
#pragma fieldalign shared2 __zsys_ddl_smsg_setmode_reply
typedef struct __zsys_ddl_smsg_setmode_reply
{
   short                           z_msgnumber;
   short                           z_param1;
   short                           z_param2;
} zsys_ddl_smsg_setmode_reply_def;
// #pragma section zsys_ddl_smsg_resetsync
#pragma fieldalign shared2 __zsys_ddl_smsg_resetsync
typedef struct __zsys_ddl_smsg_resetsync
{
   short                           z_msgnumber;
} zsys_ddl_smsg_resetsync_def;
// #pragma section zsys_ddl_smsg_controlbuf
#pragma fieldalign shared2 __zsys_ddl_smsg_controlbuf
typedef struct __zsys_ddl_smsg_controlbuf
{
   short                           z_msgnumber;
   short                           z_operation;
   short                           z_count;
   signed char                     z_buffer[244];
} zsys_ddl_smsg_controlbuf_def;
// #pragma section zsys_ddl_smsg_setparam
#pragma fieldalign shared2 __zsys_ddl_smsg_setparam
typedef struct __zsys_ddl_smsg_setparam
{
   short                           z_msgnumber;
   short                           z_function;
   short                           z_flags;
   short                           z_count;
   signed char                     z_buffer[242];
} zsys_ddl_smsg_setparam_def;
// #pragma section zsys_ddl_smsg_setparam_rply
#pragma fieldalign shared2 __zsys_ddl_smsg_setparam_rply
typedef struct __zsys_ddl_smsg_setparam_rply
{
   short                           z_msgnumber;
   short                           z_count;
   signed char                     z_buffer[246];
} zsys_ddl_smsg_setparam_rply_def;
// #pragma section zsys_ddl_smsg_qmsgcancelled
#pragma fieldalign shared2 __zsys_ddl_smsg_qmsgcancelled
typedef struct __zsys_ddl_smsg_qmsgcancelled
{
   short                           z_msgnumber;
   short                           z_tag;
} zsys_ddl_smsg_qmsgcancelled_def;
// #pragma section zsys_ddl_smsg_devinfocomp
#pragma fieldalign shared2 __zsys_ddl_smsg_devinfocomp
typedef struct __zsys_ddl_smsg_devinfocomp
{
   short                           z_msgnumber;
   long                            z_tag;
   short                           z_error;
   short                           z_reclen;
   short                           z_diskprocversion;
} zsys_ddl_smsg_devinfocomp_def;
// #pragma section zsys_ddl_smsg_remotecpudown
#pragma fieldalign shared2 __zsys_ddl_smsg_remotecpudown
typedef struct __zsys_ddl_smsg_remotecpudown
{
   short                           z_msgnumber;
   long                            z_nodenumber;
   short                           z_cpunumber;
   short                           z_nodename_len;
   short                           z_reserved[3];
   char                            z_nodename[8];
} zsys_ddl_smsg_remotecpudown_def;
// #pragma section zsys_ddl_smsg_procdeath
#pragma fieldalign shared2 __zsys_ddl_smsg_procdeath
typedef struct __zsys_ddl_smsg_procdeath
{
   union
   {
      short                           z_msgnumber;
      char                            z_base[2];
   } u_z_msgnumber;
   zsys_ddl_phandle_def            z_phandle;
	Int64                          z_cputime;
   short                           z_jobid;
   short                           z_completion_code;
   union
   {
      short                           z_termination_code;
      short                           z_killer_craid;
   } u_z_termination_code;
   zsys_ddl_ssid_def               z_subsystem;
   zsys_ddl_phandle_def            z_killer;
   short                           z_termtext_len;
   struct
   {
      short                           zoffset;
      short                           zlen;
   } z_procname;
   short                           z_flags;
   long                            z_osspid;
   short                           z_reserved;
   union
   {
      struct
      {
         signed char                     filler_0[112];
      } z_data;
      char                            z_termtext[112];
   } u_z_data;
} zsys_ddl_smsg_procdeath_def;
// #pragma section zsys_ddl_smsg_proccreate
#pragma fieldalign shared2 __zsys_ddl_smsg_proccreate
typedef struct __zsys_ddl_smsg_proccreate
{
   short                           z_msgnumber;
   long                            z_tag;
   zsys_ddl_phandle_def            z_phandle;
   int                             z_error;
   short                           z_error_detail; // not used
   short                           z_procname_len; // not used
   short                           z_reserved[4];
   union
   {
      struct
      {
         signed char                     filler_0[50];
      } z_data;
      char                            z_procname[50];
   } u_z_data;
} zsys_ddl_smsg_proccreate_def;
// #pragma section zsys_ddl_smsg_open
#pragma fieldalign shared2 __zsys_ddl_smsg_open
typedef struct __zsys_ddl_smsg_open
{
   union
   {
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
   zsys_ddl_phandle_def            z_primary;
   short                           z_qualifier_len;
   struct
   {
      short                           zoffset;
      short                           zlen;
   } z_opener_name;
   short                           z_primary_fnum;
   short                           z_craid;
   struct
   {
      short                           zoffset;
      short                           zlen;
   } z_hometerm_name;
   short                           z_reserved[5];
   union
   {
      struct
      {
         signed char                     filler_0[102];
      } z_data;
      char                            z_qualifier[102];
   } u_z_data;
} zsys_ddl_smsg_open_def;
// #pragma section zsys_ddl_smsg_open_reply
#pragma fieldalign shared2 __zsys_ddl_smsg_open_reply
typedef struct __zsys_ddl_smsg_open_reply
{
   short                           z_msgnumber;
   short                           z_openid;
} zsys_ddl_smsg_open_reply_def;
// #pragma section zsys_ddl_smsg_close
#pragma fieldalign shared2 __zsys_ddl_smsg_close
typedef struct __zsys_ddl_smsg_close
{
   short                           z_msgnumber;
   short                           z_tapedisposition;
} zsys_ddl_smsg_close_def;
// #pragma section zsys_ddl_smsg_break
#pragma fieldalign shared2 __zsys_ddl_smsg_break
typedef struct __zsys_ddl_smsg_break
{
   short                           z_msgnumber;
   short                           z_filenum;
   long                            z_tag;
} zsys_ddl_smsg_break_def;
// #pragma section zsys_ddl_smsg_devinfo
#pragma fieldalign shared2 __zsys_ddl_smsg_devinfo
typedef struct __zsys_ddl_smsg_devinfo
{
   short                           z_msgnumber;
   short                           z_reserved[3];
   short                           z_qualifier_len;
   char                            z_qualifier[18];
} zsys_ddl_smsg_devinfo_def;
// #pragma section zsys_ddl_smsg_devinfo_reply
#pragma fieldalign shared2 __zsys_ddl_smsg_devinfo_reply
typedef struct __zsys_ddl_smsg_devinfo_reply
{
   short                           z_msgnumber;
   short                           z_devtype;
   short                           z_devsubtype;
   short                           z_reserved[3];
   short                           z_reclen;
} zsys_ddl_smsg_devinfo_reply_def;
// #pragma section zsys_ddl_smsg_subname
#pragma fieldalign shared2 __zsys_ddl_smsg_subname
typedef struct __zsys_ddl_smsg_subname
{
   union
   {
      short                           z_msgnumber;
      char                            z_base[2];
   } u_z_msgnumber;
   short                           z_flags;
   short                           z_name_len;
   struct
   {
      short                           zoffset;
      short                           zlen;
   } z_pattern;
   short                           z_reserved[3];
   union
   {
      struct
      {
         signed char                     filler_0[100];
      } z_data;
      char                            z_name[100];
   } u_z_data;
} zsys_ddl_smsg_subname_def;
// #pragma section zsys_ddl_smsg_subname_reply
#pragma fieldalign shared2 __zsys_ddl_smsg_subname_reply
typedef struct __zsys_ddl_smsg_subname_reply
{
   short                           z_msgnumber;
   struct
   {
      short                           zdevtype;
      short                           zdevsubtype;
      short                           zreserved[3];
   } z_subtype30;
   short                           z_name_len;
   char                            z_name[18];
} zsys_ddl_smsg_subname_reply_def;
// #pragma section zsys_ddl_smsg_fileinfo
#pragma fieldalign shared2 __zsys_ddl_smsg_fileinfo
typedef struct __zsys_ddl_smsg_fileinfo
{
   short                           z_msgnumber;
   long                            z_tag;
   short                           z_error;
   zsys_ddl_typeinformation_def    z_info;
   short                           z_reclen;
   short                           z_flags;
} zsys_ddl_smsg_fileinfo_def;
// #pragma section zsys_ddl_smsg_filenamenext
#pragma fieldalign shared2 __zsys_ddl_smsg_filenamenext
typedef struct __zsys_ddl_smsg_filenamenext
{
   short                           z_msgnumber;
   short                           z_searchid;
   short                           z_error;
   zsys_ddl_typeinformation_def    z_info;
   short                           z_name_len;
   short                           z_reserved[5];
   char                            z_name[50];
} zsys_ddl_smsg_filenamenext_def;
// #pragma section zsys_ddl_smsg_nodedown
#pragma fieldalign shared2 __zsys_ddl_smsg_nodedown
typedef struct __zsys_ddl_smsg_nodedown
{
   short                           z_msgnumber;
   short                           z_reserved[3];
   long                            z_nodenumber;
   short                           z_nodename_len;
   char                            z_nodename[8];
} zsys_ddl_smsg_nodedown_def;
// #pragma section zsys_ddl_smsg_nodeup
#pragma fieldalign shared2 __zsys_ddl_smsg_nodeup
typedef struct __zsys_ddl_smsg_nodeup
{
   short                           z_msgnumber;
   short                           z_reserved[3];
   long                            z_nodenumber;
   short                           z_nodename_len;
   char                            z_nodename[8];
} zsys_ddl_smsg_nodeup_def;
// #pragma section zsys_ddl_smsg_gmomnotify
#pragma fieldalign shared2 __zsys_ddl_smsg_gmomnotify
typedef struct __zsys_ddl_smsg_gmomnotify
{
   short                           z_msgnumber;
   short                           z_jobid;
   zsys_ddl_phandle_def            z_phandle;
} zsys_ddl_smsg_gmomnotify_def;
// #pragma section zsys_ddl_smsg_remotecpuup
#pragma fieldalign shared2 __zsys_ddl_smsg_remotecpuup
typedef struct __zsys_ddl_smsg_remotecpuup
{
   short                           z_msgnumber;
   long                            z_nodenumber;
   short                           z_cpunumber;
   short                           z_nodename_len;
   short                           z_reserved[3];
   char                            z_nodename[8];
} zsys_ddl_smsg_remotecpuup_def;
// #pragma section zsys_ddl_smsg_procspawn
#pragma fieldalign shared2 __zsys_ddl_smsg_procspawn
typedef struct __zsys_ddl_smsg_procspawn
{
   short                           z_msgnumber;
   short                           z_reserved[13];
   long                            z_tag;
   long                            z_len;
   zsys_ddl_phandle_def            z_phandle;
   long                            z_pid;
   long                            z_errno;
   short                           z_tpcerror;
   short                           z_tpcdetail;
} zsys_ddl_smsg_procspawn_def;
// #pragma section zsys_ddl_smsg
#pragma fieldalign shared2 __zsys_ddl_smsg
typedef struct __zsys_ddl_smsg
{
   union
   {
      struct
      {
         char                            zbase[250];
      } z_msg;
      short                           z_msgnumber[125];
      zsys_ddl_smsg_cpudown_def       z_cpudown;
      zsys_ddl_smsg_cpuup_def         z_cpuup;
      zsys_ddl_smsg_settime_def       z_settime;
      zsys_ddl_smsg_poweron_def       z_poweron;
      zsys_ddl_smsg_msgmissed_def     z_msgmissed;
      zsys_ddl_smsg_3270_def          z_3270;
      zsys_ddl_smsg_timesignal_def    z_timesignal;
      zsys_ddl_smsg_lockmem_def       z_lockmemory;
      zsys_ddl_smsg_lockmemfail_def   z_lockmemoryfail;
      zsys_ddl_smsg_proctimesig_def   z_proctimesignal;
      zsys_ddl_smsg_control_def       z_control;
      zsys_ddl_smsg_setmode_def       z_setmode;
      zsys_ddl_smsg_setmode_reply_def z_setmode_reply;
      zsys_ddl_smsg_resetsync_def     z_resetsync;
      zsys_ddl_smsg_controlbuf_def    z_controlbuf;
      zsys_ddl_smsg_setparam_def      z_setparam;
      zsys_ddl_smsg_setparam_rply_def z_setparam_rply;
      zsys_ddl_smsg_qmsgcancelled_def z_qmsgcancelled;
      zsys_ddl_smsg_devinfocomp_def   z_devinfocomp;
      zsys_ddl_smsg_remotecpudown_def z_remotecpudown;
      zsys_ddl_smsg_procdeath_def     z_procdeath;
      zsys_ddl_smsg_proccreate_def    z_proccreate;
      zsys_ddl_smsg_open_def          z_open;
      zsys_ddl_smsg_open_reply_def    z_open_reply;
      zsys_ddl_smsg_close_def         z_close;
      zsys_ddl_smsg_break_def         z_break;
      zsys_ddl_smsg_devinfo_def       z_devinfo;
      zsys_ddl_smsg_subname_def       z_subname;
      zsys_ddl_smsg_subname_reply_def z_subname_reply;
      zsys_ddl_smsg_fileinfo_def      z_fileinfo;
      zsys_ddl_smsg_filenamenext_def  z_filenamenext;
      zsys_ddl_smsg_nodedown_def      z_nodedown;
      zsys_ddl_smsg_nodeup_def        z_nodeup;
      zsys_ddl_smsg_gmomnotify_def    z_gmomnotify;
      zsys_ddl_smsg_remotecpuup_def   z_remotecpuup;
      zsys_ddl_smsg_procspawn_def     z_procspawn;
   } u_z_msg;
} zsys_ddl_smsg_def;
// #pragma section zsys_ddl_logicaldeviceinfo
#pragma fieldalign shared2 __zsys_ddl_logicaldeviceinfo
typedef struct __zsys_ddl_logicaldeviceinfo
{
   long                            z_ldev;
   short                           z_primarycpu;
   short                           z_primarypin;
   short                           z_backupcpu;
   short                           z_backuppin;
   short                           z_type;
   short                           z_subtype;
   short                           z_recordsize;
   unsigned short                  z_audited:1;
   unsigned short                  z_dynamicallyconfigured:1;
   unsigned short                  z_demountable:1;
   unsigned short                  z_hassubnames:1;
} zsys_ddl_logicaldeviceinfo_def;
// #pragma section zsys_ddl_physicaldeviceinfo
#pragma fieldalign shared2 __zsys_ddl_physicaldeviceinfo
typedef struct __zsys_ddl_physicaldeviceinfo
{
   short                           z_status;
   short                           z_primary_subtype;
   short                           z_mirror_subtype;
   unsigned short                  z_has_physical_devices:1;
   unsigned short                  z_is_primary:1;
   struct
   {
      unsigned short                  z_configured:1;
      unsigned short                  z_inuse:1;
      short                           z_channel;
      short                           z_controller;
      short                           z_unit;
      short                           z_state;
   } z_pathinfo[4];
} zsys_ddl_physicaldeviceinfo_def;
// #pragma section dsclib_options
#define ZSYS_VAL_GETINFOOPT_GETNEXT 1
#define ZSYS_VAL_GETINFOOPT_BYTYPE 2
#define ZSYS_VAL_GETINFOOPT_BYSUBTYPE 4
// #pragma section dsclib_constants
#define ZSYS_VAL_DEVICEPATH_NONE 0
#define ZSYS_VAL_DEVICEPATH_PRIMARY 1
#define ZSYS_VAL_DEVICEPATH_BACKUP 2
#define ZSYS_VAL_DEVICEPATH_MIRROR 3
#define ZSYS_VAL_DEVICEPATH_MBACKUP 4
// #pragma section dsclib_devicestates
#define ZSYS_VAL_DEVSTATE_UP 0
#define ZSYS_VAL_DEVSTATE_DOWN 1
#define ZSYS_VAL_DEVSTATE_SPECIAL 2
#define ZSYS_VAL_DEVSTATE_MOUNT 3
#define ZSYS_VAL_DEVSTATE_REVIVAL 4
#define ZSYS_VAL_DEVSTATE_RESERVED 5
#define ZSYS_VAL_DEVSTATE_EXERCISE 6
#define ZSYS_VAL_DEVSTATE_EXCLUSIVE 7
#define ZSYS_VAL_DEVSTATE_HARDDOWN 8
#define ZSYS_VAL_DEVSTATE_FORMAT 9
#define ZSYS_VAL_DEVSTATE_CONFIGERR 10
#define ZSYS_VAL_DEVSTATE_LACKRESOURCE 11
#define ZSYS_VAL_DEVSTATE_PRETAKEOVER 12
#define ZSYS_VAL_DEVSTATE_UNKNOWN 13
#define ZSYS_VAL_DEVSTATE_INACCESSIBLE 99
// #pragma section cpu_itemcodes
#define ZSYS_VAL_CINF_PROCESSOR_TYPE 2
#define ZSYS_VAL_CINF_SOFTWARE_VERSION 3
#define ZSYS_VAL_CINF_PAGE_SIZE 4
#define ZSYS_VAL_CINF_MEMORY_SIZE 5
#define ZSYS_VAL_CINF_FIRST_VIRTUAL 6
#define ZSYS_VAL_CINF_SWAPPABLE_PAGES 7
#define ZSYS_VAL_CINF_FREE_PAGES 8
#define ZSYS_VAL_CINF_CURRENT_LOCKS 9
#define ZSYS_VAL_CINF_WAIT_LOCKS 10
#define ZSYS_VAL_CINF_HIGH_LOCKS 11
#define ZSYS_VAL_CINF_PAGE_FAULTS 12
#define ZSYS_VAL_CINF_SCANS_PER_CALL 13
#define ZSYS_VAL_CINF_CLOCK_CYCLES 14
#define ZSYS_VAL_CINF_MEM_PRESSURE 15
#define ZSYS_VAL_CINF_MEM_QUE_LENGTH 16
#define ZSYS_VAL_CINF_LOCAL_TIME_OFF 17
#define ZSYS_VAL_CINF_ELAPSED_TIME 18
#define ZSYS_VAL_CINF_BUSY_TIME 19
#define ZSYS_VAL_CINF_IDLE_TIME 20
#define ZSYS_VAL_CINF_INTERRUPT_TIME 21
#define ZSYS_VAL_CINF_CPU_QUE_LENGTH 22
#define ZSYS_VAL_CINF_DISPATCHES 23
#define ZSYS_VAL_CINF_PCBS_LOW_PINS 24
#define ZSYS_VAL_CINF_PCBS_HIGH_PINS 25
#define ZSYS_VAL_CINF_TIME_LST_ELMNTS 26
#define ZSYS_VAL_CINF_PROCESS_TLES 27
#define ZSYS_VAL_CINF_BREAKPOINTS 28
#define ZSYS_VAL_CINF_SEND_BUSY 29
#define ZSYS_VAL_CINF_T16_INTERRUPTS 35
#define ZSYS_VAL_CINF_DISK_CACHE_HITS 36
#define ZSYS_VAL_CINF_DISK_IOS 37
#define ZSYS_VAL_CINF_CPU_QSTATE 38
#define ZSYS_VAL_CINF_MEM_QSTATE 39
#define ZSYS_VAL_CINF_SEQ_SENDS 40
#define ZSYS_VAL_CINF_UNSEQ_SENDS 41
#define ZSYS_VAL_CINF_CME_EVENTS 42
#define ZSYS_VAL_CINF_PAGE_CREATES 43
#define ZSYS_VAL_CINF_OCI_BUSY 44
#define ZSYS_VAL_CINF_COMP_TRAPS 45
#define ZSYS_VAL_CINF_TRANSACTIONS 46
#define ZSYS_VAL_CINF_CPU_MODEL 47
#define ZSYS_VAL_CINF_CPU_NAME 48
#define ZSYS_VAL_CINF_LEGAL_NAME 49
#define ZSYS_VAL_CINF_AXCEL_BUSY 50
#define ZSYS_VAL_CINF_CLOCK_RESOL 51
#define ZSYS_VAL_CINF_CLOCK_MAX_ADJ 52
#define ZSYS_VAL_CINF_CLOCK_MAX_DRIFT 53
#define ZSYS_VAL_CINF_TUID 54
#define ZSYS_VAL_CINF_UP_COUNT 55
#define ZSYS_VAL_CINF_BASE_TIME 56
#define ZSYS_VAL_CINF_MEMMAN_FLAGS 57
#define ZSYS_VAL_CINF_SEGS_USED 58
#define ZSYS_VAL_CINF_MAX_SEG_USED 59
#define ZSYS_VAL_CINF_MINOR_VERSION 60
// #pragma section zsys_ddl_process_launch
#define ZSYS_VAL_PLAUNCH_PARMS_V_NOV95 1
#define ZSYS_VAL_PLAUNCH_PARMS_L_NOV95 96
#define ZSYS_VAL_PLAUNCH_PARMS_VER 1
#define ZSYS_VAL_PLAUNCH_PARMS_LEN 96
#pragma fieldalign shared2 __zsys_ddl_plaunch_parms
typedef struct __zsys_ddl_plaunch_parms
{
   short                           z_version;
   short                           z_length;
   zsys_ddl_char_extaddr_def       z_program_name;
   long                            z_program_name_len;
   zsys_ddl_char_extaddr_def       z_library_name;
   long                            z_library_name_len;
   zsys_ddl_char_extaddr_def       z_swapfile_name;
   long                            z_swapfile_name_len;
   zsys_ddl_char_extaddr_def       z_extswapfile_name;
   long                            z_extswapfile_name_len;
   zsys_ddl_char_extaddr_def       z_process_name;
   long                            z_process_name_len;
   zsys_ddl_char_extaddr_def       z_hometerm_name;
   long                            z_hometerm_name_len;
   zsys_ddl_char_extaddr_def       z_defines_name;
   long                            z_defines_name_len;
   long                            z_nowait_tag;
   long                            z_pfs_size;
   long                            z_mainstack_max;
   long                            z_heap_max;
   long                            z_space_guarantee;
   long                            z_create_options;
   short                           z_name_options;
   short                           z_debug_options;
   short                           z_priority;
   short                           z_cpu;
   short                           z_memory_pages;
   short                           z_jobid;
} zsys_ddl_plaunch_parms_def;
