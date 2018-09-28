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

#ifndef _CMP_SEABASE_PROCEDURES_H_
#define _CMP_SEABASE_PROCEDURES_H_

#include "CmpSeabaseDDL.h"
#include "CmpSeabaseDDLupgrade.h"

// To add a new procedure:
//   update export/lib/lib_mgmt.jar to include code for the new procedure 
//   add a define representing the procedures below
//   add a static const QString representing the create procedure text
//   add a new entry in allLibmgrRoutineInfo 
//   perform initialize trafodion, upgrade library management
// recommend that new procedures are added in alphabetic order

// If adding a new library, be sure to update upgradeSeabaseLibmgr to
// update jar/dll locations to the new version.

// At this time there is no support to drop or change the signature of an
// existing procedure.  Since customers may be using the procedures, it is
// recommended that they not be dropped or changed - instead add new ones
// to handle the required change.
  
// List of supported system procedures - in alphabetic order
#define SYSTEM_PROC_ADDLIB       "ADDLIB"
#define SYSTEM_PROC_ALTERLIB     "ALTERLIB"
#define SYSTEM_PROC_DROPLIB      "DROPLIB"
#define SYSTEM_PROC_GETFILE      "GETFILE"
#define SYSTEM_PROC_HELP         "HELP"
#define SYSTEM_PROC_LS           "LS"
#define SYSTEM_PROC_LSALL        "LSALL"
#define SYSTEM_PROC_PUT          "PUT"
#define SYSTEM_PROC_PUTFILE      "PUTFILE"
#define SYSTEM_PROC_RM           "RM"
#define SYSTEM_PROC_RMREX        "RMREX"
#define SYSTEM_TMUDF_SYNCLIBUDF  "SYNCLIBUDF"
#define SYSTEM_TMUDF_EVENT_LOG_READER "EVENT_LOG_READER"
#define SYSTEM_TMUDF_JDBC        "JDBC"

// Create procedure text for system procedures
static const QString seabaseProcAddlibDDL[] =
{
  {"  CREATE PROCEDURE IF NOT EXISTS %s.\"%s\"." SYSTEM_PROC_ADDLIB" "},
  {" ( "},
  {"  IN LIBNAME VARCHAR(1024) CHARACTER SET UTF8, "},
  {"  IN FILENAME VARCHAR(1024) CHARACTER SET UTF8, "},
  {"  IN HOSTNAME VARCHAR(1024) CHARACTER SET UTF8, "},
  {"  IN LOCALFILE VARCHAR(1024) CHARACTER SET UTF8) "},
  {"  EXTERNAL NAME 'org.trafodion.libmgmt.FileMgmt.addLib (java.lang.String,java.lang.String,java.lang.String,java.lang.String)' "},
  {"  EXTERNAL SECURITY DEFINER "},
  {"  LIBRARY %s.\"%s\".%s "},
  {"  LANGUAGE JAVA "},
  {"  PARAMETER STYLE JAVA "},
  {"  CONTAINS SQL "},
  {"  NO TRANSACTION REQUIRED "},
  {" ; "}
};

static const QString seabaseProcAlterlibDDL[] = 
{
  {"  CREATE PROCEDURE IF NOT EXISTS %s.\"%s\".ALTERLIB "},
  {" ( "},
  {"  IN LIBNAME VARCHAR(1024) CHARACTER SET UTF8,"},
  {"  IN FILENAME VARCHAR(1024) CHARACTER SET UTF8,"},
  {"  IN HOSTNAME VARCHAR(1024) CHARACTER SET UTF8,"},
  {"  IN LOCALFILE VARCHAR(1024) CHARACTER SET UTF8) "},
  {"  EXTERNAL NAME 'org.trafodion.libmgmt.FileMgmt.alterLib (java.lang.String,java.lang.String,java.lang.String,java.lang.String)' "},
  {"  EXTERNAL SECURITY DEFINER "},
  {"  LIBRARY %s.\"%s\".%s "},
  {"  LANGUAGE JAVA "},
  {"  PARAMETER STYLE JAVA "},
  {"  CONTAINS SQL "},
  {"  NO TRANSACTION REQUIRED "},
  {" ; "}
};

