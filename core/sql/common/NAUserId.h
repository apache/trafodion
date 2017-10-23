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
#ifndef NAUSERID_H
#define NAUSERID_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         NAUserId.h
 * Description:  Definition of NAUserId
 * Created:      8/19/2002
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */
#include "Platform.h"

// ----------------------------------------------------------------------------
// standard defines:
#define MAX_DBUSERNAME_LEN 128
#define MAX_USERNAME_LEN 128
#define MAX_AUTHNAME_LEN 128
#define MAX_AUTHID_AS_STRING_LEN 20
#define NA_UserIdDefault 0
// ----------------------------------------------------------------------------
// Authorization range definitions:
// Authorization ID's include users, roles, and tenants (maybe groups later)
#define MIN_USERID          33333 /* reserve 1 to 33333 for system users */
#define MAX_USERID         799999

#define MIN_ROLEID        1000000
#define MAX_ROLEID_RANGE1 1490000
#define MAX_ROLEID        1499999

// ----------------------------------------------------------------------------
// For roles and other non-user authIDS, use the following structure to create 
// new system objects
struct SystemAuthsStruct
{
   const char *authName;
   bool       isSpecialAuth;
   int32_t    authID;
};

// ----------------------------------------------------------------------------
// Definitions for system users:
// For new system tenants, generate the username and the userID; change
// CmpSeabaseDDL::updateSeabaseAuths to register the new (standard) user
#define DB__ROOT         "DB__ROOT"
#define SUPER_USER_LIT   "33333"

// If a new system defined user is added, subtract one from MIN_SYSTEM_ID and
// be sure to change MIN_USERID to the smaller value
#define SYSTEM_USER  -2
#define PUBLIC_USER  -1
#define MIN_SYSTEM_ID 33330
#define ROOT_USER_ID  33333
#define SUPER_USER    33333  

// -----------------------------------------------------------------------------
// Definitions for system roles:
// For new system roles, add a define and include it in the systemRoles constant
// When authorization is enabled, these roles are created, no additional changes
// to the code is required.
#define SYSTEM_AUTH_NAME "_SYSTEM"
#define PUBLIC_AUTH_NAME "PUBLIC"
#define DB__HIVEROLE     "DB__HIVEROLE"
#define DB__HBASEROLE    "DB__HBASEROLE"
#define DB__ROOTROLE     "DB__ROOTROLE"
#define DB__LIBMGRROLE   "DB__LIBMGRROLE"

// Most system roles do not have a predefined range of IDs, so for new roles
// just specify NA_UserIdDefault in the systemRoles struct. Role code creates
// roles for each non special system role in the list.  If NA_UserIdDefault is
// specified, the code generates a UniqueID.
#define ROOT_ROLE_ID     1000000
#define HIVE_ROLE_ID     1490000 
#define HBASE_ROLE_ID    1490001

static const SystemAuthsStruct systemRoles[] 
{ { DB__HIVEROLE, false, HIVE_ROLE_ID },
  { DB__HBASEROLE, false, HBASE_ROLE_ID },
  { DB__ROOTROLE, false, ROOT_ROLE_ID },
  { DB__LIBMGRROLE, false, NA_UserIdDefault },
  { PUBLIC_AUTH_NAME, true, PUBLIC_USER },
  { SYSTEM_AUTH_NAME, true, SYSTEM_USER } };

#define NUMBER_SPECIAL_SYSTEM_ROLES 2;

#endif  /*  NAUSERID_H*/
