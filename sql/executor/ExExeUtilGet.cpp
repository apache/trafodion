/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 1994-2014 Hewlett-Packard Development Company, L.P.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
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

#include  "ExExeUtil.h"
#include  "ex_exe_stmt_globals.h"
#include  "exp_expr.h"
#include  "exp_clause_derived.h"
#include  "ExpLOB.h"
#include  "ComRtUtils.h"

#include  "sqlcmd.h"
#include  "SqlciEnv.h"

#include  "GetErrorMessage.h"
#include  "ErrorMessage.h"
#include  "HBaseClient_JNI.h"

#include "CmpDDLCatErrorCodes.h"
#include "PrivMgrCommands.h"

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
  else if (getVersion())
    exe_util_tcb =
      new(glob->getSpace()) ExExeUtilGetMetadataInfoVersionTcb(*this, glob);
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
}

ExExeUtilGetMetadataInfoTcb::~ExExeUtilGetMetadataInfoTcb()
{
  NADELETEBASIC(queryBuf_, getGlobals()->getDefaultHeap());
  NADELETEBASIC(outputBuf_, getGlobals()->getDefaultHeap());
  NADELETEBASIC(headingBuf_, getGlobals()->getDefaultHeap());
  NADELETEBASIC(patternStr_, getGlobals()->getDefaultHeap());
}

static const QueryString getCatalogsQuery[] =
{
  {" select translate(cat_name using ucs2toutf8) from "},
  {"   HP_SYSTEM_CATALOG.system_schema.catsys   C "},
  {"        %s "},
  {" for read uncommitted access "},
  {" order by 1 "},
  {" ; "}
};

static const QueryString getSchemasInCatalogQuery[] =
{
  {" select translate(schema_name using ucs2toutf8) from "},
  {"   HP_SYSTEM_CATALOG.system_schema.catsys   C, "},
  {"   HP_SYSTEM_CATALOG.system_schema.schemata S "},
  {" where C.cat_name = '%s'      and "},
  {"       C.cat_uid = S.cat_uid  and "},
  {"       S.current_operation != 'VS' "},
  {"        %s "},
  {" for read uncommitted access "},
  {" order by 1 "},
  {" ; "}
};

static const QueryString getInvalidViewsInCatalogQuery[] =
{
  {" select translate(trim(S.schema_name) || '.' || trim(O.object_name) using ucs2toutf8) from "},
  {"   HP_SYSTEM_CATALOG.system_schema.schemata S, "},
  {"    \"%s\".HP_DEFINITION_SCHEMA.objects O "},
  {"  where S.schema_uid = O.schema_uid and "},
  {"        O.object_name_space = 'TA' and "},
  {"        O.object_type = 'VI' and "},
  {"        O.valid_def = 'N' "},
  {"  for read uncommitted access "},
  {" order by 1 "},
  {"  ; "}
};

static const QueryString getTablesInSchemaQuery[] =
{
  {" select translate(%srtrim(object_name)%s using ucs2toutf8) from "},
  {"   HP_SYSTEM_CATALOG.system_schema.schemata S, "},
  {"    \"%s\".HP_DEFINITION_SCHEMA.objects O "},
  {"  where S.schema_uid = O.schema_uid and "},
  {"        S.schema_name = '%s' and "},
  {"        O.object_name_space = 'TA' and "},
  {"        O.object_type = 'BT' "},
  {"        %s "},
  {"  for read uncommitted access "},
  {"  order by 1 "},
  {"  ; "}
};

static const QueryString getIndexesInSchemaQuery[] =
{
  {" select translate(%strim(object_name)%s using ucs2toutf8) from "},
  {"   HP_SYSTEM_CATALOG.system_schema.schemata S, "},
  {"    \"%s\".HP_DEFINITION_SCHEMA.objects O "},
  {"  where S.schema_uid = O.schema_uid and "},
  {"        S.schema_name = '%s' and "},
  {"        O.object_name_space = 'IX' "},
  {"  for read uncommitted access "},
  {"  order by 1 "},
  {"  ; "}
};

static const QueryString getViewsInSchemaQuery[] =
{
  {" select translate(%strim(object_name)%s using ucs2toutf8) from "},
  {"   HP_SYSTEM_CATALOG.system_schema.schemata S, "},
  {"    \"%s\".HP_DEFINITION_SCHEMA.objects O "},
  {"  where S.schema_uid = O.schema_uid and "},
  {"        S.schema_name = '%s' and "},
  {"        O.object_name_space = 'TA' and "},
  {"        O.object_type = 'VI' "},
  {"  for read uncommitted access "},
  {" order by 1 "},
  {"  ; "}
};

static const QueryString getInvalidViewsInSchemaQuery[] =
{
  {" select translate(%strim(object_name)%s using ucs2toutf8) from "},
  {"   HP_SYSTEM_CATALOG.system_schema.schemata S, "},
  {"    \"%s\".HP_DEFINITION_SCHEMA.objects O "},
  {"  where S.schema_uid = O.schema_uid and "},
  {"        S.schema_name = '%s' and "},
  {"        O.object_name_space = 'TA' and "},
  {"        O.object_type = 'VI' and "},
  {"        O.valid_def = 'N' "},
  {"  for read uncommitted access "},
  {" order by 1 "},
  {"  ; "}
};

static const QueryString getLibrariesInSchemaQuery[] =
{
  {" select translate(%strim(object_name)%s using ucs2toutf8) from "},
  {"   HP_SYSTEM_CATALOG.system_schema.schemata S, "},
  {"    \"%s\".HP_DEFINITION_SCHEMA.objects O "},
  {"  where S.schema_uid = O.schema_uid and "},
  {"        S.schema_name = '%s' and "},
  {"        O.object_name_space = 'LB' and "},	  //ACH Verify this is being stored correctly - SMDIO?
  {"        O.object_type = 'LB' "},
  {"  for read uncommitted access "},
  {" order by 1 "},
  {"  ; "}
};

static const QueryString getMVsInSchemaQuery[] =
{
  {" select translate(%strim(object_name)%s using ucs2toutf8) from "},
  {"   HP_SYSTEM_CATALOG.system_schema.schemata S, "},
  {"    \"%s\".HP_DEFINITION_SCHEMA.objects O "},
  {"  where S.schema_uid = O.schema_uid and "},
  {"        S.schema_name = '%s' and "},
  {"        O.object_name_space = 'TA' and "},
  {"        O.object_type = 'MV' "},
  {"  for read uncommitted access "},
  {" order by 1 "},
  {"  ; "}
};

static const QueryString getProceduresInSchemaQuery[] =
{
  {" select translate(%strim(object_name)%s using ucs2toutf8) from "},
  {"   HP_SYSTEM_CATALOG.system_schema.schemata S, "},
  {"    \"%s\".HP_DEFINITION_SCHEMA.objects O "},
  {"  where S.schema_uid = O.schema_uid and "},
  {"        S.schema_name = '%s' and "},
  {"        O.object_name_space = 'TA' and "},
  {"        O.object_type = 'UR' "},
  {"  for read uncommitted access "},
  {" order by 1 "},
  {"  ; "}
};

static const QueryString getSynonymsInSchemaQuery[] =
{
  {" select translate(%strim(object_name)%s using ucs2toutf8) from "},
  {"   HP_SYSTEM_CATALOG.system_schema.schemata S, "},
  {"    \"%s\".HP_DEFINITION_SCHEMA.objects O "},
  {"  where S.schema_uid = O.schema_uid and "},
  {"        S.schema_name = '%s' and "},
  {"        O.object_name_space = 'TA' and "},
  {"        O.object_type = 'SY' "},
  {"  for read uncommitted access "},
  {"  order by 1 "},
  {"  ; "}
};
static const QueryString getIudLogTablesInSchemaQuery[] =
{
  {" select translate(%strim(object_name)%s using ucs2toutf8) from "},
  {"   HP_SYSTEM_CATALOG.system_schema.schemata S, "},
  {"    \"%s\".HP_DEFINITION_SCHEMA.objects O "},
  {"  where S.schema_uid = O.schema_uid and "},
  {"        S.schema_name = '%s' and "},
  {"        O.object_type ='BT' and O.OBJECT_NAME_SPACE='IL' "},
  {"  for read uncommitted access "},
  {" order by 1 "},
  {"  ; "}
};

static const QueryString getRangeLogTablesInSchemaQuery[] =
{
  {" select translate(%strim(object_name)%s using ucs2toutf8) from "},
  {"   HP_SYSTEM_CATALOG.system_schema.schemata S, "},
  {"    \"%s\".HP_DEFINITION_SCHEMA.objects O "},
  {"  where S.schema_uid = O.schema_uid and "},
  {"        S.schema_name = '%s' and "},
  {"        O.object_type ='BT' and O.OBJECT_NAME_SPACE='RL' "},
  {"  for read uncommitted access "},
  {" order by 1 "},
  {"  ; "}
};

static const QueryString getTrigTempTablesInSchemaQuery[] =
{
  {" select translate(%strim(object_name)%s using ucs2toutf8) from "},
  {"   HP_SYSTEM_CATALOG.system_schema.schemata S, "},
  {"    \"%s\".HP_DEFINITION_SCHEMA.objects O "},
  {"  where S.schema_uid = O.schema_uid and "},
  {"        S.schema_name = '%s' and "},
  {"        O.object_type ='BT' and O.OBJECT_NAME_SPACE='TT' "},
  {"  for read uncommitted access "},
  {" order by 1 "},
  {"  ; "}
};

static const QueryString getIudLogOnTableQuery[] =
{
  {" select translate(%strim(object_name)%s using ucs2toutf8) from "},
  {"   HP_SYSTEM_CATALOG.system_schema.schemata S, "},
  {"    \"%s\".HP_DEFINITION_SCHEMA.objects O "},
  {"  where S.schema_uid = O.schema_uid and "},
  {"        S.schema_name = '%s' and "},
  {"        O.object_name = '%s' and " },
  {"        O.object_type ='BT' and O.OBJECT_NAME_SPACE='IL' "},
  {"  for read uncommitted access "},
  {" order by 1 "},
  {"  ; "}
};

static const QueryString getRangeLogOnTableQuery[] =
{
  {" select translate(%strim(object_name)%s using ucs2toutf8) from "},
  {"   HP_SYSTEM_CATALOG.system_schema.schemata S, "},
  {"    \"%s\".HP_DEFINITION_SCHEMA.objects O "},
  {"  where S.schema_uid = O.schema_uid and "},
  {"        S.schema_name = '%s' and "},
  {"        O.object_name = '%s' and "},
  {"        O.object_type ='BT' and O.OBJECT_NAME_SPACE='RL' "},
  {"  for read uncommitted access "},
  {" order by 1 "},
  {"  ; "}
};

static const QueryString getTrigTempOnTableQuery[] =
{
  {" select translate(%strim(object_name)%s using ucs2toutf8) from "},
  {"   HP_SYSTEM_CATALOG.system_schema.schemata S, "},
  {"    \"%s\".HP_DEFINITION_SCHEMA.objects O "},
  {"  where S.schema_uid = O.schema_uid and "},
  {"        S.schema_name = '%s' and "},
  {"        O.object_type ='BT' and O.OBJECT_NAME_SPACE='TA'  and "},
  {"        O.object_name = '%s' and "},
  {"        O.object_uid  in "},
  {"              (select used_object_uid from "},
  {"               \"%s\".HP_DEFINITION_SCHEMA.trigger_used  where "},
  {"                 is_subject_table='Y') "},  
  {"  for read uncommitted access "},
  {"  order by 1 "},
  {"  ; "}
};

static const QueryString getIudLogOnMVQuery[] =
{
  {" select translate(%strim(object_name)%s using ucs2toutf8) from "},
  {"   HP_SYSTEM_CATALOG.system_schema.schemata S, "},
  {"    \"%s\".HP_DEFINITION_SCHEMA.objects O "},
  {"  where S.schema_uid = O.schema_uid and "},
  {"        S.schema_name = '%s' and "},
  {"        O.object_name = '%s' and "},
  {"        O.object_type ='BT' and O.OBJECT_NAME_SPACE='IL' "},
  {"  for read uncommitted access "},
  {" order by 1 "},
  {"  ; "}
};

static const QueryString getRangeLogOnMVQuery[] =
{
  {" select translate(%strim(object_name)%s using ucs2toutf8) from "},
  {"   HP_SYSTEM_CATALOG.system_schema.schemata S, "},
  {"    \"%s\".HP_DEFINITION_SCHEMA.objects O "},
  {"  where S.schema_uid = O.schema_uid and "},
  {"        S.schema_name = '%s' and "},
  {"        O.object_name = '%s' and "},
  {"        O.object_type ='BT' and O.OBJECT_NAME_SPACE='RL' "},
  {"  for read uncommitted access "},
  {" order by 1 "},
  {"  ; "}
};

static const QueryString getTrigTempOnMVQuery[] =
{
  
  {" select translate(%strim(object_name)%s using ucs2toutf8) from "},
  {"   HP_SYSTEM_CATALOG.system_schema.schemata S, "},
  {"    \"%s\".HP_DEFINITION_SCHEMA.objects O "},
  {"  where S.schema_uid = O.schema_uid and "},
  {"        S.schema_name = '%s' and "},
  {"        O.object_type ='MV' and O.OBJECT_NAME_SPACE='TA'  and "},
  {"        O.object_name = '%s' and "},
  {"        O.object_uid  in "},
  {"              (select used_object_uid from "},
  {"               \"%s\".HP_DEFINITION_SCHEMA.trigger_used  where "},
  {"                 is_subject_table='Y') "},  
  {"  for read uncommitted access "},
  {"  order by 1 "},
  {"  ; "}
};