static const QString seabaseProcDroplibDDL[] = 
{
  {"  CREATE PROCEDURE IF NOT EXISTS %s.\"%s\".DROPLIB "},
  {" ( "},
  {"  IN LIBNAME VARCHAR(1024) CHARACTER SET UTF8, "},
  {"  IN MODETYPE VARCHAR(1024) CHARACTER SET ISO88591) "},
  {"  EXTERNAL NAME 'org.trafodion.libmgmt.FileMgmt.dropLib (java.lang.String,java.lang.String)' "},
  {"  EXTERNAL SECURITY DEFINER "},
  {"  LIBRARY %s.\"%s\".%s "},
  {"  LANGUAGE JAVA "},
  {"  PARAMETER STYLE JAVA "},
  {"  CONTAINS SQL "},
  {"  NO TRANSACTION REQUIRED "},
  {" ; "}
};

static const QString seabaseProcGetfileDDL[] = 
{
  {"  CREATE PROCEDURE IF NOT EXISTS %s.\"%s\".GETFILE  "},
  {" ( "},
  {"  IN FILENAME VARCHAR(256) CHARACTER SET UTF8,"},
  {"  IN OFFSET INTEGER,"},
  {"  OUT FILEDATA VARCHAR(12800) CHARACTER SET UTF8,"},
  {"  OUT DATALENGTH LARGEINT)"},
  {"  EXTERNAL NAME 'org.trafodion.libmgmt.FileMgmt.get (java.lang.String,int,java.lang.String[],long[])' "},
  {"  LIBRARY %s.\"%s\".%s "},
  {"  EXTERNAL SECURITY DEFINER "},
  {"  LANGUAGE JAVA "},
  {"  PARAMETER STYLE JAVA "},
  {"  READS SQL DATA "},
  {"  NO TRANSACTION REQUIRED "},
  {" ; "}
};

static const QString seabaseProcHelpDDL[] =
{
  {" CREATE PROCEDURE IF NOT EXISTS %s.\"%s\".HELP "},
  {" ( "},
  {" INOUT COMMANDNAME VARCHAR(2560) CHARACTER SET ISO88591) "},
  {"  EXTERNAL NAME 'org.trafodion.libmgmt.FileMgmt.help (java.lang.String[])' "},
  {"  EXTERNAL SECURITY DEFINER "},
  {"  LIBRARY %s.\"%s\".%s "},
  {"  LANGUAGE JAVA "},
  {"  PARAMETER STYLE JAVA "},
  {"  READS SQL DATA "},
  {" ; "}
};

static const QString seabaseProcLsDDL[] = 
{
  {" CREATE PROCEDURE IF NOT EXISTS %s.\"%s\".LS  "},
  {" ( "},
  {"  IN FILENAME VARCHAR(256) CHARACTER SET UTF8, "},
  {"  OUT FILENAMES VARCHAR(10240) CHARACTER SET ISO88591) "},
  {"  EXTERNAL NAME 'org.trafodion.libmgmt.FileMgmt.ls(java.lang.String,java.lang.String[])' "},
  {"  EXTERNAL SECURITY DEFINER "},
  {"  LIBRARY %s.\"%s\".%s "},
  {"  LANGUAGE JAVA "},
  {"  PARAMETER STYLE JAVA "},
  {"  READS SQL DATA "},
  {"  NO TRANSACTION REQUIRED "},
  {" ; "}
};

static const QString seabaseProcLsallDDL[] = 
{
  {"  CREATE PROCEDURE IF NOT EXISTS %s.\"%s\".LSALL "},
  {" ( "},
  {"  OUT FILENAMES VARCHAR(10240) CHARACTER SET ISO88591) "},
  {"  EXTERNAL NAME 'org.trafodion.libmgmt.FileMgmt.lsAll(java.lang.String[])' "},
  {"  EXTERNAL SECURITY DEFINER "},
  {"  LIBRARY %s.\"%s\".%s "}, 
  {"  LANGUAGE JAVA "},
  {"  PARAMETER STYLE JAVA "},
  {"  READS SQL DATA "},
  {"  NO TRANSACTION REQUIRED "},
  {" ; "}
};

