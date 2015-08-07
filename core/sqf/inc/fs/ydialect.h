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
// PREPROC: start of file: file guard

#if (!defined(ydialect_included_already) || defined(ydialect_including_section) || defined(ydialect_including_self))
//
//
#if (!defined(ydialect_including_self) && !defined(ydialect_including_section))
#define ydialect_included_already
#endif
// 
//
// PREPROC: start of section: 
#if (defined(ydialect_) || (!defined(ydialect_including_section) && !defined(ydialect_including_self)))
#undef ydialect_
//

#include "rosetta/rosgen.h" /* rosetta utilities */

// #pragma page "Messages -- Dialect Description Format"
//
//  Defining a Dialect
//
//  This section outlines some common ways of handling dialect definition to
//  simplify construction and understanding a new dialect.
//
//
//  General Rules for Dialects
//
//  Dialect code numbers are kept in the T16 Bible.
//
//  The data structures describing a dialect are in a single file
//  which is not a section of some larger file (not a ?section).  It
//  should be constructed to "stand alone" and only
//  require this file be sourced to allow compilation.
//
//  A Dialect description File has a header such as the following example:
//
//  EXAMPLE
//    Requester:  File System
//    Server:     all IOPs
//    Name:       FS_IOPALL
//
//  The last item is the <dialect-name> referenced in the remainder of this
//  section.
//  A dialect description contains the following:
//
//  A data structure for the requests and replies.  This is best done as
//  one data structure with substructures for each request and each reply.
//  This limits the degree of name conflicts and gives short meaningful
//  names to the requests (e.g. msg.open).  The name of this structure
//  should be <dialect-name>_TEMPLATE.
//
//  A set of literals for the request and reply codes.  This should be
//  mutually exclusive (no reply code = any request code).  Requests
//  should have a name format <dialect-name>_<request substruct-name>.
//  Where there is only one reply to a given request the name format
//  should be <dialect-name>_<reply-substruct-name>.  Where a request may
//  return multiple replies variations of this name should be used putting
//  "_REPLY" on the end of all reply data structures.  An additional reply
//  for the simplest reply if required in the dialect (just an error code)
//  should be defined as <dialect-name>_SIMPLE_REPLY.
//
//  A set of literals for the version numbers with the following format:
//  <dialect-name>_VERSION_<date>.
//
//  A literal defining the maximum request size in the dialect with the name:
//  <dialect-name>_REQUEST_MAXSIZE.  If the dialect contains secure messages
//  it must add in the securityblock_size_xxx literal (defined in this file
//  as SecurityBlock_Size_D00).  For example
//  Literal Frog_Toad_Request_Maxsize = 128 + SecurityBlock_Size_D00;
//
//  A literal defining the maximum reply size in the dialect with the name:
//  <dialect-name>_REPLY_MAXSIZE.

// #pragma page "Messages -- Defining VarStrings"
//
//  The VARSTRING Data Structure
//
//  If the structure contains variable length information it should
//  use the VARSTRING structure to be an offset and length to it.
//  A varstring structure is defined as

#if !defined(__GNUC__)
#pragma fieldalign shared2 varstring
#endif
struct varstring {
  int_16  offset; // Byte offset to data.  It is the offset from the
                  //    outermost structure that the varstring is
                  //    contained in (usually the base of the message).
  int_16  length; // Byte length of data, 0 = not present.
}; // struct varstring

