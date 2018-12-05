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

/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ExExeUtilGet.cpp
 * Description:
 *
 *
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

#include "ComCextdecs.h"
#include  "cli_stdh.h"
#include  "ex_stdh.h"
#include  "sql_id.h"
#include  "ex_transaction.h"
#include  "ComTdb.h"
#include  "ex_tcb.h"
#include  "ComSqlId.h"
#include  "ComMisc.h"
#include  "ComUser.h"

#include  "ExExeUtil.h"
#include  "ex_exe_stmt_globals.h"
#include  "exp_expr.h"
#include  "exp_clause_derived.h"
#include  "ExpLOB.h"
#include  "ComRtUtils.h"
#include  "CmpCommon.h"
#include  "CmpContext.h"

#include  "sqlcmd.h"
#include  "SqlciEnv.h"

#include  "GetErrorMessage.h"
#include  "ErrorMessage.h"
#include  "HBaseClient_JNI.h"

#include "CmpDDLCatErrorCodes.h"
#include "PrivMgrCommands.h"
#include "PrivMgrComponentPrivileges.h"

#include "ExpHbaseInterface.h"
#include "sql_buffer_size.h"

#include "NAType.h"
#include "HiveClient_JNI.h"

//******************************************************************************
//                                                                             *
//  These definitions were stolen from CatWellKnownTables.h
//
// Size of CHAR(128) CHARACTER SET UCS2 NOT NULL column is 256 bytes.
#define EX_MD_XXX_NAME_CHAR_LEN "128"
//
// Character type columns in metadata tables are generally case-sensitive
#define ISO_CHAR_ATTR    " CHARACTER SET ISO88591 CASESPECIFIC "
#define UCS2_CHAR_ATTR   " CHARACTER SET UCS2 CASESPECIFIC "
//
// Add explicit collate default to avoid inherit it from table or schema
#define ISO_CHAR_ATTR_2    " CHARACTER SET ISO88591 COLLATE DEFAULT CASESPECIFIC "
#define UCS2_CHAR_ATTR_2   " CHARACTER SET UCS2 COLLATE DEFAULT CASESPECIFIC "
//
// Most - if not all - columns are NNND
#define NNND_ATTR  " NOT NULL NOT DROPPABLE "
//                                                                             *
//******************************************************************************

///////////////////////////////////////////////////////////////////
ex_tcb * ExExeUtilGetMetadataInfoTdb::build(ex_globals * glob)
{
  ExExeUtilGetMetadataInfoTcb * exe_util_tcb;

  if ((groupBy() || orderBy()) ||
      (queryType() == ComTdbExeUtilGetMetadataInfo::OBJECTS_ON_TABLE_) ||
      (queryType() == ComTdbExeUtilGetMetadataInfo::OBJECTS_IN_SCHEMA_))
    exe_util_tcb =
      new(glob->getSpace()) ExExeUtilGetMetadataInfoComplexTcb(*this, glob);
  //else if (getVersion())
  //  exe_util_tcb =
  //    new(glob->getSpace()) ExExeUtilGetMetadataInfoVersionTcb(*this, glob);
  else if (queryType() == ComTdbExeUtilGetMetadataInfo::HBASE_OBJECTS_)
    exe_util_tcb =
      new(glob->getSpace()) ExExeUtilGetHbaseObjectsTcb(*this, glob);
  else
    exe_util_tcb =
      new(glob->getSpace()) ExExeUtilGetMetadataInfoTcb(*this, glob);

  exe_util_tcb->registerSubtasks();

  return (exe_util_tcb);
}

////////////////////////////////////////////////////////////////
// Constructor for class ExExeUtilGetMetadataInfoTcb
///////////////////////////////////////////////////////////////
ExExeUtilGetMetadataInfoTcb::ExExeUtilGetMetadataInfoTcb(
     const ComTdbExeUtilGetMetadataInfo & exe_util_tdb,
     ex_globals * glob)
     : ExExeUtilTcb( exe_util_tdb, NULL, glob)
{
  // Allocate the private state in each entry of the down queue
  qparent_.down->allocatePstate(this);

  step_ = INITIAL_;
  vStep_ = VIEWS_INITIAL_;

  // allocate space to hold the metadata query that will be used to retrieve
  // metadata info. 10K is big enough for it.
  queryBuf_ = new(glob->getDefaultHeap()) char[10000];

  // buffer where output will be formatted
  outputBuf_ = new(glob->getDefaultHeap()) char[4096];

  headingBuf_ = new(glob->getDefaultHeap()) char[1000];

  for (Int32 i = 0; i < NUM_MAX_PARAMS_; i++)
    {
      param_[i] = NULL;
    }

  patternStr_ = new(glob->getDefaultHeap()) char[1000];

  numOutputEntries_ = 0;
  returnRowCount_ = 0;
}

ExExeUtilGetMetadataInfoTcb::~ExExeUtilGetMetadataInfoTcb()
{
  NADELETEBASIC(queryBuf_, getGlobals()->getDefaultHeap());
  NADELETEBASIC(outputBuf_, getGlobals()->getDefaultHeap());
  NADELETEBASIC(headingBuf_, getGlobals()->getDefaultHeap());
  NADELETEBASIC(patternStr_, getGlobals()->getDefaultHeap());
}


static const QueryString getUsersForRoleQuery[] =
{
  {" select translate(rtrim(RU.grantee_name) using ucs2toutf8) "},
  {"   from %s.\"%s\".%s RU "},
  {" where (RU.grantor_ID != -2) and "},
  {"       (RU.role_name = '%s') %s "},
  {" order by 1"},
  {" ; "}
};


static const QueryString getRolesForUserQuery[] =
{
  {" select translate(rtrim(RU.role_name) using ucs2toutf8) "},
  {"   from %s.\"%s\".%s RU "},
  {" where (RU.grantor_ID != -2) and "},
  {"       (RU.grantee_name='%s') "},
  {" union select * from (values ('PUBLIC')) "},
  {" order by 1 "},
  {" ; "}
};

static const QueryString getPrivsForAuthsQuery[] =
{
  {" select translate(rtrim(object_name) using ucs2toutf8), "},
  {"    case when bitextract(privileges_bitmap,63,1) = 1 then 'S' "},
  {"      else '-' end || "},
  {"     case when bitextract(privileges_bitmap,62,1) = 1 then 'I' "},
  {"      else '-' end || "},
  {"     case when bitextract(privileges_bitmap,61,1) = 1 then 'D' "},
  {"      else '-' end || "},
  {"     case when bitextract(privileges_bitmap,60,1) = 1 then 'U' "},
  {"      else '-' end || "},
  {"     case when bitextract(privileges_bitmap,59,1) = 1 then 'G' "},
  {"      else '-' end || "},
  {"     case when bitextract(privileges_bitmap,58,1) = 1 then 'R' "},
  {"      else '-' end || "},
  {"     case when bitextract(privileges_bitmap,57,1) = 1 then 'E' "},
  {"      else '-' end as privs "},
  {" from %s.\"%s\".%s "},
  {" where grantee_id %s "},
  {" union "},
  {"  (select translate(rtrim(schema_name) using ucs2toutf8), "},
  {"    case when bitextract(privileges_bitmap,63,1) = 1 then 'S' "},
  {"      else '-' end || "},
  {"     case when bitextract(privileges_bitmap,62,1) = 1 then 'I' "},
  {"      else '-' end || "},
  {"     case when bitextract(privileges_bitmap,61,1) = 1 then 'D' "},
  {"      else '-' end || "},
  {"     case when bitextract(privileges_bitmap,60,1) = 1 then 'U' "},
  {"      else '-' end || "},
  {"     case when bitextract(privileges_bitmap,59,1) = 1 then 'G' "},
  {"      else '-' end || "},
  {"     case when bitextract(privileges_bitmap,58,1) = 1 then 'R' "},
  {"      else '-' end || "},
  {"     case when bitextract(privileges_bitmap,57,1) = 1 then 'E' "},
  {"      else '-' end as privs "},
  {"   from %s.\"%s\".%s "},
  {"   where grantee_id %s ) "},
  {" %s order by 1 " },
  {" ; "}
};

static const QueryString getPrivsForColsQuery[] =
{
  {" union "}, // for column privileges
  {"  (select translate(rtrim(object_name) using ucs2toutf8) || ' <Column> ' || "},
  {"          translate(rtrim(column_name) using ucs2toutf8), "},
  {"    case when bitextract(privileges_bitmap,63,1) = 1 then 'S' "},
  {"      else '-' end || "},
  {"     case when bitextract(privileges_bitmap,62,1) = 1 then 'I' "},
  {"      else '-' end || "},
  {"     case when bitextract(privileges_bitmap,61,1) = 1 then 'D' "},
  {"      else '-' end || "},
  {"     case when bitextract(privileges_bitmap,60,1) = 1 then 'U' "},
  {"      else '-' end || "},
  {"     case when bitextract(privileges_bitmap,59,1) = 1 then 'G' "},
  {"      else '-' end || "},
  {"     case when bitextract(privileges_bitmap,58,1) = 1 then 'R' "},
  {"      else '-' end || "},
  {"     case when bitextract(privileges_bitmap,57,1) = 1 then 'E' "},
  {"      else '-' end as privs "},
  {"   from %s.\"%s\".%s p, %s.\"%s\".%s c "},
  {"   where p.object_uid = c.object_uid "},
  {"     and p.column_number = c.column_number "},
  {"     and grantee_id %s ) "},
};

static const QueryString getPrivsForHiveColsQuery[] =
{
  {" union "}, // for privileges on hive objects
  {"  (select translate(rtrim(o.catalog_name) using ucs2toutf8) || '.' || "},
  {"          translate(rtrim(o.schema_name) using ucs2toutf8) || '.'  || "},
  {"          translate(rtrim(o.object_name) using ucs2toutf8) || '.'  || "},
  {"                        ' <Column> ' ||                    "},
  {"          translate(rtrim(column_name) using ucs2toutf8), "},
  {"    case when bitextract(privileges_bitmap,63,1) = 1 then 'S' "},
  {"      else '-' end || "},
  {"     case when bitextract(privileges_bitmap,62,1) = 1 then 'I' "},
  {"      else '-' end || "},
  {"     case when bitextract(privileges_bitmap,61,1) = 1 then 'D' "},
  {"      else '-' end || "},
  {"     case when bitextract(privileges_bitmap,60,1) = 1 then 'U' "},
  {"      else '-' end || "},
  {"     case when bitextract(privileges_bitmap,59,1) = 1 then 'G' "},
  {"      else '-' end || "},
  {"     case when bitextract(privileges_bitmap,58,1) = 1 then 'R' "},
  {"      else '-' end || "},
  {"     case when bitextract(privileges_bitmap,57,1) = 1 then 'E' "},
  {"      else '-' end as privs "},
  {"   from %s.\"%s\".%s p, %s.\"%s\".%s o,                        "},
  {"        table(hivemd(columns)) c                               "},
  {"   where p.object_uid = o.object_uid "},
  {"         and o.catalog_name = upper(c.catalog_name) "},
  {"         and o.schema_name = upper(c.schema_name) "},
  {"         and o.object_name = upper(c.table_name) "},
  {"     and p.column_number = c.column_number "},
  {"     and grantee_id %s ) "},
};

static const QueryString getComponents[] =
{
  {" select distinct translate(rtrim(component_name) using ucs2toutf8)  "},
  {"   from %s.\"%s\".%s c, %s.\"%s\".%s p "},
  {"   where c.component_uid = p.component_uid %s "},
  {" order by 1 "},
  {" ; "}
};

static const QueryString getComponentPrivileges[] = 
{
  {" select distinct translate(rtrim(operation_name) using ucs2toutf8) "},
  {" from %s.\"%s\".%s c, %s.\"%s\".%s o, "},
  {"      %s.\"%s\".%s p "},
  {" where (c.component_uid=o.component_uid) "},
  {"   and (o.component_uid=p.component_uid) "},
  {"   and (o.operation_code=p.operation_code) "},
  {"   and (o.is_system <> 'U') "},
  {"   and (c.component_name='%s') %s "},
  {" order by 1 "},
  {" ; "}
};

static const QueryString getCatalogsQuery[] =
{
  {" select * from (values ('TRAFODION'), ('HIVE')) "},
  {" order by 1 desc "},
  {" ; "}
};

static const QueryString getTrafTablesInSchemaQuery[] =
{
  {" select %sobject_name%s  from "},
  {"   %s.\"%s\".%s "},
  {"  where catalog_name = '%s' and "},
  {"        schema_name = '%s'  and "},
  {"        object_type = 'BT' %s "},
  {"  order by 1 "},
  {"  ; "}
};

static const QueryString getTrafIndexesInSchemaQuery[] =
{
  {" select object_name  from "},
  {"   %s.\"%s\".%s "},
  {"  where catalog_name = '%s' and "},
  {"        schema_name = '%s'  and "},
  {"        object_type = 'IX' %s "},
  {"  order by 1 "},
  {"  ; "}
};

static const QueryString getTrafIndexesOnTableQuery[] =
{
  {" select %sO2.object_name%s from "},
  {"   %s.\"%s\".%s I, "},
  {"   %s.\"%s\".%s O, "},
  {"   %s.\"%s\".%s O2 "},
  {"  where O.catalog_name = '%s' "},
  {"    and O.schema_name = '%s' "},
  {"    and O.object_name = '%s' "},
  {"    and I.base_table_uid = O.object_uid "},
  {"    and I.index_uid = O2.object_uid %s "},
  {" order by 1 "},
  {" ; "}
};

static const QueryString getTrafIndexesForAuth[] =
{
  {" select trim(T2.catalog_name) || '.\"' || trim(T2.schema_name) || '\".' || trim(T2.object_name) "},
  {" from %s.\"%s\".%s I, "},
  {"      %s.\"%s\".%s T, "},
  {"      %s.\"%s\".%s T2 "},
  {"  where T.catalog_name = '%s' "},
  {"    and I.base_table_uid = T.object_uid "},
  {"    and I.index_uid = T2.object_uid %s "},
  {" order by 1 "},
  {" ; "}
};

static const QueryString getTrafProceduresInSchemaQuery[] =
{
  {" select object_name  from "},
  {"   %s.\"%s\".%s T, %s.\"%s\".%s R "},
  {"  where T.catalog_name = '%s' and "},
  {"        T.schema_name = '%s'  and "},
  {"        T.object_type = 'UR'  and "},
  {"        T.object_uid = R.udr_uid  and "},
  {"        R.udr_type = 'P ' %s "},
  {"  order by 1 "},
  {"  ; "}
};

static const QueryString getTrafLibrariesInSchemaQuery[] =
{
  {" select distinct object_name  from "},
  {"   %s.\"%s\".%s T, %s.\"%s\".%s R  "},
  {"  where T.catalog_name = '%s' and "},
  {"        T.schema_name = '%s'  and "},
  {"        T.object_type = 'LB' %s "},
  {"  order by 1 "},
  {"  ; "}
};

static const QueryString getTrafLibrariesForAuthQuery[] =
{
  {" select distinct trim(catalog_name) || '.\"' || "},
  {"     trim(schema_name) || '\".' || trim(object_name) "},
  {"  from %s.\"%s\".%s "},
  {"  where catalog_name = '%s' and object_type = 'LB' %s "},
  {"  order by 1 "},
  {"  ; "}
};

static const QueryString getTrafRoutinesForAuthQuery[] =
{
  {" select distinct trim(catalog_name) || '.\"' || "},
  {"     trim(schema_name) || '\".' || trim(object_name) "},
  {"  from %s.\"%s\".%s T, %s.\"%s\".%s R  "},
  {"  where T.catalog_name = '%s' and "},
  {"        T.object_type = 'UR' and "},
  {"        T.object_uid = R.udr_uid and "},
  {"        R.udr_type = '%s' %s "},
  {"  order by 1 "},
  {"  ; "}
};

static const QueryString getTrafFunctionsInSchemaQuery[] =
{
  {" select object_name  from "},
  {"   %s.\"%s\".%s T, %s.\"%s\".%s R "},
  {"  where T.catalog_name = '%s' and "},
  {"        T.schema_name = '%s'  and "},
  {"        T.object_type = 'UR'  and "},
  {"        T.object_uid = R.udr_uid  and "},
  {"        R.udr_type = 'F ' %s "},
  {"  order by 1 "},
  {"  ; "}
};

static const QueryString getTrafTableFunctionsInSchemaQuery[] =
{
  {" select object_name  from "},
  {"   %s.\"%s\".%s T, %s.\"%s\".%s R "},
  {"  where T.catalog_name = '%s' and "},
  {"        T.schema_name = '%s'  and "},
  {"        T.object_type = 'UR'  and "},
  {"        T.object_uid = R.udr_uid  and "},
  {"        R.udr_type = 'T ' %s "},
  {"  order by 1 "},
  {"  ; "}
};

static const QueryString getTrafProceduresForLibraryQuery[] =
{
  {" select T1.schema_name || '.' || T1.object_name  from "},
  {"   %s.\"%s\".%s T, %s.\"%s\".%s R, %s.\"%s\".%s T1 "},
  {"where T.catalog_name = '%s' and T.schema_name = '%s' "},
  {"  and T.object_name = '%s' and T.object_type = 'LB' "},
  {"  and T.object_uid = R.library_uid and R.udr_uid = T1.object_uid "},
  {"  and %s %s "},
  {"order by 1 "},
  {"  ; "}
};

static const QueryString getTrafSequencesInSchemaQuery[] =
{
  {" select object_name  from "},
  {"   %s.\"%s\".%s "},
  {"  where catalog_name = '%s' and "},
  {"        schema_name = '%s'  and "},
  {"        object_type = 'SG' %s "},
  {"  order by 1 "},
  {"  ; "}
};

static const QueryString getTrafSequencesInCatalogQuery[] =
{
  {" select trim(schema_name) || '.' || object_name  from "},
  {"   %s.\"%s\".%s "},
  {"  where catalog_name = '%s' and "},
  {"        object_type = 'SG' %s "},
  {"  order by 1 "},
  {"  ; "}
};

static const QueryString getTrafViewsInCatalogQuery[] =
{
  {" select schema_name || '.' || "},
  {" object_name from "},
  {"   %s.\"%s\".%s,  %s.\"%s\".%s "},
  {"  where view_uid = object_uid and "},
  {"            catalog_name = '%s' %s "},
  {" order by 1 "},
  {"  ; "}
};

static const QueryString getTrafViewsInSchemaQuery[] =
{
  {" select object_name from "},
  {"   %s.\"%s\".%s,  %s.\"%s\".%s "},
  {"  where view_uid = object_uid and "},
  {"             catalog_name = '%s' and "},
  {"             schema_name = '%s' %s "},
  {" order by 1 "},
  {"  ; "}
};

static const QueryString getTrafObjectsInViewQuery[] =
{
  {" select trim(T.catalog_name) || '.' || trim(T.schema_name) || '.' || trim(T.object_name), "},
  {" trim(T.object_type) "},
  {"   from %s.\"%s\".%s VU,  %s.\"%s\".%s T "},
  {"  where VU.using_view_uid = "},
  {"     (select T2.object_uid from  %s.\"%s\".%s T2 "},
  {"         where T2.catalog_name = '%s' and "},
  {"                   T2.schema_name = '%s' and "},
  {"                   T2.object_name = '%s' %s ) "},
  {"     and VU.used_object_uid = T.object_uid "},
  {" order by 1 "},
  {"  ; "}
};

static const QueryString getTrafViewsOnObjectQuery[] =
{
  {" select trim(T.catalog_name) || '.' || trim(T.schema_name) || '.' || trim(T.object_name) from "},
  {"   %s.\"%s\".%s T "},
  {"   where T.object_uid in "},
  {"   (select using_view_uid from  %s.\"%s\".%s VU "},
  {"    where used_object_uid in  "},
  {"     (select object_uid from  "},
  {"        %s.\"%s\".%s T1 "},
  {"      where T1.catalog_name = '%s' "},
  {"          and T1.schema_name = '%s' "},
  {"          and T1.object_name = '%s' "},
  {"          %s %s "},
  {"     ) "},
  {"   ) "},
  {" order by 1 "},
  {" ; "}
};

static const QueryString getTrafSchemasInCatalogQuery[] =
{
  {" select distinct schema_name "},
  {"   from %s.\"%s\".%s "},
  {"  where catalog_name = '%s' %s "},
  {" order by 1 "},
  {"  ; "}
};

static const QueryString getTrafSchemasForAuthIDQuery[] =
{
  {" select T.schema_name "},
  {"   from %s.\"%s\".%s T, "},
  {"        %s.\"%s\".%s A "},
  {"  where T.object_type in ('PS', 'SS') and "},
  {"         T.object_owner = A.auth_id and "},
  {"         A.auth_id in %s "},
  {" order by 1 "},
  {"  ; "}
};

static const QueryString getTrafUsers[] = 
{
  {" select distinct auth_db_name "},
  {"   from %s.\"%s\".%s "},
  {"  where auth_type = 'U' %s "},
  {" order by 1 "},
  {"  ; "}
};

static const QueryString getTrafRoles[] = 
{
  {" select distinct auth_db_name "},
  {"   from %s.\"%s\".%s "},
  {"  where auth_type = 'R' %s "},
  {" union select * from (values ('PUBLIC')) "},
  {" order by 1 "},
  {"  ; "}
};

static const QueryString getTrafPrivsOnObject[] = 
{
  {" select grantee_name, "},
  {"   case when bitextract(privileges_bitmap,63,1) = 1 then 'S' else '-' end || "},
  {"   case when bitextract(privileges_bitmap,62,1) = 1 then 'I' else '-' end || "},
  {"   case when bitextract(privileges_bitmap,61,1) = 1 then 'D' else '-' end || "},
  {"   case when bitextract(privileges_bitmap,60,1) = 1 then 'U' else '-' end || "},
  {"   case when bitextract(privileges_bitmap,59,1) = 1 then 'G' else '-' end || "},
  {"   case when bitextract(privileges_bitmap,58,1) = 1 then 'R' else '-' end || "},
  {"   case when bitextract(privileges_bitmap,57,1) = 1 then 'E' else '-' end as privs "},
  {" from %s.\"%s\".%s "},
  {" where object_uid = "},
  {"  (select object_uid from %s.\"%s\".%s "},
  {"   where catalog_name = '%s' and schema_name = '%s' and object_name = '%s' "},
  {"     and object_type = '%s') %s "},
  {"union "},
  {"(select grantee_name, "},
  {"  case when bitextract(privileges_bitmap,63,1) = 1 then 'S' else '-' end || "},
  {"  case when bitextract(privileges_bitmap,62,1) = 1 then 'I' else '-' end || "},
  {"  case when bitextract(privileges_bitmap,61,1) = 1 then 'D' else '-' end || "},
  {"  case when bitextract(privileges_bitmap,60,1) = 1 then 'U' else '-' end || "},
  {"  case when bitextract(privileges_bitmap,59,1) = 1 then 'G' else '-' end || "},
  {"  case when bitextract(privileges_bitmap,58,1) = 1 then 'R' else '-' end || "},
  {"  case when bitextract(privileges_bitmap,57,1) = 1 then 'E' else '-' end as privs "},
  {" from %s.\"%s\".%s "},
  {" where object_uid = "},
  {"  (select object_uid from %s.\"%s\".%s "},
  {"   where catalog_name = '%s' and schema_name = '%s' and object_name = '%s' "},
  {"     and object_type = '%s') %s )"},
  {" order by 1 "},
  {" ; "}
};

static const QueryString getTrafObjectsForUser[] = 
{
  {" select trim(T.catalog_name) || '.\"' || trim(T.schema_name) || '\".' || trim(T.object_name) "},
  {" from %s.\"%s\".%s T "},
  {" where T.catalog_name = '%s' "},
  {" and T.object_type = '%s' %s "},
  {" order by 1  "},
  {" ; "}
};

static const QueryString getHiveRegObjectsInCatalogQuery[] =
{
  {" select trim(O.a) ||  "                                    },
  {" case when G.b is null and O.t != 'SS' then ' (inconsistent)' else '' end "},
  {" from "                                                    },
  {"  (select object_type, case when object_type = 'SS' "      },
  {"   then lower(trim(catalog_name) || '.' || trim(schema_name)) "},
  {"   else lower(trim(catalog_name) || '.' || "               },
  {"    trim(schema_name) || '.' || trim(object_name)) end "   },
  {"   from %s.\"%s\".%s where catalog_name = 'HIVE' and "     },
  {"                           %s %s) O(t, a) "                },
  {"  left join "                                              },
  {"   (select '%s' || '.' || trim(y) from "                   },
  {"    (get %s in catalog %s, no header) x(y)) G(b)"          },
  {"   on O.a = G.b  "                                         },
  {" order by 1 "                                              },
  {"; "                                                        }
};

static const QueryString getHBaseRegTablesInCatalogQuery[] =
{
  {" select '\"' || trim(O.s) || '\"' || '.' || trim(O.o) ||  "},
  {" case when G.b is null then ' (inconsistent)' else '' end "},
  {" from "                                                    },
  {"  (select trim(schema_name), trim(object_name)            "},
  {"   from %s.\"%s\".%s where catalog_name = 'HBASE'     "    },
  {"      and object_type = 'BT' %s) O(s, o) "                 },
  {"  left join "                                              },
  {"   (select trim(y) from "                                  },
  {"    (get external hbase objects) x(y)) G(b)"               },
  {"   on O.o = G.b  "                                         },
  {" group by 1 order by 1 "                                   },
  {"; "                                                        }
};

static const QueryString getHiveExtTablesInCatalogQuery[] =
{
  {" select trim(O.a) ||  "                                    },
  {" case when G.b is null then ' (inconsistent)' else '' end "},
  {" from "                                                    },
  {"  (select '%s' || '.' || "                                 },
  {"   lower(trim(substring(schema_name, 5, "                  },
  {"                char_length(schema_name)-5))) "            },
  {"    || '.' || lower(trim(object_name)) "                   },
  {"   from %s.\"%s\".%s where object_type = '%s' "            },
  {"    and schema_name like '|_HV|_%%|_' escape '|' %s) O(a)" },
  {"  left join "                                              },
  {"   (select '%s' || '.' || trim(y) from "                   },
  {"    (get %s in catalog %s, no header) x(y)) G(b) "         },
  {"   on O.a = G.b  "                                         },
  {" order by 1 "                                              },
  {"; "                                                        }
};

Lng32 ExExeUtilGetMetadataInfoTcb::getUsingView(Queue * infoList,
					       NABoolean isShorthandView,
					       char* &viewName, Lng32 &len)
{
  Lng32 cliRC = 0;

  while (1)
    {
      switch (vStep_)
	{
	case VIEWS_INITIAL_:
	  {
	    infoList->position();

	    vStep_ = VIEWS_FETCH_PROLOGUE_;
	  }
	break;

	case VIEWS_FETCH_PROLOGUE_:
	  {
	    if (infoList->atEnd())
	      {
		vStep_ = VIEWS_DONE_;
		break;
	      }

	    OutputInfo * vi = (OutputInfo*)infoList->getCurr();
	    char * ptr = vi->get(0);

	    char * outBuf = new(getGlobals()->getDefaultHeap())
	      char[ComMAX_3_PART_EXTERNAL_UTF8_NAME_LEN_IN_BYTES+6+1];

	    char * parts[4];
	    Lng32 numParts = 0;
	    LateNameInfo::extractParts(ptr, outBuf, numParts, parts, 
				       ((ptr[0] == '\"') ? FALSE : TRUE));

	    char query[2000];
	    if (isShorthandView)
	      {
		if (numParts == 1)
		  {
		    str_sprintf(query, "get all views on view %s, no header;",
				parts[0]);
		  }
		else if (numParts == 2)
		  {
		    str_sprintf(query, "get all views on view \"%s\".%s.%s, no header;",
				getMItdb().getCat(), parts[0], parts[1]);
		  }
		else if (numParts == 3)
		  {
		    str_sprintf(query, "get all views on view %s.%s.%s, no header;",
				parts[0], parts[1], parts[2]);
		  }
	      }
	    else
	      {
		if (numParts == 1)
		  {
		    str_sprintf(query, "get all mvs on mv %s, no header;",
				parts[0]);
		  }
		else if (numParts == 2)
		  {
		    str_sprintf(query, "get all mvs on mv \"%s\".%s.%s, no header;",
				getMItdb().getCat(), parts[0], parts[1]);
		  }
		else if (numParts == 3)
		  {
		    str_sprintf(query, "get all mvs on mv %s.%s.%s, no header;",
				parts[0], parts[1], parts[2]);
		  }
	      }

	    NADELETEBASIC(outBuf, getGlobals()->getDefaultHeap());

	    cliRC = cliInterface()->fetchRowsPrologue(query);
	    if (cliRC < 0)
	      {
                cliInterface()->allocAndRetrieveSQLDiagnostics(diagsArea_);
		vStep_ = VIEWS_ERROR_;
		break;
	      }

	    vStep_ = VIEWS_FETCH_ROW_;
	  }
	break;

	case VIEWS_FETCH_ROW_:
	  {
	    cliRC = cliInterface()->fetch();
	    if (cliRC < 0)
	      {
                cliInterface()->allocAndRetrieveSQLDiagnostics(diagsArea_);
		vStep_ = VIEWS_ERROR_;
		break;
	      }

	    if (cliRC == 100)
	      {
		vStep_ = VIEWS_FETCH_EPILOGUE_;
		break;
	      }

	    cliInterface()->getPtrAndLen(1, viewName, len);
	    return 0;
	  }
	break;

	case VIEWS_ERROR_:
	  {
	    vStep_ = VIEWS_INITIAL_;
	    return cliRC;
	  }
	break;

	case VIEWS_FETCH_EPILOGUE_:
	  {
	    cliRC = cliInterface()->fetchRowsEpilogue(0);
	    if (cliRC < 0)
	      {
                cliInterface()->allocAndRetrieveSQLDiagnostics(diagsArea_);
		vStep_ = VIEWS_ERROR_;
		break;
	      }

	    infoList->advance();

	    vStep_ = VIEWS_FETCH_PROLOGUE_;
	  }
	break;

	case VIEWS_DONE_:
	  {
	    // all done
	    vStep_ = VIEWS_INITIAL_;
	    return 100;
	  }
	break;

	}
    }
}

Lng32 ExExeUtilGetMetadataInfoTcb::getUsedObjects(Queue * infoList,
						 NABoolean isShorthandView,
						 char* &viewName, Lng32 &len)
{
  Lng32 cliRC = 0;

  while (1)
    {
      switch (vStep_)
	{
	case VIEWS_INITIAL_:
	  {
	    infoList->position();

	    vStep_ = VIEWS_FETCH_PROLOGUE_;
	  }
	break;

	case VIEWS_FETCH_PROLOGUE_:
	  {
	    if (infoList->atEnd())
	      {
		vStep_ = VIEWS_DONE_;
		break;
	      }

	    OutputInfo * vi = (OutputInfo*)infoList->getCurr();
	    char * ptr = vi->get(0);
	    char * objTyp = vi->get(1);

	    if ((objTyp) && (strcmp(objTyp, "BT") == 0))
	      {
		infoList->advance();

		vStep_ = VIEWS_FETCH_PROLOGUE_;
		break;
	      }

	    char * outBuf = new(getGlobals()->getDefaultHeap())
	      char[ComMAX_3_PART_EXTERNAL_UTF8_NAME_LEN_IN_BYTES+6+1];

	    char * parts[4];
	    Lng32 numParts = 0;
	    LateNameInfo::extractParts(ptr, outBuf, numParts, parts, TRUE);

	    char query[2000];
	    char objectStr[20];

	    if (getMItdb().queryType_ == ComTdbExeUtilGetMetadataInfo::TABLES_IN_VIEW_)
	      strcpy(objectStr, "tables");
	    else if (getMItdb().queryType_ == ComTdbExeUtilGetMetadataInfo::VIEWS_IN_VIEW_)
	      strcpy(objectStr, "views");
	    else if (getMItdb().queryType_ == ComTdbExeUtilGetMetadataInfo::OBJECTS_IN_VIEW_)
	      strcpy(objectStr, "objects");
	    //else if (getMItdb().queryType_ == ComTdbExeUtilGetMetadataInfo::TABLES_IN_MV_)
	    //  strcpy(objectStr, "tables");
	    //else if (getMItdb().queryType_ == ComTdbExeUtilGetMetadataInfo::MVS_IN_MV_)
	    //  strcpy(objectStr, "mvs");
	    //else if (getMItdb().queryType_ == ComTdbExeUtilGetMetadataInfo::OBJECTS_IN_MV_)
	    //  strcpy(objectStr, "objects");

	    char inStr[10];
	    if (isShorthandView)
	      strcpy(inStr, "view");
	    else
	      strcpy(inStr, "mv");

	    if (numParts == 1)
	      str_sprintf(query, "get all %s in %s %s, no header",
			  objectStr, inStr, parts[0]);
	    else if (numParts == 2)
	      str_sprintf(query, "get all %s in %s %s.%s, no header",
			  objectStr, inStr, parts[0], parts[1]);
	    else if (numParts == 3)
	      str_sprintf(query, "get all %s in %s %s.%s.%s, no header",
			  objectStr, inStr, parts[0], parts[1], parts[2]);

	    if (getMItdb().getPattern())
	      {
		strcat(query, ", match '");
		strcat(query, getMItdb().getPattern());
		strcat(query, "'");
	      }

	    strcat(query, ";");

	    NADELETEBASIC(outBuf, getGlobals()->getDefaultHeap());

	    cliRC = cliInterface()->fetchRowsPrologue(query);
	    if (cliRC < 0)
	      {
                cliInterface()->allocAndRetrieveSQLDiagnostics(diagsArea_);
		vStep_ = VIEWS_ERROR_;
		break;
	      }

	    vStep_ = VIEWS_FETCH_ROW_;
	  }
	break;

	case VIEWS_FETCH_ROW_:
	  {
	    cliRC = cliInterface()->fetch();
	    if (cliRC < 0)
	      {
                cliInterface()->allocAndRetrieveSQLDiagnostics(diagsArea_);
		vStep_ = VIEWS_ERROR_;
		break;
	      }

	    if (cliRC == 100)
	      {
		vStep_ = VIEWS_FETCH_EPILOGUE_;
		break;
	      }

	    cliInterface()->getPtrAndLen(1, viewName, len);
	    return 0;
	  }
	break;

	case VIEWS_ERROR_:
	  {
	    vStep_ = VIEWS_INITIAL_;
	    return cliRC;
	  }
	break;

	case VIEWS_FETCH_EPILOGUE_:
	  {
	    cliRC = cliInterface()->fetchRowsEpilogue(0);
	    if (cliRC < 0)
	      {
                cliInterface()->allocAndRetrieveSQLDiagnostics(diagsArea_);
		vStep_ = VIEWS_ERROR_;
		break;
	      }

	    infoList->advance();

	    vStep_ = VIEWS_FETCH_PROLOGUE_;
	  }
	break;

	case VIEWS_DONE_:
	  {
	    // all done
	    vStep_ = VIEWS_INITIAL_;
	    return 100;
	  }
	break;

	}
    }
}

