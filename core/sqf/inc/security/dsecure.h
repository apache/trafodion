#ifndef _DSECURE_H_
#define _DSECURE_H_

#ifndef SECLIBAPI
#define SECLIBAPI __declspec(dllimport)
#endif

#include "rosetta/rosgen.h" /* rosetta utilities */

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
//

#include "security/uid.h"
SECLIBAPI extern const NTSEC_USER SECURITY_INVALID_UID;
SECLIBAPI extern const NTSEC_USER SECURITY_SQL_PUBLIC_UID;
enum {SECURITY_INVALID_GID = 0x80000000}; // Invalid/undefined GID
// NT: Other SECURITY_**_UID values not defined.

// NT: SECURITY_MAX_* literals not defined.

// NT: PROCESSID_SET_() literals not defined.

// NSK: Scalar range tests to validate a UID/GID.
// NT: Ask the NTSEC_USER instance if it's valid.
// This filters out all SECURITY_**_UID values.
#define UID_RANGE_ERROR(uid) (!(uid).valid())
#define GID_RANGE_ERROR(gid) ((gid < 0) || (gid > 65535))

// UID_TO_G90ID is obsolete (on NSK).  Call UID_TO_G90ID_() instead.
#define UID_TO_G90ID(uid) UID_TO_G90ID_(uid)

// NSK: convert 16-bit user ID to 32-bit UID_T.
// NT: no-op.
#define G90ID_TO_UID(g90id) g90id

// NSK: derive a group ID from a Guardian ID or UID_T.
// NT: doesn't handle groups this way; return an invalid value.
#define G90ID_TO_GID(g90id) SECURITY_INVALID_GID
#define G90_UID2GID(uid) SECURITY_INVALID_GID

//SQ_LINUX #endif /* section id_literals */
//SQ_LINUX #if (defined(dsecure_h_security_info_template) || (!defined(dsecure_h_including_section) && !defined(dsecure_h_including_self)))
//SQ_LINUX #undef dsecure_h_security_info_template
// section_subs.pl - Substituting section security_info_template

// NSK: The Process Security Block (PSB) is defined here.
// NT: The PSB is defined elsewhere.

// security_options_template defines flags that callers know about.
// Some are set by passing flags to Security_MSB_Init_/Specify_.
// Also, this struct is returned by Security_MSB_Get_ (MSB_OPTIONS)
// so callers (servers) can query the flags.
// NT: This definition is not binary-compatible with the NSK version,
// but it is (mostly) source-compatible.

union security_options_template
{
	unsigned_16	initialize0;
	struct x_anonymous
	{
		unsigned_16	_filler:10;
		unsigned_16	licensed_param:1;
		unsigned_16	licensed:1;		// User Prog is (SQL-)licensed
		unsigned_16	sql_executor_param:1;
		unsigned_16	sql_executor:1;		// User running SQL_Executor
		unsigned_16	deny_grants:1;		// Deny Grants mode (NT:unused)
		unsigned_16	checkonly:1;		// "Special open" of remote
							//  process (NT:unused)
	}; // anonymous struct
}; // union security_options_template

// NSK: SMON message control buffer defined here.
// NT: SMON control buffer not relevant.

// security_info_template defines the Message Security Block (MSB)
// present in all non-Dialect-0 "secure" message (SECUREB set).
// It is pointed to by a varstring at words [4:5] of the control buffer.
// NT: The actual structure is defined elsewhere.  Some non-security
// components do use the structure name, however, so we declare it
// without defining it; this permits pointer declarations but not
// references to fields.

struct security_info_template;

// NT: MSB size literals are not based on sizeof(security_info_template),
// but are verified as large enough during startup.

// "Official" length of the largest supported Message Security Block (MSB).
// This size may change over time.
#define MSB_MAX_BYTE_LENGTH			200

// Old, deprecated name for this length, but still used in some places.
#define LENGTH_SECURITY_INFO_TEMPLATE	MSB_MAX_BYTE_LENGTH

// Next two length symbols needed by dsecure.h(msb_init).
// YOU should use MSB_TYPE1_MAX_BYTES and MSB_TYPE2_MAX_BYTES instead.

