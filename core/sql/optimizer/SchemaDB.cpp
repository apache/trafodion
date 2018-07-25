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
*************************************************************************
*
* File:         SchemaDB.C
* Description:  Schema Information
* Created:      4/27/94
* Language:     C++
* Status:       Experimental
*
*
*************************************************************************
*/

#define SQLPARSERGLOBALS_NADEFAULTS

#include <stdlib.h>
#include <string.h>

#include "ComAnsiNamePart.h"
#include "ComMPLoc.h"
#include "CmpContext.h"
#include "DefaultValidator.h"
#include "Sqlcomp.h"
#include "StmtCompilationMode.h"
#include "NAClusterInfo.h"
#include "CmpStatement.h"
// triggers -- eliezer
#include "TriggerDB.h"
#include "logmxevent.h"
#include <ComCextdecs.h>
#include "SQLCLIdev.h"
#include "ComUnits.h"
#include "CmpSeabaseDDL.h"
#include "ExpHbaseInterface.h"

#include "SqlParserGlobals.h"			// must be last #include

#include "OptimizerSimulator.h"

#include "seabed/ms.h"

// -----------------------------------------------------------------------
// global functions
// -----------------------------------------------------------------------

void InitSchemaDB()
{
  CmpCommon::context()->initSchemaDB();
}

// ***********************************************************************
// Implementation for functions
// ***********************************************************************
// Note: The SchemaDB object is created during arkcmp initialization, and
// persists until arkcmp dies.  Thus, its data members cannot be put
// anywhere but on the context heap.  If you tried to put them on the
// statement heap, they'd end up on the global system heap, because there
// is no statement heap at the time that they're created -- and even if
// there were a statement heap, as soon as it was "wiped away" these data
// members would be time bombs, since their internal heap pointers would be
// invalid.
SchemaDB::SchemaDB()
  : tableDB_(
          new CTXTHEAP NAHeap("NATable Heap", (NAHeap *)CTXTHEAP, 
          16 * 1024,
          0)),
    routineDB_(CmpCommon::contextHeap()),
    actionRoutineDB_(CmpCommon::contextHeap()),
    valueDArray_(),
    domainDList_(CmpCommon::contextHeap()),
    defaults_(CmpCommon::contextHeap()),
    defaultSchema_(CmpCommon::contextHeap()),
	// triggers -- eliezer
	// created only on demand
    triggerDB_(NULL),
    currentDiskPool_(-1),
    hbaseBlockCacheFrac_(-1.0)
{
  // error during nadefault creation. Cannot proceed. Return.
  if (! defaults_.getSqlParser_NADefaults_Ptr())
    return;

  initPerStatement();
  routineDB_.setMetadata("NEO.UDF.ROUTINES");
  actionRoutineDB_.setMetadata("NEO.UDF.UUDR_ROUTINES");
}

void SchemaDB::initPerStatement(NABoolean lightweight)
{
  if (!lightweight) {
    //CMPASSERT(domainDList_.entries() == 0);
    //CMPASSERT(valueDArray_.entries() == 1);
    //CMPASSERT(tableDB_.entries() == 0);

    // WE CANNOT DO THIS HERE:
    //	TransMode::IsolationLevel il;
    //	if (getDefaults().getIsolationLevel(il))
    //	  CmpCommon::transMode()->isolationLevel() = il;
    // BECAUSE THEN WE'D BE RESETTING DURING A TXN UNDER THE AEGIS OF
    // A "SET TRANSACTION" STMT, WHICH WOULD BE WRONG...
    // Instead, a] CmpCommon::transMode() init's itself once,
    // and b] NADefaults::validateAndInsert() updates CmpCommon::transMode glob
    // when necessary...
  }

  // The defaults table contains EXTERNAL names (and has validated them).
  // The SchemaName object we fill in here has to be INTERNAL format.

  // First, ensure the NADefaults and the SqlParser_NADefaults globals
  // have been initialized.
  //
  NAString cat, sch;
  getDefaults().getCatalogAndSchema(cat, sch);

  // NB: These two ToInternalIdentifier() calls are essential to making
  // delimited identifiers work properly. Is it not useless emitting their
  // return codes into cerr? Would it not be better to CMPASSERT them instead?
  Lng32 err1, err2;
  err1 = ToInternalIdentifier(cat);
  err2 = ToInternalIdentifier(sch);
    if (err1 || err2)
      cerr << err1 << ' ' << cat << '\t'
	   << err2 << ' ' << sch << endl;
  defaultSchema_.setCatalogName(cat);
  defaultSchema_.setSchemaName (sch);

}

// By default, this returns the ANSI default schema.
const SchemaName &SchemaDB::getDefaultSchema(UInt32 flags)
{
  if (flags & REFRESH_CACHE) 
    initPerStatement();

  return defaultSchema_;
}

SchemaDB::~SchemaDB()
{
  cleanupPerStatement();
}

