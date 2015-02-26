//*****************************************************************************
// @@@ START COPYRIGHT @@@
//
//// (C) Copyright 2013-2014 Hewlett-Packard Development Company, L.P.
////
////  Licensed under the Apache License, Version 2.0 (the "License");
////  you may not use this file except in compliance with the License.
////  You may obtain a copy of the License at
////
////      http://www.apache.org/licenses/LICENSE-2.0
////
////  Unless required by applicable law or agreed to in writing, software
////  distributed under the License is distributed on an "AS IS" BASIS,
////  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
////  See the License for the specific language governing permissions and
////  limitations under the License.
////
//// @@@ END COPYRIGHT @@@
//*****************************************************************************

#ifndef PRIVMGR_MD_DEFS_H
#define PRIVMGR_MD_DEFS_H


// *****************************************************************************
// *
// * File:         PrivMgrMDDef.h
// * Description:  This file contains definitions of all the objects managed
// *               by the privilege manager component
// *
// *****************************************************************************

// The TableDDLString is used to contain the CREATE text for privilege manager
// metadata tables
struct TableDDLString {
public:
  const char * str;
};

// The PrivMgrTableStruct is used to describe a privilege manager metadata 
// table
struct PrivMgrTableStruct
{
  const char * tableName;
  const TableDDLString * tableDDL;
  const bool isIndex;
};

// the following TableDDLStrings describe each metadata tables
static const TableDDLString componentsDDL[] =
{" ( \
   component_uid largeint not null primary key, \
   component_name varchar(128 bytes) character set ISO88591 not null, \
   is_system char(2) not null, \
   component_description varchar(80 bytes) character set ISO88591 default null \
   );" };

static const TableDDLString componentOperationsDDL[] =
{ " ( \
  component_uid largeint not null , \
  operation_code char(2 bytes) character set ISO88591 not null, \
  operation_name varchar(256 bytes) character set ISO88591 not null, \
  is_system char(2) not null, \
  operation_description char(80 bytes) character set ISO88591 default null, \
  primary key (component_uid, operation_code) \
  );" };

static const TableDDLString componentPrivilegesDDL[] = 
{ " ( \
  grantee_id int not null, \
  grantor_id int not null, \
  component_uid largeint not null, \
  operation_code char(2 bytes) character set ISO88591 not null, \
  grantee_name varchar(256 bytes) character set utf8 not null, \
  grantor_name varchar(256 bytes) character set utf8 not null, \
  grant_depth int not null, \
  primary key (grantee_id, grantor_id, component_uid, operation_code) \
  );" };

static const TableDDLString objectPrivilegesDDL[] = 
{ " ( \
  object_uid largeint not null, \
  object_name varchar(600 bytes) character set utf8 not null, \
  object_type char (2 bytes)not null, \
  grantee_id largeint not null, \
  grantee_name varchar(256 bytes) character set utf8 not null, \
  grantee_type char (2 bytes) not null, \
  grantor_id largeint not null, \
  grantor_name varchar(256 bytes) character set utf8 not null, \
  grantor_type char (2 bytes) not null, \
  privileges_bitmap largeint not null, \
  grantable_bitmap largeint not null, \
  primary key (object_uid, grantor_id, grantee_id) \
  );" };

static const TableDDLString roleUsageDDL[] = 
{ " ( \
  role_id int not null, \
  role_name varchar(256 bytes) character set utf8 not null, \
  grantee_id int not null, \
  grantee_name varchar(256 bytes) character set utf8 not null, \
  grantee_auth_class char (2 bytes) character set utf8 not null, \
  grantor_id int not null, \
  grantor_name varchar(256 bytes) character set utf8 not null, \
  grantor_auth_class char (2 bytes) character set utf8 not null, \
  grant_depth int not null, \
  primary key (role_id, grantor_id, grantee_id) \
  );" };

// The PrivMgrTableStruct describes each table
static const PrivMgrTableStruct privMgrTables[] =
  { { "OBJECT_PRIVILEGES", objectPrivilegesDDL, false }, 
    { "COMPONENTS", componentsDDL, false },
    { "COMPONENT_OPERATIONS", componentOperationsDDL, false },
    { "COMPONENT_PRIVILEGES", componentPrivilegesDDL, false },
    { "ROLE_USAGE", roleUsageDDL, false }
  };


#endif // PRIVMGR_MD_DEFS_H