#define MSB_MAX_BYTE_LENGTH_D00		MSB_MAX_BYTE_LENGTH
#define MSB_MAX_BYTE_LENGTH_JUN93	MSB_MAX_BYTE_LENGTH

// NT: obsolete Security_MSB_Append_ literals not declared.

//SQ_LINUX #endif /* section security_info_template */
//SQ_LINUX #if (defined(dsecure_h_msb_selectors) || (!defined(dsecure_h_including_section) && !defined(dsecure_h_including_self)))
//SQ_LINUX #undef dsecure_h_msb_selectors
//
// Item selector values for Security_MSB_Get[list]_
// Selector values 0..10 are reserved for attributes common to all MSBs
//
enum {MSB_PAID                     = 0,
      MSB_CAID                     = 1,
      MSB_SFG_RESULT               = 2,
      MSB_HOMETERM                 = 3,
      MSB_ORIGIN_SYSNUM            = 4,
      MSB_OPTIONS                  = 5,
      MSB_AUTH_TYPE                = 6,
      MSB_SFG_AUDIT                = 7,
      MSB_VERSION                  = 8,

      MSB_REAL_UID                 = 11,
      MSB_REAL_GID                 = 12,
      MSB_AUTH_NODE                = 13,
      MSB_AUTH_USER_NAME           = 15,
      MSB_GID_LIST                 = 16,
      MSB_GID_COUNT                = 17,
      MSB_EFFECTIVE_UID            = 18,
      MSB_EFFECTIVE_GID            = 19,
      MSB_SAVED_UID                = 20,
      MSB_SAVED_GID                = 21,
      MSB_AUTH_IS_ALIAS            = 22, // For EXPAND only
      MSB_EFFECTIVE_IS_ALIAS       = 23, // For EXPAND only
      MSB_PHANDLE                  = 24,
      MSB_AUDIT_FLAGS_INITIALIZED  = 25,
      MSB_AUDIT_AUFAIL             = 26,
      MSB_AUDIT_AUPASS             = 27,
      MSB_AUTH_IS_EFFECTIVE        = 28,
      // leave redundant literal until we have time to change Safeguard:
      MSB_USER_NAME_IS_EFFECTIVE   = MSB_AUTH_IS_EFFECTIVE, // obsolete
      MSB_LOGON_UID                = 29,
      MSB_IS_NFS_TYPE              = 30,
      MSB_SUBSYS_ID                = 31
     };

//SQ_LINUX #endif /* section msb_selectors */
//SQ_LINUX #if (defined(dsecure_h_psb_selectors) || (!defined(dsecure_h_including_section) && !defined(dsecure_h_including_self)))
//SQ_LINUX #undef dsecure_h_psb_selectors
//
// Item selector values for Security_PSB_Get_
//
enum {PSB_REAL_UID                 = 0,
      PSB_REAL_GID                 = 1,
      PSB_AUTH_NODE                = 2,
      PSB_EFFECTIVE_UID            = 3,
      PSB_EFFECTIVE_GID            = 4,
      PSB_SAVED_UID                = 5,
      PSB_SAVED_GID                = 6,
      PSB_LOGON_UID                = 7,
      // gap left for adding INT(32) items
      PSB_GID_COUNT                = 15,
      PSB_AUTH_TYPE                = 16,
      PSB_AUDIT_FLAGS_INITIALIZED  = 17,
      PSB_AUDIT_AUFAIL             = 18,
      PSB_AUDIT_AUPASS             = 19,
      // no gap left for adding INT items
      PSB_AUTH_USER_NAME           = 21,
      PSB_GID_LIST                 = 22,
      PSB_IS_EFFECTIVE             = 23,
      PSB_IS_ALIAS                 = 24,
      PSB_EFF_ALIAS                = 25,
      PSB_SUBSYS_ID                = 26
     };

// Security_PSB_Set_ callers need to be under MUTEX to modify items
// non-atomically.  Security_PSB_Get_ callers need to be under MUTEX
// to fetch multiple items from another process (but not their own process).
// Use PSB_MUTEXWORDS as the data-item parameter to MUTEX_ON.
enum {PSB_MUTEXWORDS = 64};