static const QString seabaseProcPutDDL[] =
{
  {" CREATE PROCEDURE IF NOT EXISTS %s.\"%s\".PUT "},
  {" ( "},
  {"  IN FILEDATA VARCHAR(102400) CHARACTER SET ISO88591, "},
  {"  IN FILENAME VARCHAR(256) CHARACTER SET UTF8, "},
  {"  IN CREATEFLAG INTEGER, "},
  {"  IN FILEOVERWRITE INTEGER) "},
  {"  EXTERNAL NAME 'org.trafodion.libmgmt.FileMgmt.put(java.lang.String,java.lang.String,int,int)' "},
  {"  EXTERNAL SECURITY DEFINER "},
  {"  LIBRARY %s.\"%s\".%s "},
  {"  LANGUAGE JAVA "},
  {"  PARAMETER STYLE JAVA "},
  {"  READS SQL DATA "},
  {"  NO TRANSACTION REQUIRED "},
  {" ; "}
};

static const QString seabaseProcPutFileDDL[] =
{
  {" CREATE PROCEDURE IF NOT EXISTS %s.\"%s\".PUTFILE "},
  {" ( "},
  {"  IN FILEDATA VARCHAR(102400) CHARACTER SET ISO88591, "},
  {"  IN FILENAME VARCHAR(256) CHARACTER SET UTF8, "},
  {"  IN ISFIRSTCHUNK INTEGER, "},
  {"  IN ISLASTCHUNK INTEGER, "},
  {"  IN OVERWRITEEXISTINGFILE INTEGER) "},
  {"  EXTERNAL NAME 'org.trafodion.libmgmt.FileMgmt.putFile(java.lang.String,java.lang.String,int,int,int)' "},
  {"  EXTERNAL SECURITY DEFINER "},
  {"  LIBRARY %s.\"%s\".%s "},
  {"  LANGUAGE JAVA "},
  {"  PARAMETER STYLE JAVA "},
  {"  READS SQL DATA "},
  {"  NO TRANSACTION REQUIRED "},
  {" ; "}
};

static const QString seabaseProcRmDDL[] = 
{
  {"  CREATE PROCEDURE IF NOT EXISTS %s.\"%s\".RM "},
  {" ( "},
  {"  IN FILENAME VARCHAR(256) CHARACTER SET UTF8) "},
  {"  EXTERNAL NAME 'org.trafodion.libmgmt.FileMgmt.rm(java.lang.String)' "},
  {"  EXTERNAL SECURITY DEFINER "},
  {"  LIBRARY %s.\"%s\".%s "},
  {"  LANGUAGE JAVA "},
  {"  PARAMETER STYLE JAVA "},
  {"  READS SQL DATA "},
  {"  NO TRANSACTION REQUIRED "},
  {" ; "}
};

static const QString seabaseProcRmrexDDL[] = 
{
  {"  CREATE PROCEDURE IF NOT EXISTS %s.\"%s\".RMREX  "},
  {" ( "},
  {"  IN FILENAME VARCHAR(256) CHARACTER SET UTF8, "},
  {"  OUT FILENAMES VARCHAR(10240) CHARACTER SET ISO88591) "},
  {"  EXTERNAL NAME 'org.trafodion.libmgmt.FileMgmt.rmRex(java.lang.String, java.lang.String[])' "},
  {"  EXTERNAL SECURITY DEFINER "},
  {"  LIBRARY %s.\"%s\".%s "},
  {"  LANGUAGE JAVA "},
  {"  PARAMETER STYLE JAVA "},
  {"  READS SQL DATA "},
  {"  NO TRANSACTION REQUIRED "},
  {" ; "}
};

  static const QString seabaseTMUDFSyncLibDDL[] = 
{
  {"  CREATE TABLE_MAPPING FUNCTION IF NOT EXISTS %s.\"%s\".SYNCLIBUDF() "},
  {"  EXTERNAL NAME 'org.trafodion.libmgmt.SyncLibUDF' "},
  {"  LIBRARY %s.\"%s\".%s "},
  {"  LANGUAGE JAVA "},
  {" ; "}
};

  static const QString seabaseTMUDFEventLogReaderDDL[] = 
{
  {"  CREATE TABLE_MAPPING FUNCTION IF NOT EXISTS %s.\"%s\".EVENT_LOG_READER() "},
  {"  EXTERNAL NAME 'TRAF_CPP_EVENT_LOG_READER' "},
  {"  LIBRARY %s.\"%s\".%s "},
  {"  LANGUAGE CPP "},
  {" ; "}
};

  static const QString seabaseTMUDFJDBCDDL[] = 
{
  {"  CREATE TABLE_MAPPING FUNCTION IF NOT EXISTS %s.\"%s\".JDBC() "},
  {"  EXTERNAL NAME 'org.trafodion.libmgmt.JDBCUDR' "},
  {"  LIBRARY %s.\"%s\".%s "},
  {"  LANGUAGE JAVA "},
  {" ; "}
};

