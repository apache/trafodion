/*
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
*/

#ifndef __HPSQLEXTH
#define __HPSQLEXTH

// Attribute to turn non atomic rowset behaviour OFF or ON
// default is non atomic rowsets (SQL_ATTR_ROWSET_RECOVERY_ON)
// for Atomic rowsets set this attribute to SQL_ATTR_ROWSET_RECOVERY_OFF
#define SQL_ATTR_ROWSET_RECOVERY        2000
#define SQL_ATTR_ROWSET_RECOVERY_OFF    0
#define SQL_ATTR_ROWSET_RECOVERY_ON     1

// HP Session Name connection attribute
#define SQL_ATTR_SESSIONNAME		5000
// Attribute to get the 64bit rowcount when using the 32-bit ODBC driver
#define SQL_ATTR_ROWCOUNT64_PTR		5001
// HP rolename attribute
#define SQL_ATTR_ROLENAME 			5002
// Attribute to set fetchahead connection attribute
#define SQL_ATTR_FETCHAHEAD			5003

/* Max Session Name length including terminating null */
#define SQL_MAX_SESSIONNAME_LEN		25
/* Max rolename Name length including terminating null */
#define SQL_MAX_ROLENAME_LEN		128

//wms_mapping 
#define SQL_ATTR_APPLNAME			5100
#define SQL_MAX_APPLNAME_LEN		128

// HP Security
#define SQL_ATTR_CERTIFICATE_DIR			5200
#define SQL_ATTR_CERTIFICATE_FILE			5201
#define SQL_ATTR_CERTIFICATE_FILE_ACTIVE	5202
#ifdef HOLDABLE
#define SQL_ATTR_CURSOR_HOLDABLE	-3  // 
#define SQL_NONHOLDABLE			0
#define SQL_HOLDABLE			1
#endif

#endif /* __HPSQLEXTH */