short ExExeUtilGetMetadataInfoTcb::displayHeading()
{
  if (getMItdb().noHeader())
    {
      return 0;
    }
  // make sure there is enough space to move header
  if ((qparent_.up->getSize() - qparent_.up->getLength()) < 7)
    return 1;	//come back later

  switch (getMItdb().queryType_)
    {
    case ComTdbExeUtilGetMetadataInfo::CATALOGS_:
      {
	str_sprintf(headingBuf_, "Catalogs");
      }
    break;

    case ComTdbExeUtilGetMetadataInfo::SCHEMAS_IN_CATALOG_:
      {
	str_sprintf(headingBuf_, "Schemas in Catalog %s",
		    getMItdb().getCat());
      }
    break;

    case ComTdbExeUtilGetMetadataInfo::INVALID_VIEWS_IN_CATALOG_:
      {
	str_sprintf(headingBuf_, "Invalid Views in Catalog %s",
		    getMItdb().getCat());
      }
    break;

    case ComTdbExeUtilGetMetadataInfo::TABLES_IN_CATALOG_:
      {
	str_sprintf(headingBuf_, "Tables in Catalog %s",
		    getMItdb().getCat());
      }
    break;

    case ComTdbExeUtilGetMetadataInfo::VIEWS_IN_CATALOG_:
      {
	str_sprintf(headingBuf_, "Views in Catalog %s",
		    getMItdb().getCat());
      }
    break;

    case ComTdbExeUtilGetMetadataInfo::OBJECTS_IN_CATALOG_:
      {
	str_sprintf(headingBuf_, "Objects in Catalog %s",
		    getMItdb().getCat());
      }
    break;

    case ComTdbExeUtilGetMetadataInfo::HIVE_REG_TABLES_IN_CATALOG_:
      {
	str_sprintf(headingBuf_, "Hive Registered Tables in Catalog %s",
		    getMItdb().getCat());
      }
    break;

    case ComTdbExeUtilGetMetadataInfo::HBASE_REG_TABLES_IN_CATALOG_:
      {
	str_sprintf(headingBuf_, "HBase Registered Tables in Catalog %s",
		    getMItdb().getCat());
      }
    break;

    case ComTdbExeUtilGetMetadataInfo::HBASE_OBJECTS_:
      {
	str_sprintf(headingBuf_, "External HBase objects");
      }
    break;

    case ComTdbExeUtilGetMetadataInfo::HIVE_REG_VIEWS_IN_CATALOG_:
      {
	str_sprintf(headingBuf_, "Hive Registered Views in Catalog %s",
		    getMItdb().getCat());
      }
    break;

    case ComTdbExeUtilGetMetadataInfo::HIVE_REG_SCHEMAS_IN_CATALOG_:
      {
	str_sprintf(headingBuf_, "Hive Registered Schemas in Catalog %s",
		    getMItdb().getCat());
      }
    break;

    case ComTdbExeUtilGetMetadataInfo::HIVE_REG_OBJECTS_IN_CATALOG_:
      {
	str_sprintf(headingBuf_, "Hive Registered Objects in Catalog %s",
		    getMItdb().getCat());
      }
    break;

    case ComTdbExeUtilGetMetadataInfo::HIVE_EXT_TABLES_IN_CATALOG_:
      {
	str_sprintf(headingBuf_, "Hive External Tables in Catalog %s",
		    getMItdb().getCat());
      }
    break;

    case ComTdbExeUtilGetMetadataInfo::TABLES_IN_SCHEMA_:
      {
	str_sprintf(headingBuf_, "Tables in Schema %s.%s",
		    getMItdb().getCat(), getMItdb().getSch());
      }
    break;

    case ComTdbExeUtilGetMetadataInfo::INDEXES_IN_SCHEMA_:
      {
	str_sprintf(headingBuf_, "Indexes in Schema %s.%s",
		    getMItdb().getCat(), getMItdb().getSch());
      }
    break;

    case ComTdbExeUtilGetMetadataInfo::VIEWS_IN_SCHEMA_:
      {
	str_sprintf(headingBuf_, "Views in Schema %s.%s",
		    getMItdb().getCat(), getMItdb().getSch());
      }
    break;

    case ComTdbExeUtilGetMetadataInfo::OBJECTS_IN_SCHEMA_:
      {
	str_sprintf(headingBuf_, "Objects in Schema %s.%s",
		    getMItdb().getCat(), getMItdb().getSch());
      }
    break;

    case ComTdbExeUtilGetMetadataInfo::INVALID_VIEWS_IN_SCHEMA_:
      {
	str_sprintf(headingBuf_, "Invalid Views in Schema %s.%s",
		    getMItdb().getCat(), getMItdb().getSch());
      }
    break;

    case ComTdbExeUtilGetMetadataInfo::LIBRARIES_IN_SCHEMA_:
      {
	str_sprintf(headingBuf_, "Libraries in Schema %s.%s",
		    getMItdb().getCat(), getMItdb().getSch());
      }
    break;

    case ComTdbExeUtilGetMetadataInfo::PROCEDURES_IN_SCHEMA_:
      {
	str_sprintf(headingBuf_, "Procedures in Schema %s.%s",
		    getMItdb().getCat(), getMItdb().getSch());
      }
    break;

    case ComTdbExeUtilGetMetadataInfo::FUNCTIONS_IN_SCHEMA_:
      {
	str_sprintf(headingBuf_, "Functions in Schema %s.%s",
		    getMItdb().getCat(), getMItdb().getSch());
      }
    break;

    case ComTdbExeUtilGetMetadataInfo::TABLE_FUNCTIONS_IN_SCHEMA_:
      {
	str_sprintf(headingBuf_, "Table_mapping functions in Schema %s.%s",
		    getMItdb().getCat(), getMItdb().getSch());
      }
    break;

    case ComTdbExeUtilGetMetadataInfo::INDEXES_ON_TABLE_:
      {
	str_sprintf(headingBuf_, "Indexes on Table %s.%s",
		    getMItdb().getSch(), getMItdb().getObj());
      }
    break;

    case ComTdbExeUtilGetMetadataInfo::PRIVILEGES_ON_SCHEMA_:
      {
	str_sprintf(headingBuf_, "Privileges on Schema %s.%s",
		    getMItdb().getCat(), getMItdb().getSch());
      }
    break;

    case ComTdbExeUtilGetMetadataInfo::PRIVILEGES_ON_TABLE_:
      {
	str_sprintf(headingBuf_, "Privileges on Table %s.%s",
		    getMItdb().getSch(), getMItdb().getObj());
      }
    break;

    case ComTdbExeUtilGetMetadataInfo::PRIVILEGES_ON_VIEW_:
      {
	str_sprintf(headingBuf_, "Privileges on View %s.%s",
		    getMItdb().getSch(), getMItdb().getObj());
      }
    break;

    case ComTdbExeUtilGetMetadataInfo::PRIVILEGES_ON_SEQUENCE_:
      {
        str_sprintf(headingBuf_, "Privileges on Sequence %s.%s",
                    getMItdb().getSch(), getMItdb().getObj());
      }
    break;

    case ComTdbExeUtilGetMetadataInfo::PRIVILEGES_ON_LIBRARY_:
      {
        str_sprintf(headingBuf_, "Privileges on Library %s.%s",
                    getMItdb().getSch(), getMItdb().getObj());
      }
    break;

    case ComTdbExeUtilGetMetadataInfo::PRIVILEGES_ON_ROUTINE_:
      {
        str_sprintf(headingBuf_, "Privileges on Routine %s.%s",
                    getMItdb().getSch(), getMItdb().getObj());
      }
    break;

    case ComTdbExeUtilGetMetadataInfo::VIEWS_ON_TABLE_:
    case ComTdbExeUtilGetMetadataInfo::VIEWS_ON_VIEW_:
      {
	str_sprintf(headingBuf_,
		    (getMItdb().queryType_ == ComTdbExeUtilGetMetadataInfo::VIEWS_ON_TABLE_
		     ? "Views on Table %s.%s" : "Views ON View %s.%s"),
		    getMItdb().getSch(), getMItdb().getObj());
      }
    break;

    case ComTdbExeUtilGetMetadataInfo::PARTITIONS_FOR_TABLE_:
      {
	str_sprintf(headingBuf_, "Partitions for Table %s.%s",
		    getMItdb().getSch(), getMItdb().getObj());
      }
    break;

    case ComTdbExeUtilGetMetadataInfo::PARTITIONS_FOR_INDEX_:
      {
	str_sprintf(headingBuf_, "Partitions for Index %s.%s",
		    getMItdb().getSch(), getMItdb().getObj());
      }
    break;

    case ComTdbExeUtilGetMetadataInfo::OBJECTS_ON_TABLE_:
      {
	str_sprintf(headingBuf_, "Objects on Table %s.%s",
		    getMItdb().getSch(), getMItdb().getObj());
      }
    break;

    case ComTdbExeUtilGetMetadataInfo::SEQUENCES_IN_SCHEMA_:
      {
	str_sprintf(headingBuf_, "Sequences in schema %s.%s",
		    getMItdb().getCat(), getMItdb().getSch());
      }
    break;

    case ComTdbExeUtilGetMetadataInfo::SEQUENCES_IN_CATALOG_:
      {
	str_sprintf(headingBuf_, "Sequences in catalog %s",
		    getMItdb().getCat());
      }
    break;

    case ComTdbExeUtilGetMetadataInfo::TABLES_IN_VIEW_:
    case ComTdbExeUtilGetMetadataInfo::VIEWS_IN_VIEW_:
    case ComTdbExeUtilGetMetadataInfo::OBJECTS_IN_VIEW_:
      {
	str_sprintf(headingBuf_,
		    (getMItdb().queryType_ == ComTdbExeUtilGetMetadataInfo::TABLES_IN_VIEW_ ?
		     "Tables in View %s.%s" :
		     (getMItdb().queryType_ == ComTdbExeUtilGetMetadataInfo::VIEWS_IN_VIEW_ ?
		      "Views in View %s.%s" :
		      "Objects in View %s.%s")),
		    getMItdb().getSch(), getMItdb().getObj());
      }
    break;

    case ComTdbExeUtilGetMetadataInfo::ROLES_:
        str_sprintf(headingBuf_,"Roles");
    break;

    case ComTdbExeUtilGetMetadataInfo::ROLES_FOR_ROLE_:
        str_sprintf(headingBuf_,"Roles granted Role %s",getMItdb().getParam1());
    break;

    case ComTdbExeUtilGetMetadataInfo::USERS_FOR_ROLE_:
        str_sprintf(headingBuf_,"Users granted Role %s",getMItdb().getParam1());
    break;

    case ComTdbExeUtilGetMetadataInfo::USERS_:
      {
        str_sprintf(headingBuf_,
                    (getMItdb().queryType_ == ComTdbExeUtilGetMetadataInfo::USERS_
                     ? "Users" : "Current User")
        );
      }
    break;

    case ComTdbExeUtilGetMetadataInfo::FUNCTIONS_FOR_USER_:
        str_sprintf(headingBuf_,"Functions for User %s",getMItdb().getParam1());
    break;

    case ComTdbExeUtilGetMetadataInfo::FUNCTIONS_FOR_ROLE_:
        str_sprintf(headingBuf_,"Functions for Role %s",getMItdb().getParam1());
    break;

    case ComTdbExeUtilGetMetadataInfo::INDEXES_FOR_USER_:
        str_sprintf(headingBuf_,"Indexes for User %s",getMItdb().getParam1());
    break;

    case ComTdbExeUtilGetMetadataInfo::INDEXES_FOR_ROLE_:
        str_sprintf(headingBuf_,"Indexes for Role %s",getMItdb().getParam1());
    break;

    case ComTdbExeUtilGetMetadataInfo::LIBRARIES_FOR_USER_:
        str_sprintf(headingBuf_,"Libraries for User %s", getMItdb().getParam1());
    break;

    case ComTdbExeUtilGetMetadataInfo::LIBRARIES_FOR_ROLE_:
        str_sprintf(headingBuf_,"Libraries for Role %s", getMItdb().getParam1());
    break;

    case ComTdbExeUtilGetMetadataInfo::PROCEDURES_FOR_LIBRARY_:
        str_sprintf(headingBuf_,"Procedures for Library %s.%s",getMItdb().getSch(), getMItdb().getObj());
    break;

    case ComTdbExeUtilGetMetadataInfo::FUNCTIONS_FOR_LIBRARY_:
        str_sprintf(headingBuf_,"Functions for Library %s.%s",getMItdb().getSch(), getMItdb().getObj());
    break;

    case ComTdbExeUtilGetMetadataInfo::TABLE_FUNCTIONS_FOR_LIBRARY_:
        str_sprintf(headingBuf_,"Table_mapping Functions for Library %s.%s",getMItdb().getSch(), getMItdb().getObj());
    break;

    case ComTdbExeUtilGetMetadataInfo::PRIVILEGES_FOR_USER_:
        str_sprintf(headingBuf_,"Privileges for User %s",getMItdb().getParam1());
    break;

    case ComTdbExeUtilGetMetadataInfo::PRIVILEGES_FOR_ROLE_:
        str_sprintf(headingBuf_,"Privileges for Role %s",getMItdb().getParam1());
    break;

    case ComTdbExeUtilGetMetadataInfo::PROCEDURES_FOR_USER_:
        str_sprintf(headingBuf_,"Procedures for User %s",getMItdb().getParam1());
    break;

    case ComTdbExeUtilGetMetadataInfo::PROCEDURES_FOR_ROLE_:
        str_sprintf(headingBuf_,"Procedures for Role %s",getMItdb().getParam1());
    break;

    case ComTdbExeUtilGetMetadataInfo::ROLES_FOR_USER_:
        str_sprintf(headingBuf_,"Roles for User %s",getMItdb().getParam1());
    break;

    case ComTdbExeUtilGetMetadataInfo::SCHEMAS_FOR_ROLE_:
        str_sprintf(headingBuf_,"Schemas for Role %s",getMItdb().getParam1());
    break;

    case ComTdbExeUtilGetMetadataInfo::SCHEMAS_FOR_USER_:
        str_sprintf(headingBuf_,"Schemas for User %s",getMItdb().getParam1());
    break;

    case ComTdbExeUtilGetMetadataInfo::TABLE_FUNCTIONS_FOR_USER_:
        str_sprintf(headingBuf_,"Table mapping functions for User %s",getMItdb().getParam1());
    break;

    case ComTdbExeUtilGetMetadataInfo::TABLE_FUNCTIONS_FOR_ROLE_:
        str_sprintf(headingBuf_,"Table mapping functions for Role %s",getMItdb().getParam1());
    break;

    case ComTdbExeUtilGetMetadataInfo::TABLES_FOR_USER_:
        str_sprintf(headingBuf_,"Tables for User %s",getMItdb().getParam1());
    break;

    case ComTdbExeUtilGetMetadataInfo::TABLES_FOR_ROLE_:
        str_sprintf(headingBuf_,"Tables for Role %s",getMItdb().getParam1());
    break;

    case ComTdbExeUtilGetMetadataInfo::VIEWS_FOR_USER_:
        str_sprintf(headingBuf_,"Views for User %s",getMItdb().getParam1());
    break;

    case ComTdbExeUtilGetMetadataInfo::VIEWS_FOR_ROLE_:
        str_sprintf(headingBuf_,"Views for Role %s",getMItdb().getParam1());
    break;

    case ComTdbExeUtilGetMetadataInfo::COMPONENTS_:
        str_sprintf(headingBuf_, "Components");
    break;

    case ComTdbExeUtilGetMetadataInfo::COMPONENT_PRIVILEGES_:
       {	 
      	 if (getMItdb().getParam1())
            str_sprintf(headingBuf_, "Privilege information on Component %s for %s",
                        getMItdb().getObj(),getMItdb().getParam1());
         
         else
            str_sprintf(headingBuf_, "Operation information on Component %s",
                        getMItdb().getObj());
         break;
       }

// Not supported at this time
#if 0
    case ComTdbExeUtilGetMetadataInfo::TRIGGERS_FOR_USER_:
        str_sprintf(headingBuf_,"Triggers for User %s",getMItdb().getParam1());
    break;

    case ComTdbExeUtilGetMetadataInfo::INDEXES_ON_MV_:
      {
	str_sprintf(headingBuf_, "Indexes on MV %s.%s",
		    getMItdb().getSch(), getMItdb().getObj());
      }
    break;

    case ComTdbExeUtilGetMetadataInfo::PRIVILEGES_ON_MV_:
      {
	str_sprintf(headingBuf_, "Privileges on MV %s.%s",
		    getMItdb().getSch(), getMItdb().getObj());
      }
    break;

     case ComTdbExeUtilGetMetadataInfo::IUDLOG_TABLE_ON_TABLE_:
      {
	str_sprintf(headingBuf_, "Iudlog tables  for Table %s.%s",
		    getMItdb().getSch(), getMItdb().getObj());
      }
    break;
    case ComTdbExeUtilGetMetadataInfo::RANGELOG_TABLE_ON_TABLE_:
      {
	str_sprintf(headingBuf_, "Rangelog table for Table %s.%s",
		    getMItdb().getSch(), getMItdb().getObj());
      }
    break;
     case ComTdbExeUtilGetMetadataInfo::TRIGTEMP_TABLE_ON_TABLE_:
      {
	str_sprintf(headingBuf_, "Trigger temp table for Table %s.%s",
		    getMItdb().getSch(), getMItdb().getObj());
      }
      break;
     case ComTdbExeUtilGetMetadataInfo::IUDLOG_TABLE_ON_MV_:
      {
	str_sprintf(headingBuf_, "Iudlog table  for MV %s.%s",
		    getMItdb().getSch(), getMItdb().getObj());
      }
    break;
    case ComTdbExeUtilGetMetadataInfo::RANGELOG_TABLE_ON_MV_:
      {
	str_sprintf(headingBuf_, "Rangelog table for MV %s.%s",
		    getMItdb().getSch(), getMItdb().getObj());
      }
    break;
     case ComTdbExeUtilGetMetadataInfo::TRIGTEMP_TABLE_ON_MV_:
      {
	str_sprintf(headingBuf_, "Trigger temp table for MV %s.%s",
		    getMItdb().getSch(), getMItdb().getObj());
      }
    break;
    case ComTdbExeUtilGetMetadataInfo::IUDLOG_TABLES_IN_SCHEMA_:
      {
	str_sprintf(headingBuf_, "Iud log tables in schema %s.%s",
		    getMItdb().getCat(), getMItdb().getSch());
      }
    break;
    case ComTdbExeUtilGetMetadataInfo::RANGELOG_TABLES_IN_SCHEMA_:
      {
	str_sprintf(headingBuf_, "Range log tables in schema %s.%s",
		    getMItdb().getCat(), getMItdb().getSch());
      }
    break;

    case ComTdbExeUtilGetMetadataInfo::TRIGTEMP_TABLES_IN_SCHEMA_:
      {
	str_sprintf(headingBuf_, "Trigger temp tables in schema %s.%s",
		    getMItdb().getCat(), getMItdb().getSch());
      }
    break;

    case ComTdbExeUtilGetMetadataInfo::SYNONYMS_IN_SCHEMA_:
      {
	str_sprintf(headingBuf_, "Synonyms in Schema %s.%s",
		    getMItdb().getCat(), getMItdb().getSch());
      }
    break;
    case ComTdbExeUtilGetMetadataInfo::SYNONYMS_FOR_USER_:
        str_sprintf(headingBuf_,"Synonyms for User %s",getMItdb().getParam1());
    break;

    case ComTdbExeUtilGetMetadataInfo::SYNONYMS_ON_TABLE_:
      {
	str_sprintf(headingBuf_, "Synonyms on Table %s.%s",
		    getMItdb().getSch(), getMItdb().getObj());
      }
    break;


    case ComTdbExeUtilGetMetadataInfo::MVS_IN_SCHEMA_:
      {
	str_sprintf(headingBuf_, "MVs in Schema %s.%s",
		    getMItdb().getCat(), getMItdb().getSch());
      }
    break;

    case ComTdbExeUtilGetMetadataInfo::MVS_ON_TABLE_:
    case ComTdbExeUtilGetMetadataInfo::MVS_ON_MV_:
      {
	str_sprintf(headingBuf_,
		    (getMItdb().queryType_ == ComTdbExeUtilGetMetadataInfo::MVS_ON_TABLE_
		     ? "MVs on Table %s.%s" : "MVs ON MV %s.%s"),
		    getMItdb().getSch(), getMItdb().getObj());
      }
    break;
    case ComTdbExeUtilGetMetadataInfo::TABLES_IN_MV_:
    case ComTdbExeUtilGetMetadataInfo::MVS_IN_MV_:
    case ComTdbExeUtilGetMetadataInfo::OBJECTS_IN_MV_:
      {
	str_sprintf(headingBuf_,
		    (getMItdb().queryType_ == ComTdbExeUtilGetMetadataInfo::TABLES_IN_MV_ ?
		     "Tables in MV %s.%s" :
		     (getMItdb().queryType_ == ComTdbExeUtilGetMetadataInfo::MVS_IN_MV_ ?
		      "MVs in MV %s.%s" :
		      "Objects in MV %s.%s")),
		    getMItdb().getSch(), getMItdb().getObj());
      }
    break;

    case ComTdbExeUtilGetMetadataInfo::MVS_FOR_USER_:
        str_sprintf(headingBuf_,"Materialized Views for User %s",getMItdb().getParam1());
    break;

    case ComTdbExeUtilGetMetadataInfo::MVGROUPS_FOR_USER_:
        str_sprintf(headingBuf_,"Materialized View Groups for User %s",getMItdb().getParam1());
    break;

#endif

    default:
      str_sprintf(headingBuf_, "Add to ExExeUtilGetMetadataInfoTcb::displayHeading");
      break;
    }

  moveRowToUpQueue(headingBuf_);
  str_pad(outputBuf_, strlen(headingBuf_), '=');
  outputBuf_[strlen(headingBuf_)] = 0;
  moveRowToUpQueue(outputBuf_);

  moveRowToUpQueue(" ");

  return 0;
} // ExExeUtilGetMetadataInfoTcb::displayHeading

// ----------------------------------------------------------------------------
// getAuthID
//
// Reads the "_MD_".auths table to get the auth_id from the passed in authName.
// If relationship not found for any reason, return 0, otherwise return
// the authID.
//
// TBD - should replace this with a call to currContext->getAuthIDFromName
//       this function checks for special authID and looks at cache before
//       calling metadata.  Currently there is an issue because privilege 
//       error are returned when trying to read AUTHS table.  Need to set 
//       parserflag 131072.
// ----------------------------------------------------------------------------
Int32 ExExeUtilGetMetadataInfoTcb::getAuthID(
  const char *authName,
  const char *catName,
  const char *schName, 
  const char *objName)
{
  if (strcmp(authName, PUBLIC_AUTH_NAME) == 0)
    return PUBLIC_USER;

  short rc      = 0;
  Lng32 cliRC   = 0;

  sprintf(queryBuf_, "select auth_id from %s.\"%s\".%s where auth_db_name = '%s' ",
          catName, schName, objName, authName);

  if (initializeInfoList(infoList_)) return NA_UserIdDefault;

  numOutputEntries_ = 1;
  cliRC = fetchAllRows(infoList_, queryBuf_, numOutputEntries_, FALSE, rc);
  if (cliRC < 0) 
  {
    cliInterface()->allocAndRetrieveSQLDiagnostics(diagsArea_);
    return NA_UserIdDefault;
  }

  infoList_->position();
  OutputInfo * vi = (OutputInfo*)infoList_->getCurr();
  if (vi)
    return *(Lng32*)vi->get(0);
  return NA_UserIdDefault;
}

// ----------------------------------------------------------------------------
// getGrantedPrivCmd
//
// Generates syntax that limits the result set to those objects where the 
// current user has at least one privilege assigned. The syntax unions grantees
// from object_privileges, column_privileges, and schema_privileges. The 
// grantee list (authList) includes the current user and the current users 
// roles.
// ---------------------------------------------------------------------------- 
NAString ExExeUtilGetMetadataInfoTcb::getGrantedPrivCmd(
  const NAString &authList,
  const char * cat,
  const NAString &inColumn)
{
  char buf [authList.length()*3 + MAX_SQL_IDENTIFIER_NAME_LEN*9 + 200];
  snprintf(buf, sizeof(buf), "and %s in (select object_uid from %s.\"%s\".%s "
                             "where grantee_id in %s union "
                             "(select object_uid from %s.\"%s\".%s "
                             " where grantee_id in %s) union "
                             "(select schema_uid from %s.\"%s\".%s "
                             " where grantee_id in %s))",
           inColumn.data(),
           cat, SEABASE_PRIVMGR_SCHEMA, PRIVMGR_OBJECT_PRIVILEGES, authList.data(),
           cat, SEABASE_PRIVMGR_SCHEMA, PRIVMGR_COLUMN_PRIVILEGES, authList.data(),
           cat, SEABASE_PRIVMGR_SCHEMA, PRIVMGR_SCHEMA_PRIVILEGES, authList.data());
 
  NAString cmd(buf); 
  return cmd;
}

// ----------------------------------------------------------------------------
// getRoleList
//
// Reads the "_PRIVMGR_MD_".role_usage table to return the list of role IDs
// granted to the user specified in userID.
//
// If none found, or an unexpected error occurs, NULL is returned.
// The function allocates memory for the returned role list, the caller is
// responsible for deleting this memory.
//
// The returned role list includes the roles granted, plus the userID passed
// in, plus the special role PUBLIC.  It is returned in a format that can be
// used in a query "in" clause.
//
// For example:
//   (-1, 33334, 1000004, 1000056)
// ----------------------------------------------------------------------------
char * ExExeUtilGetMetadataInfoTcb::getRoleList(
  const Int32 userID,
  const char *catName,
  const char *schName,
  const char *objName)
{
  // Always include PUBLIC
  NAString roleList("(-1");

  short rc      = 0;
  Lng32 cliRC   = 0;

  sprintf(queryBuf_, "select role_id from %s.\"%s\".%s where grantee_id = %d ",
          catName, schName, objName, userID);

  if (initializeInfoList(infoList_)) return NULL;

  numOutputEntries_ = 1;
  cliRC = fetchAllRows(infoList_, queryBuf_, numOutputEntries_, FALSE, rc);
  if (cliRC < 0)
    {
      cliInterface()->allocAndRetrieveSQLDiagnostics(diagsArea_);
      return NULL;
    }

  char buf[30];
  infoList_->position();
  while (NOT infoList_->atEnd())
    {
      OutputInfo * vi = (OutputInfo*)infoList_->getCurr();
      if (vi)
        {
          str_sprintf(buf, ", %d", *(Lng32*)vi->get(0));
          roleList += buf;
        }
          infoList_->advance();
    }
  str_sprintf(buf, ", %d)", userID);
  roleList += buf;

  char * list = new (getHeap()) char [roleList.length() + 1];
  strcpy(list, roleList.data());
  list[roleList.length()] = 0;

  return list;
}


// ----------------------------------------------------------------------------
// method:  checkUserPrivs
//
//  return TRUE to add privilege checks to queries
//  return FALSE to return all details independent of privileges
// ----------------------------------------------------------------------------
NABoolean ExExeUtilGetMetadataInfoTcb::checkUserPrivs(
  ContextCli * currContext,
  const ComTdbExeUtilGetMetadataInfo::QueryType queryType)
{
  // if no authorization, everyone sees everything
  if (!CmpCommon::context()->isAuthorizationEnabled())
    return FALSE;

  // Root user sees everything
  if (ComUser::isRootUserID())
    return FALSE;

  Int32 numRoles;
  Int32 *roleList;
  Int32 *granteeList;
  if (currContext->getRoleList(numRoles, roleList, granteeList) == SUCCESS)
  {
    char authIDAsChar[sizeof(Int32)+10];
    NAString auths;
    for (Int32 i = 0; i < numRoles; i++)
    {
      if (roleList[i] == ROOT_ROLE_ID)
        return FALSE;
    }
  }

  // any user granted the SHOW component privilege sees everything
  std::string privMDLoc = getMItdb().cat_.getPointer();
  privMDLoc += ".\"";
  privMDLoc += SEABASE_PRIVMGR_SCHEMA;
  privMDLoc += "\"";
  PrivMgrComponentPrivileges componentPrivileges(privMDLoc,getDiagsArea());

  if (componentPrivileges.hasSQLPriv(ComUser::getCurrentUser(),SQLOperation::SHOW,true)) return FALSE;

  // Check component privilege based on QueryType
  switch (queryType)
  {
    // if user has MANAGE_ROLES, can perform role operations
    case ComTdbExeUtilGetMetadataInfo::ROLES_:
    case ComTdbExeUtilGetMetadataInfo::ROLES_FOR_ROLE_:
    case ComTdbExeUtilGetMetadataInfo::ROLES_FOR_USER_:
    {
      if (componentPrivileges.hasSQLPriv(ComUser::getCurrentUser(),SQLOperation::MANAGE_ROLES, true))
        return FALSE;
      break;
    }

    // if user has MANAGE_USERS, can perform user operations
    case ComTdbExeUtilGetMetadataInfo::USERS_:
    case ComTdbExeUtilGetMetadataInfo::USERS_FOR_ROLE_:
    {
      if (componentPrivileges.hasSQLPriv(ComUser::getCurrentUser(),SQLOperation::MANAGE_USERS,true))
        return FALSE;
      break;
    }

    // if user has MANAGE_COMPONENTS, can perform component operations
    case ComTdbExeUtilGetMetadataInfo::COMPONENTS_:
    case ComTdbExeUtilGetMetadataInfo::COMPONENT_OPERATIONS_:
    case ComTdbExeUtilGetMetadataInfo::COMPONENT_PRIVILEGES_:
    {
      if (componentPrivileges.hasSQLPriv(ComUser::getCurrentUser(),SQLOperation::MANAGE_COMPONENTS,true))
        return FALSE;
      break;
    }

    // if user has MANAGE_LIBRARIES, can perform library operations
    case ComTdbExeUtilGetMetadataInfo::PROCEDURES_FOR_LIBRARY_:
    case ComTdbExeUtilGetMetadataInfo::FUNCTIONS_FOR_LIBRARY_:
    case ComTdbExeUtilGetMetadataInfo::TABLE_FUNCTIONS_FOR_LIBRARY_:
    {
      if (componentPrivileges.hasSQLPriv(ComUser::getCurrentUser(),SQLOperation::MANAGE_LIBRARY,true))
        return FALSE;
      break;
    }

   default:
     break;
  }
  return TRUE;
}

// ----------------------------------------------------------------------------
// method:  colPrivsFrag
//
// This method was added to address a performance issue.  When determining if 
// the user has column level privileges, we need to get the column name from 
// Hive.  The call to get the column name (hivemd) is very expensive.  So this
// method checks to see if the requested user has been granted any column
// level privileges on a hive table.  If so, we will go ahead and do the
// mapping (call hivemd).  If not, then we will not include the hivemd 
// fragment for the query.
//
// Since we are scanning the column privileges table anyway, we also see if 
// the requested user (or their roles) has been granted any privileges.  If so,
// we include the column privileges check in the query. 
//
// For Sentry enabled installations, we won't store Hive privileges in 
// EsgynDB metadata.  By avoiding the hivemd calls, we save a lot of time
// in processing the request.
//
//  returns additional union(s) for the getPrivForAuth query
//  returns:
//     0 - successful
//    -1 - unexpected error occurred
// ----------------------------------------------------------------------------
Int32 ExExeUtilGetMetadataInfoTcb::colPrivsFrag(
  const char *authName,
  const char * cat,
  const NAString &privWhereClause,
  NAString &colPrivsStmt)
{
  // if no authorization, skip
  if (!CmpCommon::context()->isAuthorizationEnabled())
    return 0;

  short rc      = 0;
  Lng32 cliRC   = 0;

  // See if privileges granted on Hive object or to the user/user's roles
  NAString likeClause("like 'HIVE.%'");
  sprintf(queryBuf_, "select "
                     "sum(case when (object_name %s and grantee_id %s) then 1 else 0 end), "
                     "sum(case when grantee_id %s then 1 else 0 end) "
                     "from %s.\"%s\".%s",
          likeClause.data(), privWhereClause.data(), privWhereClause.data(),
          cat, SEABASE_PRIVMGR_SCHEMA,
          PRIVMGR_COLUMN_PRIVILEGES);

  if (initializeInfoList(infoList_)) return -1;

  numOutputEntries_ = 2;
  cliRC = fetchAllRows(infoList_, queryBuf_, numOutputEntries_, FALSE, rc);
  if (cliRC < 0)
  {
    cliInterface()->retrieveSQLDiagnostics(getDiagsArea());
    return -1;
  }

  bool hasHive = false;
  bool hasGrants = false;
  infoList_->position();
  OutputInfo * vi = (OutputInfo*)infoList_->getCurr();
  if (vi && vi->get(0))
  {
    if (*(Int64*)vi->get(0) > 0)
      hasHive = true;
    if(*(Int64*)vi->get(1) > 0)
      hasGrants = true;
  }

  Int32 len = privWhereClause.length() + 500;
  char msg[len];
  snprintf(msg, len, "ExExeUtilGetMetadataUtilTcb::colPrivsFrag, user: %s, "
                     "grantees: %s, union col privs: %d, union hive cols: %d",
           authName,
           privWhereClause.data(),
           hasGrants, (hasHive && hasGrants));
  QRLogger::log(CAT_SQL_EXE, LL_DEBUG, "%s", msg);

  // Attach union with column privileges clause
  if (hasGrants)
  {
    const QueryString * grants = getPrivsForColsQuery;
    Int32 sizeOfGrants = sizeof(getPrivsForColsQuery);
    Int32 qryArraySize = sizeOfGrants / sizeof(QueryString);
    char * gluedQuery;
    Int32 gluedQuerySize;

    glueQueryFragments(qryArraySize, grants, gluedQuery, gluedQuerySize);
    char buf[strlen(gluedQuery) + privWhereClause.length() + MAX_SQL_IDENTIFIER_NAME_LEN*6 + 200];
    snprintf(buf, sizeof(buf), gluedQuery,
             cat, SEABASE_PRIVMGR_SCHEMA, PRIVMGR_COLUMN_PRIVILEGES,
             cat, SEABASE_MD_SCHEMA, SEABASE_COLUMNS,
             privWhereClause.data());
    colPrivsStmt = buf;
    NADELETEBASIC(gluedQuery, getMyHeap());
    if (hasHive)
    {
      // attach union with hivemd columns clause
      const QueryString * hive = getPrivsForHiveColsQuery;
      Int32 sizeOfHive = sizeof(getPrivsForHiveColsQuery);
      qryArraySize = sizeOfHive / sizeof(QueryString);
      glueQueryFragments(qryArraySize, hive, gluedQuery, gluedQuerySize);
      snprintf(buf, sizeof(buf), gluedQuery,
               cat, SEABASE_PRIVMGR_SCHEMA, PRIVMGR_COLUMN_PRIVILEGES,
               cat, SEABASE_MD_SCHEMA, SEABASE_OBJECTS,
               privWhereClause.data());
      colPrivsStmt += buf;
      NADELETEBASIC(gluedQuery, getMyHeap());
    }
  }
  return 0;
}

