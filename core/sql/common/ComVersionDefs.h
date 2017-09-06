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
#ifndef COMVERSIONDEFS_H
#define COMVERSIONDEFS_H

#include "Platform.h"

// Version numbers for GA releases must be a multiple of this number
#define GA_VERSION_MULTIPLE 20

// Change if we ever need to use 5 digit version numbers
#define DIGITS_IN_VERSION_NUMBER 4
// convert a version number to a 4 character string
#define VersionToString(v,s) {Int32 i=v,j=DIGITS_IN_VERSION_NUMBER; \
                              char * p = s;p[DIGITS_IN_VERSION_NUMBER]=0; \
                              while(j){p[--j]=(char)((i%10)+'0');i/=10;}}

//
// Version identifiers. 
//  

enum COM_VERSION { 
  // 
  // The versions we know about
    COM_VERS_UNKNOWN = 0
  , COM_VERS_R1_8    = 800                 // Version for the Rel 1.8 FCS software
  , COM_VERS_R2_EAP  = 1050                // Version for the Rel 2 EAP software
  , COM_VERS_R2_FCS  = 1200                // Version for the Rel 2 FCS software
  , COM_VERS_R2_1    = 1400                // Version for the Rel 2.1 (RoadRunner) software
  , COM_VERS_NEO_1   = 1420                // Version for the NEO and Highlander 1 releases
  , COM_VERS_R2_2    = 1600                // Version for the Rel 2.2 (Coyote) software
  , COM_VERS_R2_3    = 1800                // Version for the Rel 2.3 (Tiger) software
  , COM_VERS_SCH2000 = 2000                // Version for the Highlander R2 software
  , COM_VERS_NEO_2_2 = 2020                // Version for the Highlander R2.2 software
  , COM_VERS_2300    = 2300                // Version for the Highlander R2.3 software
  , COM_VERS_2400    = 2400
  , COM_VERS_2500    = 2500
  , COM_VERS_2600    = 2600
  //
  // Current version items, for stuff that gets created/generated: 
  //   Used for 
  //     1) Uprev checks. We generally don't support anything newer than what is current
  //     2) Setting the version of something we create
  //   Rules
  //     - no version can be greater than the MXV
  //     - Schema version must be >= system schema version
  //
  //   If you change a current version, or the MXV, remember to also change w:/regress/tools/setupenv !
  //
  , COM_VERS_CURR_SCHEMA         = COM_VERS_2600       // Schema version
  , COM_VERS_CURR_SYSTEM_SCHEMA  = COM_VERS_CURR_SCHEMA       // System schema version (obsolete as of R2.4)
  , COM_VERS_CURR_PLAN           = COM_VERS_2600       // Plan version
  , COM_VERS_COMPILER_VERSION    = COM_VERS_2600       // Compiler version

  //  Private definitions, not for general consumption
  //
  // The System Software Version - use to obtain build time software version only 
  //
  , COM_VERS_MXV                 = COM_VERS_2600       // System software version
  //
  // Oldest supported version items. Used for downrev checks.
  // Change as needed when <something> changes. 
  // Rules:
  //     - Oldest supported system schema version must be >= oldest supported schema version
  //     - Oldest supported module version must be >= oldest supported plan version
  //
  , COM_VERS_OLDEST_SUPPORTED_MXV                   = COM_VERS_2600      // System software version
  , COM_VERS_OLDEST_SUPPORTED_SCHEMA_VERSION        = COM_VERS_2400    // Schema version
  , COM_VERS_OLDEST_SUPPORTED_SYSTEM_SCHEMA_VERSION = COM_VERS_OLDEST_SUPPORTED_SCHEMA_VERSION    // System schema version  (obsolete as of R2.4)
  , COM_VERS_OLDEST_SUPPORTED_PLAN_VERSION          = COM_VERS_2600      // Plan version

  // Version numbers for testing - these need to change along with the MXV and oldest supported versions
  // and the versionVectorArray (see ComVersionPrivate.cpp) needs to be changed accordingly
  , COM_FAKE_UPREV_MXV                              = 3000               
  , COM_FAKE_OLDEST_SUPPORTED_MXV                   = 3000
  , COM_FAKE_OLDEST_SUPPORTED_SCHEMA_VERSION        = 2400      
  , COM_FAKE_OLDEST_SUPPORTED_SYSTEM_SCHEMA_VERSION = 2400
  , COM_FAKE_OLDEST_SUPPORTED_PLAN_VERSION          = 3000 
                 

};

