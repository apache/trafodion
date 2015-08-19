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
// PREPROC: start of section: 
#if (defined(ddctctlz_h_) || (!defined(ddctctlz_h_including_section) && !defined(ddctctlz_h_including_self)))
#undef ddctctlz_h_

#include "rosetta/rosgen.h" /* rosetta utilities */
#endif
// PREPROC: end of section: 
//
// #pragma section NRDLAYOUT
//
// PREPROC: start of section: nrdlayout
#if (defined(ddctctlz_h_nrdlayout) || (!defined(ddctctlz_h_including_section) && !defined(ddctctlz_h_including_self)))
#undef ddctctlz_h_nrdlayout
//

//#pragma page "T9050 GUARDIAN  DCT Control Decs - NRD layout  "

// The Named Resource Descriptor (NRD) is the data structure which is
// an external representation of the data which is stored in the DCT.
// It is the only form of the DCT entry which is made externally
// visible to the clients of the DCT outside of Guardian. See below for
// a detailed description of the fields.

// The NRD completely replaces the Named Destination Descriptor (NDD)
// which has been the external form of the DCT since the B00 release.
//
// The NRD and the interfaces which use it are deliberately intended to
// be extensible.  Just like the formal parameter lists for Guardian
// procedures, the NRD is expected to grow longer with time.  Similarly,
// more parameters will probably get added onto the end of the list for
// DCT_UPDATE_.  The intent of the design of this interface is that the
// NRD will GROW longer with time, but the definitions of existing fields
// in the NRD will never be changed.  Variable length items in the DCT
// (like the name field) will always be passed as separate parameters to
// the relevant procedures and will not be added to the definition of the
// NRD.

// Future changes to the NRD will always be additions onto the end of the
// NRD described below (with the possible exception of additional bits
// being defined within the MISC word).
//
// A program written for an older version of Guardian which had shorter
// NRDs should keep on working on a newer version of Guardian in which
// the NRD definition has gotten longer, and similarly, a program written
// for a newer version of Guardian should work on older versions of
// Guardian (but not pre-D00 versions).

// To this end, the procedures which output a NRD expect the caller to
// specify the length of the NRD which the caller desires.  If the actual
// length of the NRD is longer than the caller has specified, the output
// will be truncated to the length specified by the caller.  If the
// caller does not need the entire NRD to be output, the caller may
// freely elect to specify only the required length.  If the actual
// length of the NRD is shorter than the caller has specified, the
// caller's buffer will be padded to the specified length with zeros.

// The placement of a version field at the front of the NRD permits the
// caller to discover that it has received a NRD from an earlier version
// of Guardian, and to take appropriate action with respect to missing
// fields.

//#pragma fieldalign shared2 NSK_DCT_NRDescRosetta
#pragma pack(push,2)
class NSK_DCT_NRDescRosetta {
public:                                               //      -----------------------------------------
  unsigned_16  tosVersion;                            //   0: |   version of NRD                      |
  _redefarrayafter(unsigned_16,misc,0,-1,tosVersion); //      -----------------------------------------
  unsigned     dmnt:1;                                //   1: | <0>  a demountable disk               |
  unsigned     audited:1;                             //      | <1>  an audited disk                  |
  unsigned_16  _filler:1;                             //      | <2>                                   |
  unsigned     dynamic:1;                             //      | <3>  dynamically configured           |
  unsigned     has_subnames:1;                        //      | <4>  has subdevs or subvols           |
  unsigned     iop_uses_ioprm:1;                      //      | <5>  I/O process uses the IOPRM       |
  unsigned     iop_uses_iolib:1;                      //      | <6>  I/O process uses the IOLIB       |
  unsigned                                            //      | <7>  ancestor is an IOP. IOP = device |
               ancestor_is_iop:1;                     //      |      type <> DTYPPROC                 |
  unsigned                                            //      | <8>  ancestor wants process death     |
               ancestor_any_instance:1;               //      |      message to go to any instance    |
                                                      //      |      of ancestor name                 |
  unsigned_16  _filler1:7;                            //      | < 9:15>                               |
                                                      //      -----------------------------------------
  unsigned_32                                         //   2: |   dct index                           |
               dct_index;                             //      |                                       |
  _redef2(int_32,dType_sType,dType);                  //      -----------------------------------------
  unsigned_16  dType;                                 //   4: |   device type                         |
                                                      //      -----------------------------------------
  unsigned_16  dSType;                                //   5: |   device subtype                      |
                                                      //      -----------------------------------------
  unsigned_16  recSize;                               //   6: |   device recsize (dType<>0)           |
                                                      //      -----------------------------------------
  unsigned_16  oReqId;                                //   7: |   device reqid   (dType<>0)           |
                                                      //      -----------------------------------------
  unsigned_32                                         // %10: |   process pair index                  |
               processPairIndex;                      //      |                                       |
  _redef2(unsigned_32,pid1                            //      -----------------------------------------
    ,cpu1);                                           //  12: |   primary process CPU                 |
  unsigned_16  cpu1;                                  //      -----------------------------------------
  unsigned_16  pin1;                                  //  13: |   primary process PIN                 |
  _redef(unsigned_32,pid2                             //      -----------------------------------------
    ,cpu2);                                           //  14: |   backup  process CPU                 |
  unsigned_16  cpu2;                                  //      -----------------------------------------
  unsigned_16  pin2;                                  //  15: |   backup  process PIN                 |
                                                      //      -----------------------------------------
  fixed_0      verifier;                              //  16: |   process pair instance               |
                                                      //      |       identifier                      |
                                                      //      -----------------------------------------
  NSK_PHandle  ancestor;                              // %22: |   ancestor's phandle                  |
                                                      //      |     (10 WORDS)                        |
                                                      //      -----------------------------------------
};
#pragma pack(pop)