static const QueryString getIndexesOnTableQuery[] =
{
  {" select translate(%strim(O.object_name)%s using ucs2toutf8) from "},
  {"   \"%s\".HP_DEFINITION_SCHEMA.access_paths AP, "},
  {"   \"%s\".HP_DEFINITION_SCHEMA.objects O "},
  {" where  "},
  {"       ap.access_path_type = 'IX' and "},
  {"       ap.table_uid = "},
  {"          (select O.object_uid from "},
  {"           HP_SYSTEM_CATALOG.system_schema.schemata S, "},
  {"           \"%s\".HP_DEFINITION_SCHEMA.objects O "},
  {"            where S.schema_name = '%s' and "},
  {"                  S.schema_uid = O.schema_uid and "},
  {"                  O.object_name = '%s' and "},
  {"                  O.object_name_space = 'TA' and "},
  {"                  O.object_type = 'BT') and "},
  {"       ap.access_path_uid = O.object_uid and "},
  {"       O.object_name_space = 'IX'  "},
  {"  for read uncommitted access "},
  {"  order by 1 "},
  {"  ; "}
};

static const QueryString getIndexesOnMvQuery[] =
{
  {" select translate(%strim(O.object_name)%s using ucs2toutf8) from "},
  {"   \"%s\".HP_DEFINITION_SCHEMA.access_paths AP, "},
  {"   \"%s\".HP_DEFINITION_SCHEMA.objects O "},
  {" where  "},
  {"       ap.access_path_type = 'IX' and "},
  {"       ap.table_uid = "},
  {"          (select O.object_uid from "},
  {"           HP_SYSTEM_CATALOG.system_schema.schemata S, "},
  {"           \"%s\".HP_DEFINITION_SCHEMA.objects O "},
  {"            where S.schema_name = '%s' and "},
  {"                  S.schema_uid = O.schema_uid and "},
  {"                  O.object_name = '%s' and "},
  {"                  O.object_name_space = 'TA' and "},
  {"                  O.object_type = 'MV') and "},
  {"       ap.access_path_uid = O.object_uid and "},
  {"       O.object_name_space = 'IX'  "},
  {"  for read uncommitted access "},
  {"  order by 1 "},
  {"  ; "}
};

// note: fields grantor and grantee are defined as char(64) on winnt
//       and INT on NSK.
static const QueryString getPrivilegesOnObjectQuery[] =
{
  {" select "},
  {"        cast(authname(TP.grantee) "},
  {"        as char(40)) || "},
  {"        case TP.privilege_type "},
  {"               when 'S' then 'SELECT'  "},
  {"               when 'I' then 'INSERT'  "},
  {"               when 'D' then 'DELETE'  "},
  {"               when 'U' then 'UPDATE'  "},
  {"               when 'R' then 'REFERENCE'  "},
  {"               when 'E' then 'EXECUTE'  "},
  {"               when 'AB' then 'ALTER TABLE' "},
  {"               when 'AM' then 'ALTER MV'  "},
  {"               when 'AS' then 'ALTER SYNONYM' "},
  {"               when 'AV' then 'ALTER VIEW'  "},
  {"               when 'T' then 'TRIGGER'  "},
  {"               when 'M' then 'MAINTAIN'  "},
  {"               when 'RF' then 'REFRESH'  "},
  {"               when 'RO' then 'REORG'  "},
  {"               when 'US' then 'UPDATE STATS'  "},
  {"               when 'AD' then 'DATABASE ADMIN'  "},
  {"               else 'UNKNOWN' end "},
  {"      from "},
  {"        HP_SYSTEM_CATALOG.system_schema.schemata S, "},
  {"        \"%s\".HP_DEFINITION_SCHEMA.objects O, "},
  {"        \"%s\".HP_DEFINITION_SCHEMA.tbl_privileges TP "},
  {"   where "},
  {"    S.schema_uid = O.schema_uid and "},
  {"    O.object_uid = TP.table_uid and "},
  {"    O.object_name_space = 'TA' and "},
  {"    O.object_type = '%s' and "},
  {"    S.schema_name = '%s' and "},
  {"    O.object_name = '%s' "},
  {"   and TP.grantor != -2 "},
  {"   order by TP.grantee, TP.privilege_type "},
  //  {"  order by 1, 2 "},
  {" ; "}
};

static const QueryString getPrivilegesOnSchemaQuery[] =
{
  {" select "},
  {"        cast(authname(SP.grantee) "},
  {"        as char(40)) || "},
// NOTE: Any change to the following list should also be made to:
//       the SCHEMA_PRIVILEGES view definition 
//       in catman/CatCreateMetadataViews.cpp
  {"        case SP.privilege_type "},
  {" 	 	WHEN _ISO88591'A'  THEN 	_ISO88591'ALTER' " }, 
  {" 	 	WHEN _ISO88591'AA' THEN		_ISO88591'ALTER ROUTINE ACTION' " },
  {" 	 	WHEN _ISO88591'AB' THEN		_ISO88591'ALTER TABLE' " },
  {" 	 	WHEN _ISO88591'AC' THEN		_ISO88591'ARCHIVE' " },
  {" 	 	WHEN _ISO88591'AD' THEN		_ISO88591'DATABASE ADMINISTRATOR' " },
  {" 	 	WHEN _ISO88591'AG' THEN		_ISO88591'ALTER MV GROUP' " },
  {" 	 	WHEN _ISO88591'AL' THEN		_ISO88591'ALTER LIBRARY' " },
  {" 	 	WHEN _ISO88591'AM' THEN		_ISO88591'ALTER MV' " },
  {" 	 	WHEN _ISO88591'AR' THEN		_ISO88591'ALTER ROUTINE' " },
  {" 	 	WHEN _ISO88591'AS' THEN		_ISO88591'ALTER SYNONYM' " },
  {" 	 	WHEN _ISO88591'AT' THEN		_ISO88591'ALTER TRIGGER' " },
  {" 	 	WHEN _ISO88591'AV' THEN		_ISO88591'ALTER VIEW' " },
  {" 	 	WHEN _ISO88591'BA' THEN		_ISO88591'BACKUP' " },
  {" 	 	WHEN _ISO88591'BC' THEN		_ISO88591'BUSINESS CONTINUITY' " },
  {" 	 	WHEN _ISO88591'C'  THEN 	_ISO88591'CREATE' " },
  {" 	 	WHEN _ISO88591'CA' THEN		_ISO88591'CREATE ROUTINE ACTION' " },
  {" 	 	WHEN _ISO88591'CB' THEN		_ISO88591'CREATE TABLE' " },
  {" 	 	WHEN _ISO88591'CG' THEN		_ISO88591'CREATE MV GROUP' " },
  {" 	 	WHEN _ISO88591'CL' THEN		_ISO88591'CREATE LIBRARY' " },
  {" 	 	WHEN _ISO88591'CM' THEN		_ISO88591'CREATE MV' " },
  {" 	 	WHEN _ISO88591'CP' THEN		_ISO88591'CREATE PROCEDURE' " },
  {" 	 	WHEN _ISO88591'CR' THEN		_ISO88591'CREATE ROUTINE' " },
  {" 	 	WHEN _ISO88591'CS' THEN		_ISO88591'CREATE SYNONYM' " },
  {" 	 	WHEN _ISO88591'CT' THEN		_ISO88591'CREATE TRIGGER' " },
  {" 	 	WHEN _ISO88591'CV' THEN		_ISO88591'CREATE VIEW' " },
  {" 	 	WHEN _ISO88591'D'  THEN 	_ISO88591'DELETE' " },
  {" 	 	WHEN _ISO88591'DA' THEN		_ISO88591'DROP ROUTINE ACTION' " },
  {" 	 	WHEN _ISO88591'DB' THEN		_ISO88591'DROP TABLE' " },
  {" 	 	WHEN _ISO88591'DD' THEN		_ISO88591'DROP ROUTINE' " },
  {" 	 	WHEN _ISO88591'DG' THEN		_ISO88591'DROP MV GROUP' " },
  {" 	 	WHEN _ISO88591'DL' THEN		_ISO88591'DROP LIBRARY' " },
  {" 	 	WHEN _ISO88591'DM' THEN		_ISO88591'DROP MV' " },
  {" 	 	WHEN _ISO88591'DP' THEN		_ISO88591'DROP PROCEDURE' " },
  {" 	 	WHEN _ISO88591'DR' THEN		_ISO88591'DROP' " },
  {" 	 	WHEN _ISO88591'DS' THEN		_ISO88591'DROP SYNONYM' " },
  {" 	 	WHEN _ISO88591'DT' THEN		_ISO88591'DROP TRIGGER' " },
  {" 	 	WHEN _ISO88591'DV' THEN		_ISO88591'DROP VIEW' " },
  {" 	 	WHEN _ISO88591'E'  THEN 	_ISO88591'EXECUTE' " },
  {" 	 	WHEN _ISO88591'I'  THEN 	_ISO88591'INSERT' " },
  {" 	 	WHEN _ISO88591'M'  THEN 	_ISO88591'MAINTAIN' " },
  {" 	 	WHEN _ISO88591'R'  THEN 	_ISO88591'REFERENCE' " },
  {" 	 	WHEN _ISO88591'RF' THEN		_ISO88591'REFRESH' " },
  {" 	 	WHEN _ISO88591'RO' THEN		_ISO88591'REORG' " },
  {" 	 	WHEN _ISO88591'RP' THEN		_ISO88591'REPLICATE' " },
  {" 	 	WHEN _ISO88591'RS' THEN		_ISO88591'RESTORE' " },
  {" 	 	WHEN _ISO88591'S'  THEN 	_ISO88591'SELECT' " },
  {" 	 	WHEN _ISO88591'T'  THEN 	_ISO88591'TRIGGER' " },
  {" 	 	WHEN _ISO88591'U'  THEN 	_ISO88591'UPDATE' " },
  {" 	 	WHEN _ISO88591'US' THEN		_ISO88591'UPDATE STATS' " },
  {" 	 	WHEN _ISO88591'Y'  THEN 	_ISO88591'USAGE' " },
  {" 		ELSE SP.PRIVILEGE_TYPE END " },
  {"      from "},
  {"        \"%s\".HP_DEFINITION_SCHEMA.objects O, "},
  {"        \"%s\".HP_DEFINITION_SCHEMA.sch_privileges SP "},
  {"   where "},
  {"    O.object_uid = SP.table_uid and "},
  {"    O.object_type = 'SL' and "},
  {"    O.object_name = '%s'  "},
  {"    and SP.grantor != -2 "},
  {"   order by SP.grantee, SP.privilege_type "},
  {" ; "}
};

static const QueryString getSynonymsOnTableQuery[] =
{
  {" select translate(trim(S.schema_name) || '.' || trim(O.object_name) using ucs2toutf8) from "},
  {"        HP_SYSTEM_CATALOG.system_schema.schemata S, "},
  {"        \"%s\".HP_DEFINITION_SCHEMA.objects O, "},
  {"        \"%s\".HP_DEFINITION_SCHEMA.synonym_usage SU "},
  {"   where "},
  {"    S.schema_uid = O.schema_uid and "},
  {"    O.object_uid = SU.object_uid and "},
  {"    O.object_name_space = 'TA' and "},
  {"    O.object_type = 'SY' and "},
  {"    (SU.underlying_sch_uid, SU.underlying_obj_uid) = "},
  {"     (select O1.schema_uid, O1.object_uid from "},
  {"        HP_SYSTEM_CATALOG.system_schema.schemata S1, "},
  {"        \"%s\".HP_DEFINITION_SCHEMA.objects O1 "},
  {"        where S1.schema_uid = O1.schema_uid and "},
  {"              S1.schema_name = '%s' and "},
  {"              O1.object_name = '%s' and "},
  {"              O1.object_name_space = 'TA' and "},
  {"              O1.object_type = 'BT' "},
  {"     ) "},
  {"  order by 1 "},
  {" ; "}
};

static const QueryString getViewsOnObjectQuery[] =
{
  {" select translate(trim(S.schema_name) || '.' || trim(O.object_name) using ucs2toutf8) from "},
  {"   \"%s\".HP_DEFINITION_SCHEMA.objects O /* cardinality 10e6 */, "},
  {"   HP_SYSTEM_CATALOG.system_schema.schemata S "},
  {" where O.schema_uid = S.schema_uid and   "},
  {"   O.object_uid in "},
  {"   (select using_view_uid from \"%s\".HP_DEFINITION_SCHEMA.vw_tbl_usage "},
  {"    where used_obj_uid in  "},
  {"     (select object_uid from  "},
  {"       \"%s\".HP_DEFINITION_SCHEMA.objects O1 /* cardinality 10e6 */, "},
  {"       HP_SYSTEM_CATALOG.system_schema.schemata S1 "},
  {"      where S1.schema_name = '%s' "},
  {"          and S1.schema_uid = O1.schema_uid "},
  {"          and O1.object_name = '%s' "},
  {"          %s "},
  {"     ) "},
  {"   ) "},
  {" order by 1 "},
  {" ; "}
};

static const QueryString getMvsOnObjectQuery[] =
{
  {" select translate(trim(S.schema_name) || '.' || trim(O.object_name) using ucs2toutf8) from "},
  {"   \"%s\".HP_DEFINITION_SCHEMA.objects O /* cardinality 10e6 */, "},
  {"   HP_SYSTEM_CATALOG.system_schema.schemata S "},
  {" where O.schema_uid = S.schema_uid and   "},
  {"   O.object_uid in "},
  {"   (select mv_uid from \"%s\".HP_DEFINITION_SCHEMA.mvs_used "},
  {"    where used_object_uid in  "},
  {"     (select object_uid from  "},
  {"       \"%s\".HP_DEFINITION_SCHEMA.objects O1 /* cardinality 10e6 */, "},
  {"       HP_SYSTEM_CATALOG.system_schema.schemata S1 "},
  {"      where S1.schema_name = '%s' "},
  {"          and S1.schema_uid = O1.schema_uid "},
  {"          and O1.object_name = '%s' "},
  {"          %s "},
  {"     ) "},
  {"   ) "},
  {" order by 1 "},
  {" ; "}
};

static const QueryString getPartitionsForTableQuery[] =
{
  {" select trim(P.system_name) || '.' || trim(P.data_source) "},
  {"        || '.' || trim(P.file_suffix) || ' (' || "},
  {"        trim(P.partition_name) || ')' "},
  {" from "},
  {"   \"%s\".HP_DEFINITION_SCHEMA.partitions P "},
  {" where  "},
  {"       P.object_uid = %s"},
  {"  for read uncommitted access "},
  {"  ; "}
};