//////////////////////////////////////////////////////
// work() for ExExeUtilGetMetadataInfoTcb
//////////////////////////////////////////////////////
short ExExeUtilGetMetadataInfoTcb::work()
{
  short retcode = 0;
  Lng32 cliRC = 0;
  ex_expr::exp_return_type exprRetCode = ex_expr::EXPR_OK;


  // if no parent request, return
  if (qparent_.down->isEmpty())
    return WORK_OK;

  // if no room in up queue, won't be able to return data/status.
  // Come back later.
  if (qparent_.up->isFull())
    return WORK_OK;

  ex_queue_entry * pentry_down = qparent_.down->getHeadEntry();
  ExExeUtilPrivateState & pstate =
    *((ExExeUtilPrivateState*) pentry_down->pstate);

  // Get the globals stucture of the master executor.
  ExExeStmtGlobals *exeGlob = getGlobals()->castToExExeStmtGlobals();
  ExMasterStmtGlobals *masterGlob = exeGlob->castToExMasterStmtGlobals();
  ContextCli * currContext = masterGlob->getStatement()->getContext();

  while (1)
    {
      switch (step_)
	{
	case INITIAL_:
	  {
	    step_ = DISABLE_CQS_;

	    headingReturned_ = FALSE;

	    numOutputEntries_ = 1;
            returnRowCount_ = 0 ;

	    objectUid_[0] = 0;
	  }
	break;

	case DISABLE_CQS_:
	  {
	    if (disableCQS())
	      {
		step_ = HANDLE_ERROR_;
		break;
	      }

            // assume for now that everything is an HBase object

            // In the future, we may wish to check the TDB to see
            // what kind of object is being queried, and pick the
            // relevant step as a result. One thing that should be
            // kept in mind though is that we want nice semantics
            // when the object doesn't exist. Today, when a catalog
            // does not exist, for example, GET SCHEMAS simply
            // returns nothing. Similarly, when a catalog exists
            // but the schema does not, GET TABLES returns nothing.

	    step_ = SETUP_HBASE_QUERY_;
	  }
	break;

	case SETUP_HBASE_QUERY_:
	  {
	    const QueryString * qs = NULL;
	    Int32 sizeOfqs = 0;
            NAString userQuery;

	    char ausStr[1000];
            ausStr[0] = '\0';
	    char catSchValue[ComMAX_2_PART_EXTERNAL_UTF8_NAME_LEN_IN_BYTES+50];
            catSchValue[0] = '\0';
	    char endQuote[10];
            endQuote[0] = '\0';

	    if (getMItdb().returnFullyQualNames())
	      {
		str_sprintf(catSchValue, "'\"%s\".\"%s\".\"' || ",
			    getMItdb().getCat(), getMItdb().getSch());

		str_sprintf(endQuote, "|| '\"' ");
	      }

	    char cat[100];
	    char sch[100];
            char pmsch[100];
	    char tab[100];
	    char col[100];
            char indexes[100];
	    char view[100];
	    char view_usage[100];
            char auths[100];
            char role_usage[100];
            char objPrivs[100];
            char schPrivs[100];
            char colPrivs[100];
            char components[100];
            char componentOperations[100];
            char componentPrivileges[100];
            char routine[100];
            char library_usage[100];
            char hiveObjType[100];
            char hiveGetType[10];
            char hiveSysCat[10];

	    if(cliInterface()->getCQDval("SEABASE_CATALOG", cat) < 0)
	        strcpy(cat, TRAFODION_SYSCAT_LIT);
	    strcpy(sch, SEABASE_MD_SCHEMA);
	    strcpy(pmsch, SEABASE_PRIVMGR_SCHEMA);
	    strcpy(tab, SEABASE_OBJECTS);
	    strcpy(col, SEABASE_COLUMNS);
	    strcpy(view, SEABASE_VIEWS);
	    strcpy(view_usage, SEABASE_VIEWS_USAGE);
            strcpy(indexes, SEABASE_INDEXES);
            strcpy(auths, SEABASE_AUTHS);
            strcpy(objPrivs, "OBJECT_PRIVILEGES");
            strcpy(colPrivs, "COLUMN_PRIVILEGES");
            strcpy(schPrivs, "SCHEMA_PRIVILEGES");
            strcpy(role_usage, "ROLE_USAGE");
            strcpy(components, "COMPONENTS");
            strcpy(componentOperations, "COMPONENT_OPERATIONS");
            strcpy(componentPrivileges, "COMPONENT_PRIVILEGES");
            strcpy(routine, SEABASE_ROUTINES);
            strcpy(library_usage, SEABASE_LIBRARIES_USAGE);
            strcpy(hiveSysCat, HIVE_SYSTEM_CATALOG_LC);

            // Determine if need to restrict data to user visable data only.
            NABoolean doPrivCheck = checkUserPrivs(currContext, getMItdb().queryType_);  
            NAString privWhereClause;

            // get active roles for current user and put in a list that can be
            // used in a select "IN" clause.  Include the current user
            NAString authList;
            NAString colPrivsStmt;
            NAString var;

            if (CmpCommon::context()->isAuthorizationEnabled())
            {
               // always include the current user in the list of auth IDs
               char authIDAsChar[sizeof(Int32)+10];
               str_sprintf(authIDAsChar, "(%d", *currContext->getDatabaseUserID());
               authList += authIDAsChar;

               // add list of roles stored in context
               Int32 numRoles;
               Int32 *roleList;
               Int32 *granteeList;
               if (currContext->getRoleList(numRoles, roleList, granteeList) != SUCCESS)
                 numRoles = 0;
               for (Int32 i = 0; i < numRoles; i++)
               {
                 authList += ", ";
                 str_sprintf(authIDAsChar, "%d", roleList[i]);
                 authList += authIDAsChar;
               } 
               authList += ")";
            }

            // If request to get privilege information but authorization tables were not initialized,
	    if(getMItdb().queryType_ == ComTdbExeUtilGetMetadataInfo::COMPONENTS_
	      ||getMItdb().queryType_ == ComTdbExeUtilGetMetadataInfo::COMPONENT_OPERATIONS_
	      ||getMItdb().queryType_ == ComTdbExeUtilGetMetadataInfo::COMPONENT_PRIVILEGES_
              ||getMItdb().queryType_ == ComTdbExeUtilGetMetadataInfo::FUNCTIONS_FOR_USER_
              ||getMItdb().queryType_ == ComTdbExeUtilGetMetadataInfo::FUNCTIONS_FOR_ROLE_
              ||getMItdb().queryType_ == ComTdbExeUtilGetMetadataInfo::INDEXES_FOR_USER_
              ||getMItdb().queryType_ == ComTdbExeUtilGetMetadataInfo::INDEXES_FOR_ROLE_
              ||getMItdb().queryType_ == ComTdbExeUtilGetMetadataInfo::LIBRARIES_FOR_USER_
              ||getMItdb().queryType_ == ComTdbExeUtilGetMetadataInfo::LIBRARIES_FOR_ROLE_
              ||getMItdb().queryType_ == ComTdbExeUtilGetMetadataInfo::ROLES_FOR_USER_
              ||getMItdb().queryType_ == ComTdbExeUtilGetMetadataInfo::PROCEDURES_FOR_USER_
              ||getMItdb().queryType_ == ComTdbExeUtilGetMetadataInfo::PROCEDURES_FOR_ROLE_
              ||getMItdb().queryType_ == ComTdbExeUtilGetMetadataInfo::TABLE_FUNCTIONS_FOR_USER_
              ||getMItdb().queryType_ == ComTdbExeUtilGetMetadataInfo::TABLE_FUNCTIONS_FOR_ROLE_
              ||getMItdb().queryType_ == ComTdbExeUtilGetMetadataInfo::TABLES_FOR_USER_
              ||getMItdb().queryType_ == ComTdbExeUtilGetMetadataInfo::TABLES_FOR_ROLE_
              ||getMItdb().queryType_ == ComTdbExeUtilGetMetadataInfo::VIEWS_FOR_USER_
              ||getMItdb().queryType_ == ComTdbExeUtilGetMetadataInfo::VIEWS_FOR_ROLE_
              ||getMItdb().queryType_ == ComTdbExeUtilGetMetadataInfo::USERS_FOR_ROLE_
              ||getMItdb().queryType_ == ComTdbExeUtilGetMetadataInfo::PRIVILEGES_FOR_ROLE_
              ||getMItdb().queryType_ == ComTdbExeUtilGetMetadataInfo::PRIVILEGES_FOR_USER_
              ||getMItdb().queryType_ == ComTdbExeUtilGetMetadataInfo::PRIVILEGES_ON_TABLE_
              ||getMItdb().queryType_ == ComTdbExeUtilGetMetadataInfo::PRIVILEGES_ON_SEQUENCE_
              ||getMItdb().queryType_ == ComTdbExeUtilGetMetadataInfo::PRIVILEGES_ON_LIBRARY_
              ||getMItdb().queryType_ == ComTdbExeUtilGetMetadataInfo::PRIVILEGES_ON_ROUTINE_)
	    {
               if (!CmpCommon::context()->isAuthorizationEnabled())
               {
                  ExRaiseSqlError(getHeap(), &diagsArea_, -CAT_AUTHORIZATION_NOT_ENABLED);
                  step_ = HANDLE_ERROR_;
                  break;
               }
            }

	    switch (getMItdb().queryType_)
	      {
              case ComTdbExeUtilGetMetadataInfo::CATALOGS_:
                {
                  // any user can get list of catalogs, no priv checks required
                  qs = getCatalogsQuery;
                  sizeOfqs = sizeof(getCatalogsQuery);
                }
              break;

	      case ComTdbExeUtilGetMetadataInfo::TABLES_IN_SCHEMA_:
		{
		  qs = getTrafTablesInSchemaQuery;
		  sizeOfqs = sizeof(getTrafTablesInSchemaQuery);

                  if (doPrivCheck)
                    privWhereClause = getGrantedPrivCmd(authList, cat);
                  param_[0] = catSchValue;
                  param_[1] = endQuote;
		  param_[2] = cat;
		  param_[3] = sch;
		  param_[4] = tab;
		  param_[5] = getMItdb().cat_;
		  param_[6] = getMItdb().sch_;
                  param_[7] = (char *)privWhereClause.data();
		}
	      break;
	      
	      case ComTdbExeUtilGetMetadataInfo::INDEXES_IN_SCHEMA_:
		{
		  qs = getTrafIndexesInSchemaQuery;
		  sizeOfqs = sizeof(getTrafIndexesInSchemaQuery);

                  if (doPrivCheck)
                    privWhereClause = getGrantedPrivCmd(authList, cat);

		  param_[0] = cat;
		  param_[1] = sch;
		  param_[2] = tab;
		  param_[3] = getMItdb().cat_;
		  param_[4] = getMItdb().sch_;
                  param_[5] = (char *)privWhereClause.data();
		}
	      break;
	      
	      case ComTdbExeUtilGetMetadataInfo::VIEWS_IN_CATALOG_:
		{
		  qs = getTrafViewsInCatalogQuery;
		  sizeOfqs = sizeof(getTrafViewsInCatalogQuery);

                 if (doPrivCheck)
                    privWhereClause = getGrantedPrivCmd(authList, cat);

		  param_[0] = cat;
		  param_[1] = sch;
		  param_[2] = tab;
		  param_[3] = cat;
		  param_[4] = sch;
		  param_[5] = view;
		  param_[6] = getMItdb().cat_;
                  param_[7] = (char *)privWhereClause.data();
		}
	      break;
	      
              case ComTdbExeUtilGetMetadataInfo::HIVE_REG_TABLES_IN_CATALOG_:
              case ComTdbExeUtilGetMetadataInfo::HIVE_REG_VIEWS_IN_CATALOG_:
              case ComTdbExeUtilGetMetadataInfo::HIVE_REG_SCHEMAS_IN_CATALOG_:
              case ComTdbExeUtilGetMetadataInfo::HIVE_REG_OBJECTS_IN_CATALOG_:
              {
                qs = getHiveRegObjectsInCatalogQuery;
                sizeOfqs = sizeof(getHiveRegObjectsInCatalogQuery);

                if (getMItdb().queryType_ == ComTdbExeUtilGetMetadataInfo::HIVE_REG_TABLES_IN_CATALOG_)
                {
                   strcpy(hiveGetType, "tables");
                   str_sprintf(hiveObjType, " (object_type = '%s') ",
                                COM_BASE_TABLE_OBJECT_LIT);
                    }
                  else if (getMItdb().queryType_ == ComTdbExeUtilGetMetadataInfo::HIVE_REG_VIEWS_IN_CATALOG_)
                   {
                      strcpy(hiveGetType, "views");
                      str_sprintf(hiveObjType, " (object_type = '%s') ",
                                  COM_VIEW_OBJECT_LIT);
                    }
                  else if (getMItdb().queryType_ == ComTdbExeUtilGetMetadataInfo::HIVE_REG_SCHEMAS_IN_CATALOG_)
                    {
                      strcpy(hiveGetType, "schemas");
                      str_sprintf(hiveObjType, " (object_type = '%s') ",
                                  COM_SHARED_SCHEMA_OBJECT_LIT);
                    }
                  else
                    {
                      strcpy(hiveGetType, "objects");
                      str_sprintf(hiveObjType, " (object_type = '%s' or object_type = '%s' or object_type = '%s' ) ",
                                  COM_BASE_TABLE_OBJECT_LIT, 
                                  COM_VIEW_OBJECT_LIT,
                                  COM_SHARED_SCHEMA_OBJECT_LIT);
                    }
                    
                  if (doPrivCheck)
                    privWhereClause = getGrantedPrivCmd(authList, cat);

                param_[0] = cat;
                param_[1] = sch;
                param_[2] = tab;
                param_[3] = hiveObjType;
                param_[4] = (char *)privWhereClause.data();
                param_[5] = hiveSysCat;
                param_[6] = hiveGetType, 
                param_[7] = hiveSysCat;
		}
	      break;

	     case ComTdbExeUtilGetMetadataInfo::HBASE_REG_TABLES_IN_CATALOG_:
               {
                 qs = getHBaseRegTablesInCatalogQuery;
                 sizeOfqs = sizeof(getHBaseRegTablesInCatalogQuery);

                 if (doPrivCheck)
                   privWhereClause = getGrantedPrivCmd(authList, cat);

                 param_[0] = cat;
                 param_[1] = sch;
                 param_[2] = tab;
                 param_[3] = (char *)privWhereClause.data();
               }
	      break;
	      
              case ComTdbExeUtilGetMetadataInfo::HIVE_EXT_TABLES_IN_CATALOG_:
                {
                  qs = getHiveExtTablesInCatalogQuery;
                  sizeOfqs = sizeof(getHiveExtTablesInCatalogQuery);

                  if (doPrivCheck)
                    privWhereClause = getGrantedPrivCmd(authList, cat);

                  strcpy(hiveObjType, COM_BASE_TABLE_OBJECT_LIT);
                  strcpy(hiveGetType, "tables");

                  param_[0] = hiveSysCat;
                  param_[1] = cat;
                  param_[2] = sch;
                  param_[3] = tab;
                  param_[4] = hiveObjType;
                  param_[5] = (char *)privWhereClause.data();
                  param_[6] = hiveSysCat;
                  param_[7] = hiveGetType, 
                  param_[8] = hiveSysCat;
                }
              break;
	      
	      case ComTdbExeUtilGetMetadataInfo::VIEWS_IN_SCHEMA_:
		{
		  qs = getTrafViewsInSchemaQuery;
		  sizeOfqs = sizeof(getTrafViewsInSchemaQuery);

                 if (doPrivCheck)
                    privWhereClause = getGrantedPrivCmd(authList, cat);

		  param_[0] = cat;
		  param_[1] = sch;
		  param_[2] = tab;
		  param_[3] = cat;
		  param_[4] = sch;
		  param_[5] = view;
		  param_[6] = getMItdb().cat_;
		  param_[7] = getMItdb().sch_;
                  param_[8] = (char *)privWhereClause.data();
		}
	      break;

	      case ComTdbExeUtilGetMetadataInfo::TABLES_IN_VIEW_:
	      case ComTdbExeUtilGetMetadataInfo::VIEWS_IN_VIEW_:
	      case ComTdbExeUtilGetMetadataInfo::OBJECTS_IN_VIEW_:
		{
                  qs = getTrafObjectsInViewQuery;
                  sizeOfqs = sizeof(getTrafObjectsInViewQuery);

                 // If user has privs on the view, they can see referenced objects
                 // even if they don't have privileges on the referenced objects
                 if (doPrivCheck)
                    privWhereClause = getGrantedPrivCmd(authList, cat, NAString("T2.object_uid"));

                  param_[0] = cat;
                  param_[1] = sch;
                  param_[2] = view_usage;
                  param_[3] = cat;
                  param_[4] = sch;
                  param_[5] = tab;
                  param_[6] = cat;
                  param_[7] = sch;
                  param_[8] = tab;
                  param_[9] = getMItdb().cat_;
                  param_[10] = getMItdb().sch_;
                  param_[11] = getMItdb().obj_;
                  param_[12] = (char *)privWhereClause.data();

                  numOutputEntries_ = 2;
		}
	      break;

              case ComTdbExeUtilGetMetadataInfo::INDEXES_ON_TABLE_:
                {
                  qs = getTrafIndexesOnTableQuery;
                  sizeOfqs = sizeof(getTrafIndexesOnTableQuery);
                 if (doPrivCheck)
                    privWhereClause = getGrantedPrivCmd(authList, cat, NAString("O.object_uid"));

                  param_[0] = catSchValue;
                  param_[1] = endQuote;
                  param_[2] = cat;
                  param_[3] = sch;
                  param_[4] = indexes;
                  param_[5] = cat;
                  param_[6] = sch;
                  param_[7] = tab;
                  param_[8] = cat;
                  param_[9] = sch;
                  param_[10] = tab;
                  param_[11] = getMItdb().cat_;
                  param_[12] = getMItdb().sch_;
                  param_[13] = getMItdb().obj_;
                  param_[14] = (char *)privWhereClause.data();

                }
                break;

	      case ComTdbExeUtilGetMetadataInfo::VIEWS_ON_TABLE_:
	      case ComTdbExeUtilGetMetadataInfo::VIEWS_ON_VIEW_:
		{
                  qs = getTrafViewsOnObjectQuery;
                  sizeOfqs = sizeof(getTrafViewsOnObjectQuery);

                  // If user has privs on object, they can see referencing views
                  // even if they don't have privileges on the referencing views
                  if (doPrivCheck)
                    privWhereClause = getGrantedPrivCmd(authList, cat, NAString("T1.object_uid"));

                  param_[0] = cat;
                  param_[1] = sch;
                  param_[2] = tab;
                  param_[3] = cat;
                  param_[4] = sch;
                  param_[5] = view_usage;
                  param_[6] = cat;
                  param_[7] = sch;
                  param_[8] = tab;
                  param_[9] = getMItdb().cat_;
                  param_[10] = getMItdb().sch_;
                  param_[11] = getMItdb().obj_;
                  if (getMItdb().queryType_ == ComTdbExeUtilGetMetadataInfo::VIEWS_ON_TABLE_)
                    var = " and T1.object_type = 'BT' ";
                  param_[12] = (char *)var.data();
                  param_[13] = (char *)privWhereClause.data();

		}
	      break;

	      case ComTdbExeUtilGetMetadataInfo::SCHEMAS_IN_CATALOG_:
		{
		  qs = getTrafSchemasInCatalogQuery;
		  sizeOfqs = sizeof(getTrafSchemasInCatalogQuery);

                  if (doPrivCheck)
                    privWhereClause = getGrantedPrivCmd(authList, cat);

		  param_[0] = cat;
		  param_[1] = sch;
		  param_[2] = tab;
		  param_[3] = getMItdb().cat_;
                  param_[4] = (char *) privWhereClause.data();
		}
	      break;

              case ComTdbExeUtilGetMetadataInfo::SCHEMAS_FOR_USER_:
              case ComTdbExeUtilGetMetadataInfo::SCHEMAS_FOR_ROLE_:
                {
                  qs = getTrafSchemasForAuthIDQuery;
                  sizeOfqs = sizeof(getTrafSchemasForAuthIDQuery);

                  bool isRole = (getMItdb().queryType_ == ComTdbExeUtilGetMetadataInfo::SCHEMAS_FOR_ROLE_);

                  Int32 authID = *currContext->getDatabaseUserID();
                  if (!(strcmp(getMItdb().getParam1(), currContext->getDatabaseUserName()) == 0))
                    authID = getAuthID(getMItdb().getParam1(), cat, sch, auths);

                  if (isRole)
                    {
                      // if incorrect auth type, return error
                      if (!CmpSeabaseDDLauth::isRoleID(authID) && !ComUser::isPublicUserID(authID))
                        {
                          ExRaiseSqlError(getHeap(), &diagsArea_, -CAT_IS_NOT_A_ROLE, 
                              NULL, NULL, NULL,
                              getMItdb().getParam1());
                          step_ = HANDLE_ERROR_;
                          break;
                        }

                      // Cannot get schemas if current user not granted role
                      if (doPrivCheck && !ComUser::currentUserHasRole(authID))
                        {
                          ExRaiseSqlError(getHeap(), &diagsArea_, -CAT_NOT_AUTHORIZED);
                          step_ = HANDLE_ERROR_;
                          break;
                        }

                        // Return schemas that are owned by the specified role -> authID = roleID
                        char buf[30];
                        str_sprintf(buf, "(%d)", authID);
                        privWhereClause = buf;
                    }

                  else /* isUser*/ 
                    {
                      // if incorrect auth type, return error
                      if (!CmpSeabaseDDLauth::isUserID(authID))
                      {
                        ExRaiseSqlError(getHeap(), &diagsArea_, -CAT_IS_NOT_A_USER, 
                          NULL, NULL, NULL,
                          getMItdb().getParam1());
                        step_ = HANDLE_ERROR_;
                        break;
                      }

                      // Cannot get schemas for user other than the current user
                      if (doPrivCheck && authID != ComUser::getCurrentUser())
                        {
                          ExRaiseSqlError(getHeap(), &diagsArea_, -CAT_NOT_AUTHORIZED);
                          step_ = HANDLE_ERROR_;
                          break;
                        }
                    
                      // Get list of roles assigned to user, return all schemas
                      // owned by user and user's roles

                      if (authID == ComUser::getCurrentUser())
                        privWhereClause = authList;
                      else
                        {
                          char *userRoleList = getRoleList(authID, cat, pmsch, role_usage);
                          if (userRoleList)
                            {
                              privWhereClause = userRoleList;
                              NADELETEBASIC(userRoleList, getHeap());
                            }
                          else
                            {
                              // Unable to read metadata 
                              ExRaiseSqlError(getHeap(), &diagsArea_, -8001);
                              step_ = HANDLE_ERROR_;
                              break;
                            }
                        }
                    }

                  param_[0] = cat;
                  param_[1] = sch;
                  param_[2] = tab;
                  param_[3] = cat;
                  param_[4] = sch;
                  param_[5] = auths;
                  param_[6] = (char *) privWhereClause.data();
		}
	      break;

              case ComTdbExeUtilGetMetadataInfo::USERS_:
                {
                  qs = getTrafUsers;
                  sizeOfqs = sizeof(getTrafUsers);

                  if (doPrivCheck)
                  {
                     char buf[authList.length() + 100];
                     str_sprintf(buf, " and auth_id in %s", authList.data());
                     privWhereClause = buf;
                  }

                  param_[0] = cat;
                  param_[1] = sch;
                  param_[2] = auths;
                  param_[3] = (char *) privWhereClause.data();
		}
                break;
              case ComTdbExeUtilGetMetadataInfo::PROCEDURES_IN_SCHEMA_:
                {
                  qs = getTrafProceduresInSchemaQuery;
                  sizeOfqs = sizeof(getTrafProceduresInSchemaQuery);

                  if (doPrivCheck)
                    privWhereClause = getGrantedPrivCmd(authList, cat);

		  param_[0] = cat;
		  param_[1] = sch;
		  param_[2] = tab;
                  param_[3] = cat;
		  param_[4] = sch;
		  param_[5] = routine;
		  param_[6] = getMItdb().cat_;
		  param_[7] = getMItdb().sch_;
                  param_[8] = (char *) privWhereClause.data();
                }
                break ;

              case ComTdbExeUtilGetMetadataInfo::LIBRARIES_IN_SCHEMA_:
                {
                  qs = getTrafLibrariesInSchemaQuery;
                  sizeOfqs = sizeof(getTrafLibrariesInSchemaQuery);

                  if (doPrivCheck)
                  {
                    privWhereClause = "and T.object_uid = R.library_uid "; 
                    privWhereClause += getGrantedPrivCmd(authList, cat, NAString("R.udr_uid"));
                  }

                  param_[0] = cat;
                  param_[1] = sch;
                  param_[2] = tab;
                  param_[3] = cat;
                  param_[4] = sch;
                  param_[5] = routine;
                  param_[6] = getMItdb().cat_;
                  param_[7] = getMItdb().sch_;
                  param_[8] = (char *) privWhereClause.data();
                }
                break ;

              case ComTdbExeUtilGetMetadataInfo::FUNCTIONS_IN_SCHEMA_:
                {
                  qs = getTrafFunctionsInSchemaQuery;
                  sizeOfqs = sizeof(getTrafFunctionsInSchemaQuery);

                  if (doPrivCheck)
                    privWhereClause = getGrantedPrivCmd(authList, cat);

		  param_[0] = cat;
		  param_[1] = sch;
		  param_[2] = tab;
                  param_[3] = cat;
		  param_[4] = sch;
		  param_[5] = routine;
		  param_[6] = getMItdb().cat_;
		  param_[7] = getMItdb().sch_;
                  param_[8] = (char *) privWhereClause.data();
                }
                break ;

	      case ComTdbExeUtilGetMetadataInfo::TABLE_FUNCTIONS_IN_SCHEMA_:
                {
                  qs = getTrafTableFunctionsInSchemaQuery;
                  sizeOfqs = sizeof(getTrafTableFunctionsInSchemaQuery);

                  if (doPrivCheck)
                    privWhereClause = getGrantedPrivCmd(authList, cat);

		  param_[0] = cat;
		  param_[1] = sch;
		  param_[2] = tab;
                  param_[3] = cat;
		  param_[4] = sch;
		  param_[5] = routine;
		  param_[6] = getMItdb().cat_;
		  param_[7] = getMItdb().sch_;
                  param_[8] = (char *) privWhereClause.data();
                }
                break ;

              case ComTdbExeUtilGetMetadataInfo::PROCEDURES_FOR_USER_:
              case ComTdbExeUtilGetMetadataInfo::FUNCTIONS_FOR_USER_:
              case ComTdbExeUtilGetMetadataInfo::TABLE_FUNCTIONS_FOR_USER_:
                {
                  qs = getTrafRoutinesForAuthQuery;
                  sizeOfqs = sizeof(getTrafRoutinesForAuthQuery);

                  // Get the authID associated with the specified user
                  Int32 authID = *currContext->getDatabaseUserID();
                  if (!(strcmp(getMItdb().getParam1(), currContext->getDatabaseUserName()) == 0))
                    authID = getAuthID(getMItdb().getParam1(), cat, sch, auths);

                  // If not a user, we are done, don't return data
                  if (!CmpSeabaseDDLauth::isUserID(authID))
                    {
                      ExRaiseSqlError(getHeap(), &diagsArea_, -CAT_IS_NOT_A_USER);
                      step_ = HANDLE_ERROR_;
                      break;
                    }

                  // Non elevated user cannot view routines for another user
                  if (doPrivCheck && authID != ComUser::getCurrentUser())
                    {
                      ExRaiseSqlError(getHeap(), &diagsArea_, -CAT_NOT_AUTHORIZED);
                      step_ = HANDLE_ERROR_;
                      break;
                    }
                  
                  // Determine routine type
                  if (getMItdb().queryType_ == ComTdbExeUtilGetMetadataInfo::PROCEDURES_FOR_USER_)
                    var = COM_PROCEDURE_TYPE_LIT;
                  else if (getMItdb().queryType_ == ComTdbExeUtilGetMetadataInfo::FUNCTIONS_FOR_USER_)
                    var = COM_SCALAR_UDF_TYPE_LIT;
                  else
                    var = COM_TABLE_UDF_TYPE_LIT;

                  // Limit results to privileges allowed for specified user

                  if (authID == ComUser::getCurrentUser())
                    privWhereClause = getGrantedPrivCmd(authList, cat);
                  else
                    {
                      char *userRoleList = getRoleList(authID, cat, pmsch, role_usage);
                      if (userRoleList)
                        {
                          privWhereClause = getGrantedPrivCmd(userRoleList, cat, NAString ("T.object_uid"));
                          NADELETEBASIC(userRoleList, getHeap());
                        }
                      else
                        {
                          // Unable to read metadata
                          ExRaiseSqlError(getHeap(), &diagsArea_, -8001);
                          step_ = HANDLE_ERROR_;
                          break;
                        }
                    }

                  param_[0] = cat;
                  param_[1] = sch;
                  param_[2] = tab;
                  param_[3] = cat;
                  param_[4] = sch;
                  param_[5] = routine;
                  param_[6] = getMItdb().cat_;
                  param_[7] = (char *)var.data();
                  param_[8] = (char *) privWhereClause.data();
                }
                break ;

              case ComTdbExeUtilGetMetadataInfo::PROCEDURES_FOR_ROLE_:
              case ComTdbExeUtilGetMetadataInfo::FUNCTIONS_FOR_ROLE_:
              case ComTdbExeUtilGetMetadataInfo::TABLE_FUNCTIONS_FOR_ROLE_:
                {
                  qs = getTrafRoutinesForAuthQuery;
                  sizeOfqs = sizeof(getTrafRoutinesForAuthQuery);

                  // Get the authID associated with the specified role
                  Int32 authID = *currContext->getDatabaseUserID();
                  if (!(strcmp(getMItdb().getParam1(), currContext->getDatabaseUserName()) == 0))
                    authID = getAuthID(getMItdb().getParam1(), cat, sch, auths);

                  // If not a role, we are done, don't return data
                  if (!CmpSeabaseDDLauth::isRoleID(authID) && !ComUser::isPublicUserID(authID)) 
                    {
                      ExRaiseSqlError(getHeap(), &diagsArea_, -CAT_IS_NOT_A_ROLE, 
                          NULL, NULL, NULL,
                          getMItdb().getParam1());
                      step_ = HANDLE_ERROR_;
                      break;
                    }

                  // non elevated user has to be granted role
                  if (doPrivCheck && !ComUser::currentUserHasRole(authID))
                    {
                      // No priv
                      ExRaiseSqlError(getHeap(), &diagsArea_, -CAT_NOT_AUTHORIZED);
                      step_ = HANDLE_ERROR_;
                      break;
                    }

                  // determine routine type
                  if (getMItdb().queryType_ == ComTdbExeUtilGetMetadataInfo::PROCEDURES_FOR_ROLE_)
                    var = COM_PROCEDURE_TYPE_LIT;
                  else if (getMItdb().queryType_ == ComTdbExeUtilGetMetadataInfo::FUNCTIONS_FOR_ROLE_)
                    var = COM_SCALAR_UDF_TYPE_LIT;
                  else 
                    var = COM_TABLE_UDF_TYPE_LIT;

                  // Only return rows where role (authID) has been granted privs
                  char buf[30];
                  str_sprintf(buf, "(%d)", authID);
                  privWhereClause = getGrantedPrivCmd(buf, cat, NAString ("T.object_uid"));

                  param_[0] = cat;
                  param_[1] = sch;
                  param_[2] = tab;
                  param_[3] = cat;
                  param_[4] = sch;
                  param_[5] = routine;
                  param_[6] = getMItdb().cat_;
                  param_[7] = (char *)var.data();
                  param_[8] = (char *) privWhereClause.data();
                }
                break ;
                
              case ComTdbExeUtilGetMetadataInfo::PROCEDURES_FOR_LIBRARY_:
              case ComTdbExeUtilGetMetadataInfo::FUNCTIONS_FOR_LIBRARY_:
              case ComTdbExeUtilGetMetadataInfo::TABLE_FUNCTIONS_FOR_LIBRARY_:
                {
                  qs = getTrafProceduresForLibraryQuery;
                  sizeOfqs = sizeof(getTrafProceduresForLibraryQuery);

                  if (doPrivCheck)
                    privWhereClause = getGrantedPrivCmd(authList, cat, NAString("T1.object_uid"));

                  param_[0] = cat;
                  param_[1] = sch;
                  param_[2] = tab;
                  param_[3] = cat;
                  param_[4] = sch;
                  param_[5] = routine;
                  param_[6] = cat;
                  param_[7] = sch;
                  param_[8] = tab;
                  param_[9] = getMItdb().cat_;
                  param_[10] = getMItdb().sch_;
                  param_[11] = getMItdb().obj_;
                  if (getMItdb().queryType_ == ComTdbExeUtilGetMetadataInfo::PROCEDURES_FOR_LIBRARY_)
                    var = " R.udr_type = 'P ' ";
                  else if (getMItdb().queryType_ == ComTdbExeUtilGetMetadataInfo::FUNCTIONS_FOR_LIBRARY_)
                    var = " R.udr_type = 'F ' ";
                  else
                    var = " R.udr_type = 'T ' ";
                  param_[12] = (char *) var.data();
                  param_[13] = (char *) privWhereClause.data();
                }
                break ;
                
              case ComTdbExeUtilGetMetadataInfo::ROLES_:
                {
                  qs = getTrafRoles;

                  sizeOfqs = sizeof(getTrafRoles);

                  if (doPrivCheck)
                  {
                     // return roles granted to current user
                     char buf[authList.length() + 100];
                     str_sprintf(buf, " and auth_id in %s", authList.data());
                     privWhereClause = buf;
                  }

                  param_[0] = cat;
                  param_[1] = sch;
                  param_[2] = auths;
                  param_[3] = (char *) privWhereClause.data();
                }
              break;

              case ComTdbExeUtilGetMetadataInfo::USERS_FOR_ROLE_:
                {
                  qs = getUsersForRoleQuery;
                  sizeOfqs = sizeof(getUsersForRoleQuery);

                  if (doPrivCheck)
                    {
                      // If user not granted role, return an error
                      Int32 authID = *currContext->getDatabaseUserID();
                      if (!(strcmp(getMItdb().getParam1(), currContext->getDatabaseUserName()) == 0))
                        authID = getAuthID(getMItdb().getParam1(), cat, sch, auths);
                      if (!ComUser::currentUserHasRole(authID))
                        {
                           ExRaiseSqlError(getHeap(), &diagsArea_, -CAT_NOT_AUTHORIZED);
                           step_ = HANDLE_ERROR_;
                           break;
                         }

                       // limit users to the current user only
                       privWhereClause = " and grantee_name = CURRENT_USER ";
                     }

                  param_[0] = cat;
                  param_[1] = pmsch;
                  param_[2] = role_usage;
                  param_[3] = getMItdb().getParam1();
                  param_[4] = (char *) privWhereClause.data();
                }
              break;
                
              case ComTdbExeUtilGetMetadataInfo::ROLES_FOR_USER_:
                {
                  qs = getRolesForUserQuery;
                  sizeOfqs = sizeof(getRolesForUserQuery);
                  
                  if (doPrivCheck)  
                    {
                      // If user not the current user, return an error
                      // TBD - the current context contains a list of roles, 
                      //       return list to avoid metadata I/O
                      if (strcmp(getMItdb().getParam1(), currContext->getDatabaseUserName()) != 0)
                       {
                          ExRaiseSqlError(getHeap(), &diagsArea_, -CAT_NOT_AUTHORIZED);
                          step_ = HANDLE_ERROR_;
                          break;
                        } 
                    }
                  else
                    {
                      // Get the authID for the request
                      Int32 authID = *currContext->getDatabaseUserID();
                      if (!(strcmp(getMItdb().getParam1(), currContext->getDatabaseUserName()) == 0))
                        authID = getAuthID(getMItdb().getParam1(), cat, sch, auths);
                      if (!CmpSeabaseDDLauth::isUserID(authID))
                        {
                          ExRaiseSqlError(getHeap(), &diagsArea_, -CAT_IS_NOT_A_USER,
                              NULL, NULL, NULL,
                              getMItdb().getParam1());
                          step_ = HANDLE_ERROR_;
                          break;
                        }
                    }

                  param_[0] = cat;
                  param_[1] = pmsch;
                  param_[2] = role_usage;
                  param_[3] = getMItdb().getParam1();
                }
              break;
              
              case ComTdbExeUtilGetMetadataInfo::PRIVILEGES_FOR_ROLE_:
                {
                  // Get the authID for the request
                  Int32 authID = *currContext->getDatabaseUserID();
                  if (!(strcmp(getMItdb().getParam1(), currContext->getDatabaseUserName()) == 0))
                    authID = getAuthID(getMItdb().getParam1(), cat, sch, auths);

                  char buf[authList.length() + 100];

                  // Verify that requested authID is actually a role
                  if (!CmpSeabaseDDLauth::isRoleID(authID) && !ComUser::isPublicUserID(authID))
                    {
                      ExRaiseSqlError(getHeap(), &diagsArea_, -CAT_IS_NOT_A_ROLE,
                          NULL, NULL, NULL,
                          getMItdb().getParam1());
                      step_ = HANDLE_ERROR_;
                      break;
                    }

                  // Non elevated users need to be granted role
                  if (doPrivCheck && !ComUser::currentUserHasRole(authID))
                    {
                      ExRaiseSqlError(getHeap(), &diagsArea_, -CAT_NOT_AUTHORIZED);
                      step_ = HANDLE_ERROR_;
                      break;
                    }

                  // return all privs for the role
                  str_sprintf(buf, " = %d ", authID);
                  privWhereClause = buf;

                  qs = getPrivsForAuthsQuery;
                  sizeOfqs = sizeof(getPrivsForAuthsQuery);

                  // This request performs a union between 4 entities:
                  //  1. object_privileges table
                  //  2. schema_privileges table
                  //  3. column privileges table
                  //  4. hive metadata tables to retrieve column details
                  // The call to colPrivsFrag returns the required the union 
                  // statement(s) for items 3 and 4. See colPrivsFrag for details
                  if (colPrivsFrag(getMItdb().getParam1(), cat, privWhereClause, colPrivsStmt) < 0)
                  {
                    step_ = HANDLE_ERROR_;
                    break;
                  }

                  // Union privileges between object, column and schema
                  // object privs
                  param_[0] = cat;
                  param_[1] = pmsch;
                  param_[2] = objPrivs;
                  param_[3] = (char *) privWhereClause.data();

                  // schema privs
                  param_[4] = cat;
                  param_[5] = pmsch;
                  param_[6] = schPrivs;
                  param_[7] = (char *) privWhereClause.data();

                  // column privs
                  param_[8] = (char *) colPrivsStmt.data();

                  numOutputEntries_ = 2;
                }
              break;

              case ComTdbExeUtilGetMetadataInfo::PRIVILEGES_FOR_USER_:
                {
                  // Get the authID for the request
                  Int32 authID = *currContext->getDatabaseUserID();
                  if (!(strcmp(getMItdb().getParam1(), currContext->getDatabaseUserName()) == 0))
                    authID = getAuthID(getMItdb().getParam1(), cat, sch, auths);

                  // Verify that authID is a user
                  if (!CmpSeabaseDDLauth::isUserID(authID))
                    {
                      ExRaiseSqlError(getHeap(), &diagsArea_, -CAT_IS_NOT_A_USER,
                          NULL, NULL, NULL,
                          getMItdb().getParam1());
                      step_ = HANDLE_ERROR_;
                      break;
                    }

                  // Non elevated user cannot get privileges for another user
                  char buf[authList.length() + 100];
                  if (doPrivCheck) 
                    {
                      if (authID != ComUser::getCurrentUser())
                        {
                          ExRaiseSqlError(getHeap(), &diagsArea_, -CAT_NOT_AUTHORIZED);
                          step_ = HANDLE_ERROR_;
                          break;
                        }

                      // return privs for the current user and their roles
                      str_sprintf(buf, " in %s ", authList.data());
                    }

                  else
                    {
                      // Get role list for requested user
                      char *userRoleList = getRoleList(authID, cat, pmsch, role_usage);
                      if (userRoleList)
                      {
                        str_sprintf(buf, " in %s ", userRoleList);
                        NADELETEBASIC(userRoleList, getHeap());
                      }
                      else
                        str_sprintf(buf, " = %d ", authID);
                    }
                  privWhereClause = buf;

                  qs = getPrivsForAuthsQuery;
                  sizeOfqs = sizeof(getPrivsForAuthsQuery);

                  // This request performs a union between 4 entities:
                  //  1. object_privileges table
                  //  2. schema_privileges table
                  //  3. column privileges table
                  //  4. hive metadata tables to retrieve column details
                  // The call to colPrivsFrag returns the required the union 
                  // statement(s) for items 3 and 4. See colPrivsFrag for details
                  if (colPrivsFrag(getMItdb().getParam1(), cat, privWhereClause, colPrivsStmt) < 0)
                  {
                    step_ = HANDLE_ERROR_;
                    break;
                  }

                  // Union privileges between object, column and schema
                  // object privs
                  param_[0] = cat;
                  param_[1] = pmsch;
                  param_[2] = objPrivs;
                  param_[3] = (char *) privWhereClause.data();

                  // schema privs
                  param_[4] = cat;
                  param_[5] = pmsch;
                  param_[6] = schPrivs;
                  param_[7] = (char *) privWhereClause.data();

                  // column privs
                  param_[8] = (char *) colPrivsStmt.data();

                  numOutputEntries_ = 2;
                }
              break;



              case ComTdbExeUtilGetMetadataInfo::PRIVILEGES_ON_TABLE_:
              case ComTdbExeUtilGetMetadataInfo::PRIVILEGES_ON_VIEW_:
              case ComTdbExeUtilGetMetadataInfo::PRIVILEGES_ON_SEQUENCE_:
              case ComTdbExeUtilGetMetadataInfo::PRIVILEGES_ON_LIBRARY_:
              case ComTdbExeUtilGetMetadataInfo::PRIVILEGES_ON_ROUTINE_:
              {
                qs = getTrafPrivsOnObject;
                sizeOfqs = sizeof(getTrafPrivsOnObject);

                // Determine the type of object
                if (getMItdb().queryType_ == ComTdbExeUtilGetMetadataInfo::PRIVILEGES_ON_TABLE_)
                  var = COM_BASE_TABLE_OBJECT_LIT;
                else if (getMItdb().queryType_ == ComTdbExeUtilGetMetadataInfo::PRIVILEGES_ON_VIEW_)
                  var = COM_VIEW_OBJECT_LIT;
                else if (getMItdb().queryType_ == ComTdbExeUtilGetMetadataInfo::PRIVILEGES_ON_SEQUENCE_)
                  var = COM_SEQUENCE_GENERATOR_OBJECT_LIT;
                else if (getMItdb().queryType_ == ComTdbExeUtilGetMetadataInfo::PRIVILEGES_ON_LIBRARY_)
                  var = COM_LIBRARY_OBJECT_LIT;
                else
                  var = COM_USER_DEFINED_ROUTINE_OBJECT_LIT;

                char buf[authList.length() + 100];

                Int32 authID = 0;
                if (getMItdb().getParam1())
                  {
                    authID = *currContext->getDatabaseUserID();
                    if (!(strcmp(getMItdb().getParam1(), currContext->getDatabaseUserName()) == 0))
                      authID = getAuthID(getMItdb().getParam1(), cat, sch, auths);
                  }

                if (doPrivCheck)
                  {
                    if (getMItdb().getParam1())
                      {
                        if ((CmpSeabaseDDLauth::isRoleID(authID) && !ComUser::currentUserHasRole(authID)) ||
                            (CmpSeabaseDDLauth::isUserID(authID) && authID != ComUser::getCurrentUser()))
                          {
                            ExRaiseSqlError(getHeap(), &diagsArea_, -CAT_NOT_AUTHORIZED);
                            step_ = HANDLE_ERROR_;
                            break;
                          }     
                        str_sprintf(buf, " and grantee_id in %s ", authList.data());
                      }
                    else
                        str_sprintf(buf, " and grantee_id in %s ", authList.data());
                    privWhereClause = buf;
                  }
                else
                  {
                    if (getMItdb().getParam1())
                      {
                        if (strcmp(getMItdb().getParam1(), currContext->getDatabaseUserName()) == 0)
                          str_sprintf(buf, " and grantee_id in %s ", authList.data());
                        else
                          {
                            char *userRoleList = getRoleList(authID, cat, pmsch, role_usage);
                            if (userRoleList)
                              {
                                str_sprintf(buf, " and grantee_id in %s ", userRoleList);
                                NADELETEBASIC(userRoleList, getHeap());
                              }
                            else
                              str_sprintf(buf, " = %d ", authID);
                          }
                        privWhereClause = buf;
                      }
                  }

                param_[0] = cat;
                param_[1] = pmsch;
                param_[2] = objPrivs;
                param_[3] = cat;
                param_[4] = sch;
                param_[5] = tab;
                param_[6] = getMItdb().cat_;
                param_[7] = getMItdb().sch_;
                param_[8] = getMItdb().obj_;
                param_[9] = (char *)var.data();
                param_[10] = (char *)privWhereClause.data();
                param_[11] = cat;
                param_[12] = pmsch;
                param_[13] = colPrivs;
                param_[14] = cat;
                param_[15] = sch;
                param_[16] = tab;
                param_[17] = getMItdb().cat_;
                param_[18] = getMItdb().sch_;
                param_[19] = getMItdb().obj_;
                param_[20] = (char *)var.data();
                param_[21] = (char *)privWhereClause.data();

                numOutputEntries_ = 2;
                break;
              }

              case ComTdbExeUtilGetMetadataInfo::INDEXES_FOR_USER_:
                {
                  qs = getTrafIndexesForAuth;
                  sizeOfqs = sizeof(getTrafIndexesForAuth);

                  // Get the authID associated with the specified user
                  Int32 authID = *currContext->getDatabaseUserID();
                  if (!(strcmp(getMItdb().getParam1(), currContext->getDatabaseUserName()) == 0))
                    authID = getAuthID(getMItdb().getParam1(), cat, sch, auths);

                  // If not a user, we are done, don't return data
                  if (!CmpSeabaseDDLauth::isUserID(authID))
                    {
                      ExRaiseSqlError(getHeap(), &diagsArea_, -CAT_IS_NOT_A_USER, 
                          NULL, NULL, NULL,
                          getMItdb().getParam1());
                      step_ = HANDLE_ERROR_;
                      break;
                    }

                  // Non elevated user cannot view indexes for another user
                  if (doPrivCheck && authID != ComUser::getCurrentUser())
                    {
                      ExRaiseSqlError(getHeap(), &diagsArea_, -CAT_NOT_AUTHORIZED);
                      step_ = HANDLE_ERROR_;
                      break;
                    }

                  // Limit results to privileges allowed for specified user
                  
                  if (authID == ComUser::getCurrentUser())
                    privWhereClause = getGrantedPrivCmd(authList, cat, NAString("T.object_uid"));
                  else
                    {
                      char *userRoleList = getRoleList(authID, cat, pmsch, role_usage);
                      if (userRoleList)
                        {
                          privWhereClause = getGrantedPrivCmd(userRoleList, cat, NAString ("T.object_uid"));
                          NADELETEBASIC(userRoleList, getHeap());
                        }
                      else
                        {
                          // Unable to read metadata
                          ExRaiseSqlError(getHeap(), &diagsArea_, -8001);
                          step_ = HANDLE_ERROR_;
                          break;
                        }
                    }

                  param_[0] = cat;
                  param_[1] = sch;
                  param_[2] = indexes;
                  param_[3] = cat;
                  param_[4] = sch;
                  param_[5] = tab;
                  param_[6] = cat;
                  param_[7] = sch;
                  param_[8] = tab;
                  param_[9] = getMItdb().cat_;
                  param_[10] = (char *)privWhereClause.data();
                }
                break;

              case ComTdbExeUtilGetMetadataInfo::INDEXES_FOR_ROLE_:
                {
                  qs = getTrafIndexesForAuth;
                  sizeOfqs = sizeof(getTrafIndexesForAuth);

                  // Get the authID associated with the specified role
                  Int32 authID = *currContext->getDatabaseUserID();
                  if (!(strcmp(getMItdb().getParam1(), currContext->getDatabaseUserName()) == 0))
                    authID = getAuthID(getMItdb().getParam1(), cat, sch, auths);

                  // Verify that the authID is actually a role
                  if (!CmpSeabaseDDLauth::isRoleID(authID) && !ComUser::isPublicUserID(authID))
                    {
                      ExRaiseSqlError(getHeap(), &diagsArea_, -CAT_IS_NOT_A_ROLE, 
                          NULL, NULL, NULL,
                          getMItdb().getParam1());
                      step_ = HANDLE_ERROR_;
                      break;
                    }

                  // Non elevated users need to be granted role
                  if (doPrivCheck && !ComUser::currentUserHasRole(authID))
                    {
                      ExRaiseSqlError(getHeap(), &diagsArea_, -CAT_NOT_AUTHORIZED);
                      step_ = HANDLE_ERROR_;
                      break;
                    }

                  // Only return indexes that role (authID) has been granted privs
                  char buf[30];
                  str_sprintf(buf, "(%d)", authID);
                  privWhereClause = getGrantedPrivCmd(buf, cat, NAString ("T.object_uid"));

                  param_[0] = cat;
                  param_[1] = sch;
                  param_[2] = indexes;
                  param_[3] = cat;
                  param_[4] = sch;
                  param_[5] = tab;
                  param_[6] = cat;
                  param_[7] = sch;
                  param_[8] = tab;
                  param_[9] = getMItdb().cat_;
                  param_[10] = (char *)privWhereClause.data();
                }
                break;


              case ComTdbExeUtilGetMetadataInfo::TABLES_FOR_USER_:
              case ComTdbExeUtilGetMetadataInfo::VIEWS_FOR_USER_:
                {
                  qs = getTrafObjectsForUser;
                  sizeOfqs = sizeof(getTrafObjectsForUser);

                  // Get the authID associated with the specified user
                  Int32 authID = *currContext->getDatabaseUserID();
                  if (!(strcmp(getMItdb().getParam1(), currContext->getDatabaseUserName()) == 0))
                    authID = getAuthID(getMItdb().getParam1(), cat, sch, auths);

                  // Verify that the authID is actually a user
                  if (!CmpSeabaseDDLauth::isUserID(authID))
                    {
                      ExRaiseSqlError(getHeap(), &diagsArea_, -CAT_IS_NOT_A_USER, 
                          NULL, NULL, NULL,
                          getMItdb().getParam1());
                      step_ = HANDLE_ERROR_;
                      break;
                    }
  
                  // Non elevated user cannot view objects for another user
                  if (doPrivCheck && authID != ComUser::getCurrentUser())
                    {
                      ExRaiseSqlError(getHeap(), &diagsArea_, -CAT_NOT_AUTHORIZED);
                      step_ = HANDLE_ERROR_;
                      break;
                    }

                  if (getMItdb().queryType_ == ComTdbExeUtilGetMetadataInfo::TABLES_FOR_USER_)
                    var = COM_BASE_TABLE_OBJECT_LIT;
                  else
                    var = COM_VIEW_OBJECT_LIT;

                  // Limit results to privileges allowed for specified user
                  
                  if (authID == ComUser::getCurrentUser())
                    privWhereClause = getGrantedPrivCmd(authList, cat, NAString ("T.object_uid"));

                  // Getting objects for a user other than the current user
                  else
                    {
                      char *userRoleList = getRoleList(authID, cat, pmsch, role_usage);
                      if (userRoleList)
                        {
                          privWhereClause = getGrantedPrivCmd(userRoleList, cat, NAString ("T.object_uid"));
                          NADELETEBASIC(userRoleList, getHeap());
                        }
                      else 
                        {
                          // Unable to read metadata
                          ExRaiseSqlError(getHeap(), &diagsArea_, -8001);
                          step_ = HANDLE_ERROR_;
                          break;
                        }
                    }

                  param_[0] = cat;
                  param_[1] = sch;
                  param_[2] = tab;
                  param_[3] = getMItdb().cat_;
                  param_[4] = (char *)var.data();
                  param_[5] = (char *)privWhereClause.data();
                }
              break;

              case ComTdbExeUtilGetMetadataInfo::TABLES_FOR_ROLE_:
              case ComTdbExeUtilGetMetadataInfo::VIEWS_FOR_ROLE_:
                {
                  qs = getTrafObjectsForUser;
                  sizeOfqs = sizeof(getTrafObjectsForUser);

                  // Get the authID associated with the specified user
                  Int32 authID = *currContext->getDatabaseUserID();
                  if (!(strcmp(getMItdb().getParam1(), currContext->getDatabaseUserName()) == 0))
                    authID = getAuthID(getMItdb().getParam1(), cat, sch, auths);

                  // Verify that the specified authID is actually a role
                  if (!CmpSeabaseDDLauth::isRoleID(authID) && !ComUser::isPublicUserID(authID)) 
                    {
                      ExRaiseSqlError(getHeap(), &diagsArea_, -CAT_IS_NOT_A_ROLE, 
                          NULL, NULL, NULL,
                          getMItdb().getParam1());
                      step_ = HANDLE_ERROR_;
                      break;
                    }

                  // Non elevated users must be granted the specified role
                  if (doPrivCheck && !ComUser::currentUserHasRole(authID))
                    {
                      ExRaiseSqlError(getHeap(), &diagsArea_, -CAT_NOT_AUTHORIZED);
                      step_ = HANDLE_ERROR_;
                      break;
                    }

                  if (getMItdb().queryType_ == ComTdbExeUtilGetMetadataInfo::TABLES_FOR_ROLE_)
                    var = COM_BASE_TABLE_OBJECT_LIT;
                  else
                    var = COM_VIEW_OBJECT_LIT;

                  // Only return objects where role (authID) has been granted privs 
                  char buf[30];
                  str_sprintf(buf, "(%d)", authID);
                  privWhereClause = getGrantedPrivCmd(buf, cat, NAString ("T.object_uid"));

                  param_[0] = cat;
                  param_[1] = sch;
                  param_[2] = tab;
                  param_[3] = getMItdb().cat_;
                  param_[4] = (char *)var.data();
                  param_[5] = (char *)privWhereClause.data();
                }
              break;

              case ComTdbExeUtilGetMetadataInfo::LIBRARIES_FOR_USER_:
                {
                  qs = getTrafLibrariesForAuthQuery;
                  sizeOfqs = sizeof(getTrafLibrariesForAuthQuery);

                  // Get the authID associated with the specified user
                  Int32 authID = *currContext->getDatabaseUserID();
                  if (!(strcmp(getMItdb().getParam1(), currContext->getDatabaseUserName()) == 0))
                    authID = getAuthID(getMItdb().getParam1(), cat, sch, auths);

                  // Verify that the specified authID is actually a user
                  if (!CmpSeabaseDDLauth::isUserID(authID))
                    {
                      ExRaiseSqlError(getHeap(), &diagsArea_, -CAT_IS_NOT_A_USER, 
                          NULL, NULL, NULL,
                          getMItdb().getParam1());
                      step_ = HANDLE_ERROR_;
                      break;
                    }

                  // Non elevated user cannot view libraries for another user
                  if (doPrivCheck && authID != ComUser::getCurrentUser())
                    {
                      ExRaiseSqlError(getHeap(), &diagsArea_, -CAT_NOT_AUTHORIZED);
                      step_ = HANDLE_ERROR_;
                      break;
                    }
                  
                  // Return libraries that are owned by the user/user's roles
                  // or that the user/user's role have been granted privileges
                    
                  if (authID == ComUser::getCurrentUser())
                    privWhereClause += getGrantedPrivCmd(authList, cat);
                  else
                    {
                      char *userRoleList = getRoleList(authID, cat, pmsch, role_usage);
                      if (userRoleList)
                        {
                          privWhereClause = getGrantedPrivCmd(userRoleList, cat);
                          NADELETEBASIC(userRoleList, getHeap());
                        }
                      else
                        {
                          // Unable to read metadata 
                          ExRaiseSqlError(getHeap(), &diagsArea_, -8001);
                          step_ = HANDLE_ERROR_;
                          break;
                        }
                    }

                  param_[0] = cat;
                  param_[1] = sch;
                  param_[2] = tab;
                  param_[3] = getMItdb().cat_;
                  param_[4] = (char *) privWhereClause.data();
                }
                break ;

              case ComTdbExeUtilGetMetadataInfo::LIBRARIES_FOR_ROLE_:
                {
                  qs = getTrafLibrariesForAuthQuery;
                  sizeOfqs = sizeof(getTrafLibrariesForAuthQuery);

                  // Get the authID associated with the specified role
                  Int32 authID = *currContext->getDatabaseUserID();
                  if (!(strcmp(getMItdb().getParam1(), currContext->getDatabaseUserName()) == 0))
                    authID = getAuthID(getMItdb().getParam1(), cat, sch, auths);

                  // Verify that the specified authID is actually a role
                  if (!CmpSeabaseDDLauth::isRoleID(authID) && !ComUser::isPublicUserID(authID)) 
                    {
                      ExRaiseSqlError(getHeap(), &diagsArea_, -CAT_IS_NOT_A_ROLE, 
                          NULL, NULL, NULL,
                          getMItdb().getParam1());
                      step_ = HANDLE_ERROR_;
                      break;
                    }

                  // Non elevated users must be granted role
                  if (doPrivCheck && !ComUser::currentUserHasRole(authID))
                    {
                      ExRaiseSqlError(getHeap(), &diagsArea_, -CAT_NOT_AUTHORIZED);
                      step_ = HANDLE_ERROR_;
                      break;
                    }

                  // Return libraries that are owned by the user/user's roles
                  // or that the user/user's role have been granted privileges

                  if (authID == ComUser::getCurrentUser())
                    privWhereClause += getGrantedPrivCmd(authList, cat);
                  else
                    {
                      char *userRoleList = getRoleList(authID, cat, pmsch, role_usage);
                      if (userRoleList)
                        {
                          privWhereClause = getGrantedPrivCmd(userRoleList, cat);
                          NADELETEBASIC(userRoleList, getHeap());
                        }
                      else
                        {
                          // Unable to read metadata 
                          ExRaiseSqlError(getHeap(), &diagsArea_, -8001);
                          step_ = HANDLE_ERROR_;
                          break;
                        }
                    }

                  param_[0] = cat;
                  param_[1] = sch;
                  param_[2] = tab;
                  param_[3] = getMItdb().cat_;
                  param_[4] = (char *) privWhereClause.data();
                }
                break ;

              case ComTdbExeUtilGetMetadataInfo::COMPONENTS_:
              {
                qs = getComponents;
                sizeOfqs = sizeof(getComponents);

                if (doPrivCheck)
                  {
                     char buf[authList.length() + 100];
                     str_sprintf(buf, " and p.grantee_id in %s", authList.data());
                     privWhereClause = buf;
                  }

                param_[0] = cat;
                param_[1] = pmsch;
                param_[2] = components;
                param_[3] = cat;
                param_[4] = pmsch;
                param_[5] = componentPrivileges;
                param_[6] = (char *) privWhereClause.data();
              }
              break;

              case ComTdbExeUtilGetMetadataInfo::COMPONENT_PRIVILEGES_:
              {
                 qs = getComponentPrivileges;
                 sizeOfqs = sizeof(getComponentPrivileges);

                 // Get privileges for auth name
                 if (getMItdb().getParam1()) 
                 {
                    // Get the authID associated with the request's auth name
                    // If can't find authID, NA_UserIdDefault is returned which 
                    // indicates an invalid authID.
                    Int32 authID = *currContext->getDatabaseUserID();
                    if (!(strcmp(getMItdb().getParam1(), currContext->getDatabaseUserName()) == 0))
                      authID = getAuthID(getMItdb().getParam1(), cat, sch, auths);

                    // if incorrect auth type, return error
                    if (!CmpSeabaseDDLauth::isRoleID(authID) && !CmpSeabaseDDLauth::isUserID(authID) &&
                        !ComUser::isPublicUserID(authID))
                    {
                      ExRaiseSqlError(getHeap(), &diagsArea_, -CAT_IS_NOT_CORRECT_AUTHID,
                          NULL, NULL, NULL,
                          getMItdb().getParam1(), "user or role");
                      step_ = HANDLE_ERROR_;
                      break;
                    }

                    if (doPrivCheck)
                    {
                      // If asking for privileges for a user that has no privs, return error
                      if ((CmpSeabaseDDLauth::isRoleID(authID) && !ComUser::currentUserHasRole(authID)) ||
                          (CmpSeabaseDDLauth::isUserID(authID) && authID != ComUser::getCurrentUser()))
                      {
                        ExRaiseSqlError(getHeap(), &diagsArea_, -CAT_NOT_AUTHORIZED);
                        step_ = HANDLE_ERROR_;
                        break;
                       }

                       privWhereClause += "and (grantee_name = '";
                       privWhereClause += getMItdb().getParam1();
                       privWhereClause += "'";
                       if (CmpSeabaseDDLauth::isUserID(authID) && getMItdb().cascade())
                         {
                            privWhereClause += " or grantee_id in ";
                            privWhereClause += authList.data();
                         }
                        privWhereClause += ")";
                    }
                    else
                    { 
                       privWhereClause += "and (grantee_name = '";
                       privWhereClause += getMItdb().getParam1();
                       privWhereClause += "'";

                       // if authname is a user and specified cascade, include roles
                       if (CmpSeabaseDDLauth::isUserID(authID) && getMItdb().cascade())
                       {
                          char buf[300 + MAX_AUTHNAME_LEN + 200];
                          str_sprintf(buf, "or p.grantee_id in (select role_id from "
                                           "%s.\"%s\".%s where grantee_name = '%s') "
                                           "or p.grantee_id = -1",
                                      cat, pmsch, role_usage, getMItdb().getParam1());
                          privWhereClause += buf;
                       }
                       privWhereClause += ')';  
                    }
                 }

                 // no specific authname specified, get current users results
                 else
                 {
                    // Limit results to current user and current users roles
                    if (getMItdb().cascade())
                    {
                       privWhereClause += " and p.grantee_id in ";
                       privWhereClause += authList.data();
                    }
                    // limit results to current user
                    else
                    {
                       privWhereClause += " and p.grantee_name = '";
                       privWhereClause += currContext->getDatabaseUserName();
                       privWhereClause += "'";
                    }
                 }

                 param_[0] = cat;
                 param_[1] = pmsch;
                 param_[2] = components;
                 param_[3] = cat;
                 param_[4] = pmsch;
                 param_[5] = componentOperations;
                 param_[6] = cat;
                 param_[7] = pmsch;
                 param_[8] = componentPrivileges;
                 param_[9] = getMItdb().getObj();
                 param_[10] = (char *) privWhereClause.data();
              }
              break;

              case ComTdbExeUtilGetMetadataInfo::SEQUENCES_IN_CATALOG_:
                {
                  qs = getTrafSequencesInCatalogQuery;
                  sizeOfqs = sizeof(getTrafSequencesInCatalogQuery);

                  if (doPrivCheck)
                    privWhereClause = getGrantedPrivCmd(authList, cat);

		  param_[0] = cat;
		  param_[1] = sch;
		  param_[2] = tab;
		  param_[3] = getMItdb().cat_;
                  param_[4] = (char *) privWhereClause.data();
                }
                break ;

              case ComTdbExeUtilGetMetadataInfo::SEQUENCES_IN_SCHEMA_:
                {
                  qs = getTrafSequencesInSchemaQuery;
                  sizeOfqs = sizeof(getTrafSequencesInSchemaQuery);

                  if (doPrivCheck)
                    privWhereClause = getGrantedPrivCmd(authList, cat);

		  param_[0] = cat;
		  param_[1] = sch;
		  param_[2] = tab;
		  param_[3] = getMItdb().cat_;
		  param_[4] = getMItdb().sch_;
                  param_[5] = (char *) privWhereClause.data();
                }
                break ;

	      default:
		{
                  ExRaiseSqlError(getHeap(), &diagsArea_, -4218, 
                       NULL, NULL, NULL,
                       "GET");
		  step_ = HANDLE_ERROR_;
		}
		break;
	      }
         
             if (step_ == HANDLE_ERROR_)
               break;

	     Int32 qryArraySize = sizeOfqs / sizeof(QueryString);
	     char * gluedQuery;
	     Lng32 gluedQuerySize;
	     glueQueryFragments(qryArraySize, qs, 
				gluedQuery, gluedQuerySize);
	     
	     str_sprintf(queryBuf_, gluedQuery,
			 param_[0], param_[1], param_[2], param_[3], param_[4],
			 param_[5], param_[6], param_[7], param_[8], param_[9],
			 param_[10], param_[11], param_[12], param_[13], param_[14], param_[15],
                         param_[16], param_[17], param_[18], param_[19], param_[20],
                         param_[21]);
             NADELETEBASIC(gluedQuery, getMyHeap());	     
	     step_ = FETCH_ALL_ROWS_;
	  }
	  break;

	case FETCH_ALL_ROWS_:
	  {
	    if (initializeInfoList(infoList_))
	      {
		step_ = HANDLE_ERROR_;
		break;
	      }
 
	    if (fetchAllRows(infoList_, queryBuf_, numOutputEntries_,
			     FALSE, retcode) < 0)
	      {
                cliInterface()->allocAndRetrieveSQLDiagnostics(diagsArea_);
		step_ = HANDLE_ERROR_;
		break;
	      }

	    infoList_->position();

	    step_ = RETURN_ROW_;
	  }
	break;

	case RETURN_ROW_:
	  {
	    if (infoList_->atEnd())
	      {
		step_ = ENABLE_CQS_;
		break;
	      }

	    if (qparent_.up->isFull())
	      return WORK_OK;

	    OutputInfo * vi = (OutputInfo*)infoList_->getCurr();

	    char * ptr = vi->get(0);
	    short len = (short)(ptr ? strlen(ptr) : 0);
 
	    exprRetCode = ex_expr::EXPR_TRUE;

            if ((getMItdb().queryType_ == ComTdbExeUtilGetMetadataInfo::PRIVILEGES_FOR_USER_) ||
                (getMItdb().queryType_ == ComTdbExeUtilGetMetadataInfo::PRIVILEGES_FOR_ROLE_) ||
                (getMItdb().queryType_ == ComTdbExeUtilGetMetadataInfo::PRIVILEGES_ON_VIEW_) ||
                (getMItdb().queryType_ == ComTdbExeUtilGetMetadataInfo::PRIVILEGES_ON_TABLE_) || 
                (getMItdb().queryType_ == ComTdbExeUtilGetMetadataInfo::PRIVILEGES_ON_SEQUENCE_) || 
                (getMItdb().queryType_ == ComTdbExeUtilGetMetadataInfo::PRIVILEGES_ON_LIBRARY_) || 
                (getMItdb().queryType_ == ComTdbExeUtilGetMetadataInfo::PRIVILEGES_ON_ROUTINE_))

            {
              // output:  privileges<4spaces>object name
              NAString outputStr (vi->get(1));
              outputStr += "    ";
              outputStr += ptr;
  
              char * outputCharStr = new char[outputStr.length() + 1];
              memset (outputCharStr,'\0', outputStr.length() + 1);
              str_cpy_all(outputCharStr, outputStr.data(), outputStr.length());
              ptr = outputCharStr;
              len = outputStr.length();
            }

// Not supported at this time
#if 0
	    if ((getMItdb().queryType_ == ComTdbExeUtilGetMetadataInfo::TRIGTEMP_TABLE_ON_TABLE_ ) || 
		(getMItdb().queryType_ == ComTdbExeUtilGetMetadataInfo::TRIGTEMP_TABLE_ON_MV_ ))
	      {
		//append __TEMP to the name
		short len1 = strlen(ptr);
		char *nameString = new char[len1+7+1];
	        memset(nameString,'\0',len1+7+1);
		ComBoolean isQuoted = FALSE;
	
		str_cpy_all(nameString, vi->get(0),len1);
		if ( '"' == nameString[ len1 - 1 ] )
		  {
		    isQuoted = TRUE;
		  }
		if (isQuoted)
		  str_cpy_all(&nameString[len1-1],"__TEMP\"",8);
		else
		  str_cpy_all(&nameString[len1],"__TEMP",6);
		
		ptr = nameString;
		if (isQuoted)
		  len = len1+7;
		else
		  len = len1+6;

			    
	      }
#endif

	    if (((getMItdb().queryType_ == ComTdbExeUtilGetMetadataInfo::TABLES_IN_VIEW_)
		 //|| (getMItdb().queryType_ == ComTdbExeUtilGetMetadataInfo::TABLES_IN_MV_)
                ) &&
		(vi->get(1) && (strcmp(vi->get(1), "BT") != 0)))
	      exprRetCode = ex_expr::EXPR_FALSE;
	    else if ((getMItdb().queryType_ == ComTdbExeUtilGetMetadataInfo::VIEWS_IN_VIEW_) &&
		(vi->get(1) && (strcmp(vi->get(1), "VI") != 0)))
	      exprRetCode = ex_expr::EXPR_FALSE;
	    //else if ((getMItdb().queryType_ == ComTdbExeUtilGetMetadataInfo::MVS_IN_MV_) &&
            //         (vi->get(1) && (strcmp(vi->get(1), "MV") != 0)))
	    //  exprRetCode = ex_expr::EXPR_FALSE;

	    if (exprRetCode == ex_expr::EXPR_TRUE)
	      exprRetCode = evalScanExpr(ptr, len, TRUE);
	    if (exprRetCode == ex_expr::EXPR_FALSE)
	      {
		// row does not pass the scan expression,
		// move to the next row.
		infoList_->advance();
		break;
	      }
	    else if (exprRetCode == ex_expr::EXPR_ERROR)
	     {
		step_ = HANDLE_ERROR_;
		break;
	      }

	    if (NOT headingReturned_)
	      {
		step_ = DISPLAY_HEADING_;
		break;
	      }

            // if returned table name is an external name, convert it to the native name.
            // Do it for tables_in_view and objects_in_view operations only.
            NAString nativeName;
	    if (((getMItdb().queryType_ == ComTdbExeUtilGetMetadataInfo::TABLES_IN_VIEW_) ||
		 (getMItdb().queryType_ == ComTdbExeUtilGetMetadataInfo::OBJECTS_IN_VIEW_)) &&
		(vi->get(1) && (strcmp(vi->get(1), "BT") == 0)))
              {
                char tempBuf[2*len];
                Lng32 numParts = 0;
                char *parts[4];
                LateNameInfo::extractParts(ptr, tempBuf, numParts, parts, TRUE);
                if (numParts == 3)
                  {
                    NAString catName(parts[0]);
                    NAString schName(parts[1]);
                    NAString objName(parts[2]);
                    if (ComIsTrafodionExternalSchemaName(schName))
                      {
                        ComObjectName tableName(catName,
                                                schName,
                                                objName,
                                                COM_TABLE_NAME);
                        
                        nativeName =
                          ComConvertTrafNameToNativeName
                          (tableName.getCatalogNamePartAsAnsiString(),
                           tableName.getSchemaNamePartAsAnsiString(),
                           tableName.getObjectNamePartAsAnsiString());
                        ptr = (char*)nativeName.data();
						len = nativeName.length();
                      }
                  }
              }

	    short rc = 0;
	    if (moveRowToUpQueue(ptr, len, &rc))
            {
	      return rc;
            }

	    infoList_->advance();
            incReturnRowCount();
	  }
	break;

	case DISPLAY_HEADING_:
	  {
	    retcode = displayHeading();
	    if (retcode == 1)
	      return WORK_OK;
	    else if (retcode < 0)
	      {
		step_ = HANDLE_ERROR_;
		break;
	      }

	    headingReturned_ = TRUE;

	    step_ = RETURN_ROW_;
	  }
	break;

	case ENABLE_CQS_:
	  {
	    if (restoreCQS())
	      {
		step_ = HANDLE_ERROR_;
		break;
	      }

	    if ((getMItdb().queryType_ == ComTdbExeUtilGetMetadataInfo::VIEWS_ON_TABLE_) ||
		//(getMItdb().queryType_ == ComTdbExeUtilGetMetadataInfo::MVS_ON_TABLE_) ||
		//(getMItdb().queryType_ == ComTdbExeUtilGetMetadataInfo::MVS_ON_VIEW_) ||
		(getMItdb().queryType_ == ComTdbExeUtilGetMetadataInfo::VIEWS_ON_VIEW_)) 

	      step_ = GET_USING_VIEWS_;
	    else if ((getMItdb().queryType_ == ComTdbExeUtilGetMetadataInfo::TABLES_IN_VIEW_) ||
		     (getMItdb().queryType_ == ComTdbExeUtilGetMetadataInfo::VIEWS_IN_VIEW_) ||
		     //(getMItdb().queryType_ == ComTdbExeUtilGetMetadataInfo::TABLES_IN_MV_) ||
		     //(getMItdb().queryType_ == ComTdbExeUtilGetMetadataInfo::MVS_IN_MV_) ||
		     //(getMItdb().queryType_ == ComTdbExeUtilGetMetadataInfo::OBJECTS_IN_MV_) ||
		     (getMItdb().queryType_ == ComTdbExeUtilGetMetadataInfo::OBJECTS_IN_VIEW_))
	      step_ = GET_USED_OBJECTS_;
	    else
	      step_ = DONE_;
	  }
	break;

	case GET_USING_VIEWS_:
	  {
	    if (qparent_.up->isFull())
	      return WORK_OK;

	    if (NOT getMItdb().allObjs())
	      {
		step_ = DONE_;
		break;
	      }

	    char * viewName = NULL;
	    Lng32 len = 0;
	    if ((getMItdb().queryType_ == ComTdbExeUtilGetMetadataInfo::VIEWS_ON_TABLE_) ||
		(getMItdb().queryType_ == ComTdbExeUtilGetMetadataInfo::VIEWS_ON_VIEW_))
	      cliRC = getUsingView(infoList_, TRUE, viewName, len);
	    else
	      cliRC = getUsingView(infoList_, FALSE, viewName, len);

	    if (cliRC == 100)
	      {
		step_ = DONE_;
		break;
	      }
	    else if (cliRC < 0)
	      {
		step_ = HANDLE_ERROR_;
		break;
	      }

	    exprRetCode = evalScanExpr(viewName, len, TRUE);
	    if (exprRetCode == ex_expr::EXPR_FALSE)
	      {
		// row does not pass the scan expression,
		// move to the next row.
		break;
	      }
	    else if (exprRetCode == ex_expr::EXPR_ERROR)
	      {
		step_ = HANDLE_ERROR_;
		break;
	      }

	    short rc = 0;
	    moveRowToUpQueue(viewName, len, &rc);
	  }
	break;

	case GET_USED_OBJECTS_:
	  {
	    if (qparent_.up->isFull())
	      return WORK_OK;

	    if (NOT getMItdb().allObjs())
	      {
		step_ = DONE_;
		break;
	      }

	    char * viewName = NULL;
	    Lng32 len = 0;
	    if ((getMItdb().queryType_ == ComTdbExeUtilGetMetadataInfo::TABLES_IN_VIEW_) ||
		(getMItdb().queryType_ == ComTdbExeUtilGetMetadataInfo::VIEWS_IN_VIEW_) ||
		(getMItdb().queryType_ == ComTdbExeUtilGetMetadataInfo::OBJECTS_IN_VIEW_))
	      cliRC = getUsedObjects(infoList_, TRUE, viewName, len);
	    else
	      cliRC = getUsedObjects(infoList_, FALSE, viewName, len);

	    if (cliRC == 100)
	      {
		step_ = DONE_;
		break;
	      }
	    else if (cliRC < 0)
	      {
		step_ = HANDLE_ERROR_;
		break;
	      }

	    exprRetCode = evalScanExpr(viewName, len, TRUE);
	    if (exprRetCode == ex_expr::EXPR_FALSE)
	      {
		// row does not pass the scan expression,
		// move to the next row.
		break;
	      }
	    else if (exprRetCode == ex_expr::EXPR_ERROR)
	      {
		step_ = HANDLE_ERROR_;
		break;
	      }

	    short rc = 0;
	    moveRowToUpQueue(viewName, len, &rc);
	  }
	break;

	case HANDLE_ERROR_:
	  {
	    restoreCQS();

	    retcode = handleError();
	    if (retcode == 1)
	      {
		return WORK_OK;
	      } // if (retcode
	    
	    step_ = DONE_;
	  }
	break;

	case DONE_:
	  {
            if (NOT getMItdb().noHeader() && getReturnRowCount() > 0)
            {
              short rc = 0;
              char returnMsg[256];
              memset(returnMsg, 0, 256);
              sprintf(returnMsg, "\n=======================\n %d row(s) returned", getReturnRowCount());
              moveRowToUpQueue(returnMsg, strlen(returnMsg), &rc);
            }

	    retcode = handleDone();
	    if (retcode == 1)
	      return WORK_OK;

	    step_ = INITIAL_;

	    return WORK_OK;
	  }
	break;

	}

    }

  return 0;
}