//  The offset is computed from the lowest level of statically
//  enclosing structure as in the following example:
//
//  Struct Msg (*); Begin
//    String Base[0:-1];     ! 0: for access (as in MESSAGE_HEADER define)
//    Int A;                 ! 0:
//    Struct SubRec; Begin
//       Int B;                   ! 2:
//       Struct Name (VARSTRING); ! 4: variable len name (descriptor)
//    end; !struct subrec
//    Int C;                 ! 8:
//    String data[0:-1];     ! 10: where variable data may be placed
//  End; ! struct msg
//
//  Then Msg.Subrec.Name.Offset = 10 or greater, and
//  @Msg.Base[ Msg.Subrec.Name.Offset ] is the address of the data.
//  [ Note: The variable data in a few instances might itself be a structure
//    including varstrings; its own varstring offsets would be relative to
//    its own base. ]
//
//
// The following define allows direct access to the data referenced by a
//  varstring with no address pointer required (though using
//  @VARSTRING_ACCESS(msg,vars) will generate the address).
//
//  For example:
//     locname ':=' VARSTRING_ACCESS( Msg, Subrec.Name ) FOR
//                  VARSTRING_LENGTH( Msg, Subrec.Name ) BYTES;

// MSG is a pointer
#define VARSTRING_ACCESS(msg,vars) (&(msg)->base())[ (msg)->vars.offset ]
#define VSACC( msg, vars ) VARSTRING_ACCESS(msg,vars)
#define VARSTRING_LENGTH(msg,vars) (msg)->vars.length
#define VSLEN( msg, vars ) VARSTRING_LENGTH(msg,vars)

// To initialize the varstring offset use the following:
//  E.G.  VARSTRING_SETOFFSET(msg, open.filename, ptr);
//  (both pointers must be string or .EXT as a byte offset is required)

#define VARSTRING_SETOFFSET( msg, vars, ptr)                                  \
   (msg)->vars.offset = (int_16)_xlow ((int_ptr)ptr - (int_ptr)msg)
#define VSSET( msg, vars, ptr) VARSTRING_SETOFFSET(msg, vars, ptr)

// #pragma page "Messages -- Message System Parameters as Part of a Message"
//
//  The message system provides the server with the following
//
//  Request control buffer size:
//
//  This is NEVER used to convey "version" information about the
//  request.  Servers should read the full control buffer even if it is larger
//  than the expected buffer to allow new versions to be read with the
//  variable data still at the end of the fixed data (which is larger
//  in a new version)
//
//
//  Request data buffer size:
//
//  This is AWAYS used to denote the data size.  In particular
//  the control part of a message that uses a data buffer should not
//  have a field for the size of the data in the buffer.  The control
//  part may contain sizes of subsets of the data if it represents
//  multiple objects.
//
//
//  Maximum reply control size:
//
//  This is NEVER used to convey version about the desired reply.
//  Each dialect has a literal denoting the recommended size for
//  any reply in the dialect.  The Server's reply must fit into the
//  requester's maximum control buffer (so version handling may be more
//  complex in that the control reply may have to actually be formated in
//  the old version).
//
//
//  Maximum reply data size:
//
//  This is AWAYS used to denote the size of the desired data (e.g.
//  the read count).
//
//
//  Some Standard Defines
//
//  The MESSAGE_HEADER define is used as the first fields in the dialect
//  data structure.  It uses the maximum of the request and reply maximum
//  size literals to set the size of the message data structure.
//
//  The INTERROGATE^DEF define is used to define the basic form of interrogate
//  version request and reply (i.e. only the fields defined
//  in \(msgfmts\)).  If a more complex request or reply is desired then
//  it should be defined just like any other request using the name
//  INTERROGATE and INTERROGATE_REPLY.
//
//  The request code must be manually included as
//
//        <dialect_name>_INTERROGATE = 0
//                     and
//        <dialect_name>_INTERROGATE_REPLY = 1.
//
//
//  Requests and Replies
//
//  All request substructures are equivalenced to REQUEST with bounds of
//  [0:-1] (this allows the possibility of the header items changing
//  before D00 with only recompilation required).
//
//    For example    OPEN [0:-1] = REQUEST.
//
//  All reply substructures are equivalenced to REPLY with bounds of
//  [0:-1].  For replies that map 1 to 1 to requests the naming convention
//  of <request-name>_REPLY should be used (e.g. OPEN_REPLY).
//  For the simplest reply (just an error return) the following should be
//  defined:
//
//    STRUCT SIMPLE_REPLY;
//    BEGIN  INT NODATA[0:-1]; END;
//
//  This allows some uniformity on how this special, simple response is
//  described.  Of course if this is not a valid reply to any request it
//  should not be included in the dialect.
//