static const QueryString getPartitionsForIndexQuery[] =
{
  {" select trim(P.system_name) || '.' || trim(P.data_source) "},
  {"        || '.' || trim(P.file_suffix) || ' (' || "},
  {"        trim(P.partition_name) || ')' "},
  {" from "},
  {"   \"%s\".HP_DEFINITION_SCHEMA.partitions P "},
  {" where  "},
  {"       P.object_uid = %s"},
  {"  ; "}
};

static const QueryString getObjectsInViewQuery[] =
{
  {" select translate(trim(S.schema_name) || '.' || trim(O.object_name) using ucs2toutf8), "},
  {"        O.object_type from "},
  {"  \"%s\".HP_DEFINITION_SCHEMA.vw_tbl_usage, "},
  {"  \"%s\".HP_DEFINITION_SCHEMA.objects O, "},
  {"  HP_SYSTEM_CATALOG.system_schema.schemata S "},
  {" where using_view_uid = "},
  {" ( "},
  {" select O2.object_uid from "},
  {"   \"%s\".HP_DEFINITION_SCHEMA.objects O2 /* cardinality 10e6 */, "},
  {"   HP_SYSTEM_CATALOG.system_schema.schemata S2 "},
  {" where O2.schema_uid = S2.schema_uid and  "},
  {"       O2.object_type = 'VI' and "},
  {"       S2.schema_name = '%s' and "},
  {"       O2.object_name = '%s' "},
  {" ) "},
  {" and used_obj_uid = O.object_uid  "},
  {" and O.schema_uid = S.schema_uid "},
  {" %s "},
  {" ; "},
};

static const QueryString getObjectsInMVQuery[] =
{
  {" select translate(trim(S.schema_name) || '.' || trim(O.object_name) using ucs2toutf8), "},
  {"        O.object_type from "},
  {"  \"%s\".HP_DEFINITION_SCHEMA.mvs_used M, "},
  {"  \"%s\".HP_DEFINITION_SCHEMA.objects O, "},
  {"  HP_SYSTEM_CATALOG.system_schema.schemata S "},
  {" where M.mv_uid = "},
  {" ( "},
  {" select O2.object_uid from "},
  {"   \"%s\".HP_DEFINITION_SCHEMA.objects O2 /* cardinality 10e6 */, "},
  {"   HP_SYSTEM_CATALOG.system_schema.schemata S2 "},
  {" where O2.schema_uid = S2.schema_uid and  "},
  {"       O2.object_type = 'MV' and "},
  {"       S2.schema_name = '%s' and "},
  {"       O2.object_name = '%s' "},
  {" ) "},
  {" and M.used_object_uid = O.object_uid  "},
  {" and O.schema_uid = S.schema_uid "},
  {" %s "},
  {" ; "},
};

static const QueryString getRolesForRoleQuery[] =
{
  {" select translate(rtrim(R.role_name) using ucs2toutf8) from "},
  {"   hp_system_catalog.hp_security_schema.roles R, "},
  {"   hp_system_catalog.hp_security_schema.id_mapping I, "},
  {"   hp_system_catalog.hp_security_schema.role_usage RU "},
  {" where (R.role_id=RU.role_id) and "},
  {"       (I.auth_id=RU.grantee) and "},
  {"       (I.id_type='R') and "},
  {"       (RU.grantor != -2) and "},
  {"       (R.role_name='%s') "},
  {"        %s "},
  {" for read uncommitted access "},
  {" order by 1"},
  {" ; "}
};

static const QueryString getUsersForRoleQuery[] =
{
  {" select translate(rtrim(RU.grantee_name) using ucs2toutf8) "},
  {"   from %s.\"%s\".%s RU "},
  {" where (RU.grantor_ID != -2) and "},
  {"       (RU.role_name='%s') "},
  {" for read uncommitted access "},
  {" order by 1"},
  {" ; "}
};

static const QueryString getUsersQuery[] =
{
  {" select translate(trim(user_name) using ucs2toutf8) from "},
  {"   HP_SYSTEM_CATALOG.HP_SECURITY_SCHEMA.users   U "},
  {"        %s "},
  {" for read uncommitted access "},
  {" order by 1 "},
  {" ; "}
};

// How to get current_user without selecting from a table?
static const QueryString getCurrentUserQuery[] =
{
  {" values (CURRENT_USER) "},
  {" ; "}
};

static const QueryString getCatalogsForUserQuery[] =
{
  {" select translate(rtrim(cat_name) using ucs2toutf8) from "},
  {" hp_system_catalog.system_schema.catsys c, "},
  {" hp_system_catalog.hp_security_schema.users u "},
  {" where	(c.cat_owner=u.user_id) and "},
  {       " (u.user_name='%s') "},
  {       " %s "},
  {" for read uncommitted access "},
  {" order by c.cat_name "},
  {" ; "}
};

static const QueryString getSchemasForUserQuery[] =
{
  {" select translate(rtrim(c.cat_name) || '.' || rtrim(s.schema_name) using ucs2toutf8) from "},
  {" hp_system_catalog.system_schema.catsys C, "},
  {" hp_system_catalog.system_schema.schemata S, "},
  {" hp_system_catalog.hp_security_schema.users U "},
  {" where	(C.cat_uid=S.cat_uid) and "},
  {       " (S.schema_owner=U.user_id) and "},
  {       " (U.user_name='%s') "},
  {       " %s "},
  {" for read uncommitted access "},
  {" order by C.cat_name, S.schema_name "},
  {" ; "}
};

static const QueryString getRolesForUserQuery[] =
{
  {" select translate(rtrim(RU.role_name) using ucs2toutf8) "},
  {"   from %s.\"%s\".%s RU "},
  {" where (RU.grantor_ID != -2) and "},
  {"       (RU.grantee_name='%s') "},
  {" for read uncommitted access "},
  {" order by 1 "},
  {" ; "}
};

static const QueryString getObjsForUserQuery[] =
{
  {" SELECT translate(rtrim(cat_name) || '.' || rtrim(sch_name) || '.' || rtrim(obj_name) using ucs2toutf8)"},
  {" FROM objsForUser "},
  {" FOR READ UNCOMMITTED ACCESS "},
  {" ORDER BY cat_name, sch_name, obj_name"},
  {" ; "}
};

static const QueryString getPrivsForUserQuery[] =
{
  {" SELECT translate(rtrim(cat_name) || '.' || rtrim(sch_name) || '.' || rtrim(obj_name) using ucs2toutf8)"},
  {    " || '  ' || priv_type "},
  {" FROM privsForUser "},
  {" FOR READ UNCOMMITTED ACCESS "},
  {" ORDER BY cat_name, sch_name, obj_name, priv_type"},
  {" ; "}
};

static const QueryString getProceduresForLibraryQuery[] =
{
  // 1) could check that object_type of entry is a library
  //    but if it is not, UID won't be found in routines table
  // 2) could use routine_libs_usage instead of routines table once 
  //    UID size issue is fixed.   It is not a keyed access either way
  //    unless we add a secondary index on library_uid 
  {" SELECT translate(trim(s_proc.schema_name) || '.' ||       "}, 
  {"               trim(o_proc.object_name) using ucs2toutf8)  "},
  {" FROM   \"%s\".hp_definition_schema.objects        o_proc, "}, 
  {"        hp_system_catalog.system_schema.schemata   s_proc, "},
  {"        \"%s\".hp_definition_schema.objects        o_lib,  "},
  {"        hp_system_catalog.system_schema.schemata   s_lib,  "},
  {"        \"%s\".hp_definition_schema.routines       r       "},
  {" WHERE  s_lib.schema_name = '%s'                       and "},
  {"        s_lib.schema_uid = o_lib.schema_uid            and "},
  {"        o_lib.object_name = '%s'                       and "},
  {"        o_lib.object_uid = r.library_object_id         and "},
  {"        r.udr_uid = o_proc.object_uid                  and "},
  {"        o_proc.schema_uid = s_proc.schema_uid              "},
  {" ORDER BY 1    "},
  {" ; "}
};
//SELECT * FROM ROUTINE_LIBS_USAGE WHERE LIBRARY_UID = 
// Get UID from OBJECTS table based on library name?
static const QueryString getComponents[] =
{
  {" select translate(rtrim(component_name) using ucs2toutf8)  "},
  {"   from %s.\"%s\".%s "},
  {" for read uncommitted access "},
  {" order by component_name "},
  {" ; "}
};

static const QueryString getComponentOperations[] = 
{
  {" select translate(rtrim(operation_name) using ucs2toutf8), "},
  {"        translate(rtrim(operation_code) using ucs2toutf8) from "},
  {"    %s.\"%s\".%s c, "},
  {"    %s.\"%s\".%s o "},
  {" where (c.component_uid=o.component_uid) and "},
  {"       (c.component_name='%s')  "},
  {" for read uncommitted access "},
  {" order by 1 "},
  {" ; "}
};

static const QueryString getComponentPrivilegesForUser[] =
{
  {" select distinct translate(rtrim(o.operation_name) using ucs2toutf8), "},
  {"                 translate(rtrim(o.operation_code) using ucs2toutf8) from "},
  {"    %s.\"%s\".%s c, "},
  {"    %s.\"%s\".%s o, "},
  {"    %s.\"%s\".%s p "},
  {" where (c.component_uid = p.component_uid) and "},
  {"       (c.component_uid = o.component_uid) and "},
  {"       (c.component_name='%s') and "},
  {"       (p.operation_code = o.operation_code) and "},
  {"       ((p.grantee_name = '%s') or "},
  {"        (p.grantee_name in (select role_name from "},
  {"          %s.\"%s\".%s ru "},
  {"          where ru.grantee_name = '%s'))"},
  {" order by 1 " },
  {" for read uncommitted access "},
  {" ; " }
};





static const QueryString getTrafTablesInSchemaQuery[] =
{
  {" select object_name  from "},
  {"   %s.\"%s\".%s T "},
  {"  where T.catalog_name = '%s' and "},
  {"        T.schema_name = '%s'  and "},
  {"        T.object_type = 'BT'  "},
  {"  for read uncommitted access "},
  {"  order by 1 "},
  {"  ; "}
};

static const QueryString getTrafIndexesInSchemaQuery[] =
{
  {" select object_name  from "},
  {"   %s.\"%s\".%s T "},
  {"  where T.catalog_name = '%s' and "},
  {"        T.schema_name = '%s'  and "},
  {"        T.object_type = 'IX'  "},
  {"  for read uncommitted access "},
  {"  order by 1 "},
  {"  ; "}
};

static const QueryString getTrafProceduresInSchemaQuery[] =
{
  {" select object_name  from "},
  {"   %s.\"%s\".%s T, %s.\"%s\".%s R "},
  {"  where T.catalog_name = '%s' and "},
  {"        T.schema_name = '%s'  and "},
  {"        T.object_type = 'UR'  and "},
  {"        T.object_uid = R.udr_uid  and "},
  {"        R.udr_type = 'P ' "},
  {"  for read uncommitted access "},
  {"  order by 1 "},
  {"  ; "}
};

static const QueryString getTrafLibrariesInSchemaQuery[] =
{
  {" select object_name  from "},
  {"   %s.\"%s\".%s T "},
  {"  where T.catalog_name = '%s' and "},
  {"        T.schema_name = '%s'  and "},
  {"        T.object_type = 'LB' "},
  {"  for read uncommitted access "},
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
  {"        R.udr_type = 'F ' "},
  {"  for read uncommitted access "},
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
  {"        R.udr_type = 'T ' "},
  {"  for read uncommitted access "},
  {"  order by 1 "},
  {"  ; "}
};

static const QueryString getTrafProceduresForLibraryQuery[] =
{
  {" select T.schema_name || '.' || T.object_name  from "},
  {"   %s.\"%s\".%s T, %s.\"%s\".%s R, %s.\"%s\".%s LU "},
  {"where T.object_uid = R.udr_uid  and "},
  {"      T.object_uid = LU.used_udr_uid  and "},
  {"      LU.using_library_uid = (select object_uid from %s.\"%s\".%s T1 "},
  {"      where T1.object_type = 'LB' and T1.catalog_name = '%s' and "},
  {"            T1.schema_name = '%s' and T1.object_name = '%s') and "},
  {"      %s  "}, // fot udr_type: procedure, function, or table_mapping fn.
  {"  for read uncommitted access "},
  {"  order by 1 "},
  {"  ; "}
};

static const QueryString getTrafSequencesInSchemaQuery[] =
{
  {" select object_name  from "},
  {"   %s.\"%s\".%s T "},
  {"  where T.catalog_name = '%s' and "},
  {"        T.schema_name = '%s'  and "},
  {"        T.object_type = 'SG' "},
  {"  for read uncommitted access "},
  {"  order by 1 "},
  {"  ; "}
};

static const QueryString getTrafSequencesInCatalogQuery[] =
{
  {" select trim(schema_name) || '.' || object_name  from "},
  {"   %s.\"%s\".%s T "},
  {"  where T.catalog_name = '%s' and "},
  {"        T.object_type = 'SG' "},
  {"  for read uncommitted access "},
  {"  order by 1 "},
  {"  ; "}
};

static const QueryString getTrafViewsInCatalogQuery[] =
{
  {" select T.schema_name || '.' || "},
  {" T.object_name from "},
  {"   %s.\"%s\".%s T,  %s.\"%s\".%s V "},
  {"  where V.view_uid = T.object_uid and "},
  {"            T.catalog_name = '%s' "},
  {"  for read uncommitted access "},
  {" order by 1 "},
  {"  ; "}
};

static const QueryString getTrafViewsInSchemaQuery[] =
{
  {" select T.object_name from "},
  {"   %s.\"%s\".%s T,  %s.\"%s\".%s V "},
  {"  where V.view_uid = T.object_uid and "},
  {"             T.catalog_name = '%s' and "},
  {"             T.schema_name = '%s' "},
  {"  for read uncommitted access "},
  {" order by 1 "},
  {"  ; "}
};