// DETAILED DESCRIPTION OF NRD FIELDS :
//
// tosVersion
//    This field indicates the version of Guardian running on the
//    system on which the process resides.  Since the length of the NRD
//    is expected to grow with later versions of Guardian, this field
//    also implicitly identifies the length of the NRD.
//
// misc
//    A Word of Bit flags.  The bits are defined below.
//
//    dmnt
//       1 = A "demountable" disk.
//
//    audited
//       1 = a TMF audited disk.
//
//    dynamic
//       1 = logical device which was created by COUP.
//       0 = logical device which was created by SYSGEN.
//
//    has_subnames
//       For logical devices, "subnames" are subdevice names (e.g.
//       $OSP.#LOCAL) or subvolume names.  For named processes,
//       "subnames" are qualifier names, which are somewhat similar to
//       subdevice names (e.g. $S.#PRN.DEFAULT).  Through this bit, a
//       process (or logical device) informs the file system whether or
//       not it supports searches of its subnames (if any) by the file
//       system.  Each process (pair) controls the setting of this bit
//       in its own DCT entry (see PROCESS_SETINFO_).
//
//       1 = process or device implements subnames, and it wants
//           them searched by FILENAME_FINDNEXT.
//       0 = Process or device has no subnames, or does not want
//           them searched by FILENAME_FINDNEXT.
//
// dct_index
//    This field is the index into the DCT. For logical devices this is
//    also the LDEV number.
//
// dType
//    A non-zero value indicates the NRD entry is a Logical Device,
//    and dType is its device type.  A zero value indicates a named
//    Process.
//
// dSType
//    The sub-type of the logical device or named process.
//
// recSize
//    For logical devices only.  Specifies the "record size" of the
//    device.  Its actual meaning is dependent on the Device Type.
//
// oReqId
//    For logical devices only.  This is the Device Request ID field,
//    also known as the "UP ID".  This field is normally accessed and
//    manipulated by IO processes.  Its value is usually changed each
//    time the device is DOWNed.  This field is used in the generation
//    of error 60.
//
// processPairIndex
//    This field is provided for use in the phandle, and corresponds
//    one-to-one with a process (or pair of processes) identified in
//    the DCT.  A process (pair) with multiple device names (e.g.
//    TERMPROCESS) has multiple DCT_INDEXs but only one
//    processPairIndex.
//
// pid1
//    This definition is provided as a convenience to the programmer and
//    provides a way to pick up cpu1 and pin1 with one operation.  Note
//    that none of the procedural interfaces accept this.
//
// cpu1
//    The CPU number of the "primary" process of this process pair.
//
// pin1
//    The PIN of the "primary" process of this process pair.  For
//    logical devices, this will be -1 if cpu1 is down.
//
// pid2
//    This definition is provided as a convenience to the programmer and
//    provides a way to pick up cpu2 and pin2 with one operation.
//
// cpu2
//    The CPU number of the "backup" process of this process pair.  If
//    this entry belongs to a named process than -1 if no backup exists.
//
// pin2
//    The PIN of the "backup" process of this process pair.  This will be
//    -1 if cpu2 is down.
//
// verifier
//    A sequence number generated to distinguish one instance of a
//    process pair from another.  Used primarily for the <verifier>
//    field of the phandle.
//
// ancestor
//    The phandle of the ancestor of the named process or logical
//    device.  For devices created by SYSGEN, this is the "Null"
//    phandle.