void SchemaDB::cleanupPerStatement()
{
  domainDList_.clear();
  valueDArray_.clear();
  // Create a NULL entry (valueId = 0). This
  // is done so that an uninitialized value id (= 0) could
  // be detected as an invalid entry.
  valueDArray_.insertAt(0,NULL);

  // NATables are now on the statement heap, so there've already been
  // destroyed at this point.

  // Reset metadata (i.e. NATable & histogram) caches after statement
  tableDB_.resetAfterStatement();
  routineDB_.resetAfterStatement();
  actionRoutineDB_.resetAfterStatement();
  CURRCONTEXT_HISTCACHE->resetAfterStatement();
  CURRCONTEXT_HISTCACHE->traceTablesFinalize();
  CURRCONTEXT_HISTCACHE->monitor();
  
  // applies when allocating the TriggerDB from the ContextHeap:
  if (Trigger::Heap() == CmpCommon::contextHeap()) {
	NABoolean deallocationNeeded = triggerDB_->cleanupPerStatement();
	if (deallocationNeeded) {
		delete triggerDB_;
		triggerDB_ = NULL;
	}
  }
  else { // the TriggerDB_ memory was be deallocated with the StatementHeap
	  triggerDB_= NULL;
  }

// EJF 8/14/01 - gpClusterInfo is NULL in NT for NSK preprocessors.

  if (gpClusterInfo)
     gpClusterInfo->cleanupPerStatement();
  
  // Reset static members (quasi-globals) of other classes
  ItemExpr::cleanupPerStatement();

}

void SchemaDB::dropStmtTables()
{
  // Now a noop:
  //domainDList_.clear();
}

void SchemaDB::createStmtTables()
{
  // Now a noop:
  //dropStmtTables();		// (used to have to) cleanup from last stmt
}

NABoolean SchemaDB::endTransaction()
{
  return TRUE;
}

float SchemaDB::getHbaseBlockCacheFrac()
{
  if (hbaseBlockCacheFrac_ < 0) // access JNI layer first time to set value
  {
    CmpSeabaseDDL cmpSBD(STMTHEAP);
    ExpHbaseInterface* ehi = cmpSBD.allocEHI();
    if (!ehi)
      hbaseBlockCacheFrac_ = 0.4 ; // hbase default default
    else {
    float frac;
    Lng32 retcode;
    retcode = ehi->getBlockCacheFraction(frac);
    if (retcode < 0)
      hbaseBlockCacheFrac_ = 0.4 ; // hbase default default
    else
      hbaseBlockCacheFrac_ = frac;
    cmpSBD.deallocEHI(ehi);
    }
  }
  return hbaseBlockCacheFrac_ ;
}

//****************************************************************************
// CollationDB stuff that can't be compiled if in ../common/CharInfo.cpp
//****************************************************************************

Lng32 CollationDB::nextUserCo_(CharInfo::FIRST_USER_DEFINED_COLLATION);

CharInfo::Collation CollationDB::insert(QualifiedName &qn,
					const SchemaName *defaultSchema,
					CollationInfo::CollationFlags flags)
{
  Int32 defaultMatchCount = 0;
  if (defaultSchema)
    defaultMatchCount = qn.applyDefaults(*defaultSchema);

  CMPASSERT(!qn.getCatalogName().isNull()); // fully qualified w/ all defaults

  size_t siz[CollationInfo::SIZEARRAY_SIZE];
  NAString nam(qn.getQualifiedNameAsAnsiString(siz));
  CMPASSERT(siz[0] == 3);		    // fully qualified w/ all defaults

  return insert(nam, siz, flags, defaultMatchCount);
}

CharInfo::Collation CollationDB::insert(ComMPLoc &loc,
					const ComMPLoc *defaultMPLoc,
					CollationInfo::CollationFlags flags)
{
  Int32 defaultMatchCount = 0;
  if (defaultMPLoc)
    defaultMatchCount = loc.applyDefaults(*defaultMPLoc);

  CMPASSERT(loc.isValid(ComMPLoc::FILE));

  size_t siz[CollationInfo::SIZEARRAY_SIZE];
  NAString nam(loc.getMPName(siz));
  CMPASSERT(siz[0] == 3 || siz[0] == 4);    // was defaulted out to $vol or \sys

  return insert(nam, siz, flags, defaultMatchCount);
}

CharInfo::Collation CollationDB::insert(const char *nam,
					size_t *siz,
					CollationInfo::CollationFlags flags,
					Int32 defaultMatchCount)
{
  CMPASSERT(defaultMatchCount >= 0);
  size_t mat = (size_t)defaultMatchCount;
  CMPASSERT(siz[0] > mat);		    // up to n-1 name parts can match
  siz[0] = mat;				    // no longer cnt of total nameparts,
  					    // now it is cnt of MATCHING parts

  // Well before getting to overflow, wrap around to begin again
  // with automatically generated numbers for non-builtin collations.
  if (nextUserCo_ > 2147000000)                       // well shy of INT_MAX
    nextUserCo_ = CharInfo::FIRST_USER_DEFINED_COLLATION;
  CharInfo::Collation co = (CharInfo::Collation)nextUserCo_++;

  CollationInfo *collInfo = new (heap_)
  		   CollationInfo(heap_, co, nam, flags, siz);

  CollationDBSupertype::insert(collInfo);
  refreshNeeded() = FALSE;
  return co;
}

// triggers -- eliezer
//
// -- SchemaDB::getTriggerDB
//
// The TriggerDB is constructed only if needed. 
//
TriggerDB * SchemaDB::getTriggerDB() {
	if (!triggerDB_)
		triggerDB_ = new (Trigger::Heap()) TriggerDB(Trigger::Heap());
	return triggerDB_;
}