static const QueryString getTrafObjectsInViewQuery[] =
{
  {" select T.object_name from "},
  {"   %s.\"%s\".%s VU,  %s.\"%s\".%s T "},
  {"  where VU.using_view_uid = "},
  {"     (select T2.object_uid from  %s.\"%s\".%s T2 "},
  {"         where T2.catalog_name = '%s' and "},
  {"                   T2.schema_name = '%s' and "},
  {"                   T2.object_name = '%s' ) "},
  {"     and VU.used_object_uid = T.object_uid "},
  {"  for read uncommitted access "},
  {" order by 1 "},
  {"  ; "}
};

static const QueryString getTrafViewsOnObjectQuery[] =
{
  {" select %sT.object_name %s  from "},
  {"   %s.\"%s\".%s T "},
  {"   where T.object_uid in "},
  {"   (select using_view_uid from  %s.\"%s\".%s VU "},
  {"    where used_object_uid in  "},
  {"     (select object_uid from  "},
  {"        %s.\"%s\".%s T1 "},
  {"      where T1.catalog_name = '%s' "},
  {"          and T1.schema_name = '%s' "},
  {"          and T1.object_name = '%s' "},
  {"          %s "},
  {"     ) "},
  {"   ) "},
  {" order by 1 "},
  {" ; "}
};

static const QueryString getTrafSchemasInCatalogQuery[] =
{
  {" select distinct T.schema_name "},
  {"   from %s.\"%s\".%s T "},
  {"  where T.catalog_name = '%s' "},
  {"  for read uncommitted access "},
  {" order by 1 "},
  {"  ; "}
};

static const QueryString getTrafUsers[] = 
{
  {" select distinct T.auth_db_name "},
  {"   from %s.\"%s\".%s T "},
  {"  where T.auth_type = '%s' "},
  {"  for read uncommitted access "},
  {" order by 1 "},
  {"  ; "}
};