// #pragma page "Messages -- Dialect Numbers"
//
// Format:  DIALECT_<requester>_<server>  = nn,-- %oo, <comment>
//                                             --      <product #>: <file name>
//
enum {DIALECT_ZERO                = 0,   // %0,  Unenlightened messages
                                         //      T9050: DOLDREQS,DMON0
                                         //      T9055: JFSFS0, JFSIOPA0,JFSDISK0
                                         //             DDIAL0, DSTATUS0
      DIALECT_MSG_ERROR           = 1,   // %1,  Message system: includes only error
                                         //      replies.
                                         //      T9050: DMSGHI(MSG_ERROR_DIALECT)
      DIALECT_FS_IOPALL           = 2,   // %2,  File sys to all iop
                                         //      T9055: DFSIOPAL
      DIALECT_FS_IOPNONDISK       = 3,   // %3,  File sys to non disk iop
                                         //      T9055: DFSIOPNO
      DIALECT_FS_IOPDISK          = 4,   // %4,  File sys to disk iop (dp2,odp)
                                         //      T9055: DFSIOPDI
      DIALECT_FS_FS               = 5,   // %5,  File sys to file sys
                                         //      T9055: DFSFS
      DIALECT_FS_FSCHECK          = 6,   // %6,  File sys to checkmonitor
                                         //      T9055: JFSFSCHK
      DIALECT_DSC_IOP             = 7,   // %7,  Pup/coup to all iops currently only
                                         //      non disks
                                         //      T9023: DDSCIOP
      DIALECT_SYSTEMSTATUS        = 8,   // %10, Guardian Status info
                                         //      T9055: DSTATUS, KSYSMSG
      DIALECT_IOPRM_IOPRM         = 9,   // %11, IopRm to IopRm (mainly for
                                         //      checkpoint)
                                         //      T9219: DIOPRM2
      DIALECT_ZSVR_TPPROC         = 10,  // %12, Tapestry to Tape process
                                         //      T6958: DZSVRTP
      DIALECT_TMF_IOPDISK         = 11,  // %13, Tmf to disk iop
                                         //      T9066: NTMFDPIF
      DIALECT_IOPNONDISK_FS       = 12,  // %14, Non-disk iop to fs (break)
                                         //      T9055: DIOPNOFS
      DIALECT_MSGSYS              = 13,  // %15, Message system (not special msg
                                         //      sys response with an error)
                                         //      T9050: DMSGHI (MSGSYS_LH_DIALECT)
      DIALECT_IOPRM_IOP           = 14,  // %16, IOPRM to IOP
                                         //      T9219: DIOPRMIO
      DIALECT_GUARD_IO            = 15,  // %17, Guardian IO
                                         //      T9219: DGUARDIO
      DIALECT_OPER                = 16,  // %20, FS/Guard to $0
                                         //      T9055: DOPERATR
      DIALECT_MEASURE             = 17,  // %21, Measure to Measure
                                         //      T9086: DMDIAL
      DIALECT_PRCMON              = 18,  // %22, Monitor process control
                                         //      T9050: DPRCMON
      DIALECT_BUSMON              = 19,  // %23, Monitor Bus control
                                         //      T9050: DBUSMON
      DIALECT_MISMON              = 20,  // %24, Misc. to Monitor
                                         //      T9050: DMISMON
      DIALECT_CPUMON              = 21,  // %25, Monitor Cpu control
                                         //      T9050: DCPUMON
      DIALECT_MEMMON              = 22,  // %26, Monitor Memory control
                                         //      T9050: DMEMMON
      DIALECT_DSCMON              = 23,  // %27, Monitor Dynamic Sys. Conf.
                                         //      T9050: DDSCMON
      DIALECT_EMSMON              = 24,  // %30, Monitor Event Mgmt. Sys.
                                         //      T9050: <<< unknown file >>>
      DIALECT_PCOLL_CDIST         = 25,  // %31, $0 to $Z0 for keeping $Z0 in sync
                                         //      with $0.
                                         //      T9631: D0Z0
      DIALECT_FS_MONITOR          = 26,  // %32, Fs to Monitor
                                         //      T9055: DFSMON
      DIALECT_MEMORYSEND          = 27,  // %33, Guardian memory function to copy
                                         //      sections of resident segments.
                                         //      Local only
                                         //      T9050: DMEMSEND
      DIALECT_EXPLH_NAM           = 28,  // %34, Expand line handler to nam I/f
                                         //      T9057: DLHNAM
      DIALECT_EXPMGR_EXPLH        = 29,  // %35, Expand manager to expand line
                                         //      handler
                                         //      T9117: DMGRLH
      DIALECT_EXPMGR_EXPNCP       = 30,  // %36, Expand manager to Ncp
                                         //      T9117: DMGRNCP
      DIALECT_ZNUP_EXPLH          = 31,  // %37, $Znup to Expand line handler
                                         //      T9057: DZNUPLH
      DIALECT_EXPLH_EXPLH         = 32,  // %40, Expand Line handler to lh
                                         //      T9057: DLHLH
      DIALECT_EXPLH_EXPNCP        = 33,  // %41, Expand line handler to NCP
                                         //      T9057: DLHNCP
      DIALECT_ZNUP_EXPNCP         = 34,  // %42, $Znup to Expand Ncp
                                         //      T9057: DZNUPNCP
      DIALECT_EXPNCP_EXPLH        = 35,  // %43, Expand Ncp to Expand Line handler
                                         //      T9057: DNCPLH
      DIALECT_NAM_EXPLH           = 36,  // %44, Nam (ipb/x25/lan) to expand line
                                         //      handler.
                                         //      T9057: DNAMLH
      DIALECT_GUARD_ZNUP          = 37,  // %45, Guardian kernel to $Znup
                                         //      T9050: JGRDZNUP
      DIALECT_EXPMGR_ZNUP         = 38,  // %46, Expand manager to $Znup ($Zexp)
                                         //      T9117: DMGRZNUP
      DIALECT_EXPNCP_EXPNCP       = 39,  // %47, Expand Net control primary to
                                         //      backup
                                         //      T9057: DNCPNCP
      DIALECT_GUARD_EXPNCP        = 40,  // %50, Guardian Libs to Ncp
                                         //      T9050: DGRDNCP
      DIALECT_TMDS_IOP            = 41,  // %51, Tmds to Iop
                                         //      T9459: DTMDSIOP
      DIALECT_PCOLL_AUX           = 42,  // %52, $0 to $Zoper (the $0 aux process)
                                         //      T9631: DZ0ZOPR
      DIALECT_CMP_IOP             = 43,  // %53, Cmp to Iop (comm)
                                         //      T9394: DCMPIOP
      DIALECT_FS_IOPENVOY         = 44,  // %54, File sys to Envoy Comm IOP
                                         //      T9055: DFSIOPEN
                                   // 45,-- %55, unused
      DIALECT_AM3270_TR3271       = 46,  // %56, Am 3270 to/from Tr 3271
                                         //      T9058: DAMTR
      DIALECT_COMMIOP_CSM         = 47,  // %57, Comm Iops to Communication
                                         //      Subsystem Manager (CSM)
                                         //      T9339: DIOPCSM
      DIALECT_CSM_CSM             = 48,  // %60, Commumication Subsystem manager
                                         //      primary to backup
                                         //      T9339: DCSMCSM
      DIALECT_FS_LINKMON          = 49,  // %61, File sys to Rabbit LinkMon
                                         //      T9055: DFSLINK
      DIALECT_MONIPB              = 50,  // %62, Monitor to IPB Monitor
                                         //      T9050: JMONIPB
      DIALECT_PX_TAST             = 51,  // %63, Utility to OSS socket agent
                                         //      T8397: STOPTAH
      DIALECT_AMPRIM_AMBKUP       = 52,  // %64, AM3270 and TR3271 primary to
                                         //      backup Iop
                                         //      T9058: DPRIMBK
      DIALECT_SNAX_X25            = 53,  // %65, Snax/Xf line handler to X25am
                                         //      device
                                         //      T9064: <<<unknown file>>>
      DIALECT_X25_SNAX            = 54,  // %66, X25am device to Snax/Xf line
                                         //      handler
                                         //      T9064: <<unknown file>>>
      DIALECT_DP2_DP2             = 55,  // %67, DP2 to DP2 dialect (local only)
                                         //      T9053: DDP2DP2
      DIALECT_FS2_IOPDISK         = 56,  // %70, FS2 (sql file system) to DP2
                                         //      T9196: DDP2SQLD
      DIALECT_IOPLIB_IOP          = 57,  // %71, IopLib (snax) to Iop (snax)
                                         //      T9064: <<<unknown file>>>
      DIALECT_TMP_LOCAL           = 58,  // %72, TMF3 processes to TMP
                                         //      T9066: <<<unknown file>>>
      DIALECT_ENVOY_ENVOY         = 59,  // %73, Envoy Primary to Backup
                                         //      T9051: DENVENV
      DIALECT_PXFS_NS             = 60,  // %74  Posix File System to Name Server
                                         //      T8620: FSNSEXTH
      DIALECT_PXFS_DP2            = 61,  // %75  Posix File System to DP2
                                         //      T9053: DPXFSDP2
      DIALECT_PXFS_MON            = 62,  // %76  Posix File System to Posix Monito
                                         //      T????: <<<unknown file>>>
      DIALECT_PXFM_PS             = 63,  // %77  Posix File Manager to Pipe Serve
                                         //      T????: <<<unknown file>>>
      DIALECT_PXNS_DP2            = 64,  // %100 Posix Name Server to DP2
                                         //      T9053: DPXNSDP2
      DIALECT_PXNS_PS             = 65,  // %101 Posix Name Server to Pipe Server
                                         //      T8620: NSPSEXTH
      DIALECT_PXPIPE_PS           = 66,  // %102 Posix Pipe Library to Pipe Server
                                         //      T8620: PIPSEXTH
      DIALECT_PXMON_NS            = 67,  // %103 Posix Monitor to Name Server
                                         //      T8620: PMNSEXTH
      DIALECT_PXMON_FM            = 68,  // %104 Posix Monitor to File Manager
                                         //      T????: <<<unknown file>>>
      DIALECT_DSC_ODP             = 69,  // %105 Dsc (Coup) to Odp (optical disk)
                                         //      T9123: DDSCODP
      DIALECT_PXMON_IOP           = 70,  // %106 Posix Monitor to IOPs
                                         //      T9053: DPXMRIOP
      DIALECT_DP2_PXFM            = 71,  // %107 Dp2 to Posix File Manager
                                         //      T9123: DDP2PXFM
      DIALECT_DP2UTIL_DP2         = 72,  // %110 Dp2 utility to Dp2
                                         //      T9053: DUTIDP2