//#pragma page ""

// Two literals are provided which define the length of the NRD in
// bytes.  NRD_LENGTH defines the length of the NRD in the current
// release.  In a future release, when the NRD is lengthened, the value
// of the literal NRD_LENGTH will be changed to the then-current
// length.  Programs which use the symbol NRD_LENGTH in expressions for
// the sizes of buffers, when recompiled against the new Guardian files
// of later releases, will automatically get the new buffer sizes.

// The literal NRD_LENGTH_D00, defines the length of the NRD in the D00
// release.  In a later release when the NRD is lengthened, the value
// of NRD_LENGTH_D00 will remain unchanged, and a new literal (e.g.
// NRD_LENGTH_E00) will be created to describe the new length.  In
// later releases, the unchanged literals for older releases will be
// useful for determining the length of older versions of the NRD.

enum {NRD_LENGTH = _len( NSK_DCT_NRDescRosetta )};
enum {NRD_LENGTH_D00 = _len( NSK_DCT_NRDescRosetta )};



//
#endif
// PREPROC: end of section: nrdlayout
//
// #pragma section DCT_NOWAIT
//
// PREPROC: start of section: dct_nowait
#if (defined(ddctctlz_h_dct_nowait) || (!defined(ddctctlz_h_including_section) && !defined(ddctctlz_h_including_self)))
#undef ddctctlz_h_dct_nowait
//

//#pragma page "T9050 GUARDIAN  Dct Control Decs - DCT Nowait Request Literals"

enum {DCT_REQUEST_CTRLBUF_LENGTH = 100}; // Same as GUARD_ZNUP_REQUEST_MAXSIZE;

enum {DCT_REPLY_CTRLBUF_LENGTH = 200};   // Same as GUARD_ZNUP_REPLY_MAXSIZE;

enum {DCT_REQUEST_DATABUF_LENGTH = 18};  // Same as $LEN( PPD^TEMPLATE ).

enum {DCT_REPLY_DATABUF_LENGTH = 18};    // Same as $LEN( PPD^TEMPLATE ).




//
#endif
// PREPROC: end of section: dct_nowait
//
// #pragma section PPDLAYOUT
//
// PREPROC: start of section: ppdlayout
#if (defined(ddctctlz_h_ppdlayout) || (!defined(ddctctlz_h_including_section) && !defined(ddctctlz_h_including_self)))
#undef ddctctlz_h_ppdlayout
//

//#pragma page "T9050 GUARDIAN  Dct Control Decs - PPD Layout"
// This struct was used and externalized in pre-D00 systems, since it was
// externalized it is still included in this file.

//#pragma fieldalign shared2 NSK_DCT_ppd
class NSK_DCT_ppd {
public:
  union {                         //       ---------------------------------
    unsigned_16    name[3];       //    0: *      process name             *
                                  //       ----                         ----
    unsigned_char  nameS;         //    1: *     - blank filled            *
  };                              //       ----                         ----
                                  //    2: *                               *
  _redef2(unsigned_32,pids,pid1); //       ---------------------------------
  unsigned_16      pid1;          //    3  *     primary cpu,pin           *
                                  //       ---------------------------------
  unsigned_16      pid2;          //    4: *     backup  cpu,pin           *
  union {                         //       ---------------------------------
    int_16         ancestor[4];   //    5: *          ancestor             *
                                  //       ----                         ----
    unsigned_char  ancestorS;     //    6: *          process              *
                                  //       ----                         ----
  };                              //    7: *            id                 *
                                  //       ----                         ----
};                                //  %10: *                               *
                                  //       ---------------------------------