////////////////////////////////////////////////////////////////
// Constructor for class ExExeUtilGetMetadataInfoComplexTcb
///////////////////////////////////////////////////////////////
ExExeUtilGetMetadataInfoComplexTcb::ExExeUtilGetMetadataInfoComplexTcb(
     const ComTdbExeUtilGetMetadataInfo & exe_util_tdb,
     ex_globals * glob)
     : ExExeUtilGetMetadataInfoTcb( exe_util_tdb, glob)
{
}

ExExeUtilGetMetadataInfoComplexTcb::~ExExeUtilGetMetadataInfoComplexTcb()
{
}

//////////////////////////////////////////////////////
// work() for ExExeUtilGetMetadataInfoComplexTcb
//////////////////////////////////////////////////////
short ExExeUtilGetMetadataInfoComplexTcb::work()
{
  short retcode = 0;
  Lng32 cliRC = 0;
  ex_expr::exp_return_type exprRetCode = ex_expr::EXPR_OK;

  // if no parent request, return
  if (qparent_.down->isEmpty())
    return WORK_OK;

  // if no room in up queue, won't be able to return data/status.
  // Come back later.
  if (qparent_.up->isFull())
    return WORK_OK;

  ex_queue_entry * pentry_down = qparent_.down->getHeadEntry();
  ExExeUtilPrivateState & pstate =
    *((ExExeUtilPrivateState*) pentry_down->pstate);

  // Get the globals stucture of the master executor.
  ExExeStmtGlobals *exeGlob = getGlobals()->castToExExeStmtGlobals();
  ExMasterStmtGlobals *masterGlob = exeGlob->castToExMasterStmtGlobals();
  ContextCli * currContext = masterGlob->getStatement()->getContext();

  while (1)
    {
      switch (step_)
	{
	case INITIAL_:
	  {
	    step_ = SETUP_QUERY_;
            returnRowCount_ = 0 ;
	  }
	break;

	case SETUP_QUERY_:
	  {
	    patternStr_[0] = '\0';
	    if (getMItdb().getPattern())
	      {
		str_sprintf(patternStr_, ", match '%s' ",
			    getMItdb().getPattern());
	      }

	    step_ = FETCH_ALL_ROWS_;

	    char rfn[200];
	    if (getMItdb().returnFullyQualNames())
	      strcpy(rfn, ", return full names ");
	    else
	      strcpy(rfn, " ");

	    switch (getMItdb().queryType_)
	      {
	      case ComTdbExeUtilGetMetadataInfo::VIEWS_ON_TABLE_:
		{
		  str_sprintf(queryBuf_, "select * from (get all views on table \"%s\".\"%s\".\"%s\", no header %s %s) x(a) group by a order by 1",
			      getMItdb().getCat(), getMItdb().getSch(), getMItdb().getObj(),
			      rfn,
			      patternStr_);
		}
	      break;

	      case ComTdbExeUtilGetMetadataInfo::VIEWS_ON_VIEW_:
		{
		  str_sprintf(queryBuf_, "select * from (get all views on view \"%s\".\"%s\".\"%s\", no header %s %s) xxx(aaa) group by aaa order by 1",
			      getMItdb().getCat(), getMItdb().getSch(), getMItdb().getObj(),
			      rfn,
			      patternStr_);
		}
	      break;

	      case ComTdbExeUtilGetMetadataInfo::TABLES_IN_VIEW_:
		{
		  str_sprintf(queryBuf_, "select * from (get all tables in view \"%s\".\"%s\".\"%s\", no header %s) xxx(aaa) group by aaa order by 1",
			      getMItdb().getCat(), getMItdb().getSch(), getMItdb().getObj(),
			      patternStr_);
		}
	      break;

	      case ComTdbExeUtilGetMetadataInfo::VIEWS_IN_VIEW_:
		{
		  str_sprintf(queryBuf_, "select * from (get all views in view \"%s\".\"%s\".\"%s\", no header %s) xxx(aaa) group by aaa order by 1",
			      getMItdb().getCat(), getMItdb().getSch(), getMItdb().getObj(),
			      patternStr_);
		}
	      break;

	      case ComTdbExeUtilGetMetadataInfo::OBJECTS_IN_VIEW_:
		{
		  str_sprintf(queryBuf_, "select * from (get all objects in view \"%s\".\"%s\".\"%s\", no header %s) xxx(aaa) group by aaa order by 1",
			      getMItdb().getCat(), getMItdb().getSch(), getMItdb().getObj(),
			      patternStr_);
		}
	      break;

	      case ComTdbExeUtilGetMetadataInfo::OBJECTS_ON_TABLE_:
		{
		  step_ = FETCH_ALL_ROWS_FOR_OBJECTS_;
		}
	      break;

	      case ComTdbExeUtilGetMetadataInfo::OBJECTS_IN_SCHEMA_:
		{
		  step_ = FETCH_ALL_ROWS_IN_SCHEMA_;
		}
	      break;

// not supported at this time
#if 0
	      case ComTdbExeUtilGetMetadataInfo::MVS_ON_TABLE_:
		{
		  str_sprintf(queryBuf_, "select * from (get all mvs on table \"%s\".\"%s\".\"%s\", no header %s) xxx(aaa) group by aaa order by 1",
			      getMItdb().getCat(), getMItdb().getSch(), getMItdb().getObj(),
			      patternStr_);
		}
	      break;

	      case ComTdbExeUtilGetMetadataInfo::MVS_ON_MV_:
		{
		  str_sprintf(queryBuf_, "select * from (get all mvs on mv \"%s\".\"%s\".\"%s\", no header %s) xxx(aaa) group by aaa order by 1",
			      getMItdb().getCat(), getMItdb().getSch(), getMItdb().getObj(),
			      patternStr_);
		}
	      break;

	      case ComTdbExeUtilGetMetadataInfo::TABLES_IN_MV_:
		{
		  str_sprintf(queryBuf_, "select * from (get all tables in mv \"%s\".\"%s\".\"%s\", no header %s) xxx(aaa) group by aaa order by 1",
			      getMItdb().getCat(), getMItdb().getSch(), getMItdb().getObj(),
			      patternStr_);
		}
	      break;

	      case ComTdbExeUtilGetMetadataInfo::MVS_IN_MV_:
		{
		  str_sprintf(queryBuf_, "select * from (get all mvs in mv \"%s\".\"%s\".\"%s\", no header %s) xxx(aaa) group by aaa order by 1",
			      getMItdb().getCat(), getMItdb().getSch(), getMItdb().getObj(),
			      patternStr_);
		}
	      break;

	      case ComTdbExeUtilGetMetadataInfo::OBJECTS_IN_MV_:
		{
		  str_sprintf(queryBuf_, "select * from (get all objects in mv \"%s\".\"%s\".\"%s\", no header %s) xxx(aaa) group by aaa order by 1",
			      getMItdb().getCat(), getMItdb().getSch(), getMItdb().getObj(),
			      patternStr_);
		}
	      break;

#endif
	      default:
		{
                  ExRaiseSqlError(getHeap(), &diagsArea_, -4298, 
                            NULL, NULL, NULL, "GET");
		  step_ = HANDLE_ERROR_;
		}
	      break;

	      } // switch
	  }
	break;

	case FETCH_ALL_ROWS_:
	  {
	    if (initializeInfoList(infoList_))
	      {
		step_ = HANDLE_ERROR_;
		break;
	      }

	    if (fetchAllRows(infoList_, queryBuf_, 1, FALSE, retcode) < 0)
	      {
		step_ = HANDLE_ERROR_;

		break;
	      }

	    infoList_->position();

	    step_ = DISPLAY_HEADING_;
	  }
	break;

	case FETCH_ALL_ROWS_FOR_OBJECTS_:
	  {
	    if (initializeInfoList(infoList_))
	      {
		step_ = HANDLE_ERROR_;
		break;
	      }

	    char ausStr[20];
	    strcpy(ausStr, "");
	    if (getMItdb().systemObjs())
	      strcpy(ausStr, "SYSTEM");
	    else if (getMItdb().allObjs())
	      strcpy(ausStr, "ALL");

	    // Get indexes on table
	    str_sprintf(queryBuf_, "get indexes on table \"%s\".\"%s\".\"%s\" %s",
			getMItdb().getCat(), getMItdb().getSch(), getMItdb().getObj(),
			patternStr_);

	    if (fetchAllRows(infoList_, queryBuf_, 1, FALSE, retcode) < 0)
	      {
		step_ = HANDLE_ERROR_;

		break;
	      }

	    NABoolean rowsFound = FALSE;

	    // insert a NULL entry, this will cause a blank row to be returned
	    if (retcode != 100) // some rows were found
	      {
		infoList_->insert((new(getHeap()) OutputInfo(1)));
		rowsFound = TRUE;
	      }

	    // Get views on table
	    str_sprintf(queryBuf_, "get %s views on table \"%s\".\"%s\".\"%s\" %s",
			ausStr,
			getMItdb().getCat(), getMItdb().getSch(), getMItdb().getObj(),
			patternStr_);

	    if (fetchAllRows(infoList_, queryBuf_, 1, FALSE, retcode) < 0)
	      {
		step_ = HANDLE_ERROR_;

		break;
	      }

	    // insert a NULL entry, this will cause a blank row to be returned
	    if (retcode != 100) // some rows were found
	      {
		infoList_->insert((new(getHeap()) OutputInfo(1)));
		rowsFound = TRUE;
	      }

	    // Get mvs on table
	    str_sprintf(queryBuf_, "get %s mvs on table \"%s\".\"%s\".\"%s\" %s",
			ausStr,
			getMItdb().getCat(), getMItdb().getSch(), getMItdb().getObj(),
			patternStr_);

	    if (fetchAllRows(infoList_, queryBuf_, 1, FALSE, retcode) < 0)
	      {
                // if error is 4222 (command not supported), ignore it.
                if (getDiagsArea() != NULL) {
                   if (getDiagsArea()->mainSQLCODE() != -4222)
                   {
                    step_ = HANDLE_ERROR_;
                    break;
                  }
                  getDiagsArea()->clear();
                }
	      }

	    // insert a NULL entry, this will cause a blank row to be returned
	    if (retcode != 100)
	      {
		infoList_->insert((new(getHeap()) OutputInfo(1)));
		rowsFound = TRUE;
	      }

	    // Get synonyms on table
	    str_sprintf(queryBuf_, "get synonyms on table \"%s\".\"%s\".\"%s\" %s",
			getMItdb().getCat(), getMItdb().getSch(), getMItdb().getObj(),
			patternStr_);

	    if (fetchAllRows(infoList_, queryBuf_, 1, FALSE, retcode) < 0)
	      {
                // if error is 4222 (command not supported), ignore it.
                if (getDiagsArea() != NULL) {
                  if (getDiagsArea()->mainSQLCODE() != -4222)
                  {
                    step_ = HANDLE_ERROR_;
                    
                    break;
                  }
                  getDiagsArea()->clear();
                }
	      }

	    // insert a NULL entry, this will cause a blank row to be returned
	    if (retcode != 100)
	      {
		infoList_->insert((new(getHeap()) OutputInfo(1)));
		rowsFound = TRUE;
	      }

	    if (rowsFound)
	      infoList_->removeTail();

	    infoList_->position();

	    step_ = RETURN_ROW_;
	  }
	break;

	case FETCH_ALL_ROWS_IN_SCHEMA_:
	  {
	    if (initializeInfoList(infoList_))
	      {
		step_ = HANDLE_ERROR_;
		break;
	      }

	    NABoolean systemObjs = FALSE;
	    char ausStr[20];
	    strcpy(ausStr, "");
	    if (getMItdb().systemObjs())
	      {
		strcpy(ausStr, "SYSTEM");
		systemObjs = TRUE;
	      }
	    else if (getMItdb().allObjs())
	      strcpy(ausStr, "ALL");

	    // Get tables in schema
	    str_sprintf(queryBuf_, "get %s tables in schema \"%s\".\"%s\" %s",
			ausStr,
			getMItdb().getCat(), getMItdb().getSch(),
			patternStr_);

	    retcode = 100;
	    if (fetchAllRows(infoList_, queryBuf_, 1, FALSE, retcode) < 0)
	      {
		step_ = HANDLE_ERROR_;

		break;
	      }

	    NABoolean rowsFound = FALSE;

	    // insert a NULL entry, this will cause a blank row to be returned
	    if (retcode != 100) // some rows were found
	      {
		infoList_->insert((new(getHeap()) OutputInfo(1)));
		rowsFound = TRUE;
	      }

	    // Get views in schema
	    str_sprintf(queryBuf_, "get views in schema \"%s\".\"%s\" %s",
			getMItdb().getCat(), getMItdb().getSch(),
			patternStr_);

	    retcode = 100;
	    if (NOT systemObjs)
	      {
		if (fetchAllRows(infoList_, queryBuf_, 1, FALSE, retcode) < 0)
		  {
		    step_ = HANDLE_ERROR_;

		    break;
		  }
	      }

	    // insert a NULL entry, this will cause a blank row to be returned
	    if (retcode != 100) // some rows were found
	      {
		infoList_->insert((new(getHeap()) OutputInfo(1)));
		rowsFound = TRUE;
	      }

            // get indexes, mvs, synonyms for trafodion catalog
            if (strcmp(getMItdb().getCat(), TRAFODION_SYSCAT_LIT) == 0)
              {
                // Get indexes in schema
                str_sprintf(queryBuf_, "get indexes in schema \"%s\".\"%s\" %s",
                            getMItdb().getCat(), getMItdb().getSch(),
                            patternStr_);
                
                retcode = 100;
                if (NOT systemObjs)
                  {
                    if (fetchAllRows(infoList_, queryBuf_, 1, FALSE, retcode) < 0)
                      {
                        step_ = HANDLE_ERROR_;
                        
                        break;
                      }
                  }
                
                // insert a NULL entry, this will cause a blank row to be returned
                if (retcode != 100) // some rows were found
                  {
                    infoList_->insert((new(getHeap()) OutputInfo(1)));
                    rowsFound = TRUE;
                  }

                // Get mvs in schema
                str_sprintf(queryBuf_, "get mvs in schema \"%s\".\"%s\" %s",
                            getMItdb().getCat(), getMItdb().getSch(),
                            patternStr_);
                
                retcode = 100;
                if (NOT systemObjs)
                  {
                    if (fetchAllRows(infoList_, queryBuf_, 1, FALSE, retcode) < 0)
                      {
                        // if error is 4222 (command not supported), ignore it.
                        if (getDiagsArea() != NULL) {
                          if (getDiagsArea()->mainSQLCODE() != -4222)
                          {
                            step_ = HANDLE_ERROR_;
                            
                            break;
                          }
                          getDiagsArea()->clear();
                        }
                      }
                  }
                
                // insert a NULL entry, this will cause a blank row to be returned
                if (retcode != 100) // some rows were found
                  {
                    infoList_->insert((new(getHeap()) OutputInfo(1)));
                    rowsFound = TRUE;
                  }
                
                // Get synonyms in schema
                str_sprintf(queryBuf_, "get synonyms in schema \"%s\".\"%s\" %s",
                            getMItdb().getCat(), getMItdb().getSch(),
                            patternStr_);
                
                retcode = 100;
                if (NOT systemObjs)
                  {
                    if (fetchAllRows(infoList_, queryBuf_, 1, FALSE, retcode) < 0)
                      {
                        // if error is 4222 (command not supported), ignore it.
                        if (getDiagsArea() != NULL) {
                           if (getDiagsArea()->mainSQLCODE() != -4222)
                           {
                             step_ = HANDLE_ERROR_;
                             break;
                           }
                           getDiagsArea()->clear();
                        }
                      }
                  }
                
                // insert a NULL entry, this will cause a blank row to be returned
                if (retcode != 100) // some rows were found
                  {
                    infoList_->insert((new(getHeap()) OutputInfo(1)));
                    rowsFound = TRUE;
                  }
              } // not HIVE catalog

	    if (rowsFound)
	      infoList_->removeTail();

	    infoList_->position();

	    step_ = RETURN_ROW_;
	  }
	break;

	case DISPLAY_HEADING_:
	  {
	    if (infoList_->atEnd())
	      {
		step_ = DONE_;
		break;
	      }

	    retcode = displayHeading();
	    if (retcode == 1)
	      return WORK_OK;
	    else if (retcode < 0)
	      {
		step_ = HANDLE_ERROR_;
		break;
	      }

	    step_ = RETURN_ROW_;
	  }
	break;

	case RETURN_ROW_:
	  {
	    if (infoList_->atEnd())
	      {
		step_ = DONE_;
		break;
	      }

	    if (qparent_.up->isFull())
	      return WORK_OK;

	    OutputInfo * vi = (OutputInfo*)infoList_->getCurr();

	    short rc = 0;
	    char * ptr = vi->get(0);
	    short len = (short)(ptr ? strlen(ptr) : 0);

	    if (ptr)
	      moveRowToUpQueue(ptr, len, &rc);
	    else
	      moveRowToUpQueue(" ", 0, &rc);

	    infoList_->advance();
            incReturnRowCount();
	  }
	break;

	case HANDLE_ERROR_:
	  {
	    retcode = handleError();
	    if (retcode == 1)
	      return WORK_OK;

	    step_ = DONE_;
	  }
	break;

	case DONE_:
	  {
            if (NOT getMItdb().noHeader() && getReturnRowCount() > 0)
            {
              short rc = 0;
              char returnMsg[256];
              memset(returnMsg, 0, 256);
              sprintf(returnMsg, "\n=======================\n %d row(s) returned", getReturnRowCount());
              moveRowToUpQueue(returnMsg, strlen(returnMsg), &rc);
            }
	    retcode = handleDone();
	    if (retcode == 1)
	      return WORK_OK;

	    step_ = INITIAL_;

	    return WORK_OK;
	  }
	break;

	}
    }

  return 0;
}

