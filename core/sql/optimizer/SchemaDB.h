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
#ifndef SCHEMADB_H
#define SCHEMADB_H
/* -*-C++-*-
*************************************************************************
*
* File:         SchemaDB.h
* Description:  Schema Information
* Created:      4/27/94
* Language:     C++
*
*
*************************************************************************
*/


#include "NAAssert.h"	 // required after including a RogueWave file!

#include "BaseTypes.h"
#include "charinfo.h"
#include "ColumnDesc.h"
#include "ComTransInfo.h"
#include "CmpContext.h"
#include "DomainDesc.h"
#include "NADefaults.h"
#include "NARoutineDB.h"
#include "NATable.h"
#include "NARoutine.h"
#include "TableDesc.h"
#include "ValueDesc.h"

// triggers -- eliezer
class TriggerDB;
class NARoutineDB;

// ----------------------------------------------------------------------
// contents of this file
// ----------------------------------------------------------------------
class SchemaDB;
class POSRule;
class POSInfo;

// ***********************************************************************
// Information derived from the Information Schema tables (IST's) is
// stored in SchemaDB.
//
// The SchemaDB contains two "databases" that span the compilation 
// of multiple SQL statments. They are
//
// 1) The NATableDB - it contains the physical schema information
//                    for an SQL table or table-valued stored procedure
//                    corresponding to a given qualified (ANSI/NSK/UNIX) name.
// 2) The FilesetDB - it contains the description regarding each fileset 
//                    that corresponds to a NATable in the NATableDB.
//
// The SchemaDB also contains three "statement tables". Each statement
// table contains information that is relevant for the compilation of 
// a single SQL statement. The statement tables are reallocated on a
// per statement basis.
// 
// Important note : The SchemaDB is designed to last across statements.
// For the fields need to be initialized/reset at each statement, they
// need to be done in
//  void initPerStatement();
//  void cleanupPerStatement();
// See notes below about CmpContext...!
// ***********************************************************************
class SchemaDB : public NABasicObject
{
public:

  // --------------------------------------------------------------------
  // Constructor function
  // --------------------------------------------------------------------
  SchemaDB() ;

  // copy ctor
  SchemaDB(const SchemaDB & orig) ; // not written

  // --------------------------------------------------------------------
  // Destructor function
  // --------------------------------------------------------------------
  ~SchemaDB();
  
  // --------------------------------------------------------------------
  // Accessor functions
  // --------------------------------------------------------------------
  NATableDB       * getNATableDB()          { return &tableDB_; }
  ValueDescArray  & getValueDescArray()     { return valueDArray_; }
  NADefaults      & getDefaults()           { return defaults_; }
  NARoutineDB     * getNARoutineDB()        { return &routineDB_; }
  NARoutineDB     * getNARoutineActionDB()  { return &actionRoutineDB_; }

  //   Flags:  	  The ON/ENABLE bit		The OFF value
  enum Flags	{ REFRESH_CACHE		= 0x1,  NO_REFRESH_CACHE	= 0,
  		  APPLY_NAMETYPE_RULES	= 0x2,  FORCE_ANSI_NAMETYPE	= 0
		};

  const SchemaName& getDefaultSchema(UInt32 flags =
				     NO_REFRESH_CACHE | FORCE_ANSI_NAMETYPE);
  // triggers -- eliezer
  TriggerDB		  * getTriggerDB();
  RefConstraintList *getRIs(QualifiedName &subjectTable, 
		ComOperation operation);

  // --------------------------------------------------------------------
  // Mutator functions
  // --------------------------------------------------------------------

  // --------------------------------------------------------------------
  // The SchemaDB maintains a set of internal tables. They must be 
  // re-initialized explicitly per statement -- this used to be via a
  // call to createStmtTables, which is no longer useful, just a noop --
  // now **ONLY** CmpContext should manage the init/cleanupPerStatement
  // calls, which is the current mechanism!
  //
  // --------------------------------------------------------------------
  void createStmtTables();      // no longer used (formerly required)
  void dropStmtTables();        // no longer used
  NABoolean endTransaction();   // implicitly called (idempotent)

  // --------------------------------------------------------------------
  // Methods for addimg new entries to the "statement tables".
  // --------------------------------------------------------------------
  void insertValueDesc(ValueDesc *vdesc) { valueDArray_.insert(vdesc); }
  void insertDomainDesc(DomainDesc *ddesc) { domainDList_.insert(ddesc); }
  