//SQ_LINUX #endif /* section psb_selectors */
//SQ_LINUX #if (defined(dsecure_h_msb_init) || (!defined(dsecure_h_including_section) && !defined(dsecure_h_including_self)))
//SQ_LINUX #undef dsecure_h_msb_init
//
// The SECURITY_INFO_TEMPLATE section of this file must be ?SOURCE'd in
// prior to this (MSB_INIT) section.
//
// Init_flag values used with Security_MSB_Init_:
enum {MSB_INIT_DEFAULT  = 0,
      MSB_CHECK_ONLY    = 1, // Check message security, but don't
                             // deliver message to target process
      MSB_ACCESS        = 2, // POSIX.1 access() support
      MSB_SQLSUBSYS     = 4
     };                      // Caller invokes SQL Subsystem privilege
//
// MSB_selector values used with Security_MSB_Init_:
enum {MSB_TYPE1  = 1, // D00 MSB
      MSB_TYPE2  = 2
     };               // D30 MSB
//
// Max byte lengths for MSB types.  (For use in dialect declarations.)
//
enum {MSB_TYPE1_MAX_BYTES = MSB_MAX_BYTE_LENGTH_D00};
enum {MSB_TYPE2_MAX_BYTES = MSB_MAX_BYTE_LENGTH_JUN93};

//SQ_LINUX #endif /* section msb_init */
//SQ_LINUX #if (defined(dsecure_h_sfg_responses) || (!defined(dsecure_h_including_section) && !defined(dsecure_h_including_self)))
//SQ_LINUX #undef dsecure_h_sfg_responses
//
// Safeguard responses
//
// Possible values for SAFEGUARDRESULT and SAFEGUARD_PARTIAL in the      --TSQL
// Security_Block.  OBISAYSNO is only valid for SAFEGUARD_PARTIAL.       --TSQL
//---------------------------------------------------------------------- --TSQL
enum {OBISAYSYES   = 1, //TSQL
      OBIDEFAULTS  = 2, //TSQL
      OBISAYSNO    = 3
     };                 //TSQL

//SQ_LINUX #endif /* section sfg_responses */
//SQ_LINUX #if (defined(dsecure_h_posix_access) || (!defined(dsecure_h_including_section) && !defined(dsecure_h_including_self)))
//SQ_LINUX #undef dsecure_h_posix_access
//
// POSIX file modes access permissions
//
enum {SFG_PX_R     = 4,
      SFG_PX_W     = 2,
      SFG_PX_X     = 1,

      SFG_PX_RWX   = 7,
      SFG_PX_RW    = 6,
      SFG_PX_NONE  = 0
     };

//SQ_LINUX #endif /* section posix_access */
//SQ_LINUX #if (defined(dsecure_h_g90_access) || (!defined(dsecure_h_including_section) && !defined(dsecure_h_including_self)))
//SQ_LINUX #undef dsecure_h_g90_access

//#pragma page "T6533 Standard Security [DSECURE] -- Access Control declarations"
//-----------------------------------------------------------------------------
//
// Access Control Declarations
//
//-----------------------------------------------------------------------------
//
// Access granted by evaluating an Enscribe file security vector is
// represented using the following structure:
//
class access_template {
public:
  inline int_16 *all_bits(void) {return (int_16 *) this;};
  unsigned_16  execute:1;
  unsigned_16  write:1;
  unsigned_16  read:1;
  unsigned_16  purge:1;
  unsigned_16  _filler:12;
};
//
// Guardian 90 Access Granted Literals
// (values match ACCESS_TEMPLATE)
//
enum {G90_VECTOR_P     = 8,
      G90_VECTOR_R     = 4,
      G90_VECTOR_W     = 2,
      G90_VECTOR_E     = 1,
      G90_VECTOR_RW    = 6,
      G90_VECTOR_RWEP  = 15,
      G90_VECTOR_NONE  = 0
     };