static const QueryString getTrafRoles[] = 
{
  {" select distinct T.auth_db_name "},
  {"   from %s.\"%s\".%s T "},
  {"  where T.auth_type = 'R' "},
  {"  for read uncommitted access "},
  {" order by 1 "},
  {"  ; "}
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
		cliInterface()->retrieveSQLDiagnostics(getDiagsArea());

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
		cliInterface()->retrieveSQLDiagnostics(getDiagsArea());

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
		cliInterface()->retrieveSQLDiagnostics(getDiagsArea());

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
	    else if (getMItdb().queryType_ == ComTdbExeUtilGetMetadataInfo::TABLES_IN_MV_)
	      strcpy(objectStr, "tables");
	    else if (getMItdb().queryType_ == ComTdbExeUtilGetMetadataInfo::MVS_IN_MV_)
	      strcpy(objectStr, "mvs");
	    else if (getMItdb().queryType_ == ComTdbExeUtilGetMetadataInfo::OBJECTS_IN_MV_)
	      strcpy(objectStr, "objects");

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
		cliInterface()->retrieveSQLDiagnostics(getDiagsArea());

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
		cliInterface()->retrieveSQLDiagnostics(getDiagsArea());

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
		cliInterface()->retrieveSQLDiagnostics(getDiagsArea());

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

ex_expr::exp_return_type ExExeUtilGetMetadataInfoTcb::evalScanExpr(char * ptr, Lng32 len)
{
  ex_expr::exp_return_type exprRetCode = ex_expr::EXPR_OK;

  if (getMItdb().scanExpr_)
    {
      ex_queue_entry * pentry_down = qparent_.down->getHeadEntry();

      char * exprPtr = new(getGlobals()->getDefaultHeap())
	char[SQL_VARCHAR_HDR_SIZE + len];
      short shortLen = (short)len;
      str_cpy_all((char*)exprPtr, (char*)&shortLen, SQL_VARCHAR_HDR_SIZE);
      str_cpy_all(&exprPtr[SQL_VARCHAR_HDR_SIZE], ptr, shortLen);
      workAtp_->getTupp(getMItdb().workAtpIndex())
	.setDataPointer(exprPtr);

      exprRetCode =
	getMItdb().scanExpr_->eval(pentry_down->getAtp(), workAtp_);

      NADELETEBASIC(exprPtr, getGlobals()->getDefaultHeap());
    }

  return exprRetCode;
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

    case ComTdbExeUtilGetMetadataInfo::VIEWS_IN_CATALOG_:
      {
	str_sprintf(headingBuf_, "Views in Catalog %s",
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

    case ComTdbExeUtilGetMetadataInfo::MVS_IN_SCHEMA_:
      {
	str_sprintf(headingBuf_, "MVs in Schema %s.%s",
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

    case ComTdbExeUtilGetMetadataInfo::SYNONYMS_IN_SCHEMA_:
      {
	str_sprintf(headingBuf_, "Synonyms in Schema %s.%s",
		    getMItdb().getCat(), getMItdb().getSch());
      }
    break;

    case ComTdbExeUtilGetMetadataInfo::INDEXES_ON_TABLE_:
      {
	str_sprintf(headingBuf_, "Indexes on Table %s.%s",
		    getMItdb().getSch(), getMItdb().getObj());
      }
    break;

    case ComTdbExeUtilGetMetadataInfo::INDEXES_ON_MV_:
      {
	str_sprintf(headingBuf_, "Indexes on MV %s.%s",
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

    case ComTdbExeUtilGetMetadataInfo::PRIVILEGES_ON_MV_:
      {
	str_sprintf(headingBuf_, "Privileges on MV %s.%s",
		    getMItdb().getSch(), getMItdb().getObj());
      }
    break;

    case ComTdbExeUtilGetMetadataInfo::PRIVILEGES_ON_VIEW_:
      {
	str_sprintf(headingBuf_, "Privileges on View %s.%s",
		    getMItdb().getSch(), getMItdb().getObj());
      }
    break;

    case ComTdbExeUtilGetMetadataInfo::SYNONYMS_ON_TABLE_:
      {
	str_sprintf(headingBuf_, "Synonyms on Table %s.%s",
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

    case ComTdbExeUtilGetMetadataInfo::MVS_ON_TABLE_:
    case ComTdbExeUtilGetMetadataInfo::MVS_ON_MV_:
      {
	str_sprintf(headingBuf_,
		    (getMItdb().queryType_ == ComTdbExeUtilGetMetadataInfo::MVS_ON_TABLE_
		     ? "MVs on Table %s.%s" : "MVs ON MV %s.%s"),
		    getMItdb().getSch(), getMItdb().getObj());
      }
    break;

    case ComTdbExeUtilGetMetadataInfo::PARTITIONS_FOR_TABLE_:
      {
	str_sprintf(headingBuf_, "Partitions for Table %s.%s",
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
    case ComTdbExeUtilGetMetadataInfo::CURRENT_USER_:
      {
        str_sprintf(headingBuf_,
                    (getMItdb().queryType_ == ComTdbExeUtilGetMetadataInfo::USERS_
                     ? "Users" : "Current User")
        );
      }
    break;

    case ComTdbExeUtilGetMetadataInfo::CATALOGS_FOR_USER_:
        str_sprintf(headingBuf_,"Catalogs for User %s",getMItdb().getParam1());
    break;

    case ComTdbExeUtilGetMetadataInfo::INDEXES_FOR_USER_:
        str_sprintf(headingBuf_,"Indexes for User %s",getMItdb().getParam1());
    break;

    case ComTdbExeUtilGetMetadataInfo::LIBRARIES_FOR_USER_:
        str_sprintf(headingBuf_,"Libraries for User %s", getMItdb().getParam1());
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

    case ComTdbExeUtilGetMetadataInfo::MVS_FOR_USER_:
        str_sprintf(headingBuf_,"Materialized Views for User %s",getMItdb().getParam1());
    break;

    case ComTdbExeUtilGetMetadataInfo::MVGROUPS_FOR_USER_:
        str_sprintf(headingBuf_,"Materialized View Groups for User %s",getMItdb().getParam1());
    break;

    case ComTdbExeUtilGetMetadataInfo::PRIVILEGES_FOR_USER_:
        str_sprintf(headingBuf_,"Privileges for User %s",getMItdb().getParam1());
    break;

    case ComTdbExeUtilGetMetadataInfo::PROCEDURES_FOR_USER_:
        str_sprintf(headingBuf_,"Procedures for User %s",getMItdb().getParam1());
    break;

    case ComTdbExeUtilGetMetadataInfo::ROLES_FOR_USER_:
        str_sprintf(headingBuf_,"Roles for User %s",getMItdb().getParam1());
    break;

    case ComTdbExeUtilGetMetadataInfo::SCHEMAS_FOR_USER_:
        str_sprintf(headingBuf_,"Schemas for User %s",getMItdb().getParam1());
    break;

    case ComTdbExeUtilGetMetadataInfo::SYNONYMS_FOR_USER_:
        str_sprintf(headingBuf_,"Synonyms for User %s",getMItdb().getParam1());
    break;

    case ComTdbExeUtilGetMetadataInfo::TABLES_FOR_USER_:
        str_sprintf(headingBuf_,"Tables for User %s",getMItdb().getParam1());
    break;

    case ComTdbExeUtilGetMetadataInfo::TRIGGERS_FOR_USER_:
        str_sprintf(headingBuf_,"Triggers for User %s",getMItdb().getParam1());
    break;

    case ComTdbExeUtilGetMetadataInfo::VIEWS_FOR_USER_:
        str_sprintf(headingBuf_,"Views for User %s",getMItdb().getParam1());
    break;

    case ComTdbExeUtilGetMetadataInfo::COMPONENTS_:
        str_sprintf(headingBuf_, "Components");
    break;

   case ComTdbExeUtilGetMetadataInfo::COMPONENT_OPERATIONS_:
      {
        str_sprintf(headingBuf_, "Operations for Component %s",
                    getMItdb().getObj());
        break;
      }
   case ComTdbExeUtilGetMetadataInfo::COMPONENT_PRIVILEGES_:
      {	//TODO: Add user?
        str_sprintf(headingBuf_, "Privilege information on Component %s",
                    getMItdb().getObj());
        break;
      }

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

//////////////////////////////////////////////////////////////////////
// PrivilengesTypeForUserQuery() for ExExeUtilGetMetadataInfoTcb
//////////////////////////////////////////////////////////////////////
Lng32 ExExeUtilGetMetadataInfoTcb::setupPrivilegesTypeForUserQuery()
{
  static const char setupSchemaPrivsForUserQuery[] =
  {" INSERT INTO privsForUser "
   " SELECT _UCS2'%s',"
          " S.schema_name,"
          " _UCS2'',"
     " CASE SP.privilege_type "
       " WHEN 'S'  THEN 'SELECT'"
       " WHEN 'D'  THEN 'DELETE'"
       " WHEN 'I'  THEN 'INSERT'"
       " WHEN 'U'  THEN 'UPDATE'"
       " WHEN 'R'  THEN 'REFERENCE'"
       " WHEN 'E'  THEN 'EXECUTE'"
       " WHEN 'AB' THEN 'ALTER TABLE'"
       " WHEN 'AD' THEN 'DATABASE ADMIN'"
       " WHEN 'AM' THEN 'ALTER MV'"
       " WHEN 'AS' THEN 'ALTER SYNONYM'"
       " WHEN 'AT' THEN 'ALTER TRIGGER'"
       " WHEN 'AV' THEN 'ALTER VIEW'"
       " WHEN 'CB' THEN 'CREATE TABLE'"
       " WHEN 'CM' THEN 'CREATE MV'"
       " WHEN 'CV' THEN 'CREATE VIEW'"
       " WHEN 'DB' THEN 'DROP TABLE'"
       " WHEN 'DV' THEN 'DROP VIEW'"
       " ELSE SP.privilege_type"
     " END "
   " FROM HP_SYSTEM_CATALOG.SYSTEM_SCHEMA.schemata S,"
        " HP_SYSTEM_CATALOG.HP_SECURITY_SCHEMA.users U,"
        " %s.HP_DEFINITION_SCHEMA.objects O,"
        " %s.HP_DEFINITION_SCHEMA.sch_privileges SP"
   " WHERE U.user_name   = '%s' AND "
         " S.cat_uid     = %Ld AND "
         " S.schema_uid  = O.schema_uid AND "
         " O.object_uid  = SP.table_uid AND "
         " SP.grantee    = U.user_id AND "
         " SP.grantor   != -2 "
   " FOR READ UNCOMMITTED ACCESS "
   " ; "
  };

  static const char setupTablePrivsForUserQuery[] =
  {" INSERT INTO privsForUser "
   " SELECT _UCS2'%s',"
          " S.schema_name,"
     " CAST (CASE "
             " WHEN O.object_type = 'SL' THEN ''"
             " ELSE O.object_name "
           " END "
           " AS char(128)), "
     " CASE TP.privilege_type "
       " WHEN 'S'  THEN 'SELECT'"
       " WHEN 'D'  THEN 'DELETE'"
       " WHEN 'I'  THEN 'INSERT'"
       " WHEN 'U'  THEN 'UPDATE'"
       " WHEN 'R'  THEN 'REFERENCE'"
       " ELSE TP.privilege_type"
     " END "
   " FROM HP_SYSTEM_CATALOG.SYSTEM_SCHEMA.schemata S,"
        " HP_SYSTEM_CATALOG.HP_SECURITY_SCHEMA.users U,"
        " %s.HP_DEFINITION_SCHEMA.objects O,"
        " %s.HP_DEFINITION_SCHEMA.tbl_privileges TP"
   " WHERE U.user_name  = '%s' AND"
         " S.cat_uid    = %Ld AND"
         " S.schema_uid = O.schema_uid AND"
         " O.object_uid = TP.table_uid AND"
         " TP.grantee   = U.user_id AND"
         " TP.grantor  != -2"
   " FOR READ UNCOMMITTED ACCESS"
   " ; "
  };

  short rc      = 0;
  Lng32 cliRC   = 0;
  Lng32 len     = 0;
  Lng32 retcode = SQL_Success;

  char * user_name = getMItdb().getParam1();  // ucs2 or utf8 param1?

  ExeCliInterface cliInterface(getHeap());

  cliRC = cliInterface.executeImmediate(
    "create volatile table privsForUser "
    "(cat_name  char(128) CHARACTER SET UCS2 CASESPECIFIC  NOT NULL NOT DROPPABLE,"
    " sch_name  char(128) CHARACTER SET UCS2 CASESPECIFIC  NOT NULL NOT DROPPABLE,"
    " obj_name  char(128) CHARACTER SET UCS2 CASESPECIFIC  default NULL,"
    " priv_type char( 14) CHARACTER SET ISO88591 CASESPECIFIC NOT NULL NOT DROPPABLE, "
    " primary key(cat_name, sch_name, obj_name, priv_type)"
    ") no partition; ");
  if (cliRC < 0) return cliRC;

  sprintf(queryBuf_,"select translate(rtrim(cat_name) using ucs2toutf8), cat_uid "
                    "from hp_system_catalog.system_schema.catsys;"
         );

  if (initializeInfoList(infoList_)) return -1;

  numOutputEntries_ = 2;
  cliRC = fetchAllRows(infoList_, queryBuf_, numOutputEntries_, FALSE, rc);
  if (cliRC < 0) return cliRC;

  infoList_->position();
  for (Int32 i32 = 0; i32 < infoList_->numEntries(); i32++)
  {
    OutputInfo * vi = (OutputInfo*)infoList_->getCurr();
    char * cat_name = vi->get(0);
    NAString extCatName(ToAnsiIdentifier(cat_name));

    char * ptr = vi->get(1);
    len = (short)(ptr ? strlen(ptr) : 0);
    Int64 * cat_uid = (Int64 *)ptr;

    // Process table of schema privileges for current catalog
    sprintf(queryBuf_, setupSchemaPrivsForUserQuery,
            cat_name, extCatName.data(), extCatName.data(), user_name, *cat_uid);

    cliRC = cliInterface.executeImmediate(queryBuf_);
    if (cliRC < 0) return cliRC;

    // Process table of table privileges for current catalog
    sprintf(queryBuf_, setupTablePrivsForUserQuery,
            cat_name, extCatName.data(), extCatName.data(), user_name, *cat_uid);

    cliRC = cliInterface.executeImmediate(queryBuf_);
    if (cliRC < 0) return cliRC;

    infoList_->advance();
  }

  return cliRC;

} // ExExeUtilGetMetadataInfoTcb::setupPrivilegesTypeForUserQuery

//////////////////////////////////////////////////////////////////
// setupObjectTypeForUserQuery() for ExExeUtilGetMetadataInfoTcb
//////////////////////////////////////////////////////////////////
Lng32 ExExeUtilGetMetadataInfoTcb::setupObjectTypeForUserQuery()
{
  static const char setupObjsForUserQuery[] =
  {" INSERT into objsForUser"
   " SELECT _UCS2'%s',"
          " S.schema_name,"
          " O.object_name"
   " FROM HP_SYSTEM_CATALOG.SYSTEM_SCHEMA.SCHEMATA S,"
        " HP_SYSTEM_CATALOG.HP_SECURITY_SCHEMA.USERS U,"
        " %s.HP_DEFINITION_SCHEMA.OBJECTS O"
   " WHERE S.CAT_UID      = %Ld AND"
         " S.SCHEMA_UID   = O.SCHEMA_UID AND"
         " U.USER_NAME    = '%s' AND"
         " O.OBJECT_TYPE  = '%s' AND"
         " O.OBJECT_OWNER = U.USER_ID"
         " %s"
   " FOR READ UNCOMMITTED ACCESS"
   " ; "
  };

  short rc      = 0;
  Lng32 cliRC   = 0;
  Lng32 len     = 0;
  Lng32 retcode = SQL_Success;
  char obj_type[3];
  char predStr[500] = {""};

  char * user_name = getMItdb().getParam1();

  switch (getMItdb().queryType_)
  {
    case ComTdbExeUtilGetMetadataInfo::INDEXES_FOR_USER_:
    {
      strcpy(obj_type, "IX");
      str_sprintf(predStr, " AND O.object_name_space = 'IX' "
                           " AND O.object_security_class = 'UT' ");
      break;
    }
    case ComTdbExeUtilGetMetadataInfo::LIBRARIES_FOR_USER_:
    {
      strcpy(obj_type, "LB");//ACH verify this is being stored as expected - SMDIO?
      break;
    }
    case ComTdbExeUtilGetMetadataInfo::MVS_FOR_USER_:
    {
      strcpy(obj_type, "MV");
      break;
    }
    case ComTdbExeUtilGetMetadataInfo::MVGROUPS_FOR_USER_:
    {
      strcpy(obj_type, "RG");
      break;
    }
    case ComTdbExeUtilGetMetadataInfo::PROCEDURES_FOR_USER_:
    {
      strcpy(obj_type, "UR");
      break;
    }
    case ComTdbExeUtilGetMetadataInfo::SYNONYMS_FOR_USER_:
    {
      strcpy(obj_type, "SY");
      break;
    }
    case ComTdbExeUtilGetMetadataInfo::TABLES_FOR_USER_:
    {
      strcpy(obj_type, "BT");
      if (getMItdb().userObjs())
      {
        str_sprintf(predStr, " and O.object_name_space = 'TA' "
                             " and O.object_security_class = 'UT' ");
      }
      else if (getMItdb().systemObjs())
      {
        str_sprintf(predStr, " and O.object_security_class <> 'UT' ");
      }
      break;
    }
    case ComTdbExeUtilGetMetadataInfo::TRIGGERS_FOR_USER_:
    {
      strcpy(obj_type, "TR");
      break;
    }
    case ComTdbExeUtilGetMetadataInfo::VIEWS_FOR_USER_:
    {
      strcpy(obj_type, "VI");
      break;
    }
    default:
      return -1;
  }

  ExeCliInterface cliInterface(getHeap());

  cliRC = cliInterface.executeImmediate(
    "create volatile table objsForUser "
    "(cat_name  char(128) CHARACTER SET UCS2 CASESPECIFIC  NOT NULL NOT DROPPABLE,"
    " sch_name  char(128) CHARACTER SET UCS2 CASESPECIFIC  NOT NULL NOT DROPPABLE,"
    " obj_name  char(128) CHARACTER SET UCS2 CASESPECIFIC  default NULL,"
    " primary key (cat_name, sch_name, obj_name)"
    ") no partition; ");
  if (cliRC < 0) return cliRC;

  sprintf(queryBuf_,"select translate(rtrim(cat_name) using ucs2toutf8), cat_uid "
                    "from hp_system_catalog.system_schema.catsys;"
         );

  if (initializeInfoList(infoList_)) return -1;

  numOutputEntries_ = 2;
  cliRC = fetchAllRows(infoList_, queryBuf_, numOutputEntries_, FALSE, rc);
  if (cliRC < 0) return cliRC;

  infoList_->position();
  for (Int32 i32 = 0; i32 < infoList_->numEntries(); i32++)
  {
    OutputInfo * vi = (OutputInfo*)infoList_->getCurr();
    char * cat_name = vi->get(0);
    NAString extCatName(ToAnsiIdentifier(cat_name));

    char * ptr = vi->get(1);
    len = (short)(ptr ? strlen(ptr) : 0);
    Int64 * cat_uid = (Int64 *)ptr;

    sprintf(queryBuf_, setupObjsForUserQuery,
            cat_name, extCatName.data(),*cat_uid, user_name, obj_type, predStr);

    cliRC = cliInterface.executeImmediate(queryBuf_);
    if (cliRC < 0) return cliRC;

    infoList_->advance();
  }

  return cliRC;

} // ExExeUtilGetMetadataInfoTcb::setupObjectTypeForUser()

Int32 ExExeUtilGetMetadataInfoTcb::setupAuthIDInfo(const char *authName,
                                                   char authID[MAX_AUTHIDTYPE_CHAR],
                                                   AuthIdType authIDType)
{
  short rc      = 0;
  Lng32 cliRC   = 0;
  char predStr[500] = {""};

  switch (authIDType)
  {
    case ROLES_:
      str_sprintf(predStr, "select cast(auth_id as char(10)) from "
           " hp_system_catalog.hp_security_schema.id_mapping "
           " where auth_id = (select role_id from "
           "                    hp_system_catalog.hp_security_schema.roles "
           "                   where role_name = '%s') "
           " for read uncommitted access; ",
        authName, authName);
      break;

    case USERS_:
      str_sprintf(predStr, "select cast(auth_id as char(10)) from "
           " hp_system_catalog.hp_security_schema.id_mapping "
           " where auth_id = (select user_id from "
           "                    hp_system_catalog.hp_security_schema.users "
           "                   where user_name = '%s') "
           " for read uncommitted access; ",
        authName, authName);
      break;
     
    default:
      str_sprintf(predStr, "select cast(auth_id as char(10)) from "
           " hp_system_catalog.hp_security_schema.id_mapping "
           " where (auth_id = (select user_id from "
           "                    hp_system_catalog.hp_security_schema.users "
           "                   where user_name = '%s') "
           "    or  auth_id = (select role_id from "
           "                    hp_system_catalog.hp_security_schema.roles "
           "                   where role_name = '%s')) "
           " for read uncommitted access; ",
        authName, authName);
      break;
  }

  sprintf(queryBuf_, predStr );

  if (initializeInfoList(infoList_)) return -1;

  numOutputEntries_ = 1;
  cliRC = fetchAllRows(infoList_, queryBuf_, numOutputEntries_, FALSE, rc);
  if (cliRC != 0) return cliRC;
  if (rc == 100) return rc;

  infoList_->position();
  OutputInfo * vi = (OutputInfo*)infoList_->getCurr();
  const char *pAuthID = vi->get(0);
  str_cpy_and_null (authID, pAuthID, 10);

  return cliRC;
}

Int32 ExExeUtilGetMetadataInfoTcb::setupCurrentUserInfo(char *userName)
{
  short rc      = 0;
  Lng32 cliRC   = 0;

  sprintf(queryBuf_, "values (current_user); ");

  if (initializeInfoList(infoList_)) return -1;

  numOutputEntries_ = 1;
  cliRC = fetchAllRows(infoList_, queryBuf_, numOutputEntries_, FALSE, rc);
  if (cliRC < 0) return cliRC;

  infoList_->position();
  OutputInfo * vi = (OutputInfo*)infoList_->getCurr();
  const char *pUserName = vi->get(0);
  short len = strlen(pUserName);
  str_cpy_and_null(userName, pUserName, len);

// Null terminate since the above will not unless source has a blank.

  userName[len] = '\0';

  return cliRC;
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

  char ausStr[2000];
  str_sprintf(ausStr, "");
  char predStr[2000];
  str_sprintf(predStr, "");
  char catSchValue[ComMAX_2_PART_EXTERNAL_UTF8_NAME_LEN_IN_BYTES+50];
  str_sprintf(catSchValue, "");
  char endQuote[10];
  str_sprintf(endQuote, "");

  while (1)
    {
      switch (step_)
	{
	case INITIAL_:
	  {
	    step_ = DISABLE_CQS_;

	    headingReturned_ = FALSE;

	    numOutputEntries_ = 1;

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

            if (getMItdb().getObjectUid())
              step_ = GET_OBJECT_UID_;
            else if (getMItdb().isHbase())
	      step_ = SETUP_HBASE_QUERY_;
	    else
              step_ = SETUP_QUERY_;
	  }
	break;

	case GET_OBJECT_UID_:
	  {
	    if (getObjectUid(getMItdb().getCat(), getMItdb().sch_,
			     getMItdb().obj_,
			     getMItdb().isIndex(), getMItdb().isMv(),
			     objectUid_))
	      {
		step_ = HANDLE_ERROR_;
		break;
	      }

	    step_ = SETUP_QUERY_;
	  }
	break;


	case SETUP_HBASE_QUERY_:
	  {
	    const QueryString * qs = NULL;
	    Int32 sizeOfqs = 0;
            NAString userQuery;

	    char ausStr[1000];
	    str_sprintf(ausStr, "");
	    char predStr[1000];
	    str_sprintf(predStr, "");
	    char catSchValue[400];
	    str_sprintf(catSchValue, "");
	    char endQuote[10];
	    str_sprintf(endQuote, "");

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
	    char view[100];
	    char view_usage[100];
            char auths[100];
            char role_usage[100];
            char components[100];
            char componentOperations[100];
            char routine[100];
            char library_usage[100];

	    cliInterface()->getCQDval("SEABASE_CATALOG", cat);

	    //	    strcpy(cat, TRAFODION_SYSCAT_LIT);
	    strcpy(sch, SEABASE_MD_SCHEMA);
	    strcpy(pmsch, SEABASE_PRIVMGR_SCHEMA);
	    strcpy(tab, SEABASE_OBJECTS);
	    strcpy(view, SEABASE_VIEWS);
	    strcpy(view_usage, SEABASE_VIEWS_USAGE);
            strcpy(auths, SEABASE_AUTHS);
            strcpy(role_usage, "ROLE_USAGE");
            strcpy(components, "COMPONENTS");
            strcpy(componentOperations, "COMPONENT_OPERATIONS");
            strcpy(routine, SEABASE_ROUTINES);
            strcpy(library_usage, SEABASE_LIBRARIES_USAGE);


          // "get components;" or "get component privileges on <component>;" are called,
          // but authorization tables were not initialized,
	      if(getMItdb().queryType_ == ComTdbExeUtilGetMetadataInfo::COMPONENTS_
	      ||getMItdb().queryType_ == ComTdbExeUtilGetMetadataInfo::COMPONENT_OPERATIONS_
	      ||getMItdb().queryType_ == ComTdbExeUtilGetMetadataInfo::COMPONENT_PRIVILEGES_)
	      {
                NAString privMDLoc = getMItdb().cat_.getPointer();
                privMDLoc += ".\"";
                privMDLoc += SEABASE_PRIVMGR_SCHEMA;
                privMDLoc += "\"";
                PrivMgrCommands privMgrInterface(privMDLoc.data(), CmpCommon::diags());
                if (!privMgrInterface.isAuthorizationEnabled())
                {
                    ComDiagsArea * diags = getDiagsArea();
                    *diags << DgSqlCode(-CAT_AUTHORIZATION_NOT_ENABLED);
                    step_ = HANDLE_ERROR_;
                    break;
                }
         }

	    switch (getMItdb().queryType_)
	      {
	      case ComTdbExeUtilGetMetadataInfo::TABLES_IN_SCHEMA_:
		{
		  qs = getTrafTablesInSchemaQuery;
		  sizeOfqs = sizeof(getTrafTablesInSchemaQuery);

		  param_[0] = cat;
		  param_[1] = sch;
		  param_[2] = tab;
		  param_[3] = getMItdb().cat_;
		  param_[4] = getMItdb().sch_;
		}
	      break;
	      
	      case ComTdbExeUtilGetMetadataInfo::INDEXES_IN_SCHEMA_:
		{
		  qs = getTrafIndexesInSchemaQuery;
		  sizeOfqs = sizeof(getTrafIndexesInSchemaQuery);

		  param_[0] = cat;
		  param_[1] = sch;
		  param_[2] = tab;
		  param_[3] = getMItdb().cat_;
		  param_[4] = getMItdb().sch_;
		}
	      break;
	      
	      case ComTdbExeUtilGetMetadataInfo::VIEWS_IN_CATALOG_:
		{
		  qs = getTrafViewsInCatalogQuery;
		  sizeOfqs = sizeof(getTrafViewsInCatalogQuery);

		  param_[0] = cat;
		  param_[1] = sch;
		  param_[2] = tab;
		  param_[3] = cat;
		  param_[4] = sch;
		  param_[5] = view;
		  param_[6] = getMItdb().cat_;
		}
	      break;
	      
	      case ComTdbExeUtilGetMetadataInfo::VIEWS_IN_SCHEMA_:
		{
		  qs = getTrafViewsInSchemaQuery;
		  sizeOfqs = sizeof(getTrafViewsInSchemaQuery);

		  param_[0] = cat;
		  param_[1] = sch;
		  param_[2] = tab;
		  param_[3] = cat;
		  param_[4] = sch;
		  param_[5] = view;
		  param_[6] = getMItdb().cat_;
		  param_[7] = getMItdb().sch_;
		}
	      break;

	      case ComTdbExeUtilGetMetadataInfo::TABLES_IN_VIEW_:
	      case ComTdbExeUtilGetMetadataInfo::VIEWS_IN_VIEW_:
	      case ComTdbExeUtilGetMetadataInfo::OBJECTS_IN_VIEW_:
		{
		  qs = getTrafObjectsInViewQuery;
		  sizeOfqs = sizeof(getTrafObjectsInViewQuery);

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
		}
	      break;

	      case ComTdbExeUtilGetMetadataInfo::VIEWS_ON_TABLE_:
	      case ComTdbExeUtilGetMetadataInfo::VIEWS_ON_VIEW_:
		{
		  qs = getTrafViewsOnObjectQuery;
		  sizeOfqs = sizeof(getTrafViewsOnObjectQuery);

		  param_[0] = catSchValue;
		  param_[1] = endQuote;
		  param_[2] = cat;
		  param_[3] = sch;
		  param_[4] = tab;
		  param_[5] = cat;
		  param_[6] = sch;
		  param_[7] = view_usage;
		  param_[8] = cat;
		  param_[9] = sch;
		  param_[10] = tab;
		  param_[11] = getMItdb().cat_;
		  param_[12] = getMItdb().sch_;
		  param_[13] = getMItdb().obj_;

		  if (getMItdb().queryType_ == ComTdbExeUtilGetMetadataInfo::VIEWS_ON_TABLE_)
		    strcpy(ausStr, " and T1.object_type = 'BT' ");
		  param_[14] = ausStr;
		}
	      break;

	      case ComTdbExeUtilGetMetadataInfo::SCHEMAS_IN_CATALOG_:
		{
		  qs = getTrafSchemasInCatalogQuery;
		  sizeOfqs = sizeof(getTrafSchemasInCatalogQuery);

		  param_[0] = cat;
		  param_[1] = sch;
		  param_[2] = tab;
		  param_[3] = getMItdb().cat_;
		}
	      break;
             case ComTdbExeUtilGetMetadataInfo::USERS_:
                {
                  qs = getTrafUsers;
                  sizeOfqs = sizeof(getTrafUsers);

                  char type[2];
                  type[0] = 'U';
                  type[1] = 0;
                  param_[0] = cat;
                  param_[1] = sch;
                  param_[2] = auths;
                  param_[3] = (char *)&type;
		}
                break;
              case ComTdbExeUtilGetMetadataInfo::PROCEDURES_IN_SCHEMA_:
                {
                  qs = getTrafProceduresInSchemaQuery;
                  sizeOfqs = sizeof(getTrafProceduresInSchemaQuery);

		  param_[0] = cat;
		  param_[1] = sch;
		  param_[2] = tab;
                  param_[3] = cat;
		  param_[4] = sch;
		  param_[5] = routine;
		  param_[6] = getMItdb().cat_;
		  param_[7] = getMItdb().sch_;
                }
                break ;
              case ComTdbExeUtilGetMetadataInfo::LIBRARIES_IN_SCHEMA_:
                {
                  qs = getTrafLibrariesInSchemaQuery;
                  sizeOfqs = sizeof(getTrafLibrariesInSchemaQuery);

		  param_[0] = cat;
		  param_[1] = sch;
		  param_[2] = tab;
		  param_[3] = getMItdb().cat_;
		  param_[4] = getMItdb().sch_;
                }
                break ;
              case ComTdbExeUtilGetMetadataInfo::FUNCTIONS_IN_SCHEMA_:
                {
                  qs = getTrafFunctionsInSchemaQuery;
                  sizeOfqs = sizeof(getTrafFunctionsInSchemaQuery);

		  param_[0] = cat;
		  param_[1] = sch;
		  param_[2] = tab;
                  param_[3] = cat;
		  param_[4] = sch;
		  param_[5] = routine;
		  param_[6] = getMItdb().cat_;
		  param_[7] = getMItdb().sch_;
                }
                break ;
	      case ComTdbExeUtilGetMetadataInfo::TABLE_FUNCTIONS_IN_SCHEMA_:
                {
                  qs = getTrafTableFunctionsInSchemaQuery;
                  sizeOfqs = sizeof(getTrafTableFunctionsInSchemaQuery);

		  param_[0] = cat;
		  param_[1] = sch;
		  param_[2] = tab;
                  param_[3] = cat;
		  param_[4] = sch;
		  param_[5] = routine;
		  param_[6] = getMItdb().cat_;
		  param_[7] = getMItdb().sch_;
                }
                break ;
              case ComTdbExeUtilGetMetadataInfo::PROCEDURES_FOR_LIBRARY_:
              case ComTdbExeUtilGetMetadataInfo::FUNCTIONS_FOR_LIBRARY_:
              case ComTdbExeUtilGetMetadataInfo::TABLE_FUNCTIONS_FOR_LIBRARY_:
                {
                  qs = getTrafProceduresForLibraryQuery;
                  sizeOfqs = sizeof(getTrafProceduresForLibraryQuery);

		  param_[0] = cat;
		  param_[1] = sch;
		  param_[2] = tab;
                  param_[3] = cat;
		  param_[4] = sch;
		  param_[5] = routine;
                  param_[6] = cat;
		  param_[7] = sch;
		  param_[8] = library_usage;
                  param_[9] = cat;
		  param_[10] = sch;
		  param_[11] = tab;
                  param_[12] = getMItdb().cat_;
		  param_[13] = getMItdb().sch_;
		  param_[14] = getMItdb().obj_;
                  if (getMItdb().queryType_ == ComTdbExeUtilGetMetadataInfo::PROCEDURES_FOR_LIBRARY_)
		    strcpy(ausStr, " R.udr_type = 'P ' ");
                  else if (getMItdb().queryType_ == ComTdbExeUtilGetMetadataInfo::FUNCTIONS_FOR_LIBRARY_)
		    strcpy(ausStr, " R.udr_type = 'F ' ");
                   else
		    strcpy(ausStr, " R.udr_type = 'T ' ");
		  param_[15] = ausStr;
                }
                break ;
                
              case ComTdbExeUtilGetMetadataInfo::ROLES_:
                {
                  qs = getTrafRoles;
                  sizeOfqs = sizeof(getTrafRoles);

                  param_[0] = cat;
                  param_[1] = sch;
                  param_[2] = auths;
                }
              break;

              case ComTdbExeUtilGetMetadataInfo::USERS_FOR_ROLE_:
                {
                  qs = getUsersForRoleQuery;
                  sizeOfqs = sizeof(getUsersForRoleQuery);
                  
                  param_[0] = cat;
                  param_[1] = pmsch;
                  param_[2] = role_usage;
                  param_[3] = getMItdb().getParam1();
                }
              break;
                
              case ComTdbExeUtilGetMetadataInfo::ROLES_FOR_USER_:
                {
                  qs = getRolesForUserQuery;
                  sizeOfqs = sizeof(getRolesForUserQuery);
                  
                  param_[0] = cat;
                  param_[1] = pmsch;
                  param_[2] = role_usage;
                  param_[3] = getMItdb().getParam1();
                }
              break;
              
              case ComTdbExeUtilGetMetadataInfo::COMPONENTS_:
              {
                qs = getComponents;
                sizeOfqs = sizeof(getComponents);

                param_[0] = cat;
                param_[1] = pmsch;
                param_[2] = components;
              }
              break;

              case ComTdbExeUtilGetMetadataInfo::COMPONENT_OPERATIONS_:
              {
                qs = getComponentOperations;
                sizeOfqs = sizeof(getComponentOperations);

                param_[0] = cat;
                param_[1] = pmsch;
                param_[2] = components;
                param_[3] = cat;
                param_[4] = pmsch;
                param_[5] = componentOperations;
                param_[6] = getMItdb().getObj();
              }
              break;

              case ComTdbExeUtilGetMetadataInfo::COMPONENT_PRIVILEGES_:
              {
                qs = getComponentPrivilegesForUser;
                sizeOfqs = sizeof(getComponentOperations);

                param_[0] = cat;
                param_[1] = pmsch;
                param_[2] = components;
                param_[3] = cat;
                param_[4] = pmsch;
                param_[5] = componentOperations;
                param_[6] = getMItdb().getObj();
              }
              break;

              case ComTdbExeUtilGetMetadataInfo::SEQUENCES_IN_CATALOG_:
                {
                  qs = getTrafSequencesInCatalogQuery;
                  sizeOfqs = sizeof(getTrafSequencesInCatalogQuery);

		  param_[0] = cat;
		  param_[1] = sch;
		  param_[2] = tab;
		  param_[3] = getMItdb().cat_;
                }
                break ;

              case ComTdbExeUtilGetMetadataInfo::SEQUENCES_IN_SCHEMA_:
                {
                  qs = getTrafSequencesInSchemaQuery;
                  sizeOfqs = sizeof(getTrafSequencesInSchemaQuery);

		  param_[0] = cat;
		  param_[1] = sch;
		  param_[2] = tab;
		  param_[3] = getMItdb().cat_;
		  param_[4] = getMItdb().sch_;
                }
                break ;

	      default:
		{
		  ExHandleErrors(qparent_,
				 pentry_down,
				 0,
				 getGlobals(),
				 NULL,
				 (ExeErrorCode)-4218,
				 NULL,
				 "GET"
				 );
		  step_ = HANDLE_ERROR_;
		}
		break;
	      }

	     Int32 qryArraySize = sizeOfqs / sizeof(QueryString);
	     char * gluedQuery;
	     Lng32 gluedQuerySize;
	     glueQueryFragments(qryArraySize, qs, 
				gluedQuery, gluedQuerySize);
	     
	     str_sprintf(queryBuf_, gluedQuery,
			 param_[0], param_[1], param_[2], param_[3], param_[4],
			 param_[5], param_[6], param_[7], param_[8], param_[9],
			 param_[10], param_[11], param_[12], param_[13], param_[14], param_[15]);
	     
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
		cliInterface()->retrieveSQLDiagnostics(getDiagsArea());

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

            // For component privileges, attach priv_type and priv_name
            // together
            if (getMItdb().queryType_ == ComTdbExeUtilGetMetadataInfo::COMPONENT_PRIVILEGES_)
            {
               char * ptr1 = vi->get(1);
               short len1 = strlen(ptr1);  

               // catenate type and name  with comma and space in between
               char catString[128 + 2 + 2 + 1];
               len = len + len1 + 2; // 2 for comma and space 
               char * rPtr = &catString[0];
               if (ptr1)
               {
                 str_cpy_all(rPtr, ptr1 ,len1);
                 rPtr[len1] = ',';
                 rPtr[len1+1] = ' ';
                 rPtr[len1+2] = 0;
                 strcat (rPtr, vi->get(0));
                 ptr = rPtr;
               }
             }

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
	    if (((getMItdb().queryType_ == ComTdbExeUtilGetMetadataInfo::TABLES_IN_VIEW_) ||
		 (getMItdb().queryType_ == ComTdbExeUtilGetMetadataInfo::TABLES_IN_MV_)) &&
		(vi->get(1) && (strcmp(vi->get(1), "BT") != 0)))
	      exprRetCode = ex_expr::EXPR_FALSE;
	    else if ((getMItdb().queryType_ == ComTdbExeUtilGetMetadataInfo::VIEWS_IN_VIEW_) &&
		(vi->get(1) && (strcmp(vi->get(1), "VI") != 0)))
	      exprRetCode = ex_expr::EXPR_FALSE;
	    else if ((getMItdb().queryType_ == ComTdbExeUtilGetMetadataInfo::MVS_IN_MV_) &&
		(vi->get(1) && (strcmp(vi->get(1), "MV") != 0)))
	      exprRetCode = ex_expr::EXPR_FALSE;

	    if (exprRetCode == ex_expr::EXPR_TRUE)
	      exprRetCode = evalScanExpr(ptr, len);
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
	    if (moveRowToUpQueue(ptr, len, &rc))
            {
	      return rc;
            }

	    infoList_->advance();
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
		(getMItdb().queryType_ == ComTdbExeUtilGetMetadataInfo::VIEWS_ON_VIEW_) ||
		(getMItdb().queryType_ == ComTdbExeUtilGetMetadataInfo::MVS_ON_TABLE_) ||
		(getMItdb().queryType_ == ComTdbExeUtilGetMetadataInfo::MVS_ON_VIEW_))

	      step_ = GET_USING_VIEWS_;
	    else if ((getMItdb().queryType_ == ComTdbExeUtilGetMetadataInfo::TABLES_IN_VIEW_) ||
		     (getMItdb().queryType_ == ComTdbExeUtilGetMetadataInfo::VIEWS_IN_VIEW_) ||
		     (getMItdb().queryType_ == ComTdbExeUtilGetMetadataInfo::OBJECTS_IN_VIEW_) ||
		     (getMItdb().queryType_ == ComTdbExeUtilGetMetadataInfo::TABLES_IN_MV_) ||
		     (getMItdb().queryType_ == ComTdbExeUtilGetMetadataInfo::MVS_IN_MV_) ||
		     (getMItdb().queryType_ == ComTdbExeUtilGetMetadataInfo::OBJECTS_IN_MV_))
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

	    exprRetCode = evalScanExpr(viewName, len);
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

	    exprRetCode = evalScanExpr(viewName, len);
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
		switch (getMItdb().queryType_)
		  {
		  case ComTdbExeUtilGetMetadataInfo::INDEXES_FOR_USER_:
		  case ComTdbExeUtilGetMetadataInfo::LIBRARIES_FOR_USER_:
		  case ComTdbExeUtilGetMetadataInfo::MVS_FOR_USER_:
		  case ComTdbExeUtilGetMetadataInfo::MVGROUPS_FOR_USER_:
		  case ComTdbExeUtilGetMetadataInfo::PROCEDURES_FOR_USER_:
		  case ComTdbExeUtilGetMetadataInfo::SYNONYMS_FOR_USER_:
		  case ComTdbExeUtilGetMetadataInfo::TABLES_FOR_USER_:
		  case ComTdbExeUtilGetMetadataInfo::VIEWS_FOR_USER_:
		    {
		      cliInterface()->executeImmediate("drop table objsForUser;");
		      break;
		    }
		  case ComTdbExeUtilGetMetadataInfo::PRIVILEGES_FOR_USER_:
		    {
		      cliInterface()->executeImmediate("drop table privsForUser;");
		      break;
		    }
		  default:
		    ;
		  } // switch
		return WORK_OK;
	      } // if (retcode
	    
	    step_ = DONE_;
	  }
	break;

	case DONE_:
	  {
	    switch (getMItdb().queryType_)
	      {
	      case ComTdbExeUtilGetMetadataInfo::INDEXES_FOR_USER_:
	      case ComTdbExeUtilGetMetadataInfo::LIBRARIES_FOR_USER_:
	      case ComTdbExeUtilGetMetadataInfo::MVS_FOR_USER_:
	      case ComTdbExeUtilGetMetadataInfo::MVGROUPS_FOR_USER_:
	      case ComTdbExeUtilGetMetadataInfo::PROCEDURES_FOR_USER_:
	      case ComTdbExeUtilGetMetadataInfo::SYNONYMS_FOR_USER_:
	      case ComTdbExeUtilGetMetadataInfo::TABLES_FOR_USER_:
	      case ComTdbExeUtilGetMetadataInfo::TRIGGERS_FOR_USER_:
	      case ComTdbExeUtilGetMetadataInfo::VIEWS_FOR_USER_:
		{
		  cliRC = cliInterface()->executeImmediate("drop table objsForUser;");
		  if (cliRC < 0)
		    {
		      cliInterface()->retrieveSQLDiagnostics(getDiagsArea());
		      retcode = handleError();
		      if (retcode == 1)		// qparent_.up->isFull()
			return WORK_OK;
		    }
		}
		break;
		
	      case ComTdbExeUtilGetMetadataInfo::PRIVILEGES_FOR_USER_:
		{
		  cliRC = cliInterface()->executeImmediate("drop table privsForUser;");
		  if (cliRC < 0)
		    {
		      cliInterface()->retrieveSQLDiagnostics(getDiagsArea());
		      retcode = handleError();
		      if (retcode == 1)		// qparent_.up->isFull()
			return WORK_OK;
		    }
		}
		break;
		
	      default:
		;
	      } // switch
	    
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
	  }
	break;

	case SETUP_QUERY_:
	  {
	    str_sprintf(patternStr_, "");
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

	      default:
		{
		  ExHandleErrors(qparent_,
				 pentry_down,
				 0,
				 getGlobals(),
				 NULL,
				 (ExeErrorCode)-4218,
				 NULL,
				 "GET"
				 );
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
		step_ = HANDLE_ERROR_;

		break;
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
		step_ = HANDLE_ERROR_;

		break;
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

	    // Get mvs in schema
	    str_sprintf(queryBuf_, "get mvs in schema \"%s\".\"%s\" %s",
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

	    // Get synonyms in schema
	    str_sprintf(queryBuf_, "get synonyms in schema \"%s\".\"%s\" %s",
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

	}
    }

  return 0;
}

////////////////////////////////////////////////////////////////
// Constructor for class ExExeUtilGetMetadataInfoVersionTcb
///////////////////////////////////////////////////////////////
ExExeUtilGetMetadataInfoVersionTcb::ExExeUtilGetMetadataInfoVersionTcb(
     const ComTdbExeUtilGetMetadataInfo & exe_util_tdb,
     ex_globals * glob)
     : ExExeUtilGetMetadataInfoTcb( exe_util_tdb, glob)
{
}

static const QueryString getVersionForSchemasInCatalogQuery[] =
{
  {" select translate(trim(S.schema_name) using ucs2toutf8), "},
  {" cast(S.schema_version as char(4)) "},
  {"   from  "},
  {"     HP_SYSTEM_CATALOG.system_schema.catsys C, "},
  {"     HP_SYSTEM_CATALOG.system_schema.schemata S "},
  {"   where "},
  {"     C.cat_name = '%s' and "},
  {"     C.cat_uid = S.cat_uid "},
  {"     %s "},
  {"   order by 1 "},
  {" ; "}
};

static const QueryString getVersionForObjectsInSchemaQuery[] =
{
  {" select translate(trim(O.object_name) using ucs2toutf8), "},
  {" cast(S.schema_version as char(4)), cast(O.object_feature_version as char(4)), "},
  {" cast(O.rcb_version as char(4)) "},
  {"   from  "},
  {"     HP_SYSTEM_CATALOG.system_schema.catsys C, "},
  {"     HP_SYSTEM_CATALOG.system_schema.schemata S, "},
  {"     \"%s\".HP_DEFINITION_SCHEMA.objects O "},
  {"   where "},
  {"     C.cat_name = '%s' and "},
  {"     S.schema_name = '%s' and "},
  {"     C.cat_uid = S.cat_uid and "},
  {"     S.schema_uid = O.schema_uid "},
  {"     %s "},
  {"     order by 1 "},
  {" ; "}
};
//////////////////////////////////////////////////////
// work() for ExExeUtilGetMetadataInfoVersionTcb
//////////////////////////////////////////////////////
short ExExeUtilGetMetadataInfoVersionTcb::work()
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
	  }
	break;

	case DISABLE_CQS_:
	  {
	    if (disableCQS())
	      {
		step_ = HANDLE_ERROR_;
		break;
	      }

	    step_ = SETUP_QUERY_;
	  }
	break;

	case SETUP_QUERY_:
	  {
	    const QueryString * qs;
	    Int32 sizeOfqs = 0;

	    char predStr[2000];
	    str_sprintf(predStr, "");

	    str_sprintf(patternStr_, "");
	    if (getMItdb().getPattern())
	      {
		str_sprintf(patternStr_, ", match '%s' ",
			    getMItdb().getPattern());
	      }

	    if (getMItdb().queryType() == ComTdbExeUtilGetMetadataInfo::SCHEMAS_IN_CATALOG_)
	      {
		qs = getVersionForSchemasInCatalogQuery;
		sizeOfqs = sizeof(getVersionForSchemasInCatalogQuery);

		param_[0] = getMItdb().cat_;
		param_[1] = predStr;

		if (getMItdb().userObjs())
		  {
		    str_sprintf(predStr, " and (S.schema_name <> 'HP_DEFINITION_SCHEMA' and S.schema_name <> 'MXCS_SCHEMA' and S.schema_name <> 'SYSTEM_DEFAULTS_SCHEMA' and S.schema_name <> 'SYSTEM_SCHEMA' and S.schema_name <> 'PUBLIC_ACCESS_SCHEMA' and S.schema_name <> 'HP_ROUTINES' and S.schema_name <> 'HP_SECURITY_SCHEMA' and S.schema_name <> 'MANAGEABILITY' and left(S.schema_name, 1) <> '@' and S.current_operation <> 'VS' ) ");
		  }
		else if (getMItdb().systemObjs())
		  {
		    str_sprintf(predStr, " and (S.schema_name = 'HP_DEFINITION_SCHEMA' or S.schema_name = 'MXCS_SCHEMA' or S.schema_name = 'SYSTEM_DEFAULTS_SCHEMA' or S.schema_name = 'SYSTEM_SCHEMA' or S.schema_name = 'PUBLIC_ACCESS_SCHEMA' or S.schema_name = 'HP_ROUTINES' or S.schema_name = 'HP_SECURITY_SCHEMA' or S.schema_name = 'MANAGEABILITY' or left(S.schema_name, 1) = '@' or S.current_operation = 'VS') ");
		  }

		numOutputEntries_ = 2;
	      }
	    else
	      {
		qs = getVersionForObjectsInSchemaQuery;
		sizeOfqs = sizeof(getVersionForObjectsInSchemaQuery);

		param_[0] = getMItdb().cat_;
		param_[1] = getMItdb().cat_;
		param_[2] = getMItdb().sch_;
		param_[3] = predStr;

		switch (getMItdb().queryType_)
		  {
		  case ComTdbExeUtilGetMetadataInfo::TABLES_IN_SCHEMA_:
		    {
		      strcat(predStr, " and O.object_name_space = 'TA' and O.object_type = 'BT' ");

		      if (getMItdb().userObjs())
			{
			  strcat(predStr, " and O.object_security_class = 'UT' ");
			}
		      else if (getMItdb().systemObjs())
			{
			  strcat(predStr, " and O.object_security_class <> 'UT' ");
			}

		    }
		  break;

		  case ComTdbExeUtilGetMetadataInfo::INDEXES_IN_SCHEMA_:
		    {
		      strcat(predStr, " and O.object_name_space = 'IX' and O.object_type = 'IX' ");
		    }
		  break;

		  case ComTdbExeUtilGetMetadataInfo::VIEWS_IN_SCHEMA_:
		    {
		      strcat(predStr, " and O.object_name_space = 'TA' and O.object_type = 'VI' ");
		    }
		  break;

		  case ComTdbExeUtilGetMetadataInfo::LIBRARIES_IN_SCHEMA_:
		    {
		      strcat(predStr, " and O.object_name_space = 'LB' and O.object_type = 'LB' "); //ACH VErfiy this is stored correctly - SMDIO?
		    }
		  break;

		  case ComTdbExeUtilGetMetadataInfo::MVS_IN_SCHEMA_:
		    {
		      strcat(predStr, " and O.object_name_space = 'TA' and O.object_type = 'MV' ");
		    }
		  break;

		  case ComTdbExeUtilGetMetadataInfo::PROCEDURES_IN_SCHEMA_:
		    {
		      strcat(predStr, " and O.object_name_space = 'TA' and O.object_type = 'UR' ");
		    }
		  break;

		  case ComTdbExeUtilGetMetadataInfo::SYNONYMS_IN_SCHEMA_:
		    {
		      strcat(predStr, " and O.object_name_space = 'TA' and O.object_type = 'SY' ");
		    }
		  break;

		  case ComTdbExeUtilGetMetadataInfo::OBJECTS_IN_SCHEMA_:
		    {
		    }
		  break;

		  default:
		    {
		      ExHandleErrors(qparent_,
				     pentry_down,
				     0,
				     getGlobals(),
				     NULL,
				     (ExeErrorCode)-4218,
				     NULL,
				     "GET"
				     );
		      step_ = HANDLE_ERROR_;
		    }
		  break;

		  } // switch

		numOutputEntries_ = 4;
	      }

	    Int32 qryArraySize = sizeOfqs / sizeof(QueryString);
	    char * gluedQuery;
	    Lng32 gluedQuerySize;
	    glueQueryFragments(qryArraySize,  qs,
			       gluedQuery, gluedQuerySize);

	    str_sprintf(queryBuf_, gluedQuery,
			param_[0], param_[1], param_[2], param_[3],
			param_[4], param_[5], param_[6], param_[7],
			param_[8], param_[9], param_[10], param_[11],
			param_[12], param_[13], param_[14]);

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
		step_ = HANDLE_ERROR_;

		break;
	      }

	    infoList_->position();

	    // find out the max length of the object name entry.
	    // This will help in formatting of output.
	    infoList_->position();
	    maxObjLen_ = 0;
	    while (NOT infoList_->atEnd())
	      {
		OutputInfo * oi = (OutputInfo*)infoList_->getCurr();
		if (strlen(oi->get(0)) > maxObjLen_)
		  maxObjLen_ = strlen(oi->get(0));

		infoList_->advance();
	      }

	    infoList_->position();

	    step_ = DISPLAY_HEADING_;
	  }
	break;

	case DISPLAY_HEADING_:
	  {
	    if (infoList_->atEnd())
	      {
		step_ = DONE_;
		break;
	      }

	    // make sure there is enough space to move header
	    if (isUpQueueFull(5))
	      {
		return WORK_OK; // come back later
	      }

	    if (numOutputEntries_ == 2)
	      {
		maxObjLen_ = MAXOF(strlen("Schema"), maxObjLen_);
		str_sprintf(formatStr_, "%%%ds  %%4s", maxObjLen_);

		str_sprintf(headingBuf_, formatStr_,
			    "Schema", "OSV");
	      }
	    else
	      {
		maxObjLen_ = MAXOF(strlen("Object"), maxObjLen_);
		str_sprintf(formatStr_, "%%%ds  %%4s  %%4s  %%4s", maxObjLen_);

		str_sprintf(headingBuf_, formatStr_,
			    "Object", "OSV", "OFV", "RCBV");
	      }

	    Lng32 len = strlen(headingBuf_);
	    moveRowToUpQueue(headingBuf_);
	    str_pad(headingBuf_, len, '=');
	    headingBuf_[len] = 0;
	    moveRowToUpQueue(headingBuf_);

	    moveRowToUpQueue(" ");

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

	    if (numOutputEntries_ == 2)
	      {
		str_sprintf(outputBuf_, formatStr_,
			    vi->get(0), vi->get(1));
	      }
	    else
	      {
		str_sprintf(outputBuf_, formatStr_,
			    vi->get(0), vi->get(1), vi->get(2),
			    vi->get(3));
	      }

	    short rc = 0;
	    moveRowToUpQueue(outputBuf_, 0, &rc);

	    infoList_->advance();
	  }
	break;

	case ENABLE_CQS_:
	  {
	    if (restoreCQS())
	      {
		step_ = HANDLE_ERROR_;
		break;
	      }

	    step_ = DONE_;
	  }
	break;

	case HANDLE_ERROR_:
	  {
	    restoreCQS();

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

	}
    }

  return 0;
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
  str_sprintf(buf, "select rtrim(table_name) from table(hivemd(tables, %s.%s))", getMItdb().getCat(), getMItdb().getSch());
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
	    
	    short rc = 0;
	    retcode = fetchAllHiveRows(infoList_, 1, rc);
	    if (retcode < 0)
	      {
                cliInterface()->retrieveSQLDiagnostics(getDiagsArea());
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
	    ptr += SQL_VARCHAR_HDR_SIZE;
 
	    if (NOT headingReturned_)
	      {
		step_ = DISPLAY_HEADING_;
		break;
	      }

	    short rc = 0;
	    if (moveRowToUpQueue(ptr, len, &rc))
            {
	      return rc;
            }

	    infoList_->advance();
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

//LCOV_EXCL_START

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

	case ERROR_:
	  {
	    if (qparent_.up->isFull())
	      return WORK_OK;

	    ExHandleErrors(qparent_,
			   pentry_down,
			   0,
			   getGlobals(),
			   NULL,
			   (ExeErrorCode)cliRC,
			   NULL,
			   NULL
			   );
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

	case ERROR_:
	  {
	    if (qparent_.up->isFull())
	      return WORK_OK;

	    ExHandleErrors(qparent_,
			   pentry_down,
			   0,
			   getGlobals(),
			   NULL,
			   (ExeErrorCode)cliRC,
			   NULL,
			   NULL
			   );
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
ex_tcb * ExExeUtilHiveMDaccessTdb::build(ex_globals * glob)
{
  ExExeUtilHiveMDaccessTcb * exe_util_tcb;

  exe_util_tcb =
    new(glob->getSpace()) ExExeUtilHiveMDaccessTcb(*this, glob);
  
  exe_util_tcb->registerSubtasks();

  return (exe_util_tcb);
}

////////////////////////////////////////////////////////////////
// Constructor for class ExExeUtilGetMetadataInfoTcb
///////////////////////////////////////////////////////////////
ExExeUtilHiveMDaccessTcb::ExExeUtilHiveMDaccessTcb(
     const ComTdbExeUtilHiveMDaccess & exe_util_tdb,
     ex_globals * glob)
  : ExExeUtilTcb( exe_util_tdb, NULL, glob),
    hiveMD_(NULL),
    currColDesc_(NULL),
    currKeyDesc_(NULL),
    tblNames_(getHeap()),
    pos_(0)
{
  //  queryBuf_ = new(glob->getDefaultHeap()) char[4096];
  step_ = INITIAL_;

  mdRow_ = new(getHeap()) char[exe_util_tdb.outputRowlen_];
}

ExExeUtilHiveMDaccessTcb::~ExExeUtilHiveMDaccessTcb()
{

}

// should move this method to common dir.
Lng32 ExExeUtilHiveMDaccessTcb::getFSTypeFromHiveColType(const char* hiveType)
{
  if ( !strcmp(hiveType, "tinyint")) 
    return REC_BIN16_SIGNED;

  if ( !strcmp(hiveType, "smallint")) 
    return REC_BIN16_SIGNED;
 
  if ( !strcmp(hiveType, "int")) 
    return REC_BIN32_SIGNED;

  if ( !strcmp(hiveType, "bigint"))
    return REC_BIN64_SIGNED;

  if ( !strcmp(hiveType, "string"))
    return REC_BYTE_V_ASCII;

  if ( !strcmp(hiveType, "float"))
    return REC_FLOAT32;

  if ( !strcmp(hiveType, "double"))
    return REC_FLOAT64;

  if ( !strcmp(hiveType, "timestamp"))
    return REC_DATETIME;

  return -1;
}

Lng32 ExExeUtilHiveMDaccessTcb::getLengthFromHiveColType(const char* hiveType)
{
  if ( !strcmp(hiveType, "tinyint")) 
    return 2;

  if ( !strcmp(hiveType, "smallint")) 
    return 2;
 
  if ( !strcmp(hiveType, "int")) 
    return 4;

  if ( !strcmp(hiveType, "bigint"))
    return 8;

  if ( !strcmp(hiveType, "string"))
    return 32000; // TBD: add cqd.

  if ( !strcmp(hiveType, "float"))
    return 4;

  if ( !strcmp(hiveType, "double"))
    return 8;

  if ( !strcmp(hiveType, "timestamp"))
    return 26; //Is this internal or display length? REC_DATETIME;

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
  
  ComDiagsArea * diags = getDiagsArea();

  while (1)
    {
      switch (step_)
	{
	case INITIAL_:
	  {
	    if (hiveMD_)
	      NADELETEBASIC(hiveMD_, getHeap());
	    
            char val[5];
            cliInterface()->getCQDval("HIVE_CATALOG", hiveCat_);
            cliInterface()->getCQDval("HIVE_DEFAULT_SCHEMA", hiveSch_);

            char userTblSch[256];
     
            cliInterface()->getCQDval("HIVE_DEFAULT_SCHEMA", userTblSch);
            NAString hiveDefaultSch(userTblSch);
            hiveDefaultSch.toLower();
            // the current schema name has been lower cased in the tdb

            hiveMD_ = new (getHeap()) HiveMetaData();

            char* currSch = hiveMDtdb().getSchema();
// change schema name to "default", since the default schema name Hive uses, 
//if necessary. In our stack the default hive schema name is usually "HIVE"
            if (!strcmp(hiveDefaultSch.data(), currSch))
              currSch = (char *) hiveMD_->getDefaultSchemaName(); 

            NABoolean readEntireSchema = FALSE;
            if (hiveMDtdb().mdType_ != ComTdbExeUtilHiveMDaccess::TABLES_) {
              readEntireSchema = TRUE;
            }

            retStatus = hiveMD_->init(readEntireSchema,
                                      currSch,
                                      hiveMDtdb().hivePredStr());
            if (!retStatus)
              {
                *diags << DgSqlCode(-1190)
                       << DgString0(hiveMD_->getErrMethodName())
                       << DgString1(hiveMD_->getErrCodeStr())
                       << DgString2(hiveMD_->getErrDetail())
                       << DgInt0(hiveMD_->getErrCode());
                step_ = HANDLE_ERROR_;
                break;
              }
            if (!readEntireSchema) 
            {
              HVC_RetCode retCode = hiveMD_->getClient()->
                getAllTables(currSch, tblNames_);
              if ((retCode != HVC_OK) && (retCode != HVC_DONE)) 
                {
                  *diags << DgSqlCode(-1190)
                         << DgString0((char*)
                                      "HiveClient_JNI::getAllTables()")
                         << DgString1(hiveMD_->getClient()->
                                      getErrorText(retCode))
                         << DgInt0(retCode)
                         << DgString2(GetCliGlobals()->getJniErrorStr());
                  step_ = HANDLE_ERROR_;
                  break;
                }
            }
	    step_ = POSITION_;
	  }
	  break;

	case POSITION_:
	  {
            hive_tbl_desc * htd = NULL;
            if (hiveMDtdb().mdType_ != ComTdbExeUtilHiveMDaccess::TABLES_) {
              hiveMD_->position();
              htd = hiveMD_->getNext();
            }
            else
              pos_ = 0; // we are not reading the entire schema.

	    if (hiveMDtdb().mdType_ == ComTdbExeUtilHiveMDaccess::TABLES_)
	      {
		step_ = FETCH_TABLE_;
	      }
	    else if (hiveMDtdb().mdType_ == ComTdbExeUtilHiveMDaccess::COLUMNS_)
	      {
		if (htd)
		  currColDesc_ = htd->getColumns();
		else
		  currColDesc_ = NULL;

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

	case FETCH_TABLE_:
	  {
	    if (qparent_.up->isFull())
	      return WORK_OK;

            if (hiveMDtdb().mdType_ != ComTdbExeUtilHiveMDaccess::TABLES_) {
              if (hiveMD_->atEnd())
              {
                step_ = DONE_;
                break;
              }
            }
            else {
              if (pos_ >= tblNames_.entries())
	      {
		step_ = DONE_;
		break;
	      }
            }

            HiveMDTablesColInfoStruct *s =(HiveMDTablesColInfoStruct*)mdRow_;

	    str_cpy(s->catName, hiveCat_, 256, ' ');
	    str_cpy(s->schName, hiveSch_, 256, ' ');

            if (hiveMDtdb().mdType_ != ComTdbExeUtilHiveMDaccess::TABLES_) {
              struct hive_tbl_desc * htd = hiveMD_->getNext();
              str_cpy(s->tblName, htd->tblName_, 256, ' ');
            }
            else {
              str_cpy(s->tblName, tblNames_[pos_]->c_str(), 256, ' ');
	      //              delete tblNames_[pos_]; 
              //delete the allocation by StringArrayList::get(i)
            }
            
	    step_ = APPLY_PRED_;
	  }
	  break;
	  
	case FETCH_COLUMN_: //does not work with JNI
	  {
	    if (qparent_.up->isFull())
	      return WORK_OK;

	    if (! currColDesc_) 
	      {
		step_ = DONE_;
		break;
	      }
	    
	    struct hive_tbl_desc * htd = hiveMD_->getNext();
	    struct hive_column_desc * hcd = currColDesc_;
	    
	    HiveMDColumnsColInfoStruct *infoCol =
	      (HiveMDColumnsColInfoStruct*)mdRow_;
	    
	    str_cpy(infoCol->catName, hiveCat_, 256, ' ');
	    str_cpy(infoCol->schName, hiveSch_, 256, ' ');
	    str_cpy(infoCol->tblName, htd->tblName_, 256, ' ');
	    str_cpy(infoCol->colName, hcd->name_, 256, ' ');

	    infoCol->fsDatatype = getFSTypeFromHiveColType(hcd->type_);
	    if (infoCol->fsDatatype < 0)
	      {
		char strP[300];
		sprintf(strP, "Datatype %s is not supported.", hcd->type_);
		*diags << DgSqlCode(-CLI_GET_METADATA_INFO_ERROR)
		       << DgString0(strP);
		
		step_ = HANDLE_ERROR_;
		break;
	      }
	    
	    const char * sdtStr = Descriptor::ansiTypeStrFromFSType(infoCol->fsDatatype);
	    str_cpy(infoCol->sqlDatatype, sdtStr, 24, ' ');
	    infoCol->colSize = getLengthFromHiveColType(hcd->type_);
	    infoCol->colScale = 0;

	    // only iso charset
	    if ((infoCol->fsDatatype == REC_BYTE_F_ASCII) ||
		(infoCol->fsDatatype == REC_BYTE_V_ASCII))
	      str_cpy(infoCol->charSet, "ISO88591", 40, ' ');
	    else
	      str_pad(infoCol->charSet, 40, ' ');

	    infoCol->colPrecision = 0;
	    infoCol->nullable = 1;
	    infoCol->colNum = hcd->intIndex_;
	    infoCol->dtCode = 0;
	    infoCol->dtStartField = 0;
	    infoCol->dtEndField = 0;
	    str_pad(infoCol->dtQualifier, 28, ' ');

	    if (infoCol->fsDatatype == REC_DATETIME)
	      {
		// hive currently only supports timestamp
		infoCol->dtCode = SQLDTCODE_TIMESTAMP;
		infoCol->colScale = 6;
		str_cpy(infoCol->dtQualifier, "(6)", 28, ' ');
		infoCol->dtStartField = 1;
		infoCol->dtEndField = 6;
	      }

	    // no default value
	    str_cpy(infoCol->defVal, " ", 240, ' ');

	    step_ = APPLY_PRED_;
	  }
	  break;

	case FETCH_PKEY_:  // does not work with JNI
	  {
	    if (qparent_.up->isFull())
	      return WORK_OK;

	    if (! currKeyDesc_)
	      {
		step_ = DONE_;
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
	    if (hiveMDtdb().mdType_ == ComTdbExeUtilHiveMDaccess::TABLES_)
	      {
                pos_++;
		step_ = FETCH_TABLE_;
	      }
            // next two else blocks do not work with JNI
	    else if (hiveMDtdb().mdType_ == ComTdbExeUtilHiveMDaccess::COLUMNS_)
	      {
		if (currColDesc_)
		  currColDesc_ = currColDesc_->next_;
		
		if (! currColDesc_)
		  {
		    // move to the next table
		    hiveMD_->advance();
		    
		    if (! hiveMD_->atEnd())
		      currColDesc_ = hiveMD_->getNext()->getColumns();
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

//LCOV_EXCL_STOP