struct LibmgrRoutineInfo
{
  // type of the UDR (used in grant)
  const char * udrType;

  // name of the UDR
  const char * newName;

  // ddl stmt corresponding to the current ddl.
  const QString *newDDL;
  Lng32 sizeOfnewDDL;

  enum LibmgrLibEnum
    {
      JAVA_LIB,
      CPP_LIB
    } whichLib;

  enum LibmgrRoleEnum
    {
      LIBMGR_ROLE,
      PUBLIC
    } whichRole;
};

static const LibmgrRoutineInfo allLibmgrRoutineInfo[] = {
  {"PROCEDURE",
   SYSTEM_PROC_ADDLIB, 
   seabaseProcAddlibDDL, 
   sizeof(seabaseProcAddlibDDL),
   LibmgrRoutineInfo::JAVA_LIB,
   LibmgrRoutineInfo::LIBMGR_ROLE
  },

  {"PROCEDURE",
   SYSTEM_PROC_ALTERLIB, 
   seabaseProcAlterlibDDL, 
   sizeof(seabaseProcAlterlibDDL),
   LibmgrRoutineInfo::JAVA_LIB,
   LibmgrRoutineInfo::LIBMGR_ROLE
  },

  {"PROCEDURE",
   SYSTEM_PROC_DROPLIB, 
   seabaseProcDroplibDDL, 
   sizeof(seabaseProcDroplibDDL),
   LibmgrRoutineInfo::JAVA_LIB,
   LibmgrRoutineInfo::LIBMGR_ROLE
  },

  {"PROCEDURE",
   SYSTEM_PROC_GETFILE, 
   seabaseProcGetfileDDL, 
   sizeof(seabaseProcGetfileDDL),
   LibmgrRoutineInfo::JAVA_LIB,
   LibmgrRoutineInfo::LIBMGR_ROLE
  },

  {"PROCEDURE",
   SYSTEM_PROC_HELP, 
   seabaseProcHelpDDL, 
   sizeof(seabaseProcHelpDDL),
   LibmgrRoutineInfo::JAVA_LIB,
   LibmgrRoutineInfo::LIBMGR_ROLE
  },

  {"PROCEDURE",
   SYSTEM_PROC_LS, 
   seabaseProcLsDDL, 
   sizeof(seabaseProcLsDDL),
   LibmgrRoutineInfo::JAVA_LIB,
   LibmgrRoutineInfo::LIBMGR_ROLE
  },

  {"PROCEDURE",
   SYSTEM_PROC_LSALL, 
   seabaseProcLsallDDL, 
   sizeof(seabaseProcLsallDDL),
   LibmgrRoutineInfo::JAVA_LIB,
   LibmgrRoutineInfo::LIBMGR_ROLE
  },

  {"PROCEDURE",
   SYSTEM_PROC_PUT, 
   seabaseProcPutDDL, 
   sizeof(seabaseProcPutDDL),
   LibmgrRoutineInfo::JAVA_LIB,
   LibmgrRoutineInfo::LIBMGR_ROLE
  },

  {"PROCEDURE",
   SYSTEM_PROC_PUTFILE, 
   seabaseProcPutFileDDL, 
   sizeof(seabaseProcPutFileDDL),
   LibmgrRoutineInfo::JAVA_LIB,
   LibmgrRoutineInfo::LIBMGR_ROLE
  },

  {"PROCEDURE",
   SYSTEM_PROC_RM, 
   seabaseProcRmDDL, 
   sizeof(seabaseProcRmDDL),
   LibmgrRoutineInfo::JAVA_LIB,
   LibmgrRoutineInfo::LIBMGR_ROLE
  },

  {"PROCEDURE",
   SYSTEM_PROC_RMREX, 
   seabaseProcRmrexDDL, 
   sizeof(seabaseProcRmrexDDL),
   LibmgrRoutineInfo::JAVA_LIB,
   LibmgrRoutineInfo::LIBMGR_ROLE
  },

  {"FUNCTION",
   SYSTEM_TMUDF_SYNCLIBUDF,
   seabaseTMUDFSyncLibDDL,
   sizeof(seabaseTMUDFSyncLibDDL),
   LibmgrRoutineInfo::JAVA_LIB,
   LibmgrRoutineInfo::LIBMGR_ROLE
  },

  {"FUNCTION",
   SYSTEM_TMUDF_EVENT_LOG_READER,
   seabaseTMUDFEventLogReaderDDL,
   sizeof(seabaseTMUDFEventLogReaderDDL),
   LibmgrRoutineInfo::CPP_LIB,
   LibmgrRoutineInfo::PUBLIC
  },

  {"FUNCTION",
   SYSTEM_TMUDF_JDBC,
   seabaseTMUDFJDBCDDL,
   sizeof(seabaseTMUDFJDBCDDL),
   LibmgrRoutineInfo::JAVA_LIB,
   LibmgrRoutineInfo::PUBLIC
  }

};