inline
COM_VERSION  ComVersion_GetCurrentSchemaVersion (void) {return COM_VERS_CURR_SCHEMA;};
inline  
COM_VERSION  ComVersion_GetMXV (void) {return COM_VERS_MXV;};
inline
COM_VERSION  ComVersion_GetCurrentPlanVersion (void) {return COM_VERS_CURR_PLAN;};

//
// Version error numbers 
//
enum VersionErrorCode {
  // General stuff
  VERSION_NO_ERROR				 	= 0,
  VERSION_FIRST_ERROR					= 25000,
  VERSION_GENERAL_WARNING				= 25001,
  VERSION_PROGRAMMING_ERROR                             = 25002,
  VERSION_SUPPRESS_TX_ABORTED                           = 25003,
  VERSION_UPGRADE_IN_PROGRESS                           = 25004,
  VERSION_DOWNGRADE_IN_PROGRESS                         = 25005,
  VERSION_OFV_HIGHER_THAN_OSV                     	= 25006,

  // Software versioning error codes
  VERSION_REMOTE_MXV_OLDER_THAN_OLDEST_SUPPORTED	= 25100,
  VERSION_REMOTE_MXV_NOT_COMPATIBLE_WITH_NON_GA_NODE	= 25101,
  VERSION_REMOTE_MXV_NON_GA				= 25102,
  VERSION_COMPILER_VERSION_NOT_FOUND			= 25103,
  VERSION_ACCESSING_MXV_OLDER_THAN_OLDEST_SUPPORTED     = 25104,
  VERSION_REMOTE_MXV_OLDER_THAN_SUPPORTED_BY_FUNCTION   = 25105,


  // Database object versioning error codes
  VERSION_OSV_OLDER_THAN_OLDEST_SUPPORTED		= 25200,
  VERSION_OSV_HIGHER_THAN_MXV				= 25201,
  VERSION_OSV_OLDER_THAN_OLDEST_SUPPORTED_NODE		= 25202,
  VERSION_OSV_HIGHER_THAN_MXV_NODE			= 25203,
  VERSION_OSV_HIGHER_THAN_COMPILER_DML  		= 25204,
  VERSION_OSV_HIGHER_THAN_COMPILER_DDL			= 25205,
  VERSION_SYSSCH_HIGHER_THAN_COMPILER			= 25206,
  VERSION_SYSSCH_OLDER_THAN_OLDEST_SUPPORTED		= 25207,
  VERSION_SYSSCH_HIGHER_THAN_MXV			= 25208,
  VERSION_RELATED_SCHEMAS_MUST_MATCH			= 25209,
  VERSION_SCHEMA_OLDER_THAN_OLDEST_SUPPORTED		= 25210,
  VERSION_OBJECT_NON_GA_MXV_GA			 	= 25211,
  VERSION_OBJECT_DIFFERS_FROM_MXV_NON_GA		= 25212,
  VERSION_SYSSCH_NON_GA_MXV_GA				= 25213,
  VERSION_SYSSCH_DIFFERS_FROM_MXV_NON_GA		= 25214,
  VERSION_SCHEMA_NON_GA_MXV_GA				= 25215,
  VERSION_SCHEMA_DIFFERS_FROM_MXV_NON_GA		= 25216,
  VERSION_COMPILER_WAS_STARTED				= 25217,
  VERSION_SCHEMA_HIGHER_THAN_MXV			= 25218,
  VERSION_MIXED_VERSION_QUERY   			= 25219,