      //DIALECT_TMFRESMON_LOCAL        = 73, -- %111 Local TMF components to TMF
      //      Resident Monitor
      //      T????:<<<unknown file>>>
      //DIALECT_TMFMON2_LOCAL          = 74, -- %112 Local TMF components to TMF
      //      TMFMON2
      //      T????:<<<unknown file>>>
      //DIALECT_TMF_NETWORK            = 75, -- %113 TMF TMP to remote TMP
                                         //      T????:<<<unknown file>>>
      DIALECT_TCPIP_TCPIP         = 76,  // %114 TCP primary to TCP backup
                                         //      T9551:DTCPTCP
      DIALECT_SNA_LANTR           = 77,  // %115 Token ring between SNAX/XF & TLAM
                                         //      T9375:DSNATR
      DIALECT_LANTR_SNA           = 78,  // %116 Token ring between TLAM & SNAX/XF
                                         //      T9375:DTRSNA
      DIALECT_DEBUG_ODP           = 79,  // %117 Debug to Optical disk
                                         //      T9123: DDBGODP
      DIALECT_ODP_ODP             = 80,  // %120 Odp to Odp backup
                                         //      T9123: DODPODP
      DIALECT_PRC_PRC             = 81,  // %121 Process Control to Process Control
                                         //      T9050: DPRCPRC
      DIALECT_RCV_IOPDISK         = 82,  // %122 TMF recovery (file & volume
                                         //      recovery) to IOP disk
                                         //      T9066: NRCVDPIF
      DIALECT_FS_COMM             = 83,  // %123 Fs to fscomm
                                         //      T9055: DFSCOMM
      DIALECT_ORSERV_DP2          = 84,  // %124 DP2 to FUP ORSERV
                                         //      T9053: DDP2ORE
      DIALECT_PXNS_NS             = 85,  // %125 Posix Name server to its backup
                                         //      T8621: NSNSTOPC
      DIALECT_SMSPP_DP2           = 86,  // %126 SMS pool process to DP2
                                         //   S7050: SHPPDPEN
      DIALECT_PM_SMSPP            = 87,  // %127 Placement Managers (currently SMS
                                         //      logical volume process and POSIX
                                         //      name server) to SMS pool process
                                         //   S7050: SHPMPPEH
      DIALECT_FS_SMS              = 88,  // %130 File system to SMS processes
                                         //   S7050: SHFSPPEH
      DIALECT_SMSMP_SMSPP         = 89,  // %131 SMS master process ($ZSMS) to
                                         //      SMS pool process.
                                         //   S7050: SHMPPPEH
      DIALECT_SMSPP_PM            = 90,  // %132  SMS Pool process to Placement
                                         //       Managers (currently SMS logical
                                         //       volume process and POSIX name
                                         //       server)
                                         //   S7050: SHPPPMEH
      DIALECT_SEC_SFG             = 91,  // %133
                                         //   T9750: DSECSFG
      DIALECT_TIF_CLIENTS         = 92,  // %134 TIF to TIF clients.
                                         //   T8698:TMFMON2, T8923:TIFSERVE
      DIALECT_XIO_XIO             = 93,  // %135
                                         //   XIO internal dialect
                                         //   T8374:DXIOXIO
      DIALECT_TP_TA               = 94,  // %136 OSS sockets: Transport Providers
                                         //   (such as TCP) to Transport Agent
                                         //   ($ZTAnn process)
                                         //   T8397:DTPTA (tal),tptaext.h (C)
      DIALECT_TA_TP               = 95,  // %137 OSS sockets: Transport Agent
                                         //   ($ZTAnn) to Transport Providers
                                         //   (TCP, et.al.)
                                         //   T8397:DTATP (tal),tatpext.h (C)
      DIALECT_PUP_SMS             = 96,  // %140 PUP to SMS process for SMS
                                         //   configuration <prod>:<file>
      DIALECT_ATP_ATP             = 97,  // %141 ATP6100 trace commands to
                                         //   backup. T9337:<file>
      DIALECT_CP_CP               = 98,  // %142 CP6100 trace commands to
                                         //   backup. T9338:<file>
      DIALECT_PXNS_LS             = 99,  // %143 Posix Name Server to AF_UNIX Local
                                         //   Server. T8621:NSSOCKC to T8994:ISNCS
      DIALECT_PXLS_NS             = 100, // %144 Posix AF_UNIX Local Server to
                                         //   Name Server. T8994:ISNCS to
                                         //   T8621:NSSOCKC
      DIALECT_PXMSGQ_MS           = 101, // %145 Posix Message Queue Library to
                                         //      Message Queue Server
                                         //      <prod>:<file>
      DIALECT_PXPS_PS             = 102, // %146 OSS Pipe Server to OSS Pipe
                                         //   Server.
                                         //   T8624 pspsexth
      DIALECT_PXMON_LS            = 103, // %147 OSS Posix Monitor to OSS Local
                                         //   Server.
                                         //   T8994 pmlsexth
      DIALECT_PXLS_LS             = 104, // %150 OSS Local Server to OSS Local
                                         //   Server.
                                         //   T8994 lsnstopc
      DIALECT_ACPXF_ACPXF         = 105, // %151 EnvoyACP/XF Trace command to
                                         //   backup.
                                         //   T9088:BSBFS
      DIALECT_CONF_ZCNG           = 106, // %152 Configuration services API to
                                         //   Config Utility Process ($ZCNF)
                                         //   T6586:<file>
      //DIALECT_TESTER_IOP           = 107, -- %153 Error Injector to IOP
      //   <product>:<file>
      //DIALECT_REQ_SMS              = 108, -- %154 (TMF) Requester Process to SMS
      //   <product>:<file>
      //DIALECT_TMFRCV_SMS           = 109, -- %155 TMF Recovery Process to SMS
                                         //   <product>:<file>
      DIALECT_IOP_CONMGR          = 110, // %156 Sierra WAN IOP to Connection
                                         //   Manager
                                         //   T8365:<file>
      DIALECT_ZCNF_CKPT           = 111, // %157 Config Utility Process ($ZCNF)
                                         //   Checkpoint to backup
                                         //   T6586:<file>
      DIALECT_REQ_SCSIIFM         = 112, // %160 Requester to SCSI IFM
                                         //   T1075:<file>
      DIALECT_NSKCOM_SRLMON       = 113, // %161 NSKCOM to SRLMON
                                         //   T5838:JNSKSRLH
      DIALECT_SRLMON_SRLCTL       = 114, // %162 SRLMON to SRLCTL
                                         //   T7898:JMONMSGH
      DIALECT_SRLCTL_SRLMON       = 115, // %163 SRLCTL to SRLMON
                                         //   T7898:JCTLMSGH
      DIALECT_QUERY_SRLCTL        = 116, // %164 Phoenix etc. to SRLCTL
                                         //   T7898:JCTLMSGH
      DIALECT_SRLCTL_MONITOR      = 117, // %165 SRLCTL to MONITOR
                                         //   T7898:JCTLMSGH