  // -----------------------------------------------------------------------
  // Methods to initialize and cleanup the statement wide members.
  // For the members need to be initialized/reset for each statement,
  // the initialization code has to be put into the following routines.
  // SchemaDB is designed to stay across statements. So the constructor
  // does not get called for each statement.
  //
  // See notes above about CmpContext, which should be the **only** caller!
  // -----------------------------------------------------------------------
  void initPerStatement(NABoolean lightweight = FALSE);		// before stmt
  void cleanupPerStatement();					// after stmt

  Lng32 getCurrentDiskPool() { return currentDiskPool_; }

  void setCurrentDiskPool(Lng32 diskPool) 
                                      { currentDiskPool_ = diskPool; }

  void incCurrentDiskPool() { currentDiskPool_++; }

  float getHbaseBlockCacheFrac();
  
private:  

  // --------------------------------------------------------------------
  // A hash table that uses the qualified tablename as the hash key.
  // This is a collection of NATables that persists across the 
  // compilation of all SQL statements. It is deallocated when the 
  // SQL compiler terminates processing. For now, (11/20/96) tableDB_
  // needs to be cleanup up for each statement, because it is caching
  // the information without checking the timestamp for reload if 
  // necessary.
  // --------------------------------------------------------------------
  NATableDB tableDB_;

  // --------------------------------------------------------------------
  // A collection of ValueDesc (value descriptors).
  // A column can have one or more ValueDescs allocated for it.  
  // This collection is rebuilt for each SQL statement. It is reset
  // at the end of compilation for each statement.
  // --------------------------------------------------------------------
  ValueDescArray valueDArray_;

  // --------------------------------------------------------------------
  // A collection of DomainDesc (domain descriptors).
  // There is one Domain descriptor per column that is referenced in 
  // the query. This collection is rebuilt for each SQL statement.
  // --------------------------------------------------------------------
  DomainDescList domainDList_;

  // --------------------------------------------------------------------
  // In-memory table of defaults values.
  // --------------------------------------------------------------------
  NADefaults defaults_;

  // --------------------------------------------------------------------------
  // The default catalog and schema might come from the NADefaults
  // or be overridden via Ansi rules (the MODULE stmt, CREATE SCHEMA stmt).
  //
  // They might also be supplanted by the MPLOC, if NAMETYPE NSK is in effect
  // (this is of course a Tandem extension).
  //
  // The Ansi override/defaulting occurs in BindWA, not here.
  //
  // The Tandem NAMETYPE NSK defaulting occurs in method getDefaultSchema().
  // Note that defaultSchema_ always contains the ANSI schema,
  // so a call to the get method will by default get the ANSI schema --
  // only by passing the flag bit APPLY_NAMETYPE_RULES might you get 
  // the NAMETYPE NSK version of the schema (which is to say the MPLOC version).
  //
  //   This is sufficiently complicated logic, that the default schema appears
  //   in this class, as opposed to class SqlParser_NADefaults.
  //   Most other of our NADefaults follow no Ansi-required standard
  //   and generally are fixed/global.
  //   Those ones can be gotten directly from NADefaults,
  //   or (for better performance) from the "precompiled" SqlParser_NADefaults
  //   globals -- see NADefaults and SqlParserGlobalsCmn.h.
  // --------------------------------------------------------------------------
  SchemaName defaultSchema_;			// always ANSI (not MPLOC)

  // triggers -- eliezer
  // created on demand
  TriggerDB *triggerDB_;

  // Hash lookup of (UDR) NARoutine objects
  NARoutineDB routineDB_;
  NARoutineDB actionRoutineDB_;

  Lng32 currentDiskPool_;
  float hbaseBlockCacheFrac_;

}; // class SchemaDB

void InitSchemaDB();

inline SchemaDB *ActiveSchemaDB()  { return CmpCommon::context()->schemaDB_; }
inline SchemaDB *ActiveSchemaDB_Safe()
{ return CmpCommon::context() ? CmpCommon::context()->schemaDB_ : NULL; }

inline double getDefaultAsDouble(const Int32& key)
{
  return ActiveSchemaDB()->getDefaults().getAsDouble(key);
}

inline Lng32 getDefaultAsLong(const Int32& key)
{
  return ActiveSchemaDB()->getDefaults().getAsLong(key);
}

#endif /* SCHEMADB_H */