//
#endif
// PREPROC: end of section: ppdlayout
//
// #pragma section DEVICETYPES
//
// PREPROC: start of section: devicetypes
#if (defined(ddctctlz_h_devicetypes) || (!defined(ddctctlz_h_including_section) && !defined(ddctctlz_h_including_self)))
#undef ddctctlz_h_devicetypes
//

//#pragma page "T9050 GUARDIAN  DCT Control Decs - Device Types"
//
// Device types:
//
enum {DTYPNLH        = 63, // Net line handler
      DTYPNCP        = 62, // NCP
      DTYPX25AM      = 61, // X25AM
      DTYPAMTR       = 60, // AM3270/TR3271
      DTYPAM6520     = 59, // AM6520
      DTYPSNAX       = 58, // SNAX
      DTYPGDS        = 57, // General Device Support
      DTYPEMLAN      = 56, // Multilan
      DTYPOSI        = 55, // OSI
      DTYPDDNAM      = 54, // DDNAM
      DTYPATP6100    = 53, // 6100 Async termprocess
      DTYPZSMS       = 52, // SMS master process
      DTYPCP6100     = 51, // 6100 Bisync point-point
      DTYPCSM6100    = 50, // CSM 6100
      DTYPSNAX5      = 49, // SNAX5
      DTYPTCPIP      = 48, // TCP/IP
      DTYPSNAXNT     = 47, // SNAX NT2.1
      DTYPTELNET     = 46, // TELNET Server Process
      DTYPSQLGWTY    = 38, // SQL Server Gateway
      DTYPISDN       = 37, // ISDN
      DTYPAPPLE      = 36, // Appletalk
                           // 32 - 35 reserved for customer use
      DTYPJUKE       = 30, // Optical Storage Facility
                           // disk changer
      DTYPMIOP       = 29, // Maintenance I/O Process
      DTYPZNUP       = 28, // Network utility process
      DTYPOLM        = 27, // Optical Link Monitor
      DTYPTHL        = 26, // Tandem Hyper Link
      DTYPSTRGPOOL   = 25, // SMS Storage Pool
      DTYPTMP        = 21, // TMF Transaction Monitor Process
                           // 20 - 23 reserved for TMF
                           // 18 - 19 reserved for comm
      DTYPSNAXNT21   = 17, // SNAX/NT21 SNALUXS
      DTYPNFS        = 16, // NFS
      DTYPCRYPTO     = 15, // 3501 data encyrption device
      DTYPSNALU      = 14, // Sna Logical Unit
      DTYPSNAXSM     = 13, // Snax service manager
      DTYPTIL        = 12, // Tandem to IBM Link
      DTYPBSCOMM     = 11, // bit sync envoy
      DTYPETERM3270  = 10, // dummy device type for AM/TR
      DTYPX25PROC    = 9,  // X25AM process to process
      DTYPCARD       = 8,  // card reader
      DTYPCOMM       = 7,  // communication line
      DTYPTERM       = 6,  // terminal
      DTYPLP         = 5,  // line printer
      DTYPTAPE       = 4,  // magnetic tape
      DTYPDISC       = 3,  // disc
      DTYPRECV       = 2,  // $receive
      DTYPOP         = 1,  // operator
      DTYPPROC       = 0
     };                    // process

enum {LDEVOPR   = 0, // LDEV entry location of operator process
      LDEVNCP   = 1, // LDEV entry location of Network Control
      LDEVTMP   = 2, // LDEV entry location of TMP
      LDEVOSP   = 3, // LDEV entry location of OSP process
      LDEVZNUP  = 4, // LDEV entry location of ZNUP process
      LDEVFREE  = 5
     };              // Unassigned LDEVs start here

// The following are double word literals for the well known LDEVs
enum {LDEV_OPR   = 0, // LDEV entry location of operator process
      LDEV_NCP   = 1, // LDEV entry location of Network Control
      LDEV_TMP   = 2, // LDEV entry location of TMP
      LDEV_OSP   = 3, // LDEV entry location of OSP process
      LDEV_ZNUP  = 4, // LDEV entry location of ZNUP process
      LDEV_FREE  = 5
     };               // Unassigned LDEVs start here

// The following defines can be used by callers of FILEINFO, therefore they
// are kept in this file.