      DIALECT_TM_DP2_SQ           = 118,

      DIALECT_TM_AMP_SQ           = 121,

      DIALECT_AMP_AMP_SQ          = 122,

      DIALECT_LAST_VALID_DIALECT  = 122 // Range check

      // DIALECT_next_to_be_allocated   = 123
     };                                  // Range check

// #pragma page "Message Dialects - Common Structures"
//
// The OPENID is returned by a server (IOP) to requester (FS) on open reply
//  This must always be treated as a unit by the requester and
//  does not have to be constructed the same way for any two
//  IOPs.
// As viewed by the IOP and IOPRM it must contain some kind of open index and
//  some validity check (like the old request code).
// In particular if an IOP is handling multiple Ldevs (termprocess, odp)
//  it may want to place the opener's ldev into one word of the openid
//  as it is not a part of many messages done under an open.
//
// For ease of crossreferencing the uses of Openids use the define
//  instead of fixed.
//
#define OPENID_DEF fixed_0
//
// The FILEID is sent by requester (FS) to server (IOP) in open request
// This is used for duplicate open (wrongid) checking, each open for
//  a process will have a unique value in the struct (qualified by process).
//
#define FILEID_DEF fixed_0
//
// The TCBREF^DEF is used for transaction references in messages.  This is not
// the internal definition and is either a transid or a tcbref.
// Its validity in any message is denoted
// by an additional bit (usually Flags.Transid_valid).  Additionally, and
// all zero transid in the null transid.
//
#define TCBREF_DEF fixed_0