////////////////////////////////////////////////////////////////
// Constructor for class ExExeUtilGetHbaseObjectsTcb
///////////////////////////////////////////////////////////////
ExExeUtilGetHbaseObjectsTcb::ExExeUtilGetHbaseObjectsTcb(
     const ComTdbExeUtilGetMetadataInfo & exe_util_tdb,
     ex_globals * glob)
     : ExExeUtilGetMetadataInfoTcb( exe_util_tdb, glob)
{
  ehi_ = ExpHbaseInterface::newInstance(glob->getDefaultHeap(),
					(char*)exe_util_tdb.server(), 
					(char*)exe_util_tdb.zkPort());

  hbaseName_ = NULL;
  hbaseNameBuf_ = new(getGlobals()->getDefaultHeap()) 
    char[ComMAX_3_PART_EXTERNAL_UTF8_NAME_LEN_IN_BYTES+6+1];
  outBuf_ = new(getGlobals()->getDefaultHeap())
    char[ComMAX_3_PART_EXTERNAL_UTF8_NAME_LEN_IN_BYTES+6+1];

  hbaseTables_ = NULL;
}

ExExeUtilGetHbaseObjectsTcb::~ExExeUtilGetHbaseObjectsTcb()
{
  if (ehi_)
    delete ehi_;

  if (hbaseNameBuf_)
    NADELETEBASIC(hbaseNameBuf_, getGlobals()->getDefaultHeap());

  if (outBuf_)
    NADELETEBASIC(outBuf_, getGlobals()->getDefaultHeap());
}

//////////////////////////////////////////////////////
// work() for ExExeUtilGetHbaseObjectsTcb
//////////////////////////////////////////////////////
short ExExeUtilGetHbaseObjectsTcb::work()
{
  short retcode = 0;
  Lng32 cliRC = 0;
  ex_expr::exp_return_type exprRetCode = ex_expr::EXPR_OK;

 // if no parent request, return
  if (qparent_.down->isEmpty())
    return WORK_OK;

  // if no room in up queue, won't be able to return data/status.
  // Come back later.
  if (qparent_.up->isFull())
    return WORK_OK;
  
  ex_queue_entry * pentry_down = qparent_.down->getHeadEntry();
  ExExeUtilPrivateState & pstate =
    *((ExExeUtilPrivateState*) pentry_down->pstate);
  
  // Get the globals stucture of the master executor.
  ExExeStmtGlobals *exeGlob = getGlobals()->castToExExeStmtGlobals();
  ExMasterStmtGlobals *masterGlob = exeGlob->castToExMasterStmtGlobals();
  ContextCli * currContext = masterGlob->getStatement()->getContext();
  
  while (1)
    {
      switch (step_)
	{
	case INITIAL_:
	  {
            if (ehi_ == NULL)
              {
                step_ = HANDLE_ERROR_;
                break;
              }

            step_ = SETUP_HBASE_QUERY_;
	  }
	break;

        case SETUP_HBASE_QUERY_:
          {
            // Since HBase tables are native and Trafodion does not manage them
            // limit who can view these objects
            if (((currContext->getSqlParserFlags() & 0x20000) == 0) &&
                !ComUser::isRootUserID() && 
                !ComUser::currentUserHasRole(ROOT_ROLE_ID) &&
                !ComUser::currentUserHasRole(HBASE_ROLE_ID))
              {
                step_ = DONE_;
                break;
              }
            hbaseTables_ = ehi_->listAll("");
            if (! hbaseTables_)
              {
                step_ = HANDLE_ERROR_;
                break;
              }

            currIndex_ = 0;

            if (currIndex_ == hbaseTables_->entries())
              {
                step_ = DONE_;
                break;
              }

            step_ = DISPLAY_HEADING_;
          }
          break;

        case DISPLAY_HEADING_:
          {
            retcode = displayHeading();
            if (retcode == 1)
              return WORK_OK;
            else if (retcode < 0)
              {
                step_ = HANDLE_ERROR_;
                break;
              }

            headingReturned_ = TRUE;

            step_ = PROCESS_NEXT_ROW_;
          }
        break;

        case PROCESS_NEXT_ROW_:
          {
            if (currIndex_ == hbaseTables_->entries())
              {
                step_ = DONE_;
                break;
              }

            HbaseStr *hbaseName = &hbaseTables_->at(currIndex_);
            if (hbaseName->len > ComMAX_3_PART_EXTERNAL_UTF8_NAME_LEN_IN_BYTES+6)
                hbaseName->len = ComMAX_3_PART_EXTERNAL_UTF8_NAME_LEN_IN_BYTES+6; 
            strncpy(hbaseNameBuf_, hbaseName->val, hbaseName->len);
            hbaseNameBuf_[hbaseName->len] = 0;
            hbaseName_ = hbaseNameBuf_;

            Lng32 numParts = 0;
            char *parts[4];
            LateNameInfo::extractParts(hbaseName_, outBuf_, numParts, parts, FALSE);

            currIndex_++;

            if (getMItdb().allObjs())
              {
                step_ = EVAL_EXPR_;
                break;
              }

            NABoolean sysObj = FALSE;
            NABoolean externalObj = FALSE;

            // only trafodion objects will be returned. They are 3-part name that
            // start with TRAFODION.
            if (numParts != 3)
              {
                externalObj = TRUE;
              }
            else
              {
                NAString catalogNamePart(parts[0]);
                NAString schemaNamePart(parts[1]);
                NAString objectNamePart(parts[2]);
                
                if (catalogNamePart != TRAFODION_SYSCAT_LIT)
                  {
                    externalObj = TRUE;
                  }
                else
                  {
                    if (ComIsTrafodionReservedSchema("", catalogNamePart, schemaNamePart))
                      {
                        sysObj = TRUE;
                      }
                  }
              }

            if ((getMItdb().externalObjs()) &&
                (externalObj))
              {
                step_ = EVAL_EXPR_;
                break;
              }
            else if ((getMItdb().systemObjs()) &&
                (sysObj))
              {
                step_ = EVAL_EXPR_;
                break;
              }
            else if ((getMItdb().userObjs()) &&
                     ((NOT sysObj) && (NOT externalObj)))
             {
                step_ = EVAL_EXPR_;
                break;
              }
 
            step_ = PROCESS_NEXT_ROW_;
          }
          break;

        case EVAL_EXPR_:
          {
            exprRetCode = evalScanExpr(hbaseName_, strlen(hbaseName_), TRUE);
	    if (exprRetCode == ex_expr::EXPR_FALSE)
	      {
		// row does not pass the scan expression,
		// move to the next row.
		step_ = PROCESS_NEXT_ROW_;
		break;
	      }
            
            step_ = RETURN_ROW_;
          }
          break;

        case RETURN_ROW_:
          {
	    if (qparent_.up->isFull())
	      return WORK_OK;

	    short rc = 0;
	    moveRowToUpQueue(hbaseName_, 0, &rc);

            step_ = PROCESS_NEXT_ROW_;
          }
          break;

	case HANDLE_ERROR_:
	  {
	    retcode = handleError();
	    if (retcode == 1)
	      return WORK_OK;

	    step_ = DONE_;
	  }
	break;

	case DONE_:
	  {
            if (hbaseTables_ != NULL) {
               deleteNAArray(getHeap(), hbaseTables_);
               hbaseTables_ = NULL;
            }
	    retcode = handleDone();
	    if (retcode == 1)
	      return WORK_OK;

	    step_ = INITIAL_;

	    return WORK_OK;
	  }
	break;
        }
    }

  return WORK_OK;
}

///////////////////////////////////////////////////////////////////
ex_tcb * ExExeUtilGetHiveMetadataInfoTdb::build(ex_globals * glob)
{
  ExExeUtilGetHiveMetadataInfoTcb * exe_util_tcb;

  exe_util_tcb =
    new(glob->getSpace()) ExExeUtilGetHiveMetadataInfoTcb(*this, glob);
  
  exe_util_tcb->registerSubtasks();

  return (exe_util_tcb);
}

////////////////////////////////////////////////////////////////
// Constructor for class ExExeUtilGetMetadataInfoTcb
///////////////////////////////////////////////////////////////
ExExeUtilGetHiveMetadataInfoTcb::ExExeUtilGetHiveMetadataInfoTcb(
     const ComTdbExeUtilGetHiveMetadataInfo & exe_util_tdb,
     ex_globals * glob)
     : ExExeUtilGetMetadataInfoTcb( exe_util_tdb, glob)
{
  queryBuf_ = new(glob->getDefaultHeap()) char[4096];
}

ExExeUtilGetHiveMetadataInfoTcb::~ExExeUtilGetHiveMetadataInfoTcb()
{
}

//////////////////////////////////////////////////////
// work() for ExExeUtilGetHiveMetadataInfoTcb
//////////////////////////////////////////////////////
short ExExeUtilGetHiveMetadataInfoTcb::fetchAllHiveRows(Queue * &infoList, 
							Lng32 numOutputEntries,
							short &rc)
{
  Lng32 cliRC = 0;
  rc = 0;

  char buf[2000];
  char wherePred[400];
  if ((getMItdb().queryType() == ComTdbExeUtilGetMetadataInfo::TABLES_IN_SCHEMA_) ||
      (getMItdb().queryType() == ComTdbExeUtilGetMetadataInfo::TABLES_IN_CATALOG_))
    strcpy(wherePred, " where hive_table_type = 'MANAGED_TABLE' or hive_table_type = 'EXTERNAL_TABLE' ");
  else if ((getMItdb().queryType() == ComTdbExeUtilGetMetadataInfo::VIEWS_IN_SCHEMA_) ||
           (getMItdb().queryType() == ComTdbExeUtilGetMetadataInfo::VIEWS_IN_CATALOG_))
    strcpy(wherePred, " where hive_table_type = 'VIRTUAL_VIEW' ");
  else if ((getMItdb().queryType() == ComTdbExeUtilGetMetadataInfo::OBJECTS_IN_SCHEMA_) ||
           (getMItdb().queryType() == ComTdbExeUtilGetMetadataInfo::OBJECTS_IN_CATALOG_) ||
           (getMItdb().queryType() == ComTdbExeUtilGetMetadataInfo::SCHEMAS_IN_CATALOG_))
    strcpy(wherePred, " ");

  if (getMItdb().queryType() == ComTdbExeUtilGetMetadataInfo::SCHEMAS_IN_CATALOG_)
    str_sprintf(buf, "select trim(schema_name) from table(hivemd(schemas)) group by 1 order by 1");
  else if (getMItdb().getSch())
    str_sprintf(buf, "select rtrim(table_name) from table(hivemd(tables, \"%s\")) %s order by 1", 
                getMItdb().getSch(),
                wherePred);
  else
    str_sprintf(buf, "select trim(schema_name) || '.' || trim(table_name) from table(hivemd(tables)) %s order by 1", 
                wherePred);
    
  cliRC = fetchAllRows(infoList, buf, 1, TRUE, rc, FALSE);

  return cliRC;
}
  
short ExExeUtilGetHiveMetadataInfoTcb::work()
{
  short retcode = 0;
  Lng32 cliRC = 0;
  ex_expr::exp_return_type exprRetCode = ex_expr::EXPR_OK;

  // if no parent request, return
  if (qparent_.down->isEmpty())
    return WORK_OK;

  // if no room in up queue, won't be able to return data/status.
  // Come back later.
  if (qparent_.up->isFull())
    return WORK_OK;

  ex_queue_entry * pentry_down = qparent_.down->getHeadEntry();
  ExExeUtilPrivateState & pstate =
    *((ExExeUtilPrivateState*) pentry_down->pstate);

  // Get the globals stucture of the master executor.
  ExExeStmtGlobals *exeGlob = getGlobals()->castToExExeStmtGlobals();
  ExMasterStmtGlobals *masterGlob = exeGlob->castToExMasterStmtGlobals();
  ContextCli * currContext = masterGlob->getStatement()->getContext();

  while (1)
    {
      switch (step_)
	{
	case INITIAL_:
	  {
	    step_ = FETCH_ALL_ROWS_;
	  }
	  break;

	case FETCH_ALL_ROWS_:
	  {
	    if (initializeInfoList(infoList_))
	      {
		step_ = HANDLE_ERROR_;
		break;
	      }
	    
            // Since Hive tables are native and Trafodion does not manage them
            // limit the users that can see the data.
            if (((currContext->getSqlParserFlags() & 0x20000) == 0) &&
                !ComUser::isRootUserID() && 
                !ComUser::currentUserHasRole(ROOT_ROLE_ID) &&
                !ComUser::currentUserHasRole(HIVE_ROLE_ID))
              {
                step_ = DONE_;
                break;
              }

	    short rc = 0;
	    retcode = fetchAllHiveRows(infoList_, 1, rc);
	    if (retcode < 0)
	      {
                cliInterface()->allocAndRetrieveSQLDiagnostics(diagsArea_);
		step_ = HANDLE_ERROR_;
		break;
	      }

	    infoList_->position();

	    headingReturned_ = FALSE;

	    step_ = RETURN_ROW_;
	  }
	  break;

	case RETURN_ROW_:
	  {
	    if (infoList_->atEnd())
	      {
		step_ = DONE_;
		break;
	      }

	    if (qparent_.up->isFull())
	      return WORK_OK;

	    OutputInfo * vi = (OutputInfo*)infoList_->getCurr();
	    char * ptr = vi->get(0);
	    short len = *(short*)ptr;
 
	    ex_expr::exp_return_type exprRetCode = 
              exprRetCode = evalScanExpr(ptr, len, FALSE);
	    if (exprRetCode == ex_expr::EXPR_FALSE)
	      {
		// row does not pass the scan expression,
		// move to the next row.
                infoList_->advance();
		break;
	      }
	    else if (exprRetCode == ex_expr::EXPR_ERROR)
	      {
		step_ = HANDLE_ERROR_;
		break;
	      }

	    if (NOT headingReturned_)
	      {
		step_ = DISPLAY_HEADING_;
		break;
	      }

	    short rc = 0;
	    ptr += SQL_VARCHAR_HDR_SIZE;
	    if (moveRowToUpQueue(ptr, len, &rc))
            {
	      return rc;
            }

	    infoList_->advance();
            incReturnRowCount();
	  }
	  break;

	case DISPLAY_HEADING_:
	  {
	    retcode = displayHeading();
	    if (retcode == 1)
	      return WORK_OK;
	    else if (retcode < 0)
	      {
		step_ = HANDLE_ERROR_;
		break;
	      }

	    headingReturned_ = TRUE;

	    step_ = RETURN_ROW_;
	  }
	break;

	case HANDLE_ERROR_:
	  {
	    retcode = handleError();
	    if (retcode == 1)
	      return WORK_OK;

	    step_ = DONE_;
	  }
	break;

	case DONE_:
	  {
	    retcode = handleDone();
	    if (retcode == 1)
	      return WORK_OK;

	    step_ = INITIAL_;

	    return WORK_OK;
	  }
	break;

	} // switch
    } // while
}