#define DTYP_TYPE 4,9// Fields in device type word
#define DTYP_SUBTYPE 10,15
#define DTYP_REMOVABLE 0,0// Disc only
#define DTYP_TMFAUDIT 1,1 // Disc only
#define DTYP_DP2 2,2      // Disc only
#define DTYP_DYNAMIC 3,3  // Dynamically Configured Device

//
#endif
// PREPROC: end of section: devicetypes
//
// #pragma section PROCIDLAYOUT
//
// PREPROC: start of section: procidlayout
#if (defined(ddctctlz_h_procidlayout) || (!defined(ddctctlz_h_including_section) && !defined(ddctctlz_h_including_self)))
#undef ddctctlz_h_procidlayout
//

//#pragma page "T9050 GUARDIAN  Dct Control Decs - Process ID Layout"
// This struct was used and externalized in pre-D00 systems, since it was
// externalized it is still included in this file.
// A Process ID comes in two flavors.  For a named process, the process name
// is followed by the PID; for an unnamed process, the creation timestamp is
// followed by the PID.  It is this external definition that limited
// pre-D00 system process names to 6 characters.

//#pragma fieldalign shared2 NSK_DCTProcIdRosetta
struct NSK_DCTProcIdRosetta {
  union {                        //       ---------------------------------
    unsigned_16    name[3];      //    0: *        process name           *
                                 //       ----                         ----
    unsigned_char  nameS;        //    1: *       - blank filled          *
                                 //       ----                         ----
    unsigned_16    crt/*[0:3]*/; //    2: *     or creation timestamp     *
  };                             //       ---------------------------------
  unsigned_16      pid;          //    3  *          cpu,pin              *
                                 //       ---------------------------------
};

//
#endif
// PREPROC: end of section: procidlayout
//
// #pragma section PROCESSUBTYPES
//
// PREPROC: start of section: processubtypes
#if (defined(ddctctlz_h_processubtypes) || (!defined(ddctctlz_h_including_section) && !defined(ddctctlz_h_including_self)))
#undef ddctctlz_h_processubtypes
//

//#pragma page "T9050 GUARDIAN  DCT Control Decs - Process Sub Types"
//
// The following priv process subtypes are currently known by Guardian:
//
enum {FOXIPBMON_SUBTYPE          = 0,
      CMP_PROCESS_SUBTYPE        = 1,
      SAFEGUARD_PROCESS_SUBTYPE  = 2,
      LINKMON_PROCESS_SUBTYPE    = 3,
      TMP_PROCESS_SUBTYPE        = 4,
      TORUSMASTER_SUBTYPE        = 5,
      TORUSSLAVE_SUBTYPE         = 6
     };

//
#endif
// PREPROC: end of section: processubtypes
//
// #pragma section SMSPROCESSSUBTYPES
//
// PREPROC: start of section: smsprocesssubtypes
#if (defined(ddctctlz_h_smsprocesssubtypes) || (!defined(ddctctlz_h_including_section) && !defined(ddctctlz_h_including_self)))
#undef ddctctlz_h_smsprocesssubtypes
//

//#pragma page "T9050 GUARDIAN  DCT Control Decs - SMS Process Sub Types"
enum {SMS_MASTER_PROCESS_SUBTYPE  = 0,
      SMS_STRGPOOL_SUBTYPE        = 0,
      SMS_VIRTUAL_DISK_SUBTYPE    = 36
     };

//
// The following define is passed an NRD, an NRL or a DCTENTRY struct name and
// tests to see if the related process is an SMS process.  This happens to
// work as all structs use the same field names.
//
#define IS_SMS_PROCESS( r )                                                   \
   (((_low((r)->dType) == DTYPZSMS )/* assume all subtypes are sms processes */)\
    ||                                                                        \
    ((_low((r)->dType) == DTYPSTRGPOOL )/* assume all subtypes are sms processes */)\
    ||                                                                        \
    ((_low((r)->dType) == DTYPDISC ) && ((int_16)(r)->dSType == SMS_VIRTUAL_DISK_SUBTYPE )))


#endif
// PREPROC: end of section: smsprocesssubtypes
//
//
#if (!defined(ddctctlz_h_including_self))
#undef ddctctlz_h_including_section
#endif
// end of file