  // Upgrade/Downgrade related versioning error codes
  VERSION_CANNOT_DOWNGRADE_OFV_TOO_HIGH                 = 25250,
  VERSION_UPGRADE_DOWNGRADE_TARGET_VERSION_TOO_LOW      = 25251,
  VERSION_UPGRADE_DOWNGRADE_TARGET_VERSION_TOO_HIGH     = 25252,
  VERSION_UPGRADE_DOWNGRADE_RELATED_SCHEMAS_EXIST       = 25253,
  VERSION_UPGRADE_DOWNGRADE_RELATED_CATALOGS_EXIST      = 25254,
  VERSION_CANNOT_UPGRADE_TO_UNSUPPORTED_VERSION         = 25255,
  VERSION_CANNOT_DOWNGRADE_TO_UNSUPPORTED_VERSION       = 25256,
  VERSION_UPGRADE_DOWNGRADE_NO_SCHEMAS_AFFECTED         = 25257,
  VERSION_UPGRADE_DOWNGRADE_NOT_AUTHORIZED              = 25258,
  VERSION_INVALID_SCHEMA_VERSION                        = 25259,
  VERSION_UPGRADE_DOWNGRADE_NO_OPERATION_IN_PROGRESS    = 25260,
  VERSION_UPGRADE_DOWNGRADE_ORIGINAL_OPERATION_ACTIVE   = 25261,
  VERSION_UPGRADE_DOWNGRADE_METADATA_SCHEMA_NOT_ALLOWED = 25262,
  VERSION_UPGRADE_DOWNGRADE_USER_DEFINED_TRANSACTION    = 25263,
  VERSION_UPGRADE_DOWNGRADE_CANNOT_DROP_DEFSCH          = 25264,
  VERSION_UPGRADE_DOWNGRADE_INTERNAL_ERROR              = 25265,
  VERSION_UPGRADE_DOWNGRADE_TOM_WRONG_VERSION           = 25266,
  VERSION_CANNOT_DOWNGRADE_SCHEMA_LEVEL_PRIVS_GRANTED   = 25267,

  // Plan and module versioning error codes
  VERSION_MODULE_VERSION_OLDER_THAN_OLDEST_SUPPORTED	= 25300,
  VERSION_MODULE_VERSION_HIGHER_THAN_MXV		= 25301,
  VERSION_PLAN_VERSION_OLDER_THAN_OLDEST_SUPPORTED	= 25302,
  VERSION_PLAN_VERSION_HIGHER_THAN_MXV			= 25303,
  VERSION_COMPILER_USED_TO_COMPILE_QUERY		= 25304,
  VERSION_COMPILER_VERSION_LOWER_THAN_OLDEST_SUPPORTED  = 25305,
  VERSION_PLAN_VERSION_ERROR_NON_RETRYABLE_STATEMENT    = 25306,
  VERSION_PLAN_VERSION_NON_GA_MXV_GA			= 25307,
  VERSION_EXECUTING_GA_PLAN_ON_NON_GA_SYSTEM		= 25308,
  VERSION_MODULE_VERSION_NON_GA_MXV_GA			= 25309,
  VERSION_EXECUTING_GA_MODULE_ON_NON_GA_SYSTEM		= 25310,
  VERSION_SYSTEM_MODULE_INCORRECT_VERSION		= 25311,
  VERSION_DOWNREV_COMPILER_NEEDED_1			= 25312,
  VERSION_DOWNREV_COMPILER_NEEDED_2			= 25313,

  VERSION_LAST_ERROR					= 25399,

  // Duplicate defs for other error codes, to make the compiler happy
  VERSION_SYSSCH_IS_UNAVAILABLE                         = 25407,
  VERSION_NODE_IS_UNAVAILABLE                           = 25420,
  VERSION_SQLMX_IS_NOT_INSTALLED                        = 1037
 };

// The number of supported compiler versions.  When this number changes you
// must modify ComVersion_getSupportedVersionsArray in ComVersionPublic.cpp.
#define COM_VERS_NUM_SUPPORTED_MXV 4

//
// Versioning for SQL/MX audit records. DP2 puts an 8-bit SQL/MX
// version number into every DML audit record for an MX table. Make
// sure the values in this enum are all 8-bit quantities.
// 
// These values are currently used in 2 places:
// 
// 1. In DP2 when an MX table is opened, DP2 calls
//    EXECUTOR_DP2_UNPACK_RCB and we we return information to DP2 about
//    the current, highest, and lowest supported MX audit versions for
//    that particular file.
//
// 2. In the CLI, our MXARLIB routines that map DML audit into
//    presentation format contain version checks to make sure the DML
//    audit record is in a format supported by the current software.
//
//  COM_AUDIT_VERS_SCH200 added for the new aligned row format.
//  Must be able to detect a version mismatch at open time when using RDF.
enum COM_AUDIT_VERSION
{
  COM_AUDIT_VERS_UNKNOWN            = 0,
  COM_AUDIT_VERS_R2_FCS             = 10,
  COM_AUDIT_VERS_SCH2000            = 12,  // For new aligned row format
  COM_AUDIT_VERS_CURR               = COM_AUDIT_VERS_SCH2000,
  COM_AUDIT_VERS_OLDEST_SUPPORTED   = COM_AUDIT_VERS_R2_FCS,
  COM_AUDIT_VERS_HIGHEST_SUPPORTED  = COM_AUDIT_VERS_SCH2000
};


#endif