////////////////////////////////////////////////////////////////////////
// Redefine virtual method allocatePstates, to be used by dynamic queue
// resizing, as well as the initial queue construction.
////////////////////////////////////////////////////////////////////////
ex_tcb_private_state * ExExeUtilGetMetadataInfoTcb::allocatePstates(
     Lng32 &numElems,      // inout, desired/actual elements
     Lng32 &pstateLength)  // out, length of one element
{
  PstateAllocator<ExExeUtilGetMetadataInfoPrivateState> pa;

  return pa.allocatePstates(this, numElems, pstateLength);
}


/////////////////////////////////////////////////////////////////////////////
// Constructor and destructor for ExeUtil_private_state
/////////////////////////////////////////////////////////////////////////////
ExExeUtilGetMetadataInfoPrivateState::ExExeUtilGetMetadataInfoPrivateState()
{
}

ExExeUtilGetMetadataInfoPrivateState::~ExExeUtilGetMetadataInfoPrivateState()
{
};



///////////////////////////////////////////////////////////////////
// class ExExeUtilShowSetTdb
///////////////////////////////////////////////////////////////
ex_tcb * ExExeUtilShowSetTdb::build(ex_globals * glob)
{
  ExExeUtilShowSetTcb * exe_util_tcb;

  exe_util_tcb = new(glob->getSpace()) ExExeUtilShowSetTcb(*this, glob);

  exe_util_tcb->registerSubtasks();

  return (exe_util_tcb);
}

ExExeUtilShowSetTcb::ExExeUtilShowSetTcb(const ComTdbExeUtilShowSet & exe_util_tdb,
					 ex_globals * glob)
     : ExExeUtilTcb(exe_util_tdb, NULL, glob),
       step_(EMPTY_)
{
}

short ExExeUtilShowSetTcb::work()
{
  // if no parent request, return
  if (qparent_.down->isEmpty())
    return WORK_OK;

  ex_queue_entry * pentry_down = qparent_.down->getHeadEntry();

  ContextCli *currContext =
      getGlobals()->castToExExeStmtGlobals()->castToExMasterStmtGlobals()->
        getStatement()->getContext();

  SessionDefaults * sd = currContext->getSessionDefaults();

  while (1)
    {
      switch (step_)
	{
	case EMPTY_:
	  {
	    sd->position();

	    step_ = RETURN_HEADER_;
	  }
	break;

	case RETURN_HEADER_:
	  {
	    // if no room in up queue for 2 display rows,
	    // won't be able to return data/status.
	    // Come back later.
	    if ((qparent_.up->getSize() - qparent_.up->getLength()) < 3)
	      return -1;

	    moveRowToUpQueue("  ");
	    moveRowToUpQueue("Current SESSION DEFAULTs");

	    step_ = RETURNING_DEFAULT_;
	  }
	break;

	case RETURNING_DEFAULT_:
	  {
	    // if no room in up queue, won't be able to return data/status.
	    // Come back later.
	    if (qparent_.up->isFull())
	      return WORK_OK;

	    char * attributeString = NULL;
	    char * attributeValue  = NULL;
	    Lng32 isCQD;
	    Lng32 fromDefaultsTable;
	    Lng32 isSSD;
	    Lng32 isExternalized = 0;
	    Lng32 eof = 0;
	    while ((NOT eof) && (NOT isExternalized))
	      {
		eof = sd->getNextSessionDefault(attributeString,
						attributeValue,
						isCQD,
						fromDefaultsTable,
						isSSD,
						isExternalized);

		if (ssTdb().getType() == ComTdbExeUtilShowSet::ALL_)
		  isExternalized = TRUE;
	      }

	    if (eof)
	      {
		step_ = DONE_;
		break;
	      }

	    char formattedStr[2000];
	    strcpy(formattedStr, "  ");
	    byte_str_cpy(&formattedStr[2], 28, attributeString,
			 strlen(attributeString),' ');
	    formattedStr[2+28] = 0;

	    if (attributeValue)
	      strcat(formattedStr, attributeValue);

	    moveRowToUpQueue(formattedStr);
	  }
	break;

	case DONE_:
	  {
	    if (qparent_.up->isFull())
	      return WORK_OK;

	    // all ok. Return EOF.
	    ex_queue_entry * up_entry = qparent_.up->getTailEntry();

	    up_entry->upState.parentIndex =
	      pentry_down->downState.parentIndex;

	    up_entry->upState.setMatchNo(0);
	    up_entry->upState.status = ex_queue::Q_NO_DATA;

	    // insert into parent
	    qparent_.up->insert();

	    step_ = EMPTY_;

	    qparent_.down->removeHead();

	    return WORK_OK;
	  }
	  break;

	} // switch
    }

  return 0;
}


///////////////////////////////////////////////////////////////////
ex_tcb * ExExeUtilGetUIDTdb::build(ex_globals * glob)
{
  ex_tcb * exe_util_tcb;

  exe_util_tcb = new(glob->getSpace()) ExExeUtilGetUIDTcb(*this, glob);

  exe_util_tcb->registerSubtasks();

  return (exe_util_tcb);
}

////////////////////////////////////////////////////////////////
// Constructor for class ExExeUtilGetUIDTcb
///////////////////////////////////////////////////////////////
ExExeUtilGetUIDTcb::ExExeUtilGetUIDTcb(
     const ComTdbExeUtilGetUID & exe_util_tdb,
     ex_globals * glob)
     : ExExeUtilTcb( exe_util_tdb, NULL, glob)
{
  // Allocate the private state in each entry of the down queue
  qparent_.down->allocatePstate(this);

  step_ = INITIAL_;

}

ExExeUtilGetUIDTcb::~ExExeUtilGetUIDTcb()
{
}

//////////////////////////////////////////////////////
// work() for ExExeUtilGetUIDTcb
//////////////////////////////////////////////////////
short ExExeUtilGetUIDTcb::work()
{
  //  short rc = 0;
  Lng32 cliRC = 0;

  // if no parent request, return
  if (qparent_.down->isEmpty())
    return WORK_OK;

  // if no room in up queue, won't be able to return data/status.
  // Come back later.
  if (qparent_.up->isFull())
    return WORK_OK;

  ex_queue_entry * pentry_down = qparent_.down->getHeadEntry();
  ExExeUtilPrivateState & pstate =
    *((ExExeUtilPrivateState*) pentry_down->pstate);

  // Get the globals stucture of the master executor.
  ExExeStmtGlobals *exeGlob = getGlobals()->castToExExeStmtGlobals();
  ExMasterStmtGlobals *masterGlob = exeGlob->castToExMasterStmtGlobals();
  ContextCli * currContext = masterGlob->getStatement()->getContext();

  while (1)
    {
      switch (step_)
	{
	case INITIAL_:
	  {
	    step_ = RETURN_UID_;
	  }
	break;

	case RETURN_UID_:
	  {
	    if (qparent_.up->isFull())
	      return WORK_OK;

	    moveRowToUpQueue((const char*)&(getUIDTdb().uid_),
			     sizeof(getUIDTdb().uid_),
			     NULL, FALSE);

	    step_ = DONE_;
	  }
	break;
      case DONE_:
	  {
	    if (qparent_.up->isFull())
	      return WORK_OK;

	    // Return EOF.
	    ex_queue_entry * up_entry = qparent_.up->getTailEntry();

	    up_entry->upState.parentIndex =
	      pentry_down->downState.parentIndex;

	    up_entry->upState.setMatchNo(0);
	    up_entry->upState.status = ex_queue::Q_NO_DATA;

	    // insert into parent
	    qparent_.up->insert();

	    qparent_.down->removeHead();

	    step_ = INITIAL_;
	    return WORK_OK;
	  }

	break;

	default:
	  break;

	}

    }

  return 0;
}


////////////////////////////////////////////////////////////////////////
// Redefine virtual method allocatePstates, to be used by dynamic queue
// resizing, as well as the initial queue construction.
////////////////////////////////////////////////////////////////////////
ex_tcb_private_state * ExExeUtilGetUIDTcb::allocatePstates(
     Lng32 &numElems,      // inout, desired/actual elements
     Lng32 &pstateLength)  // out, length of one element
{
  PstateAllocator<ExExeUtilGetUIDPrivateState> pa;

  return pa.allocatePstates(this, numElems, pstateLength);
}

/////////////////////////////////////////////////////////////////////////////
// Constructor and destructor for ExeUtil_private_state
/////////////////////////////////////////////////////////////////////////////
ExExeUtilGetUIDPrivateState::ExExeUtilGetUIDPrivateState()
{
}

ExExeUtilGetUIDPrivateState::~ExExeUtilGetUIDPrivateState()
{
};

///////////////////////////////////////////////////////////////////
ex_tcb * ExExeUtilGetQIDTdb::build(ex_globals * glob)
{
  ex_tcb * exe_util_tcb;

  exe_util_tcb = new(glob->getSpace()) ExExeUtilGetQIDTcb(*this, glob);

  exe_util_tcb->registerSubtasks();

  return (exe_util_tcb);
}

////////////////////////////////////////////////////////////////
// Constructor for class ExExeUtilGetQIDTcb
///////////////////////////////////////////////////////////////
ExExeUtilGetQIDTcb::ExExeUtilGetQIDTcb(
     const ComTdbExeUtilGetQID & exe_util_tdb,
     ex_globals * glob)
     : ExExeUtilTcb( exe_util_tdb, NULL, glob)
{
  // Allocate the private state in each entry of the down queue
  qparent_.down->allocatePstate(this);

  step_ = INITIAL_;

}

ExExeUtilGetQIDTcb::~ExExeUtilGetQIDTcb()
{
}

//////////////////////////////////////////////////////
// work() for ExExeUtilGetQIDTcb
//////////////////////////////////////////////////////
short ExExeUtilGetQIDTcb::work()
{
  short retcode = 0;
  Lng32 cliRC = 0;

  // if no parent request, return
  if (qparent_.down->isEmpty())
    return WORK_OK;

  // if no room in up queue, won't be able to return data/status.
  // Come back later.
  if (qparent_.up->isFull())
    return WORK_OK;

  ex_queue_entry * pentry_down = qparent_.down->getHeadEntry();
  ExExeUtilPrivateState & pstate =
    *((ExExeUtilPrivateState*) pentry_down->pstate);

  // Get the globals stucture of the master executor.
  ExExeStmtGlobals *exeGlob = getGlobals()->castToExExeStmtGlobals();
  ExMasterStmtGlobals *masterGlob = exeGlob->castToExMasterStmtGlobals();
  ContextCli * currContext = masterGlob->getStatement()->getContext();

  while (1)
    {
      switch (step_)
	{
	case INITIAL_:
	  {
	    step_ = RETURN_QID_;
	  }
	break;

	case RETURN_QID_:
	  {
	    if (qparent_.up->isFull())
	      return WORK_OK;

            /* get statement from context */
            SQLMODULE_ID module;
            init_SQLMODULE_ID(&module);

            SQLSTMT_ID stmtId;
            memset (&stmtId, 0, sizeof(SQLSTMT_ID));

            // Allocate a SQL statement
            init_SQLSTMT_ID(&stmtId, SQLCLI_CURRENT_VERSION, stmt_name, 
                            &module, getQIDTdb().getStmtName(), NULL, NULL, 
                            strlen(getQIDTdb().getStmtName()));

            Statement * stmt = currContext->getStatement(&stmtId);
            
            /* stmt must exist */
            if (!stmt)
              {
                ExRaiseSqlError(getHeap(), &diagsArea_, -CLI_STMT_NOT_EXISTS);
                step_ = ERROR_;
                break;
              }
            
	    moveRowToUpQueue(stmt->getUniqueStmtId());

	    step_ = DONE_;
	  }
	break;

	case ERROR_:
	  {
	    retcode = handleError();
	    if (retcode == 1)
	      return WORK_OK;

	    step_ = DONE_;
	  }
	break;

	case DONE_:
	  {
	    retcode = handleDone();
	    if (retcode == 1)
	      return WORK_OK;

	    step_ = INITIAL_;
	    return WORK_OK;
	  }

	break;

	default:
	  break;

	}

    }

  return 0;
}


////////////////////////////////////////////////////////////////////////
// Redefine virtual method allocatePstates, to be used by dynamic queue
// resizing, as well as the initial queue construction.
////////////////////////////////////////////////////////////////////////
ex_tcb_private_state * ExExeUtilGetQIDTcb::allocatePstates(
     Lng32 &numElems,      // inout, desired/actual elements
     Lng32 &pstateLength)  // out, length of one element
{
  PstateAllocator<ExExeUtilGetQIDPrivateState> pa;

  return pa.allocatePstates(this, numElems, pstateLength);
}

/////////////////////////////////////////////////////////////////////////////
// Constructor and destructor for ExeUtil_private_state
/////////////////////////////////////////////////////////////////////////////
ExExeUtilGetQIDPrivateState::ExExeUtilGetQIDPrivateState()
{
}

ExExeUtilGetQIDPrivateState::~ExExeUtilGetQIDPrivateState()
{
};

///////////////////////////////////////////////////////////////////
ex_tcb * ExExeUtilGetErrorInfoTdb::build(ex_globals * glob)
{
  ExExeUtilGetErrorInfoTcb * exe_util_tcb;

  exe_util_tcb =
    new(glob->getSpace()) ExExeUtilGetErrorInfoTcb(*this, glob);

  exe_util_tcb->registerSubtasks();

  return (exe_util_tcb);
}

////////////////////////////////////////////////////////////////
// Constructor for class ExExeUtilGetErrorInfoTcb
///////////////////////////////////////////////////////////////
ExExeUtilGetErrorInfoTcb::ExExeUtilGetErrorInfoTcb(
     const ComTdbExeUtilGetErrorInfo & exe_util_tdb,
     ex_globals * glob)
     : ExExeUtilTcb( exe_util_tdb, NULL, glob)
{
  // Allocate the private state in each entry of the down queue
  qparent_.down->allocatePstate(this);

  step_ = INITIAL_;

  // buffer where output will be formatted
  outputBuf_ = new(glob->getDefaultHeap()) char[4096];
}

//////////////////////////////////////////////////////
// work() for ExExeUtilGetErrorInfoTcb
//////////////////////////////////////////////////////
short ExExeUtilGetErrorInfoTcb::work()
{
  short retcode = 0;
  Lng32 cliRC = 0;
  ex_expr::exp_return_type exprRetCode = ex_expr::EXPR_OK;

  // if no parent request, return
  if (qparent_.down->isEmpty())
    return WORK_OK;

  // if no room in up queue, won't be able to return data/status.
  // Come back later.
  if (qparent_.up->isFull())
    return WORK_OK;

  ex_queue_entry * pentry_down = qparent_.down->getHeadEntry();
  ExExeUtilPrivateState & pstate =
    *((ExExeUtilPrivateState*) pentry_down->pstate);

  // Get the globals stucture of the master executor.
  ExExeStmtGlobals *exeGlob = getGlobals()->castToExExeStmtGlobals();
  ExMasterStmtGlobals *masterGlob = exeGlob->castToExMasterStmtGlobals();
  ContextCli * currContext = masterGlob->getStatement()->getContext();

  while (1)
    {
      switch (step_)
	{
	case INITIAL_:
	  {
	    step_ = RETURN_TEXT_;
	  }
	break;

	case RETURN_TEXT_:
	  {
	    if ((qparent_.up->getSize() - qparent_.up->getLength()) < 10)
	      return WORK_OK;

	    Lng32 warnNum = ABS(geiTdb().errNum_);
	    Lng32 errNum = -geiTdb().errNum_;
	    
	    char sqlstateErr[10];
	    char sqlstateWarn[10];

	    ComSQLSTATE(errNum, sqlstateErr);
	    ComSQLSTATE(warnNum, sqlstateWarn);

	    NAWchar * errorMsg;
	    NABoolean msgNotFound = 
	      GetErrorMessage(errNum, errorMsg, ERROR_TEXT);

	    Lng32 bufSize = 2 * ErrorMessage::MSG_BUF_SIZE + 16;
	    char * isoErrorMsg = new(getGlobals()->getDefaultHeap()) 
	      char[bufSize];

	    moveRowToUpQueue("");

	    if ((! msgNotFound) || (errNum == 0))
	      {	      

		UnicodeStringToLocale
		  (CharInfo::ISO88591,
		   errorMsg, NAWstrlen(errorMsg), isoErrorMsg, bufSize);

		   str_sprintf(outputBuf_, "*** SQLSTATE (Err): %s SQLSTATE (Warn): %s", 
			    sqlstateErr, sqlstateWarn);
		moveRowToUpQueue(outputBuf_);

		str_sprintf(outputBuf_, "%s", isoErrorMsg);
		moveRowToUpQueue(outputBuf_);
	      }
	    else
	      {
		str_sprintf(outputBuf_, "*** WARNING[%d]",
			    warnNum);
		moveRowToUpQueue(outputBuf_);

		str_sprintf(outputBuf_, "*** ERROR[16001] The error number %d is not used in SQL.",
			    warnNum);
		moveRowToUpQueue(outputBuf_);
	      }

	    NADELETEBASIC(isoErrorMsg, getGlobals()->getDefaultHeap());

	    step_ = DONE_;
	  }
	  break;

	case DONE_:
	  {
	    if (qparent_.up->isFull())
	      return WORK_OK;

	    // Return EOF.
	    ex_queue_entry * up_entry = qparent_.up->getTailEntry();

	    up_entry->upState.parentIndex =
	      pentry_down->downState.parentIndex;

	    up_entry->upState.setMatchNo(0);
	    up_entry->upState.status = ex_queue::Q_NO_DATA;

	    // insert into parent
	    qparent_.up->insert();

	    qparent_.down->removeHead();

	    step_ = INITIAL_;
	    return WORK_OK;
	  } // case
	  break;
	} // switch
    } // while
}

////////////////////////////////////////////////////////////////////////
// Redefine virtual method allocatePstates, to be used by dynamic queue
// resizing, as well as the initial queue construction.
////////////////////////////////////////////////////////////////////////
ex_tcb_private_state * ExExeUtilGetErrorInfoTcb::allocatePstates(
     Lng32 &numElems,      // inout, desired/actual elements
     Lng32 &pstateLength)  // out, length of one element
{
  PstateAllocator<ExExeUtilGetErrorInfoPrivateState> pa;

  return pa.allocatePstates(this, numElems, pstateLength);
}


/////////////////////////////////////////////////////////////////////////////
// Constructor and destructor for ExeUtil_private_state
/////////////////////////////////////////////////////////////////////////////
ExExeUtilGetErrorInfoPrivateState::ExExeUtilGetErrorInfoPrivateState()
{
}

ExExeUtilGetErrorInfoPrivateState::~ExExeUtilGetErrorInfoPrivateState()
{
}

///////////////////////////////////////////////////////////////////
// class ExExeUtilLobShowddlTdb
///////////////////////////////////////////////////////////////
ex_tcb * ExExeUtilLobShowddlTdb::build(ex_globals * glob)
{
  ExExeUtilLobShowddlTcb * exe_util_tcb;

  exe_util_tcb = new(glob->getSpace()) ExExeUtilLobShowddlTcb(*this, glob);

  exe_util_tcb->registerSubtasks();

  return (exe_util_tcb);
}

ExExeUtilLobShowddlTcb::ExExeUtilLobShowddlTcb
(
 const ComTdbExeUtilLobShowddl & exe_util_tdb,
 ex_globals * glob)
  : ExExeUtilTcb(exe_util_tdb, NULL, glob),
    step_(INITIAL_)
{
  strcpy(lobMDNameBuf_,"");
  lobMDNameLen_=0;
  lobMDName_ = NULL;
  
  Lng32 currLobNum_ = 0;
  strcpy(sdOptionStr_,"");
}

short ExExeUtilLobShowddlTcb::fetchRows(char * query, short &rc)
{
  Lng32 cliRC = 0;

  if (initializeInfoList(infoList_)) 
    {
      step_ = HANDLE_ERROR_;
      
      return -1;
    }
  
  rc = 0;
  cliRC = 
    fetchAllRows(infoList_, query, 1, FALSE, rc);
  if (cliRC < 0) 
    {
      cliInterface()->allocAndRetrieveSQLDiagnostics(diagsArea_);
      step_ = HANDLE_ERROR_;
      return -1;
    }
  
  infoList_->position();
  
  return 0;
}

short ExExeUtilLobShowddlTcb::returnRows(short &rc)
{
  if (infoList_->atEnd())
    {
      return 100;
    }
  
  if (qparent_.up->isFull())
    {
      rc = WORK_OK;
      return -1;
    }
  
  OutputInfo * vi = (OutputInfo*)infoList_->getCurr();
  
  char * ptr = vi->get(0);
  short len = (short)(ptr ? strlen(ptr) : 0);
  
  if (moveRowToUpQueue(ptr, len, &rc))
    {
      return -1;
    }
  
  infoList_->advance();

  return 0;
}

short ExExeUtilLobShowddlTcb::work()
{
  Lng32 cliRC = 0;
  short retcode = 0;

  // if no parent request, return
  if (qparent_.down->isEmpty())
    return WORK_OK;

  // if no room in up queue, won't be able to return data/status.
  // Come back later.
  if (qparent_.up->isFull())
    return WORK_OK;

  ex_queue_entry * pentry_down = qparent_.down->getHeadEntry();
  ExExeUtilPrivateState & pstate =
    *((ExExeUtilPrivateState*) pentry_down->pstate);

  ContextCli *currContext =
    getGlobals()->castToExExeStmtGlobals()->castToExMasterStmtGlobals()->
    getStatement()->getContext();

  while (1)
    {
      switch (step_) 
	{
	case INITIAL_:
	  {
	    query_ = new(getGlobals()->getDefaultHeap()) char[4096];

	    lobMDNameLen_ = 1024;
	    lobMDName_ = 
	      ExpLOBoper::ExpGetLOBMDName
	      (lobTdb().schNameLen_, lobTdb().schName(), lobTdb().objectUID_,
	       lobMDNameBuf_, lobMDNameLen_);

	    strcpy(sdOptionStr_, " ");

	    switch (lobTdb().sdOptions_)
	      {
	      case 4:
		strcpy(sdOptionStr_, ", detail");
		break;

	      case 8:
		strcpy(sdOptionStr_, ", brief");
		break;

	      case 32:
		strcpy(sdOptionStr_, ", external");
		break;

	      case 64:
		strcpy(sdOptionStr_, ", internal");
		break;
	      }

	    step_ = FETCH_TABLE_SHOWDDL_;
	  }
	  break;

	case FETCH_TABLE_SHOWDDL_:
	  {
	    str_sprintf(query_, "showddl %s %s", 
			lobTdb().getTableName(),
			sdOptionStr_);

	    if (fetchRows(query_, retcode))
	      {
		break;
	      }

	    step_ = RETURN_TABLE_SHOWDDL_;
	  }
	  break;

	case RETURN_TABLE_SHOWDDL_:
	  {
	    cliRC = returnRows(retcode);
	    if (cliRC == -1)
	      {
		return retcode;
	      }
	    else if (cliRC == 100)
	      {
		step_ = FETCH_METADATA_SHOWDDL_;
		return WORK_RESCHEDULE_AND_RETURN;
	      }
	  }
	  break;

	case FETCH_METADATA_SHOWDDL_:
	  {
	    if ((qparent_.up->getSize() - qparent_.up->getLength()) < 6)
	      return WORK_OK;	//come back later

	    moveRowToUpQueue("");	    
	    moveRowToUpQueue("LOB Metadata");
	    moveRowToUpQueue("============");

	    str_sprintf(query_, "showddl table(ghost table %s) %s", 
			lobMDName_,
			sdOptionStr_);

	    // set parserflags to allow ghost table
	    currContext->setSqlParserFlags(0x1);

	    cliRC = fetchRows(query_, retcode);

	    currContext->resetSqlParserFlags(0x1);

	    if (cliRC < 0)
	      {
		break;
	      }

	    step_ = RETURN_METADATA_SHOWDDL_;
	  }
	  break;

	case RETURN_METADATA_SHOWDDL_:
	  {
	    cliRC = returnRows(retcode);
	    if (cliRC == -1)
	      {
		return retcode;
	      }
	    else if (cliRC == 100)
	      {
		currLobNum_ = 1;
		step_ = RETURN_LOB_NAME_;
		return WORK_RESCHEDULE_AND_RETURN;
	      }
	  }
	  break;

	case RETURN_LOB_NAME_:
	  {
	    if ((qparent_.up->getSize() - qparent_.up->getLength()) < 15)
	      return WORK_OK;	//come back later

	    if (currLobNum_ > lobTdb().numLOBs())
	      {
		step_ = DONE_;
		break;
	      }

	    moveRowToUpQueue(" ");
	    moveRowToUpQueue("************************************************");
	    str_sprintf(query_, "LobNum: %d", currLobNum_);
	    moveRowToUpQueue(query_);
	    moveRowToUpQueue(" ");

	    moveRowToUpQueue("Data Storage");
	    moveRowToUpQueue("============");
	    moveRowToUpQueue(" ");

	    char tgtLobNameBuf[100];
	    char * tgtLobName = 
	      ExpLOBoper::ExpGetLOBname
	      (lobTdb().objectUID_, lobTdb().getLOBnum(currLobNum_), 
	       tgtLobNameBuf, 100);

	    if (lobTdb().getIsExternalLobCol(currLobNum_))
              str_sprintf(query_, "<External HDFS location>");
            else 
              str_sprintf(query_, "Location: %s", 
			lobTdb().getLOBloc(currLobNum_));
	    moveRowToUpQueue(query_);

            if (lobTdb().getIsExternalLobCol(currLobNum_))
              str_sprintf(query_, "<External HDFS file>");
            else 
              str_sprintf(query_, "DataFile: %s", tgtLobName);
	    moveRowToUpQueue(query_);

	    step_ = FETCH_LOB_DESC_HANDLE_SHOWDDL_;

	    return WORK_RESCHEDULE_AND_RETURN;
	  }
	  break;

	case FETCH_LOB_DESC_HANDLE_SHOWDDL_:
	  {
	    if ((qparent_.up->getSize() - qparent_.up->getLength()) < 6)
	      return WORK_OK;	//come back later

	    moveRowToUpQueue("");	    
	    moveRowToUpQueue("LOB Descriptor Handle");
	    moveRowToUpQueue("=====================");

	    char lobDescNameBuf[1024];
	    Lng32 lobDescNameLen = 1024;
	    char * lobDescName = 
	      ExpLOBoper::ExpGetLOBDescHandleName
	      (lobTdb().schNameLen_, lobTdb().schName(), 
	       lobTdb().objectUID_,
	       lobTdb().getLOBnum(currLobNum_), 
	       lobDescNameBuf, lobDescNameLen);
	    str_sprintf(query_, "showddl table(ghost table %s) %s", 
			lobDescName,
			sdOptionStr_);

	    // set parserflags to allow ghost table
	    currContext->setSqlParserFlags(0x1);

	    cliRC = fetchRows(query_, retcode);

	    currContext->resetSqlParserFlags(0x1);

	    if (cliRC < 0)
	      {
		break;
	      }

	    step_ = RETURN_LOB_DESC_HANDLE_SHOWDDL_;
	  }
	  break;

	case RETURN_LOB_DESC_HANDLE_SHOWDDL_:
	  {
	    cliRC = returnRows(retcode);
	    if (cliRC == -1)
	      {
		return retcode;
	      }
	    else if (cliRC == 100)
	      {
		step_ = FETCH_LOB_DESC_CHUNKS_SHOWDDL_;
		return WORK_RESCHEDULE_AND_RETURN;
	      }
	  }
	  break;

	case FETCH_LOB_DESC_CHUNKS_SHOWDDL_:
	  {
	    if ((qparent_.up->getSize() - qparent_.up->getLength()) < 6)
	      return WORK_OK;	//come back later

	    moveRowToUpQueue("");	    
	    moveRowToUpQueue("LOB Descriptor Chunks");
	    moveRowToUpQueue("=====================");

	    char lobDescNameBuf[1024];
	    Lng32 lobDescNameLen = 1024;
	    char * lobDescName = 
	      ExpLOBoper::ExpGetLOBDescChunksName
	      (lobTdb().schNameLen_, lobTdb().schName(), 
	       lobTdb().objectUID_,
	       lobTdb().getLOBnum(currLobNum_), 
	       lobDescNameBuf, lobDescNameLen);
	    str_sprintf(query_, "showddl table(ghost table %s) %s", 
			lobDescName,
			sdOptionStr_);

	    // set parserflags to allow ghost table
	    currContext->setSqlParserFlags(0x1);

	    cliRC = fetchRows(query_, retcode);

	    currContext->resetSqlParserFlags(0x1);

	    if (cliRC < 0)
	      {
		break;
	      }

	    step_ = RETURN_LOB_DESC_CHUNKS_SHOWDDL_;
	  }
	  break;

	case RETURN_LOB_DESC_CHUNKS_SHOWDDL_:
	  {
	    cliRC = returnRows(retcode);
	    if (cliRC == -1)
	      {
		return retcode;
	      }
	    else if (cliRC == 100)
	      {
		currLobNum_++;
		step_ = RETURN_LOB_NAME_;
		return WORK_RESCHEDULE_AND_RETURN;
	      }
	  }
	  break;

	case HANDLE_ERROR_:
	  {
	    retcode = handleError();
	    if (retcode == 1)
	      return WORK_OK;

	    step_ = DONE_;
	  }
	  break;

	case DONE_:
	  {
	    retcode = handleDone();
	    if (retcode == 1)
	      return WORK_OK;

	    step_ = INITIAL_;
	    return WORK_OK;
	  }
	  break;

	} // switch
    }

  return 0;
}

///////////////////////////////////////////////////////////////////
ex_tcb * ExExeUtilHiveMDaccessTdb::build(ex_globals * glob)
{
  ExExeUtilHiveMDaccessTcb * exe_util_tcb;

  exe_util_tcb =
    new(glob->getSpace()) ExExeUtilHiveMDaccessTcb(*this, glob);

  exe_util_tcb->registerSubtasks();

  return (exe_util_tcb);
}

////////////////////////////////////////////////////////////////
// Constructor for class ExExeUtilHiveMDaccessTcb
///////////////////////////////////////////////////////////////
ExExeUtilHiveMDaccessTcb::ExExeUtilHiveMDaccessTcb(
     const ComTdbExeUtilHiveMDaccess & exe_util_tdb,
     ex_globals * glob)
  : ExExeUtilTcb( exe_util_tdb, NULL, glob),
    hiveMD_(NULL),
    currColDesc_(NULL),
    currKeyDesc_(NULL),
    schNames_(getHeap()),
    tblNames_(getHeap()),
    currSchNum_(0),
    currColNum_(0)
{
  step_ = INITIAL_;

  mdRow_ = new(getHeap()) char[exe_util_tdb.outputRowlen_];
}

ExExeUtilHiveMDaccessTcb::~ExExeUtilHiveMDaccessTcb()
{

}

// should move this method to common dir.
Lng32 ExExeUtilHiveMDaccessTcb::getTypeAttrsFromHiveColType(const char* hiveType,
                                                            NABoolean isORC,
                                                            Lng32 &fstype,
                                                            Lng32 &length,
                                                            Lng32 &precision,
                                                            Lng32 &scale,
                                                            char *sqlType,
                                                            char *displayType,
                                                            char *charset)
{
  short rc = 0;

  fstype = -1;
  length = -1;
  precision = -1;
  scale = -1;
  NAType * nat = NAType::getNATypeForHive(hiveType, getHeap());
  if (nat)
    {
      fstype = nat->getFSDatatype();
      length = nat->getNominalSize();
      precision = nat->getPrecision();
      scale = nat->getScale();

      const char * sdtStr = 
        Descriptor::ansiTypeStrFromFSType(fstype);
      strcpy(sqlType, sdtStr);

      NAString displayTypeNAS;
      rc = nat->getMyTypeAsText(&displayTypeNAS, FALSE, FALSE);
      if (rc)
        {
          delete nat;
          return -1;
        }

      strcpy(displayType, displayTypeNAS.data());

      charset[0] = 0;
      CharInfo::CharSet charSetEnum = nat->getCharSet();
      if (charSetEnum != CharInfo::UnknownCharSet)
        {
          const char * charSetName = CharInfo::getCharSetName(charSetEnum);
          if (charSetName)
            strcpy(charset, charSetName);
        }

      delete nat;
      return 0;
    }

  return -1;
}