// #pragma page "Messages -- Standard Request and Reply Headers"

//
//   This is the first item in all dialect definitions, except dialect 0.
//   The size is determined by the dialect designer and should be the
//    value that would be allocated by a requester to issue an arbitrary message
//    in the dialect.
//
#define MESSAGE_HEADER                                                        \
   _redef(int_16,first_word,dialect_type);                    /*  (generic word for Tal substruct access)*/\
   int_16  dialect_type;                                      /* defined above, for both request and reply*/\
   _redef(unsigned_char,base,dialect_type);                   /* used for varstring offset accesses*/\
   /* request format*/                                                        \
   int_16  request_type; /* defined by each dialect*/                         \
   _redef2(int_32,dialect_request_type,dialect_type /* A short way to check both the dialect*/\
     );                                                       /*  and the request i.e.*/\
                                                              /*  msg.dialect_request_type = $Dbll(di,req)*/\
   int_16  request_version;                                   /* highest version that requester understand*/\
   int_16  minimum_interpretation_version;                           /* lowest version req. will accept*/\
   typedef class _ydialect_h_as_class_1 /* all requests equivalenced to this*/\
   {                                                                          \
   public:                                                                    \
     /**** If you add any data to this struct, change this*/                  \
     /**** literal to reflect the new size of the struct.*/                   \
     enum {_size = 0};                                                        \
     _redefarray(int_16,nodata,0,-1,this[0]); /* use xxx_REQUEST_MAXSIZE literal for len*/\
   } __request;                                                               \
   _redefarrayafter                                                           \
     (__request,request,0,-1,minimum_interpretation_version); /* substruct request*/\
                                                         /* reply format*/    \
   _redef(int_16,reply_type,request_type);                    /* reply code*/ \
   _redef(int_16,reply_version,request_version);              /* version of the reply*/\
   _redef(int_16,error,minimum_interpretation_version);                       \
   typedef class _ydialect_h_as_class_2{                                      \
   public:                                                                    \
     /**** If you add any data to this struct, change this*/                  \
     /**** literal to reflect the new size of the struct.*/                   \
     enum {_size = 0};                                                        \
     _redefarray(int_16,nodata,0,-1,this[0]); /* use xxx_REPLY_MAXSIZE literal for length*/\
   } /* substruct reply*/                                                     \
   __reply;                                                                   \
   _redefarrayafter(__reply,reply,0,-1,minimum_interpretation_version)