/////////////////////////////////////////////////////////////////////
//
// Information about changed old metadata tables from which upgrade
// is being done to the current version.
// These definitions have changed in the current version of code.
// 
// Old definitions have the form (for ex for METRIC_QUERY_TABLE table):
//            createOldTrafv??MetricQueryTable[]
// v?? is the old version.
//
// When definitions change, make new entries between
// START_OLD_MD_v?? and END_OLD_MD_v??.
// Do not remove older entries. We want to keep them around for
// historical purpose.
//
// Change entries in allReposUpgradeInfo[] struct in this file
// to reflect the 'old' repository tables.
//
//////////////////////////////////////////////////////////////////////
//----------------------------------------------------------------
//-- LIBRARIES
//----------------------------------------------------------------
static const QString createLibrariesTable[] =
  {
    {" create table %s.\"%s\"." SEABASE_LIBRARIES" "},
    {" ( "},
    {"  library_uid largeint not null not serialized, "},
    {"  library_filename varchar(512) character set iso88591 not null not serialized, "},
    {"  library_storage  blob,"},
    {"  version int not null not serialized, "},
    {"  flags largeint not null not serialized "},
    {" ) "},
    {" primary key (library_uid) "},
    {" attribute hbase format "},
    {" ; "}
  };

#define SEABASE_LIBRARIES_OLD SEABASE_LIBRARIES"_OLD"
static const QString createOldTrafv210LibrariesTable[] =
{
{" create table %s.\"%s\"." SEABASE_LIBRARIES" "},
    {" ( "},
    {"  library_uid largeint not null not serialized, "},
    {"  library_filename varchar(512) character set iso88591 not null not serialized, "},
    {"  version int not null not serialized, "},
    {"  flags largeint not null not serialized "},
    {" ) "},
    {" primary key (library_uid) "},
    {" attribute hbase format "},
    {" ; "}
  };

static const MDUpgradeInfo allLibrariesUpgradeInfo[] = {
  // LIBRARIES
  {
    SEABASE_LIBRARIES,  SEABASE_LIBRARIES_OLD,
    createLibrariesTable,  sizeof(createLibrariesTable),
    createOldTrafv210LibrariesTable,  sizeof(createOldTrafv210LibrariesTable),
    NULL, 0,
    TRUE,
    //new table columns
    "library_uid,"
    "library_filename,"
    "library_storage,"
    "version,"
    "flags",
    //old table columns
    "library_uid,"
    "library_filename,"
    "version,"
    "flags",
    NULL, TRUE, FALSE, FALSE, FALSE, FALSE, FALSE}
};


  
#endif
