//*****************************************************************************
// @@@ START COPYRIGHT @@@
//
//// (C) Copyright 2013-2015 Hewlett-Packard Development Company, L.P.
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

// List of tables that make up the privilege manager component
#define PRIVMGR_OBJECT_PRIVILEGES "OBJECT_PRIVILEGES"
#define PRIVMGR_COLUMN_PRIVILEGES "COLUMN_PRIVILEGES"
#define PRIVMGR_COMPONENTS "COMPONENTS"
#define PRIVMGR_COMPONENT_OPERATIONS "COMPONENT_OPERATIONS"
#define PRIVMGR_COMPONENT_PRIVILEGES "COMPONENT_PRIVILEGES"
#define PRIVMGR_ROLE_USAGE "ROLE_USAGE"
#define PRIVMGR_SCHEMA_PRIVILEGES "SCHEMA_PRIVILEGES"

enum PrivMgrTableEnum { OBJECT_PRIVILEGES_ENUM = 30,
                            COLUMN_PRIVILEGES_ENUM = 31,
                            SCHEMA_PRIVIELGES_ENUM = 32,
                            COMPONENTS_ENUM        = 33,
                            COMPONENT_OPERATIONS_ENUM  = 34,
                            COMPONENT_PRIVILEGES_ENUM  = 35,
                            ROLE_USAGES_ENUM           = 36,
                            OBJECTS_ENUM               = 37,
                            UNKNOWN_ENUM               = 38
                          };

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

// Trafodion creates HBase tables that concatenate the catalog, schema, and
// object name together.  The max HBase name can only be 255.  As long as
// we create Trafodion objects in HBase the same way, the object_name variables
// stored in the PrivMgr tables cannot exceed 255.  If we decide to change
// our naming convention, this size could change. 
  
// the following TableDDLStrings describe each metadata tables
static const TableDDLString columnPrivilegesDDL[] =
{" ( \
   object_uid largeint not null, \
   object_name varchar(600 bytes) character set utf8 not null, \
   grantee_id int not null, \
   grantee_name varchar(256 bytes) character set utf8 not null, \
   grantor_id int not null, \
   grantor_name varchar(256 bytes) character set utf8 not null, \
   column_number int not null, \
   privileges_bitmap largeint not null, \
   grantable_bitmap largeint not null, \
   primary key (object_uid, grantee_id, grantor_id, column_number) \
   );" };

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
  
static const TableDDLString schemaPrivilegesDDL[] = 
{ " ( \
  schema_uid largeint not null, \
  schema_name varchar(600 bytes) character set utf8 not null, \
  grantee_id int not null, \
  grantee_name varchar(256 bytes) character set utf8 not null, \
  grantor_id int not null, \
  grantor_name varchar(256 bytes) character set utf8 not null, \
  privileges_bitmap largeint not null, \
  grantable_bitmap largeint not null, \
  primary key (schema_uid, grantor_id, grantee_id) \
  );" };

// The PrivMgrTableStruct describes each table
static const PrivMgrTableStruct privMgrTables[] =
  { { PRIVMGR_OBJECT_PRIVILEGES, objectPrivilegesDDL, false }, 
    { PRIVMGR_COLUMN_PRIVILEGES, columnPrivilegesDDL, false },
    { PRIVMGR_COMPONENTS, componentsDDL, false },
    { PRIVMGR_COMPONENT_OPERATIONS, componentOperationsDDL, false },
    { PRIVMGR_COMPONENT_PRIVILEGES, componentPrivilegesDDL, false },
    { PRIVMGR_ROLE_USAGE, roleUsageDDL, false }
   ,{ PRIVMGR_SCHEMA_PRIVILEGES, schemaPrivilegesDDL, false } 
  };


#endif // PRIVMGR_MD_DEFS_H