#if !defined(__GNUC__)
#pragma fieldalign shared2 message_header_template
#endif
class message_header_template {
public:
  MESSAGE_HEADER;
};

// #pragma page "Messages - Dialects and Common data Structures"
//
// INTERROGATE^DEF:  This define generates the basic interrogate version
//   request and reply.  For more complex ones define them as other requests
//
#define INTERROGATE_DEF                                                       \
   typedef class _ydialect_h_as_class_0{                                                            \
   public:                                                                    \
     /**** If you add any data to this struct, change this*/                  \
     /**** literal to reflect the new size of the struct.*/                   \
     enum {_size = 0};                                                        \
     _redefarray(int_16,nodata,0,-1,this[0]); /* placeholder, header is all*/ \
   } __interrogate;                                                           \
   _redefarray(__interrogate,interrogate,0,-1,this[1]); /* substruct interrogate*/\
                                                   /**/                       \
   typedef struct _ydialect_h_as_0 {                                                           \
     int_16  responder_version;                         /* Server's (highest) version*/\
                                                        /*  of this dialect*/ \
     int_16  responder_tosversion;                      /* Server's OS version*/\
   } /* substruct interrogate_reply*/                                         \
   __interrogate_reply;                                                       \
   _redefarray(__interrogate_reply,interrogate_reply,0,-1,this[1])

// #pragma page "Messages -- Secure Messages"
//
// The following defines the length of a securityblock which are part
//
enum {SECURITYBLOCK_SIZE_D00 = 98}; // At D00, size in bytes
// #pragma page "Messages -- Dialect Description Format"

//
#endif
// PREPROC: end of section: 
//
//
#if (!defined(ydialect_including_self))
#undef ydialect_including_section
#endif

#endif // file guard
// end of file