short ExExeUtilHiveMDaccessTcb::work()
{
  short retcode = 0;
  Lng32 cliRC = 0;
  NABoolean retStatus = FALSE;
  ex_expr::exp_return_type exprRetCode = ex_expr::EXPR_OK;

  // if no parent request, return
  if (qparent_.down->isEmpty())
    return WORK_OK;

  // if no room in up queue, won't be able to return data/status.
  // Come back later.
  if (qparent_.up->isFull())
    return WORK_OK;

  ex_queue_entry * pentry_down = qparent_.down->getHeadEntry();
  ExExeUtilPrivateState & pstate =
    *((ExExeUtilPrivateState*) pentry_down->pstate);

  // Get the globals stucture of the master executor.
  ExExeStmtGlobals *exeGlob = getGlobals()->castToExExeStmtGlobals();
  ExMasterStmtGlobals *masterGlob = exeGlob->castToExMasterStmtGlobals();
  ContextCli * currContext = masterGlob->getStatement()->getContext();
  
  while (1)
    {
      switch (step_)
	{
	case INITIAL_:
	  {
	    if (hiveMD_)
	      NADELETEBASIC(hiveMD_, getHeap());

            hiveMD_ = new (getHeap()) HiveMetaData((NAHeap *)getHeap());

            if (hiveMDtdb().getCatalog())
              strcpy(hiveCat_, hiveMDtdb().getCatalog());
 
            retStatus = hiveMD_->init();
            if (!retStatus)
              {
                Lng32 intParam1 =  hiveMD_->getErrCode();
                ExRaiseSqlError(getHeap(), &diagsArea_, -1190, 
                       &intParam1, NULL, NULL,
                       hiveMD_->getErrMethodName(),
                       hiveMD_->getErrCodeStr(),
                       hiveMD_->getErrDetail());
                step_ = HANDLE_ERROR_;
                break;
              }

            step_ = SETUP_SCHEMAS_;
	  }
	  break;

        case SETUP_SCHEMAS_:
          {
            schNames_.clear();

	    if ((hiveMDtdb().mdType_ == ComTdbExeUtilHiveMDaccess::SCHEMAS_) ||
                (! hiveMDtdb().getSchema()))
              {
                HVC_RetCode retCode = HiveClient_JNI::getAllSchemas((NAHeap *)getHeap(), schNames_);
                if ((retCode != HVC_OK) && (retCode != HVC_DONE)) 
                  {
		    ExRaiseSqlError(getHeap(), &diagsArea_, -1190, 
                           (Lng32 *)&retCode, NULL, NULL, 
                           (char*)"HiveClient_JNI::getAllSchemas()",
                           HiveClient_JNI::getErrorText(retCode),
                           GetCliGlobals()->getJniErrorStr());
                    step_ = HANDLE_ERROR_;
                    break;
                  }
              }
            else
              {
                if ((!strcmp(hiveMDtdb().getSchema(), HIVE_SYSTEM_SCHEMA_LC)) ||
                    (!strcmp(hiveMDtdb().getSchema(), HIVE_SYSTEM_SCHEMA)))
                  {
                    strcpy(schForHive_, HIVE_DEFAULT_SCHEMA_EXE);
                  }
                else
                  {
                    strcpy(schForHive_, hiveMDtdb().getSchema());
                  }

                NAText * nat = new(getHeap()) NAText(schForHive_);
                schNames_.insert(nat);
              }

            currSchNum_ = 0;

            if (hiveMDtdb().mdType_ == ComTdbExeUtilHiveMDaccess::SCHEMAS_)
              step_ = POSITION_;
            else
              step_ = GET_ALL_TABLES_IN_SCHEMA_;
          }
          break;

        case GET_ALL_TABLES_IN_SCHEMA_:
          {
            if (currSchNum_ == schNames_.entries())
              {
                step_ = DONE_;
                break;
              }

            hiveMD_->clear();
            tblNames_.clear();

            char* currSch = (char*)schNames_[currSchNum_]->c_str();
            strcpy(schForHive_, currSch);
            if (! strcmp(schForHive_,  HIVE_DEFAULT_SCHEMA_EXE))
              strcpy(hiveSch_, HIVE_SYSTEM_SCHEMA_LC);
            else
              strcpy(hiveSch_, schForHive_);

            char* currObj = hiveMDtdb().getObject();

            if (! currObj)
              {
                HVC_RetCode retCode = HiveClient_JNI::getAllTables((NAHeap *)getHeap(), currSch, tblNames_);
                if (retCode == HVC_ERROR_EXISTS_EXCEPTION)
                  {
		    ExRaiseSqlError(getHeap(), &diagsArea_, -1003, 
                           NULL, NULL, NULL, 
                           hiveCat_,
                           hiveSch_);
                    step_ = HANDLE_ERROR_;
                    break;
                  }
                else if ((retCode != HVC_OK) && (retCode != HVC_DONE)) 
                  {
		    ExRaiseSqlError(getHeap(), &diagsArea_, -1190, 
                           (Lng32 *)&retCode, NULL, NULL, 
                           (char*)"HiveClient_JNI::getAllTables()",
                           HiveClient_JNI::getErrorText(retCode),
                           GetCliGlobals()->getJniErrorStr());
                    step_ = HANDLE_ERROR_;
                    break;
                  }
              }
            else
              {
                NAText * nat = new(getHeap()) NAText(currObj);
                tblNames_.insert(nat);
              }

            // read info for entries specified in tblNames_
            int i = 0;
            while (i < tblNames_.entries())
              {
                hiveMD_->getTableDesc(schForHive_, tblNames_[i]->c_str(), 
                      FALSE, FALSE, FALSE /*dont read partn info*/);
                i++;
              }

            step_ = POSITION_;
          }
          break;

	case POSITION_:
	  {
            hive_tbl_desc * htd = NULL;
            hiveMD_->position();
            htd = hiveMD_->getNext();

	    if (hiveMDtdb().mdType_ == ComTdbExeUtilHiveMDaccess::SCHEMAS_)
	      {
                currSchNum_ = 0;
		step_ = FETCH_SCHEMA_;
	      }
	    else if (hiveMDtdb().mdType_ == ComTdbExeUtilHiveMDaccess::TABLES_)
	      {
		step_ = FETCH_TABLE_;
	      }
	    else if (hiveMDtdb().mdType_ == ComTdbExeUtilHiveMDaccess::COLUMNS_)
	      {
                currColNum_ = 0;

		if (htd)
                  {
                    currColDesc_ = htd->getColumns();
                    currPartnDesc_ = htd->getPartKey();
                  }
		else
                  {
                    currColDesc_ = NULL;
                    currPartnDesc_ = NULL;
                  }

		step_ = FETCH_COLUMN_;
	      }
	    else if (hiveMDtdb().mdType_ == ComTdbExeUtilHiveMDaccess::PKEYS_)
	      {
		if (htd)
		  currKeyDesc_ = htd->getBucketingKeys();
		else
		  currKeyDesc_ = NULL;

		step_ = FETCH_PKEY_;
	      }
	    else
	      {
		step_ = DONE_;
		break;
	      }
	  }
	  break;

	case FETCH_SCHEMA_: 
	  {
	    if (qparent_.up->isFull())
	      return WORK_OK;

	    if (currSchNum_ == schNames_.entries())
	      {
		step_ = DONE_;
		break;
	      }

	    HiveMDSchemasColInfoStruct *infoCol =
              (HiveMDSchemasColInfoStruct*)mdRow_;

	    str_cpy(infoCol->catName, hiveCat_, 256, ' ');
            if (strcmp(schNames_[currSchNum_]->c_str(), HIVE_DEFAULT_SCHEMA_EXE) == 0)
              str_cpy(infoCol->schName, HIVE_SYSTEM_SCHEMA_LC, 256, ' ');
            else
              str_cpy(infoCol->schName, schNames_[currSchNum_]->c_str(), 256, ' ');
     	    
	    step_ = APPLY_PRED_;
	  }
	  break;
	  
	case FETCH_TABLE_:
	  {
	    if (qparent_.up->isFull())
	      return WORK_OK;

            if (hiveMD_->atEnd())
              {
                step_ = ADVANCE_SCHEMA_;
                break;
              }

            HiveMDTablesColInfoStruct *s =(HiveMDTablesColInfoStruct*)mdRow_;

	    str_cpy(s->catName, hiveCat_, 256, ' ');
	    str_cpy(s->schName, hiveSch_, 256, ' ');

            struct hive_tbl_desc * htd = hiveMD_->getNext();
            str_cpy(s->tblName, htd->tblName_, 256, ' ');
            
            memset(s->fileFormat, ' ', 24);
            if (htd->getSDs())
              {
                if (htd->getSDs()->isOrcFile())
                  str_cpy(s->fileFormat, "ORC", 24, ' ');
                else if (htd->getSDs()->isTextFile())
                  str_cpy(s->fileFormat, "TEXTFILE", 24, ' ');
                else if (htd->getSDs()->isSequenceFile())
                  str_cpy(s->fileFormat, "SEQUENCE", 24, ' ');
              }

            // htd->creationTS_ is the number of seconds from epoch.
            // convert it to juliantimestamp
            s->createTime = htd->creationTS_*1000000 + COM_EPOCH_TIMESTAMP;

            s->numCols = htd->getNumOfCols();
            s->numPartCols = htd->getNumOfPartCols();
            s->numSortCols = htd->getNumOfSortCols();
            s->numBucketCols = htd->getNumOfBucketCols();

            s->fieldDelimiter = htd->getSDs()->fieldTerminator_;
            s->recordTerminator = htd->getSDs()->recordTerminator_;
            memset(s->nullFormat, ' ', 8);
            if (htd->getSDs()->nullFormat_)
              str_cpy(s->nullFormat, htd->getSDs()->nullFormat_, 8, ' ');
            if (htd->getSDs()->location_ != NULL)
               str_cpy(s->location, htd->getSDs()->location_, 1024, ' ');

            str_cpy(s->hiveTableType, htd->tableType_, 128, ' ');

            str_cpy(s->hiveOwner, htd->owner_, 256, ' ');

	    step_ = APPLY_PRED_;
	  }
	  break;
	  
	case FETCH_COLUMN_:
	  {
	    if (qparent_.up->isFull())
	      return WORK_OK;

	    if ((! currColDesc_) &&
                (! currPartnDesc_))
	      {
                step_ = ADVANCE_SCHEMA_;
		break;
	      }
	    
	    struct hive_tbl_desc * htd = hiveMD_->getNext();
	    struct hive_column_desc * hcd = currColDesc_;
	    struct hive_pkey_desc * hpd = currPartnDesc_;
	    
	    HiveMDColumnsColInfoStruct *infoCol =
	      (HiveMDColumnsColInfoStruct*)mdRow_;
	    
	    str_cpy(infoCol->catName, hiveCat_, 256, ' ');
	    str_cpy(infoCol->schName, hiveSch_, 256, ' ');
	    str_cpy(infoCol->tblName, htd->tblName_, 256, ' ');
            str_cpy(infoCol->colName, 
                    (hcd ? hcd->name_ : hpd->name_), 256, ' ');

            Lng32 fstype = -1;
            Lng32 length = -1;
            Lng32 precision = -1;
            Lng32 scale = -1;

            // HIVEMD defines used below are defined in ComTdbExeUtil.h
            char sqlType[HIVEMD_DATA_TYPE_LEN+1];
            char displayType[HIVEMD_DISPLAY_DATA_TYPE_LEN+1];
            char charset[HIVEMD_CHARSET_LEN+1];
            retcode = 
              getTypeAttrsFromHiveColType(hcd ? hcd->type_ : hpd->type_,
                                          htd->getSDs()->isOrcFile(),
                                          fstype, length, precision, scale,
                                          sqlType, displayType, charset);
                 
	    if (retcode < 0)
	      {
                // add a warning and continue.
		char strP[1001];
		snprintf(strP, 1000, "Datatype %s for column '%s' in table %s.%s.%s is not supported. This table will be ignored.", 
                        (hcd ? hcd->type_ : hpd->type_),
                        (hcd ? hcd->name_ : hpd->name_),
                        hiveCat_, hiveSch_, htd->tblName_);
                ExRaiseSqlError(getHeap(), &diagsArea_, CLI_GET_METADATA_INFO_ERROR,
                      NULL, NULL, NULL,
                      strP);
		step_ = ADVANCE_ROW_;
		break;
	      }
	    
            infoCol->fsDatatype = fstype;

	    str_cpy(infoCol->sqlDatatype, sqlType, HIVEMD_DATA_TYPE_LEN, ' ');

	    str_cpy(infoCol->displayDatatype, displayType, HIVEMD_DISPLAY_DATA_TYPE_LEN, ' ');

            str_cpy(infoCol->hiveDatatype, (hcd ? hcd->type_ : hpd->type_), 
                    HIVEMD_DATA_TYPE_LEN, ' ');

            infoCol->colSize = length;
            infoCol->colPrecision = precision;
            infoCol->colScale = scale;

            str_pad(infoCol->charSet, HIVEMD_CHARSET_LEN, ' ');
            if (strlen(charset) > 0)
              str_cpy(infoCol->charSet, charset, HIVEMD_CHARSET_LEN, ' ');

	    infoCol->nullable = 1;

	    infoCol->dtCode = 0;
	    infoCol->dtStartField = 0;
	    infoCol->dtEndField = 0;
	    str_pad(infoCol->dtQualifier, 28, ' ');

	    if (infoCol->fsDatatype == REC_DATETIME)
	    {
              if(infoCol->colSize > 10) {
		infoCol->dtCode = SQLDTCODE_TIMESTAMP;
		infoCol->colScale = 6;
		str_cpy(infoCol->dtQualifier, "(6)", 28, ' ');
		infoCol->dtStartField = 1;
		infoCol->dtEndField = 6;
              }
              else
              {
		infoCol->dtCode = SQLDTCODE_DATE;
		infoCol->colScale = 0;
	        str_pad(infoCol->dtQualifier, HIVEMD_DT_QUALIFIER_LEN, ' ');
		infoCol->dtStartField = 1;
		infoCol->dtEndField = 3;
              }
	    }

	    // no default value
	    str_cpy(infoCol->defVal, " ", 240, ' ');

	    infoCol->colNum = currColNum_++;
            infoCol->partColNum = hcd ? -1 : hpd->idx_;
            infoCol->bucketColNum = 
              htd->getBucketColNum(hcd ? hcd->name_ : hpd->name_);
            infoCol->sortColNum = 
              htd->getSortColNum(hcd ? hcd->name_ : hpd->name_);

	    step_ = APPLY_PRED_;
	  }
	  break;

	case FETCH_PKEY_:  // does not work with JNI
	  {
	    if (qparent_.up->isFull())
	      return WORK_OK;

	    if (! currKeyDesc_)
	      {
                step_ = ADVANCE_SCHEMA_;
		break;
	      }

	    struct hive_tbl_desc * htd = hiveMD_->getNext();
	    struct hive_bkey_desc * hbd = currKeyDesc_;

	    HiveMDPKeysColInfoStruct *infoCol =(HiveMDPKeysColInfoStruct*)mdRow_;

	    str_cpy(infoCol->catName, hiveCat_, 256, ' ');
	    str_cpy(infoCol->schName, hiveSch_, 256, ' ');
	    str_cpy(infoCol->tblName, htd->tblName_, 256, ' ');
	    str_cpy(infoCol->colName, hbd->name_, 256, ' ');
	    infoCol->ordPos = hbd->idx_;
     	    
	    step_ = APPLY_PRED_;
	  }
	  break;
	  
	  case APPLY_PRED_:
	  {
	    if (!hiveMDtdb().scanExpr_)
	      {
		step_ = RETURN_ROW_;
		break;
	      }

	    ex_expr::exp_return_type exprRetCode = ex_expr::EXPR_OK;

	    workAtp_->getTupp(hiveMDtdb().workAtpIndex())
	      .setDataPointer(mdRow_);

	    exprRetCode =
	      hiveMDtdb().scanExpr_->eval(pentry_down->getAtp(), workAtp_);
	    
	    if (exprRetCode == ex_expr::EXPR_ERROR)
	      {
		step_ = HANDLE_ERROR_;
		break;
	      }

	    if (exprRetCode == ex_expr::EXPR_TRUE)
	      {
		step_ = RETURN_ROW_;
		break;
	      }

	    step_ = ADVANCE_ROW_;
	  }
	  break;

	case RETURN_ROW_:
	  {
	    if (qparent_.up->isFull())
	      return WORK_OK;

	    short rc = 0;
	    if (moveRowToUpQueue(mdRow_, hiveMDtdb().outputRowlen(), &rc, FALSE))
	      return rc;
	    
	    step_ = ADVANCE_ROW_;
	  }
	  break;

	case ADVANCE_ROW_:
	  {
	    if (hiveMDtdb().mdType_ == ComTdbExeUtilHiveMDaccess::SCHEMAS_)
	      {
                // move to the next schema
                currSchNum_++;

		step_ = FETCH_SCHEMA_;
	      }
	    else if (hiveMDtdb().mdType_ == ComTdbExeUtilHiveMDaccess::TABLES_)
	      {
                // move to the next table
                hiveMD_->advance();

		step_ = FETCH_TABLE_;
	      }

            // next two else blocks do not work with JNI
	    else if (hiveMDtdb().mdType_ == ComTdbExeUtilHiveMDaccess::COLUMNS_)
	      {
		if (currColDesc_)
		  currColDesc_ = currColDesc_->next_;
                else if (currPartnDesc_)
                  currPartnDesc_ = currPartnDesc_->next_;

		if ((! currColDesc_) &&
                    (! currPartnDesc_))
		  {
                    currColNum_ = 0;

		    // move to the next table
		    hiveMD_->advance();
		    
		    if (! hiveMD_->atEnd())
                      {
                        currColDesc_ = hiveMD_->getNext()->getColumns();
                        currPartnDesc_ = hiveMD_->getNext()->getPartKey();
                      }
		  }

		step_ = FETCH_COLUMN_;
	      }
	    else if (hiveMDtdb().mdType_ == ComTdbExeUtilHiveMDaccess::PKEYS_)
	      {
		if (currKeyDesc_)
		  currKeyDesc_ = currKeyDesc_->next_;
		
		if (! currKeyDesc_)
		  {
		    // move to the next table
		    hiveMD_->advance();
		    
		    if (! hiveMD_->atEnd())
		      currKeyDesc_ = hiveMD_->getNext()->getBucketingKeys();
		  }

		step_ = FETCH_PKEY_;
	      }
	    else
	      step_ = HANDLE_ERROR_;
	  }
	  break;

        case ADVANCE_SCHEMA_:
          {
            currSchNum_++;
            step_ = GET_ALL_TABLES_IN_SCHEMA_;
          }
          break;
          
	case HANDLE_ERROR_:
	  {
	    retcode = handleError();
	    if (retcode == 1)
	      return WORK_OK;

	    step_ = DONE_;
	  }
	break;

	case DONE_:
	  {
	    retcode = handleDone();
	    if (retcode == 1)
	      return WORK_OK;

	    step_ = INITIAL_;

	    return WORK_OK;
	  }
	break;

	} // switch
    } // while
}

////////////////////////////////////////////////////////////////////////
// Redefine virtual method allocatePstates, to be used by dynamic queue
// resizing, as well as the initial queue construction.
////////////////////////////////////////////////////////////////////////
ex_tcb_private_state * ExExeUtilHiveMDaccessTcb::allocatePstates(
     Lng32 &numElems,      // inout, desired/actual elements
     Lng32 &pstateLength)  // out, length of one element
{
  PstateAllocator<ExExeUtilHiveMDaccessPrivateState> pa;

  return pa.allocatePstates(this, numElems, pstateLength);
}


/////////////////////////////////////////////////////////////////////////////
// Constructor and destructor for ExeUtil_private_state
/////////////////////////////////////////////////////////////////////////////
ExExeUtilHiveMDaccessPrivateState::ExExeUtilHiveMDaccessPrivateState()
{
}

ExExeUtilHiveMDaccessPrivateState::~ExExeUtilHiveMDaccessPrivateState()
{
};

///////////////////////////////////////////////////////////////////
ex_tcb * ExExeUtilRegionStatsTdb::build(ex_globals * glob)
{
  ExExeUtilRegionStatsTcb * exe_util_tcb;

  if (displayFormat())
    exe_util_tcb = new(glob->getSpace()) ExExeUtilRegionStatsFormatTcb(*this, glob);
  else if (clusterView())
    exe_util_tcb = new(glob->getSpace()) ExExeUtilClusterStatsTcb(*this, glob);
  else
    exe_util_tcb = new(glob->getSpace()) ExExeUtilRegionStatsTcb(*this, glob);
  
  exe_util_tcb->registerSubtasks();

  return (exe_util_tcb);
}

////////////////////////////////////////////////////////////////
// Constructor for class ExExeUtilRegionStatsTcb
///////////////////////////////////////////////////////////////
ExExeUtilRegionStatsTcb::ExExeUtilRegionStatsTcb(
     const ComTdbExeUtilRegionStats & exe_util_tdb,
     ex_globals * glob)
     : ExExeUtilTcb( exe_util_tdb, NULL, glob)
{
  statsBuf_ = new(glob->getDefaultHeap()) char[sizeof(ComTdbRegionStatsVirtTableColumnStruct)];
  statsBufLen_ = sizeof(ComTdbRegionStatsVirtTableColumnStruct);

  stats_ = (ComTdbRegionStatsVirtTableColumnStruct*)statsBuf_;

  inputNameBuf_ = NULL;
  if (exe_util_tdb.inputExpr_)
    {
      inputNameBuf_ = new(glob->getDefaultHeap()) char[exe_util_tdb.inputRowlen_];
    }

  int jniDebugPort = 0;
  int jniDebugTimeout = 0;
  ehi_ = ExpHbaseInterface::newInstance(glob->getDefaultHeap(),
					(char*)"", //exe_util_tdb.server(), 
					(char*)""); //exe_util_tdb.zkPort(),
  regionInfoList_ = NULL;
  
  tableName_ = new(glob->getDefaultHeap()) char[2000];

  // get hbase rootdir location. Max linux pathlength is 1024.
  hbaseRootdir_ = new(glob->getDefaultHeap()) char[1030];
  strcpy(hbaseRootdir_, "/hbase");

  step_ = INITIAL_;
}

ExExeUtilRegionStatsTcb::~ExExeUtilRegionStatsTcb()
{
  if (statsBuf_)
    NADELETEBASIC(statsBuf_, getGlobals()->getDefaultHeap());

  if (ehi_)
    delete ehi_;

  statsBuf_ = NULL;
}

//////////////////////////////////////////////////////
// work() for ExExeUtilRegionStatsTcb
//////////////////////////////////////////////////////
Int64 ExExeUtilRegionStatsTcb::getEmbeddedNumValue
(char* &sep, char endChar, NABoolean adjustLen)
{
  Int64 num = -1;
  char * sepEnd = strchr(sep+1, endChar);
  if (sepEnd)
    {
      char longBuf[30];

      Lng32 len = sepEnd - sep - 1;
      str_cpy_all(longBuf, (sep+1), len);
      longBuf[len] = 0;                
      
      num = str_atoi(longBuf, len);

      sep += len + 1;

      if ((adjustLen) && (num == 0))
        num = 1024;
    }

  return num;
}

short ExExeUtilRegionStatsTcb::collectStats(char * tableName)
{
  // populate catName_, schName_, objName_.
  if (extractParts(tableName,
                   &catName_, &schName_, &objName_))
    {
      return -1;
    }

  // collect stats from ehi.
  HbaseStr tblName;
  
  if (NAString(catName_) == HBASE_SYSTEM_CATALOG)
    extNameForHbase_ = NAString(objName_);
  else
    extNameForHbase_ =
      NAString(catName_) + "." + NAString(schName_) + "." + NAString(objName_);
  tblName.val = (char*)extNameForHbase_.data();
  tblName.len = extNameForHbase_.length();
  
  regionInfoList_ = ehi_->getRegionStats(tblName);
  if (! regionInfoList_)
    {
      return -1;
    }
 
  currIndex_ = 0;

  return 0;
}

short ExExeUtilRegionStatsTcb::populateStats
(Int32 currIndex)
{
  str_pad(stats_->catalogName, sizeof(stats_->catalogName), ' ');
  str_cpy_all(stats_->catalogName, catName_, strlen(catName_));

  str_pad(stats_->schemaName, sizeof(stats_->schemaName), ' ');
  str_cpy_all(stats_->schemaName, schName_, strlen(schName_));

  str_pad(stats_->objectName, sizeof(stats_->objectName), ' ');
  str_cpy_all(stats_->objectName, objName_, strlen(objName_));
  
  str_pad(stats_->regionServer, sizeof(stats_->regionServer), ' ');

  str_pad(stats_->regionName, sizeof(stats_->regionName), ' ');
  stats_->regionNum       = currIndex_+1;
  
  char regionInfoBuf[5000];
  Int32 len = 0;
  char *regionInfo = regionInfoBuf;
  char *val = regionInfoList_->at(currIndex).val;
  len = regionInfoList_->at(currIndex).len; 
  if (len >= sizeof(regionInfoBuf))
     len = sizeof(regionInfoBuf)-1;
  strncpy(regionInfoBuf, val, len);
  regionInfoBuf[len] = '\0';
  stats_->numStores                = 0;
  stats_->numStoreFiles            = 0;
  stats_->storeFileUncompSize      = 0;
  stats_->storeFileSize            = 0;
  stats_->memStoreSize             = 0;
  
  char longBuf[30];
  char * sep1 = strchr(regionInfo, '|');
  if (sep1)
    {
      str_cpy_all(stats_->regionServer, regionInfo, 
                  (Lng32)(sep1 - regionInfo)); 
    }

  char * sepStart = sep1+1;
  sep1 = strchr(sepStart, '|');
  if (sep1)
    {
      str_cpy_all(stats_->regionName, sepStart, 
                  (Lng32)(sep1 - sepStart)); 
    }
  
  sepStart = sep1;
  stats_->numStores = getEmbeddedNumValue(sepStart, '|', FALSE);
  stats_->numStoreFiles = getEmbeddedNumValue(sepStart, '|', FALSE);
  stats_->storeFileUncompSize = getEmbeddedNumValue(sepStart, '|', FALSE);
  stats_->storeFileSize = getEmbeddedNumValue(sepStart, '|', FALSE);
  stats_->memStoreSize = getEmbeddedNumValue(sepStart, '|', FALSE);
  stats_->readRequestsCount = getEmbeddedNumValue(sepStart, '|', FALSE);
  stats_->writeRequestsCount = getEmbeddedNumValue(sepStart, '|', FALSE);
  
  return 0;
}

short ExExeUtilRegionStatsTcb::work()
{
  short retcode = 0;
  Lng32 cliRC = 0;
  ex_expr::exp_return_type exprRetCode = ex_expr::EXPR_OK;

  // if no parent request, return
  if (qparent_.down->isEmpty())
    return WORK_OK;
  
  // if no room in up queue, won't be able to return data/status.
  // Come back later.
  if (qparent_.up->isFull())
    return WORK_OK;
  
  ex_queue_entry * pentry_down = qparent_.down->getHeadEntry();
  ExExeUtilPrivateState & pstate =
    *((ExExeUtilPrivateState*) pentry_down->pstate);

  // Get the globals stucture of the master executor.
  ExExeStmtGlobals *exeGlob = getGlobals()->castToExExeStmtGlobals();
  ExMasterStmtGlobals *masterGlob = exeGlob->castToExMasterStmtGlobals();
  ContextCli * currContext = masterGlob->getCliGlobals()->currContext();

  while (1)
    {
      switch (step_)
	{
	case INITIAL_:
	  {
            if (ehi_ == NULL)
              {
                step_ = HANDLE_ERROR_;
                break;
              }

            if (getDLStdb().inputExpr())
              {
                step_ = EVAL_INPUT_;
                break;
              }

            strcpy(tableName_, getDLStdb().getTableName());

	    step_ = COLLECT_STATS_;
	  }
	break;

        case EVAL_INPUT_:
          {
	    workAtp_->getTupp(getDLStdb().workAtpIndex())
	      .setDataPointer(inputNameBuf_);

	    ex_expr::exp_return_type exprRetCode =
	      getDLStdb().inputExpr()->eval(pentry_down->getAtp(), workAtp_);
	    if (exprRetCode == ex_expr::EXPR_ERROR)
	      {
		step_ = HANDLE_ERROR_;
		break;
	      }

            short len = *(short*)inputNameBuf_;
            str_cpy_all(tableName_, &inputNameBuf_[2], len);
            tableName_[len] = 0;

            step_ = COLLECT_STATS_;
          }
          break;

        case COLLECT_STATS_:
          {
            if (collectStats(tableName_))
              {
                ExRaiseSqlError(getHeap(), &diagsArea_, -8451,
                     NULL, NULL, NULL,
                     getSqlJniErrorStr());
                step_ = HANDLE_ERROR_;
                break;
              }

            currIndex_ = 0;

            step_ = POPULATE_STATS_BUF_;
          }
          break;

	case POPULATE_STATS_BUF_:
	  {
            if (currIndex_ == regionInfoList_->entries())
              {
                step_ = DONE_;
                break;
              }
            
            if (populateStats(currIndex_))
              {
                step_ = HANDLE_ERROR_;
                break;
              }

	    step_ = EVAL_EXPR_;
	  }
	break;

        case EVAL_EXPR_:
          {
            exprRetCode = evalScanExpr((char*)stats_, statsBufLen_, FALSE);
	    if (exprRetCode == ex_expr::EXPR_FALSE)
	      {
		// row does not pass the scan expression,
		// move to the next row.
                currIndex_++;

		step_ = POPULATE_STATS_BUF_;
		break;
	      }
            
            step_ = RETURN_STATS_BUF_;
          }
          break;

	case RETURN_STATS_BUF_:
	  {
	    if (qparent_.up->isFull())
	      return WORK_OK;

	    short rc = 0;
	    if (moveRowToUpQueue((char*)stats_, statsBufLen_, &rc, FALSE))
	      return rc;

            currIndex_++;

            step_ = POPULATE_STATS_BUF_;
	  }
	break;

	case HANDLE_ERROR_:
	  {
	    retcode = handleError();
	    if (retcode == 1)
	      return WORK_OK;
	    
	    step_ = DONE_;
	  }
	break;
	
	case DONE_:
	  {
            if (regionInfoList_ != NULL) {
               deleteNAArray(getHeap(), regionInfoList_);
               regionInfoList_ = NULL;
            }
	    retcode = handleDone();
	    if (retcode == 1)
	      return WORK_OK;
	    
	    step_ = INITIAL_;
	    
	    return WORK_CALL_AGAIN;
	  }
	break;


	} // switch

    } // while

  return WORK_OK;
}

/////////////////////////////////////////////////////////////////////////////
// Constructor and destructor for ExeUtil_private_state
/////////////////////////////////////////////////////////////////////////////
ExExeUtilRegionStatsPrivateState::ExExeUtilRegionStatsPrivateState()
{
}

ExExeUtilRegionStatsPrivateState::~ExExeUtilRegionStatsPrivateState()
{
};


////////////////////////////////////////////////////////////////
// Constructor for class ExExeUtilRegionStatsFormatTcb
///////////////////////////////////////////////////////////////
ExExeUtilRegionStatsFormatTcb::ExExeUtilRegionStatsFormatTcb(
     const ComTdbExeUtilRegionStats & exe_util_tdb,
     ex_globals * glob)
     : ExExeUtilRegionStatsTcb( exe_util_tdb, glob)
{
  statsTotalsBuf_ = new(glob->getDefaultHeap()) char[sizeof(ComTdbRegionStatsVirtTableColumnStruct)];

  statsTotals_ = (ComTdbRegionStatsVirtTableColumnStruct*)statsTotalsBuf_;

  initTotals();

  step_ = INITIAL_;
}

static NAString removeTrailingBlanks(char * name, Lng32 maxLen)
{
  NAString nas;
  
  if (! name)
    return nas;

  Lng32 i = maxLen;
  while ((i > 0) && (name[i-1] == ' '))
    {
      i--;
    }

  if (i > 0)
    nas = NAString(name, i);

  return nas;
}

short ExExeUtilRegionStatsFormatTcb::initTotals()
{
  statsTotals_->numStores                 = 0;
  statsTotals_->numStoreFiles             = 0;
  statsTotals_->readRequestsCount         = 0;
  statsTotals_->writeRequestsCount        = 0;
  statsTotals_->storeFileUncompSize      = 0;
  statsTotals_->storeFileSize            = 0;
  statsTotals_->memStoreSize             = 0;

  return 0;
}

short ExExeUtilRegionStatsFormatTcb::computeTotals()
{
  str_pad(statsTotals_->catalogName, sizeof(statsTotals_->catalogName), ' ');
  str_cpy_all(statsTotals_->catalogName, catName_, strlen(catName_));

  str_pad(statsTotals_->schemaName, sizeof(statsTotals_->schemaName), ' ');
  str_cpy_all(statsTotals_->schemaName, schName_, strlen(schName_));

  str_pad(statsTotals_->objectName, sizeof(statsTotals_->objectName), ' ');
  str_cpy_all(statsTotals_->objectName, objName_, strlen(objName_));

  str_pad(statsTotals_->regionServer, sizeof(statsTotals_->regionServer), ' ');

  str_pad(statsTotals_->regionName, sizeof(statsTotals_->regionName), ' ');

  for (Int32 currIndex = 0; currIndex < regionInfoList_->entries(); currIndex++)
    {
      if (populateStats(currIndex))
        return -1;

      statsTotals_->numStores           += stats_->numStores;
      statsTotals_->numStoreFiles       += stats_->numStoreFiles;
      statsTotals_->storeFileUncompSize += stats_->storeFileUncompSize;
      statsTotals_->storeFileSize       += stats_->storeFileSize;
      statsTotals_->memStoreSize        += stats_->memStoreSize;  
      statsTotals_->readRequestsCount   += stats_->readRequestsCount;
      statsTotals_->writeRequestsCount  += stats_->writeRequestsCount;
    }
  
  return 0;
}

