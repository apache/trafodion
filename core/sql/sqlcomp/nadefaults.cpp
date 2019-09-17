/* -*-C++-*-
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
 *****************************************************************************
 *
 * File:         NADefaults.cpp
 * Description:  Implementation for the defaults table class, NADefaults.
 *
 * Created:      7/11/96
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

#define   SQLPARSERGLOBALS_FLAGS	// must precede all #include's
#define   SQLPARSERGLOBALS_NADEFAULTS

#include "Platform.h"
#include "NADefaults.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef NA_HAS_SEARCH_H
  #include <search.h>	// use the bsearch binary search routine of the C RTL
#else
  #include <unistd.h>	// on OSS, bsearch comes from unistd.h
#endif

  #include "nsk/nskport.h"

#if   !defined(NDEBUG)
#endif


#include "CliDefs.h"
#include "CmpContext.h"
#include "CmpErrors.h"
#include "ComObjectName.h"
#include "ComRtUtils.h"
#include "ComSchemaName.h"
#include "ex_error.h"
#include "DefaultConstants.h"
#include "DefaultValidator.h"
#include "NAClusterInfo.h"
#include "parser.h"
#include "sql_id.h"
#include "SQLCLIdev.h"
#include "Sqlcomp.h"
#include "StmtCompilationMode.h"
#include "OptimizerSimulator.h"
#include "CmpSeabaseDDL.h"
#include "Globals.h"
#include "QCache.h"

#include "SqlParserGlobals.h"		// MUST be last #include!

#include "seabed/ms.h"
#include "seabed/fs.h"


#define   NADHEAP		 CTXTHEAP
#define   ERRWARN(msg)		 ToErrorOrWarning(msg, errOrWarn)
#define   ERRWARNLOOP(msg)	 ToErrorOrWarning(msg, errOrWarnLOOP)

#define   ENUM_RANGE_CHECK(e)	 (e >= 0 && (size_t)e < numDefaultAttributes())
#define   ATTR_RANGE_CHECK	 ENUM_RANGE_CHECK(attrEnum)
#ifndef NDEBUG
  #define ATTR_RANGE_ASSERT	 CMPASSERT(ATTR_RANGE_CHECK)
#else
  #define ATTR_RANGE_ASSERT
#endif

// -------------------------------------------------------------------------
// This table contains defaults used in SQLARK.
// To add a default, put it in sqlcomp/DefaultConstants.h and in this table.
//
// The #define declares the domain (allowed range of values) of the attr-value;
// typically it is Int1 or UI1 (signed or unsigned integral, >=1)
// to prevent division-by-zero errors in the calling code.
//
// The first column is the internal enum value from sqlcomp/DefaultConstants.h.
// The second column is the default value as a string.
//
// The DDxxxx macro identifies the domain of the attribute
// (the range and properties of the possible values).
//
// XDDxxxx does the same *and* externalizes the attribute
// (makes it visible to SHOWCONTROL; *you* need to tell Pubs to document it).
//
// SDDxxxx does the same and externalizes the attribute to HP support personnel
// (makes it visible to HPDM when support is logged on; *you* need to tell Pubs
//  to document it in the support manual. You can set the
//  SHOWCONTROL_SUPPORT_ATTRS CQD to ON to see all the externalized and
//  support-level CQDs).
//
//  For instance, DDflt0 allows any nonnegative floating-point number, while
//  DDflte allows any positive float (the e stands for epsilon, that tiniest
//  scintilla >0 in classical calculus, and something like +1E-38 on a Pentium).
//  DDui allows only nonnegative integral values (ui=unsigned int),
//  DDui1 allows only ints > 0, DDui2 only nonzero multiples of 2, etc.
//
//  DDkwd validates keywords.  Each attribute that is DDkwd has its own subset
//  of acceptable tokens -- the default behavior is that the attr is bivalent
//  (ON/OFF or TRUE/FALSE or ENABLE/DISABLE).  If you want different keywords,
//  see enum DefaultToken in DefaultConstants.h, and NADefaults::token() below.
//
//  Other DD's validate percentages, and Ansi names.  Certainly more could be
//  defined, for more restrictive ranges or other criteria.

// *************************************************************************
// NOTE: You must keep the entire list in alphabetical order,
// or else the lookup will not work!!!!!!! Use only CAPITAL LETTERS!!!!!!!!!
// *************************************************************************
// NOTE 2: If you choose to "hide" the default default value by setting it to
// "ENABLE" or "SYSTEM" or "", your code must handle this possibility.
//
// See OptPhysRelExpr.cpp's handling of PARALLEL_NUM_ESPS,
// an unsigned positive int which also accepts the keyword setting of "SYSTEM".
// See ImplRule.cpp's use of INSERT_VSBB, a keyword attr which allows "SYSTEM".
//
// A simple way to handle ON/OFF keywords that you want to hide the default for:
// Take OPTIMIZER_PRUNING as an example.  Right now, it appears below with
// default "OFF", and opt.cpp does
//	DisablePruning = (NADEFAULT(OPTIMIZER_PRUNING) == DF_OFF);
// To hide the default default,
// you would enter it below as "SYSTEM", and opt.cpp would do
//	DisablePruning = (NADEFAULT(OPTIMIZER_PRUNING) != DF_ON);
// (i.e., DF_OFF and DF_SYSTEM would be treated identically, as desired).
// *************************************************************************
// NOTE 3: The user is always allowed to say
//	CONTROL QUERY DEFAULT attrname 'SYSTEM';   -- or 'ENABLE' or ''
// What this means is that the current setting for that attribute
// reverts to its default-default value.  This default-default value
// may or may not be "SYSTEM"; this is completely orthogonal/irrelevant
// to the CQD usage.
//
// One gotcha:  'ENABLE' is a synonym for 'SYSTEM', *EXCEPT* when the
// SYSTEM default (the default-default) is "DISABLE".
// In this case, 'ENABLE' is a synonym for 'ON'
// (the opposite of the synonyms DISABLE/OFF).
// *************************************************************************
// NOTE 4: After modifying this static table in any way, INCLUDING A CODE MERGE,
// for a quick sanity check, run w:/toolbin/checkNAD.
// For a complete consistency check, compile this file, link arkcmp, and
// runregr TEST050.
// *************************************************************************

struct DefaultDefault
{
  enum DefaultConstants	 attrEnum;
  const char		*attrName;
  const char		*value;
  const DefaultValidator *validator;
  UInt32	 flags;
};

#define  DD(name,value,validator)	{ name, "" # name "", value, validator }
#define FDD(name,value,validator,flags)	{ name, "" # name "", value, validator, flags }
#define XDD(name,value,validator)	FDD(name,value,validator,DEFAULT_IS_EXTERNALIZED)
#define SDD(name,value,validator)	FDD(name,value,validator,DEFAULT_IS_FOR_SUPPORT)

#define DDS(name,value,validator)	FDD(name,value,validator,DEFAULT_IS_SSD)
#define XDDS(name,value,validator)	FDD(name,value,validator,DEFAULT_IS_SSD | DEFAULT_IS_EXTERNALIZED)
#define SDDS(name,value,validator)	FDD(name,value,validator,DEFAULT_IS_SSD | DEFAULT_IS_FOR_SUPPORT)

#define  DD_____(name,value)		 DD(name,value,&validateUnknown)
#define XDD_____(name,value)		XDD(name,value,&validateUnknown)
#define SDD_____(name,value)		SDD(name,value,&validateUnknown)
#define  DDS_____(name,value)		 DDS(name,value,&validateUnknown)
#define XDDS_____(name,value)		XDDS(name,value,&validateUnknown)
#define  DDansi_(name,value)		 DD(name,value,&validateAnsiName)
#define XDDansi_(name,value)		XDD(name,value,&validateAnsiName)

#define  DDcoll_(name,value)		 DD(name,value,&validateCollList)

#define  DDint__(name,value)		 DD(name,value,&validateInt)
#define SDDint__(name,value)		SDD(name,value,&validateInt)
#define XDDint__(name,value)		XDD(name,value,&validateInt)
#define  DDSint__(name,value)		 DDS(name,value,&validateInt)
#define XDDSint__(name,value)		XDDS(name,value,&validateInt)
#define XDDintN2(name,value)		XDD(name,value,&validateIntNeg2)
#define  DDintN1__(name,value)		 DD(name,value,&validateIntNeg1)
#define  DDpct__(name,value)		 DD(name,value,&validatePct)
#define XDDpct__(name,value)            XDD(name,value,&validatePct)
#define SDDpct__(name,value)            SDD(name,value,&validatePct)
#define  DDpct1_50(name,value)           DD(name,value,&validatePct1_t50)
#define  DD0_10485760(name,value)	 DD(name,value,&validate0_10485760)
#define  DD0_255(name,value)		 DD(name,value,&validate0_255)
#define  DD0_200000(name,value)	         DD(name,value,&validate0_200000)
#define XDD0_200000(name,value)	        XDD(name,value,&validate0_200000)
#define  DD1_200000(name,value)	         DD(name,value,&validate1_200000)
#define	XDDui30_32000(name,value)	XDD(name,value,&validate30_32000)
#define	 DDui30_246(name,value)	         DD(name,value,&validate30_246)
#define	 DDui50_4194303(name,value)	 DD(name,value,&validate50_4194303)
#define  DD1_24(name,value)              DD(name,value,&validate1_24)
#define XDD1_1024(name,value)           XDD(name,value,&validate1_1024)
#define  DD1_1024(name,value)            DD(name,value,&validate1_1024)
#define  DD18_128(name,value)            DD(name,value,&validate18_128)
#define  DD1_128(name,value)             DD(name,value,&validate1_128)
#define  DDui___(name,value)		 DD(name,value,&validateUI)
#define XDDui___(name,value)		XDD(name,value,&validateUI)
#define SDDui___(name,value)		SDD(name,value,&validateUI)
#define  DDui1__(name,value)		 DD(name,value,&validateUI1)
#define XDDui1__(name,value)		XDD(name,value,&validateUI1)
#define SDDui1__(name,value)		SDD(name,value,&validateUI1)
#define  DDui2__(name,value)		 DD(name,value,&validateUI2)
#define XDDui2__(name,value)		XDD(name,value,&validateUI2)
#define  DDui8__(name,value)		 DD(name,value,&validateUI8)
#define  DDui512(name,value)		 DD(name,value,&validateUI512)
#define	 DDui0_5(name,value)		 DD(name,value,&validateUIntFrom0To5)
#define	XDDui0_5(name,value)		XDD(name,value,&validateUIntFrom0To5)
#define	 DDui1_6(name,value)		 DD(name,value,&validateUIntFrom1To6)
#define	 DDui1_8(name,value)		 DD(name,value,&validateUIntFrom1To8)
#define	 DDui1_10(name,value)		 DD(name,value,&validateUIntFrom1To10)
#define	 DDui2_10(name,value)		 DD(name,value,&validateUIntFrom2To10)
#define	 DDui1500_4000(name,value)	 DD(name,value,&validateUIntFrom1500To4000)
#define  DDipcBu(name,value)		 DD(name,value,&validateIPCBuf)
#define XDDipcBu(name,value)		XDD(name,value,&validateIPCBuf)
#define  DDflt__(name,value)		 DD(name,value,&validateFlt)
#define XDDflt__(name,value)		XDD(name,value,&validateFlt)
#define SDDflt__(name,value)		SDD(name,value,&validateFlt)
#define  DDflt0_(name,value)		 DD(name,value,&validateFlt0)
#define XDDflt0_(name,value)		XDD(name,value,&validateFlt0)
#define SDDflt0_(name,value)		SDD(name,value,&validateFlt0)
#define  DDflte_(name,value)		 DD(name,value,&validateFltE)
#define XDDflte_(name,value)		XDD(name,value,&validateFltE)
#define SDDflte_(name,value)		SDD(name,value,&validateFltE)
#define  DDflt1_(name,value)		 DD(name,value,&validateFlt1)
#define XDDflt1_(name,value)		XDD(name,value,&validateFlt1)
#define  DDflt_0_1(name,value)		 DD(name,value,&validateFlt_0_1)
#define XDDflt_0_1(name,value)		XDD(name,value,&validateFlt_0_1)
#define  DDkwd__(name,value)		 DD(name,value,&validateKwd)
#define XDDkwd__(name,value)		XDD(name,value,&validateKwd)
#define SDDkwd__(name,value)		SDD(name,value,&validateKwd)
#define  DDSkwd__(name,value)		 DDS(name,value,&validateKwd)
#define SDDSkwd__(name,value)		SDDS(name,value,&validateKwd)
#define  DD1_4096(name,value)	         DD(name,value,&validate1_4096)
#define  DD0_18(name,value)	         DD(name,value,&validate0_18)
#define DD0_64(name,value)	        DD(name,value,&validate0_64)
#define DD16_64(name,value)	        DD(name,value,&validate16_64)
#define  DDalis_(name,value)		 DD(name,value,&validateAnsiList)
#define XDDalis_(name,value)		XDD(name,value,&validateAnsiList)
#define XDDpos__(name,value)            XDD(name,value,&validatePOSTableSizes)
#define SDDpos__(name,value)            SDD(name,value,&validatePOSTableSizes)
#define  DDpos__(name,value)             DD(name,value,&validatePOSTableSizes)
#define  DDtp___(name,value)             DD(name,value,&validateTraceStr)
#define  DDosch_(name,value)             DD(name,value,&validateOverrideSchema)
#define SDDosch_(name,value)            SDD(name,value,&validateOverrideSchema)
#define  DDpsch_(name,value)             DD(name,value,&validatePublicSchema)
#define SDDpsch_(name,value)            SDD(name,value,&validatePublicSchema)
#define  DDrlis_(name,value)		 DD(name,value,&validateRoleNameList)
#define XDDrlis_(name,value)		XDD(name,value,&validateRoleNameList)
#define  DDrver_(name,value)             DD(name,value,&validateReplIoVersion)
#define XDDMVA__(name,value)            XDD(name,value,&validateMVAge)
#define	 DDusht_(name,value)		 DD(name,value,&validate_uint16)


const DefaultValidator	validateUnknown;
const DefaultValidator	validateAnsiName(CASE_SENSITIVE_ANSI); // e.g. 'c.s.tbl'

      ValidateCollationList	validateCollList(TRUE/*mp-format*/);  // list collations
const ValidateInt		validateInt;	// allows neg, zero, pos ints
const ValidateIntNeg1         validateIntNeg1;// allows -1 to +infinity ints
const ValidateIntNeg1         validateIntNeg2;// allows -1 to +infinity ints
const ValidatePercent		validatePct;	// allows zero to 100 (integral %age)
const ValidateNumericRange    validatePct1_t50(VALID_UINT, 1, (float)50);// allows 1 to 50 (integral %age)
const Validate_0_10485760	validate0_10485760; // allows zero to 10Meg (integer)
const Validate_0_255		validate0_255;	// allows zero to 255 (integer)
const Validate_0_200000	validate0_200000;	// allows zero to 200000 (integer)
const Validate_1_200000	validate1_200000;	// allows 1 to 200000 (integer)
const Validate_30_32000       validate30_32000;     // allows 30 to 32000
const Validate_30_246         validate30_246;     // allows 30 to 246
const Validate_50_4194303     validate50_4194303;     // allows 50 to 4194303 (integer)
const Validate_1_24	validate1_24;	    // allows 1 to 24 (integer)
const ValidateUInt            validateUI;     // allows zero and pos
const ValidateUInt1		validateUI1;	// allows pos only (>= 1)
const ValidateUInt2		validateUI2(2);	// allows pos multiples of 2 only
const ValidateUInt2		validateUI8(8);		// pos multiples of 8 only
const ValidateUInt2		validateUI512(512);	// pos multiples of 512 only
const ValidateUIntFrom0To5	validateUIntFrom0To5;	// integer from 0 to 5
const ValidateUIntFrom1500To4000 validateUIntFrom1500To4000;	// integer from 1 to 6
const ValidateUIntFrom1To6	validateUIntFrom1To6;	// integer from 1 to 6
const ValidateUIntFrom1To8	validateUIntFrom1To8;	// integer from 1 to 8
const ValidateUIntFrom1To10	validateUIntFrom1To10;	// integer from 1 to 10
const ValidateUIntFrom2To10	validateUIntFrom2To10;	// integer from 2 to 10
const ValidateIPCBuf		validateIPCBuf;	// for IPC message buffers (DP2 msgs)
const ValidateFlt		validateFlt;	// allows neg, zero, pos (all nums)
const ValidateFltMin0		validateFlt0;	// allows zero and pos
const ValidateFltMinEpsilon	validateFltE;	// allows pos only (>= epsilon > 0)
const ValidateFltMin1		validateFlt1;	// allows pos only (>= 1)
const ValidateSelectivity	ValidateSelectivity;	// allows 0 to 1 (float)
const ValidateFlt_0_1		validateFlt_0_1;	// allows 0 to 1 (float)
const ValidateKeyword		validateKwd;	// allows relevant keywords only
const Validate_1_4096		validate1_4096;	    // allows 1 to 4096 (integer) which is max character size supported.
const Validate_0_18		validate0_18;	    // allows 0 to 18 (integer) because 18 is max precision supported.
const Validate_1_1024     validate1_1024;     // allows 1 to 1024 (integer).
const Validate_0_64           validate0_64;      // allows 0 to 64 (integer)
const Validate_16_64          validate16_64;     // allows 16 to 64 (integer)
const Validate_18_128         validate18_128;     // allows 18 to 128 (integer).
const Validate_1_128          validate1_128;     // allows 1 to 128 (integer).
// allows ':' separated list of three part ANSI names
const ValidateAnsiList        validateAnsiList;
// allows ',' separated list of role names
const ValidateRoleNameList    validateRoleNameList;
const ValidatePOSTableSizes   validatePOSTableSizes;
const ValidateTraceStr        validateTraceStr;
const ValidateOverrideSchema  validateOverrideSchema;  // check OverrideSchema format
const ValidatePublicSchema    validatePublicSchema;
// This high value should be same as default value of REPLICATE_IO_VERSION
const ValidateReplIoVersion   validateReplIoVersion(11,17);
const ValidateMVAge           validateMVAge;
const Validate_uint16         validate_uint16;

// See the NOTEs above for how to maintain this list!
THREAD_P DefaultDefault defaultDefaults[] = {
 DDflt0_(ACCEPTABLE_INPUTESTLOGPROP_ERROR,	"0.5"),

SDDint__(AFFINITY_VALUE,                        "-2"),

// controls the ESP allocation per core. 
 DDkwd__(AGGRESSIVE_ESP_ALLOCATION_PER_CORE,        "OFF"),


 // this should be used for testing only. DML should not be executed on
 // non-audited tables
 DDkwd__(ALLOW_DML_ON_NONAUDITED_TABLE,        "OFF"),

 // DP2_EXECUTOR_POSITION_SAMPLE method in DP2.
 // Valid values are ON, OFF and SYSTEM
 // ON => choose DP2_ROW_SAMPLING over row sampling in EID, if sampling % is less than 50.
 // OFF => choose EID row sampling over DP2 row sampling regardless of sampling %
 // SYSTEM => update stats will choose DP row sampling if sampling % is less than 5.

SDDkwd__(ALLOW_DP2_ROW_SAMPLING,               "SYSTEM"),

 DDkwd__(ALLOW_FIRSTN_IN_SUBQUERIES,	       "TRUE"),

 // ON/OFF flag to invoke ghost objects from non-licensed process (non-super.super user) who can not use parserflags
 DDkwd__(ALLOW_GHOST_OBJECTS, "OFF"),

 // This default, if set to ON, will allow Translate nodes (to/from UCS2)
 // to be automatically inserted by the Binder if some children of an
 // ItemExpr are declared as UCS2 and some are declared as ISO88591.
  DDkwd__(ALLOW_IMPLICIT_CHAR_CASTING, "ON"),

 // this default, if set to ON, will allow certain incompatible
 // assignment, like string to int. The assignment will be done by
 // implicitely CASTing one operand to another as long as CAST between
 // the two is supported. See binder for details.
  DDkwd__(ALLOW_INCOMPATIBLE_ASSIGNMENT,	"ON"),

 // this default, if set to ON, will allow certain incompatible
 // comparisons, like string to int. The comparison will be done by
 // implicitely CASTing one operand to another as long as CAST between
 // the two is supported. See binder for details.
  DDkwd__(ALLOW_INCOMPATIBLE_COMPARISON,	"ON"),

 // this default, if set to ON, will allow certain incompatible
 // comparisons. This includes incompatible comparisons, assignments,
 // conversions, UNION, arith, string and case stmts.
 // See binder(BindItemExpr.cpp, SynthType.cpp) for details.
  DDkwd__(ALLOW_INCOMPATIBLE_OPERATIONS,	"ON"),

  // if set to 2, the replicateNonKeyVEGPred() mdamkey method
  // will try to use inputs to filter out VEG elements that are not
  // local  to the associated table to minimize predicate replication.
  // It is defaulted to 0 (off), as there is some concern that this algoritm
  // might produce to few replications, which could lead to incorrect results.
  // Setting the Value to 1 will try a simpler optimization
  DDui___(ALLOW_INPUT_PRED_REPLICATION_REDUCTION,"0"),

  // if set to ON, then isolation level (read committed, etc) could be
  // specified in a regular CREATE VIEW (not a create MV) statement.
  DDkwd__(ALLOW_ISOLATION_LEVEL_IN_CREATE_VIEW,	"ON"),

  // if set to ON, then we allow subqueries of degree > 1 in the
  // select list.
  DDkwd__(ALLOW_MULTIDEGREE_SUBQ_IN_SELECTLIST,	"SYSTEM"),


  // by default, a primary key or unique constraint must be non-nullable.
  // This default, if set, allows them to be nullable.
  // The default value is OFF.
  DDkwd__(ALLOW_NULLABLE_UNIQUE_KEY_CONSTRAINT,	"OFF"),

  // if set to ON, then ORDER BY could be
  // specified in a regular CREATE VIEW (not a create MV) statement.
  DDkwd__(ALLOW_ORDER_BY_IN_CREATE_VIEW,	"ON"),

  DDkwd__(ALLOW_ORDER_BY_IN_SUBQUERIES,	        "ON"),

  // rand() function in sql is disabled unless this CQD is turned on
  DDkwd__(ALLOW_RAND_FUNCTION,			"ON"),

  DDkwd__(ALLOW_RANGE_PARTITIONING,	        "TRUE"),

  DDkwd__(ALLOW_RENAME_OF_MVF_OR_SUBQ,           "OFF"),

  DDkwd__(ALLOW_RISKY_UPDATE_WITH_NO_ROLLBACK,        "OFF"),

  DDkwd__(ALLOW_SUBQ_IN_SET,                     "SYSTEM"),

  DDkwd__(ALLOW_UNEXTERNALIZED_MAINTAIN_OPTIONS, "OFF"),

  // Allow users to grant privileges to role using the with grant option
  DDkwd__(ALLOW_WGO_FOR_ROLES,                  "ON"),

  DDSkwd__(ALTPRI_ESP,                          ""),
  DDSkwd__(ALTPRI_MASTER,	                ""),

  DDS_____(AQR_ENTRIES,                          ""),
  DDkwd__(AQR_WNR,                              "ON"),
  DDkwd__(AQR_WNR_DELETE_NO_ROWCOUNT,           "OFF"),
  DDkwd__(AQR_WNR_EXPLAIN_INSERT,               "OFF"),
  DDkwd__(AQR_WNR_INSERT_CLEANUP,               "OFF"),
  DDkwd__(AQR_WNR_LOCK_INSERT_TARGET,           "OFF"),

  DDkwd__(ARKCMP_FAKE_HW,		        "OFF"),
  DDkwd__(ASG_FEATURE,		                "ON"),
// Set ASM cache
  DDkwd__(ASM_ALLOWED,				"ON"),
// Precompute statistics in ASM
  DDkwd__(ASM_PRECOMPUTE,			"OFF"),
  DDkwd__(ASYMMETRIC_JOIN_TRANSFORMATION,       "MAXIMUM"),
  DDkwd__(ATTEMPT_ASYNCHRONOUS_ACCESS,		"ON"),
  DDkwd__(ATTEMPT_ESP_PARALLELISM,		"ON"),
  DDkwd__(ATTEMPT_REVERSE_SYNCHRONOUS_ORDER,    "ON"),

  DDkwd__(AUTOMATIC_RECOMPILATION,		"OFF"),

  DDkwd__(AUTO_QUERY_RETRY,                     "SYSTEM"),
 XDDkwd__(AUTO_QUERY_RETRY_WARNINGS,            "OFF"),

  DDkwd__(BASE_NUM_PAS_ON_ACTIVE_PARTS,		"OFF"),
  DDint__(BEGIN_TRANSACTION_FOR_SELECT,         "1"),

 // see comments in DefaultConstants.h
  DDkwd__(BIGNUM_IO,		                "SYSTEM"),

  DDint__(BLOCK_ENCRYPTION_MODE,             "0"),
 XDDkwd__(BLOCK_TO_PREVENT_HALLOWEEN,           "ON"),

  DDflte_(BMO_CITIZENSHIP_FACTOR,             "1."),

  DDflte_(BMO_MEMORY_EQUAL_QUOTA_SHARE_RATIO,        "0.5"),
  DDui___(BMO_MEMORY_ESTIMATE_OUTLIER_FACTOR,        "10"),
  DDflte_(BMO_MEMORY_ESTIMATE_RATIO_CAP,             "0.7"),
  DDui___(BMO_MEMORY_LIMIT_LOWER_BOUND_HASHGROUPBY , "25"),
  DDui___(BMO_MEMORY_LIMIT_LOWER_BOUND_HASHJOIN,     "25"),
  DDui___(BMO_MEMORY_LIMIT_LOWER_BOUND_SORT ,        "200"),
 XDDui___(BMO_MEMORY_LIMIT_PER_NODE_IN_MB,	     "10240"),
  DDui___(BMO_MEMORY_LIMIT_UPPER_BOUND,              "1200"),

  DDui1__(BMO_MEMORY_SIZE,                      "204800"),
  // percentage of physical main memory availabe for BMO.
  // This value is only used by HJ and HGB to come up with
  // an initial estimate for the number of clusters to allocate.
  // It does NOT by any means determine the amount of memory
  // used by a BMO. The memory usage depends on the amount of
  // memory available during execution and the amount of input
  // data.
  DDflte_(BMO_MEMORY_USAGE_PERCENT,             "5."),

 // When on, then try to bulk move nullable and variable length column values.
  DDkwd__(BULK_MOVE_NULL_VARCHAR,               "ON"),

  //Temporary fix to bypass volatile schema name checking for non-table objects - ALM Case#4764
  DDkwd__(BYPASS_CHECK_FOR_VOLATILE_SCHEMA_NAME,               "OFF"),

  DDkwd__(CACHE_HISTOGRAMS,                  "ON"),
  DDkwd__(CACHE_HISTOGRAMS_CHECK_FOR_LEAKS, "OFF"),
  DD0_200000(CACHE_HISTOGRAMS_IN_KB,            "32768"),
  DDkwd__(CACHE_HISTOGRAMS_MONITOR_HIST_DETAIL, "OFF"),
  DDkwd__(CACHE_HISTOGRAMS_MONITOR_MEM_DETAIL,  "OFF"),
  DD_____(CACHE_HISTOGRAMS_MONITOR_OUTPUT_FILE, ""),

 DD_____(CACHE_HISTOGRAMS_TRACE_OUTPUT_FILE, ""),
 DDkwd__(CALL_EMBEDDED_ARKCMP,       "OFF"),
 DDui___(CANCEL_MINIMUM_BLOCKING_INTERVAL,      "60"),

 DDkwd__(CANCEL_QUERY_ALLOWED,       "ON"), // Make sure it is in sync with SSD

 DDkwd__(CASCADED_GROUPBY_TRANSFORMATION,       "ON"),

 XDDansi_(CATALOG,				TRAFODION_SYSCAT_LIT),

 DDkwd__(CAT_ALLOW_NEW_FEATUREX, "OFF"),



 DDkwd__(CAT_DEFAULT_COMPRESSION, "NONE"),
 // Metadata table distribution schemes
 // OFF     - Place all metadata tables on one single disk
 // LOCAL_NODE - Distribute metadata tables across disks on local segment
 //              where first schema in the catalog is created
 // ON      - Distribute metadata tables across disks in local segment
 //              and visible remote segments
SDDkwd__(CAT_DISTRIBUTE_METADATA,             "ON"),
 //SDDkwd__(CAT_DISTRIBUTE_METADATA,             "ON"),

 // This disables Query Invalidation processing in catman when set to "OFF"
SDDkwd__(CAT_ENABLE_QUERY_INVALIDATION, "ON"),




// This enables the DB Limits functionality.  If set to OFF, then blocksize
// is restricted to 4096 and clustering key size is limited to 255 bytes.
// DB Limits checking is turned off on NT since NT's DP2 does not support
// large blocks or keys.
  DDkwd__(CAT_LARGE_BLOCKS_LARGE_KEYS, "ON"),

// Controls how pathnames for routines/procedures/SPJs are interpreted
  DDkwd__(CAT_LIBRARY_PATH_RELATIVE, "OFF"),






  // CMP_ERR_LOG_FILE indicates where to save a log for certain errors.
  DD_____(CMP_ERR_LOG_FILE,    "tdm_arkcmp_errors.log"),


  DDint__(COMPILER_IDLE_TIMEOUT,                    "1800"), // To match with set session defaults value

  // tracking compilers specific defaults
  DDint__(COMPILER_TRACKING_INTERVAL, "0"),
  DD_____(COMPILER_TRACKING_LOGFILE,  "NONE"),
  DDkwd__(COMPILER_TRACKING_LOGTABLE, "OFF"),

  DDkwd__(COMPILE_TIME_MONITOR,			"OFF"),
  DD_____(COMPILE_TIME_MONITOR_LOG_ALLTIME_ONLY, "OFF"),
  DD_____(COMPILE_TIME_MONITOR_OUTPUT_FILE,	"NONE"),


  // Switch between new aligned internal format and exploded format
  DDkwd__(COMPRESSED_INTERNAL_FORMAT,                           "SYSTEM"),
  DDkwd__(COMPRESSED_INTERNAL_FORMAT_BMO,                       "SYSTEM"),
  DDkwd__(COMPRESSED_INTERNAL_FORMAT_BMO_AFFINITY,              "ON"),
  DDkwd__(COMPRESSED_INTERNAL_FORMAT_BULK_MOVE,                 "ON"),
  DDflt0_(COMPRESSED_INTERNAL_FORMAT_DEFRAG_RATIO,              "0.30"),
  DDkwd__(COMPRESSED_INTERNAL_FORMAT_EXPLAIN,                   "OFF"),
  DDui1__(COMPRESSED_INTERNAL_FORMAT_MIN_ROW_SIZE,              "32"),
  DDkwd__(COMPRESSED_INTERNAL_FORMAT_ROOT_DOES_CONVERSION,      "OFF"),
  DDflt0_(COMPRESSED_INTERNAL_FORMAT_ROW_SIZE_ADJ,              "0.90"),


  XDDkwd__(COMPRESSION_TYPE, "NONE"),


  // These are switches and variables to use for compiler debugging
  DDkwd__(COMP_BOOL_1,      "OFF"),
  DDkwd__(COMP_BOOL_100,      "OFF"),
  DDkwd__(COMP_BOOL_101,      "OFF"),
  DDkwd__(COMP_BOOL_102,      "OFF"),
  DDkwd__(COMP_BOOL_103,      "OFF"),
  DDkwd__(COMP_BOOL_104,      "OFF"),
  DDkwd__(COMP_BOOL_106,      "OFF"),
  DDkwd__(COMP_BOOL_107,      "ON"), // Being used for testing default predicate synthesis in cardinality estimation
  DDkwd__(COMP_BOOL_108,      "ON"), // Being used for testing default predicate synthesis in cardinality estimation

  DDkwd__(COMP_BOOL_11,     "OFF"),
  DDkwd__(COMP_BOOL_110,      "OFF"),
  DDkwd__(COMP_BOOL_111,      "OFF"),
  DDkwd__(COMP_BOOL_112,      "OFF"),
  DDkwd__(COMP_BOOL_113,      "OFF"),
  DDkwd__(COMP_BOOL_114,      "ON"),  // If set to "OFF", turns off JIRA TRAFODION-3296 fix
  DDkwd__(COMP_BOOL_115,      "OFF"),
  DDkwd__(COMP_BOOL_116,      "OFF"),
  DDkwd__(COMP_BOOL_117,      "OFF"),
  DDkwd__(COMP_BOOL_118,      "OFF"), // soln 10-100508-0135 - allow undo of fix.
  DDkwd__(COMP_BOOL_119,      "OFF"),

  DDkwd__(COMP_BOOL_12,     "OFF"),
  DDkwd__(COMP_BOOL_120,      "OFF"),
  DDkwd__(COMP_BOOL_122,      "ON"), // Solution 10-081203-7708 fix
  DDkwd__(COMP_BOOL_123,      "OFF"),
  DDkwd__(COMP_BOOL_124,      "OFF"),
  DDkwd__(COMP_BOOL_125,      "ON"),
  DDkwd__(COMP_BOOL_126,      "OFF"),
  DDkwd__(COMP_BOOL_127,      "ON"),
  DDkwd__(COMP_BOOL_128,      "ON"),
  DDkwd__(COMP_BOOL_129,      "ON"),

  DDkwd__(COMP_BOOL_13,     "OFF"),
  DDkwd__(COMP_BOOL_130,      "ON"),
  DDkwd__(COMP_BOOL_131,      "OFF"),
  DDkwd__(COMP_BOOL_132,      "OFF"),
  DDkwd__(COMP_BOOL_133,      "OFF"),
  DDkwd__(COMP_BOOL_134,      "ON"),
  DDkwd__(COMP_BOOL_135,      "ON"),
  DDkwd__(COMP_BOOL_136,      "OFF"),
  DDkwd__(COMP_BOOL_137,      "OFF"), // ON enables logging of RewriteJoinPred
  DDkwd__(COMP_BOOL_138,      "OFF"), // ON disables tryToRewriteJoinPredicate
  DDkwd__(COMP_BOOL_139,      "OFF"),

  DDkwd__(COMP_BOOL_14,     "ON"),
  DDkwd__(COMP_BOOL_140,      "ON"),
  DDkwd__(COMP_BOOL_141,      "ON"),  // Used for testing MC UEC adjustment for uplifting join cardinality
  DDkwd__(COMP_BOOL_144,      "OFF"), // only Key columns usage as a part of materialization of disjuncts is controlled by the CQD
  DDkwd__(COMP_BOOL_145,      "ON"), // Used for selectivity adjustment for MC Joins
  DDkwd__(COMP_BOOL_147,      "OFF"),
  DDkwd__(COMP_BOOL_148,      "ON"), // Used for GroupBy Cardinality Enhancement for complex expressions
  DDkwd__(COMP_BOOL_149,      "ON"), // Used for testing multi-col uniqueness cardinality enhancement

  DDkwd__(COMP_BOOL_15,     "ON"),  // If ON, use runtime ROUND for BigNums; if OFF, rewrite ROUND for BigNums in terms of other functions
  DDkwd__(COMP_BOOL_150,      "OFF"),
  DDkwd__(COMP_BOOL_151,      "OFF"),
  DDkwd__(COMP_BOOL_152,      "OFF"),
  DDkwd__(COMP_BOOL_153,      "ON"),  // skew buster: ON == use round robin, else Co-located.
  DDkwd__(COMP_BOOL_154,      "OFF"),
  DDkwd__(COMP_BOOL_155,      "OFF"),
  DDkwd__(COMP_BOOL_156,      "ON"),  // Used by RTS to turn on RTS Stats collection for ROOT operators
  DDkwd__(COMP_BOOL_158,      "ON"),  // ON --> allows equijoins on VARCHAR/VARCHAR and CHAR/VARCHAR to be rewritten as VEGPreds
  DDkwd__(COMP_BOOL_159,      "OFF"),

  DDkwd__(COMP_BOOL_160,      "OFF"),
  DDkwd__(COMP_BOOL_161,      "OFF"),
  DDkwd__(COMP_BOOL_162,      "ON"),  // transform NOT EXISTS subquery using anti_semijoin instead of Join-Agg
  DDkwd__(COMP_BOOL_163,      "OFF"),
  DDkwd__(COMP_BOOL_164,      "OFF"),
  DDkwd__(COMP_BOOL_165,      "ON"), // set to 'ON' in M5 for SQ
  DDkwd__(COMP_BOOL_166,      "OFF"),  // ON --> turn off fix for 10-100310-8659.
  DDkwd__(COMP_BOOL_167,      "OFF"),
  DDkwd__(COMP_BOOL_168,      "ON"),
  DDkwd__(COMP_BOOL_169,      "OFF"),

  DDkwd__(COMP_BOOL_171,      "OFF"),
  DDkwd__(COMP_BOOL_172,      "OFF"),
  DDkwd__(COMP_BOOL_173,      "OFF"), //  fix: make odbc params nullable
  DDkwd__(COMP_BOOL_174,      "ON"),  // internal usage: merge stmt
  DDkwd__(COMP_BOOL_175,      "OFF"), // internal usage: merge stmt
  DDkwd__(COMP_BOOL_176,      "OFF"),
  DDkwd__(COMP_BOOL_177,      "OFF"),
  DDkwd__(COMP_BOOL_178,      "OFF"),

  DDkwd__(COMP_BOOL_18,     "OFF"),
  DDkwd__(COMP_BOOL_183,      "OFF"),
  DDkwd__(COMP_BOOL_184,      "ON"), // ON => use min probe size for mdam. Using min probe size of 1 or 2 currently has a bug so this is not the default. OFF => use default probe size of 100
  DDkwd__(COMP_BOOL_185,      "ON"), //Fix, allows extract(year from current_date) to be treated as a userinput
  DDkwd__(COMP_BOOL_186,      "OFF"),
  DDkwd__(COMP_BOOL_187,      "OFF"), // reserved for internal usage
  DDkwd__(COMP_BOOL_188,      "OFF"),
  DDkwd__(COMP_BOOL_189,      "OFF"), // reserved for internal usage

  DDkwd__(COMP_BOOL_19,       "OFF"),
  DDkwd__(COMP_BOOL_190,      "OFF"),
  DDkwd__(COMP_BOOL_191,      "OFF"), // Temp for UDF metadata switch
  DDkwd__(COMP_BOOL_192,      "OFF"),
  DDkwd__(COMP_BOOL_193,      "OFF"),
  DDkwd__(COMP_BOOL_194,      "ON"),  // If "OFF", turns off JIRA Trafodion 3325 fix
  DDkwd__(COMP_BOOL_196,      "OFF"),
  DDkwd__(COMP_BOOL_197,      "OFF"),
  DDkwd__(COMP_BOOL_198,      "OFF"),
  DDkwd__(COMP_BOOL_199,      "ON"),

  DDkwd__(COMP_BOOL_2,			"OFF"),
  DDkwd__(COMP_BOOL_20,			"OFF"),   // ON -> disable ability of stmt to be canceled.
  DDkwd__(COMP_BOOL_200,		"OFF"),
  DDkwd__(COMP_BOOL_201,		"OFF"),
  DDkwd__(COMP_BOOL_202,		"ON"),// For SQ:
                                               //   ON: excluding fixup cost
                                               //       for EXCHANGE for
                                               //       anti-surf logic;
                                               //   OFF: do include.
                                               // Change to ON in M5
  DDkwd__(COMP_BOOL_203,		"OFF"),
  DDkwd__(COMP_BOOL_206,		"OFF"), // Internal Usage
  DDkwd__(COMP_BOOL_207,		"OFF"), // Internal Usage
  DDkwd__(COMP_BOOL_21,			"OFF"),
  DDkwd__(COMP_BOOL_210,		"ON"),
  DDkwd__(COMP_BOOL_211,		"ON"), // controls removing constants from group expression
  DDkwd__(COMP_BOOL_217,                "OFF"),
  DDkwd__(COMP_BOOL_219,                "OFF"), // for InMem obj defn
  DDkwd__(COMP_BOOL_22,			"ON"),
  DDkwd__(COMP_BOOL_221,		"OFF"), // unnests a subquery even when there is no explicit correlation
                                                // bulk replicate features
                                                // bulk replicate features
  DDkwd__(COMP_BOOL_226,                "OFF"),  // ON enables UNLOAD feature
                                                // for disk label stats.
  DDkwd__(COMP_BOOL_23,			"ON"),
  DDkwd__(COMP_BOOL_24,			"OFF"), // AS enhancement to adjust maxDoP
  DDkwd__(COMP_BOOL_25,			"OFF"), // Being used in Cardinality Estimation
  DDkwd__(COMP_BOOL_27,			"OFF"),
  DDkwd__(COMP_BOOL_28,			"OFF"),
  DDkwd__(COMP_BOOL_29,			"OFF"),

  DDkwd__(COMP_BOOL_3,			"OFF"),
  DDkwd__(COMP_BOOL_30,			"ON"),
  DDkwd__(COMP_BOOL_31,			"OFF"),
  DDkwd__(COMP_BOOL_32,			"OFF"),
  DDkwd__(COMP_BOOL_33,			"OFF"),
  DDkwd__(COMP_BOOL_34,			"OFF"),
  DDkwd__(COMP_BOOL_35,			"OFF"),
  DDkwd__(COMP_BOOL_36,			"OFF"),
  DDkwd__(COMP_BOOL_37,			"OFF"),
  DDkwd__(COMP_BOOL_38,			"OFF"),
  DDkwd__(COMP_BOOL_39,			"OFF"),

  DDkwd__(COMP_BOOL_4,			"OFF"),
  DDkwd__(COMP_BOOL_40,			"ON"),
  DDkwd__(COMP_BOOL_41,			"OFF"),
  DDkwd__(COMP_BOOL_42,			"ON"),
  DDkwd__(COMP_BOOL_44,			"OFF"),
  DDkwd__(COMP_BOOL_45,			"ON"),
  DDkwd__(COMP_BOOL_46,			"OFF"),
  DDkwd__(COMP_BOOL_47,			"ON"),
  DDkwd__(COMP_BOOL_48,			"ON"), // Turned "Off" because of Regression failure
  DDkwd__(COMP_BOOL_49,			"OFF"),

  DDkwd__(COMP_BOOL_5,			"ON"),
  DDkwd__(COMP_BOOL_51,			"OFF"),
  DDkwd__(COMP_BOOL_52,			"OFF"),
  DDkwd__(COMP_BOOL_53,			"ON"), //Turned "ON" for OCB Cost
  DDkwd__(COMP_BOOL_54,			"OFF"),
  DDkwd__(COMP_BOOL_55,			"OFF"),
  DDkwd__(COMP_BOOL_56,			"OFF"),
  DDkwd__(COMP_BOOL_57,			"ON"),
  DDkwd__(COMP_BOOL_59,			"OFF"),

  // comp_bool_60 is used in costing of an exchange operator. This is
  // used in deciding to use  Nodemap decoupling and other exchange
  // costing logic.
  DDkwd__(COMP_BOOL_60,			"ON"),
  DDkwd__(COMP_BOOL_61,			"OFF"),
  DDkwd__(COMP_BOOL_62,			"OFF"),
  DDkwd__(COMP_BOOL_63,			"OFF"),
  DDkwd__(COMP_BOOL_64,			"OFF"),
  DDkwd__(COMP_BOOL_65,			"OFF"),
  DDkwd__(COMP_BOOL_66,			"OFF"),
  DDkwd__(COMP_BOOL_67,			"ON"), // Being used in Cardinality Estimation
  DDkwd__(COMP_BOOL_68,			"ON"),
  DDkwd__(COMP_BOOL_69,			"OFF"),

  DDkwd__(COMP_BOOL_7,			"OFF"),
  DDkwd__(COMP_BOOL_70,			"ON"),
  DDkwd__(COMP_BOOL_71,			"OFF"),
  DDkwd__(COMP_BOOL_72,			"OFF"),
  DDkwd__(COMP_BOOL_73,			"OFF"),
  DDkwd__(COMP_BOOL_74,			"ON"),
  DDkwd__(COMP_BOOL_75,			"ON"),
  DDkwd__(COMP_BOOL_76,			"ON"),
  DDkwd__(COMP_BOOL_77,			"OFF"),
  DDkwd__(COMP_BOOL_78,			"OFF"),
  DDkwd__(COMP_BOOL_79,			"ON"),

  DDkwd__(COMP_BOOL_8,			"OFF"),
  DDkwd__(COMP_BOOL_80,			"OFF"),
  DDkwd__(COMP_BOOL_81,			"OFF"),
  DDkwd__(COMP_BOOL_82,			"OFF"),
  DDkwd__(COMP_BOOL_83,			"ON"),
  DDkwd__(COMP_BOOL_84,			"OFF"),
  DDkwd__(COMP_BOOL_85,			"OFF"),
  DDkwd__(COMP_BOOL_86,			"OFF"),
  DDkwd__(COMP_BOOL_87,			"OFF"),
  DDkwd__(COMP_BOOL_88,			"OFF"),

  DDkwd__(COMP_BOOL_9,			"OFF"),
  DDkwd__(COMP_BOOL_90,			"ON"),
  DDkwd__(COMP_BOOL_91,			"OFF"),
  DDkwd__(COMP_BOOL_92,			"OFF"), // used by generator.
  DDkwd__(COMP_BOOL_93,			"ON"), // turn on pushdown for IUDs involving MVs. Default is off
  DDkwd__(COMP_BOOL_94,			"OFF"),
  DDkwd__(COMP_BOOL_95,			"OFF"),
  DDkwd__(COMP_BOOL_96,			"OFF"),
  DDkwd__(COMP_BOOL_97,			"OFF"),
  DDkwd__(COMP_BOOL_98,			"ON"),
  DDkwd__(COMP_BOOL_99,			"OFF"),

  DDflt0_(COMP_FLOAT_0,	        "0.002"),
  DDflt0_(COMP_FLOAT_1,	        "0.00002"),
  DDflt0_(COMP_FLOAT_2,	        "0"),
  DDflt0_(COMP_FLOAT_3,	        "0.01"),
  DDflt0_(COMP_FLOAT_4,	        "1.1"),
  DDflt__(COMP_FLOAT_5,	        "0.01"), // For Split Top cost adjustments : 0.25
  DDflt__(COMP_FLOAT_6,	        "0.67"), // used to set the fudge factor which
										 // is used to estimate cardinality of an
										 // aggregate function in an equi-join expression
  DDflt__(COMP_FLOAT_7,	        "1.5"),
  DDflt__(COMP_FLOAT_8,	        "0.8"),
  // min expected #groups when HGB under right side of NLJ
  DDflt__(COMP_FLOAT_9,	        "1002.0"),

  DDint__(COMP_INT_0,         "5000"),
  DDint__(COMP_INT_10,         "3"),
  DDint__(COMP_INT_11,         "-1"),
  DDint__(COMP_INT_14,         "0"),
  DDint__(COMP_INT_15,         "7"),
  DDint__(COMP_INT_16,         "1000000"),
  DDint__(COMP_INT_17,         "1000000"),
  DDint__(COMP_INT_18,         "1"),
  DDint__(COMP_INT_19,         "2"),
  DDint__(COMP_INT_2,          "1"),
  DDint__(COMP_INT_20,         "4"),
  DDint__(COMP_INT_21,         "0"),
  DDint__(COMP_INT_22,         "0"), // used to control old parser based INLIST transformation
  // 0 ==> OFF, positive value implies ON and has the effect of implicitly shutting down much of OR_PRED transformations
  // this cqd has been retained as a fallback in case OR_PRED has bugs.
  DDint__(COMP_INT_23,         "22"),
  DDint__(COMP_INT_24,         "1000000000"),
  DDint__(COMP_INT_26,         "1"),
  DDint__(COMP_INT_3,          "5"),
  DDint__(COMP_INT_30,         "5"),
  DDint__(COMP_INT_31,         "5"),
  DDint__(COMP_INT_32,         "100"),
  DDint__(COMP_INT_34,         "10000"), // lower bound: 10000
  DDint__(COMP_INT_35,         "500000"), // upper bound: 200000
  DDint__(COMP_INT_36,         "128"),   // Bounds for producer for OCB
  DDint__(COMP_INT_38,         "0"),    // test master's abend
  DDint__(COMP_INT_39,         "0"),    // test esp's abend
  DDint__(COMP_INT_4,         "400"),
  DDint__(COMP_INT_40,         "10"),   // this defines the percentage of selectivity after applying equality predicates on single column histograms
                                        // beyond which the optimizer should use MC stats
  DDint__(COMP_INT_43,         "3"), // this is only for testing purposes. Once HIST_USE_SAMPLE_FOR_CARDINALITY_ESTIMATION  is set to ON by default, the value of this CQD should be adjusted
  DDint__(COMP_INT_44,         "1000000"), // frequency threshold above which
                                           // a boundary value will be inclded
                                           // in the frequentValueList (stats)
  DDint__(COMP_INT_45,         "300"),
  DDint__(COMP_INT_46,         "10"),
  DDint__(COMP_INT_47,         "0"),
  DDint__(COMP_INT_48,         "32"),  // # trips thru scheduler task list before eval of CPU time limit.


  DDint__(COMP_INT_5,         "0"),
  DDint__(COMP_INT_50,         "0"),
  DDint__(COMP_INT_51,         "0"),
  DDint__(COMP_INT_54,         "0"),
  // comp_int_60 is used in costing of an exchnage operator. It is
  // used to indicate buffer size of a DP2 exchange when sending
  // messages down.
  DDint__(COMP_INT_60,         "4"),
  DDint__(COMP_INT_61,         "0"),
  // Exchange operator default value
  DDint__(COMP_INT_62,         "10000"),
  DDint__(COMP_INT_63,         "10000"), // SG Insert issue
  DDint__(COMP_INT_66,         "0"), // to change #buffers per flushed cluster
  DDint__(COMP_INT_67,         "8"), // to test #outer-buffers per a batch
  DDint__(COMP_INT_7,         "10000000"),
  DDint__(COMP_INT_70,         "1000000"),
  DDint__(COMP_INT_71,         "0"),
  // if set to 1, allows keyPredicate to be inserted without passing key col.
  DDint__(COMP_INT_73,         "1"),
  // if set to 1, disables cursor_delete plan if there are no alternate indexes.
  DDint__(COMP_INT_74,         "0"),
  DDint__(COMP_INT_77,         "0"),
  DDint__(COMP_INT_79,         "0"),
  // this is used temporaraly as value for parallel threshold
  // in case ATTEMPT_ESP_PARALLELISM is set to MAXIMUM
  DDint__(COMP_INT_8,         "20"),
  DDint__(COMP_INT_80,         "3"),


  DDint__(COMP_INT_89,         "2"),
  DDint__(COMP_INT_9,         "0"),
  DDint__(COMP_INT_90,         "0"),
  DDint__(COMP_INT_95,         "0"),
  DDint__(COMP_INT_98,         "512"),
  DDint__(COMP_INT_99,         "10"),

  DD_____(COMP_STRING_1,	    "NONE"),
  DD_____(COMP_STRING_2,	    ""),
  DD_____(COMP_STRING_5,	    ""),


  DDkwd__(CONSTANT_FOLDING,                     "OFF"),

  DDkwd__(COSTING_SHORTCUT_GROUPBY_FIX,                "ON"),
  DDflt0_(COST_PROBE_DENSITY_THRESHOLD, ".25"),

  DDflt0_(CPUCOST_COMPARE_COMPLEX_DATA_TYPE_OVERHEAD,	"10."),
  DDflt0_(CPUCOST_COMPARE_COMPLEX_DATA_TYPE_PER_BYTE,	".1"),

  // Same as CPUCOST_PREDICATE_COMPARISON
  // Change HH_OP_PROBE_HASH_TABLE when you change this value:
    DDflt0_(CPUCOST_COMPARE_SIMPLE_DATA_TYPE,   ".200"),
  // no cost overhead assumed:
  DDflt0_(CPUCOST_COPY_ROW_OVERHEAD,		"0."),

  // change CPUCOST_HASH_PER_KEY when changing this value
  DDflt0_(CPUCOST_COPY_ROW_PER_BYTE,		".0007"),

  DDflt0_(CPUCOST_COPY_SIMPLE_DATA_TYPE,	".005"),
  // This is a per data request overhead cost paid by the cpu
  DDflt0_(CPUCOST_DATARQST_OVHD,		".01"),
  DDflt0_(CPUCOST_ENCODE_PER_BYTE,		".002"),
  DDflt0_(CPUCOST_ESP_INITIALIZATION,		"10"),

  // The previous observation had calculated the number of seconds to
  // aggregate incorrectly. Now:
  //  Number of seconds to scan 100,000 rows @ 208 bytes: 4
  //  Number of seconds to scan 100,000 rows @ 208 bytes and aggregate
  // 15 aggregates: 17
  // Thus, number of seconds per aggregate = (17-4)/15 = 0.866667
  // CPUCOST_PER_ROW = 1.13333/(0.00005*100,000) = 0.1733
  // previous observation
  // It takes 13.96 seconds to aggregate 99,999 rows using
  // 15 expressions, thus at 0.00005 et_cpu, we have that
  // the cost to eval an arith op is:
  // 6.14 / (0.00005 * 99,9999 * 15) = 0.0819
  DDflt0_(CPUCOST_EVAL_ARITH_OP,		".0305"),

  DDflt0_(CPUCOST_EVAL_FUNC_DEFAULT,		"10."),
  DDflt0_(CPUCOST_EVAL_SIMPLE_PREDICATE,	"1."),
  DDflt0_(CPUCOST_EXCHANGE_COST_PER_BYTE,             ".002"),
  DDflt0_(CPUCOST_EXCHANGE_INTERNODE_COST_PER_BYTE,   ".008"),
  // was 0.1, but now 0.011
  // XDDflt0_(CPUCOST_EXCHANGE_REMOTENODE_COST_PER_BYTE,    ".011"),
  
  // Set the additional cost of copying a byte to message buffer for
  // remote node to be the same as for inter node, 0.01
  // Also change it to be internalized
  DDflt0_(CPUCOST_EXCHANGE_REMOTENODE_COST_PER_BYTE,    ".01"),
  // Assume
  // CPUCOST_HASH_PER_KEY = 4 * CPUCOST_HASH_PER_BYTE
  // History:
  // Before 01/06/98: 0.005
  DDflt0_(CPUCOST_HASH_PER_BYTE,		".057325"),

  // Assume
  // CPUCOST_HASH_PER_KEY = 4 * CPUCOST_HASH_PER_BYTE
  // From observation:
  // For a case when all the hash table fits into memory:
  // 01/05/98: 42,105 rows inserted per second @ 0.00005 seconds
  // per thousand of instructions, give:
  // seconds to insert one row = 1/42105 = 0.00002375
  // thd. of instructions per row inserted = 1/42105/0.00005 = 0.4750
  // The cost is distributed as follows:
  // CPUCOST_HASH_PER_KEY + CPUCOST_HASH_PER_BYTE*4 +
  // HH_OP_INSERT_ROW_TO_CHAIN + CPUCOST_COPY_ROW_PER_BYTE * 4
  // = 0.4750
  // Thus we have:
  // 2* CPUCOST_HASH_PER_KEY + 0.01 + 0.0016*4 = 0.4750
  // -> CPUCOST_HASH_PER_KEY = 0.4586/2 = 0.2293
  // History:
  // Before 01/06/98: 0.02
  // Change
  // CPUCOST_HASH_PER_BYTE
  // when changing this value
  DDflt0_(CPUCOST_HASH_PER_KEY,			"1.29"),
  DDflt0_(CPUCOST_LIKE_COMPARE_OVERHEAD,	"10."),
  DDflt0_(CPUCOST_LIKE_COMPARE_PER_BYTE,	".1"),
  DDflt0_(CPUCOST_LOCK_ROW,			".01"),
  DDflt0_(CPUCOST_NJ_TUPLST_FF,			"10."),

  //  Observation (A971125_1):
  // CPU time to scan 100,000 rows with no exe pred: 10
  // CPU time to scan 100,000 rows with an exe pred like
  //    nonkeycol < K: 11
  // CPU time spend in every row: 1/100,000 = .00001
  // Thus, at 0.00005 th. inst. per sec we have: 0.00001/0.00005 =
  // 0.2 thousand inst. to evaluate every row:
  //
  // Predicate comparison is very expensive right now (10/08/97)
  // (cost it that it takes like 1000 instruction for one comparison)
  // 10/08/97: 1.
  // Change
  // CPUCOST_COMPARE_SIMPLE_DATA_TYPE
  // when you change this value:
  // History
  // Before 04/30/98: .2
  DDflt0_(CPUCOST_PREDICATE_COMPARISON,		".08"),

  // Cost of copying the data from disk to the DP2 Cache:
  DDflt0_(CPUCOST_SCAN_DSK_TO_DP2_PER_KB,		"2.5"),
  DDflt0_(CPUCOST_SCAN_DSK_TO_DP2_PER_SEEK,		"0.0"),

  // The communication between DP2 and ExeInDp2 requires to encode
  // and decode the key.
  DDflt0_(CPUCOST_SCAN_KEY_LENGTH,		"0."),

  // The communication between DP2 and ExeInDp2 is complex and
  // ever changing. The following factor is introduced to
  // make the costing of scan fit observed CPU time for the scan:
  DDflt0_(CPUCOST_SCAN_OVH_PER_KB,		"0.984215"),
  DDflt0_(CPUCOST_SCAN_OVH_PER_ROW,		"0.0"),

  // It takes about 1/3 of a second to open a table, thus with a
  // 0.00005 ff for cpu elapsed time we get:
  // 1/3/0.00005 = 7000 thousands instructions
  // CPUCOST_SUBSET_OPEN lumps together all the overhead needed
  // to set-up the access to each partition. Thus it is a blocking
  // cost, nothing can overlap with it.
  DDflt0_(CPUCOST_SUBSET_OPEN,			"7000"),
  DDflt0_(CPUCOST_SUBSET_OPEN_AFTER_FIRST,	"1250"),



 DDkwd__(CREATE_OBJECTS_IN_METADATA_ONLY,      "OFF"),

 DDkwd__(CROSS_PRODUCT_CONTROL,	               "ON"),

 // CQDs for Common Subexpressions (CSEs)
 // cache queries containing temp tables for common subexpressions
 DDkwd__(CSE_CACHE_TEMP_QUERIES,               "OFF"),
 // "cleanup obsolete volatile tables" command cleans up Hive temp tables
 DDkwd__(CSE_CLEANUP_HIVE_TABLES,              "OFF"),
 // don't temp if all consumers have preds on n key columns
 DDui___(CSE_COMMON_KEY_PRED_CONTROL,          "1"),
 // emit warnings that help diagnose why CSEs are not shared
 DDkwd__(CSE_DEBUG_WARNINGS,                   "OFF"),
 // create a CommonSubExpr node for CTEs defined in WITH clauses (OFF/ON)
 DDkwd__(CSE_FOR_WITH,                         "OFF"),
 // use Hive tables as temp tables
 DDkwd__(CSE_HIVE_TEMP_TABLE,                  "ON"),
 // don't temp if avg consumer has preds on more than n percent of key cols
 DDflt0_(CSE_PCT_KEY_COL_PRED_CONTROL,         "49.9"),
 // print debugging info on stdout
 DDkwd__(CSE_PRINT_DEBUG_INFO,                 "OFF"),
 // limit temp table size (based on max. card and regular card)
 DDflt0_(CSE_TEMP_TABLE_MAX_MAX_SIZE,          "1E12"),
 DDflt0_(CSE_TEMP_TABLE_MAX_SIZE,              "1E9"),
 // implement CommonSubExpr as a temp table (OFF/SYSTEM/ON)
 DDkwd__(CSE_USE_TEMP,                         "SYSTEM"),

SDDui___(CYCLIC_ESP_PLACEMENT,                  "1"),

  // if this one is "ON" it overwrites optimizer heuristics 4 & 5 as "ON"
  // if it's "OFF" then the defaults of the two heuristics will be used
  DDkwd__(DATA_FLOW_OPTIMIZATION,		"ON"),


  DDkwd__(DDL_EXPLAIN,                           "OFF"),
  DDkwd__(DDL_TRANSACTIONS,         "ON"),


 SDDkwd__(DEFAULT_CHARSET,           (char *)SQLCHARSETSTRING_ISO88591),
 XDDui1__(DEFAULT_DEGREE_OF_PARALLELISM,    "2"),

 SDDkwd__(DEFAULT_SCHEMA_ACCESS_ONLY, "OFF"),
 SDDkwd__(DEFAULT_SCHEMA_NAMETYPE, "SYSTEM"),

// These DEF_xxx values of "" get filled in by updateSystemParameters().
  #define def_DEF_CHUNK_SIZE			 5000000.0
  #define str_DEF_CHUNK_SIZE			"5000000.0"
  //
  DDui2__(DEF_CHUNK_SIZE,                       str_DEF_CHUNK_SIZE),
  DD_____(DEF_CPU_ARCHITECTURE,			""),
  DDui1__(DEF_DISCS_ON_CLUSTER,			""),
  DDui1__(DEF_INSTRUCTIONS_SECOND,		""),
  DDui___(DEF_LOCAL_CLUSTER_NUMBER,		""),
  DDui___(DEF_LOCAL_SMP_NODE_NUMBER,		""),
//DEF_MAX_HISTORY_ROWS made external RV 06/21/01 CR 10-010425-2440
  XDDui1__(DEF_MAX_HISTORY_ROWS,			"1024"),
  DDui___(DEF_NUM_BM_CHUNKS,			""),
  DDui1__(DEF_NUM_NODES_IN_ACTIVE_CLUSTERS,	""),
  DDui1__(DEF_NUM_SMP_CPUS,			""),
  DDui2__(DEF_PAGE_SIZE,			""),
  DDui1__(DEF_PHYSICAL_MEMORY_AVAILABLE,	""),
  DDui1__(DEF_TOTAL_MEMORY_AVAILABLE,           ""),
  DDui1__(DEF_VIRTUAL_MEMORY_AVAILABLE,		""),

  DDkwd__(DESTROY_ORDER_AFTER_REPARTITIONING, "OFF"),

  // detailed executor statistics
  DDkwd__(DETAILED_STATISTICS,                  "OPERATOR"),

  DDkwd__(DIMENSIONAL_QUERY_OPTIMIZATION,		"OFF"),
  DDkwd__(DISABLE_BUFFERED_INSERTS,		"OFF"),
  DDkwd__(DISABLE_READ_ONLY,		"OFF"),

  DD_____(DISPLAY_DATA_FLOW_GRAPH,		"OFF"),
 XDDkwd__(DISPLAY_DIVISION_BY_COLUMNS,          "OFF"),


  // temp. disable dop reduction logic
  DDflt0_(DOP_REDUCTION_ROWCOUNT_THRESHOLD,	"0.0"),





  // DP2 Cache defaults as of 06/08/98.
  DDui1__(DP2_CACHE_1024_BLOCKS,		"152"),
  DDui1__(DP2_CACHE_16K_BLOCKS,                 "1024"),
  DDui1__(DP2_CACHE_2048_BLOCKS,		"150"),
  DDui1__(DP2_CACHE_32K_BLOCKS,                 "512"),
  DDui1__(DP2_CACHE_4096_BLOCKS,              "4096"),
  DDui1__(DP2_CACHE_512_BLOCKS,			"152"),
  DDui1__(DP2_CACHE_8K_BLOCKS,                  "2048"),
  // Exchange Costing
  // 6/12/98.
  // End of buffer header is 32 bytes or .0313 KB.
  // Each Exchange->DP2 request is 48 bytes or .0469 KB.
  DDflte_(DP2_END_OF_BUFFER_HEADER_SIZE,	".0313"),
  DDflte_(DP2_EXCHANGE_REQUEST_SIZE,		".0469"),
  DDui2__(DP2_MAX_READ_PER_ACCESS_IN_KB,        "256"),
  // The buffer size, as of 10/07/97 is 32K
  DDui2__(DP2_MESSAGE_BUFFER_SIZE,            "56"),
  // Exchange Costing
  // 6/12/98.
  // Message header for Exchange->DP2 is 18 bytes or .0176 KB
  DDflte_(DP2_MESSAGE_HEADER_SIZE,		".0176"),
  DDui2__(DP2_MESSAGE_HEADER_SIZE_BYTES,	"18"),
  DDui1__(DP2_MINIMUM_FILE_SIZE_FOR_SEEK_IN_BLOCKS,  "256"),


  DDui1__(DP2_SEQ_READS_WITHOUT_SEEKS,               "100"),
  DDkwd__(DYNAMIC_HISTOGRAM_COMPRESSION,              "ON"),
  DDui2__(DYN_PA_QUEUE_RESIZE_INIT_DOWN,        "1024"),
  DDui2__(DYN_PA_QUEUE_RESIZE_INIT_UP,          "1024"),
  DDui2__(DYN_QUEUE_RESIZE_FACTOR,		"4"),
  DDui2__(DYN_QUEUE_RESIZE_INIT_DOWN,		"4"),
  DDui2__(DYN_QUEUE_RESIZE_INIT_UP,		"4"),
  DDui1__(DYN_QUEUE_RESIZE_LIMIT,		"9"),
  DDkwd__(DYN_QUEUE_RESIZE_OVERRIDE,             "OFF"),

  DDkwd__(EID_SPACE_USAGE_OPT,			"OFF"),

  DDkwd__(ELIMINATE_REDUNDANT_JOINS,             "ON"),


  DDSint__(ESP_ASSIGN_DEPTH,                    "0"),

  DDSint__(ESP_FIXUP_PRIORITY_DELTA,            "0"),
  DDint__(ESP_IDLE_TIMEOUT,                    "1800"), // To match with set session defaults value
  DDkwd__(ESP_MULTI_FRAGMENTS,			"ON"),
  DDui1500_4000(ESP_MULTI_FRAGMENT_QUOTA_VM,	"4000"),
  DDui1_8(ESP_NUM_FRAGMENTS,			"3"),

  DDSint__(ESP_PRIORITY,                        "0"),
  DDSint__(ESP_PRIORITY_DELTA,                  "0"),

 // Disable hints - if SYSTEM, enable on SSD, and disable only on HDD
  DDkwd__(EXE_BMO_DISABLE_CMP_HINTS_OVERFLOW_HASH,	"SYSTEM"),
  DDkwd__(EXE_BMO_DISABLE_CMP_HINTS_OVERFLOW_SORT,	"SYSTEM"),
  DDkwd__(EXE_BMO_DISABLE_OVERFLOW,		"OFF"),
  DDui___(EXE_BMO_MIN_SIZE_BEFORE_PRESSURE_CHECK_IN_MB,	"50"),
  DDkwd__(EXE_BMO_SET_BUFFERED_WRITES,		"OFF"),

SDDkwd__(EXE_DIAGNOSTIC_EVENTS,		"OFF"),
 DDui1__(EXE_HGB_INITIAL_HT_SIZE,		"262144"), // == hash buffer

 DDflt__(EXE_HJ_MIN_NUM_CLUSTERS,	        "4"),

  DDkwd__(EXE_LOG_RETRY_IPC,                    "OFF"),

 // Total size of memory (in MB) available to BMOs (e.g., 1200 MB)
 SDDui___(EXE_MEMORY_AVAILABLE_IN_MB,		"1200"),

 SDDui___(EXE_MEMORY_FOR_PARTIALHGB_IN_MB,	"100"),

 SDDui___(EXE_MEMORY_FOR_PROBE_CACHE_IN_MB,	"100"),

 SDDui___(EXE_MEMORY_FOR_UNPACK_ROWS_IN_MB,	"1024"),

  // lower-bound memory limit for BMOs/nbmos (in MB)
  DDui___(EXE_MEMORY_LIMIT_LOWER_BOUND_EXCHANGE, "10"),
  DDui___(EXE_MEMORY_LIMIT_LOWER_BOUND_MERGEJOIN, "10"),
  DDui___(EXE_MEMORY_LIMIT_LOWER_BOUND_SEQUENCE , "10"),

 // Override the memory quota system; set limit per each and every BMO
 SDDflt__(EXE_MEM_LIMIT_PER_BMO_IN_MB,	        "0"),

  DDui1__(EXE_NUM_CONCURRENT_SCRATCH_IOS,	"4"), //

  DDkwd__(EXE_PARALLEL_DDL,                     "ON"),


  DDkwd__(EXE_SINGLE_BMO_QUOTA,                 "ON"),

  // The following 3 are only for testing overflow; zero value means: ignore
  DDui___(EXE_TEST_FORCE_CLUSTER_SPLIT_AFTER_MB,      "0"),
  DDui___(EXE_TEST_FORCE_HASH_LOOP_AFTER_NUM_BUFFERS, "0"),
  DDui___(EXE_TEST_HASH_FORCE_OVERFLOW_EVERY,         "0"),

  DDkwd__(EXE_UTIL_RWRS,		                "OFF"),

  DDkwd__(EXPAND_DP2_SHORT_ROWS,		"ON"),

 XDDint__(EXPLAIN_DESCRIPTION_COLUMN_SIZE,    "-1"),

  DDkwd__(EXPLAIN_DETAIL_COST_FOR_CALIBRATION,  "FALSE"),

  DDkwd__(EXPLAIN_DISPLAY_FORMAT,		"EXTERNAL"),

  DDkwd__(EXPLAIN_IN_RMS, 		        "OFF"),

  DDkwd__(EXPLAIN_OPTION_C,                     "OFF"),

  DDui___(EXPLAIN_OUTPUT_ROW_SIZE,   "80"),

  DDui1__(EXPLAIN_ROOT_INPUT_VARS_MAX,           "2000"), // maximum number of inputs that we can tolerate to 
                                                          // explain information for inputVars expression
                                                          // this is needed to avoid stack overflow

  DDkwd__(EXPLAIN_SPACE_OPT, 		        "ON"),

  DDkwd__(EXPLAIN_STRATEGIZER_PARAMETERS,  "OFF"),


  // Calibration
  // 01/23/98: 50.
  // Original: .1
  DDflte_(EX_OP_ALLOCATE_BUFFER,                "50."),
  DDflte_(EX_OP_ALLOCATE_BUFFER_POOL,           ".1"),
  DDflte_(EX_OP_ALLOCATE_TUPLE,                 ".05"),

  // copy_atp affects the costing of NJ
  // History:
  // 08/21/98: 0.02, The previous change affected more than one operrator
  // 08/13/98: 1.0
  // 01/08/98: 0.02
  DDflte_(EX_OP_COPY_ATP,			"1.1335"),

  DDkwd__(FAKE_VOLUME_ASSIGNMENTS,		"OFF"),
  DDui1__(FAKE_VOLUME_NUM_VOLUMES,              "24"),

 // upper and lower limit (2,10) must be in sync with error values in 
 //ExFastTransport.cpp
  DDkwd__(FAST_EXTRACT_DIAGS,			"OFF"),
  DDui2_10(FAST_EXTRACT_IO_BUFFERS,             "6"),
  DDui___(FAST_EXTRACT_IO_TIMEOUT_SEC,          "60"),
  DDkwd__(FIND_COMMON_SUBEXPRS_IN_OR,		"ON"),

  DDui___(FLOAT_ESP_RANDOM_NUM_SEED,    "0"),

  DDkwd__(FORCE_BUSHY_CQS,			"ON"),

  DDkwd__(FORCE_PARALLEL_INSERT_SELECT,         "OFF"),
  DDkwd__(FORCE_PASS_ONE,		"OFF"),
  DDkwd__(FORCE_PASS_TWO,		"ON"),

 // Control if plan fragments need to be compressed
 //
  DDui___(FRAG_COMPRESSION_THRESHOLD,           "16"),

 // Controls FSO Tests for debug
 //
  DDui___(FSO_RUN_TESTS,                        "0"),

 // Controls use of Simple File Scan Optimizer
 // IF 0 - Use original "Complex" File Scan Optimizer.
 //        (in case simple causes problems)
 // IF 1 - Use logic to determine FSO to use. (default)
 // IF 2 - Use logic to determine FSO to use, but also use new
 //        executor predicate costing.
 // IF >2 - Always use new "Simple" File Scan Optimizer.
 //         (not recommended)
 //
  DDui___(FSO_TO_USE,                           "1"),

  // Disallow/Allow full outer joins in MultiJoin framework
  DDkwd__(FULL_OUTER_JOINS_SPOIL_JBB,     "OFF"),

  DDkwd__(GA_PROP_INDEXES_ARITY_1,         "ON"),

 // this default value is filled in
 // NADefaults::initCurrentDefaultsWithDefaultDefaults. The default value
 // is ON for static compiles and OFF for dynamic queries.
  DDkwd__(GENERATE_EXPLAIN,			"ON"),
  DDui___(GEN_CS_BUFFER_SIZE,			"0"),
  DDui___(GEN_CS_NUM_BUFFERS,			"0"),
  DDui___(GEN_CS_SIZE_DOWN,	                "4"),
  DDui___(GEN_CS_SIZE_UP,	                "4"),
  DDkwd__(GEN_DBLIMITS_LARGER_BUFSIZE,          "ON"),
  DDui1__(GEN_DDL_BUFFER_SIZE,			"30000"),
  DDui1__(GEN_DDL_NUM_BUFFERS,			"4"),
  DDui1__(GEN_DDL_SIZE_DOWN,			"2"),
  DDui1__(GEN_DDL_SIZE_UP,			"32"),
  DDui1__(GEN_DESC_BUFFER_SIZE,			"10240"),
  DDui1__(GEN_DESC_NUM_BUFFERS,			"4"),
  DDui1__(GEN_DESC_SIZE_DOWN,			"2"),
  DDui1__(GEN_DESC_SIZE_UP,			"16"),
  DDui1__(GEN_DP2I_BUFFER_SIZE,			"10000"),
  DDui1__(GEN_DP2I_NUM_BUFFERS,			"2"),
  DDui1__(GEN_DP2I_SIZE_DOWN,			"32"),
  DDui1__(GEN_DP2I_SIZE_UP,			"64"),
  DDui1__(GEN_DPSO_BUFFER_SIZE,			"10240"),
  DDui1__(GEN_DPSO_SIZE_DOWN,			"2048"),
  DDui1__(GEN_DPSO_SIZE_UP,			"2048"),
  DDui1__(GEN_DPUO_NUM_BUFFERS,			"4"),
  DDint__(GEN_EXCHANGE_MAX_MEM_IN_KB,           "4000"),
  DDint__(GEN_EXCHANGE_MSG_COUNT,               "80"),

  // Fast extract settings are for UDR method invocations
  DDui1__(GEN_FE_BUFFER_SIZE,                  "31000"),
  DDui1__(GEN_FE_NUM_BUFFERS,                  "2"),
  DDui1__(GEN_FE_SIZE_DOWN,                    "4"),
  DDui1__(GEN_FE_SIZE_UP,                      "4"),



  // Do not alter the buffer size; it must be 56K for SCRATCH_MGMT_OPTION == 5
  DDui1__(GEN_HGBY_BUFFER_SIZE,			"262144"),
  DDui1__(GEN_HGBY_NUM_BUFFERS ,		"5"),
  DDui1__(GEN_HGBY_PARTIAL_GROUP_FLUSH_THRESHOLD, "100"),
  DDui___(GEN_HGBY_PARTIAL_GROUP_ROWS_PER_CLUSTER, "0"),
  DDui1__(GEN_HGBY_SIZE_DOWN,			"2048"),
  DDui1__(GEN_HGBY_SIZE_UP,			"2048"),
  // Do not alter the buffer size; it must be 56K for SCRATCH_MGMT_OPTION == 5
  DDui1__(GEN_HSHJ_BUFFER_SIZE,			"262144"),

 // Controls use of the hash join min/max optimization.
  DDkwd__(GEN_HSHJ_MIN_MAX_OPT,			"OFF"),
  DDui1__(GEN_HSHJ_NUM_BUFFERS,			"1"),
  DDui1__(GEN_HSHJ_SIZE_DOWN,			"2048"),
  DDui1__(GEN_HSHJ_SIZE_UP,			"2048"),

  // Controls LeanEr Expression generation
  DDkwd__(GEN_LEANER_EXPRESSIONS,               "ON"),

  DDui1__(GEN_MEM_PRESSURE_THRESHOLD,		"10000"),
  DDui1__(GEN_MJ_BUFFER_SIZE,			"32768"),
  DDui1__(GEN_MJ_NUM_BUFFERS,			"1"),
  DDui1__(GEN_MJ_SIZE_DOWN,			"2"),
  DDui1__(GEN_MJ_SIZE_UP,			"2048"),
  DDui1__(GEN_ONLJ_BUFFER_SIZE,			"5120"),
  DDui1__(GEN_ONLJ_LEFT_CHILD_QUEUE_DOWN,       "4"),
  DDui1__(GEN_ONLJ_LEFT_CHILD_QUEUE_UP,         "2048"),
  DDui1__(GEN_ONLJ_NUM_BUFFERS,			"5"),
  DDui1__(GEN_ONLJ_RIGHT_SIDE_QUEUE_DOWN,       "2048"),
  DDui1__(GEN_ONLJ_RIGHT_SIDE_QUEUE_UP,         "2048"),
  DDkwd__(GEN_ONLJ_SET_QUEUE_LEFT,              "ON"),
  DDkwd__(GEN_ONLJ_SET_QUEUE_RIGHT,             "ON"),
  DDui1__(GEN_ONLJ_SIZE_DOWN,			"2048"),
  DDui1__(GEN_ONLJ_SIZE_UP,			"2048"),
  DDipcBu(GEN_PA_BUFFER_SIZE,			"31000"),
  DDui1__(GEN_PA_NUM_BUFFERS,			"5"),
  DDui1__(GEN_PROBE_CACHE_NUM_ENTRIES,          "16384"),// number of entries
  DDui___(GEN_PROBE_CACHE_NUM_INNER,            "0"), //0 means compiler decides  
  DDui1__(GEN_PROBE_CACHE_SIZE_DOWN,            "2048"),
  DDui1__(GEN_PROBE_CACHE_SIZE_UP,              "2048"),
  DDui1__(GEN_SAMPLE_SIZE_DOWN,			"16"),
  DDui1__(GEN_SAMPLE_SIZE_UP,			"16"),
  DDui1__(GEN_SEQFUNC_BUFFER_SIZE,		"10240"),
  DDui1__(GEN_SEQFUNC_NUM_BUFFERS,		"10"),
  DDui1__(GEN_SEQFUNC_SIZE_DOWN,		"512"),
  DDui1__(GEN_SEQFUNC_SIZE_UP,			"2048"),
  DDui1__(GEN_SGBY_BUFFER_SIZE,			"5120"),
  DDui1__(GEN_SGBY_NUM_BUFFERS,			"5"),
  DDui1__(GEN_SGBY_SIZE_DOWN,			"2048"),
  DDui1__(GEN_SGBY_SIZE_UP,			"2048"),
  DDui1__(GEN_SID_BUFFER_SIZE,			"1024"),
  DDui1__(GEN_SID_NUM_BUFFERS,			"4"),
  DDui1__(GEN_SNDB_NUM_BUFFERS,			"4"),
  DDui1__(GEN_SNDB_SIZE_DOWN,			"4"),
  DDui1__(GEN_SNDB_SIZE_UP,			"128"),
  DDui___(GEN_SNDT_BUFFER_SIZE_DOWN,		"0"),
  DDui___(GEN_SNDT_BUFFER_SIZE_UP,		"0"),
  DDui1__(GEN_SNDT_NUM_BUFFERS,			"2"),
  DDkwd__(GEN_SNDT_RESTRICT_SEND_BUFFERS,       "ON"),
  DDui1__(GEN_SNDT_SIZE_DOWN,			"4"),
  DDui1__(GEN_SNDT_SIZE_UP,			"128"),
  DDui1__(GEN_SORT_MAX_BUFFER_SIZE,		"5242880"),
  DDui1__(GEN_SORT_MAX_NUM_BUFFERS,             "160"),
  DDui1__(GEN_SORT_NUM_BUFFERS,			"2"),
  DDui1__(GEN_SORT_SIZE_DOWN,			"8"),
  DDui1__(GEN_SORT_SIZE_UP,			"1024"),
  DDkwd__(GEN_SORT_TOPN,		        "ON"),
  DDui1__(GEN_SORT_TOPN_THRESHOLD,              "10000"),
  DDui1__(GEN_SPLT_SIZE_UP,			"2048"),
  DDui1__(GEN_TFLO_BUFFER_SIZE,			"5120"),
  DDui1__(GEN_TFLO_NUM_BUFFERS,			"2"),
  DDui1__(GEN_TFLO_SIZE_DOWN,			"8"),
  DDui1__(GEN_TFLO_SIZE_UP,			"16"),

  DDui512(GEN_TIMEOUT_BUFFER_SIZE,		"4096"),
  DDui1__(GEN_TIMEOUT_NUM_BUFFERS,		"1"),
  DDui2__(GEN_TIMEOUT_SIZE_DOWN,		"2"),
  DDui2__(GEN_TIMEOUT_SIZE_UP,			"4"),

  DDui1__(GEN_TRAN_BUFFER_SIZE,			"4096"),
  DDui1__(GEN_TRAN_NUM_BUFFERS,			"1"),
  DDui1__(GEN_TRAN_SIZE_DOWN,			"2"),
  DDui1__(GEN_TRAN_SIZE_UP,			"4"),
  DDui1__(GEN_TRSP_BUFFER_SIZE,			"10240"),
  DDui1__(GEN_TRSP_NUM_BUFFERS,			"5"),
  DDui1__(GEN_TRSP_SIZE_DOWN,			"2048"),
  DDui1__(GEN_TRSP_SIZE_UP,			"2048"),
  DDui1__(GEN_TUPL_BUFFER_SIZE,			"1024"),
  DDui1__(GEN_TUPL_NUM_BUFFERS,			"4"),
  DDui1__(GEN_TUPL_SIZE_DOWN,			"2048"),
  DDui1__(GEN_TUPL_SIZE_UP,			"2048"),

  // GEN_UDRRS_ settings are for stored procedure result
  // set proxy plans
  DDui1__(GEN_UDRRS_BUFFER_SIZE,                "31000"),
  DDui1__(GEN_UDRRS_NUM_BUFFERS,                "2"),
  DDui1__(GEN_UDRRS_SIZE_DOWN,                  "4"),
  DDui1__(GEN_UDRRS_SIZE_UP,                    "128"),

  // GEN_UDR_ settings are for UDR method invocations
  DDui1__(GEN_UDR_BUFFER_SIZE,                  "31000"),
  DDui1__(GEN_UDR_NUM_BUFFERS,                  "2"),
  DDui1__(GEN_UDR_SIZE_DOWN,                    "4"),
  DDui1__(GEN_UDR_SIZE_UP,                      "4"),

  DDui1__(GEN_UN_BUFFER_SIZE,			"10240"),
  DDui1__(GEN_UN_NUM_BUFFERS,			"5"),
  DDui1__(GEN_UN_SIZE_DOWN,			"2048"),
  DDui1__(GEN_UN_SIZE_UP,			"2048"),


  // When less or equal to this CQD (5000 rows by default), a partial root 
  // will be running in the Master. Set to 0 to disable the feature.
  DDint__(GROUP_BY_PARTIAL_ROOT_THRESHOLD,	"5000"),
  DDkwd__(GROUP_BY_PUSH_TO_BOTH_SIDES_OF_JOIN,    "ON"),

  DDkwd__(GROUP_OR_ORDER_BY_EXPR,		"ON"),

  // HASH_JOINS ON means do HASH_JOINS
 XDDkwd__(HASH_JOINS,				"ON"),

  // HBase defaults
  // Some of the more important ones:

  // HBASE_CATALOG:                Catalog of "_ROW_" and "_CELL_" schemas
  // HBASE_COPROCESSORS:           Enable use of co-processors for aggregates.
  //                               need to set the coprocessor in HBase config file
  // HBASE_ESTIMATE_ROW_COUNT_VIA_COPROCESSOR:  If ON, use a coprocessor when
  //                               estimating row counts; if OFF, use client side
  //                               code (the latter doesn't work if HBase encryption
  //                               is being used)
  // HBASE_INTERFACE:              JNI or JNI_TRX (transactional interface)
  // HBASE_MAX_COLUMN_xxx_LENGTH:  Max length of some
  //                               string columns in the "_ROW_" and "_CELL_" schemas
  // HBASE_SQL_IUD_SEMANTICS:      Off: Don't check for existing rows for insert/update

  DDkwd__(HBASE_ASYNC_DROP_TABLE,		"OFF"),
  DDkwd__(HBASE_ASYNC_OPERATIONS,		"ON"),
 // HBASE_CACHE_BLOCKS, ON => cache every scan, OFF => cache no scan
 // SYSTEM => cache scans which take less than 1 RS block cache mem.
 DDui___(HBASE_BLOCK_SIZE,                      "65536"),
 DDkwd__(HBASE_CACHE_BLOCKS,		"SYSTEM"),
  DD_____(HBASE_CATALOG,                        "HBASE"),
  DDkwd__(HBASE_CHECK_AND_UPDEL_OPT,		"ON"),

 DDkwd__(HBASE_COMPRESSION_OPTION,		             ""),

 DDkwd__(HBASE_COPROCESSORS,		             "ON"),


 DDkwd__(HBASE_DATA_BLOCK_ENCODING_OPTION,		             ""),

 // If set to 'OFF' we get a stub cost of 1 for delete operations.
 // We can remove this once the delete costing code has broader
 // exposure.
 DDkwd__(HBASE_DELETE_COSTING,		             "ON"),
 DDflt0_(HBASE_DOP_PARALLEL_SCANNER,             "0."),
 DDkwd__(HBASE_ESTIMATE_ROW_COUNT_VIA_COPROCESSOR,   "OFF"),
 DDkwd__(HBASE_FILTER_PREDS,		             "OFF"),
 DDkwd__(HBASE_HASH2_PARTITIONING,                   "ON"),
 DDui___(HBASE_INDEX_LEVEL,                          "0"),
 DDui___(HBASE_MAX_COLUMN_INFO_LENGTH,                "10000"),
 DDui___(HBASE_MAX_COLUMN_NAME_LENGTH,               "100"),
 DDui___(HBASE_MAX_COLUMN_VAL_LENGTH,                  "1000"),
 DDui___(HBASE_MAX_ESPS,                        "9999"),
 DDui___(HBASE_MAX_NUM_SEARCH_KEYS,                  "512"),
 DDkwd__(HBASE_MEMSTORE_FLUSH_SIZE_OPTION,              ""),
 DDui1__(HBASE_MIN_BYTES_PER_ESP_PARTITION,     "67108864"),

  DDkwd__(HBASE_NATIVE_IUD,		"ON"),
 DDui1__(HBASE_NUM_CACHE_ROWS_MAX,	"1024"),
 DDui1__(HBASE_NUM_CACHE_ROWS_MIN,	"100"),

  DDkwd__(HBASE_RANGE_PARTITIONING,	        "ON"),
  DDkwd__(HBASE_RANGE_PARTITIONING_MC_SPLIT,	"ON"),
  DDkwd__(HBASE_RANGE_PARTITIONING_PARTIAL_COLS,"ON"),
 DDui___(HBASE_REGION_SERVER_MAX_HEAP_SIZE,     "1024"), // in units of MB

  DDkwd__(HBASE_ROWSET_VSBB_OPT,		"ON"),
  DDusht_(HBASE_ROWSET_VSBB_SIZE,        	"1024"),
  DDflt0_(HBASE_SALTED_TABLE_MAX_FILE_SIZE,	"0"),
  DDkwd__(HBASE_SALTED_TABLE_SET_SPLIT_POLICY,	"ON"),
 DDkwd__(HBASE_SERIALIZATION,		"ON"),
 
  DD_____(HBASE_SERVER,                         ""), 
  DDkwd__(HBASE_SMALL_SCANNER,      "OFF"),
  DDkwd__(HBASE_SQL_IUD_SEMANTICS,		"ON"),
  DDkwd__(HBASE_STATS_PARTITIONING,           	"ON"),

  // If set to 'OFF' we get a stub cost of 0 for update operations.
  DDkwd__(HBASE_UPDATE_COSTING,		             "ON"),

  DDkwd__(HBASE_UPDEL_CURSOR_OPT,		"ON"),
  DDui___(HBASE_USE_FAKED_REGIONS,		"0"),
  DD_____(HBASE_ZOOKEEPER_PORT,                 ""),

  DDui1__(HDFS_IO_BUFFERSIZE,                            "65536"),
  DDui___(HDFS_IO_BUFFERSIZE_BYTES,               "0"),
  DDui___(HDFS_IO_INTERIM_BYTEARRAY_SIZE_IN_KB,    "1024"),
  // The value 0 denotes RangeTail = max record length of table.
  DDui___(HDFS_IO_RANGE_TAIL,                     "0"),
  DDkwd__(HDFS_PREFETCH,                           "ON"),
  DDkwd__(HDFS_READ_CONTINUE_ON_ERROR,                           "OFF"),
  DDui1__(HDFS_REPLICATION,                            "1"),
  DDkwd__(HDFS_USE_CURSOR_MULTI,                    "OFF"),

  DDkwd__(HGB_BITMUX,                           "OFF"),
  DDflt0_(HGB_CPUCOST_INITIALIZE,		"1."),
  DDflt0_(HGB_DP2_MEMORY_LIMIT,			"10000."),
  DDflte_(HGB_GROUPING_FACTOR_FOR_SPILLED_CLUSTERS,	".5"),
  DDflte_(HGB_MAX_TABLE_SIZE_FOR_CLUSTERS,      "4E5"),
  DDflte_(HGB_MEMORY_AVAILABLE_FOR_CLUSTERS,    "10"),
  DDflte_(HH_OP_ALLOCATE_HASH_TABLE,		".05"),
  DDflt1_(HH_OP_HASHED_ROW_OVERHEAD,		"8."),

  // From observation:
  // 03/11/98: probing the hash table is very inexpensive,
  // thus reduce this to almost zero.
  // change
  // CPUCOST_HASH_PER_KEY
  // when changing this value
  // It takes around 2 seconds to insert 100,000 rows into the chain:
  // @ 0.00005 secs per k instr:
  // k instr= 2/0.00005/100000 = 0.4
  // History:
  // Before 03/11/98: 0.4
  // Initially: 0.01
  DDflte_(HH_OP_INSERT_ROW_TO_CHAIN,		"0.51"),

  // From observation:
  // 03/11/98: probing the hash table is very inexpensive,
  // thus reduce this to almost zero.
  // 01/05/98: 15,433 rows probed per second @ 0.00005 seconds
  // per thousand of instructions, give:
  // seconds to probe one row = 1/15,433 = 0.000064796
  // This time includes: time to position and to compare. Thus
  // subtract the time to compare to arrive to the proper number:
  // thd. of instructions per row inserted =
  // 1/15,433/0.00005 - CPUCOST_COMPARE_SIMPLE_DATA_TYPE =
  // 1.2959 - 0.2 = 1.0959
  // History:
  // Before 03/11/98: 1.0959
  // Before 01/05/98: 0.01
  DDflt0_(HH_OP_PROBE_HASH_TABLE,		"0.011"),

  // Added 10/16/02
  DDkwd__(HIDE_INDEXES,                          "NONE"),

  DDansi_(HISTOGRAMS_SCHEMA,                     ""),

  // -------------------------------------------------------------------------
  // Histogram fudge factors
  // -------------------------------------------------------------------------
//HIST_BASE_REDUCTION and HIST_PREFETCH externalized 08/21/01 CR 10-010713-3895

  DDkwd__(HIST_ASSUME_INDEPENDENT_REDUCTION,      "ON"),
  DDflt0_(HIST_BASE_REDUCTION_FUDGE_FACTOR, "0.1"),
  DDflt0_(HIST_CONSTANT_ALPHA,              "0.5"),
  DDflt_0_1(HIST_DEFAULT_BASE_SEL_FOR_LIKE_WILDCARD,    "0.50"),
  DDui1__(HIST_DEFAULT_NUMBER_OF_INTERVALS, "50"),
  DDui1__(HIST_DEFAULT_SAMPLE_MAX, "1000000"),
  DDui1__(HIST_DEFAULT_SAMPLE_MIN,   "10000"),
  DDflt_0_1(HIST_DEFAULT_SAMPLE_RATIO, "0.01"),
  DDflte_(HIST_DEFAULT_SEL_FOR_BOOLEAN,		"0.3333"),
  DDflt_0_1(HIST_DEFAULT_SEL_FOR_IS_NULL,		"0.01"),
  DDflt_0_1(HIST_DEFAULT_SEL_FOR_JOIN_EQUAL,	"0.3333"),
  DDflt_0_1(HIST_DEFAULT_SEL_FOR_JOIN_RANGE,	"0.3333"),
  DDflt_0_1(HIST_DEFAULT_SEL_FOR_LIKE_NO_WILDCARD,"1.0"),
  DDflt_0_1(HIST_DEFAULT_SEL_FOR_LIKE_WILDCARD,	"0.10"),
  DDflt_0_1(HIST_DEFAULT_SEL_FOR_PRED_EQUAL,	"0.01"),
  DDflt_0_1(HIST_DEFAULT_SEL_FOR_PRED_RANGE,	"0.3333"),

  // control the amount of data in each partition of the persistent sample tble.
  DDflt1_(HIST_FETCHCOUNT_SCRATCH_VOL_THRESHOLD, "10240000"),

  DDkwd__(HIST_FREQ_VALS_NULL_FIX,              "ON"),
  DDkwd__(HIST_INCLUDE_SKEW_FOR_NON_INNER_JOIN,      "ON"),
  DDkwd__(HIST_INTERMEDIATE_REDUCTION,      "OFF"),
  DDflt0_(HIST_INTERMEDIATE_REDUCTION_FUDGE_FACTOR, "0.25"),
  DDflt_0_1(HIST_JOIN_CARD_LOWBOUND,		"1.0"),
  DDui1__(HIST_LOW_UEC_THRESHOLD,       "55"),
  DDui1__(HIST_MAX_NUMBER_OF_INTERVALS,		"10000"),

  DDkwd__(HIST_MC_STATS_NEEDED,                 "ON"),
  DDkwd__(HIST_MERGE_FREQ_VALS_FIX,            "ON"),

  // Histogram min/max optimization: when the predicate is of form
  // T.A = MIN/MAX(S.B), replace the histogram(T.A) with
  // single_int_histogram(MIN/MAX(S.B)). Do this only when
  // there is no local predicate on S and there exists a frequent
  // value that is equals to MIN/MAX(S.B).
  DDkwd__(HIST_MIN_MAX_OPTIMIZATION,                 "ON"),

  // This CQD is used to control the number of missing stats warnings
  // that should be generated.
  // 0 ? Display no warnings.
  // 1 ? Display only missing single column stats warnings. These include 6008 and 6011
  // 2 ? Display all single column missing stats warnings and
  //     multi-column missing stats warnings for Scans only.
  // 3 ? Display all missing single column stats warnings and missing
  //     multi-column stats warnings for Scans and Join operators only..
  // 4 ? Display all missing single column stats and missing multi-column
  //     stats warnings for all operators including Scans, Joins and GroupBys.
  // The CQD also does not have an impact on the auto update stats behavior. The stats will
  // still be automatically generated even if the warnings have been suppressed.
  // USTAT_AUTO_MISSING_STATS_LEVEL.
  // Default behavior is to generate all warnings
  XDDui___(HIST_MISSING_STATS_WARNING_LEVEL,	"4"),

  DDflt1_(HIST_NO_STATS_ROWCOUNT,               "100"),
  DDflt1_(HIST_NO_STATS_UEC,                    "2"),
  DDflt1_(HIST_NO_STATS_UEC_CHAR1,              "10"),
  DDui1__(HIST_NUM_ADDITIONAL_DAYS_TO_EXTRAPOLATE, "4"),
  DDintN1__(HIST_ON_DEMAND_STATS_SIZE,            "0"),
  DDui___(HIST_OPTIMISTIC_CARD_OPTIMIZATION,    "1"),
 XDDkwd__(HIST_PREFETCH,                        "ON"),
 XDDkwd__(HIST_REMOVE_TRAILING_BLANKS,          "ON"), // should remove after verifying code is solid
  DDansi_(HIST_ROOT_NODE,                            ""),
 XDDflt1_(HIST_ROWCOUNT_REQUIRING_STATS,        "500"),
  DD_____(HIST_SCRATCH_VOL,                     ""),
  // control the amount of data in each partition of the sample tble.
  DDflt1_(HIST_SCRATCH_VOL_THRESHOLD,           "10240000"),
  DDflt_0_1(HIST_SKEW_COST_ADJUSTMENT,            "0.2"),
  DDkwd__(HIST_SKIP_MC_FOR_NONKEY_JOIN_COLUMNS,   "OFF"),
  DDui___(HIST_TUPLE_FREQVAL_LIST_THRESHOLD,     "40"),
  DDkwd__(HIST_USE_HIGH_FREQUENCY_INFO,          "ON"),
 XDDkwd__(HIST_USE_SAMPLE_FOR_CARDINALITY_ESTIMATION ,      "ON"), 

 // CQDs for Trafodion on Hive
 // Main ones to use:
 // HIVE_MAX_STRING_LENGTH_IN_BYTES: Hive "string" data type gets converted
 //                                  into a VARCHAR with this length
 // HIVE_MIN_BYTES_PER_ESP_PARTITION: Make one ESP for this many bytes
 // HIVE_NUM_ESPS_PER_DATANODE: Equivalent of MAX_ESPS_PER_CPU_PER_OP
 //                             Note that this is really per SeaQuest node

  DD_____(HIVE_CATALOG,                                ""),

  DDkwd__(HIVE_CREATE_TABLE_LIKE_PARTITION_NO_NULL, "OFF"),
  DDkwd__(HIVE_CTAS_IN_NATIVE_MODE,             "OFF"),

  DDkwd__(HIVE_DATA_MOD_CHECK,                  "ON"),

  DDkwd__(HIVE_DEFAULT_CHARSET,            (char *)SQLCHARSETSTRING_UTF8),
  DD_____(HIVE_DEFAULT_SCHEMA,                  "HIVE"),
  DD_____(HIVE_FILE_CHARSET,                    ""),
  DD_____(HIVE_HDFS_STATS_LOG_FILE,             ""),
  DDui___(HIVE_INSERT_ERROR_MODE,               "1"),
  DDint__(HIVE_LIB_HDFS_PORT_OVERRIDE,          "-1"),
  DDint__(HIVE_LOCALITY_BALANCE_LEVEL,          "0"),
  DDui___(HIVE_MAX_BINARY_LENGTH,               "128"), 
  DDui___(HIVE_MAX_ESPS,                        "9999"),
  DDui___(HIVE_MAX_STRING_LENGTH_IN_BYTES,      "32000"),
  DDint__(HIVE_METADATA_REFRESH_INTERVAL,       "0"),
  DDflt0_(HIVE_MIN_BYTES_PER_ESP_PARTITION,     "67108864"),
  DDkwd__(HIVE_NO_REGISTER_OBJECTS,             "OFF"),
  DDui___(HIVE_NUM_ESPS_PER_DATANODE,           "2"),
  DDpct__(HIVE_NUM_ESPS_ROUND_DEVIATION,        "34"),
  DDint__(HIVE_SCAN_SPECIAL_MODE,                "0"),
  DDkwd__(HIVE_SORT_HDFS_HOSTS,                 "ON"),
  DDkwd__(HIVE_USE_EXT_TABLE_ATTRS,             "ON"),
  DD_____(HIVE_USE_FAKE_SQ_NODE_NAMES,          "" ),
  DDkwd__(HIVE_USE_FAKE_TABLE_DESC,             "OFF"),
  DDkwd__(HIVE_USE_HASH2_AS_PARTFUNCION,        "ON"),
  DDkwd__(HIVE_VIEWS,                           "ON"),

 // -------------------------------------------------------------------------

  DDflt0_(HJ_CPUCOST_INITIALIZE,		"1."),
  DDui1__(HJ_INITIAL_BUCKETS_PER_CLUSTER,	"4."),
  DDkwd__(HJ_NEW_MCSB_PLAN,			"OFF"),
  DDint__(HJ_SCAN_TO_NJ_PROBE_SPEED_RATIO,        "2000"),
  DDkwd__(HJ_TYPE,				"HYBRID"),
  DDkwd__(HQC_CONVDOIT_DISABLE_NUMERIC_CHECK, "OFF"),
  DDkwd__(HQC_LOG, "OFF"),
  DD_____(HQC_LOG_FILE,    ""),
  DDui1_10(HQC_MAX_VALUES_PER_KEY, "5"), 
  DDkwd__(HYBRID_QUERY_CACHE, "ON"),
  DDkwd__(IF_LOCKED,				"WAIT"),



  DDkwd__(IMPLICIT_DATETIME_INTERVAL_HOSTVAR_CONVERSION,   "FALSE"),
  DDkwd__(IMPLICIT_HOSTVAR_CONVERSION,		"FALSE"),

  // threshold for the number of rows inserted into a volatile/temp
  // table which will cause an automatic update stats.
  // -1 indicates do not upd stats. 0 indicates always upd stats.
  DDint__(IMPLICIT_UPD_STATS_THRESHOLD,   "-1"), //"10000"),

  DDkwd__(INCORPORATE_SKEW_IN_COSTING,		  "ON"),
  DDkwd__(INDEX_ELIMINATION_LEVEL,              "AGGRESSIVE"),
  DDui1__(INDEX_ELIMINATION_THRESHOLD,          "50"),
  DDkwd__(INDEX_HINT_WARNINGS,                  "ON"),
 SDDkwd__(INFER_CHARSET,			"OFF"),

  // UDF initial row cost CQDs
  DDui___(INITIAL_UDF_CPU_COST,                  "100"),
  DDui___(INITIAL_UDF_IO_COST,                   "1"),
  DDui___(INITIAL_UDF_MSG_COST,                  "2"),
  DDkwd__(INPUT_CHARSET,            (char *)SQLCHARSETSTRING_ISO88591), // SQLCHARSETSTRING_UTF8
 XDDkwd__(INSERT_VSBB,				"SYSTEM"),
 //10-040621-7139-begin
 //This CDQ will alllow the user to force the compiler to
 //choose an interactive access path. ie., prefer access path with
 //index in it. If such a path is not found which ever access path is
 //available is chosen.
  DDkwd__(INTERACTIVE_ACCESS,		"OFF"),
 //10-040621-7139-end

  DDkwd__(IN_MEMORY_OBJECT_DEFN,                "OFF"),

  // History:
  // 3/11/99 Changed to zero because in large tables the read-ahead
  //         seems negligible (and/or hard to simulate)
  // Before 3/11/99: 0.58
 DDflt0_(IO_TRANSFER_COST_PREFETCH_MISSES_FRACTION,	"0."),
 XDDkwd__(ISOLATION_LEVEL,			"READ_COMMITTED"),
XDDkwd__(ISOLATION_LEVEL_FOR_UPDATES,		"NONE"),
SDDkwd__(ISO_MAPPING,           (char *)SQLCHARSETSTRING_ISO88591),

 DDkwd__(IS_DB_TRANSPORTER,                     "OFF"),

 DDkwd__(IS_SQLCI,				"FALSE"),

 DDkwd__(IUD_NONAUDITED_INDEX_MAINT,           "OFF"),

  DDkwd__(JDBC_PROCESS,				"FALSE"),

  // Force the join order given by the user
 XDDkwd__(JOIN_ORDER_BY_USER,			"OFF"),

  DDkwd__(KEYLESS_NESTED_JOINS,                 "OFF"),

 XDDkwd__(LAST0_MODE,				"OFF"),

  DDansi_(LDAP_USERNAME, ""),

  // Disallow/Allow left joins in MultiJoin framework
  DDkwd__(LEFT_JOINS_SPOIL_JBB,        "OFF"),

  DDkwd__(LIMIT_HBASE_SCAN_DOP,                 "OFF"),

  // if this default is set to ON, then the max precision of a numeric
  // expression(arithmetic, aggregate) is limited to MAX_NUMERIC_PRECISION
  // (= 18). If this is set to OFF, the default value, then the max precision
  // is computed based on the operands and the operation which could make the
  // result a software datatype(BIGNUM). Software datatypes give better
  // precision but degraded performance.
  SDDkwd__(LIMIT_MAX_NUMERIC_PRECISION,		"SYSTEM"),

 // Size in MB  used to perform garbage collection  to lob data file 
  // Recommended size is 20GB   . Change to adjust disk usage. If -1 it means
 // don't do GC during insert/update operations. 
  DDint__(LOB_GC_LIMIT_SIZE,            "-1"),
  
  DDint__(LOB_HDFS_PORT,                       "0"),
  DD_____(LOB_HDFS_SERVER,                 "default"), 
 // For JDBC/ODBC batch operations, LOB  size limited to 4K bytes
  DDint__(LOB_INPUT_LIMIT_FOR_BATCH,  "16384"),
 // Control the locking via RMS shared lock. This ensures the CLI and HDFS 
 // operations for any LOB UID are done under a lock so concurrent operations 
 // wont conflict and cause incosistent data. For non concurrent applications, 
 // we can turn this off as a performance enhancement. 
  DDkwd__(LOB_LOCKING,          "OFF"),
   // Size of memoryin Megabytes  used to perform I/O to lob data file 
  // default size is 128MB   . Change to adjust memory usage. 
  DDint__(LOB_MAX_CHUNK_MEM_SIZE,            "128"), 
  // default size is 5 G  (5120 M)
  DDint__(LOB_MAX_SIZE,                         "5120"),
  // default size is 32000bytes. Change this to extract more data into memory.
  DDui___(LOB_OUTPUT_SIZE,                         "32000"),

  DD_____(LOB_STORAGE_FILE_DIR,                 "/user/trafodion/lobs"), 

  // storage types defined in exp/ExpLOBenum.h. 
  // Default is hdfs_file (value = 2)
  DDint__(LOB_STORAGE_TYPE,                     "2"),

  //New default size for buffer size for local node
  DDui2__(LOCAL_MESSAGE_BUFFER_SIZE,		"50"),

  DDansi_(MAINTAIN_CATALOG,			"NEO"),

  // Set the maintain control table timeout to 5 minutes
  DDint__(MAINTAIN_CONTROL_TABLE_TIMEOUT,       "30000"),


  DDkwd__(MARIAQUEST_PROCESS,                   "OFF"),

  DDSint__(MASTER_PRIORITY,                     "0"),
  DDSint__(MASTER_PRIORITY_DELTA,               "0"),

  DDint__(MATCH_CONSTANTS_OF_EQUALITY_PREDICATES, "2"),

  DDui1__(MAX_ACCESS_NODES_PER_ESP,	"1024"),


  DDint__(MAX_DEPTH_TO_CHECK_FOR_CYCLIC_PLAN,   "1"),

  // default value of maximum dp2 groups for a hash-groupby
  DDui1__(MAX_DP2_HASHBY_GROUPS, "1000"),

  //
  // The max number of ESPs per cpu for a given operator.
  // i.e. this number times the number of available CPUs is "max pipelines".
  // 
  // On Linux, "CPU" means cores.
  //
  DDflt__(MAX_ESPS_PER_CPU_PER_OP,	"0.5"),

  DDui1__(MAX_EXPRS_USED_FOR_CONST_FOLDING,  "1000"),

  // used in hash groupby costing in esp/master
  DDui1__(MAX_HEADER_ENTREIS_PER_HASH_TABLE, "250000"),

  DDui1__(MAX_LONG_VARCHAR_DEFAULT_SIZE,	"2000"),
  DDui1__(MAX_LONG_WVARCHAR_DEFAULT_SIZE,   "2000"),

  DD18_128(MAX_NUMERIC_PRECISION_ALLOWED,        "128"),



  // The max number of skewed values detected - skew buster
  DDui1__(MAX_SKEW_VALUES_DETECTED,             "10000"),

  // multi-column skew inner table broadcast threashold in bytes (=1 MB)
  DDui___(MC_SKEW_INNER_BROADCAST_THRESHOLD,    "1000000"),

  // multi-column skew sensitivity threshold
  //
  // For new MCSB (that is, we utilize MC skews directly),
  //  apply the MC skew buster when
  //    frequency of MC skews > MC_SKEW_SENSITIVITY_THRESHOLD / count_of_cpus
  //
  // For old MCSB (that is, we guess MC skews from SC skews),
  //  apply the MC skew buster when
  //   SFa,b... * countOfPipeline > MC_SKEW_SENSITIVITY_THRESHOLD
  //   SFa,b ... is the skew factor for multi column a,b,...
  //
  XDDflt__(MC_SKEW_SENSITIVITY_THRESHOLD,        "0.1"),

  // Applies additional heuristics to turn off MDAM. This code was added
  // at a time when cumulative probe cost was not considered in the MDAM
  // cost model (a MAJOR error). The meaning of the values:
  // 0 - do nothing (no additional heuristics)
  // 1 - require that more key columns have predicates than key columns
  //     without predicates
  // 2 - require that the UECs of key columns lacking predicates is below
  //     a given threshold
  // 3 - AND of 1 and 2
  // These heuristics made sense when the cost model was broken and we
  // traversed more deeply into the key than we should have. The cost model
  // now takes cumulative probe cost into account so these should no longer
  // be necessary (and indeed these heuristics can cause us to miss some
  // very good plans, because they prevent us from considering MDAM plans
  // that probe prefix columns before a column with high UEC, for example).
  DDui___(MDAM_APPLY_RESTRICTION_CHECK,	            "0"),

  // MDAM_COSTING_REWRITE: If ON, then the simple cost model costing
  // for MDAM will use the rewritten code. If OFF, it will use the
  // older code (which was current in September 2017, at the time
  // the rewrite commenced). This only has an effect if SIMPLE_COST_MODEL
  // is ON (which is the current default).
  XDDkwd__(MDAM_COSTING_REWRITE,		"OFF"),


  DDflt0_(MDAM_CPUCOST_NET_OVH,			"2000."),


  // The cost that takes to build the mdam network per predicate:
  // (we assume that the cost to build the mdam network is a linear function
  // of the key predicates)
  DDflt0_(MDAM_CPUCOST_NET_PER_PRED,		".5"),

  // Added by JIRA TRAFODION-2765: Allows consideration of MDAM
  // in more general circumstances.
  XDDkwd__(MDAM_FSO_SIMPLE_RULE,		"ON"),

  // controls the max. number of seek positions under which MDAM will be
  // allowed. Set it to 0 turns off the feature.
  XDDui___(MDAM_NO_STATS_POSITIONS_THRESHOLD,       "10"),

  // a multiplier of probe cost used for MDAM
  DDflt0_(MDAM_PROBE_TAX,			"3"),

  // MDAM_SCAN_METHOD ON means MDAM is enabled,
  // OFF means MDAM is disabled. MDAM is enabled by default
  // externalized 06/21/01 RV
 // mdam off on open source at this point
  XDDkwd__(MDAM_SCAN_METHOD,			"ON"),

  DDflt0_(MDAM_SELECTION_DEFAULT,		"8.0"),

  // Overhead charge for a subset in the rewritten MDAM costing code
  DDflt0_(MDAM_SUBSET_FACTOR,                   "8.0"),

  DDflt0_(MDAM_TOTAL_UEC_CHECK_MIN_RC_THRESHOLD, "10000"),
  DDflt0_(MDAM_TOTAL_UEC_CHECK_UEC_THRESHOLD,	 "0.2"),

  DDkwd__(MDAM_TRACING,			        "OFF"),

  // controls the max. number of probes at which MDAM under NJ plan will be
  // generated. Set it to 0 turns off the feature.
  XDDui___(MDAM_UNDER_NJ_PROBES_THRESHOLD,       "0"),

  // controls the amount of penalty for CPU resource required that is
  // beyond the value specified by MDOP_CPUS_SOFT_LIMIT. The number of extra CPUs
  // actually allocated is computed as the origial value divided by the CQD.
  // If the CQD is set to 1 (default), then there is no penalty.
  DDflt1_(MDOP_CPUS_PENALTY,      "70"),

  // specify the limit beyond which the number of CPUs will be limited.
  DDui1__(MDOP_CPUS_SOFT_LIMIT,   "64"),

  // controls the amount of penalty for CPU resource per memory unit
  // required that is beyond the value specified by MDOP_CPUS_SOFT_LIMIT.
  // The number of extra CPUs actually allocated is computed as the
  // origial value divided by the CQD.
  DDflt1_(MDOP_MEMORY_PENALTY,    "70"),

  // CQD to test/enforce heap memory upper limits
  // values are in KB
  DDui___(MEMORY_LIMIT_CMPCTXT_UPPER_KB,       "0"),
  DDui___(MEMORY_LIMIT_CMPSTMT_UPPER_KB,       "0"),
  DDui___(MEMORY_LIMIT_HISTCACHE_UPPER_KB,     "0"),
  DDui___(MEMORY_LIMIT_NATABLECACHE_UPPER_KB,  "0"),
  DDui___(MEMORY_LIMIT_QCACHE_UPPER_KB,        "0"),
  // Checked at compile time. Set to -1 to disable check.
  // Value should be >= EXE_MEMORY_FOR_UNPACK_ROWS_IN_MB
  DDint__(MEMORY_LIMIT_ROWSET_IN_MB,         "1024"),

  // SQL/MX Compiler/Optimzer Memory Monitor.
  DDkwd__(MEMORY_MONITOR,			"OFF"),
  DDui1__(MEMORY_MONITOR_AFTER_TASKS,         "30000"),
  DDkwd__(MEMORY_MONITOR_IN_DETAIL,		"OFF"),
  DD_____(MEMORY_MONITOR_LOGFILE,	       "NONE"),
  DDkwd__(MEMORY_MONITOR_LOG_INSTANTLY,	        "OFF"),
  DDui1__(MEMORY_MONITOR_TASK_INTERVAL,        "5000"),

  // Hash join currently uses 20 Mb before it overflows, use this
  // as the limit
  DDui1__(MEMORY_UNITS_SIZE,			"20480"),

  // amount of memory available per CPU for any query
 SDDflte_(MEMORY_UNIT_ESP,      "300"),

  DDflt1_(MEMORY_USAGE_NICE_CONTEXT_FACTOR,	"1"),
  DDflt1_(MEMORY_USAGE_OPT_PASS_FACTOR,		"1.5"),
  DDui1__(MEMORY_USAGE_SAFETY_NET,              "500"),

  // MERGE_JOINS ON means do MERGE_JOINS
 XDDkwd__(MERGE_JOINS,				"ON"),
  DDkwd__(MERGE_JOIN_ACCEPT_MULTIPLE_NJ_PROBES,	"ON"),
  DDkwd__(MERGE_JOIN_CONTROL,			"OFF"),
  DDkwd__(MERGE_JOIN_WITH_POSSIBLE_DEADLOCK, "OFF"),

  // controls if merge/upsert is supported on table with a unique index
  DDkwd__(MERGE_WITH_UNIQUE_INDEX,   "ON"),

 SDDui___(METADATA_CACHE_SIZE,    "20"),
  //-------------------------------------------------------------------
  // Minimum ESP parallelism. If the user does not specify this value
  // (default value 0 does not change) then the number of segments
  // (totalNumCPUs/16, where totalNumCPUs=gpClusterInfo->numOfSMPs())
  // will be used as the value of minimum ESP parallelism. If user sets
  // this value it should be integer between 1 and totalNumCPUs. In
  // this case actual value of minimum ESP parallelism will be
  // min(CDQ value, MDOP), where MDOP (maximum degree of parallelism)
  // is defined by adaptive segmentation
  //-------------------------------------------------------------------
  DDui___(MINIMUM_ESP_PARALLELISM,	        "0"),
  DDui1__(MIN_LONG_VARCHAR_DEFAULT_SIZE,	"1"),
  DDui1__(MIN_LONG_WVARCHAR_DEFAULT_SIZE,	"1"),
  DDkwd__(MIN_MAX_OPTIMIZATION,			"ON"),
  DDpct__(MJ_BMO_QUOTA_PERCENT,			"0"),
  DDflt0_(MJ_CPUCOST_CLEAR_LIST,		".01"),
  DDflt0_(MJ_CPUCOST_GET_NEXT_ROW_FROM_LIST,	".01"),
  // calibrated 01/16/98:
  // 01/13/98 40000., this did not work with small tables
  // Before 01/13/98: 0.5
  DDflt0_(MJ_CPUCOST_INITIALIZE,		"1."),
  // Before 03/12/98: 0.4
  // Before 01/13/98: 0.01
  DDflt0_(MJ_CPUCOST_INSERT_ROW_TO_LIST,	".0001"),
  DDflt0_(MJ_CPUCOST_REWIND_LIST,		".01"),
  DDflte_(MJ_LIST_NODE_SIZE,			".01"),
  DDkwd__(MJ_OVERFLOW,				"ON"),

  DDkwd__(MODE_SEABASE,			    "ON"),
  DDkwd__(MODE_SEAHIVE,			    "ON"),

 SDDkwd__(MODE_SPECIAL_1,                       "OFF"),

  DDkwd__(MODE_SPECIAL_4,                       "OFF"),


  // Tests suggest that RELEASE is about 2.5 times faster than DEBUG
  // RELEASE is always faster than DEBUG code so this default must be
  // at least one.
  DDflt1_(MSCF_DEBUG_TO_RELEASE_MULTIPLIER,		"2.5"),

  // MSCF_ET_CPU units are seconds/thousand of CPU instructions
  // History:
  // Before 02/01/99, the speed was calibrated for debug, now its is for
  // release: 0.00005
  DDflte_(MSCF_ET_CPU,				"0.000014"),
  // was 0.00002 12/2k

  // MSCF_ET_IO_TRANSFER units are seconds/Kb
  // History
  // Changed to '0.000455' to reflect new calibration data
  // Before 03/11/99 "0.000283"
  DDflte_(MSCF_ET_IO_TRANSFER,                        "0.00002"),
  // Assume time to transfer a KB of local message is 5 times
  // faster than the time to transfer a KB from disk
  // Units of MSCF_ET_LOCAL_MSG_TRANSFER are seconds/Kb

  DDflte_(MSCF_ET_LOCAL_MSG_TRANSFER,		"0.000046"),
  // : for calibration on 04/08/2004
  // Seek time will be derived from disk type.

  // MSCF_ET_NUM_IO_SEEKS units are seconds
  DDflte_(MSCF_ET_NUM_IO_SEEKS,                       "0.0038"),

  // Assume sending a local message takes 1000 cpu instructions
  DDflte_(MSCF_ET_NUM_LOCAL_MSGS,		"0.000125"),
  // Assume sending a remote message takes 10000 cpu instructions
  //  DDflte_(MSCF_ET_NUM_REMOTE_MSGS,		"0.00125"),
  
  // Change the number of instructions to encode a remote message to be
  // the same as the local message
  DDflte_(MSCF_ET_NUM_REMOTE_MSGS,		"0.000125"),
  // Assume 1MB/second transfer rate for transferring remote message bytes
  // (Based on 10 Megabit/second Ethernet transfer rate)
  // MSCF_ET_REMOTE_MSG_TRANSFER units are kb/Sec
  //  DDflte_(MSCF_ET_REMOTE_MSG_TRANSFER,		"0.001"),
  
  // the remote msg are 10% more costly than the local transfer
  // but also may depend on the physical link, so externalize it
  DDflte_(MSCF_ET_REMOTE_MSG_TRANSFER,		"0.00005"),

  // -------------------------------------------------------------------------
  // Factors used for estimating overlappability of I/O and messaging used
  // in the calculation for overlapped addition
  // Assume 50% overlap for now.
  // -------------------------------------------------------------------------
  DDflte_(MSCF_OV_IO,				"0.5"),
  DDflte_(MSCF_OV_MSG,				"0.5"),



  DDkwd__(MTD_GENERATE_CC_PREDS,		"ON"),
  DDint__(MTD_MDAM_NJ_UEC_THRESHOLD,		"100"),

  // Allow for the setting of the row count in a long running operation
 XDDui1__(MULTI_COMMIT_SIZE,			"10000"),

  // try the join order specified in the queries, this will cause the
  // enumeration of the initial join order specified by the user
  // among the join orders enumerated
  // ** This is currently OFF by default **
  DDkwd__(MULTI_JOIN_CONSIDER_INITIAL_JOIN_ORDER,      "OFF"),

  // used in JBBSubsetAnalysis::isAStarPattern for finding lowest cost
  // outer subtree for NJ into fact table.
  DDflt0_(MULTI_JOIN_PROBE_HASH_TABLE,	       "0.000001"),


 SDDint__(MULTI_JOIN_THRESHOLD,			"3"),

  DDint__(MULTI_PASS_JOIN_ELIM_LIMIT,           "5"),

  DDflt0_(MU_CPUCOST_INITIALIZE,		".05"),
  DDui___(MU_INITIAL_BUFFER_COUNT,		"5."),
  DDflte_(MU_INITIAL_BUFFER_SIZE,		"1033.7891"),

  //--------------------------------------------------------------------------
  //++ MV
 XDDkwd__(MVGROUP_AUTOMATIC_CREATION,           "ON"),

  DDkwd__(MVQR_ALL_JBBS_IN_QD,            "OFF"),
#ifdef NDEBUG
  DDkwd__(MVQR_ENABLE_LOGGING,            "OFF"), // No logging by default for release
#else
  DDkwd__(MVQR_ENABLE_LOGGING,            "ON"),
#endif
  DDkwd__(MVQR_LOG_QUERY_DESCRIPTORS,     "OFF"),
  DDint__(MVQR_MAX_EXPR_DEPTH,            "20"),
  DDint__(MVQR_MAX_EXPR_SIZE,             "100"),
  DDkwd__(MVQR_PARAMETERIZE_EQ_PRED,      "ON"),
  DDkwd__(MVQR_PRIVATE_QMS_INIT,          "SMD"),
  DDkwd__(MVQR_PUBLISH_TO,                "BOTH"),
  DDansi_(MVQR_REWRITE_CANDIDATES,        ""),
 XDDkwd__(MVQR_REWRITE_ENABLED_OPTION,    "OFF"),  // @ZX -- change to ON later
 XDDui0_5(MVQR_REWRITE_LEVEL,             "0"),
 XDDkwd__(MVQR_REWRITE_SINGLE_TABLE_QUERIES, "ON"),
  DDkwd__(MVQR_USE_EXTRA_HUB_TABLES,      "ON"),
  DDkwd__(MVQR_USE_RI_FOR_EXTRA_HUB_TABLES, "OFF"),
  DD_____(MVQR_WORKLOAD_ANALYSIS_MV_NAME, ""),

 XDDMVA__(MV_AGE,				"0 MINUTES"),
 XDDkwd__(MV_ALLOW_SELECT_SYSTEM_ADDED_COLUMNS, "OFF"),
  DDkwd__(MV_AS_ROW_TRIGGER,			"OFF"),
  DDkwd__(MV_DUMP_DEBUG_INFO, "OFF"),
  DDkwd__(MV_ENABLE_INTERNAL_REFRESH_SHOWPLAN, "OFF"),


  DDui___(MV_LOG_CLEANUP_SAFETY_FACTOR,	"200"),
  DDui___(MV_LOG_CLEANUP_USE_MULTI_COMMIT,	"1"),

 SDDkwd__(MV_LOG_PUSH_DOWN_DP2_DELETE,           "OFF"), // push down mv logging tp dp2 for delete
 SDDkwd__(MV_LOG_PUSH_DOWN_DP2_INSERT,           "OFF"), // push down mv logging tp dp2 for insert
 SDDkwd__(MV_LOG_PUSH_DOWN_DP2_UPDATE,           "ON"),  // push down mv logging tp dp2 for update

 SDDui___(MV_REFRESH_MAX_PARALLELISM,		"0"),
  DDui___(MV_REFRESH_MAX_PIPELINING,		"0"),

  DDint__(MV_REFRESH_MDELTA_MAX_DELTAS_THRESHOLD,                   "31"),
  DDint__(MV_REFRESH_MDELTA_MAX_JOIN_SIZE_FOR_SINGLE_PHASE,         "3"),
  DDint__(MV_REFRESH_MDELTA_MIN_JOIN_SIZE_FOR_SINGLE_PRODUCT_PHASE, "8"),
  DDint__(MV_REFRESH_MDELTA_PHASE_SIZE_FOR_MID_RANGE,               "6"),

  DDkwd__(MV_TRACE_INCONSISTENCY,		"OFF"),

  DDSint__(MXCMP_PRIORITY,                     "0"),
  DDSint__(MXCMP_PRIORITY_DELTA,               "0"),

  DDkwd__(NAMETYPE,				"ANSI"),

  DDkwd__(NAR_DEPOBJ_ENABLE,                     "ON"),
  DDkwd__(NAR_DEPOBJ_ENABLE2,                     "ON"),
 // NATIONAL_CHARSET reuses the "kwd" logic here, w/o having to add any
 // DF_ token constants (this can be considered either clever or kludgy coding).
  DDkwd__(NATIONAL_CHARSET,		(char *)SQLCHARSETSTRING_UNICODE),


  // These CQDs are reserved for NCM. These are mostly used for
  // internal testing, turning on/off features for debugging, and for tuning.
  // In normal situations, these will not be externalized in keeping
  // with the very few CQDs philosophy of NCM.
  // These are applicable only in conjunction with SIMPLE_COST_MODEL 'on'.
  DDflt__(NCM_CACHE_SIZE_IN_BLOCKS,		"52"),
  DDflt__(NCM_COSTLIMIT_FACTOR,                 "0.05"), //change to 0.05
  DDint__(NCM_ESP_FIXUP_WEIGHT,			"300"),
  DDkwd__(NCM_ESP_STARTUP_FIX,                  "ON"),
  DDflt__(NCM_EXCH_MERGE_FACTOR,                "0.10"), // change to 0.10
  DDkwd__(NCM_EXCH_NDCS_FIX,                    "ON"), // change to ON
  DDkwd__(NCM_HBASE_COSTING,                    "ON"), // change to ON
  DDkwd__(NCM_HGB_OVERFLOW_COSTING,		"ON"),
  DDkwd__(NCM_HJ_OVERFLOW_COSTING,		"ON"),
  DDflt__(NCM_IND_JOIN_COST_ADJ_FACTOR,         "1.0"),
  DDflt__(NCM_IND_JOIN_SELECTIVITY,             "1.0"),
  DDflt__(NCM_IND_SCAN_COST_ADJ_FACTOR,         "1.0"),
  DDflt__(NCM_IND_SCAN_SELECTIVITY,             "1.0"),
  DDflt__(NCM_MAP_CPU_FACTOR,                   "4.0"),
  DDflt__(NCM_MAP_MSG_FACTOR,                   "4.0"),
  DDflt__(NCM_MAP_RANDIO_FACTOR,                "4.0"),
  DDflt__(NCM_MAP_SEQIO_FACTOR,                 "4.0"),
  DDflt__(NCM_MDAM_COST_ADJ_FACTOR,             "1.0"),
  DDflt__(NCM_MJ_TO_HJ_FACTOR,		        "0.6"),
  DDflt__(NCM_NJ_PC_THRESHOLD,                  "1.0"),
  DDflt0_(NCM_NJ_PROBES_MAXCARD_FACTOR,         "10000"),
  DDkwd__(NCM_NJ_SEQIO_FIX,                     "ON"), // change to ON
  DDint__(NCM_NUM_SORT_RUNS,			"4"),
  DDflt__(NCM_PAR_ADJ_FACTOR,                   "0.10"),
  DDkwd__(NCM_PAR_GRPBY_ADJ,			"ON"),
  DDkwd__(NCM_PRINT_ROWSIZE,			"OFF"),
  DDflt__(NCM_RAND_IO_ROWSIZE_FACTOR,		"0"),
  DDflt__(NCM_RAND_IO_WEIGHT,                   "3258"),
  DDflt__(NCM_SEQ_IO_ROWSIZE_FACTOR,		"0"),
  DDflt__(NCM_SEQ_IO_WEIGHT,			"543"),
  DDflt__(NCM_SERIAL_NJ_FACTOR,			"2"),
  DDflt__(NCM_SGB_TO_HGB_FACTOR,		"0.8"),
  DDkwd__(NCM_SKEW_COST_ADJ_FOR_PROBES,		"OFF"),
  DDkwd__(NCM_SORT_OVERFLOW_COSTING,		"ON"),
  DDflt__(NCM_TUPLES_ROWSIZE_FACTOR,		"0.5"),
  DDflt__(NCM_UDR_NANOSEC_FACTOR,               "0.01"),

  // NESTED_JOINS ON means do NESTED_JOINS
 XDDkwd__(NESTED_JOINS,				"ON"),

  // max. number of ESPs that will deal with skews for OCR
  // 0 means to turn off the feature
  DDintN1__(NESTED_JOINS_ANTISKEW_ESPS ,  "16"),

  DDkwd__(NESTED_JOINS_CHECK_LEADING_KEY_SKEW,     "OFF"),
  DDkwd__(NESTED_JOINS_FULL_INNER_KEY,     "OFF"),
  DDkwd__(NESTED_JOINS_KEYLESS_INNERJOINS,     "ON"),
  DDui1__(NESTED_JOINS_LEADING_KEY_SKEW_THRESHOLD,    "15"),
  DDkwd__(NESTED_JOINS_NO_NSQUARE_OPENS,   "ON"),

  DDkwd__(NESTED_JOINS_OCR_GROUPING,            "OFF"),

  // 128X32 being the default threshold for OCR.
  // 128 partitions per table and 32 ESPs per NJ operator
 SDDint__(NESTED_JOINS_OCR_MAXOPEN_THRESHOLD,   "4096"),


  // PLAN0 is solely controlled by OCR. If this CQD is off, then
  // PLAN0 is off unconditionally. This CQD is used by OCR unit test.
  DDkwd__(NESTED_JOINS_PLAN0,                   "ON"),

  // try the explicit sort plan when plan2 produces a non-sort plan
  DDkwd__(NESTED_JOINS_PLAN3_TRY_SORT,		"ON"),

  // Enable caching for eligible nested joins - see NestedJoin::preCodeGen.
  DDkwd__(NESTED_JOIN_CACHE,                    "ON"),
  // Enable pulling up of predicates into probe cache
  DDkwd__(NESTED_JOIN_CACHE_PREDS,              "ON"),
  // Nested Join Heuristic
  DDkwd__(NESTED_JOIN_CONTROL,			"ON"),


  // Allow nested join for cross products
  DDkwd__(NESTED_JOIN_FOR_CROSS_PRODUCTS,	"ON"),
  DDkwd__(NEW_MDAM,		"ON"),
  DDkwd__(NEW_OPT_DRIVER,		"ON"),



  DDflt0_(NJ_CPUCOST_INITIALIZE,		".1"),

  DDflt0_(NJ_CPUCOST_PASS_ROW,			".02"),

  DDflte_(NJ_INC_AFTERLIMIT,			"0.0055"),
  DDflte_(NJ_INC_MOVEROWS,			"0.0015"),
  DDflte_(NJ_INC_UPTOLIMIT,			"0.0225"),

  DDui___(NJ_INITIAL_BUFFER_COUNT,		"5"),
  DDui1__(NJ_INITIAL_BUFFER_SIZE,		"5"),

  DDui1__(NJ_MAX_SEEK_DISTANCE,			"5000"),

  // UDF costing CQDs for processing a steady state row
  DDui___(NORMAL_UDF_CPU_COST,                   "100"),
  DDui___(NORMAL_UDF_IO_COST,                    "0"),
  DDui___(NORMAL_UDF_MSG_COST,                   "2"),

  XDDui30_32000(NOT_ATOMIC_FAILURE_LIMIT,	       "32000"),

  //NOT IN ANSI NULL semantics rule
  DDkwd__(NOT_IN_ANSI_NULL_SEMANTICS,          "ON"),
  //NOT IN optimization
  DDkwd__(NOT_IN_OPTIMIZATION,		        "ON"),
  //NOT IN outer column optimization
  DDkwd__(NOT_IN_OUTER_OPTIMIZATION,            "ON"),
  // NOT IN skew buster optimization
  DDkwd__(NOT_IN_SKEW_BUSTER_OPTIMIZATION,      "ON"),

  DDkwd__(NOT_NULL_CONSTRAINT_DROPPABLE_OPTION, "OFF"),


  // NSK DEBUG defaults
  DDansi_(NSK_DBG,                                      "OFF"),
  DDansi_(NSK_DBG_COMPILE_INSTANCE,                     "USER"),
  DDkwd__(NSK_DBG_GENERIC,                              "OFF"),
  DDansi_(NSK_DBG_LOG_FILE,                             ""),
  DDkwd__(NSK_DBG_MJRULES_TRACKING,                     "OFF"),
  DDkwd__(NSK_DBG_PRINT_CHAR_INPUT,                     "OFF"),
  DDkwd__(NSK_DBG_PRINT_CHAR_OUTPUT,                    "OFF"),
  DDkwd__(NSK_DBG_PRINT_CONSTRAINT,                     "OFF"),
  DDkwd__(NSK_DBG_PRINT_CONTEXT,                        "OFF"),
  DDkwd__(NSK_DBG_PRINT_CONTEXT_POINTER,                "OFF"),
  DDkwd__(NSK_DBG_PRINT_COST,                           "OFF"),
  DDkwd__(NSK_DBG_PRINT_INDEX_ELIMINATION,              "OFF"),
  DDkwd__(NSK_DBG_PRINT_ITEM_EXPR,                      "OFF"),
  DDkwd__(NSK_DBG_PRINT_LOG_PROP,                       "OFF"),
  DDkwd__(NSK_DBG_PRINT_PHYS_PROP,                      "OFF"),
  DDkwd__(NSK_DBG_PRINT_TASK,                           "OFF"),
  DDkwd__(NSK_DBG_PRINT_TASK_STACK,                     "OFF"),
  DDkwd__(NSK_DBG_QUERY_LOGGING_ONLY,                   "OFF"),
  DDansi_(NSK_DBG_QUERY_PREFIX,                         ""),
  DDkwd__(NSK_DBG_SHOW_PASS1_PLAN,                      "OFF"),
  DDkwd__(NSK_DBG_SHOW_PASS2_PLAN,                      "OFF"),
  DDkwd__(NSK_DBG_SHOW_PLAN_LOG,                        "OFF"),
  DDkwd__(NSK_DBG_SHOW_TREE_AFTER_ANALYSIS,             "OFF"),
  DDkwd__(NSK_DBG_SHOW_TREE_AFTER_BINDING,              "OFF"),
  DDkwd__(NSK_DBG_SHOW_TREE_AFTER_CODEGEN,              "OFF"),
  DDkwd__(NSK_DBG_SHOW_TREE_AFTER_NORMALIZATION,        "OFF"),
  DDkwd__(NSK_DBG_SHOW_TREE_AFTER_PARSING,              "OFF"),
  DDkwd__(NSK_DBG_SHOW_TREE_AFTER_PRE_CODEGEN,          "OFF"),
  DDkwd__(NSK_DBG_SHOW_TREE_AFTER_SEMANTIC_QUERY_OPTIMIZATION, "OFF"),
  DDkwd__(NSK_DBG_SHOW_TREE_AFTER_TRANSFORMATION,       "OFF"),

  DDflt0_(NUMBER_OF_PARTITIONS_DEVIATION,	"0.25"),
  DDui1__(NUMBER_OF_ROWS_PARALLEL_THRESHOLD,	"5000"),
  DDui1__(NUMBER_OF_USERS,			"1"),
  DDui1__(NUM_OF_BLOCKS_PER_ACCESS,		"SYSTEM"),
  DDflt0_(NUM_OF_PARTS_DEVIATION_TYPE2_JOINS,	"SYSTEM"),

  DDkwd__(NVCI_PROCESS,				"FALSE"),
  DDflt0_(OCB_COST_ADJSTFCTR, "0.996"),
  DDui___(OCR_FOR_SIDETREE_INSERT,              "1"),

  DDkwd__(ODBC_PROCESS,				"FALSE"),

  DDflte_(OHJ_BMO_REUSE_SORTED_BMOFACTOR_LIMIT,	"3.0"),
  DDflte_(OHJ_BMO_REUSE_SORTED_UECRATIO_UPPERLIMIT, "0.7"),
  DDflte_(OHJ_BMO_REUSE_UNSORTED_UECRATIO_UPPERLIMIT, "0.01"),
  DDflte_(OHJ_VBMOLIMIT,			"5.0"),

  DDui1__(OLAP_BUFFER_SIZE,		        "262144"), // Do not alter (goes to DP2)
  DDkwd__(OLAP_CAN_INVERSE_ORDER,               "ON"),
  DDui1__(OLAP_MAX_FIXED_WINDOW_EXTRA_BUFFERS,  "2"),
  DDui1__(OLAP_MAX_FIXED_WINDOW_FRAME,          "50000"),
  DDui1__(OLAP_MAX_NUMBER_OF_BUFFERS,           "100000"),
  DDui___(OLAP_MAX_ROWS_IN_OLAP_BUFFER,             "0"),

  //aplies for fixed window-- number of additional oplap buffers
  //to allocate on top of the minumum numbers

  DDkwd__(OLD_HASH2_GROUPING,                   "FALSE"),

  DDkwd__(OLT_QUERY_OPT,			"ON"),
  DDkwd__(OLT_QUERY_OPT_LEAN,			"OFF"),

  // -----------------------------------------------------------------------
  // Optimizer pruning heuristics.
  // -----------------------------------------------------------------------
  DDkwd__(OPH_EXITHJCRCONTCHILOOP,	                        "ON"),
  DDkwd__(OPH_EXITMJCRCONTCHILOOP,	                        "ON"),
  DDkwd__(OPH_EXITNJCRCONTCHILOOP,	                        "OFF"),
  DDkwd__(OPH_PRUNE_WHEN_COST_LIMIT_EXCEEDED,	                "OFF"),
  DDflt__(OPH_PRUNING_COMPLEXITY_THRESHOLD,                     "10.0"),
  DDflt__(OPH_PRUNING_PASS2_COST_LIMIT,	                        "-1.0"),
  DDkwd__(OPH_REDUCE_COST_LIMIT_FROM_CANDIDATES,		"OFF"),
  DDkwd__(OPH_REDUCE_COST_LIMIT_FROM_PASS1_SOLUTION,		"ON"),
  DDkwd__(OPH_REUSE_FAILED_PLAN,				"ON"),
  DDkwd__(OPH_REUSE_OPERATOR_COST,				"OFF"),
  DDkwd__(OPH_SKIP_OGT_FOR_SHARED_GC_FAILED_CL,			"OFF"),
  DDkwd__(OPH_USE_CACHED_ELAPSED_TIME,	                        "ON"),
  DDkwd__(OPH_USE_CANDIDATE_PLANS,				"OFF"),
  DDkwd__(OPH_USE_COMPARE_COST_THRESHOLD,			"ON"),
  DDkwd__(OPH_USE_CONSERVATIVE_COST_LIMIT,			"OFF"),
  DDkwd__(OPH_USE_ENFORCER_PLAN_PROMOTION,			"OFF"),
  DDkwd__(OPH_USE_FAILED_PLAN_COST,				"ON"),
  DDkwd__(OPH_USE_NICE_CONTEXT,					"OFF"),
  DDkwd__(OPH_USE_ORDERED_MJ_PRED,				"OFF"),
  DDkwd__(OPH_USE_PWS_FLAG_FOR_CONTEXT,				"OFF"),


  DDkwd__(OPTIMIZATION_GOAL,			"LASTROW"),
 XDDkwd__(OPTIMIZATION_LEVEL,			"3"),
  DDpct__(OPTIMIZATION_LEVEL_1_CONSTANT_1,	"50"),
  DDpct__(OPTIMIZATION_LEVEL_1_CONSTANT_2,	"0"),
  DDui1__(OPTIMIZATION_LEVEL_1_IMMUNITY_LIMIT,		"5000"),
  DDui1__(OPTIMIZATION_LEVEL_1_MJENUM_LIMIT,		"20"),
  DDui1__(OPTIMIZATION_LEVEL_1_SAFETY_NET,		"30000"),
  DDflt__(OPTIMIZATION_LEVEL_1_SAFETY_NET_MULTIPLE,	"3.0"),
  DDui1__(OPTIMIZATION_LEVEL_1_THRESHOLD,		"1000"),
  DDui1__(OPTIMIZATION_TASKS_LIMIT,		"2000000000"),
  // Optimizer Graceful Termination:
  // 1=> randomProbabilistic pruning
  // > 1 pruning based on potential
  DDui1__(OPTIMIZER_GRACEFUL_TERMINATION, "2"),
  DDkwd__(OPTIMIZER_HEURISTIC_1,		"OFF"),
  DDkwd__(OPTIMIZER_HEURISTIC_2,		"OFF"),
  DDkwd__(OPTIMIZER_HEURISTIC_3,		"OFF"),
  DDkwd__(OPTIMIZER_HEURISTIC_4,		"OFF"),
  DDkwd__(OPTIMIZER_HEURISTIC_5,		"OFF"),

  // Tells the compiler to print costing information
  DDkwd__(OPTIMIZER_PRINT_COST,			"OFF"),

  // Tells the compiler to issue a warning with its internal counters
  DDkwd__(OPTIMIZER_PRINT_INTERNAL_COUNTERS,	"OFF"),

  // Pruning is OFF because of bugs, turn to ON when bugs are fixed
  // (03/03/98)
 SDDkwd__(OPTIMIZER_PRUNING,			"ON"),
  DDkwd__(OPTIMIZER_PRUNING_FIX_1,              "ON"), //change to ON

  DDkwd__(OPTIMIZER_SYNTH_FUNC_DEPENDENCIES,    "ON"),

  //OPTS_PUSH_DOWN_DAM made external RV 06/21/01 CR 10-010425-2440
  DDui___(OPTS_PUSH_DOWN_DAM,                   "0"),

  DDkwd__(ORDERED_HASH_JOIN_CONTROL,            "ON"),
 SDDkwd__(OR_OPTIMIZATION,                      "ON"),
  DDkwd__(OR_PRED_ADD_BLOCK_TO_IN_LIST,         "ON"),
  DDkwd__(OR_PRED_KEEP_CAST_VC_UCS2,            "ON"),
  // controls the jump table method of evaluating an or pred. in a scan node
  // 0 => feature is OFF, positive integer denotes max OR pred that will be
  // processed through a jump table.
  DDint__(OR_PRED_TO_JUMPTABLE,                 "2000"),
  // controls semijoin method of evaluating an or pred.
  // 0 => feature is OFF, positive number means if pred do not cover key cols
  // and jump table is not available, then the transformation is done if
  // inlist is larger than this value.
  DDint__(OR_PRED_TO_SEMIJOIN,                 "100"),
  // Ratio of tablesize (without application of any preds)to probes below
  // which semijoin trans. is favoured.
  DDflt0_(OR_PRED_TO_SEMIJOIN_PROBES_MAX_RATIO,                 "0.001"),
  // Minimum table size beyond which semijoin trans. is considered
  DDint__(OR_PRED_TO_SEMIJOIN_TABLE_MIN_SIZE,                 "10000"),



  DDui2__(OS_MESSAGE_BUFFER_SIZE,    "32"),
 // if set to "ansi", datetime output is in ansi format. Currently only
 // used in special_1 mode if the caller needs datetime value in
 // ansi format (like, during upd stats).
  DDansi_(OUTPUT_DATE_FORMAT,		        ""),

  // Overflow mode for scratch files
  DDkwd__(OVERFLOW_MODE,                  "DISK"),

  // Sequence generator override identity values
  DDkwd__(OVERRIDE_GENERATED_IDENTITY_VALUES,	  "OFF"),

// allow users to specify a source schema to be
// replaced by a target schema
 SDDosch_(OVERRIDE_SCHEMA,              ""),

  // Allows users to specify their own SYSKEY value. In other words
  // the system does not generate one for them.

  // Prior to this CQD, pm_regenerate_syskey_for_insert was being used
  // to preserve the syskey. Carrying over these comments from
  // pm_regenerate_syskey_for_insert
  // For audited target partition, PM does the copy in multiple transactions
  // In each transaction PM does a insert/select from the source to the target
  // partition. The clustering key values from the last row of a transaction
  // is used as begin key value for the next transaction. If the table
  // has a syskey then it gets regenerated and last row contains the new
  // value for the syskey. This obviously causes us to start at a different
  // place then we intended to start from. The following default when set
  // to off forces the engine to not regenerate syskey.

  DDkwd__(OVERRIDE_SYSKEY,	      "OFF"),

  DDui___(PARALLEL_ESP_NODEMASK,		"0"),

  // by default all parallelism heuristics are switched ON.
  DDkwd__(PARALLEL_HEURISTIC_1,                  "ON"),
  DDkwd__(PARALLEL_HEURISTIC_2,                  "ON"),
  DDkwd__(PARALLEL_HEURISTIC_3,                  "ON"),
  DDkwd__(PARALLEL_HEURISTIC_4,                  "ON"),

  // If PARALLEL_NUM_ESPS is "SYSTEM",
  // optimizer will compute the number of ESPs.
 XDDui1__(PARALLEL_NUM_ESPS,			"SYSTEM"),


  // is partial sort applicable; if so adjust sort cost accordingly
  DDflt0_(PARTIAL_SORT_ADJST_FCTR,        "1"),

  DDint__(PARTITIONING_SCHEME_SHARING,	"1"),

  // The optimal number of partition access nodes for a process.
  // NOTE: Setting this to anything other than 1 will cause problems
  // with Cascades plan stealing! Don't do it unless you have to!
  DDui1__(PARTITION_ACCESS_NODES_PER_ESP,	"1"),

  DD_____(PCODE_DEBUG_LOGDIR,        ""  ), // Pathname of log directory for PCode work
  DDint__(PCODE_EXPR_CACHE_CMP_ONLY, "0" ), // PCode Expr Cache compare-only mode
  DDint__(PCODE_EXPR_CACHE_DEBUG,    "0" ), // PCode Expr Cache debug (set to 1 to enable dbg logging)
  DDint__(PCODE_EXPR_CACHE_ENABLED,  "0" ), // PCode Expr Cache Enabled (set to 0 to disable the cache)
  DD0_10485760(PCODE_EXPR_CACHE_SIZE,"2000000"), // PCode Expr Cache Max Size

  // Maximum number of PCODE Branch Instructions in an Expr
  // for which we will attempt PCODE optimizations.
  // NOTE: Default value reduced to 12000 for Trafodion to avoid stack
  //       overflow in PCODE optimization where recursion is used.
  DDint__(PCODE_MAX_OPT_BRANCH_CNT,          "12000"),

  // Maximum number of PCODE Instructions in an Expr
  // for which we will attempt PCODE optimizations.
  DDint__(PCODE_MAX_OPT_INST_CNT,            "50000"),

  DDint__(PCODE_NE_DBG_LEVEL, "-1"), // Native Expression Debug Level
  DDint__(PCODE_NE_ENABLED,   "1" ), // Native Expressions Enabled
  DDkwd__(PCODE_NE_IN_SHOWPLAN, "ON"), // Native Expression in Showplan output


  DDint__(PCODE_OPT_FLAGS,                      "60"),
 
  DDkwd__(PCODE_OPT_LEVEL,		"MAXIMUM"),

  DDint__(PHY_MEM_CONTINGENCY_MB,     "3072"),

  DDkwd__(PLAN_STEALING,			"ON"),

  // Partition OVerlay Support (POS) options
  SDDkwd__(POS,                                 "DISK_POOL"),
   DD_____(POS_DISKS_IN_SEGMENT,                 ""),
   DD_____(POS_DISK_POOL,			"0"),
  SDDui___(POS_NUM_DISK_POOLS,                  "0"),
   DDui___(POS_NUM_OF_PARTNS,                   "SYSTEM"),
  SDDint__(POS_NUM_OF_TEMP_TABLE_PARTNS,        "SYSTEM"),
  SDDpos__(POS_TEMP_TABLE_SIZE,                 ""),
   DDui___(POS_TEST_NUM_NODES,  "0"),

  // Use info from right child to require order on left child of NJ
//PREFERRED_PROBING_ORDER_FOR_NESTED_JOIN made external RV 06/21/01 CR 10-010425-2440

   DDkwd__(PREFERRED_PROBING_ORDER_FOR_NESTED_JOIN,	"OFF"),

   DD0_18(PRESERVE_MIN_SCALE,            "0"),

  DDkwd__(PRIMARY_KEY_CONSTRAINT_DROPPABLE_OPTION,      "OFF"),
  DDkwd__(PSHOLD_CLOSE_ON_ROLLBACK,	"OFF"),
  DDkwd__(PSHOLD_UPDATE_BEFORE_FETCH,	"OFF"),
 SDDpsch_(PUBLIC_SCHEMA_NAME, ""),


 // Query Invalidation - Debug/Regression test CQDs -- DO NOT externalize these
 DD_____(QI_PATH, "" ),  // Specifies cat.sch.object path for object to have cache entries removed
 DD0_255(QI_PRIV, "0"),  // Note: 0 disables the Debug Mechanism.  Set non-zero to kick out cache entries.
                         // Then set back to 0 *before* setting to a non-zero value again.

  // Do the query analysis phase
  DDkwd__(QUERY_ANALYSIS,	"ON"),

 // query_cache max should be 200 MB. Set it 0 to turn off query cache
 //XDD0_200000(QUERY_CACHE,                  "0"),
 XDD0_200000(QUERY_CACHE,                  "16384"),

 // the initial average plan size (in kbytes) to use for configuring the
 // number of hash buckets to use for mxcmp's hash table of cached plans
 DD1_200000(QUERY_CACHE_AVERAGE_PLAN_SIZE, "30"),

 // literals longer than this are not parameterized
 DDui___(QUERY_CACHE_MAX_CHAR_LEN,         "32000"),

 // a query with more than QUERY_CACHE_MAX_EXPRS ExprNodes is not cacheable
 DDint__(QUERY_CACHE_MAX_EXPRS,            "1000"),

 // the largest number of cache entries that an unusually large cache
 // entry is allowed to displace from mxcmp's cache of query plans
 DD0_200000(QUERY_CACHE_MAX_VICTIMS,      "10"),
 DD0_255(QUERY_CACHE_REQUIRED_PREFIX_KEYS, "255"),
 DDkwd__(QUERY_CACHE_RUNTIME,               "ON"),
SDDflt0_(QUERY_CACHE_SELECTIVITY_TOLERANCE,       "0"),

 // query cache statement pinning is off by default
 DDkwd__(QUERY_CACHE_STATEMENT_PINNING,	"OFF"),
 DDkwd__(QUERY_CACHE_STATISTICS,            "OFF"),
 DD_____(QUERY_CACHE_STATISTICS_FILE,       "qcachsts"),
 DDkwd__(QUERY_CACHE_TABLENAME,               "OFF"),
 DDkwd__(QUERY_CACHE_USE_CONVDOIT_FOR_BACKPATCH, "ON"),

  // Limit CPU time a query can use in master or any ESP.  Unit is seconds.
 XDDint__(QUERY_LIMIT_SQL_PROCESS_CPU,         "0"),
 // Extra debugging info for QUERY_LIMIT feature.
 DDkwd__(QUERY_LIMIT_SQL_PROCESS_CPU_DEBUG,              "OFF"),
  // For X-prod HJ: (# of rows joined * LIMIT) before preempt.
 DDint__(QUERY_LIMIT_SQL_PROCESS_CPU_XPROD,         "10000"),
 // controls various expr optimizations based on bit flags.
 // see enum QueryOptimizationOptions in DefaultConstants.h
 DDint__(QUERY_OPTIMIZATION_OPTIONS,        "3"),
 DDkwd__(QUERY_STRATEGIZER,              "ON"),
 DDkwd__(QUERY_TEMPLATE_CACHE,              "ON"),
 DDkwd__(QUERY_TEXT_CACHE,                  "SYSTEM"),

 DDkwd__(R2_HALLOWEEN_SUPPORT,              "OFF"),

 DDkwd__(RANGESPEC_TRANSFORMATION,         "ON"), // RangeSpec Transformation CQD.
 // To be ANSI compliant you would have to set this default to 'FALSE'
 DDkwd__(READONLY_CURSOR,			"TRUE"),

 DDflt0_(READ_AHEAD_MAX_BLOCKS,	    "16.0"),
 // OFF means Ansi/NIST setting, ON is more similar to the SQL/MP behavior
 DDkwd__(RECOMPILATION_WARNINGS,		"OFF"),
  // CLI caller to redrive CTAS(create table as) for child query monitoring
  DDkwd__(REDRIVE_CTAS,		"OFF"),
  // The group by reduction for pushing a partial group by past the
  //  right side of the TSJ must be at least this much.  If 0.0, then
  //  pushing it will always be tried.
  DDflt0_(REDUCTION_TO_PUSH_GB_PAST_TSJ,    "0.0000000001"),
  // This is the code base for the calibration machine. It must be either
  // "DEBUG" or "RELEASE"
  // History:
  // Before 02/01/99: DEBUG

  DDkwd__(REFERENCE_CODE,		"RELEASE"),

  // This is the frequency of the representative CPU of the base calibration
  // cluster.
  // REFERENCE_CPU_FREQUENCY units are MhZ
  DDflte_(REFERENCE_CPU_FREQUENCY,	"199."),

  // This is the seek time of the representative disk of the base
  // calibration cluster.
  // REFERENCE_IO_SEEK_TIME units are seconds
  DDflte_(REFERENCE_IO_SEEK_TIME,     "0.0038"),

  // This is the sequential transfer rate for the representative
  // disk of the base calibration cluster.
  // REFERENCE_IO_SEQ_READ_RATE units are Mb/Sec
  DDflte_(REFERENCE_IO_SEQ_READ_RATE, "50.0"),

  // This is the transfer rate for the fast speed connection of
  // nodes in the base calibration cluster.
  // REFERENCE_MSG_LOCAL_RATE units are Mb/Sec
  DDflte_(REFERENCE_MSG_LOCAL_RATE,     "10."),

  // This is the timeper local msg for the fast speed connection of
  // nodes in the base calibration cluster.
  // REFERENCE_MSG_LOCAL_TIME units are seconds
  DDflte_(REFERENCE_MSG_LOCAL_TIME,     "0.000125"),

  // This is the transfer rate for the connection among clusters
  // in the base calibration cluster (this only applies to NSK)
  // REFERENCE_MSG_REMOTE_RATE units are Mb/Sec
  DDflte_(REFERENCE_MSG_REMOTE_RATE,	"1."),

  // This is the time per remote msg for the fast speed connection of
  // nodes in the base calibration cluster.
  // REFERENCE_MSG_REMOTE_TIME units are seconds
  DDflte_(REFERENCE_MSG_REMOTE_TIME,            "0.00125"),
  DDkwd__(REF_CONSTRAINT_NO_ACTION_LIKE_RESTRICT,     "SYSTEM"),



  DDrlis_(REPLICATE_ALLOW_ROLES,                ""),
  // Determines the compression type to be used with DDL when replicating 
  DDkwd__(REPLICATE_COMPRESSION_TYPE,           "SYSTEM"),
  // Determines if DISK POOL setting should be passed with DDL when replicating
  DDkwd__(REPLICATE_DISK_POOL,                  "ON"),
  // VERSION of the message from the source system to maintain compatibility
  // This version should be same as REPL_IO_VERSION_CURR in executor/ExeReplInterface.h
  // Make changes accordingly in validataorReplIoVersion validator
  DDrver_(REPLICATE_IO_VERSION,                 "17"),




  DDkwd__(REUSE_BASIC_COST,			"ON"),

  // if set, tables are not closed at the end of a query. This allows
  // the same open to be reused for the next query which accesses that
  // table.
  // If the table is shared opened by multiple openers from the same
  // process, then the share count is decremented until it reaches 1.
  // At that time, the last open is preserved so it could be reused.
  // Tables are closed if user id changes.
  DDkwd__(REUSE_OPENS,			        "ON"),

  // multiplicative factor used to inflate cost of risky operators.
  // = 1.0 means do not demand an insurance premium from risky operators.
  // = 1.2 means demand a 20% insurance premium that cost of risky operators
  // must overcome before they will be chosen over less-risky operators.
  DDflt0_(RISK_PREMIUM_MJ,     "1.15"),
 XDDflt0_(RISK_PREMIUM_NJ,     "1.0"),
 XDDflt0_(RISK_PREMIUM_SERIAL, "1.0"),
 XDDui___(RISK_PREMIUM_SERIAL_SCALEBACK_MAXCARD_THRESHOLD, "10000"),

  DDflt0_(ROBUST_HJ_TO_NJ_FUDGE_FACTOR, "0.0"),
  DDflt0_(ROBUST_PAR_GRPBY_EXCHANGE_FCTR, "0.25"),
  DDflt0_(ROBUST_PAR_GRPBY_LEAF_FCTR, "0.25"),


  // external master CQD that sets following internal CQDs
  //                              robust_query_optimization
  //                              MINIMUM  SYSTEM  HIGH MAXIMUM
  // risk_premium_NJ                 1.0   system  2.5  5.0
  // risk_premium_SERIAL             1.0   system  1.5  2.0
  // partitioning_scheme_sharing     0     system  2    2
  // robust_hj_to_nj_fudge_factor    0.0   system  3.0  1.0
  // robust_sortgroupby              0     system  2    2
  // risk_premium_MJ                 1.0   system  1.5  2.0
  // see optimizer/ControlDB.cpp ControlDB::doRobustQueryOptimizationCQDs
  // for the actual cqds that set these values
 XDDkwd__(ROBUST_QUERY_OPTIMIZATION,            "SYSTEM"),

  // 0: allow sort group by in all
  // 1: disallow sort group by from partial grpByRoot if no order requirement
  // 2: disallow sort group by from partial grpByRoot
  // 3: disallow sort group by in ESP
  DDint__(ROBUST_SORTGROUPBY,  "1"),

 SDDui___(ROUNDING_MODE,			"0"),

  DDui___(ROUTINE_CACHE_SIZE,                   "20"),

  // UDF default Uec
  DDui___(ROUTINE_DEFAULT_UEC,                   "1"),

  DDkwd__(ROUTINE_JOINS_SPOIL_JBB,              "OFF"),
  DDkwd__(ROWSET_ROW_COUNT,	                "OFF"),
  DDint__(SAP_KEY_NJ_TABLE_SIZE_THRESHOLD,      "10000000"),
  DDkwd__(SAP_PREFER_KEY_NESTED_JOIN,           "OFF"),
  DDint__(SAP_TUPLELIST_SIZE_THRESHOLD,         "5000"),
 XDDansi_(SCHEMA,                               "SEABASE"), 
 //specify a : separated list of full path names where scratch files
 //should reside. Ensure each specified directoy exisst on each node and 
 //Trafodion user has permissions to access them.
 DD_____(SCRATCH_DIRS,                        ""),
 DDkwd__(SCRATCH_DISK_LOGGING,                 "OFF"),
 SDDpct__(SCRATCH_FREESPACE_THRESHOLD_PERCENT,      "1"),
  DDui___(SCRATCH_IO_BLOCKSIZE_SORT_MAX,        "5242880"),
  //On LINUX, writev and readv calls are used to perform
  //scratch file IO. This CQD sets the vector size to use
  //in writev and readv calls. Overall IO size is affected
  //by this cqd. Also, related cqds that are related to
  //IO size are: COMP_INT_67, GEN_HGBY_BUFFER_SIZE.
  //GEN_HSHJ_BUFFER_SIZE, OLAP_BUFFER_SIZE,
  //EXE_HGB_INITIAL_HT_SIZE. Vector size is no-op on other
  //platforms.
  DDui___(SCRATCH_IO_VECTOR_SIZE_HASH,          "8"),
  DDui___(SCRATCH_IO_VECTOR_SIZE_SORT,          "1"),
  DDui___(SCRATCH_MAX_OPENS_HASH, "1"),
  DDui___(SCRATCH_MAX_OPENS_SORT, "1"),
  DDui___(SCRATCH_MGMT_OPTION, "11"),
  DDkwd__(SCRATCH_PREALLOCATE_EXTENTS,             "OFF"),

 DD_____(SEABASE_CATALOG,                                TRAFODION_SYSCAT_LIT),

  DDkwd__(SEABASE_VOLATILE_TABLES,             "ON"),

  // SeaMonster messaging -- the default can be ON, OFF, or SYSTEM.
  // When the default is SYSTEM we take the setting from env var
  // SQ_SEAMONSTER which will have a value of 0 or 1.
  DDkwd__(SEAMONSTER,                  "SYSTEM"),

  // If the inner table of a semi-join has fewer rows than this,
  // we'll allow it to be transformed to a join.
  DDflt1_(SEMIJOIN_TO_INNERJOIN_INNER_ALLOWANCE,  "100.0"),
 // Ratio of right child cardinality to uec above which semijoin 
 // trans. is favored.
  DDflt1_(SEMIJOIN_TO_INNERJOIN_REDUCTION_RATIO,  "5.0"),
 SDDkwd__(SEMIJOIN_TO_INNERJOIN_TRANSFORMATION, "SYSTEM"),
  // Disallow/Allow semi and anti-semi joins in MultiJoin framework
  DDkwd__(SEMI_JOINS_SPOIL_JBB,        "OFF"),


  DDansi_(SESSION_ID,                           ""),

  DDkwd__(SESSION_IN_USE,		        "OFF"),

  DDansi_(SESSION_USERNAME,                     ""),

  DDflt0_(SGB_CPUCOST_INITIALIZE,		".05"),
  DDui___(SGB_INITIAL_BUFFER_COUNT,		"5."),
  DDui1__(SGB_INITIAL_BUFFER_SIZE,		"5."),
  DDkwd__(SHARE_TEMPLATE_CACHED_PLANS,	    "ON"),
  DDui___(SHORT_OPTIMIZATION_PASS_THRESHOLD,		"12"),

 SDDkwd__(SHOWCONTROL_SHOW_ALL,         	"OFF"),
 SDDkwd__(SHOWCONTROL_SHOW_SUPPORT,		"OFF"),
  DDkwd__(SHOWDDL_DISPLAY_FORMAT, 		"EXTERNAL"),
  DDkwd__(SHOWDDL_DISPLAY_PRIVILEGE_GRANTS,     "SYSTEM"),
 DDint__(SHOWDDL_FOR_REPLICATE, 		"0"),
  DDkwd__(SHOWWARN_OPT,		"ON"),

 DDkwd__(SIMPLE_COST_MODEL,                    "ON"),

 XDDkwd__(SKEW_EXPLAIN,                         "ON"),
 XDDflt__(SKEW_ROWCOUNT_THRESHOLD,              "1000000"), // Column row count
                                                           // threshold below
                                                           // which skew
                                                           // buster is disabled.
 XDDflt__(SKEW_SENSITIVITY_THRESHOLD,           "0.1"),

  DDkwd__(SKIP_UNAVAILABLE_PARTITION,		"OFF"),


  DDui0_5(SOFT_REQ_HASH_TYPE,                   "2"),
  DDkwd__(SORT_ALGO,                  "QS"),
  // Calibration
  // 01/23/98: 10000
  // Original: 10.
  DDflt0_(SORT_CPUCOST_INITIALIZE,		"10000."),

  DDui1__(SORT_EX_BUFFER_SIZE,			"5."),
  DDkwd__(SORT_INTERMEDIATE_SCRATCH_CLEANUP,    "ON"),
  DDui1__(SORT_IO_BUFFER_SIZE,                        "128."),
  
  DD1_200000(SORT_MAX_HEAP_SIZE_MB,            "800"),
  DDkwd__(SORT_MEMORY_QUOTA_SYSTEM,             "ON"),
  DD1_128(SORT_MERGE_BUFFER_UNIT_56KB,          "1"),

  // Calibration
  // 04/06/2005: 1.5
  DDflte_(SORT_QS_FACTOR,			"1.5"),

  //Maximum records after which sort would switch over to
  //iterative heap sort. Most often in partial sort, we may want
  //do a quick sort or similar to avoid larger in-memory sort
  //setup.
  DDint__(SORT_REC_THRESHOLD,          "1000"),

  // Calibration
  DDflte_(SORT_RS_FACTOR,			"3.55"),
  // Calibration
  // 04/06/2005: 2.1
  DDflte_(SORT_RW_FACTOR,			"2.1"),

  DDflte_(SORT_TREE_NODE_SIZE,			".012"),

  DDkwd__(SQLMX_REGRESS,                                    "OFF"),
  SDDui___(SSD_BMO_MAX_MEM_THRESHOLD_IN_MB,     "1200"),

  // BertBert VV
  // Timeout for a streaming cursor to return to the fetch(), even if no
  // rows to return.  The cursor is NOT closed, it just gives control to
  // the user again.
  // "0" means no timeout, just check instead.
  // "negative" means never timeout.
  // "positive" means the number of centiseconds to wait before timing out.
 XDDint__(STREAM_TIMEOUT,			"-1"),

XDDkwd__(SUBQUERY_UNNESTING,			"ON"),
 DDkwd__(SUBQUERY_UNNESTING_P2,			"ON"),
 DDkwd__(SUBSTRING_TRANSFORMATION,		"OFF"),

  DDkwd__(SUPPRESS_CHAR_LIMIT_CHECK,            "OFF"),
 XDDkwd__(TABLELOCK,				"SYSTEM"),

  // This is the code base for the end user calibration cluster.
  // It must be either "DEBUG" or "RELEASE"
#ifdef NDEBUG
  DDkwd__(TARGET_CODE,				"RELEASE"),
#else
  DDkwd__(TARGET_CODE,				"DEBUG"),
#endif
  // This is the frequency of the representative CPU of the end user
  // cluster.
  // TARGET_CPU_FREQUENCY units are MhZ.
  DDflte_(TARGET_CPU_FREQUENCY,		"199."),

  // This is the seek time of the representative disk of the end user
  // cluster.
  // TARGET_IO_SEEK_TIME units are seconds
  DDflte_(TARGET_IO_SEEK_TIME,                "0.0038"),

  // This is the sequential transfer rate for the representative
  // disk of the end user cluster.
  // TARGET_IO_SEQ_READ_RATE units are Mb/Sec
  DDflte_(TARGET_IO_SEQ_READ_RATE,    "50.0"),

  // This is the transfer rate for the fast speed connection of
  // nodes in the end user cluster.
  // TARGET_MSG_LOCAL_RATE units are Mb/Sec
  DDflte_(TARGET_MSG_LOCAL_RATE,		"10."),

  // This is the per msg time for the fast speed connection of
  // nodes in the end user cluster.
  // TARGET_MSG_LOCAL_TIME are seconds
  DDflte_(TARGET_MSG_LOCAL_TIME,		"0.000125"),

  // This is the transfer rate for the connection among clusters
  // in the end user cluster (this only applies to NSK)
  // TARGET_MSG_REMOTE_RATE units are Mb/Sec
  DDflte_(TARGET_MSG_REMOTE_RATE,	"1."),

  // This is the per msg time for the the connection among clusters
  // nodes in the end user cluster.
  // TARGET_MSG_REMOTE_TIME are seconds
  DDflte_(TARGET_MSG_REMOTE_TIME,		"0.00125"),

  DDkwd__(TERMINAL_CHARSET,             (char *)SQLCHARSETSTRING_ISO88591),

  DDint__(TEST_PASS_ONE_ASSERT_TASK_NUMBER,	"-1"),
  DDint__(TEST_PASS_TWO_ASSERT_TASK_NUMBER,	"-1"),

  XDDintN2(TIMEOUT,				"6000"),
 
  DDflt0_(TMUDF_CARDINALITY_FACTOR, "1"),
  DDflt0_(TMUDF_LEAF_CARDINALITY, "1"),

  DDkwd__(TOTAL_RESOURCE_COSTING,               "ON"),
 
  DDkwd__(TRAF_ALIGNED_ROW_FORMAT,                 "ON"),   
 
  DDkwd__(TRAF_ALLOW_ESP_COLOCATION,             "OFF"),   
 
  DDkwd__(TRAF_ALLOW_RESERVED_COLNAMES,          "OFF"),   
 
  DDkwd__(TRAF_ALLOW_SELF_REF_CONSTR,                 "ON"),   

  DDkwd__(TRAF_ALTER_ADD_PKEY_AS_UNIQUE_CONSTRAINT, "OFF"),   

  DDkwd__(TRAF_ALTER_COL_ATTRS,                 "ON"),   

  DDkwd__(TRAF_AUTO_CREATE_SCHEMA,                 "OFF"),   

  DDkwd__(TRAF_BINARY_INPUT,                          "OFF"),
  DDkwd__(TRAF_BINARY_OUTPUT,                         "OFF"),
  DDkwd__(TRAF_BINARY_SPJ_SUPPORT,                    "OFF"),
  DDkwd__(TRAF_BINARY_SUPPORT,                        "OFF"),

  DDkwd__(TRAF_BLOB_AS_VARCHAR,                 "OFF"), //set to OFF to enable Lobs support  

  DDkwd__(TRAF_BOOLEAN_IO,                        "OFF"),

  DDkwd__(TRAF_BOOTSTRAP_MD_MODE,                            "OFF"),     

  DDkwd__(TRAF_CLOB_AS_VARCHAR,                 "OFF"), //set to OFF to enable Lobs support  

  DDkwd__(TRAF_COL_LENGTH_IS_CHAR,                 "ON"),   

  DDkwd__(TRAF_CREATE_SIGNED_NUMERIC_LITERAL,      "ON"),   

  DDansi_(TRAF_CREATE_TABLE_WITH_UID,          ""),

  DDkwd__(TRAF_CREATE_TINYINT_LITERAL,        "ON"),   

  DDkwd__(TRAF_DDL_ON_HIVE_OBJECTS,             "ON"),

  DDkwd__(TRAF_DEFAULT_COL_CHARSET,            (char *)SQLCHARSETSTRING_ISO88591),
 
  DDkwd__(TRAF_ENABLE_ORC_FORMAT,                 "OFF"),   

  DDkwd__(TRAF_HBASE_MAPPED_TABLES,             "ON"),   
  DDkwd__(TRAF_HBASE_MAPPED_TABLES_IUD,         "OFF"),   

  DDkwd__(TRAF_INDEX_ALIGNED_ROW_FORMAT,        "ON"),   
  DDkwd__(TRAF_INDEX_CREATE_OPT,          "OFF"),

  DDkwd__(TRAF_LARGEINT_UNSIGNED_IO,                        "OFF"),

  DDkwd__(TRAF_LOAD_ALLOW_RISKY_INDEX_MAINTENANCE,        "OFF"),
  DDkwd__(TRAF_LOAD_CONTINUE_ON_ERROR,          "OFF"),
  DD_____(TRAF_LOAD_ERROR_COUNT_ID,             "" ),
  DD_____(TRAF_LOAD_ERROR_COUNT_TABLE,          "ERRORCOUNTER" ),
  DD_____(TRAF_LOAD_ERROR_LOGGING_LOCATION,     "/user/trafodion/bulkload/logs" ),
  DDint__(TRAF_LOAD_FLUSH_SIZE_IN_KB,           "1024"),
  DDkwd__(TRAF_LOAD_FORCE_CIF,                  "ON"),
  DDkwd__(TRAF_LOAD_LOG_ERROR_ROWS,             "OFF"),
  DDint__(TRAF_LOAD_MAX_ERROR_ROWS,             "0"),
  DDint__(TRAF_LOAD_MAX_HFILE_SIZE,             "10240"), // in MB -->10GB by default

  DDkwd__(TRAF_LOAD_PREP_ADJUST_PART_FUNC,      "ON"),
  DDkwd__(TRAF_LOAD_PREP_CLEANUP,               "ON"),
  DDkwd__(TRAF_LOAD_PREP_KEEP_HFILES,           "OFF"),
  DDkwd__(TRAF_LOAD_PREP_SKIP_DUPLICATES ,      "OFF"),

  //need add code to check if folder exists or not. if not issue an error and ask
  //user to create it
  DD_____(TRAF_LOAD_PREP_TMP_LOCATION,                 "/user/trafodion/bulkload/" ),
  DDkwd__(TRAF_LOAD_TAKE_SNAPSHOT ,                    "OFF"),
  DDkwd__(TRAF_LOAD_USE_FOR_INDEXES,   "ON"),
  DDkwd__(TRAF_LOAD_USE_FOR_STATS,     "OFF"),

  DDkwd__(TRAF_MAKE_PKEY_COLUMNS_NOT_NULL,    "ON"),

  // max size in bytes of a char or varchar column. Set to 16M
  DDui___(TRAF_MAX_CHARACTER_COL_LENGTH,     MAX_CHAR_COL_LENGTH_IN_BYTES_STR),
  DDkwd__(TRAF_MAX_CHARACTER_COL_LENGTH_OVERRIDE,    "OFF"),
  // max size in MB of a row that canbe accomodated  in scanner cache when 
  // using th the default scanner cache size. 
  DDint__(TRAF_MAX_ROWSIZE_IN_CACHE,     "10"),

  DDkwd__(TRAF_MULTI_COL_FAM,     "ON"),

  DDkwd__(TRAF_NO_CONSTR_VALIDATION,                   "OFF"),

  DDkwd__(TRAF_NO_DTM_XN,      "OFF"),

  DDkwd__(TRAF_NO_HBASE_DROP_CREATE,                   "OFF"),

  DDint__(TRAF_NUM_HBASE_VERSIONS,                     "0"),

  DDint__(TRAF_NUM_OF_SALT_PARTNS,                     "-1"),

  DDkwd__(TRAF_READ_OBJECT_DESC,                       "OFF"),   

  DDkwd__(TRAF_RELOAD_NATABLE_CACHE,                   "OFF"),
  DD_____(TRAF_SAMPLE_TABLE_LOCATION,                  "/user/trafodion/sample/"),
  DDint__(TRAF_SEQUENCE_CACHE_SIZE,        "-1"),

  DDint__(TRAF_SEQUENCE_RETRY_TIMES,        "100"),

  DDkwd__(TRAF_SIMILARITY_CHECK,			"ROOT"),

  DDkwd__(TRAF_STORE_OBJECT_DESC,                    "OFF"),   

  DDkwd__(TRAF_STRING_AUTO_TRUNCATE,      "OFF"),
  DDkwd__(TRAF_STRING_AUTO_TRUNCATE_WARNING,      "OFF"),

  //TRAF_TABLE_SNAPSHOT_SCAN CQD can be set to :
  //NONE-->    Snapshot scan is disabled and regular scan is used , 
  //SUFFIX --> Snapshot scan enabled for the bulk unload (bulk unload 
  //           behavior id not changed)
  //LATEST --> enabled for the scan independently from bulk unload
  //           the latest snapshot is used if it exists
  DDkwd__(TRAF_TABLE_SNAPSHOT_SCAN,                    "NONE"),
  DD_____(TRAF_TABLE_SNAPSHOT_SCAN_SNAP_SUFFIX,        "SNAP"),
  //when the estimated table size is below the threshold (in MBs)
  //defined by  TRAF_TABLE_SNAPSHOT_SCAN_TABLE_SIZE_THRESHOLD
  //regular scan instead of snapshot scan
  //does not apply to bulk unload which maintains the old behavior
  DDint__(TRAF_TABLE_SNAPSHOT_SCAN_TABLE_SIZE_THRESHOLD, "1000"), 
  //timeout before we give up when trying to create the snapshot scanner
  DDint__(TRAF_TABLE_SNAPSHOT_SCAN_TIMEOUT,            "6000"),
  //location for temporary links and files produced by snapshot scan
  DD_____(TRAF_TABLE_SNAPSHOT_SCAN_TMP_LOCATION,       "/user/trafodion/bulkload/"),

  DDkwd__(TRAF_TINYINT_INPUT_PARAMS,                   "OFF"),
  DDkwd__(TRAF_TINYINT_RETURN_VALUES,                  "OFF"),
  DDkwd__(TRAF_TINYINT_SPJ_SUPPORT,                    "OFF"),
  DDkwd__(TRAF_TINYINT_SUPPORT,                        "ON"),

  // DTM Transaction Type: MVCC, SSCC
  XDDkwd__(TRAF_TRANS_TYPE,                            "MVCC"),

  DD_____(TRAF_UNLOAD_DEF_DELIMITER,                   "|" ),
  DD_____(TRAF_UNLOAD_DEF_RECORD_SEPARATOR,            "\n" ),
  DDint__(TRAF_UNLOAD_HDFS_COMPRESS,                   "0"),
  DDkwd__(TRAF_UPSERT_ADJUST_PARAMS,                   "OFF"),
  DDkwd__(TRAF_UPSERT_MODE,                            "MERGE"),
  DDkwd__(TRAF_UPSERT_TO_EFF_TREE,                     "ON"),
  DDint__(TRAF_UPSERT_WB_SIZE,                         "2097152"),
  DDkwd__(TRAF_UPSERT_WRITE_TO_WAL,                    "OFF"),

  DDkwd__(TRAF_USE_REGION_XN,                          "OFF"),

  DDkwd__(TRAF_USE_RWRS_FOR_MD_INSERT,                   "ON"),
  DDkwd__(TRANSLATE_ERROR,                             "ON"),
  DDkwd__(TRANSLATE_ERROR_UNICODE_TO_UNICODE,          "ON"),

  DDkwd__(TRY_DP2_REPARTITION_ALWAYS,		"OFF"),

 SDDkwd__(TRY_PASS_ONE_IF_PASS_TWO_FAILS,       "OFF"),

  // Disallow/Allow TSJs in MultiJoin framework
  DDkwd__(TSJS_SPOIL_JBB,       "OFF"),

  // type a CASE expression or ValueIdUnion as varchar if its leaves
  // are of type CHAR of unequal length
  DDkwd__(TYPE_UNIONED_CHAR_AS_VARCHAR,       "ON"),

  // UDF scalar indicating maximum number of rows out for each row in.
  DDui___(UDF_FANOUT,                            "1"),
  // Must be in form <cat>.<sch>.  Delimited catalog names not allowed.
  DD_____(UDF_METADATA_SCHEMA,                  "TRAFODION.\"_UDF_\""), 
                                                                         
  DDkwd__(UDF_SUBQ_IN_AGGS_AND_GBYS,            "SYSTEM"),

 XDDui___(UDR_DEBUG_FLAGS,                      "0"), // see sqludr/sqludr.h for values
 SDD_____(UDR_JAVA_OPTIONS,                     "OFF"),
  DD_____(UDR_JAVA_OPTION_DELIMITERS,           " "),
 XDDui___(UDR_JVM_DEBUG_PORT,                   "0"),
 XDDui___(UDR_JVM_DEBUG_TIMEOUT,                "0"),

  DDkwd__(UNAVAILABLE_PARTITION,		"STOP"),	// "?" used?
 DDkwd__(UNC_PROCESS,				"OFF"),
 SDDkwd__(UNIQUE_HASH_JOINS,                    "SYSTEM"),
 SDDui___(UNIQUE_HASH_JOIN_MAX_INNER_SIZE,      "1000"),
 SDDui___(UNIQUE_HASH_JOIN_MAX_INNER_SIZE_PER_INSTANCE, "100"),
 SDDui___(UNIQUE_HASH_JOIN_MAX_INNER_TABLES,    "2"),
  DDui___(UNOPTIMIZED_ESP_BUFFER_SIZE_DOWN,    "31000"),
  DDui___(UNOPTIMIZED_ESP_BUFFER_SIZE_UP,      "31000"),
  DDui1__(UPDATED_BYTES_PER_ESP,		"400000"),
  DDkwd__(UPDATE_CLUSTERING_OR_UNIQUE_INDEX_KEY,"ON"),

  DDkwd__(UPD_ABORT_ON_ERROR,                   "OFF"),

 XDDkwd__(UPD_ORDERED,				"ON"),

  DDkwd__(UPD_PARTIAL_ON_ERROR,                 "OFF"),
  DDkwd__(UPD_SAVEPOINT_ON_ERROR,               "ON"),

  DDkwd__(USER_EXPERIENCE_LEVEL,  "BEGINNER"),

  // ------------------------------------------------------------------------
  // This default will use a new type of an ASSERT, CCMPASSERT as a CMPASSERT
  // when ON, else use that as a DCMPASSERT. Changed this default to OFF
  // just before the final build for R2 07/23/2004 RV
  // -------------------------------------------------------------------------
  DDkwd__(USE_CCMPASSERT_AS_CMPASSERT,		"OFF"),


 // Use Hive tables as source for traf ustat and popindex
  DDkwd__(USE_HIVE_SOURCE,            ""),
  // Use large queues on RHS of Flow/Nested Join when appropriate
  DDkwd__(USE_LARGE_QUEUES,                     "ON"),


 XDDkwd__(USE_LIBHDFS,                          "OFF"),

  DDkwd__(USE_LIB_BLOB_STORE,                   "ON"),         

  DDkwd__(USE_MAINTAIN_CONTROL_TABLE,          "OFF"),

  DDkwd__(USE_OLD_DT_CONSTRUCTOR,      "OFF"),

  // Adaptive segmentation, use operator max to determine degree of parallelism
  DDui___(USE_OPERATOR_MAX_FOR_DOP,     "1"),

// Specify the number of partitions before invoking parallel label operations
  DDui1__(USE_PARALLEL_FOR_NUM_PARTITIONS,       "32"),

  DDkwd__(USTAT_ADD_SALTED_KEY_PREFIXES_FOR_MC, "ON"),   // When ON, generate MCs for primary key prefixes as well as full key
                                                         //   of salted table when ON EVERY KEY or ON EVERY COLUMN is specified.
  DDkwd__(USTAT_ATTEMPT_ESP_PARALLELISM,        "ON"),   // for reading column values
  DDkwd__(USTAT_AUTOMATIC_LOGGING,              "ON"),   // OFF gives same semantics as 
                                                         // UPDATE STATISTICS LOG OFF, while
                                                         // ON gives same semantics as 
                                                         // UPDATE STATISTICS LOG SYSTEM 
  DDui___(USTAT_AUTOMATION_INTERVAL,            "0"),
  DDkwd__(USTAT_AUTO_EMPTYHIST_TWO_TRANS,       "OFF"),  // When ON empty hist insert will be 2 trans.
  DDkwd__(USTAT_AUTO_FOR_VOLATILE_TABLES,       "OFF"),  // Toggle for vol tbl histogram usage
  DDui1__(USTAT_AUTO_MC_MAX_WIDTH,              "10"),   // The max columns in an MC histogram for automation.
  DDui___(USTAT_AUTO_MISSING_STATS_LEVEL,	"4"),    // Similar to HIST_MISSING_STATS_WARNING_LEVEL, but controls
                                                         // if automation inserts missing stats to HISTOGRAMS table.
                                                         // 0 - insert no stats,
                                                         // 1 - insert single col hists,
                                                         // 2 - insert all single col hists and MC hists for scans,
                                                         // 3 - insert all single col hists and MC stats for scans and joins.
                                                         // 4 - insert all single col hists and MC stats for scans, joins, and groupbys.
  DDui1__(USTAT_AUTO_READTIME_UPDATE_INTERVAL,  "86400"),  // Seconds between updates of READ_COUNT.
                                                 // Should be > CACHE_HISTOGRAMS_REFRESH_INTERVAL.

  DDkwd__(USTAT_COLLECT_FILE_STATS,             "ON"), // do we collect file stats

  DDkwd__(USTAT_COLLECT_MC_SKEW_VALUES,         "OFF"),

  DDkwd__(USTAT_COMPACT_VARCHARS,               "OFF"),  // If on, internal sort does not pad out varchars
  DD_____(USTAT_CQDS_ALLOWED_FOR_SPAWNED_COMPILERS, ""), // list of CQDs that can be pushed to seconday compilers
                                                         // CQDs are delimited by ","

  DD_____(USTAT_DEBUG_TEST,                     ""),
  DDkwd__(USTAT_DELETE_NO_ROLLBACK,             "ON"),   // If ON, use DELETE WITH NO ROLLBACK in IUS when updating sample table
  DDflte_(USTAT_DSHMAX,		                "50.0"),
  DDkwd__(USTAT_ESTIMATE_HBASE_ROW_COUNT,       "ON"),
  DDkwd__(USTAT_FETCHCOUNT_ACTIVE,              "OFF"),
  DDkwd__(USTAT_FORCE_MOM_ESTIMATOR,            "OFF"),
  DDkwd__(USTAT_FORCE_TEMP,                     "OFF"),
  DDflt0_(USTAT_FREQ_SIZE_PERCENT,              "0.5"),  // >100 effectively disables
  DDflt0_(USTAT_GAP_PERCENT,                    "10.0"),
  DDflt0_(USTAT_GAP_SIZE_MULTIPLIER,            "1.5"),
  DDui___(USTAT_HBASE_SAMPLE_RETURN_INTERVAL,   "10000000"), // Avoid scanner timeout by including on average at
                                                             //   least one row per this many when sampling within HBase.
  DDflt0_(USTAT_INCREMENTAL_FALSE_PROBABILITY,   "0.01"),
  DDkwd__(USTAT_INCREMENTAL_UPDATE_STATISTICS,   "SAMPLE"), // "SAMPLE" ==> don't use Counting Bloom Filters

  DDkwd__(USTAT_INTERNAL_SORT,                  "HYBRID"),

  DDkwd__(USTAT_IS_IGNORE_UEC_FOR_MC,           "OFF"),   // if MCIS is ON, use IS to compute SC stats
  DDflt_0_1(USTAT_IS_MEMORY_FRACTION,             "0.6"),
  DDflt0_(USTAT_IUS_INTERVAL_ROWCOUNT_CHANGE_THRESHOLD, "0.05"),
  DDflt0_(USTAT_IUS_INTERVAL_UEC_CHANGE_THRESHOLD, "0.05"),

  DDui1_6(USTAT_IUS_MAX_NUM_HASH_FUNCS,         "5"),

  // the max disk space IUS CBFs can use is 
  // MINOF(USTAT_IUS_MAX_PERSISTENT_DATA_IN_MB, 
  //       TtotalSpace * USTAT_IUS_MAX_PERSISTENT_DATA_IN_PERCENTAGE)
  DDui___(USTAT_IUS_MAX_PERSISTENT_DATA_IN_MB,        "50000"), // 50GB
  DDflt0_(USTAT_IUS_MAX_PERSISTENT_DATA_IN_PERCENTAGE,  "0.20"), // 20% of the total

  DDui1_6(USTAT_IUS_MAX_TRANSACTION_DURATION,  "5"),   // in minutes
  DDkwd__(USTAT_IUS_NO_BLOCK,                   "OFF"),
  DDansi_(USTAT_IUS_PERSISTENT_CBF_PATH,        "SYSTEM"),

  DDflt0_(USTAT_IUS_TOTAL_ROWCOUNT_CHANGE_THRESHOLD, "0.05"),
  DDflt0_(USTAT_IUS_TOTAL_UEC_CHANGE_THRESHOLD, "0.05"),
  DDkwd__(USTAT_IUS_USE_PERIODIC_SAMPLING,        "OFF"),


  DDkwd__(USTAT_JIT_LOGGING,                    "OFF"),
  DD_____(USTAT_LOG,                            ""),   // if non-empty, gives second qualifier to ustat log file name
  DDui30_246(USTAT_MAX_CHAR_BOUNDARY_LEN,       "30"),   // Values can be 30-246.
  DDui___(USTAT_MAX_CHAR_COL_LENGTH_IN_BYTES,   "256"),  // When computing UECs, char cols are limited to this many bytes
  DDflt0_   (USTAT_MAX_CHAR_DATASIZE_FOR_IS,    "1000"),  // max data size in MB for char type to use 
 XDDui___(USTAT_MAX_READ_AGE_IN_MIN,            "5760"),

                                                         // internal sort without checking UEC.
  DDflt0_(USTAT_MIN_CHAR_UEC_FOR_IS,            "0.2"),  // minimum UEC for char type to use internal sort
  DDflt0_(USTAT_MIN_DEC_BIN_UEC_FOR_IS,         "0.0"),  // minimum UEC for binary types to use internal sort

 DDflt0_(USTAT_MIN_ESTIMATE_FOR_ROWCOUNT,      "10000000"),
 XDDui1__(USTAT_MIN_ROWCOUNT_FOR_LOW_SAMPLE,    "1000000"),
 XDDui1__(USTAT_MIN_ROWCOUNT_FOR_SAMPLE,        "10000"),
  DDflt0_(USTAT_MODIFY_DEFAULT_UEC,             "0.05"),
  DDui1__(USTAT_MULTI_COLUMN_LIMIT,             "10"),
  DDflt0_(USTAT_NAHEAP_ESTIMATED_MAX,           "1.3"),  // estimated max memory allocation (in GB) feasible with NAHEAP.
  DDui1__(USTAT_NUM_MC_GROUPS_FOR_KEYS,         "10"),
  DDkwd__(USTAT_PROCESS_GAPS,                   "ON"),
  DD0_255(USTAT_RETRY_DELAY,                    "100"),
  DD0_255(USTAT_RETRY_LIMIT,                    "3"),
  DD0_255(USTAT_RETRY_NEC_COLS_LIMIT,           "3"),       // by default, use retry for AddNecessaryColumns
  DDpct__(USTAT_SAMPLE_PERCENT_DIFF,            "10"),
  DDansi_(USTAT_SAMPLE_TABLE_NAME,              " "),
  DDansi_(USTAT_SAMPLE_TABLE_NAME_CREATE,       " "),
  DDkwd__(USTAT_SHOW_MC_INTERVAL_INFO,          "OFF"),
  DDkwd__(USTAT_SHOW_MFV_INFO,                  "OFF"),
  DDflte_(USTAT_UEC_HI_RATIO,                   "0.5"),
  DDflte_(USTAT_UEC_LOW_RATIO,                  "0.1"),
  DDkwd__(USTAT_USE_BACKING_SAMPLE,             "OFF"),
  DDkwd__(USTAT_USE_BULK_LOAD,                  "OFF"),
  DDkwd__(USTAT_USE_GROUPING_FOR_SAMPLING,      "ON"),
  DDkwd__(USTAT_USE_INTERNAL_SORT_FOR_MC,       "ON"),
  DDkwd__(USTAT_USE_IS_WHEN_NO_STATS,           "ON"), // use IS when no histograms exist for the column
  DDkwd__(USTAT_USE_SLIDING_SAMPLE_RATIO,       "ON"), // Trend sampling rate down w/increasing table size, going
                                                       //   flat at 1%.
 XDDflt1_(USTAT_YOULL_LIKELY_BE_SORRY,          "100000000"),  // guard against unintentional long-running UPDATE STATS

  DDkwd__(VALIDATE_VIEWS_AT_OPEN_TIME,		"OFF"),

  //this is the default length of a param which is typed as a VARCHAR.
  DD1_4096(VARCHAR_PARAM_DEFAULT_SIZE,		"255"),

  // allows pcodes for varchars
  DDkwd__(VARCHAR_PCODE,                        "ON"),

  DDansi_(VOLATILE_CATALOG,			""),

  DDkwd__(VOLATILE_SCHEMA_IN_USE,		"OFF"),

  // if this is set to ON or SYSTEM, then find a suitable key among all the
  // columns of a volatile table.
  // If this is set to OFF, and there is no user specified primary key or
  // store by clause, then make the first column of the volatile table
  // to be the clustering key.
  DDkwd__(VOLATILE_TABLE_FIND_SUITABLE_KEY,     "SYSTEM"),


  DDkwd__(VSBB_TEST_MODE,			"OFF"),
 XDDkwd__(WMS_CHILD_QUERY_MONITORING,                       "OFF"),
 XDDkwd__(WMS_QUERY_MONITORING,                       "OFF"),
  // amount of work we are willing to assign per CPU for any query
  // not running at full system parallelism
 SDDflte_(WORK_UNIT_ESP,      "0.08"),
 SDDflte_(WORK_UNIT_ESP_DATA_COPY_COST, "0.001"),
 
  // ZIG_ZAG_TREES ON means do ZIG_ZAG_TREES
  // $$$ OFF for beta
  DDkwd__(ZIG_ZAG_TREES,			"SYSTEM"),
  DDkwd__(ZIG_ZAG_TREES_CONTROL,		"OFF")

};

//
// NOTE: The  defDefIx_ array  is an array of integers that map
//       'enum' values to defaultDefaults[] entries.
//       The  defDefIx_ array  could probably be made global static
//       since all threads should map the same 'enum' values to the
//       same defaultDefaults[] entries.  Such as change is being
//       left to a future round of optimizations.
//
static THREAD_P size_t defDefIx_[__NUM_DEFAULT_ATTRIBUTES];

inline static const char *getAttrName(Int32 attrEnum)
{
  return defaultDefaults[defDefIx_[attrEnum]].attrName;
}

inline static const char *getDefaultDefaultValue(Int32 attrEnum)
{
  return defaultDefaults[defDefIx_[attrEnum]].value;
}

inline static const DefaultValidator *validator(Int32 attrEnum)
{
  return defaultDefaults[defDefIx_[attrEnum]].validator;
}

inline static UInt32 getFlags(Int32 attrEnum)
{
  return defaultDefaults[defDefIx_[attrEnum]].flags;
}

inline static NABoolean isFlagOn(Int32 attrEnum, NADefaultFlags flagbit)
{
  return defaultDefaults[defDefIx_[attrEnum]].flags & (UInt32)flagbit;
}

inline static void setFlagOn(Int32 attrEnum, NADefaultFlags flagbit)
{
  defaultDefaults[defDefIx_[attrEnum]].flags |= (UInt32)flagbit;
}

static NABoolean isSynonymOfRESET(NAString &value)
{
  return (value == "RESET");
}

static NABoolean isSynonymOfSYSTEM(Int32 attrEnum, NAString &value)
{
  if (value == "")
    return TRUE;
  if (value == "SYSTEM")
    return !isFlagOn(attrEnum, DEFAULT_ALLOWS_SEPARATE_SYSTEM);
  if (value == "ENABLE"){
    value = "ON";
	return FALSE;
	}
  else if (value == "DISABLE"){
    value = "OFF";
	return FALSE;
	}
 //   if (getDefaultDefaultValue(attrEnum) != NAString("DISABLE"))  // cast reqd!!
 //     return TRUE;
 //   else
 //     value = "ON";
  return FALSE;
}

// Helper class used for holding and restoring CQDs
class NADefaults::HeldDefaults
{
  public:
   
    HeldDefaults(void);

    ~HeldDefaults(void);

    // CMPASSERT's on stack overflow
    void pushDefault(const char * value);

    // returns null if nothing to pop
    char * popDefault(void);

  private:

    enum { STACK_SIZE = 3 };

    int stackPointer_;
    char * stackValue_[STACK_SIZE];
    
};

// Methods for helper class HeldDefaults
NADefaults::HeldDefaults::HeldDefaults(void) : stackPointer_(0)
{            
  for (int i = 0; i < STACK_SIZE; i++)
    stackValue_[i] = NULL;
}

NADefaults::HeldDefaults::~HeldDefaults(void)
{
  for (int i = 0; i < STACK_SIZE; i++)
  {
    if (stackValue_[i])
    {
      NADELETEBASIC(stackValue_[i], NADHEAP);
    }
  }
}

// CMPASSERT's on stack overflow
void NADefaults::HeldDefaults::pushDefault(const char * value)
{
  CMPASSERT(stackPointer_ < STACK_SIZE);
  stackValue_[stackPointer_] = new NADHEAP char[strlen(value) + 1];
  strcpy(stackValue_[stackPointer_],value);
  stackPointer_++;
}

// returns null if nothing to pop
char * NADefaults::HeldDefaults::popDefault(void)
{
  char * result = 0;
  if (stackPointer_ > 0)
  {
    stackPointer_--;
    result = stackValue_[stackPointer_];
    stackValue_[stackPointer_] = NULL;
  }
  return result;
}


size_t NADefaults::numDefaultAttributes()
{
  return (size_t)__NUM_DEFAULT_ATTRIBUTES;
}

// Returns current defaults in alphabetic order (for SHOWCONTROL listing).
const char *NADefaults::getCurrentDefaultsAttrNameAndValue(
		     size_t ix, const char* &name, const char* &value,
		     NABoolean userDefaultsOnly)
{
  if (ix < numDefaultAttributes()) {
    NABoolean get = FALSE;
    if (userDefaultsOnly)
      {
	// if this default was entered by user, return it.
	get = userDefault(defaultDefaults[ix].attrEnum);
      }
    else
      {
        // display the control if
        // - it is externalized or
        // - it is for support only and a CQD is set to show those, or
        // - a CQD is set to show all the controls
	get =
	  (defaultDefaults[ix].flags & DEFAULT_IS_EXTERNALIZED) || // bit-AND
	  ((defaultDefaults[ix].flags & DEFAULT_IS_FOR_SUPPORT) &&
           (getToken(SHOWCONTROL_SHOW_SUPPORT) == DF_ON)) ||
	  (getToken(SHOWCONTROL_SHOW_ALL) == DF_ON);
      }
    if (get) {
      name  = defaultDefaults[ix].attrName;
      value = currentDefaults_[defaultDefaults[ix].attrEnum];
      return name;
    }
  }
  return name = value = NULL;
}

// -----------------------------------------------------------------------
// convert the default defaults into a table organized by enum values
// -----------------------------------------------------------------------
void NADefaults::initCurrentDefaultsWithDefaultDefaults()
{
  deleteMe();


  const size_t numAttrs = numDefaultAttributes();
  if (numAttrs != sizeof(defaultDefaults) / sizeof(DefaultDefault))
    return;

  CMPASSERT_STRING
    (numAttrs == sizeof(defaultDefaults) / sizeof(DefaultDefault),
     "Check sqlcomp/DefaultConstants.h for a gap in enum DefaultConstants or sqlcomp/nadefaults.cpp for duplicate entries in array defaultDefaults[].");

  SqlParser_NADefaults_Glob =
  SqlParser_NADefaults_ = new NADHEAP SqlParser_NADefaults();

  provenances_		= new NADHEAP char [numAttrs];   // enum fits in 2 bits
  flags_		= new NADHEAP char [numAttrs];
  resetToDefaults_	= new NADHEAP char * [numAttrs];
  currentDefaults_	= new NADHEAP const char * [numAttrs];
  currentFloats_	= new NADHEAP float * [numAttrs];
  currentTokens_	= new NADHEAP DefaultToken * [numAttrs];
  currentState_		= INIT_DEFAULT_DEFAULTS;
  heldDefaults_	        = new NADHEAP HeldDefaults * [numAttrs];

  // reset all entries
  size_t i = 0;
  for (i = 0; i < numAttrs; i++)
    {
      provenances_[i]	   = currentState_;
      flags_[i]            = 0;
      defDefIx_[i]	   = 0;
    }

  memset( resetToDefaults_, 0, sizeof(char *) * numAttrs );
  memset( currentDefaults_, 0, sizeof(char *) * numAttrs );
  memset( currentFloats_, 0, sizeof(float *) * numAttrs );
  memset( currentTokens_, 0, sizeof(DefaultToken *) * numAttrs );
  memset( heldDefaults_, 0, sizeof(HeldDefaults *) * numAttrs );

  #ifndef NDEBUG
    // This env-var turns on consistency checking of default-defaults and
    // other static info.  The env-var does not get passed from sqlci to arkdev
    // until *AFTER* the initialization code runs, so you must do a static
    // arkcmp compile to do this checking.  TEST050 does this, in fact.
    NABoolean nadval = !!getenv("NADEFAULTS_VALIDATE");
  #endif

  // for each entry of the (alphabetically sorted) default defaults
  // table, enter the default default into the current default table
  // which is sorted by enum values
  NAString prevAttrName;
  for (i = 0; i < numAttrs; i++)
    {
      // the enum must be less than the max (if this assert fails
      // you might have made the range of constants in the enum
      // non-contiguous by assigning hard-coded numbers to some entries)
      CMPASSERT(ENUM_RANGE_CHECK(defaultDefaults[i].attrEnum));

      // can't have the same enum value twice in defaultDefaults
      CMPASSERT(currentDefaults_[defaultDefaults[i].attrEnum] == NULL);

      // set currentDefaults_[enum] to the static string,
      // leaving the "allocated from heap" flag as FALSE
      char * value = new NADHEAP char[strlen(defaultDefaults[i].value) + 1];
      strcpy(value,defaultDefaults[i].value);

      // trim trailing spaces (except UDR_JAVA_OPTION_DELIMITERS, since
      // trailing space is allowed for it)
      if (defaultDefaults[i].attrEnum != UDR_JAVA_OPTION_DELIMITERS)
      {
        Lng32 len = strlen(value);
        while ((len > 0) && (value[len-1] == ' '))
	{
	  value[len-1] = 0;
	  len--;
	}
      }

      currentDefaults_[defaultDefaults[i].attrEnum] = value;


      // set up our backlink which maps [enum] to its defaultDefaults entry
      defDefIx_[defaultDefaults[i].attrEnum] = i;

      // attrs must be in ascending sorted order. If not, error out.
      if (prevAttrName > defaultDefaults[i].attrName)
        {
          SqlParser_NADefaults_ = NULL;

          return;
        }
      prevAttrName = defaultDefaults[i].attrName;

      // validate initial default default values
      CMPASSERT(defaultDefaults[i].validator);
      if (! defaultDefaults[i].validator->validate(
               defaultDefaults[i].value,
               this,
               defaultDefaults[i].attrEnum,
               +1/*warning*/))
        {
          SqlParser_NADefaults_ = NULL;

          cerr << "\nERROR: " << defaultDefaults[i].attrName
               << " has invalid value" << defaultDefaults[i].value << endl;

          return;
         }

      // for debugging only
      #ifndef NDEBUG
       if (nadval) {	// additional sanity checking we want to do occasionally

	  NAString v;

	  // ensure the static table really is in alphabetic order
	  CMPASSERT(i == 0 ||
		    strcmp(defaultDefaults[i-1].attrName,
			   defaultDefaults[i].attrName) < 0);

	  // ensure these names are fit and trim and in canonical form
	  v = defaultDefaults[i].attrName;
	  TrimNAStringSpace(v);
	  v.toUpper();
	  CMPASSERT(v == defaultDefaults[i].attrName);

	  // validate initial default default values
	  CMPASSERT(defaultDefaults[i].validator);
	  defaultDefaults[i].validator->validate(
					    defaultDefaults[i].value,
					    this,
					    defaultDefaults[i].attrEnum,
					    +1/*warning*/);

	  // ensure these values are fit and trim and in canonical form
	  v = defaultDefaults[i].value;
	  TrimNAStringSpace(v);
	  defaultDefaults[i].validator->applyUpper(v);
	  CMPASSERT(v == defaultDefaults[i].value);

	  // alert the programmer
	  if (isSynonymOfSYSTEM(defaultDefaults[i].attrEnum, v))
	    if (v != "" || defaultDefaults[i].validator != &validateAnsiName)
	      cerr << "\nWARNING: " << defaultDefaults[i].attrName
		   << " has SYSTEM default ("
		   << v << ");\n\t read NOTE 2 in " << __FILE__ << endl;
	  if (isSynonymOfRESET(v))
	    if (v != "" || defaultDefaults[i].validator != &validateAnsiName)
	      cerr << "\nWARNING: " << defaultDefaults[i].attrName
		   << " has RESET default ("
		   << v << ");\n\t this makes no sense!" << endl;
	  if (defaultDefaults[i].validator == &validateUnknown)
	      cerr << "\nWARNING: " << defaultDefaults[i].attrName
		   << " has a NO-OP validator" << endl;

	  // the token keyword array must have no missing strings,
	  // it must also be in alphabetic order,
	  // each entry must be canonical, and
	  // must have no embedded spaces (see token() method, space/uscore...)
	  if (i == 0)
	    for (size_t j = 0; j < DF_lastToken; j++) {
	      CMPASSERT(keywords_[j]);
	      CMPASSERT(j == 0 || strcmp(keywords_[j-1], keywords_[j]) < 0);

	      NAString v(keywords_[j]);
	      TrimNAStringSpace(v);
	      v.toUpper();		// we know keywords must be caseINsens
	      CMPASSERT(v == keywords_[j]);

	      CMPASSERT(v.first(' ') == NA_NPOS);

	    }
	}	// if env-var
      #endif	// NDEBUG

    } // for i

  // set the default value for GENERATE_EXPLAIN depending on whether
  // this is a static compile or a dynamic compile.
  if (CmpCommon::context()->GetMode() == STMT_STATIC)
    {
      currentDefaults_[GENERATE_EXPLAIN] = "ON";
    }
  else
    {
      currentDefaults_[GENERATE_EXPLAIN] = "OFF";
      currentDefaults_[DETAILED_STATISTICS] = "OPERATOR";
    }

  // set the default value of hive_catalog to the hive_system_catalog
  currentDefaults_[HIVE_CATALOG] = HIVE_SYSTEM_CATALOG;

  // set the default value of hbase_catalog to the hbase_system_catalog
  currentDefaults_[HBASE_CATALOG] = HBASE_SYSTEM_CATALOG;
  currentDefaults_[SEABASE_CATALOG] = TRAFODION_SYSCAT_LIT;

  // Test for TM_USE_SSCC from ms.env.
  // Only a setting of TM_USE_SSCC set to 1 will change the value to SSCC.
  // Otherwise, the default will remain at MVCC.
  char * ev = getenv("TM_USE_SSCC");
  Lng32 useValue = 0;

  if (ev) {
    useValue = (Lng32)str_atoi(ev, str_len(ev));
    if (useValue == 1)
      currentDefaults_[TRAF_TRANS_TYPE] = "SSCC";
  }

// Begin: Temporary workaround for SQL build regressions to pass
  NABoolean resetNeoDefaults = FALSE;
    // On SQ, the way to get an envvar from inside a un-attached process
    // is to use the msg_getenv_str() call and set the env inside
    // the SQ_PROP_ property file. In this case the property
    // file is $TRAF_CONF/tdm_arkcmp.env which contains the line
    // "SQLMX_REGRESS=1". This file was generated by tools/setuplnxenv.
   //    resetNeoDefaults = (msg_getenv_str("SQLMX_REGRESS") != NULL);
    resetNeoDefaults = (getenv("SQLMX_REGRESS") != NULL);

  if(resetNeoDefaults)
  {

    // turn on INTERNAL format for SHOWDDL statements
    currentDefaults_[SHOWDDL_DISPLAY_FORMAT] = "INTERNAL";
  }
// End: Temporary workaround for SQL build regressions to pass

  // Cache all the default keywords up front,
  // leaving other non-keyword token to be cached on demand.
  // The "keyword" that is not cached is the kludge/clever trick that
  // Matt puts in for NATIONAL_CHARSET.
  NAString tmp( NADHEAP );
  for ( i = 0; i < numAttrs; i++ )
  {
#ifndef NDEBUG
    const DefaultValidatorType validatorType = validator(i)->getType();
#endif

    if ( validator(i)->getType() == VALID_KWD && (i != NATIONAL_CHARSET) &&
         (i != INPUT_CHARSET) && (i != ISO_MAPPING) )
    {
      currentTokens_[i] = new NADHEAP DefaultToken;

      // do not call 'token' method as it will return an error if FALSE
      // is to be inserted. Just directly assign DF_OFF to non-resetable defs.
      if (isNonResetableAttribute(defaultDefaults[defDefIx_[i]].attrName))
	*currentTokens_[i] = DF_OFF;
      else
	*currentTokens_[i] = token( i, tmp );
    }
  }

  if (getToken(MODE_SEABASE) == DF_ON)
    {
      currentDefaults_[CATALOG] = TRAFODION_SYSCAT_LIT;

      if (getToken(SEABASE_VOLATILE_TABLES) == DF_ON)
	{
	  NAString sbCat = getValue(SEABASE_CATALOG);
	  CmpCommon::context()->sqlSession()->setVolatileCatalogName(sbCat, TRUE);
	}
    }

  SqlParser_NADefaults_->NAMETYPE_		= getToken(NAMETYPE);
  SqlParser_NADefaults_->NATIONAL_CHARSET_	=
    CharInfo::getCharSetEnum(currentDefaults_[NATIONAL_CHARSET]);
  SqlParser_NADefaults_->ISO_MAPPING_      =
    CharInfo::getCharSetEnum(currentDefaults_[ISO_MAPPING]);
  SqlParser_NADefaults_->DEFAULT_CHARSET_      =
    CharInfo::getCharSetEnum(currentDefaults_[DEFAULT_CHARSET]);
  SqlParser_NADefaults_->ORIG_DEFAULT_CHARSET_      =
    CharInfo::getCharSetEnum(currentDefaults_[DEFAULT_CHARSET]);

  // Set the NAString_isoMappingCS memory cache for use by routines
  // ToInternalIdentifier() and ToAnsiIdentifier[2|3]() in module
  // w:/common/NAString[2].cpp.  These routines currently cannot
  // access SqlParser_ISO_MAPPING directly due to the complex
  // build hierarchy.
  NAString_setIsoMapCS((SQLCHARSET_CODE) SqlParser_NADefaults_->ISO_MAPPING_);

}

NADefaults::NADefaults(NAMemory * h)
  : provenances_(NULL)
  , flags_(NULL)
  , resetToDefaults_(NULL)
  , currentDefaults_(NULL)
  , currentFloats_(NULL)
  , currentTokens_(NULL)
  , heldDefaults_(NULL)
  , currentState_(UNINITIALIZED)
  , readFromSQDefaultsTable_(FALSE)
  , SqlParser_NADefaults_(NULL)
  , catSchSetToUserID_(0)
  , heap_(h)
  , resetAll_(FALSE)
  , defFlags_(0)
  , tablesRead_(h)
{
  static THREAD_P NABoolean systemParamterUpdated = FALSE;
  // First (but only if NSK-LITE Services exist),
  // write system parameters (attributes DEF_*) into DefaultDefaults,
  if (!systemParamterUpdated && !cmpCurrentContext->isStandalone())
  {
     updateSystemParameters();
     systemParamterUpdated = TRUE;
  }

  // then copy DefaultDefaults into CurrentDefaults.
  initCurrentDefaultsWithDefaultDefaults();

  // Set additional defaultDefaults flags:

  // If an attr allows ON/OFF/SYSTEM and the default-default is not SYSTEM,
  // then you must set this flag.  Otherwise, CQD attr 'system' will revert
  // the value back to the default-default, which is not SYSTEM.
  //	setFlagOn(...attr..., DEFAULT_ALLOWS_SEPARATE_SYSTEM);
  //
  // (See attESPPara in OptPhysRelExpr.cpp.)
  setFlagOn(ATTEMPT_ESP_PARALLELISM, DEFAULT_ALLOWS_SEPARATE_SYSTEM);
  setFlagOn(HJ_TYPE, DEFAULT_ALLOWS_SEPARATE_SYSTEM);
  setFlagOn(ZIG_ZAG_TREES, DEFAULT_ALLOWS_SEPARATE_SYSTEM);
  setFlagOn(COMPRESSED_INTERNAL_FORMAT, DEFAULT_ALLOWS_SEPARATE_SYSTEM);
  setFlagOn(COMPRESSED_INTERNAL_FORMAT_BMO, DEFAULT_ALLOWS_SEPARATE_SYSTEM);
  setFlagOn(HBASE_SMALL_SCANNER, DEFAULT_ALLOWS_SEPARATE_SYSTEM);
}

NADefaults::~NADefaults()
{
  deleteMe();
}

void NADefaults::deleteMe()
{
  if (resetToDefaults_) {
    for (size_t i = numDefaultAttributes(); i--; )
      NADELETEBASIC(resetToDefaults_[i], NADHEAP);
    NADELETEBASIC(resetToDefaults_, NADHEAP);
  }

  if (currentDefaults_) {
    for (size_t i = numDefaultAttributes(); i--; )
      if (provenances_[i] > INIT_DEFAULT_DEFAULTS)
	NADELETEBASIC(currentDefaults_[i], NADHEAP);
    NADELETEBASIC(currentDefaults_, NADHEAP);
  }

  if (currentFloats_) {
    for (size_t i = numDefaultAttributes(); i--; )
      NADELETEBASIC(currentFloats_[i], NADHEAP);
    NADELETEBASIC(currentFloats_, NADHEAP);
  }

  if (currentTokens_) {
    for (size_t i = numDefaultAttributes(); i--; )
      NADELETEBASIC(currentTokens_[i], NADHEAP);
    NADELETEBASIC(currentTokens_, NADHEAP);
  }

  if (heldDefaults_) {
    for (size_t i = numDefaultAttributes(); i--; )
      NADELETE(heldDefaults_[i], HeldDefaults, NADHEAP);
    NADELETEBASIC(heldDefaults_, NADHEAP);
  }

  for (CollIndex i = tablesRead_.entries(); i--; )
    tablesRead_.removeAt(i);

  NADELETEBASIC(provenances_, NADHEAP);
  NADELETEBASIC(flags_, NADHEAP);
  NADELETE(SqlParser_NADefaults_, SqlParser_NADefaults, NADHEAP);
}

// -----------------------------------------------------------------------
// Find the attribute name from its enum value in the defaults table.
// -----------------------------------------------------------------------
const char *NADefaults::lookupAttrName(Int32 attrEnum,
				       Int32 errOrWarn)
{
  if (ATTR_RANGE_CHECK) return getAttrName(attrEnum);

  static THREAD_P char noSuchAttr[20];
  sprintf(noSuchAttr, "**%d**", attrEnum);
  if (errOrWarn)  // $0~string0 is not the name of any DEFAULTS table attribute.
    *CmpCommon::diags() << DgSqlCode(ERRWARN(2050)) << DgString0(noSuchAttr);
  return noSuchAttr;
}

// -----------------------------------------------------------------------
// Find the enum value from its string representation in the defaults table.
// -----------------------------------------------------------------------
enum DefaultConstants NADefaults::lookupAttrName(const char *name,
						 Int32 errOrWarn,
						 Int32 *position)
{
  NAString attrName(name);
  TrimNAStringSpace(attrName, FALSE, TRUE);	// trim trailing blanks only
  attrName.toUpper();

  // start with the full range of defaultDefaults
  size_t lo = 0;
  size_t hi = numDefaultAttributes();
  size_t split;
  Int32 cresult;

  // perform a binary search in the ordered table defaultDefaults
  do
    {
      // compare the token with the middle entry in the range
      split = (lo + hi) / 2;
      cresult = attrName.compareTo(defaultDefaults[split].attrName);

      if (cresult < 0)
	{
	  // token < split value, search first half of range
	  hi = split;
	}
      else if (cresult > 0)
	{
	  if (lo == split)	// been there, done that
	    {
	      CMPASSERT(lo == hi-1);
	      break;
	    }
	  // token > split value, search second half of range
	  lo = split;
	}
    }
  while (cresult != 0 && lo < hi);

  if (position != 0)
    *position = split;

  // if the last comparison result was equal, return value at "split"
  if (cresult == 0)
    return defaultDefaults[split].attrEnum;

  // otherwise the string has no corresponding enum value
  if (errOrWarn)  // $0~string0 is not the name of any DEFAULTS table attribute.
    *CmpCommon::diags() << DgSqlCode(ERRWARN(2050)) << DgString0(attrName);
  return __INVALID_DEFAULT_ATTRIBUTE;	// negative
}

#define WIDEST_CPUARCH_VALUE 30	// also wider than any utoa_() result

static void utoa_(UInt32 val, char *buf)  { sprintf(buf, "%u", val); }

static void itoa_(Int32 val, char *buf)	   { sprintf(buf, "%d", val); }

static void ftoa_(float val, char *buf)
{
   snprintf(buf, WIDEST_CPUARCH_VALUE, "%0.2f", val);
}


// Updates the system parameters in the defaultDefaults table.
void NADefaults::updateSystemParameters(NABoolean reInit)
{

  static const char *arrayOfSystemParameters[] = {
      "DEF_CPU_ARCHITECTURE",
      "DEF_DISCS_ON_CLUSTER",
      "DEF_INSTRUCTIONS_SECOND",
      "DEF_PAGE_SIZE",
      "DEF_LOCAL_CLUSTER_NUMBER",
      "DEF_LOCAL_SMP_NODE_NUMBER",
      "DEF_NUM_SMP_CPUS",
      "MAX_ESPS_PER_CPU_PER_OP",
      "DEFAULT_DEGREE_OF_PARALLELISM",
      "DEF_NUM_NODES_IN_ACTIVE_CLUSTERS",
      // this is deliberately not in the list:  "DEF_CHUNK_SIZE",
      "DEF_NUM_BM_CHUNKS",
      "DEF_PHYSICAL_MEMORY_AVAILABLE", //returned in KB not bytes
      "DEF_TOTAL_MEMORY_AVAILABLE",		 //returned in KB not bytes
      "DEF_VIRTUAL_MEMORY_AVAILABLE"
      , "USTAT_IUS_PERSISTENT_CBF_PATH"
   }; //returned in KB not bytes

  char valuestr[WIDEST_CPUARCH_VALUE];

  //  Set up global cluster information.
  setUpClusterInfo(CmpCommon::contextHeap());

  //  Extract SMP node number and cluster number where this arkcmp is running.
  short nodeNum = 0;
  Int32   clusterNum = 0;
  OSIM_getNodeAndClusterNumbers(nodeNum, clusterNum);

  // First (but only if NSK-LITE Services exist),
  // write system parameters (attributes DEF_*) into DefaultDefaults,
  // then copy DefaultDefaults into CurrentDefaults.
  if (!cmpCurrentContext->isStandalone())  {

  size_t numElements = sizeof(arrayOfSystemParameters) / sizeof(char *);
  for (size_t i = 0; i < numElements; i++) {

    Int32 j;
    // perform a lookup for the string, using a binary search
    lookupAttrName(arrayOfSystemParameters[i], -1, &j);

    CMPASSERT(j >= 0);

    if(reInit)
      NADELETEBASIC(defaultDefaults[j].value,NADHEAP);
    char *newValue = new (GetCliGlobals()->exCollHeap()) char[WIDEST_CPUARCH_VALUE];
    newValue[0] = '\0';
    defaultDefaults[j].value = newValue;

    switch(defaultDefaults[j].attrEnum) {

    case DEF_CPU_ARCHITECTURE:

      switch(gpClusterInfo->cpuArchitecture()) {
				       // 123456789!1234567890@123456789
      case CPU_ARCH_INTEL_80386:
	strcpy(newValue, "INTEL_80386");
	break;

      case CPU_ARCH_INTEL_80486:
	strcpy(newValue, "INTEL_80486");
	break;

      case CPU_ARCH_PENTIUM:
	strcpy(newValue, "PENTIUM");
	break;

      case CPU_ARCH_PENTIUM_PRO:
	strcpy(newValue, "PENTIUM_PRO");
	break;

      case CPU_ARCH_MIPS:
	strcpy(newValue, "MIPS");
	break;

      case CPU_ARCH_ALPHA:
	strcpy(newValue, "ALPHA");
	break;

      case CPU_ARCH_PPC:
	strcpy(newValue, "PPC");
	break;

      default:
	strcpy(newValue, "UNKNOWN");
	break;
      }
      if(reInit)
        ActiveSchemaDB()->
          getDefaults().
            updateCurrentDefaultsForOSIM(&defaultDefaults[j], FALSE);
      break;


    case DEF_DISCS_ON_CLUSTER:
      strcpy(newValue, "8");
      if(reInit)
        ActiveSchemaDB()->
          getDefaults().
            updateCurrentDefaultsForOSIM(&defaultDefaults[j]);
      break;

    case DEF_PAGE_SIZE:
      utoa_(gpClusterInfo->pageSize(), valuestr);
      strcpy(newValue, valuestr);
      if(reInit)
        ActiveSchemaDB()->
          getDefaults().
            updateCurrentDefaultsForOSIM(&defaultDefaults[j]);
      break;

    case DEF_LOCAL_CLUSTER_NUMBER:
      utoa_(clusterNum, valuestr);
      strcpy(newValue, valuestr);
      if(reInit)
        ActiveSchemaDB()->
          getDefaults().
            updateCurrentDefaultsForOSIM(&defaultDefaults[j]);
      break;

    case DEF_LOCAL_SMP_NODE_NUMBER:
      utoa_(nodeNum, valuestr);
      strcpy(newValue, valuestr);
      if(reInit)
        ActiveSchemaDB()->
          getDefaults().
            updateCurrentDefaultsForOSIM(&defaultDefaults[j]);
      break;

    case DEF_NUM_SMP_CPUS:
      utoa_(gpClusterInfo->numberOfCpusPerSMP(), valuestr);
      strcpy(newValue, valuestr);
      if(reInit)
        ActiveSchemaDB()->
          getDefaults().
            updateCurrentDefaultsForOSIM(&defaultDefaults[j]);
      break;

    case DEFAULT_DEGREE_OF_PARALLELISM:
      {
        Lng32 x = 2;

	utoa_(x, valuestr);
	strcpy(newValue, valuestr);
	if(reInit)
	  ActiveSchemaDB()->
	    getDefaults().
            updateCurrentDefaultsForOSIM(&defaultDefaults[j]);
      }
      break;

    case MAX_ESPS_PER_CPU_PER_OP:
      {
        float espsPerCore = computeNumESPsPerCore(FALSE);
        ftoa_(espsPerCore, valuestr);
        strcpy(newValue, valuestr);
        if(reInit)
          ActiveSchemaDB()->
            getDefaults().
            updateCurrentDefaultsForOSIM(&defaultDefaults[j]);
      }
      break;

    case DEF_NUM_NODES_IN_ACTIVE_CLUSTERS:

      utoa_(gpClusterInfo->numOfPhysicalSMPs(), valuestr);
      strcpy(newValue, valuestr);

      if(reInit)
        ActiveSchemaDB()->
          getDefaults().
            updateCurrentDefaultsForOSIM(&defaultDefaults[j]);
      break;

    case DEF_PHYSICAL_MEMORY_AVAILABLE:
      utoa_(gpClusterInfo->physicalMemoryAvailable(), valuestr);
      strcpy(newValue, valuestr);
      if(reInit)
        ActiveSchemaDB()->
          getDefaults().
            updateCurrentDefaultsForOSIM(&defaultDefaults[j]);
      break;

    case DEF_TOTAL_MEMORY_AVAILABLE:
      utoa_(gpClusterInfo->totalMemoryAvailable(), valuestr);
      strcpy(newValue, valuestr);
      if(reInit)
        ActiveSchemaDB()->
          getDefaults().
            updateCurrentDefaultsForOSIM(&defaultDefaults[j]);
      break;

    case DEF_VIRTUAL_MEMORY_AVAILABLE:
      utoa_(gpClusterInfo->virtualMemoryAvailable(), valuestr);
      strcpy(newValue, valuestr);
      if(reInit)
        ActiveSchemaDB()->
          getDefaults().
            updateCurrentDefaultsForOSIM(&defaultDefaults[j]);
      break;

    case DEF_NUM_BM_CHUNKS:
      {
 UInt32 numChunks = (UInt32)
	  (gpClusterInfo->physicalMemoryAvailable() / def_DEF_CHUNK_SIZE / 4);
	utoa_(numChunks, valuestr);
	strcpy(newValue, valuestr);
      if(reInit)
        ActiveSchemaDB()->
          getDefaults().
            updateCurrentDefaultsForOSIM(&defaultDefaults[j]);
      }
      break;

    case DEF_INSTRUCTIONS_SECOND:
      {
 Int32 frequency, speed;
	frequency = gpClusterInfo->processorFrequency();

	switch (gpClusterInfo->cpuArchitecture()) {
	case CPU_ARCH_PENTIUM_PRO: speed = (Int32) (frequency * 0.5); break;
	case CPU_ARCH_PENTIUM:     speed = (Int32) (frequency * 0.4); break;
	default:                   speed = (Int32) (frequency * 0.3); break;
	}

	itoa_(speed, valuestr);
	strcpy(newValue, valuestr);
      if(reInit)
        ActiveSchemaDB()->
          getDefaults().
            updateCurrentDefaultsForOSIM(&defaultDefaults[j]);
      }
      break;

    case USTAT_IUS_PERSISTENT_CBF_PATH:

      {
        // set the CQD it to $HOME/cbfs
        const char* home = getenv("HOME");

        if ( home ) {
           str_cat(home, "/cbfs", newValue);
        }

      }
      break;

    default:
      #ifndef NDEBUG
        cerr << "updateSystemParameters: no case for "
	     << defaultDefaults[j].attrName << endl;
      #endif
      break;

    } // switch (arrayOfSystemParameters)
  } // for
  } // isStandalone

} // updateSystemParameters()

//==============================================================================
// Get SMP node number and cluster number on which this arkcmp.exe is running.
//==============================================================================
void
NADefaults::getNodeAndClusterNumbers(short& nodeNum, Int32& clusterNum)
{

  SB_Phandle_Type pHandle;
  Int32 error = XPROCESSHANDLE_GETMINE_(&pHandle);

  Int32 nodeNumInt; // XPROCESSHANDLE_DECOMPOSE_ takes an integer.
  Int32 pin;
  error = XPROCESSHANDLE_DECOMPOSE_(&pHandle, &nodeNumInt, &pin, &clusterNum);
  nodeNum = nodeNumInt; // Store 4-byte integer back to short integer


 CMPASSERT(error == 0);

}

inline static NABoolean initializeSQLdone()
{
  return FALSE;
}

// Setup for readFromSQLTable():
//
#include "SQLCLIdev.h"

const SQLMODULE_ID __SQL_mod_866668761818000 = {
  /* version */         SQLCLI_CURRENT_VERSION,
  /* module name */     "HP_SYSTEM_CATALOG.SYSTEM_SCHEMA.READDEF_N29_000",
  /* time stamp */      866668761818000LL,
  /* char set */        "ISO88591",
  /* name length */     47
};

static const Int32 MAX_VALUE_LEN = 1000;

// Read the SQL defaults table, to layer on further defaults.
//
// [1] This is designed such that it can be called multiple times
// (a site-wide defaults table, then a user-specific one, e.g.)
// and by default it will supersede values read/computed from earlier tables.
//
// [2] It can also be called *after* CQD's have been issued
// (e.g. from the getCatalogAndSchema() method)
// and by default it will supersede values from earlier tables
// but *not* explicitly CQD-ed settings.
//
// This default behavior is governed by the overwrite* arguments in
// various methods (see the .h file).  Naturally you can override such behavior,
// e.g., if you wanted to reset to an earlier state, erasing all user CQD's.
//
void NADefaults::readFromSQLTable(const char *tname,
				  Provenance overwriteIfNotYet,
				  Int32 errOrWarn)
{
  char value[MAX_VALUE_LEN + 1];
  // CMPASSERT(MAX_VALUE_LEN >= ComMAX_2_PART_EXTERNAL_UCS2_NAME_LEN_IN_NAWCHARS);

  // First (but only if NSK-LITE Services exist),
  // write system parameters (attributes DEF_*) into DefaultDefaults,
  // then copy DefaultDefaults into CurrentDefaults.
  if (!cmpCurrentContext->isStandalone())  {

  Lng32 initialErrCnt = CmpCommon::diags()->getNumber();


  // Set this *before* doing any insert()'s ...
  currentState_ = READ_FROM_SQL_TABLE;

  Int32 loop_here=0;
  while (loop_here > 10)
  {
    loop_here++;
    if (loop_here > 1000)
      loop_here=100;
  }

  if (tname) {

      NABoolean isSQLTable = TRUE;
      if (*tname == ' ') {	// called from NADefaults::readFromFlatFile()
	isSQLTable = FALSE;	// -- see kludge in .h file!
        tname++;
      }

      char attrName[101];	// column ATTRIBUTE VARCHAR(100) UPSHIFT
      Int32 sqlcode;
      static THREAD_P struct SQLCLI_OBJ_ID __SQL_id0;
      FILE *flatfile = NULL;

      if (isSQLTable) {
	init_SQLCLI_OBJ_ID(&__SQL_id0, SQLCLI_CURRENT_VERSION, cursor_name,
			&__SQL_mod_866668761818000, "S1", 0,
			SQLCHARSETSTRING_ISO88591, 2);

	/* EXEC SQL OPEN S1; See file NADefaults.mdf for cursor declaration */
	sqlcode = SQL_EXEC_ClearDiagnostics(&__SQL_id0);
        sqlcode = SQL_EXEC_Exec(&__SQL_id0,NULL,1,tname,NULL);
      }
      else {
        flatfile = fopen(tname, "r");
	sqlcode = flatfile ? 0 : -ABS(arkcmpErrorFileOpenForRead);
      }

      /* EXEC SQL FETCH S1 INTO :attrName, :value; */
      //   Since the DEFAULTS table is PRIMARY KEY (SUBSYSTEM, ATTRIBUTE),
      //   we'll fetch (scanning the clustering index)
      //   CATALOG before SCHEMA; this is important if user has rows like
      //   ('CATALOG','c1') and ('SCHEMA','c2.sn') --
      //   the schema setting must supersede the catalog one.
      //   We should also put an ORDER BY into the cursor decl in the .mdf,
      //   to handle user-created DEFAULTS tables w/o a PK.

      if (sqlcode >= 0)
	if (isSQLTable)
          {
            sqlcode = SQL_EXEC_Fetch(&__SQL_id0,NULL,2,attrName,NULL,value,NULL);
            if (sqlcode >= 0)
              readFromSQDefaultsTable_ = TRUE;
          }
	else {
	  value[0] = 0; // NULL terminator
	  if (fscanf(flatfile, " %100[A-Za-z0-9_#] ,", attrName) < 0) 
	    sqlcode = +100;
	  else 
	    fgets((char *) value, sizeof(value), flatfile);
	}

      // Ignore warnings except for end-of-data
      while (sqlcode >= 0 && sqlcode != +100) {

        NAString v(value);

        // skip comments, indicated by a #
        if (attrName[0] != '#')
          validateAndInsert(attrName, v, FALSE, errOrWarn, overwriteIfNotYet);

	/* EXEC SQL FETCH S1 INTO :attrName, :value; */
	if (isSQLTable)
	  sqlcode = SQL_EXEC_Fetch(&__SQL_id0,NULL,2,attrName,NULL,value,NULL);
	else {
	  value[0] = 0; // NULL terminator
	  if (fscanf(flatfile, " %100[A-Za-z0-9_#] ,", attrName) < 0) sqlcode = +100;
	  else fgets((char *) value, sizeof(value), flatfile);
	}
      }
      if (sqlcode < 0 && errOrWarn && initializeSQLdone()) {
	if (ABS(sqlcode) == ABS(CLI_MODULEFILE_OPEN_ERROR) &&
	    cmpCurrentContext->isInstalling()) {
	  // Emit no warning when (re)installing,
	  // because obviously the module will not exist before we have
	  // (re)arkcmp'd it!
	}
	else {
	  // 2001 Error $0 reading table $1.  Using $2 values.
	  CollIndex n = tablesRead_.entries();
          const char *errtext = n ? tablesRead_[n-1].data() : "default-default";
	  *CmpCommon::diags() << DgSqlCode(ERRWARN(2001))
	    << DgInt0(sqlcode) << DgTableName(tname) << DgString0(errtext);
	}
      }

      if (isSQLTable) {
	/* EXEC SQL CLOSE S1; */
	sqlcode = SQL_EXEC_ClearDiagnostics(&__SQL_id0);
        sqlcode = SQL_EXEC_CloseStmt(&__SQL_id0);

        // The above statement should not start any transactions because
        // it uses read uncommitted access.  If it ever changes, then we
        // would need to commit it at this time.
      }

  } // tname

  if (initialErrCnt < CmpCommon::diags()->getNumber() && errOrWarn)
    *CmpCommon::diags() << DgSqlCode(ERRWARN(2059))
      << DgString0(tname ? tname : "");
  } // isStandalone

} // NADefaults::readFromSQLTable()

void NADefaults::readFromSQLTables(Provenance overwriteIfNotYet, Int32 errOrWarn)
{
  NABoolean cat = FALSE;
  NABoolean sch = FALSE;


  if (getToken(MODE_SEABASE) == DF_ON && !readFromSQDefaultsTable())
    {
      // Read system defaults from configuration file.
      // keep this name in sync with file cli/SessionDefaults.cpp
      NAString confFile(getenv("TRAF_CONF"));
      confFile += "/SQSystemDefaults.conf";
      readFromFlatFile(confFile, overwriteIfNotYet, errOrWarn);  
      tablesRead_.insert(confFile);           

      CmpSeabaseDDL cmpSBD((NAHeap *)heap_, FALSE);
      Lng32 hbaseErr = 0;
      NAString hbaseErrStr;
      Lng32 errNum = cmpSBD.validateVersions(this, NULL, NULL, NULL, 
                                             NULL, NULL, NULL, 
                                             NULL, NULL, NULL, NULL,
                                             &hbaseErr, &hbaseErrStr);
      if (errNum == 0) // seabase is initialized properly
        {
          // read from seabase defaults table
          cmpSBD.readAndInitDefaultsFromSeabaseDefaultsTable
            (overwriteIfNotYet, errOrWarn, this);
          
          // set authorization state

          NABoolean checkAllPrivTables = FALSE;
          errNum = cmpSBD.isPrivMgrMetadataInitialized(this,checkAllPrivTables);
          CmpCommon::context()->setAuthorizationState(errNum);
        }
      else
	{
	  CmpCommon::context()->setIsUninitializedSeabase(TRUE);
	  CmpCommon::context()->uninitializedSeabaseErrNum() = errNum;
	  CmpCommon::context()->hbaseErrNum() = hbaseErr;
	  CmpCommon::context()->hbaseErrStr() = hbaseErrStr;
	}
    }

  currentState_ = SET_BY_CQD;	// enter the next state...

  // Make self fully consistent, by executing deferred actions last of all
  getSqlParser_NADefaults();
} // NADefaults::readFromSQLTables()

// This method is used by SchemaDB::initPerStatement
const char * NADefaults::getValueWhileInitializing(Int32 attrEnum)
{
  // We can't rely on our state_ because SQLC might have called CQD::bindNode()
  // which does a setState(SET_BY_CQD)...
  if (!tablesRead_.entries())
    if (getProvenance(attrEnum) < SET_BY_CQD)
      readFromSQLTables(SET_BY_CQD);

  return getValue(attrEnum);
}

// This method is used by SchemaDB::initPerStatement *and*
// by CmpCommon, CmpStatement, and SQLC/SQLCO.
void NADefaults::getCatalogAndSchema(NAString &cat, NAString &sch)
{
  cat = getValueWhileInitializing(CATALOG);
  sch = getValueWhileInitializing(SCHEMA);
}

// Should be called only privately and by DefaultValidator!
Int32 NADefaults::validateFloat(const char *value, float &result,
			      Int32 attrEnum, Int32 errOrWarn) const
{
  Int32 n = -1;	// NT's scanf("%n") is not quite correct; hence this code-around
  sscanf(value, "%g%n", &result, &n);
  if (n > 0 && value[n] == '\0') 
    {
      switch (attrEnum)
        {
        case HIVE_INSERT_ERROR_MODE:
          {
            Lng32 v = str_atoi(value, str_len(value));
            if (v >= 0 && v <= 3)
              return TRUE;
          }
          break;
        default:
          return TRUE;	// a valid float
        }
    }

  NAString v(value);
  NABoolean silentIf = (errOrWarn == SilentIfSYSTEM);
  if (silentIf) errOrWarn = 0/*silent*/;
  NABoolean useSYSTEM = (token(attrEnum, v, TRUE, errOrWarn) == DF_SYSTEM);

  if (useSYSTEM && silentIf)			// ValidateNumeric is caller
    return SilentIfSYSTEM;			// special it-is-valid return!

  if (errOrWarn)
    *CmpCommon::diags() << DgSqlCode(ERRWARN(2055))
      << DgString0(value)
      << DgString1(lookupAttrName(attrEnum, errOrWarn));

  if (useSYSTEM) {				// programmer error
    CMPASSERT("Numeric attr allows SYSTEM -- you need to call token() first to see if its current value is this keyword, and compute your system default value!" == NULL);
  }

  // ensure an out-of-range error if domainMatch or ValidateNumeric is called
  result = -FLT_MAX;
  return FALSE;					// not valid
}

NABoolean NADefaults::insert(Int32 attrEnum, const NAString &value, Int32 errOrWarn)
{
  // private method; callers have all already done this:    ATTR_RANGE_ASSERT;
  assert(errOrWarn != SilentIfSYSTEM);		// yeh private, but just in case

  // Update cache:
  // (Re)validate that new value is numeric.
  // Useful if programmer did not correctly specify the DefaultValidator for
  // this attr in DefaultDefaults.
  //
  if (currentFloats_[attrEnum]) {
    float result;
    if (validateFloat(value, result, attrEnum, errOrWarn))
      *currentFloats_[attrEnum] = result;
    else
      return FALSE;				// not a valid float
  }

  // Update cache for DefaultToken by deallocating the cached entry.
  if ( currentTokens_[attrEnum] )
  {
    NADELETEBASIC( currentTokens_[attrEnum], NADHEAP );
    currentTokens_[attrEnum] = NULL;
  }

  // If we're past the read-from-SQLTable phase, then
  // the first CQD of a given attr must first save the from-SQLTable value,
  // to which the user can RESET if desired.
  //
  if (currentState_ >= SET_BY_CQD && !resetToDefaults_[attrEnum]) {
    NAString currValStr(currentDefaults_[attrEnum]);
    Lng32 currValLen = str_len(currValStr) + 1;
    char *pCurrVal = new NADHEAP char[currValLen];
    str_cpy_all(pCurrVal, currValStr, currValLen);
    resetToDefaults_[attrEnum] = pCurrVal;
  }

  char *newVal = NULL;
  Lng32 newValLen = str_len(value) + 1;
  if (provenances_[attrEnum] > INIT_DEFAULT_DEFAULTS) {
    Lng32 oldValLen = str_len(currentDefaults_[attrEnum]) + 1;
    if (oldValLen >= newValLen && oldValLen < newValLen + 100)
      newVal = const_cast<char*>(currentDefaults_[attrEnum]);	// reuse, to reduce mem frag
    else
      NADELETEBASIC(currentDefaults_[attrEnum], NADHEAP);
  }

  if (!newVal) newVal = new NADHEAP char[newValLen];
  str_cpy_all(newVal, value, newValLen);
  currentDefaults_[attrEnum] = newVal;

  // when the parser flag is on for a set-once CQD
  // set its provenance as INIT_DEFAULT_DEFAULTS,
  // so the user can set it once later
  if ( isSetOnceAttribute(attrEnum) &&
       Get_SqlParser_Flags(INTERNAL_QUERY_FROM_EXEUTIL) )
  {
    provenances_[attrEnum] = INIT_DEFAULT_DEFAULTS;
  }
  else
  {
  provenances_[attrEnum] = currentState_;
  }

  return TRUE;
}

NADefaults::Provenance NADefaults::getProvenance(Int32 attrEnum) const
{
  ATTR_RANGE_ASSERT;
  return (Provenance)provenances_[attrEnum];
}

NABoolean NADefaults::getValue(Int32 attrEnum, NAString &result) const
{
  ATTR_RANGE_ASSERT;
  result = currentDefaults_[attrEnum];
  return TRUE;		// we always have a STRING REPRESENTATION value
}

NAString NADefaults::getString(Int32 attrEnum) const
{
  ATTR_RANGE_ASSERT;
  return currentDefaults_[attrEnum];
}

const char * NADefaults::getValue(Int32 attrEnum) const
{
  ATTR_RANGE_ASSERT;
  return currentDefaults_[attrEnum];
}

NABoolean NADefaults::getFloat(Int32 attrEnum, float &result) const
{
  ATTR_RANGE_ASSERT;
  if (currentFloats_[attrEnum]) {
    result = *currentFloats_[attrEnum];
  }
  else if (validateFloat(currentDefaults_[attrEnum], result, attrEnum)) {
    currentFloats_[attrEnum] = new NADHEAP float;	// cache the result
    *currentFloats_[attrEnum] = result;
  }
  else {
    return FALSE;		// result is neg, from failed validateFloat()
  }

  return TRUE;
}

double NADefaults::getAsDouble(Int32 attrEnum) const
{
  // No domainMatch() needed: any float or double (or int or uint) is okay;
  // getFloat()/validateFloat() will disallow any non-numerics.
  float flt;
  getFloat(attrEnum, flt);
  return double(flt);
}

Lng32 NADefaults::getAsLong(Int32 attrEnum) const
{
  float flt;
  getFloat(attrEnum, flt);
  if (!domainMatch(attrEnum, VALID_INT, &flt)) { CMPBREAK; }
  return Lng32(flt);
}

ULng32 NADefaults::getAsULong(Int32 attrEnum) const
{
  float flt;
  getFloat(attrEnum, flt);
  if (!domainMatch(attrEnum, VALID_UINT, &flt)) { CMPBREAK; }
  return (ULng32)(flt);
}

ULng32 NADefaults::getNumOfESPsPerNode() const
{
  return (ULng32)MAXOF(ceil(getNumOfESPsPerNodeInFloat()), 1); 
}

float NADefaults::getNumOfESPsPerNodeInFloat() const
{
   double maxEspPerCpuPerOp = getAsDouble(MAX_ESPS_PER_CPU_PER_OP);

   CollIndex cores =
     ( (CmpCommon::context() && CURRSTMT_OPTDEFAULTS->isFakeHardware())
     ) ?  
        getAsLong(DEF_NUM_SMP_CPUS) :
        gpClusterInfo->numberOfCpusPerSMP();

   return float(maxEspPerCpuPerOp * cores);
}

ULng32 NADefaults::getTotalNumOfESPsInCluster(NABoolean& fakeEnv) const
{
   fakeEnv = FALSE;

   if (getToken(PARALLEL_NUM_ESPS, 0) != DF_SYSTEM ) {
     fakeEnv = TRUE;
     return getAsLong(PARALLEL_NUM_ESPS);
   }

   float espsPerNode = getNumOfESPsPerNodeInFloat();

   CollIndex numOfNodes = gpClusterInfo->numOfSMPs();

   if ( (CmpCommon::context() && CURRSTMT_OPTDEFAULTS->isFakeHardware())) {
     fakeEnv = TRUE;
     numOfNodes = getAsLong(DEF_NUM_NODES_IN_ACTIVE_CLUSTERS);
   }

   return MAXOF(ceil(espsPerNode * numOfNodes), 1);
}

NABoolean NADefaults::domainMatch(Int32 attrEnum,
				  Int32 expectedType/*DefaultValidatorType*/,
				  float *flt) const
{
  if (validator(attrEnum)->getType() == expectedType)
    return TRUE;				// yes, domains match

  // Emit error messages only if the value is actually out-of-range.
  //
  // Users (optimizer code) should REALLY be using 'unsigned long' fields
  // and calling getAsULong, instead of using 'long' fields to retrieve
  // unsigned(DDui*) attr values via getAsLong ...
  //
  // if we get here the compiler will crash
  if (flt) {
    DefaultValidator *validator = NULL;
    if      (expectedType == VALID_INT)  validator = (DefaultValidator *)&validateInt;
    else if (expectedType == VALID_UINT) validator = (DefaultValidator *)&validateUI;

    // Explicitly check for TRUE here --
    // both FALSE/error and SilentIfSYSTEM are out-of-range/out-of-domain
    // from this method's point of view.
    if (validator)
      if (validator->validate(
		  currentDefaults_[attrEnum], this, attrEnum, -1, flt) == TRUE)
	return TRUE;	// domain mismatch, but value *is* in the domain range

  } // fall thru to emit additional failure info

  *CmpCommon::diags() << DgSqlCode(+2058)	// emit a mismatch WARNING
    << DgString0(lookupAttrName(attrEnum))
    << DgString1(validator(attrEnum)->getTypeText())
    << DgString2(DefaultValidator::getTypeText(
    		 DefaultValidatorType(expectedType)));
  #ifndef NDEBUG
    cerr << "Warning[2058] " << lookupAttrName(attrEnum)
      << " " << validator(attrEnum)->getTypeText()
      << " " << DefaultValidator::getTypeText(
      		DefaultValidatorType(expectedType))
      << " " << (flt ? *flt : 123.45) << endl;
  #endif

  return FALSE;
}

// CONTROL QUERY DEFAULT attr RESET;
//	resets the single attr to the value it had right after we read all
//	the DEFAULTS tables,
//	or the value it had right before a CQD * RESET RESET.
// CONTROL QUERY DEFAULT * RESET;
//	resets all attrs to the values they had by same criteria as above.
// CONTROL QUERY DEFAULT * RESET RESET;
//	resets the "reset-to" values so that all current values become the
//	effective "reset-to"'s -- i.e, the current values can't be lost
//	on the next  CQD * RESET;
//	Useful for apps that dynamically send startup settings that ought
//	to be preserved -- ODBC and SQLCI do this.
//
void NADefaults::resetAll(NAString &value, NABoolean reset, Int32 errOrWarn)
{
  size_t i, numAttrs = numDefaultAttributes();

  if (reset == 1) {			// CQD * RESET; (not RESET RESET)
    setResetAll(TRUE);
    for (i = 0; i < numAttrs; i++)
    {
      const char * attributeName = defaultDefaults[i].attrName;
      DefaultConstants attrEnum = lookupAttrName(attributeName, errOrWarn);

      if (isNonResetableAttribute(attributeName))
	continue;

      validateAndInsert(attributeName, value, TRUE, errOrWarn);
    }
    // if DEFAULT_SCHEMA_NAMETYPE=USER after CQD * RESET
    // set SCHEMA to LDAP_USERNAME
    // if SCHEMA has not been specified by user
    if ( (getToken(DEFAULT_SCHEMA_NAMETYPE) == DF_USER) &&
         schSetByNametype() )
    {
      setSchemaAsLdapUser();
    }
    setResetAll(FALSE);
  }
  else if (reset == 2) {
    for (i = 0; i < numAttrs; i++) {
      if (resetToDefaults_[i]) {
	// CONTROL QUERY DEFAULT * RESET RESET;  -- this code cloned below
	//   Can't reset prov, because to which?
	//	  provenances_[i] =  READ_FROM_SQL_TABLE  or  COMPUTED ??
	NADELETEBASIC(resetToDefaults_[i], NADHEAP);
	resetToDefaults_[i] = NULL;
      }
    }
  }
  else {
    CMPASSERT(!reset);
  }
}

// Reset to default-defaults, as if readFromSQLTables() had not executed,
// but setting state and provenance so no future reads will be triggered.
// See StaticCompiler and Genesis 10-990204-2469 above for motivation.
void NADefaults::undoReadsAndResetToDefaultDefaults()
{
  initCurrentDefaultsWithDefaultDefaults();
}

NABoolean NADefaults::isReadonlyAttribute(const char* attrName) const
{
   if ((( stricmp(attrName, "ISO_MAPPING") == 0 ) ||
        ( stricmp(attrName, "OVERFLOW_MODE") == 0 ) ||
	( stricmp(attrName, "SORT_ALGO") == 0 )) 
        && ( CmpCommon::getDefault(DISABLE_READ_ONLY) == DF_ON ))
     return FALSE; // for internal development and testing purposes

   if (( stricmp(attrName, "ISO_MAPPING") == 0 )||
       ( stricmp(attrName, "NATIONAL_CHARSET") == 0 ) ||
       ( stricmp(attrName, "VALIDATE_VIEWS_AT_OPEN_TIME") == 0 ) ||
       ( stricmp(attrName, "USER_EXPERIENCE_LEVEL") == 0 ) ||
       ( stricmp(attrName, "POS_DISKS_IN_SEGMENT") == 0 ) ||
       ( stricmp(attrName, "BMO_MEMORY_LIMIT_LOWER_BOUND_HASHJOIN") == 0 ) ||
       ( stricmp(attrName, "EXE_MEMORY_LIMIT_LOWER_BOUND_MERGEJOIN") == 0 ) ||
       ( stricmp(attrName, "BMO_MEMORY_LIMIT_LOWER_BOUND_HASHGROUPBY") == 0 ) ||
       ( stricmp(attrName, "BMO_MEMORY_LIMIT_LOWER_BOUND_SORT") == 0 ) ||
       ( stricmp(attrName, "EXE_MEMORY_LIMIT_LOWER_BOUND_SEQUENCE") == 0 ) ||
       ( stricmp(attrName, "EXE_MEMORY_LIMIT_LOWER_BOUND_EXCHANGE") == 0 ) ||
       ( stricmp(attrName, "SORT_ALGO") == 0 ) ||
       ( stricmp(attrName, "OVERFLOW_MODE") == 0 )
     )
     return TRUE;

   if (strlen(attrName) > 0)
     {
       DefaultConstants v = lookupAttrName(attrName, 0, 0);
       if ((v != __INVALID_DEFAULT_ATTRIBUTE) &&
	   (getFlags(v) & DEFAULT_IS_SSD))
	 return TRUE;
     }

   return FALSE;
}

// these defaults cannot be reset or set to FALSE through a cqd.
NABoolean NADefaults::isNonResetableAttribute(const char* attrName) const
{
   if (( stricmp(attrName, "IS_SQLCI") == 0 ) ||
       ( stricmp(attrName, "NVCI_PROCESS") == 0 ) ||
       ( stricmp(attrName, "SESSION_ID") == 0 ) ||
       ( stricmp(attrName, "LDAP_USERNAME") == 0 ) ||
       ( stricmp(attrName, "VOLATILE_SCHEMA_IN_USE") == 0 ) ||
       ( stricmp(attrName, "SESSION_USERNAME") == 0 ) )
     return TRUE;

   return FALSE;
}

// these defaults can be set only once by user.
NABoolean NADefaults::isSetOnceAttribute(Int32 attrEnum) const
{
  if ( attrEnum == DEFAULT_SCHEMA_ACCESS_ONLY )
    return TRUE;
  
   return FALSE;
}

void NADefaults::resetSessionOnlyDefaults()
{
  NAString value;
  validateAndInsert("NVCI_PROCESS", value, 3, 0);
}

// Parameter <reset> must not be a reference (&);
// see <value = ... fall thru> below.
enum DefaultConstants NADefaults::validateAndInsert(const char *attrName,
						    NAString &value,
						    NABoolean reset,
						    Int32 errOrWarn,
						    Provenance overwriteIfNotYet)
{
  NABoolean overwrite = FALSE;
  NABoolean isJDBC = FALSE;
  NABoolean isODBC = FALSE;

  if (ActiveSchemaDB())
  {
     isJDBC = (CmpCommon::getDefault(JDBC_PROCESS) == DF_ON ? TRUE : FALSE);
     isODBC = (CmpCommon::getDefault(ODBC_PROCESS) == DF_ON ? TRUE : FALSE);
  }

  if (reset && !attrName[0]) {			// CONTROL QUERY DEFAULT * RESET
    overwrite = currentState_ < overwriteIfNotYet;
    if (overwrite) resetAll(value, reset, errOrWarn);
    return (DefaultConstants)0;			// success
  }

  // Perform a lookup for the string, using a binary search.
  DefaultConstants attrEnum = lookupAttrName(attrName, errOrWarn);

  if (attrEnum >= 0) {				// valid attrName

    // ignore DEFAULT_SCHEMA_ACCESS_ONLY if it is in system defaults
    if ( attrEnum == DEFAULT_SCHEMA_ACCESS_ONLY &&
          getState() < SET_BY_CQD )
      return attrEnum;

    // do the following check when
    // this is the primary mxcmp
    // and INTERNAL_QUERY_FROM_EXEUTIL is not set

    if (!CmpCommon::context()->isSecondaryMxcmp() &&
        !Get_SqlParser_Flags(INTERNAL_QUERY_FROM_EXEUTIL))
    {
      // This logic will catch if the set-once CQD 
      // is set, but the ALLOW_SET_ONCE_DEFAULTS parserflags
      // are not set.  This is absolutely necessary for security
      // to ensure that the correct parserflags are set.
      if ((isSetOnceAttribute(attrEnum)) &&
	  (!isResetAll()) &&                // no error msg for cqd * reset
	  (NOT Get_SqlParser_Flags(ALLOW_SET_ONCE_DEFAULTS)))
	{
	  *CmpCommon::diags() << DgSqlCode(-30042) << DgString0(attrName);
	  return attrEnum;
	}

      // if DEFAULT_SCHEMA_ACCESS_ONLY is on,
      // users cannot change the following CQDs
      if ( getState() >= SET_BY_CQD &&
           getToken(DEFAULT_SCHEMA_ACCESS_ONLY) == DF_ON )
      {
        if (attrEnum == SCHEMA ||
            attrEnum == PUBLIC_SCHEMA_NAME ||
            attrEnum == DEFAULT_SCHEMA_NAMETYPE)
        {
          if (!isResetAll())              // no error msg for cqd * reset
    		    *CmpCommon::diags() << DgSqlCode(-30043) << DgString0(attrName);
          return attrEnum;
        }
      }
    }
    else
    {
      // ignore LAST0_MODE cqd if we are in secondary mxcmp or if
      // internal_query_from_exeutil is set. This cqd is not meant
      // to apply in these cases
      if ( attrEnum == LAST0_MODE )
        return attrEnum;
      
    }
  
      
    overwrite = getProvenance(attrEnum) < overwriteIfNotYet;

    // Put value into canonical form (trimmed, upcased where pertinent).
    //
    // Possibly revert to initial default default value -- see NOTE 3 up above.
    // Note further that ANSI names cannot revert on values of
    // 'SYSTEM' or 'ENABLE', as those are legal cat/sch/tbl names,
    // nor can they revert on '' (empty/blank), as ANSI requires us to
    // emit a syntax error for this.
    //
    // Possibly RESET to read-from-table value (before any CQD value).
    //
    TrimNAStringSpace(value);
    if (validator(attrEnum) != &validateAnsiName && !reset) {
      validator(attrEnum)->applyUpper(value);

      if (isSynonymOfSYSTEM(attrEnum, value))
        value = getDefaultDefaultValue(attrEnum);
      else if (isSynonymOfRESET(value))		// CQD attr 'RESET'; ...
        reset = 1;
    }

    if (reset) {				// CQD attr RESET;

      if ((isNonResetableAttribute(attrName)) &&
	  (reset != 3))
	return attrEnum;

      if (!resetToDefaults_[attrEnum]) {
	if (overwrite)
	  value = currentDefaults_[attrEnum];	// return actual val to caller

	if (attrEnum == ISOLATION_LEVEL)
	  {
	    // reset this in the global area
	    TransMode::IsolationLevel il;
	    getIsolationLevel(il);
	    CmpCommon::transMode()->updateAccessModeFromIsolationLevel(il);
 	  }
        // Solution: 10-060418-5903. Do not update MXCMP global access mode
        // with CQD ISOLATION_LEVEL_FOR_UPDATES as it will overwrite that
        // set by ISOLATION_LEVE. The CQD ISOLATION_LEVEL_FOR_UPDATES is
        // always accessed directly when necessary.
	//else if (attrEnum == ISOLATION_LEVEL_FOR_UPDATES)
	//  {
	//    // reset this in the global area
	//    TransMode::IsolationLevel il;
	//    getIsolationLevel(il, getToken(attrEnum));
	//    CmpCommon::transMode()->updateAccessModeFromIsolationLevel(il,
        //						       FALSE);
 	//  }

	return attrEnum;
      }
      value = resetToDefaults_[attrEnum];	// fall thru, REINSERT this val
    }

    if (attrEnum == CATALOG) {
      if (!setCatalog(value, errOrWarn, overwrite))
        attrEnum = __INVALID_DEFAULT_ATTRIBUTE;
      else
	{
	  if (getState() == READ_FROM_SQL_TABLE)
	    {
	      // set the volatile catalog to be same as the catalog read from
	      // defaults table. If there is no catalog or volatile_catalog
	      // specified in the defaults table, then volatile catalog name
	      // will be the default catalog in use in the session where
	      // volatile tables are created.
	      CmpCommon::context()->sqlSession()->setVolatileCatalogName(value);
	    }
	}
    }
    else if (attrEnum == SCHEMA) {
      if (!setSchema(value, errOrWarn, overwrite))
        attrEnum = __INVALID_DEFAULT_ATTRIBUTE;
      else
	{
	  if (getState() == READ_FROM_SQL_TABLE)
	    {
	      // set the volatile catalog to be same as the catalog read from
	      // defaults table. If there is no catalog or volatile_catalog
	      // specified in the defaults table, then volatile catalog name
	      // will be the default catalog in use in the session where
	      // volatile tables are created.
	      NAString cat(getValue(CATALOG));
	      CmpCommon::context()->sqlSession()->setVolatileCatalogName(cat);
	    }
	}
    }
   
    else {
      if ( attrEnum == MAX_LONG_VARCHAR_DEFAULT_SIZE ||
           attrEnum == MAX_LONG_WVARCHAR_DEFAULT_SIZE ) {

 ULng32 minLength;
	switch (attrEnum) {
	case MAX_LONG_VARCHAR_DEFAULT_SIZE:
	  minLength = (Lng32)getAsULong(MIN_LONG_VARCHAR_DEFAULT_SIZE);
	  break;
	case MAX_LONG_WVARCHAR_DEFAULT_SIZE:
	  minLength = (Lng32)getAsULong(MIN_LONG_WVARCHAR_DEFAULT_SIZE);
	  break;
	default:
	  attrEnum = __INVALID_DEFAULT_ATTRIBUTE;
	}

	if ( attrEnum != __INVALID_DEFAULT_ATTRIBUTE ) {
	  UInt32 newMaxLength;
	  Int32 n = -1;
	  sscanf(value.data(), "%u%n", &newMaxLength, &n);
	  if ( n>0 && (UInt32)n == value.length() ) {    // a valid unsigned number
	    if ( newMaxLength < minLength )
	      {
		*CmpCommon::diags() << DgSqlCode(-2030) << DgInt0((Lng32)minLength);
		attrEnum = __INVALID_DEFAULT_ATTRIBUTE;
	      }
	  }
	}
      }

      if ( attrEnum == MIN_LONG_VARCHAR_DEFAULT_SIZE ||
           attrEnum == MIN_LONG_WVARCHAR_DEFAULT_SIZE ) {

 ULng32 maxLength;
	switch (attrEnum) {
	case MIN_LONG_VARCHAR_DEFAULT_SIZE:
	  maxLength = getAsULong(MAX_LONG_VARCHAR_DEFAULT_SIZE);
	  break;
	case MIN_LONG_WVARCHAR_DEFAULT_SIZE:
	  maxLength = getAsULong(MAX_LONG_WVARCHAR_DEFAULT_SIZE);
	  break;
	default:
	  attrEnum = __INVALID_DEFAULT_ATTRIBUTE;
	}

	if ( attrEnum != __INVALID_DEFAULT_ATTRIBUTE ) {
	  UInt32 newMinLength;
	  Int32 n = -1;
	  sscanf(value.data(), "%u%n", &newMinLength, &n);
	  if ( n>0 && (UInt32)n == value.length() ) {    // a valid unsigned number
	    if ( newMinLength > maxLength )
	      {
		*CmpCommon::diags() << DgSqlCode(-2029) << DgInt0((Lng32)maxLength);
		attrEnum = __INVALID_DEFAULT_ATTRIBUTE;
	      }
	  }
	}
      }

      if (errOrWarn && (attrEnum == ROUNDING_MODE))
	{
	  if (NOT ((value.length() == 1) &&
		   ((*value.data() == '0') ||
		    (*value.data() == '1') ||
		    (*value.data() == '2'))))
            {
	      *CmpCommon::diags() << DgSqlCode(-2055)
				  << DgString0(value)
				  << DgString1(lookupAttrName(attrEnum));

	      attrEnum = __INVALID_DEFAULT_ATTRIBUTE;
	    }
	}

      if ( attrEnum == SCRATCH_MAX_OPENS_HASH ||
	   attrEnum == SCRATCH_MAX_OPENS_SORT )
	{
	  if (NOT ((value.length() == 1) &&
		   ((*value.data() == '1') ||
		    (*value.data() == '2') ||
                    (*value.data() == '3') ||
		    (*value.data() == '4'))))
	    {
	      *CmpCommon::diags() << DgSqlCode(-2055)
				  << DgString0(value)
				  << DgString1(lookupAttrName(attrEnum));

	      attrEnum = __INVALID_DEFAULT_ATTRIBUTE;
	    }
	}

      if (attrEnum != __INVALID_DEFAULT_ATTRIBUTE)
	{

	  // We know that the MP_COLLATIONS validator emits only warnings
	  // and always returns TRUE.  On the validate-but-do-not-insert step
	  // (CQD compilation), those warnings will be seen by the user.
	  // On the validate-AND-insert (CQD execution), there is no need
	  // to repeat them (besides, that causes Executor to choke on the
	  // warnings in the diags and say 'Error fetching from TCB tree').

	  Int32 isValid = TRUE;
	  if (!overwrite || currentState_ < SET_BY_CQD ||
	      validator(attrEnum) != &validateCollList)
	    isValid = validator(attrEnum)->validate(value, this, attrEnum, errOrWarn);

	  // if an internal reset is being done, then make it a valid attr
	  // even if the 'validate' method above returned invalid.
	  if ((!isValid) &&
	      (isNonResetableAttribute(attrName)) &&
	      (reset == 3))
	    {
	      isValid = TRUE;
	    }

	  if (!isValid)
	    attrEnum = __INVALID_DEFAULT_ATTRIBUTE;

	  else if (overwrite) {
	    if (isValid == SilentIfSYSTEM) {    // defDef value was "SYSTEM" or ""
	      // Undo any caching from getFloat()
	      NADELETEBASIC(currentFloats_[attrEnum], NADHEAP);
	      currentFloats_[attrEnum] = NULL;

	      // Undo any caching from getToken()
	      NADELETEBASIC( currentTokens_[attrEnum], NADHEAP );
	      currentTokens_[attrEnum] = NULL;

	      // Now fall thru to insert the string "SYSTEM" or ""
	    }
	   
	    if (!insert(attrEnum, value, errOrWarn))
	      attrEnum = __INVALID_DEFAULT_ATTRIBUTE;
	  }	  // overwrite (i.e. insert)
	}
    } // not special val/ins for CAT, SCH, or MPLOC
  }	  // valid attrName

  if (attrEnum >= 0) {
    if (overwrite) {
      if ((! reset) &&
	  (currentState_ == SET_BY_CQD))
	{
	  // indicate that this attribute was set by a user CQD.
	  setUserDefault(attrEnum, TRUE);
	}

      switch (attrEnum) {

      case CATALOG:
      case SCHEMA:
	break;
      case ISOLATION_LEVEL:
	{
	  // Ansi 14.1 SR 4.  See comexe/ExControlArea::addControl().
	  //##	I now think this implementation is wrong
	  //##	because this is setting GLOBAL state
	  //##	for something that should be CONTEXT-dependent.
	  //##	Will cause us headaches later, when we
	  //##	make arkcmp be a multi-context multi-threaded server.
	  TransMode::IsolationLevel il;
	  getIsolationLevel(il);
	  CmpCommon::transMode()->updateAccessModeFromIsolationLevel(il);
	}
      break;
      // Solution: 10-060418-5903. Do not update MXCMP global access mode
      // with CQD ISOLATION_LEVEL_FOR_UPDATES as it will overwrite that
      // set by ISOLATION_LEVEL. The CQD ISOLATION_LEVEL_FOR_UPDATES is
      // always accessed directly when necessary.
      //case ISOLATION_LEVEL_FOR_UPDATES:
      //{
      //  TransMode::IsolationLevel il;
      //  getIsolationLevel(il, getToken(attrEnum));
      //  CmpCommon::transMode()->updateAccessModeFromIsolationLevel(il,
      //							     FALSE);
      //}
      //break;

      case ALLOW_INCOMPATIBLE_ASSIGNMENT:
      case ALLOW_INCOMPATIBLE_COMPARISON:
	{
	  NAString val;
          
          if (value == "ON")
            val = "ON";
          else
            val = "OFF";
                    
          insert(ALLOW_INCOMPATIBLE_OPERATIONS, val, errOrWarn);
        }
        break;

      case MODE_SPECIAL_1:
	{
	  NAString val;
          
          if (value == "ON")
            val = "ON";
          else
            val = "OFF";

          insert(ALLOW_INCOMPATIBLE_OPERATIONS, val, errOrWarn);

	  // find_suitable_key to be turned off in this mode, unless
	  // it has been explicitely set.
	  if (getToken(VOLATILE_TABLE_FIND_SUITABLE_KEY) == DF_SYSTEM)
	    {
	      insert(VOLATILE_TABLE_FIND_SUITABLE_KEY, "OFF", errOrWarn);
	    }
	}
      break;

      case MODE_SPECIAL_4:
	{
	  NAString val;
          
          if (value == "ON")
            val = "ON";
          else
            val = "OFF";
                    
          insert(ALLOW_INCOMPATIBLE_OPERATIONS, val, errOrWarn);
                    
          NAString csVal;
          if (value == "ON")
            csVal = SQLCHARSETSTRING_UTF8;
          else
            csVal = "";
          
          validateAndInsert("TRAF_DEFAULT_COL_CHARSET", csVal, FALSE, errOrWarn);

          NAString notVal;
          if (value == "ON")
            notVal = "OFF";
          else
            notVal = "ON";
          insert(TRAF_COL_LENGTH_IS_CHAR, notVal, errOrWarn);

          NAString costVal1;
          NAString costVal2;
          if (value == "ON") {
            costVal1 = "8.0";
            costVal2 = "16.0" ;
          }
          else {
            costVal1 = "1.0";
            costVal2 = "1.0" ;
          }
          validateAndInsert("NCM_IND_JOIN_COST_ADJ_FACTOR", costVal1, 
                            FALSE, errOrWarn);
          validateAndInsert("NCM_IND_SCAN_COST_ADJ_FACTOR", costVal2, 
                            FALSE, errOrWarn);
 
          if (value == "ON")
            Set_SqlParser_Flags(IN_MODE_SPECIAL_4);
          else
            Reset_SqlParser_Flags(IN_MODE_SPECIAL_4);
	}
      break;

      case MODE_SEABASE:
	{
	  if (value == "ON")
	    {
	      if (NOT seabaseDefaultsTableRead())
		{
		  CmpSeabaseDDL cmpSBD((NAHeap *)heap_);
		  Lng32 errNum = cmpSBD.validateVersions(this);
		  if (errNum == 0) // seabase is initialized properly
		    {
		      // read from seabase defaults table
		      cmpSBD.readAndInitDefaultsFromSeabaseDefaultsTable
			(overwriteIfNotYet, errOrWarn, this);
		    }
		  else
		    {
		      CmpCommon::context()->setIsUninitializedSeabase(TRUE);
		      CmpCommon::context()->uninitializedSeabaseErrNum() = errNum;
		    }
		}

	      NAString sbCat = getValue(SEABASE_CATALOG);
	      insert(SEABASE_VOLATILE_TABLES, "ON", errOrWarn);
	      CmpCommon::context()->sqlSession()->setVolatileCatalogName(sbCat, TRUE);

	      insert(UPD_SAVEPOINT_ON_ERROR, "OFF", errOrWarn);
	    }
	  else
	    {
	      NAString defCat = getValue(CATALOG);
	      insert(SEABASE_VOLATILE_TABLES, "OFF", errOrWarn);
	      CmpCommon::context()->sqlSession()->setVolatileCatalogName(defCat);

	      insert(UPD_SAVEPOINT_ON_ERROR, "ON", errOrWarn);
	    }
	}
      break;

      case MEMORY_LIMIT_QCACHE_UPPER_KB:
        CURRENTQCACHE->setHeapUpperLimit((size_t) 1024 * atoi(value.data()));
        break;

      case MEMORY_LIMIT_HISTCACHE_UPPER_KB:
        CURRCONTEXT_HISTCACHE->setHeapUpperLimit((size_t) 1024 * atoi(value.data()));
        break;

      case MEMORY_LIMIT_CMPSTMT_UPPER_KB:
        STMTHEAP->setUpperLimit((size_t) 1024 * atoi(value.data()));
        break;

      case MEMORY_LIMIT_CMPCTXT_UPPER_KB:
        CTXTHEAP->setUpperLimit((size_t) 1024 * atoi(value.data()));
        break;

      case MEMORY_LIMIT_NATABLECACHE_UPPER_KB:
        ActiveSchemaDB()->getNATableDB()->setHeapUpperLimit((size_t) 1024 * atoi(value.data()));
        break;

      case NAMETYPE:
	SqlParser_NADefaults_->NAMETYPE_ = token(NAMETYPE, value, TRUE);
	break;
      case NATIONAL_CHARSET:
	SqlParser_NADefaults_->NATIONAL_CHARSET_ = CharInfo::getCharSetEnum(value);
	break;
      case SESSION_ID:
	{
	  CmpCommon::context()->sqlSession()->setSessionId(value);
	}
      break;
      case SESSION_USERNAME:
	{
	  CmpCommon::context()->sqlSession()->setSessionUsername(value);
	}
      break;
      case SESSION_IN_USE:
	{
	  CmpCommon::context()->sqlSession()->setSessionInUse
	    ((getToken(attrEnum) == DF_ON));
	}
      break;

      case SQLMX_REGRESS:
	{
	  if (value == "ON")
	    {
	      insert(SHOWDDL_DISPLAY_FORMAT, "INTERNAL", errOrWarn);
	      insert(MODE_SPECIAL_1, "OFF", errOrWarn);
	      if (getToken(VOLATILE_TABLE_FIND_SUITABLE_KEY) == DF_SYSTEM)
		{
		  insert(VOLATILE_TABLE_FIND_SUITABLE_KEY, "OFF", errOrWarn);
		}

	      char * env = getenv("SQLMX_REGRESS");
	      if (env)
		CmpCommon::context()->setSqlmxRegress(atoi(env));
	      else
		CmpCommon::context()->setSqlmxRegress(1);
	    }
	  else
	    {
	      insert(SHOWDDL_DISPLAY_FORMAT, "EXTERNAL", errOrWarn);

	      CmpCommon::context()->setSqlmxRegress(0);
	    }
	}
	break;

      case VOLATILE_CATALOG:
	{
	  CmpCommon::context()->sqlSession()->setVolatileCatalogName(value);
	}
      break;
      case VOLATILE_SCHEMA_IN_USE:
	{
	  CmpCommon::context()->sqlSession()->setVolatileSchemaInUse
	    ((getToken(attrEnum) == DF_ON));
	}
      break;

      case ISO_MAPPING:
	{
	  SqlParser_NADefaults_->ISO_MAPPING_ = CharInfo::getCharSetEnum(value);
	  // Set the NAString_isoMappingCS memory cache for use by routines
	  // ToInternalIdentifier() and ToAnsiIdentifier[2|3]() in module
	  // w:/common/NAString[2].cpp.  These routines currently cannot
	  // access SqlParser_ISO_MAPPING directly due to the complex
	  // build hierarchy.
	  NAString_setIsoMapCS((SQLCHARSET_CODE) SqlParser_NADefaults_->ISO_MAPPING_);
	}
	break;
	
      case DEFAULT_CHARSET:
	{
	  SqlParser_NADefaults_->DEFAULT_CHARSET_ = CharInfo::getCharSetEnum(value);
	  SqlParser_NADefaults_->ORIG_DEFAULT_CHARSET_ = CharInfo::getCharSetEnum(value);
	}
	break;
	
      case QUERY_TEXT_CACHE:
      {
        // If public schema is in use, query text cache has to be off
        NAString pSchema = getValue(PUBLIC_SCHEMA_NAME);
        if (pSchema != "")
          value = "OFF";
      }
      break;

      case PUBLIC_SCHEMA_NAME:
      {
        // when PUBLIC_SCHEMA is used, turn off Query Text Cache
        if ( (value != "") && !(getToken(QUERY_TEXT_CACHE) == DF_OFF) )
          insert(QUERY_TEXT_CACHE, "OFF");
        // when PUBLIC_SCHEMA is not used, reset to the default value
        if ( value == "" )
        {
          NAString v("");
          validateAndInsert("QUERY_TEXT_CACHE", v, TRUE);
        }
      }
      break;

      case LDAP_USERNAME:
      {
        // when the LDAP_USERNAME is set (first time by CLI)
        // if DEFAULT_SCHEMA_NAMETYPE is USER, set schema to LDAP_USERNAME
        if ( !value.isNull() &&
             (getToken(DEFAULT_SCHEMA_NAMETYPE) == DF_USER) &&
             !userDefault(SCHEMA) &&  // do not change user setting
             ( schSetToUserID() ||    // only when schema was initialized to guardian id
               schSetByNametype() ) ) // or changed by same CQD
        {
          setSchemaAsLdapUser(value);
          setSchByNametype(TRUE);
        }
      }
      break;

      case DEFAULT_SCHEMA_ACCESS_ONLY:
      {
        if ( value == "ON" )
        {
          NAString schemaNameType = getValue(DEFAULT_SCHEMA_NAMETYPE);
          if ( schemaNameType == "USER" )
          {
            setSchemaAsLdapUser();
          }
        }
      }
      break;

      case DEFAULT_SCHEMA_NAMETYPE:
      {
        if (  userDefault(SCHEMA) ) // if SCHEMA has been changed by user, do nothing
          break;

        if ( value == "SYSTEM" )
        // reset to default schema
        {
          if ( schSetByNametype() )      // only when schema was changed by this CQD
          {
            // do not change catSchSetToUserID_ flag
            Int32 preVal = catSchSetToUserID_;
            NAString v("");
            validateAndInsert("SCHEMA", v, TRUE);
            catSchSetToUserID_ = preVal;
          }
        }
        if ( value == "USER" )
        // set default schema to ldpa username
        {
          if ( schSetToUserID() ||   // only when schema was initialized to guardian id
               schSetByNametype() )  // or was changed by this CQD
          {
            setSchemaAsLdapUser();
            setSchByNametype(TRUE);
          }
        }
      }
      break;

      case USTAT_IUS_PERSISTENT_CBF_PATH:
      {
        // if the CBF path is SYSTEM, set it to $HOME/cbfs
        if ( value == "SYSTEM" ) {
           const char* home = getenv("HOME");

           if ( home ) {
              value = home;
              value += "/cbfs";
              validateAndInsert("USTAT_IUS_PERSISTENT_CBF_PATH", value, FALSE);
           } 
        }
      }
      break;

     case TRAF_LOAD_ERROR_LOGGING_LOCATION:
     {
        if (value.length() > 512)
        {
           *CmpCommon::diags() << DgSqlCode(-2055)
                                  << DgString0(value)
                                  << DgString1(lookupAttrName(attrEnum));
        }
     }
     break;

     case AGGRESSIVE_ESP_ALLOCATION_PER_CORE:
     {
        NABoolean useAgg = (getToken(attrEnum) == DF_ON);
        float numESPsPerCore = computeNumESPsPerCore(useAgg);
        char valuestr[WIDEST_CPUARCH_VALUE];
        ftoa_(numESPsPerCore, valuestr);
        NAString val(valuestr);
        insert(MAX_ESPS_PER_CPU_PER_OP, val, errOrWarn);
     }
     break;

     // max char col length is defined in common/ComSmallDefs.h.
     // In special cases, it could be overridden. Internal use only or
     // use only under trafodion supervision.
     case TRAF_MAX_CHARACTER_COL_LENGTH:
     {
       NABoolean override = (getToken(TRAF_MAX_CHARACTER_COL_LENGTH_OVERRIDE) == DF_ON);
       double d = atof(value.data());
       if ((NOT override) &&
           (NOT (d >= 0 && d <= MAX_CHAR_COL_LENGTH_IN_BYTES)))
         {
           *CmpCommon::diags() << DgSqlCode(-2055)
                               << DgString0(value)
                               << DgString1(lookupAttrName(attrEnum));
         }
     }
     break;

     case TRAF_MAX_CHARACTER_COL_LENGTH_OVERRIDE:
     {
       // if override is being turned off, reset max_char_len to default value.
       if (value == "OFF")
         {
           NAString val;
           validateAndInsert("TRAF_MAX_CHARACTER_COL_LENGTH", val, TRUE);
         }
     }
     break;
      case LOB_GC_LIMIT_SIZE:
        if ((atoi(value.data()) < -1)   ||  (((atoi(value.data()) >0) && (atoi(value.data())) <= 512)))
          {
            *CmpCommon::diags() << DgSqlCode(-2055)
                                  << DgString0(value)
                                  << DgString1(lookupAttrName(attrEnum));
          }
        break;

      case LOB_MAX_CHUNK_MEM_SIZE:
        if (atoi(value.data()) < 0  ||  atoi(value.data()) >  5120)
	 {
            *CmpCommon::diags() << DgSqlCode(-2055)
                                  << DgString0(value)
                                  << DgString1(lookupAttrName(attrEnum));
          }
        break;

      case LOB_INPUT_LIMIT_FOR_BATCH:
        if (atoi(value.data()) <0  && (atoi(value.data())*1024) > INT_MAX )
          {
            *CmpCommon::diags() << DgSqlCode(-2055)
                                  << DgString0(value)
                                  << DgString1(lookupAttrName(attrEnum));
          }
        break;
    default:  
    break;
    }
    }	  // code to valid overwrite (insert)

    if (reset && overwrite) {
      // CONTROL QUERY DEFAULT attr RESET;	-- this code cloned above
      //   Can't reset prov, because to which?
      //	  provenances_[attrEnum] =  READ_FROM_SQL_TABLE  or  COMPUTED ??
      NADELETEBASIC(resetToDefaults_[attrEnum], NADHEAP);
      resetToDefaults_[attrEnum] = NULL;
    }
    else if (!overwrite && errOrWarn && getProvenance(attrEnum) >= IMMUTABLE) {
      *CmpCommon::diags() << DgSqlCode(ERRWARN(2200))
			  << DgString0(lookupAttrName(attrEnum, errOrWarn));
    }
  }	  // valid attrName

  return attrEnum;

} // NADefaults::validateAndInsert()

float NADefaults::computeNumESPsPerCore(NABoolean aggressive)
{
   #define DEFAULT_ESPS_PER_NODE 2   // for conservation allocation
   #define DEFAULT_ESPS_PER_CORE 0.5 // for aggressive allocation

     // Make sure the gpClusterInfo points at an NAClusterLinux object.
     // In osim simulation mode, the pointer can point at a NAClusterNSK
     // object, for which the method numTSEsForPOS() is not defined.
   NAClusterInfoLinux* gpLinux = dynamic_cast<NAClusterInfoLinux*>(gpClusterInfo);
   assert(gpLinux);		

   // cores per node
   Lng32 coresPerNode = gpClusterInfo->numberOfCpusPerSMP();

   if ( aggressive ) {
      float totalMemory = gpLinux->totalMemoryAvailable(); // per Node, in KB
      totalMemory /= (1024*1024); // per Node, in GB
      totalMemory /= coresPerNode ; // per core, in GB
      totalMemory /= 2; // per core, 2GB per ESP
      return MINOF(DEFAULT_ESPS_PER_CORE, totalMemory);
   } else {
      Lng32 numESPsPerNode = DEFAULT_ESPS_PER_NODE;
      return (float)(numESPsPerNode)/(float)(coresPerNode);
   }

// The following lines of code are comment out but retained for possible
// future references.
//
//     // number of POS TSE
//   Lng32 numTSEsPerCluster = gpLinux->numTSEsForPOS();
//
//     // cluster nodes
//   Lng32 nodesdPerCluster = gpClusterInfo->getTotalNumberOfCPUs();
//
//     // TSEs per node
//   Lng32 TSEsPerNode = numTSEsPerCluster/nodesdPerCluster;
//
//
//
//     // For Linux/nt, we conservatively allocate ESPs per node as follows
//     // - 1 ESP per 2 cpu cores if cores are equal or less than TSEs
//     // - 1 ESP per TSE if number of cores is more than double the TSEs
//     // - 1 ESP per 2 TSEs if cores are more than TSEs but less than double the TSEs
//     // - 1 ESP per node. Only possible on NT or workstations
//     //      - number of cores less than TSEs and there are 1 or 2 cpur cores per node
//     //      - number of TSEs is less than cpu cores and there 1 or 2 TSEs per node.
//     //        This case is probable if virtual nodes are used
//
//     // TSEsPerNode is 0 for arkcmps started by the seapilot universal comsumers
//     // in this case we only consider cpu cores
//   if ( coresPerNode <= TSEsPerNode || TSEsPerNode == 0 )
//   {
//       if (coresPerNode > 1)
//           numESPsPerNode = DEFAULT_ESPS_PER_NODE; 
//   }
//   else if (coresPerNode > (TSEsPerNode*2))
//   {
//        numESPsPerNode = TSEsPerNode;
//   }
//   else if (TSEsPerNode > 1)
//   {
//        numESPsPerNode = TSEsPerNode/2;
//   }
//   else // not really needed since numESPsPerNode is set to 1 from above
//   {
//        numESPsPerNode = DEFAULT_ESPS_PER_NODE;
//   }
//        
//   return (float)(numESPsPerNode)/(float)(coresPerNode);
}

enum DefaultConstants NADefaults::holdOrRestore	(const char *attrName,
						 Lng32 holdOrRestoreCQD)
{
  DefaultConstants attrEnum = __INVALID_DEFAULT_ATTRIBUTE;
  if (holdOrRestoreCQD == 0)
    {
      *CmpCommon::diags() << DgSqlCode(-2050)
			  << DgString0(attrName);

      return attrEnum;
    }

  // Perform a lookup for the string, using a binary search.
  attrEnum = lookupAttrName(attrName, -1);

  if (attrEnum < 0)
    {
      *CmpCommon::diags() << DgSqlCode(-2050)
			  << DgString0(attrName);

      return attrEnum;
    }

  char * value = NULL;
  if (holdOrRestoreCQD == 1) // hold cqd
    {
      if (currentDefaults_[attrEnum])
	{
	  value = new NADHEAP char[strlen(currentDefaults_[attrEnum]) + 1];
	  strcpy(value, currentDefaults_[attrEnum]);
	}
      else
	{
	  value = new NADHEAP char[strlen(defaultDefaults[defDefIx_[attrEnum]].value) + 1];
	  strcpy(value, defaultDefaults[defDefIx_[attrEnum]].value);
	}

      if (! heldDefaults_[attrEnum])
        heldDefaults_[attrEnum] = new NADHEAP HeldDefaults();

      heldDefaults_[attrEnum]->pushDefault(value);
    }
  else
    {
      // restore cqd from heldDefaults_ array, if it was held.
      if (! heldDefaults_[attrEnum])
        return attrEnum;

      value = heldDefaults_[attrEnum]->popDefault();
      if (! value)
	return attrEnum;

      // there is an odd semantic that if currentDefaults_[attrEnum]
      // is null, we leave it as null, but pop a held value anyway;
      // this semantic was preserved when heldDefaults_ was converted
      // to a stack.

      if (currentDefaults_[attrEnum])
        {
          // do a validateAndInsert so the caches (such as currentToken_)
          // get updated and so appropriate semantic actions are taken.
          // Note that validateAndInsert will take care of deleting the
          // storage currently held by currentDefaults_[attrEnum].
          NAString valueS(value);
          validateAndInsert(lookupAttrName(attrEnum), // sad that we have to do a lookup again
                            valueS,
                            FALSE);
        }     
      NADELETEBASIC(value, NADHEAP);
    }

  return attrEnum;
}


const SqlParser_NADefaults *NADefaults::getSqlParser_NADefaults()
{
  return SqlParser_NADefaults_;
}

static void setCatSchErr(NAString &value,
			 Lng32 sqlCode,
			 Int32 errOrWarn,
			 NABoolean catErr = FALSE)
{
  if (!sqlCode || !errOrWarn) return;

  TrimNAStringSpace(value);		// prettify further (neater errmsg)
  *CmpCommon::diags() << DgSqlCode(ERRWARN(sqlCode))
    << DgCatalogName(value) << DgSchemaName(value)
    << DgString0(value)     << DgString1(value);

  if (value.first('"') == NA_NPOS) {	// delimited names too complicated !
    NAString namepart = value;
    size_t dot = value.first('.');
    if (dot != NA_NPOS) {
      namepart.remove(dot);
      if (!IsSqlReservedWord(namepart)) {
        namepart = value;
	namepart.remove(0, dot+1);
      }
    }
    if (IsSqlReservedWord(namepart)) {
      *CmpCommon::diags() << DgSqlCode(ERRWARN(3128))
	<< DgString0(namepart) << DgString1(namepart);
      return;
    }
  }

  // must determine if the defaults have been set up before parseDML is called
  if (IdentifyMyself::GetMyName() == I_AM_UNKNOWN){
    return;   							  // diagnostic already put into diags above.
	}
  // Produce additional (more informative) syntax error messages,
  // trying delimited-value first and then possibly regular-value-itself.
  Parser parser(CmpCommon::context());
  Lng32 errs = CmpCommon::diags()->getNumber(DgSqlCode::ERROR_);
  NAString pfx(catErr ? "SET CATALOG " : "SET SCHEMA ");
  NAString stmt;
  char c = *value.data();
  if (c && c != '\"') {
    stmt = pfx;
    stmt += "\"";
    stmt += value;
    stmt += "\"";
    stmt += ";";
    parser.parseDML(stmt, stmt.length(),
                    OBJECTNAMECHARSET
                    );
  }
  if (errs == CmpCommon::diags()->getNumber(DgSqlCode::ERROR_)) {
    stmt = pfx;
    stmt += value;
    stmt += ";";
    parser.parseDML(stmt, stmt.length(),
                    OBJECTNAMECHARSET
                    );
  }

  // Change errors to warnings if errOrWarn is +1 (i.e. warning).
  if (errOrWarn > 0) NegateAllErrors(CmpCommon::diags());
}

NABoolean NADefaults::setCatalog(NAString &value,
				 Int32 errOrWarn,
				 NABoolean overwrite,
				 NABoolean alreadyCanonical)
{
   setCatUserID(currentState_ == COMPUTED);

  // The input value is in external (Ansi) format.
  // If we are in the COMPUTED currentState_,
  // make the value strictly canonical,
  // and try non-delimited first, then delimited.
  // Prettify removes lead/trailing blanks,
  // and upcases where unquoted (for nicer errmsgs);
  // ComSchemaName parses/validates.
  //
  if (alreadyCanonical)
    ;				// leave it alone, for performance's sake
  else if (currentState_ == COMPUTED) {				// ' SQL.FOO'
    TrimNAStringSpace(value);					// 'SQL.FOO'
    NAString tmp(value);
    value = ToAnsiIdentifier(value);				// nondelim ok?
    if (value.isNull()) value = NAString("\"") + tmp + "\"";	// '"SQL.FOO"'
  }
  else
    PrettifySqlText(value);

  ComSchemaName nam(value);
  if (nam.getSchemaNamePart().isEmpty() ||	// 0 name parts, if *any* error
      !nam.getCatalogNamePart().isEmpty()) {	// 2 parts (cat.sch) is an error
    setCatSchErr(value, EXE_INVALID_CAT_NAME, errOrWarn, TRUE);
    return FALSE;				// invalid value
  }
  else {
    // Get the 1 name part (the "schema" part as far as ComSchema knows...)
    if (overwrite) insert(CATALOG, nam.getSchemaNamePartAsAnsiString());
    return TRUE;
  }
}



NABoolean NADefaults::setSchema(NAString &value,
				Int32 errOrWarn,
				NABoolean overwrite,
				NABoolean alreadyCanonical)
{
  // if this is part of CQD *RESET and it was initialized with role name
  // do not change the following flags
  // to allow DEFAULT_SCHEMA_NAMETYPE to set its value
  if (!( schSetToUserID() && isResetAll() ))
  {
    setSchUserID(currentState_ == COMPUTED);
    setSchByNametype(FALSE);
  }

  if (alreadyCanonical)
    ;				// leave it alone, for performance's sake
  else if (currentState_ == COMPUTED) {				// ' SQL.FOO'
    TrimNAStringSpace(value);					// 'SQL.FOO'
    NAString tmp(value);
    value = ToAnsiIdentifier(value);				// nondelim ok?
    if (value.isNull()) value = NAString("\"") + tmp + "\"";	// '"SQL.FOO"'
  }
  else
    PrettifySqlText(value);

  ComSchemaName nam(value);
  if (nam.getSchemaNamePart().isEmpty()) {	// 0 name parts, if *any* error
    setCatSchErr(value, EXE_INVALID_SCH_NAME, errOrWarn);
    return FALSE;				// invalid value
  }
  else {
    if (overwrite)
      insert(SCHEMA,
	     nam.getSchemaNamePartAsAnsiString());
    
    // If 2 parts, overwrite any prior catalog default
    if (!nam.getCatalogNamePart().isEmpty()) {
      if (overwrite)
	{
	  insert(CATALOG,
		 nam.getCatalogNamePartAsAnsiString());

	  if (currentState_ == SET_BY_CQD)
	    {
	      // indicate that this attribute was set by a user CQD.
	      setUserDefault(CATALOG, TRUE);
	    }
	}
    }
    return TRUE;
  }
}

NAString NADefaults::keyword(DefaultToken tok)
{
    CMPASSERT(tok >= 0 && tok < DF_lastToken);
    return keywords_[tok];
}

// Defaults Tokens
// There is a set of keywords which can appear as values of Defaults entries
// in the Defaults Table.  We declare, for each such token, a string (the
// keyword), and an enumeration value.  The string values belong in an
// array, DFkeywords, in sorted order.  The idea is we can use binary
// search in order to obtain the index of a string to the matching
// entry in this sorted array.
//
// If we define the enumerations carefully (pay attention here!), then
// that index we just found (see previous paragraph) is the enum value
// of the token.

// In simple words: this has to be in identical order with enum DefaultToken
// in DefaultConstants.h

const char *NADefaults::keywords_[DF_lastToken] = {
  "ACCUMULATED",
  "ADVANCED",
  "AGGRESSIVE",
  "ALL",
  "ANSI",
  "BEGINNER",
  "BOTH",
  "CLEAR",
  "DEBUG",
  "DISK",
  "DISK_POOL",
  "DUMP",
  "DUMP_MV",
  "EXTERNAL",
  "EXTERNAL_DETAILED",
  "FIRSTROW",
  "HARDWARE",
  "HEAP",
  "HIGH",
  "HYBRID",
  "IEEE",
  "INDEXES",
  "INTERNAL",
  "IQS",
  "JNI",
  "JNI_TRX",
  "KEYINDEXES",
  "LASTROW",
  "LATEST",
  "LEAF",
  "LOADNODUP",
  "LOCAL",
  "LOCAL_NODE",
  "LOG",
  "MAXIMUM",
  "MEDIUM",
  "MEDIUM_LOW",
  "MERGE",
  "MINIMUM",
  "MMAP",
  "MULTI_NODE",
  "MVCC",
  "NONE",
  "OFF",
  "ON",
  "OPENS_FOR_WRITE",
  "OPERATOR",
  "OPTIMAL",
  "ORDERED",
  "PERTABLE",
  "PRINT",
  "PRIVATE",
  "PUBLIC",
  "QS",
  "READ_COMMITTED",
  "READ_UNCOMMITTED",
  "RELEASE",
  "REMOTE",
  "REPEATABLE_READ",
  "REPLACE",
  "REPSEL",
  "RESOURCES",
  "RETURN",
  "ROOT",
  "SAMPLE",
  "SERIALIZABLE",
  "SHORTANSI",
  "SIMPLE",
  "SKIP",
  "SMD",
  "SOFTWARE",
  "SOURCE",
  "SQLMP",
  "SSCC",
  "SSD",
  "STOP",
  "SUFFIX",
  "SYSTEM",
  "TANDEM",
  "THRIFT",
  "USER",
  "VERTICAL",
  "WAIT",
  "WARN",
  "XML"
};

// To call bsearch we must satisfy each of its arguments.  Either
// NULL comes back, or, comes back a pointer to the element which is
// a true match for our key.  bsearch.key is upperKey.data().
// bsearch.base is keywords_.  nel is DF_lastToken.
// The next argument is sizeof char*.  Finally, the comparison
// function can simply be the strcmp function.
//
// Note that this function makes heavy reliance on the idea that
// the DefaultToken enumerations go up in sequence 0, 1, 2, 3... .
//
// We do the cast on strcmp because its signature from the header
// file is: int (*)(const char *, const char *).  In general, we're
// doing a lot of type casting in here.

static Int32 stringCompare(const void* s1, const void* s2)
{ return strcmp( * (char**) s1, * (char**) s2); }

DefaultToken NADefaults::token(Int32 attrEnum,
			       NAString &value,
			       NABoolean valueAlreadyGotten,
			       Int32 errOrWarn) const
{
  ATTR_RANGE_ASSERT;

  if (!valueAlreadyGotten) {
    value = getValue(attrEnum);	// already trim & upper (by validateAndInsert)
    TrimNAStringSpace(value);	// can't trust that the stored value is canonical
  }
  else {
    TrimNAStringSpace(value);	// can't trust that input value is canonical,
    value.toUpper();		// so here do what validateAndInsert does
  }

  DefaultToken tok = DF_noSuchToken;

  if (value.isNull())
    tok = DF_SYSTEM;
  else {
    if ((attrEnum == TERMINAL_CHARSET) ||
        (attrEnum == USE_HIVE_SOURCE) ||
        (attrEnum == HIVE_FILE_CHARSET) ||
        (attrEnum == HBASE_DATA_BLOCK_ENCODING_OPTION) ||
        (attrEnum == HBASE_MEMSTORE_FLUSH_SIZE_OPTION) ||
        (attrEnum == HBASE_COMPRESSION_OPTION))
      return DF_USER;
    
    if ( attrEnum == NATIONAL_CHARSET ||
         attrEnum == DEFAULT_CHARSET ||
         attrEnum == HIVE_DEFAULT_CHARSET ||
         attrEnum == ISO_MAPPING ||
         attrEnum == INPUT_CHARSET ||
         attrEnum == TRAF_DEFAULT_COL_CHARSET )
      {
	CharInfo::CharSet cs = CharInfo::getCharSetEnum(value);
        Int32 err_found = 0;

	if ( !CharInfo::isCharSetSupported(cs) )
	  {
	    err_found = 1;
	  } else {
	    switch( attrEnum ) {
	    case NATIONAL_CHARSET:
	      if (cs == CharInfo::KANJI_MP) break; //Allow (for regression test)
	      if ((cs != CharInfo::UNICODE) && (cs != CharInfo::ISO88591))
		err_found = 1;
	      break;
	    case DEFAULT_CHARSET:
	      if (cs != CharInfo::ISO88591
		  && cs != CharInfo::UTF8
		  // && cs != CharInfo::SJIS
		 )
		err_found = 1;
	      break;
	    case HIVE_DEFAULT_CHARSET:
            case TRAF_DEFAULT_COL_CHARSET:
	      if ((cs != CharInfo::UTF8) && (cs != CharInfo::ISO88591))
		err_found = 1;
	      break;
	    case ISO_MAPPING:
	      if (cs != CharInfo::ISO88591)
		err_found = 1;
	      break;
	    default:
	      break;
	    }
	  }
        
	if ( (err_found != 0) && errOrWarn )
	  *CmpCommon::diags() << DgSqlCode(ERRWARN(3010)) << DgString0(value);
	else
	  return DF_USER;			// kludge, return any valid token
      } //else
	//else fall thru to see if value is SYSTEM

	// OPTIMIZATION_LEVEL
      if ((attrEnum == OPTIMIZATION_LEVEL) &&
	  value.length() == 1)
	switch (*value.data()) {
	  case '0':	return DF_MINIMUM;
	  case '1':	return DF_MINIMUM;
	  case '2':	return DF_MEDIUM_LOW;
	  case '3':	return DF_MEDIUM;
	  case '4':	return DF_MEDIUM;
	  case '5':	return DF_MAXIMUM;
	}

      // PCODE_OPT_LEVEL
      if ((attrEnum == PCODE_OPT_LEVEL) &&
	  value.length() == 1)
	switch (*value.data()) {
	  case '0':	return DF_MINIMUM;
	  case '1':	return DF_MEDIUM;
	  case '2':	return DF_HIGH;
	  case '3':	return DF_MAXIMUM;
	}
      // HBASE_FILTER_PREDS
      if ((attrEnum == HBASE_FILTER_PREDS) && value.length()==1)
      switch (*value.data()){
        case '0': return DF_OFF;
        case '1': return DF_MINIMUM;
        case '2': return DF_MEDIUM;
        // in the future add DF_HIGH and DF_MAXIMUM when we implement more 
        // pushdown capabilities
      }
      
      if   (attrEnum == MVQR_REWRITE_CANDIDATES ||
            attrEnum == MVQR_WORKLOAD_ANALYSIS_MV_NAME ||
            attrEnum == HIST_SCRATCH_VOL)
        return DF_SYSTEM;   
        
      const char *k = value.data();
      char *match = (char*) bsearch(
           &k, keywords_, DF_lastToken, sizeof(char*), stringCompare);
      if (match)
        tok = (DefaultToken) (((const char**) match) - keywords_);

      else {	// Check for synonyms

        const char *c = value;
        for (; *c == '0'; c++) ;	// all ascii '0' ?
        if (*c == '\0')					// terminating nul '\0'
          tok = DF_OFF;

        else if (value.length() <= 2) {
          if (value == "1" || value == "+1" || value == "-1")
            tok = DF_ON;
        }

        else {
          if ((value == "STOP_AT") || (value == "STOP AT"))
            tok = DF_STOP;
          else if (value == "READ COMMITTED")
            tok = DF_READ_COMMITTED;
          else if (value == "READ UNCOMMITTED")
            tok = DF_READ_UNCOMMITTED;
          else if (value == "REPEATABLE READ")
            tok = DF_REPEATABLE_READ;
          else if (value == "BEGINNER")
            tok = DF_BEGINNER;
          else if (value == "ADVANCED")
            tok = DF_ADVANCED;


#define CONVERT_SYNONYM(from,to)                        \
	  else if (value == "" # from "") {             \
	    CMPASSERT(DF_ ## from == DF_ ## to);        \
	    tok = DF_ ## to;                            \
	  }
          CONVERT_SYNONYM(COMPAQ,  TANDEM)
            CONVERT_SYNONYM(DISABLE, OFF)
            CONVERT_SYNONYM(ENABLE,  SYSTEM)
            CONVERT_SYNONYM(FALSE,   OFF)
            CONVERT_SYNONYM(FULL,    MAXIMUM)
            CONVERT_SYNONYM(TRUE,    ON)
            }
      }
  }

  NABoolean isValid = FALSE;

  if (tok != DF_noSuchToken)
    switch (attrEnum) {

    case DEFAULT_SCHEMA_ACCESS_ONLY:
      if (tok == DF_ON || tok == DF_OFF)
        isValid = TRUE;
      break;

    case DEFAULT_SCHEMA_NAMETYPE:
      if (tok == DF_SYSTEM || tok == DF_USER)
    	  isValid = TRUE;
      break;

    case DETAILED_STATISTICS:
      if (tok == DF_ALL ||
	  tok == DF_ACCUMULATED || tok == DF_OPERATOR ||
	  tok == DF_PERTABLE || tok == DF_OFF)
	isValid = TRUE;
      break;

    case HIDE_INDEXES:
      if (tok  == DF_NONE        || tok == DF_ALL        ||
	  tok  == DF_VERTICAL    || tok == DF_INDEXES    || tok == DF_KEYINDEXES)
	isValid = TRUE;
    break;

    case HIVE_USE_EXT_TABLE_ATTRS:
      if (tok == DF_ALL || tok == DF_OFF || tok == DF_ON )
	isValid = TRUE;
      break;

    case INDEX_ELIMINATION_LEVEL:
      if  (tok == DF_MINIMUM	 || tok == DF_MEDIUM ||
	   tok == DF_MAXIMUM     || tok == DF_AGGRESSIVE )
	isValid = TRUE;
      break;
    case IF_LOCKED:
      if (tok == DF_RETURN	 || tok == DF_WAIT)
        isValid = TRUE;
      break;

    case INSERT_VSBB:
      if (tok == DF_OFF	 	 || tok == DF_LOADNODUP ||
          tok == DF_SYSTEM	 || tok == DF_USER)
        isValid = TRUE;
      break;
    case OVERFLOW_MODE:
      if (tok == DF_DISK || tok == DF_SSD ||
	  tok == DF_MMAP)
	isValid = TRUE;
      break;
	case SORT_ALGO:
        if(tok == DF_HEAP || tok == DF_IQS ||
           tok == DF_REPSEL || tok == DF_QS)
          isValid = TRUE;
     break;

    case QUERY_TEMPLATE_CACHE:
    case SHARE_TEMPLATE_CACHED_PLANS:
    case VSBB_TEST_MODE:
      if (tok == DF_ON || tok == DF_OFF)
	isValid = TRUE;
      break;

    case QUERY_TEXT_CACHE:
      if (tok == DF_ON || tok == DF_OFF || tok == DF_SYSTEM || tok == DF_SKIP)
	isValid = TRUE;
      break;

    case DISABLE_BUFFERED_INSERTS:
      if (tok == DF_ON || tok == DF_OFF)
	isValid = TRUE;
      break;

    case ISOLATION_LEVEL:
      {
        TransMode::IsolationLevel iltmp;
        isValid = getIsolationLevel(iltmp, tok);
      }
    break;

    case ISOLATION_LEVEL_FOR_UPDATES:
      {
        TransMode::IsolationLevel iltmp;
        isValid = getIsolationLevel(iltmp, tok);
      }
    break;

    case MVGROUP_AUTOMATIC_CREATION:
    case MV_TRACE_INCONSISTENCY: //++ MV
    case MV_AS_ROW_TRIGGER:	 //++ MV
      {
	if(DF_ON == tok || DF_OFF == tok)
	{
	  isValid = TRUE;

	}
      }
      break;

    case IUD_NONAUDITED_INDEX_MAINT:
      if (tok == DF_OFF	 	 || tok == DF_SYSTEM	 ||
	  tok == DF_WARN         || tok == DF_ON)
	isValid = TRUE;
      break;

    case HIVE_SCAN_SPECIAL_MODE:
	isValid = TRUE;
	break;

    case IS_SQLCI:
      // for primary mxcmp that is invoked for user queries, the only valid
      // value for mxci_process cqd is TRUE. This cqd is set once by mxci
      // at startup time and cannot be changed by user. That way we know that
      // a request has come in from mxci(trusted) process.
      // For secondary mxcmp's invoked for internal queries where cqd's are
      // sent using sendAllControls method, all values are valid. This will
      // ensure that if this default is not set and is sent over to secondary
      // mxcmp using an internal CQD statement, it doesn't return an error.
      if (tok == DF_ON		 || tok == DF_OFF)
        isValid = TRUE;
      break;

    case NVCI_PROCESS:
      // for primary mxcmp that is invoked for user queries, the only valid
      // value for nvci_process cqd is TRUE. This cqd is set once by nvci
      // at startup time and cannot be changed by user. That way we know that
      // a request has come in from nvci(trusted) process.
      // For secondary mxcmp's invoked for internal queries where cqd's are
      // sent using sendAllControls method, all values are valid. This will
      // ensure that if this default is not set and is sent over to secondary
      // mxcmp using an internal CQD statement, it doesn't return an error.
      if (tok == DF_ON		 || tok == DF_OFF)
        isValid = TRUE;
      break;

    case NAMETYPE:
      if (tok == DF_ANSI	 || tok == DF_SHORTANSI)
        isValid = TRUE;
      break;

    case OPTIMIZATION_GOAL:
      if (tok == DF_FIRSTROW	 || tok == DF_LASTROW ||
          tok == DF_RESOURCES)
        isValid = TRUE;
      break;

    case USER_EXPERIENCE_LEVEL:
      if (tok == DF_ADVANCED	 || tok == DF_BEGINNER)
        isValid = TRUE;
      break;


    case PCODE_OPT_LEVEL:
      if (tok == DF_OFF)
	{
	  isValid = TRUE;
	  break;
	}
      // else fall through to the next case, all those keywords are allowed
      // as well

    case ATTEMPT_ESP_PARALLELISM:
      if (tok == DF_SYSTEM	 || tok == DF_ON ||
          tok == DF_OFF	 || tok == DF_MAXIMUM)
        isValid = TRUE;
      break;

    case OPTIMIZATION_LEVEL:
      if (tok == DF_MINIMUM	 || tok == DF_MEDIUM_LOW ||
          tok == DF_MEDIUM	 || tok == DF_MAXIMUM)
        isValid = TRUE;
      break;

    case HBASE_FILTER_PREDS:
        if(tok == DF_OFF || tok == DF_ON)
        {
            if (tok == DF_ON)
                tok = DF_MINIMUM; // to keep backward compatibility
        isValid= TRUE;
        }
        break;

    case ROBUST_QUERY_OPTIMIZATION:
      if (tok == DF_MINIMUM || tok == DF_SYSTEM || tok == DF_MAXIMUM ||
          tok == DF_HIGH)
        isValid = TRUE;
      break;

    case REFERENCE_CODE:
    case TARGET_CODE:
      if (tok == DF_RELEASE	 || tok == DF_DEBUG)
        isValid = TRUE;
      break;

      /*    case ROLLBACK_ON_ERROR:
      if (tok == DF_OFF	 	 || tok == DF_ON ||
          tok == DF_SYSTEM)
        isValid = TRUE;
      break;
      */

    case AUTO_QUERY_RETRY:
      if (tok == DF_ON ||
	  tok == DF_OFF ||
	  tok == DF_SYSTEM)
	isValid = TRUE;
      break;

    case AUTO_QUERY_RETRY_WARNINGS:
    case USE_LIBHDFS:
      if (tok == DF_ON ||
	  tok == DF_OFF)
	isValid = TRUE;
      break;

    case EXE_PARALLEL_DDL:
      if (tok == DF_OFF ||
	  tok == DF_ON ||
	  tok == DF_EXTERNAL ||
	  tok == DF_INTERNAL)
	isValid = TRUE;
      break;

    case UNAVAILABLE_PARTITION:
      if (tok == DF_SKIP	 || tok == DF_STOP)
        isValid = TRUE;
      break;

    case QUERY_CACHE_STATISTICS:
      // on, off are no-ops
      if (tok == DF_PRINT || tok == DF_ON || tok == DF_OFF) isValid = TRUE;
      break;

    case QUERY_CACHE_STATEMENT_PINNING:
      if (tok == DF_CLEAR || tok == DF_ON || tok == DF_OFF) isValid = TRUE;
      break;

    case HJ_TYPE:
       if (tok == DF_ORDERED || tok == DF_HYBRID || tok == DF_SYSTEM)
	 isValid = TRUE;
      break;

    case REF_CONSTRAINT_NO_ACTION_LIKE_RESTRICT:
      if (tok == DF_OFF	 	 || tok == DF_ON ||
          tok == DF_SYSTEM)
        isValid = TRUE;
      break;

    case POS:
      if (tok == DF_LOCAL_NODE  ||  tok == DF_OFF ||
          tok == DF_MULTI_NODE  ||  tok == DF_DISK_POOL)
        isValid = TRUE;
      break;

    case USTAT_INTERNAL_SORT:
      if (tok == DF_ON || tok == DF_OFF || tok == DF_HYBRID)
        isValid = TRUE;
      break;

    case USTAT_AUTO_FOR_VOLATILE_TABLES:
      if (tok == DF_ON || tok == DF_OFF)
        isValid = TRUE;
      break;

    case SUBQUERY_UNNESTING:
      if (tok == DF_OFF || tok == DF_ON || tok == DF_DEBUG)
        isValid = TRUE;
      break;

    case SUBQUERY_UNNESTING_P2:
      if (tok == DF_OFF || tok == DF_ON || tok == DF_INTERNAL)
         isValid = TRUE;
      break;

    case SORT_INTERMEDIATE_SCRATCH_CLEANUP:
      if(tok == DF_ON || tok == DF_OFF)
        isValid = TRUE;
      break;

     case SORT_MEMORY_QUOTA_SYSTEM:
      if(tok == DF_ON || tok == DF_OFF)
        isValid = TRUE;
      break;
    /*
	If MDAM_SCAN_METHOD's value is "MAXIMUM" only, Right side of Nested Join will
	use the MDAM path
	Allowable values for MDAM_SCAN_METHOD are 'ON' | 'OFF' | 'MAXIMUM'
	*/
    case MDAM_SCAN_METHOD:
      if (tok == DF_ON || tok == DF_OFF  || tok == DF_MAXIMUM)
        isValid = TRUE;
      break;

    case SHOWDDL_DISPLAY_FORMAT:
      if (tok == DF_INTERNAL || tok == DF_EXTERNAL || tok == DF_LOG)
	isValid = TRUE;
      break;

    case SHOWDDL_DISPLAY_PRIVILEGE_GRANTS:
      if (tok == DF_SYSTEM || tok == DF_ON || tok == DF_OFF)
	isValid = TRUE;
      break;

    case EXPLAIN_DISPLAY_FORMAT:
      if (tok == DF_INTERNAL || tok == DF_EXTERNAL || tok == DF_EXTERNAL_DETAILED)
	isValid = TRUE;
      break;

    case UPDATE_CLUSTERING_OR_UNIQUE_INDEX_KEY:
      if (tok == DF_ON || tok == DF_OFF  || tok == DF_AGGRESSIVE)
        isValid = TRUE;
      break;

    case MVQR_ALL_JBBS_IN_QD:
    case MVQR_REWRITE_ENABLED_OPTION:
    case MVQR_REWRITE_SINGLE_TABLE_QUERIES:
    case MVQR_USE_EXTRA_HUB_TABLES:
    case MVQR_ENABLE_LOGGING:
      if (tok == DF_ON || tok == DF_OFF)
	isValid = TRUE;
      break;

    case MVQR_LOG_QUERY_DESCRIPTORS:
      if (tok == DF_OFF || tok == DF_DUMP || tok == DF_DUMP_MV || tok == DF_LOG)
	isValid = TRUE;
      break;

    case MVQR_PRIVATE_QMS_INIT:
      if (tok == DF_SMD || tok == DF_XML || tok == DF_NONE)
	isValid = TRUE;
      break;

    case MVQR_PUBLISH_TO:
      if (tok == DF_PUBLIC || tok == DF_PRIVATE || tok == DF_BOTH || tok == DF_NONE)
	isValid = TRUE;
      break;

    case MVQR_WORKLOAD_ANALYSIS_MV_NAME:
      isValid = TRUE;
      break;

    case ELIMINATE_REDUNDANT_JOINS:
      if (tok == DF_OFF || tok == DF_ON || tok == DF_DEBUG || tok == DF_MINIMUM)
        isValid = TRUE;
      break;

    case VOLATILE_TABLE_FIND_SUITABLE_KEY:
      if (tok == DF_SYSTEM	 || tok == DF_ON ||
          tok == DF_OFF)
        isValid = TRUE;
      break;

    case CAT_DISTRIBUTE_METADATA:
      if (tok == DF_OFF  ||  tok == DF_LOCAL_NODE  ||  tok == DF_ON)
        isValid = TRUE;
      break;

    case MV_DUMP_DEBUG_INFO:
      if (tok == DF_OFF  ||  tok == DF_ON)
        isValid = TRUE;
      break;

    case RANGESPEC_TRANSFORMATION:
      if (tok == DF_OFF || tok == DF_ON || tok == DF_MINIMUM)
        isValid = TRUE;
      break;

    case ASYMMETRIC_JOIN_TRANSFORMATION:
      if (tok == DF_MINIMUM || tok == DF_MAXIMUM)
        isValid = TRUE;
      break;

    case CAT_DEFAULT_COMPRESSION:
      if (tok == DF_NONE  || tok == DF_HARDWARE || tok == DF_SOFTWARE)
        isValid = TRUE;
      break;

    case REPLICATE_DISK_POOL:
      if (tok == DF_ON || tok == DF_OFF)
        isValid = TRUE;
      break;

    case COMPRESSION_TYPE:
      if (tok == DF_NONE  || tok == DF_HARDWARE || tok == DF_SOFTWARE)
        isValid = TRUE;
      break;

    // The DF_SAMPLE setting indicates that the persistent sample will be
    // updated incrementally, but not the histograms; they will be created
    // anew from the incrementally updated sample.
    case USTAT_INCREMENTAL_UPDATE_STATISTICS:
      if (tok == DF_OFF  || tok == DF_SAMPLE || tok == DF_ON)
        isValid = TRUE;
      break;

    case REPLICATE_COMPRESSION_TYPE:
      if (tok == DF_NONE  || tok == DF_HARDWARE || tok == DF_SOFTWARE
           || tok == DF_SOURCE || tok == DF_SYSTEM)
        isValid = TRUE;
      break;

    case REUSE_OPENS:
      if (tok==DF_ON || tok == DF_OFF || tok == DF_OPENS_FOR_WRITE)
        isValid = TRUE;
      break;

    case USE_HIVE_SOURCE:
      isValid = TRUE;
      break;

    case TRAF_SIMILARITY_CHECK:
      if (tok == DF_ROOT || tok == DF_LEAF || tok == DF_ON || tok == DF_OFF)
	isValid = TRUE;
      break;

    case TRAF_TABLE_SNAPSHOT_SCAN:
      if (tok  == DF_NONE || tok == DF_SUFFIX || tok == DF_LATEST)
        isValid = TRUE;
    break;

    case TRAF_TRANS_TYPE:
      if (tok  == DF_MVCC || tok == DF_SSCC)
        isValid = TRUE;
    break;

    case HBASE_RANGE_PARTITIONING_PARTIAL_COLS:
      if (tok == DF_OFF || tok == DF_MINIMUM ||
          tok == DF_MEDIUM || tok == DF_MAXIMUM || tok == DF_ON)
        isValid = TRUE;
      break;
    case TRAF_UPSERT_MODE:
      if (tok == DF_MERGE || tok == DF_REPLACE || tok == DF_OPTIMAL)
	isValid = TRUE;
      break;

    // Nothing needs to be added here for ON/OFF/SYSTEM keywords --
    // instead, add to DEFAULT_ALLOWS_SEPARATE_SYSTEM code in the ctor.

    default:
      if (tok == DF_ON		 || tok == DF_OFF)
	isValid = TRUE;
      break;
    }

  // See "NOTE 2" way up top.
  if (!isValid) {
    if (tok == DF_SYSTEM) {
      isValid = isFlagOn(attrEnum, DEFAULT_ALLOWS_SEPARATE_SYSTEM);
      if (!isValid) {
	NAString tmp(getDefaultDefaultValue(attrEnum));
	isValid = isSynonymOfSYSTEM(attrEnum, tmp);
      }
    }
  }

  if (!isValid) {
    tok = DF_noSuchToken;
    if (errOrWarn)
      *CmpCommon::diags() << DgSqlCode(ERRWARN(2055))
	<< DgString0(value) << DgString1(lookupAttrName(attrEnum));
  }

  return tok;
}

DefaultToken NADefaults::getToken( const Int32 attrEnum,
                                   const Int32 errOrWarn ) const
{
  // Check the cache first.
  if ( currentTokens_[attrEnum] != NULL )
  {
    return *currentTokens_[attrEnum];
  }

  // Get the token and allocate memory to store the token value.
  NAString tmp( NADHEAP );
  currentTokens_[attrEnum]  = new NADHEAP DefaultToken;
  *currentTokens_[attrEnum] = token( attrEnum, tmp, FALSE, errOrWarn );

  return *currentTokens_[attrEnum];
}

NABoolean NADefaults::getIsolationLevel(TransMode::IsolationLevel &arg,
					DefaultToken tok) const
{
  NABoolean specifiedOK = TRUE;

  if (tok == DF_noSuchToken)
    tok = getToken(ISOLATION_LEVEL);

  switch (tok) {
    case DF_READ_COMMITTED:	arg = TransMode::READ_COMMITTED_;	break;
    case DF_READ_UNCOMMITTED:	arg = TransMode::READ_UNCOMMITTED_;	break;
    case DF_REPEATABLE_READ:	arg = TransMode::REPEATABLE_READ_;	break;

    case DF_SERIALIZABLE:
    case DF_SYSTEM:		arg = TransMode::SERIALIZABLE_;		break;
    case DF_NONE:		arg = TransMode::IL_NOT_SPECIFIED_;	break;

    default:			arg = TransMode::SERIALIZABLE_;
				specifiedOK = FALSE;
				NAString value(NADHEAP);
				if (tok != DF_noSuchToken) value = keyword(tok);
				*CmpCommon::diags() << DgSqlCode(-2055)
				  << DgString0(value)
				  << DgString1("ISOLATION_LEVEL");
  }

  return specifiedOK;
}

// find the packed length for all the default values stored
// in currentDefaults_ array.
// currentDefaults_ is a fixed sized array of "char *" where each
// entry is pointing to the default value for that default.
// After pack, the default values are put in the buffer in
// sequential order with a null terminator.
Lng32 NADefaults::packedLengthDefaults()
{
  Lng32 size = 0;

  const size_t numAttrs = numDefaultAttributes();

  for (size_t i = 0; i < numAttrs; i++)
    {
      size += strlen(currentDefaults_[i]) + 1;
    }

  return size;
}

Lng32 NADefaults::packDefaultsToBuffer(char * buffer)
{
  const size_t numAttrs = numDefaultAttributes();

  Lng32 totalSize = 0;
  Lng32 size = 0;

  for (UInt32 i = 0; i < numAttrs; i++)
    {
      size = (Lng32)strlen(currentDefaults_[i]) + 1;

      strcpy(buffer, currentDefaults_[i]);

      buffer += size;

      totalSize += size;
    }


  return totalSize;
}

Lng32 NADefaults::unpackDefaultsFromBuffer(Lng32 numEntriesInBuffer,
					  char * buffer)
{
  return 0;
}

NABoolean NADefaults::isSameCQD(Lng32 numEntriesInBuffer,
				char * buffer, Lng32 bufLen)
{
  const Lng32 numCurrentDefaultAttrs = (Lng32)numDefaultAttributes();

  // check to see if the default values in 'buffer' are the same
  // as those in the currentDefaults_ array.
  // Return TRUE if they are all the same.
  if (numCurrentDefaultAttrs != numEntriesInBuffer)
    return FALSE;

  if (bufLen == 0)
    return FALSE;

  Int32 curPos = 0;
  for (Int32 i = 0; i < numEntriesInBuffer; i++)
    {
      if (strcmp(currentDefaults_[i], &buffer[curPos]) != 0)
	return FALSE;
      curPos += strlen(&buffer[curPos]) + 1;
    }

  // everything matches.
  return TRUE;
}

Lng32 NADefaults::createNewDefaults(Lng32 numEntriesInBuffer,
				   char * buffer)
{
  const Lng32 numCurrentDefaultAttrs = (Lng32)numDefaultAttributes();

  // save the current defaults
  savedCurrentDefaults_ = currentDefaults_;
  savedCurrentFloats_   = currentFloats_;
  savedCurrentTokens_   = currentTokens_;

  // VO, Plan Versioning Support.
  //
  //     This code may execute in a downrev compiler, which knows about fewer
  //     defaults than the compiler originally used to compile the statement.
  //     Only copy those defaults we know about, and skip the rest.
  Lng32 numEntriesToCopy = _min (numEntriesInBuffer, numCurrentDefaultAttrs);

  // allocate a new currentDefaults_ array and make it point to
  // the default values in the input 'buffer'.
  // If the current number of default attributes are greater than the
  // ones in the input buffer, then populate the remaining default
  // entries in the currentDefaults_ array with the values from the
  // the savedCurrentDefaults_.
  currentDefaults_	= new NADHEAP const char * [numCurrentDefaultAttrs];

  Int32 curPos = 0;
  Int32 i = 0;
  for (i = 0; i < numEntriesToCopy; i++)
    {
      currentDefaults_[i] = &buffer[curPos];
      curPos += strlen(&buffer[curPos]) + 1;
    }

  for (i = numEntriesToCopy; i < numCurrentDefaultAttrs; i++)
    {
      currentDefaults_[i] = savedCurrentDefaults_[i];
    }

  // allocate two empty arrays for floats and tokens.
  currentFloats_	= new NADHEAP float * [numCurrentDefaultAttrs];
  currentTokens_	= new NADHEAP DefaultToken * [numCurrentDefaultAttrs];
  memset( currentFloats_, 0, sizeof(float *) * numCurrentDefaultAttrs );
  memset( currentTokens_, 0, sizeof(DefaultToken *) * numCurrentDefaultAttrs );

  return 0;
}

Lng32 NADefaults::restoreDefaults(Lng32 numEntriesInBuffer,
				 char * buffer)
{
  // Deallocate the currentDefaults_ array.
  // The array entries are not to be deleted as they point to
  // entries in 'buffer' or the 'savedCurrentDefaults_'.
  // See NADefaults::createNewDefaults() method.
  if (currentDefaults_)
    {
      NADELETEBASIC(currentDefaults_, NADHEAP);
    }

  if (currentFloats_)
    {
      for (size_t i = numDefaultAttributes(); i--; )
	NADELETEBASIC(currentFloats_[i], NADHEAP);
      NADELETEBASIC(currentFloats_, NADHEAP);
    }

  if (currentTokens_)
    {
      for (size_t i = numDefaultAttributes(); i--; )
	NADELETEBASIC(currentTokens_[i], NADHEAP);
      NADELETEBASIC(currentTokens_, NADHEAP);
    }

  // restore the saved defaults
  currentDefaults_ = savedCurrentDefaults_;
  currentFloats_   = savedCurrentFloats_;
  currentTokens_   = savedCurrentTokens_;

  return 0;
}

void NADefaults::updateCurrentDefaultsForOSIM(DefaultDefault * defaultDefault,
                                              NABoolean validateFloatVal)
{
  Int32 attrEnum = defaultDefault->attrEnum;
  const char * defaultVal = defaultDefault->value;
  const char * valueStr = currentDefaults_[attrEnum];

  if(valueStr)
  {
    NADELETEBASIC(valueStr,NADHEAP);
  }

  char * value = new NADHEAP char[strlen(defaultVal) + 1];
  strcpy(value, defaultVal);
  currentDefaults_[attrEnum] = value;

  if ( validateFloatVal )
  {
    float floatVal = 0;
    if (validateFloat(currentDefaults_[attrEnum], floatVal, attrEnum)) {
      if (currentFloats_[attrEnum]) {
        NADELETEBASIC(currentFloats_[attrEnum], NADHEAP);
      }
      currentFloats_[attrEnum] = new NADHEAP float;
      *currentFloats_[attrEnum] = floatVal;
    }
  }

  if ( currentTokens_[attrEnum] )
  {
    NADELETEBASIC( currentTokens_[attrEnum], NADHEAP );
    currentTokens_[attrEnum] = NULL;
  }
}

void NADefaults::setSchemaAsLdapUser(const NAString val)
{
  NAString ldapUsername = val;
  if ( ldapUsername.isNull() )
    ldapUsername = getValue(LDAP_USERNAME);
  if ( ldapUsername.isNull() )
    return;

  ldapUsername.toUpper();
  NAString schName = '"';
  schName += ldapUsername;
  schName += '"';
  // check schema name before insert
  // may get special characters from ldap
  ComSchemaName cSchName(schName);
  if ( !cSchName.getSchemaNamePart().isEmpty() &&
        cSchName.getCatalogNamePart().isEmpty()) // should have no catalog
  {
    insert(SCHEMA, schName);
  }
  else
  {		*CmpCommon::diags() << DgSqlCode(-2055)
			<< DgString0(schName)
			<< DgString1("SCHEMA");
  }
}