//
// G90 Access Request Literals (Historical OBIPROTECTVIOLATION values)
//
enum {G90_TMFROLLFWD_ACCESS  = 7,
      G90_TMFBACKOUT_ACCESS  = 6,
      G90_TMFOLIDUMP_ACCESS  = 5,
      G90_PURGE_ACCESS       = 4,
      G90_EXECUTE_ACCESS     = 3,
      G90_WRONLY_ACCESS      = 2,
      G90_RDONLY_ACCESS      = 1,
      G90_RDWR_ACCESS        = 0
     };

//SQ_LINUX #endif /* section g90_access */
//SQ_LINUX #if (defined(dsecure_h_auth_types) || (!defined(dsecure_h_including_section) && !defined(dsecure_h_including_self)))
//SQ_LINUX #undef dsecure_h_auth_types

//#pragma page "T6533 Standard Security [DSECURE]"
// Authentication types
enum {SECURITY_UNAUTHENTICATED  = 0, // subject is unauthenticated
      // value of 1 is unused
      SECURITY_LOCALLY_AUTH     = 2, // subject is locally authenticated
      SECURITY_REMOTELY_AUTH    = 3
     };                              // subject is remotely authenticated

// Convert dialect-0 message SRE bits to an auth-info value,
// Usage:  AUTH_INFO := SRE2AUTHINFO(SRE);
#define SRE2AUTHINFO(sre)                                                     \
   !((sre)->sre_secureB) ? SECURITY_UNAUTHENTICATED :                         \
((sre)->sre_remIdB) ? SECURITY_REMOTELY_AUTH : SECURITY_LOCALLY_AUTH

//SQ_LINUX #endif /* section auth_types */
//SQ_LINUX #if (defined(dsecure_h_object) || (!defined(dsecure_h_including_section) && !defined(dsecure_h_including_self)))
//SQ_LINUX #undef dsecure_h_object

//#pragma page "T6533 Standard Security [DSECURE]"
//-----------------------------------------------------------------------------
// Object Descriptor Declarations
//-----------------------------------------------------------------------------

// Object descriptor flags for SECURITY_OBJDESC_INIT_
// Must match OBJECT_FLAGS_TEMPLATE in JSECURE

enum {OBJDESC_FLAG_ANSI_RFORK   =16, // 1=Resource fork
      OBJDESC_FLAG_ANSI_SQL     = 8, // 1=SQL/ARK object, 0=SQL/MP
      OBJDESC_FLAG_TEMPORARY    = 4, // object is temporary
      OBJDESC_FLAG_SFG_SECURED  = 2, // object protected by Safeguard
      OBJDESC_FLAG_SQL_CAT      = 1
     };                              // SQL object in a catalog

// Object types for SECURITY_OBJDESC_INIT_

enum {OBJDESC_TYPE_ENSCRIBE  = 1,
      OBJDESC_TYPE_SQL       = 2, // SQL/ARK or SQL/MP
      OBJDESC_TYPE_POSIX     = 3,

      OBJDESC_TYPE_MIN       = OBJDESC_TYPE_ENSCRIBE,
      OBJDESC_TYPE_MAX       = OBJDESC_TYPE_POSIX
     };
//SQ_LINUX #endif /* section object */
//SQ_LINUX #if (defined(dsecure_h_setmode) || (!defined(dsecure_h_including_section) && !defined(dsecure_h_including_self)))
//SQ_LINUX #undef dsecure_h_setmode
//
// Template for describing SETMODE(SM^SECURITY) operations.  Passed to
// Security_G90Vector_Advise_.  The operation flags indicate which disk
// file attributes are being set.  Unused bits must be initialized to zero.
//
class security_setmode_template {
public:
  inline int_16 *all_flags(void) {return (int_16 *) this;};
  unsigned_16  safeguard_protect:1; // Safeguard-protected (OBIPROTECT) flag
  unsigned_16  clear_on_purge:1;    // clear-on-purge flag
  unsigned_16  progid:1;            // PROGID (ASSUMEID) flag
  // save bit <13> for a future set-group-ID operation
  unsigned_16  _filler:13;          // must be zeroed
};

//SQ_LINUX #endif /* section setmode */
#endif