short ExExeUtilRegionStatsFormatTcb::work()
{
  short retcode = 0;
  Lng32 cliRC = 0;

  // if no parent request, return
  if (qparent_.down->isEmpty())
    return WORK_OK;
  
  // if no room in up queue, won't be able to return data/status.
  // Come back later.
  if (qparent_.up->isFull())
    return WORK_OK;
  
  ex_queue_entry * pentry_down = qparent_.down->getHeadEntry();
  ExExeUtilPrivateState & pstate =
    *((ExExeUtilPrivateState*) pentry_down->pstate);

  // Get the globals stucture of the master executor.
  ExExeStmtGlobals *exeGlob = getGlobals()->castToExExeStmtGlobals();
  ExMasterStmtGlobals *masterGlob = exeGlob->castToExMasterStmtGlobals();
  ContextCli * currContext = masterGlob->getCliGlobals()->currContext();

  while (1)
    {
      switch (step_)
	{
	case INITIAL_:
	  {
            if (ehi_ == NULL)
              {
                step_ = HANDLE_ERROR_;
                break;
              }
            
            initTotals();
            
            if (getDLStdb().inputExpr())
              {
                step_ = EVAL_INPUT_;
                break;
              }

            strcpy(tableName_, getDLStdb().getTableName());

	    step_ = COLLECT_STATS_;
	  }
	break;

        case EVAL_INPUT_:
          {
	    workAtp_->getTupp(getDLStdb().workAtpIndex())
	      .setDataPointer(inputNameBuf_);

	    ex_expr::exp_return_type exprRetCode =
	      getDLStdb().inputExpr()->eval(pentry_down->getAtp(), workAtp_);
	    if (exprRetCode == ex_expr::EXPR_ERROR)
	      {
		step_ = HANDLE_ERROR_;
		break;
	      }

            short len = *(short*)inputNameBuf_;
            str_cpy_all(tableName_, &inputNameBuf_[2], len);
            tableName_[len] = 0;

            step_ = COLLECT_STATS_;
          }
          break;

        case COLLECT_STATS_:
          {
            if (collectStats(tableName_) < 0)
              {
                ExRaiseSqlError(getHeap(), &diagsArea_, -8451,
                     NULL, NULL, NULL,
                     getSqlJniErrorStr());
                step_ = HANDLE_ERROR_;
                break;
              }

            currIndex_ = 0;

            step_ = COMPUTE_TOTALS_;
          }
          break;

        case COMPUTE_TOTALS_:
          {
            if (computeTotals())
              {
                step_ = HANDLE_ERROR_;
                break;
              }

            step_ = RETURN_SUMMARY_;
          }
          break;

        case RETURN_SUMMARY_:
          {
	    // make sure there is enough space to move header
	    if (isUpQueueFull(14))
	      {
		return WORK_CALL_AGAIN; // come back later
	      }

            ULng32 neededSize = SqlBufferNeededSize(14, 250);
            if (! pool_->get_free_buffer(neededSize))
              {
                return WORK_CALL_AGAIN;
              }
            
            char buf[1000];
	    short rc = 0;

            str_sprintf(buf, " ");
	    if (moveRowToUpQueue(buf, strlen(buf), &rc))
	      return rc;

            str_sprintf(buf, "Stats Summary");
	    if (moveRowToUpQueue(buf, strlen(buf), &rc))
	      return rc;
 
            str_sprintf(buf, "=============");
	    if (moveRowToUpQueue(buf, strlen(buf), &rc))
	      return rc;

            str_sprintf(buf, " ");
	    if (moveRowToUpQueue(buf, strlen(buf), &rc))
	      return rc;

            NAString objName = extNameForHbase_;
            str_sprintf(buf, "  ObjectName:              %s", objName.data());
	    if (moveRowToUpQueue(buf, strlen(buf), &rc))
	      return rc;

            str_sprintf(buf, "  NumRegions:              %d", regionInfoList_->entries());
	    if (moveRowToUpQueue(buf, strlen(buf), &rc))
	      return rc;

            str_sprintf(buf, "  RegionsLocation:         %s/data/default", 
                        hbaseRootdir_);
	    if (moveRowToUpQueue(buf, strlen(buf), &rc))
	      return rc;

            str_sprintf(buf, "  TotalNumStores:          %d", statsTotals_->numStores);
	    if (moveRowToUpQueue(buf, strlen(buf), &rc))
	      return rc;

            str_sprintf(buf, "  TotalNumStoreFiles:      %d", statsTotals_->numStoreFiles);
	    if (moveRowToUpQueue(buf, strlen(buf), &rc))
	      return rc;

            str_sprintf(buf, "  TotalUncompressedSize:   %ld", statsTotals_->storeFileUncompSize);
	    if (moveRowToUpQueue(buf, strlen(buf), &rc))
	      return rc;

           str_sprintf(buf, "  TotalStoreFileSize:      %ld", statsTotals_->storeFileSize);
	    if (moveRowToUpQueue(buf, strlen(buf), &rc))
	      return rc;

            str_sprintf(buf, "  TotalMemStoreSize:       %ld", statsTotals_->memStoreSize);
	    if (moveRowToUpQueue(buf, strlen(buf), &rc))
	      return rc;

            str_sprintf(buf, "  TotalReadRequestsCount:  %ld", statsTotals_->readRequestsCount);
	    if (moveRowToUpQueue(buf, strlen(buf), &rc))
	      return rc;

            str_sprintf(buf, "  TotalWriteRequestsCount: %ld", statsTotals_->writeRequestsCount);
	    if (moveRowToUpQueue(buf, strlen(buf), &rc))
	      return rc;

            step_ = RETURN_DETAILS_;
            return WORK_RESCHEDULE_AND_RETURN;
          }
          break;

        case RETURN_DETAILS_:
          {

            if ((getDLStdb().summaryOnly()) ||
                (regionInfoList_->entries() == 0))
              {
                step_ = DONE_;
                break;
              }

	    // make sure there is enough space to move header
	    if (isUpQueueFull(4))
	      {
		return WORK_CALL_AGAIN; // come back later
	      }

            ULng32 neededSize = SqlBufferNeededSize(4, 250);
            if (! pool_->get_free_buffer(neededSize))
              {
                return WORK_CALL_AGAIN;
              }

            char buf[1000];
	    short rc = 0;

            str_sprintf(buf, " ");
	    if (moveRowToUpQueue(buf, strlen(buf), &rc))
	      return rc;

            str_sprintf(buf, "Stats Details");
	    if (moveRowToUpQueue(buf, strlen(buf), &rc))
	      return rc;
 
            str_sprintf(buf, "=============");
	    if (moveRowToUpQueue(buf, strlen(buf), &rc))
	      return rc;

            str_sprintf(buf, " ");
	    if (moveRowToUpQueue(buf, strlen(buf), &rc))
	      return rc;

            currIndex_ = 0;
            step_ = POPULATE_STATS_BUF_;

            return WORK_RESCHEDULE_AND_RETURN;
          }
          break;

	case POPULATE_STATS_BUF_:
	  {
            if (currIndex_ == regionInfoList_->entries())
              {
                step_ = DONE_;
                break;
              }
            
            if (populateStats(currIndex_))
              {
                step_ = HANDLE_ERROR_;
                break;
              }

            step_ = RETURN_REGION_INFO_;
          }
          break;

        case RETURN_REGION_INFO_:
          {
	    // make sure there is enough space to move header
	    if (isUpQueueFull(10))
	      {
		return WORK_CALL_AGAIN; // come back later
	      }

            ULng32 neededSize = SqlBufferNeededSize(4, 100);
            if (! pool_->get_free_buffer(neededSize))
              {
                return WORK_CALL_AGAIN;
              }

            char buf[1000];
	    short rc = 0;

            str_sprintf(buf, "  RegionServer:       %s", 
                        removeTrailingBlanks(stats_->regionServer, STATS_NAME_MAX_LEN).data());
	    if (moveRowToUpQueue(buf, strlen(buf), &rc))
	      return rc;
  
            str_sprintf(buf, "  RegionNum:          %d", currIndex_+1);
	    if (moveRowToUpQueue(buf, strlen(buf), &rc))
	      return rc;

            str_sprintf(buf, "  RegionName:         %s", 
                        removeTrailingBlanks(stats_->regionName, STATS_REGION_NAME_MAX_LEN).data());
	    if (moveRowToUpQueue(buf, strlen(buf), &rc))
	      return rc;
            
            str_sprintf(buf, "  NumStores:          %d", stats_->numStores);
	    if (moveRowToUpQueue(buf, strlen(buf), &rc))
	      return rc;

            str_sprintf(buf, "  NumStoreFiles:      %d", stats_->numStoreFiles);
	    if (moveRowToUpQueue(buf, strlen(buf), &rc))
	      return rc;

            if (stats_->storeFileUncompSize == 0)
              str_sprintf(buf, "  UncompressedSize:   %ld (less than 1MB)", stats_->storeFileUncompSize);
            else
              str_sprintf(buf, "  UncompressedSize:   %ld Bytes", stats_->storeFileUncompSize);
	    if (moveRowToUpQueue(buf, strlen(buf), &rc))
	      return rc;

            if (stats_->storeFileSize == 0)
              str_sprintf(buf, "  StoreFileSize:      %ld (less than 1MB)", stats_->storeFileSize);
            else
              str_sprintf(buf, "  StoreFileSize:      %ld Bytes", stats_->storeFileSize);
	    if (moveRowToUpQueue(buf, strlen(buf), &rc))
	      return rc;

            if (stats_->memStoreSize == 0)
              str_sprintf(buf, "  MemStoreSize:       %ld (less than 1MB)", stats_->memStoreSize);
            else
              str_sprintf(buf, "  MemStoreSize:       %ld Bytes", stats_->memStoreSize);              
	    if (moveRowToUpQueue(buf, strlen(buf), &rc))
	      return rc;

            str_sprintf(buf, "  ReadRequestsCount:  %ld", stats_->readRequestsCount);
	    if (moveRowToUpQueue(buf, strlen(buf), &rc))
	      return rc;

            str_sprintf(buf, "  WriteRequestsCount: %ld", stats_->writeRequestsCount);
	    if (moveRowToUpQueue(buf, strlen(buf), &rc))
	      return rc;

            str_sprintf(buf, "   ");
	    if (moveRowToUpQueue(buf, strlen(buf), &rc))
	      return rc;

            currIndex_++;

            step_ = POPULATE_STATS_BUF_;

            return WORK_RESCHEDULE_AND_RETURN;
          }
          break;

	case HANDLE_ERROR_:
	  {
	    retcode = handleError();
	    if (retcode == 1)
	      return WORK_OK;
	    
	    step_ = DONE_;
	  }
	break;
	
	case DONE_:
	  {
            if (regionInfoList_ != NULL) {
               deleteNAArray(getHeap(), regionInfoList_);
               regionInfoList_ = NULL;
            }
	    retcode = handleDone();
	    if (retcode == 1)
	      return WORK_OK;
	    
	    step_ = INITIAL_;
	    
	    return WORK_CALL_AGAIN;
	  }
	break;


	} // switch

    } // while

  return WORK_OK;
}

////////////////////////////////////////////////////////////////
// Constructor for class ExExeUtilClusterStatsTcb
///////////////////////////////////////////////////////////////
ExExeUtilClusterStatsTcb::ExExeUtilClusterStatsTcb(
     const ComTdbExeUtilRegionStats & exe_util_tdb,
     ex_globals * glob)
     : ExExeUtilRegionStatsTcb( exe_util_tdb, glob)
{
  statsBuf_ = new(glob->getDefaultHeap()) char[sizeof(ComTdbClusterStatsVirtTableColumnStruct)];
  statsBufLen_ = sizeof(ComTdbClusterStatsVirtTableColumnStruct);

  stats_ = (ComTdbClusterStatsVirtTableColumnStruct*)statsBuf_;

  ehi_ = ExpHbaseInterface::newInstance(glob->getDefaultHeap(),
					(char*)"", 
					(char*)""); 
  regionInfoList_ = NULL;
  
  // get hbase rootdir location. Max linux pathlength is 1024.
  hbaseRootdir_ = new(glob->getDefaultHeap()) char[1030];
  strcpy(hbaseRootdir_, "/hbase");

  step_ = INITIAL_;
}

ExExeUtilClusterStatsTcb::~ExExeUtilClusterStatsTcb()
{
  if (statsBuf_)
    NADELETEBASIC(statsBuf_, getGlobals()->getDefaultHeap());

  if (ehi_)
    delete ehi_;

  statsBuf_ = NULL;
}

short ExExeUtilClusterStatsTcb::collectStats()
{
  numRegionStatsEntries_ = 0;
  regionInfoList_ = ehi_->getClusterStats(numRegionStatsEntries_);
  if (! regionInfoList_)
    {
      return 1; // EOD
    }
 
  currIndex_ = 0;

  return 0;
}

// RETURN: 1, not a TRAFODION region. 0, is a TRAFODION region.
//        -1, error.
short ExExeUtilClusterStatsTcb::populateStats
(Int32 currIndex, NABoolean nullTerminate)
{
  str_pad(stats_->catalogName, sizeof(stats_->catalogName), ' ');
  str_pad(stats_->schemaName, sizeof(stats_->schemaName), ' ');
  str_pad(stats_->objectName, sizeof(stats_->objectName), ' ');

  str_pad(stats_->regionServer, sizeof(stats_->regionServer), ' ');

  str_pad(stats_->regionName, sizeof(stats_->regionName), ' ');
   
  char regionInfoBuf[5000];
  Int32 len = 0;
  char *regionInfo = regionInfoBuf;
  char *val = regionInfoList_->at(currIndex).val;
  len = regionInfoList_->at(currIndex).len; 
  if (len >= sizeof(regionInfoBuf))
     len = sizeof(regionInfoBuf)-1;
  strncpy(regionInfoBuf, val, len);
  regionInfoBuf[len] = '\0';
  stats_->numStores                = 0;
  stats_->numStoreFiles            = 0;
  stats_->storeFileUncompSize      = 0;
  stats_->storeFileSize            = 0;
  stats_->memStoreSize             = 0;
  
  char longBuf[30];
  char * sep1 = strchr(regionInfo, '|');
  if (sep1)
    {
      str_cpy_all(stats_->regionServer, regionInfo, 
                  (Lng32)(sep1 - regionInfo)); 

      if (nullTerminate)
        stats_->regionServer[sep1 - regionInfo] = 0;
    }

  char * sepStart = sep1+1;
  sep1 = strchr(sepStart, '|');
  if (sep1)
    {
      str_cpy_all(stats_->regionName, sepStart, 
                  (Lng32)(sep1 - sepStart)); 

      if (nullTerminate)
        stats_->regionName[sep1 - sepStart] = 0;
    }

  char tableName[3*STATS_NAME_MAX_LEN + 3];
  sepStart = sep1+1;
  sep1 = strchr(sepStart, '|');
  if (sep1)
    {
      str_cpy_all(tableName, sepStart, 
                  (Lng32)(sep1 - sepStart)); 

      tableName[sep1 - sepStart] = 0;

      char tableNameBuf[3*STATS_NAME_MAX_LEN + 30];

      Lng32 numParts = 0;
      char *parts[4];
      LateNameInfo::extractParts(tableName, tableNameBuf, numParts, parts, FALSE);

      if (numParts == 3)
        {
          str_cpy_all(stats_->catalogName, parts[0], strlen(parts[0]));
          if (nullTerminate)
            stats_->catalogName[strlen(parts[0])] = 0;

          str_cpy_all(stats_->schemaName, parts[1], strlen(parts[1]));
          if (nullTerminate)
            stats_->schemaName[strlen(parts[1])] = 0;
      
          str_cpy_all(stats_->objectName, parts[2], strlen(parts[2]));
          if (nullTerminate)
            stats_->objectName[strlen(parts[2])] = 0;
        }

      if ((numParts != 3) ||
          (str_cmp(stats_->catalogName, TRAFODION_SYSCAT_LIT, strlen(TRAFODION_SYSCAT_LIT)) != 0))
        {
          // this is not a trafodion region, skip it.
          return 1;
        }
    }
  
  sepStart = sep1;
  stats_->numStores = getEmbeddedNumValue(sepStart, '|', FALSE);
  stats_->numStoreFiles = getEmbeddedNumValue(sepStart, '|', FALSE);
  stats_->storeFileUncompSize = getEmbeddedNumValue(sepStart, '|', FALSE);
  stats_->storeFileSize = getEmbeddedNumValue(sepStart, '|', FALSE);
  stats_->memStoreSize = getEmbeddedNumValue(sepStart, '|', FALSE);
  stats_->readRequestsCount = getEmbeddedNumValue(sepStart, '|', FALSE);
  stats_->writeRequestsCount = getEmbeddedNumValue(sepStart, '|', FALSE);
  
  return 0;
}

short ExExeUtilClusterStatsTcb::work()
{
  short retcode = 0;
  Lng32 cliRC = 0;
  ex_expr::exp_return_type exprRetCode = ex_expr::EXPR_OK;

  // if no parent request, return
  if (qparent_.down->isEmpty())
    return WORK_OK;
  
  // if no room in up queue, won't be able to return data/status.
  // Come back later.
  if (qparent_.up->isFull())
    return WORK_OK;
  
  ex_queue_entry * pentry_down = qparent_.down->getHeadEntry();
  ExExeUtilPrivateState & pstate =
    *((ExExeUtilPrivateState*) pentry_down->pstate);

  // Get the globals stucture of the master executor.
  ExExeStmtGlobals *exeGlob = getGlobals()->castToExExeStmtGlobals();
  ExMasterStmtGlobals *masterGlob = exeGlob->castToExMasterStmtGlobals();
  ContextCli * currContext = masterGlob->getCliGlobals()->currContext();

  while (1)
    {
      switch (step_)
	{
	case INITIAL_:
	  {
            if (ehi_ == NULL)
              {
                step_ = HANDLE_ERROR_;
                break;
              }

	    step_ = COLLECT_STATS_;
	  }
	break;

       case COLLECT_STATS_:
          {
            retcode = collectStats();
            if (retcode == 1) // EOD
              {
                step_ = DONE_;
                break;
              }
            else if (retcode < 0)
              {
                ExRaiseSqlError(getHeap(), &diagsArea_, -8451,
                     NULL, NULL, NULL,
                     getSqlJniErrorStr());
                step_ = HANDLE_ERROR_;
                break;
              }

            currIndex_ = 0;

            step_ = POPULATE_STATS_BUF_;
          }
          break;

	case POPULATE_STATS_BUF_:
	  {
            if (currIndex_ == numRegionStatsEntries_) //regionInfoList_->entries())
              {
                step_ = COLLECT_STATS_;
                break;
              }
            
            retcode = populateStats(currIndex_);
            if (retcode == 1) // not TRAFODION region, skip it
              {
                currIndex_++;

                step_ = POPULATE_STATS_BUF_;
                break;
              }
            else if (retcode < 0)
              {
                step_ = HANDLE_ERROR_;
                break;
              }

	    step_ = EVAL_EXPR_;
	  }
	break;

        case EVAL_EXPR_:
          {
            exprRetCode = evalScanExpr((char*)stats_, statsBufLen_, FALSE);
	    if (exprRetCode == ex_expr::EXPR_FALSE)
	      {
		// row does not pass the scan expression,
		// move to the next row.
                currIndex_++;

		step_ = POPULATE_STATS_BUF_;
		break;
	      }
            
            step_ = RETURN_STATS_BUF_;
          }
          break;

	case RETURN_STATS_BUF_:
	  {
	    if (qparent_.up->isFull())
	      return WORK_OK;

	    short rc = 0;
	    if (moveRowToUpQueue((char*)stats_, statsBufLen_, &rc, FALSE))
	      return rc;

 
            currIndex_++;

            step_ = POPULATE_STATS_BUF_;
	  }
	break;

	case HANDLE_ERROR_:
	  {
	    retcode = handleError();
	    if (retcode == 1)
	      return WORK_OK;
	    
	    step_ = DONE_;
	  }
	break;
	
	case DONE_:
	  {
            if (regionInfoList_ != NULL) {
               deleteNAArray(getHeap(), regionInfoList_);
               regionInfoList_ = NULL;
            }
	    retcode = handleDone();
	    if (retcode == 1)
	      return WORK_OK;
	    
	    step_ = INITIAL_;
	    
	    return WORK_CALL_AGAIN;
	  }
	break;


	} // switch

    } // while

  return WORK_OK;
}


///////////////////////////////////////////////////////////////////
ex_tcb * ExExeUtilLobInfoTdb::build(ex_globals * glob)
{

  if (isTableFormat())
    {
      ExExeUtilLobInfoTableTcb *exe_util_tcb = new(glob->getSpace()) ExExeUtilLobInfoTableTcb(*this,glob);
      exe_util_tcb->registerSubtasks();

      return (exe_util_tcb);
    }
  else
    {


    ExExeUtilLobInfoTcb *exe_util_tcb = new(glob->getSpace()) ExExeUtilLobInfoTcb(*this, glob);
    exe_util_tcb->registerSubtasks();

    return (exe_util_tcb);
    }
 
}

////////////////////////////////////////////////////////////////
// Constructor for class ExExeUtilLobInfoTcb
///////////////////////////////////////////////////////////////
ExExeUtilLobInfoTcb::ExExeUtilLobInfoTcb(
     const ComTdbExeUtilLobInfo & exe_util_tdb,
     ex_globals * glob)
     : ExExeUtilTcb( exe_util_tdb, NULL, glob)
{
  

  inputNameBuf_ = NULL;
  if (exe_util_tdb.inputExpr_)
    {
      inputNameBuf_ = new(glob->getDefaultHeap()) char[exe_util_tdb.inputRowlen_];
    }

  tableName_ = new(glob->getDefaultHeap()) char[2000];
  currLobNum_ = 1;
  step_ = INITIAL_;
}

ExExeUtilLobInfoTcb::~ExExeUtilLobInfoTcb()
{
  if (tableName_)
    NADELETEBASIC(tableName_, getGlobals()->getDefaultHeap());
  if(inputNameBuf_) 
     NADELETEBASIC(inputNameBuf_, getGlobals()->getDefaultHeap());
 
  tableName_ = NULL;
  inputNameBuf_ = NULL;
}

short ExExeUtilLobInfoTcb::collectAndReturnLobInfo(char * tableName,Int32 currLobNum, ContextCli *currContext)
{
  char *catName = NULL;
  char *schName = NULL;
  char *objName = NULL;
  Int32 offset = 0;
  char columnName[LOBINFO_MAX_FILE_LEN]= {'\0'};
  char lobLocation[LOBINFO_MAX_FILE_LEN]={'\0'};
  char lobDataFilePath[LOBINFO_MAX_FILE_LEN]={'\0'};
  Int64 lobEOD=0;

  char buf[LOBINFO_MAX_FILE_LEN+500];
  short rc = 0;
  if (isUpQueueFull(5))
    {
      return WORK_CALL_AGAIN; // come back later
    }
  

  // populate catName, schName, objName.
  if (extractParts(tableName,
                   &catName, &schName, &objName))
    {
      return -1;
    }
  str_pad(buf,sizeof(buf),' ');
  //column name
  offset = (currLobNum-1)*LOBINFO_MAX_FILE_LEN; 
  strcpy(columnName, &((getLItdb().getLobColList())[offset]));
  removeTrailingBlanks(columnName, LOBINFO_MAX_FILE_LEN);
  str_sprintf(buf, "  ColumnName :  %s", columnName);
  if (moveRowToUpQueue(buf, strlen(buf), &rc))
    return rc;

  //lob location  
 
   strcpy(lobLocation, &((getLItdb().getLobLocList())[offset]));
  removeTrailingBlanks(lobLocation, LOBINFO_MAX_FILE_LEN);
if (getLItdb().getLobTypeList()[(currLobNum-1)*sizeof(Int32)] == Lob_External_HDFS_File)
  str_sprintf(buf, "  Lob Location :  External HDFS Location");
else
  str_sprintf(buf, "  Lob Location :  %s", lobLocation);
  if (moveRowToUpQueue(buf, strlen(buf), &rc))
    return rc;      
  
  char lobDescChunkFileBuf[LOBINFO_MAX_FILE_LEN*2];
  //Get the descriptor chunks table name
  char *lobDescChunksFile =
    ExpLOBoper::ExpGetLOBDescChunksName(strlen(schName),schName,
                                        getLItdb().objectUID_, currLobNum, 
                                        lobDescChunkFileBuf, LOBINFO_MAX_FILE_LEN*2);
 
  char *query = new(getGlobals()->getDefaultHeap()) char[4096]; 
  // lobDataFile
  char tgtLobNameBuf[LOBINFO_MAX_FILE_LEN];
 
  
   char *lobDataFile = 
     ExpLOBoper::ExpGetLOBname
     (getLItdb().objectUID_, currLobNum, 
      tgtLobNameBuf, LOBINFO_MAX_FILE_LEN);
    
    
 
  removeTrailingBlanks(lobDataFile, LOBINFO_MAX_FILE_LEN);
  if (getLItdb().getLobTypeList()[(currLobNum-1)*sizeof(Int32)] == Lob_External_HDFS_File)
    str_sprintf(buf, "  LOB Data File:  External HDFS File");
  else
    str_sprintf(buf, "  LOB Data File:  %s", lobDataFile);
  if (moveRowToUpQueue(buf, strlen(buf), &rc))
    return rc;  
                
  //EOD of LOB data file
  snprintf(lobDataFilePath, LOBINFO_MAX_FILE_LEN, "%s/%s", lobLocation, lobDataFile);
  HDFS_Client_RetCode hdfsClientRetcode;
  lobEOD = HdfsClient::hdfsSize(lobDataFilePath, hdfsClientRetcode);
  if (hdfsClientRetcode != HDFS_CLIENT_OK) 
     return LOB_DATA_FILE_OPEN_ERROR;

  str_sprintf(buf, "  LOB EOD :  %ld", lobEOD);
  if (moveRowToUpQueue(buf, strlen(buf), &rc))
    return rc;

  // Sum of all the lobDescChunks for used space

   
  str_sprintf (query,  "select sum(chunklen) from  %s ", lobDescChunksFile);
  // set parserflags to allow ghost table
  currContext->setSqlParserFlags(0x1);	

  Int64 outlen = 0;Lng32 len = 0;
  Int32 cliRC = cliInterface()->executeImmediate(query,(char *)&outlen, &len, FALSE);
  if ((len ==0) ||(getLItdb().getLobTypeList()[(currLobNum-1)*sizeof(Int32)] == Lob_External_HDFS_File))
    outlen = 0;
  NADELETEBASIC(query, getGlobals()->getDefaultHeap());
  
  currContext->resetSqlParserFlags(0x1);
  if (cliRC <0 )
    {
      return cliRC;
    }

  str_sprintf(buf, "  LOB Used Len :  %ld", outlen);
  if (moveRowToUpQueue(buf, strlen(buf), &rc))
    return rc;
  return 0;
}
short ExExeUtilLobInfoTcb::work()
{
  short retcode = 0;
  Lng32 cliRC = 0;
  const char *parentQid = NULL;
  char buf[1000];
     short rc = 0;
  // if no parent request, return
  if (qparent_.down->isEmpty())
    return WORK_OK;
  
  // if no room in up queue, won't be able to return data/status.
  // Come back later.
  if (qparent_.up->isFull())
    return WORK_OK;
  
  ex_queue_entry * pentry_down = qparent_.down->getHeadEntry();
  ExExeUtilPrivateState & pstate =
    *((ExExeUtilPrivateState*) pentry_down->pstate);

  // Get the globals stucture of the master executor.
  ExExeStmtGlobals *exeGlob = getGlobals()->castToExExeStmtGlobals();
  ExMasterStmtGlobals *masterGlob = exeGlob->castToExMasterStmtGlobals();
  ContextCli * currContext = masterGlob->getCliGlobals()->currContext();
   ExExeStmtGlobals *stmtGlobals = getGlobals()->castToExExeStmtGlobals();
  while (1)
    {
      switch (step_)
	{
	case INITIAL_:
	  {
            if (isUpQueueFull(3))
	      {
		return WORK_CALL_AGAIN; // come back later
	      }
           
             
            if (getLItdb().inputExpr())
              {
                step_ = EVAL_INPUT_;
                break;
              }

            strcpy(tableName_, getLItdb().getTableName());
            str_pad(buf,1000,'\0');
            str_sprintf(buf, " ");
	    if (moveRowToUpQueue(buf, strlen(buf), &rc))
              return rc;
            removeTrailingBlanks(tableName_, LOBINFO_MAX_FILE_LEN);
            str_pad(buf,1000,'\0');
            str_sprintf(buf, "Lob Information for table: %s", tableName_);
            if (moveRowToUpQueue(buf, strlen(buf), &rc))
              return rc;
            str_pad(buf,1000,'\0');
             str_sprintf(buf, "=========================");
            if (moveRowToUpQueue(buf, strlen(buf), &rc))
              return rc;
            str_pad(buf,1000,'\0');
            str_sprintf(buf, " ");
	    if (moveRowToUpQueue(buf, strlen(buf), &rc))
	      return rc;
            
	    step_ = COLLECT_LOBINFO_;
           
	  }
	break;

        case EVAL_INPUT_:
          {
	    workAtp_->getTupp(getLItdb().workAtpIndex())
	      .setDataPointer(inputNameBuf_);

	    ex_expr::exp_return_type exprRetCode =
	      getLItdb().inputExpr()->eval(pentry_down->getAtp(), workAtp_);
	    if (exprRetCode == ex_expr::EXPR_ERROR)
	      {
		step_ = HANDLE_ERROR_;
		break;
	      }

            short len = *(short*)inputNameBuf_;
            str_cpy_all(tableName_, &inputNameBuf_[2], len);
            tableName_[len] = 0;

           
            step_ = COLLECT_LOBINFO_;
          }
          break;

        case COLLECT_LOBINFO_:
          {
            if (getLItdb().getNumLobs() == 0)
              {
                strcpy(buf, "  Num Lob Columns = 0");
                if (moveRowToUpQueue(buf, strlen(buf), &rc))
                  return rc;
                step_ = DONE_;
                break;
              }
            if (currLobNum_ == getLItdb().getNumLobs()+1)
              {
                step_ = DONE_;
                break;
              }
            if (collectAndReturnLobInfo(tableName_,currLobNum_, currContext))
              {
                step_ = HANDLE_ERROR_;
                break;
              }
            currLobNum_++;
            
          }
          break;



	case HANDLE_ERROR_:
	  {
            
	    retcode = handleError();
	    if (retcode == 1)
	      return WORK_OK;
	    
	    step_ = DONE_;
	  }
	break;
	
	case DONE_:
	  {
           
	    retcode = handleDone();
	    if (retcode == 1)
	      return WORK_OK;
	    
	    step_ = INITIAL_;
	    
	    return WORK_OK;
	  }
	break;


	} // switch

    } // while

  return WORK_OK;
}



////////////////////////////////////////////////////////////////
// Constructor for class ExExeUtilLobInfoTableTcb
///////////////////////////////////////////////////////////////
ExExeUtilLobInfoTableTcb::ExExeUtilLobInfoTableTcb(
     const ComTdbExeUtilLobInfo & exe_util_tdb,
     ex_globals * glob)
     : ExExeUtilTcb( exe_util_tdb, NULL, glob)
{
  lobInfoBuf_ = new(glob->getDefaultHeap()) char[sizeof(ComTdbLobInfoVirtTableColumnStruct)];
  lobInfoBufLen_ = sizeof(ComTdbLobInfoVirtTableColumnStruct);

  lobInfo_ = (ComTdbLobInfoVirtTableColumnStruct*)lobInfoBuf_;

  inputNameBuf_ = NULL;
  if (exe_util_tdb.inputExpr_)
    {
      inputNameBuf_ = new(glob->getDefaultHeap()) char[exe_util_tdb.inputRowlen_];
    }

  
  tableName_ = new(glob->getDefaultHeap()) char[2000];
  currLobNum_ = 1;
  step_ = INITIAL_;
}

ExExeUtilLobInfoTableTcb::~ExExeUtilLobInfoTableTcb()
{
  if (lobInfoBuf_)
    NADELETEBASIC(lobInfoBuf_, getGlobals()->getDefaultHeap());
  if (tableName_)
    NADELETEBASIC(tableName_, getGlobals()->getDefaultHeap());
  if(inputNameBuf_) 
     NADELETEBASIC(inputNameBuf_, getGlobals()->getDefaultHeap());

  tableName_ = NULL;
  inputNameBuf_ = NULL;

  lobInfoBuf_ = NULL;
}
short ExExeUtilLobInfoTableTcb:: populateLobInfo(Int32 currIndex, NABoolean nullTerminate )
{
  return 0;
}
short ExExeUtilLobInfoTableTcb::collectLobInfo(char * tableName,Int32 currLobNum, ContextCli *currContext)
{
  char *catName = NULL;
  char *schName = NULL;
  char *objName = NULL;
  Int32 offset = 0;
  char columnName[LOBINFO_MAX_FILE_LEN]= {'\0'};
  char lobDataFilePath[LOBINFO_MAX_FILE_LEN]={'\0'};
  Int64 lobEOD=0;

  // populate catName, schName, objName.
  if (extractParts(tableName,
                   &catName, &schName, &objName))
    {
      return -1;
    }
  str_pad((char *)lobInfo_,sizeof(ComTdbLobInfoVirtTableColumnStruct),' ');
  
  str_cpy_all(lobInfo_->catalogName,catName,strlen(catName));
  str_cpy_all(lobInfo_->schemaName,schName,strlen(schName));
  str_cpy_all(lobInfo_->objectName,objName,strlen(objName));

  //column name
  offset = (currLobNum-1)*LOBINFO_MAX_FILE_LEN; 
  str_cpy_all(lobInfo_->columnName, &((getLItdb().getLobColList())[offset]),
              strlen(&((getLItdb().getLobColList())[offset])));

  char *lobLocation = new(getGlobals()->getDefaultHeap()) char[LOBINFO_MAX_FILE_LEN]  ;
 
   
  lobLocation = &((getLItdb().getLobLocList())[offset]);
 if (getLItdb().getLobTypeList()[(currLobNum-1)*sizeof(Int32)] == Lob_External_HDFS_File)
 str_cpy_all(lobInfo_->lobLocation, "External HDFS Location", strlen("External HDFS Location"));
 else 
   str_cpy_all(lobInfo_->lobLocation, (char *)&lobLocation[0], strlen(lobLocation));
                          
  // lobDataFile
  char tgtLobNameBuf[LOBINFO_MAX_FILE_LEN];
  char query[4096];
  char lobDescChunkFileBuf[LOBINFO_MAX_FILE_LEN*2];
 
  //Get the descriptor chunks table name
  char *lobDescChunksFile =
    ExpLOBoper::ExpGetLOBDescChunksName(strlen(schName),schName,
                                        getLItdb().objectUID_, currLobNum, 
                                        lobDescChunkFileBuf, LOBINFO_MAX_FILE_LEN*2);
    char *lobDataFile = 
	      ExpLOBoper::ExpGetLOBname
      (getLItdb().objectUID_, currLobNum, 
	       tgtLobNameBuf, LOBINFO_MAX_FILE_LEN);
   
  if (getLItdb().getLobTypeList()[(currLobNum-1)*sizeof(Int32)] == Lob_External_HDFS_File)
    {
      str_cpy_all(lobInfo_->lobDataFile, "External HDFS File" ,strlen("External HDFS File"));  
    }
  else
    {
      str_cpy_all(lobInfo_->lobDataFile,  lobDataFile,strlen(lobDataFile));
    }             
  //EOD of LOB data file
  snprintf(lobDataFilePath, LOBINFO_MAX_FILE_LEN, "%s/%s", lobLocation, lobDataFile);
  HDFS_Client_RetCode hdfsClientRetcode;
  lobEOD = HdfsClient::hdfsSize(lobDataFilePath, hdfsClientRetcode);
  if (hdfsClientRetcode != HDFS_CLIENT_OK) 
     return LOB_DATA_FILE_OPEN_ERROR;

  lobInfo_->lobDataFileSizeEod=lobEOD;
  // Sum of all the lobDescChunks for used space

  str_sprintf (query,  "select sum(chunklen) from  %s ", lobDescChunksFile);

  // set parserflags to allow ghost table
  currContext->setSqlParserFlags(0x1);
	
  Int64 outlen = 0;Lng32 len = 0;
  Int32 cliRC = cliInterface()->executeImmediate(query,(char *)&outlen, &len, FALSE);
  if ((len == 0) || (getLItdb().getLobTypeList()[(currLobNum-1)*sizeof(Int32)] == Lob_External_HDFS_File))
    outlen = 0;
  lobInfo_->lobDataFileSizeUsed = outlen;
  currContext->resetSqlParserFlags(0x1);
        

  if (cliRC <0 )
    {
      return cliRC;
    }
  
  return 0;
}
short ExExeUtilLobInfoTableTcb::work()
{
  short retcode = 0;
  Lng32 cliRC = 0;
  const char *parentQid = NULL;
  // if no parent request, return
  if (qparent_.down->isEmpty())
    return WORK_OK;
  
  // if no room in up queue, won't be able to return data/status.
  // Come back later.
  if (qparent_.up->isFull())
    return WORK_OK;
  
  ex_queue_entry * pentry_down = qparent_.down->getHeadEntry();
  ExExeUtilPrivateState & pstate =
    *((ExExeUtilPrivateState*) pentry_down->pstate);

  // Get the globals stucture of the master executor.
  ExExeStmtGlobals *exeGlob = getGlobals()->castToExExeStmtGlobals();
  ExMasterStmtGlobals *masterGlob = exeGlob->castToExMasterStmtGlobals();
  ContextCli * currContext = masterGlob->getCliGlobals()->currContext();
  
  ExExeStmtGlobals *stmtGlobals = getGlobals()->castToExExeStmtGlobals();
  if (stmtGlobals->castToExMasterStmtGlobals())
    parentQid = stmtGlobals->castToExMasterStmtGlobals()->
      getStatement()->getUniqueStmtId();
  else 
  {
    ExEspStmtGlobals *espGlobals = stmtGlobals->castToExEspStmtGlobals();
    if (espGlobals && espGlobals->getStmtStats())
      parentQid = espGlobals->getStmtStats()->getQueryId();
  }
  ExeCliInterface cliInterface(getHeap(), 0, NULL, parentQid);
  while (1)
    {
      switch (step_)
	{
	case INITIAL_:
	  {
           
            if (getLItdb().inputExpr())
              {
                step_ = EVAL_INPUT_;
                break;
              }

            strcpy(tableName_, getLItdb().getTableName());

	    step_ = COLLECT_LOBINFO_;
	  }
	break;

        case EVAL_INPUT_:
          {
	    workAtp_->getTupp(getLItdb().workAtpIndex())
	      .setDataPointer(inputNameBuf_);

	    ex_expr::exp_return_type exprRetCode =
	      getLItdb().inputExpr()->eval(pentry_down->getAtp(), workAtp_);
	    if (exprRetCode == ex_expr::EXPR_ERROR)
	      {
		step_ = HANDLE_ERROR_;
		break;
	      }

            short len = *(short*)inputNameBuf_;
            str_cpy_all(tableName_, &inputNameBuf_[2], len);
            tableName_[len] = 0;

            step_ = COLLECT_LOBINFO_;
          }
          break;

        case COLLECT_LOBINFO_:
          {
            if (currLobNum_ == getLItdb().getNumLobs()+1)
              {
                step_ = DONE_;
                break;
              }
            if (collectLobInfo(tableName_,currLobNum_, currContext))
              {
                step_ = HANDLE_ERROR_;
                break;
              }

            step_ = POPULATE_LOBINFO_BUF_;
          }
          break;

	case POPULATE_LOBINFO_BUF_:
	  {
            
            
            if (populateLobInfo(currLobNum_))
              {
                step_ = HANDLE_ERROR_;
                break;
                }

	    step_ = RETURN_LOBINFO_BUF_;
	  }
	break;

	case RETURN_LOBINFO_BUF_:
	  {
	    if (qparent_.up->isFull())
	      return WORK_OK;

	    short rc = 0;
	    if (moveRowToUpQueue((char*)lobInfo_, lobInfoBufLen_, &rc, FALSE))
	      return rc;

            currLobNum_++;

            step_ = COLLECT_LOBINFO_;
	  }
	break;

	case HANDLE_ERROR_:
	  {
	    retcode = handleError();
	    if (retcode == 1)
	      return WORK_OK;
	    
	    step_ = DONE_;
	  }
	break;
	
	case DONE_:
	  {
	    retcode = handleDone();
	    if (retcode == 1)
	      return WORK_OK;
	    
	    step_ = INITIAL_;
	    
	    return WORK_CALL_AGAIN;
	  }
	break;


	} // switch

    } // while

  return WORK_OK;
}
