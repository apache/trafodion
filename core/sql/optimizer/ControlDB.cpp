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
 * File:         ControlDB.C
 * Description:  Repository for CONTROL statements
 *               (the ControlDB retains info during compilation of multiple
 *               statements, similar to SchemaDB)
 * Created:      6/12/96
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

	//##TBD:  In ControlDB.[ch]*, and consumer CmpDescribe.cpp:
	//##	Save the ControlXXX::sqlText of each CQD/CT/CS
	//##	(CQS's shapeText already IS the sqlText),
	//##	and use that to reestablish Arkcmp environment
	//##	(e.g. as done in CmpDescribe's sendAllControls func).


#include "ControlDB.h"

#include "CmpCommon.h"
#include "CmpContext.h"
#include "ComSchemaName.h"

#include "AllRelExpr.h"			// various operators for CQS
#include "RelPackedRows.h"
#include "RelSample.h"
#include "RelSequence.h"
#include "ItemColRef.h"			// ConstValue for CQS
#include "CmpMain.h"
#include "QCache.h"
#include "CmpMemoryMonitor.h"
#include "OptimizerSimulator.h"

#include "hs_log.h"
#include "PCodeExprCache.h"
#include "cli_stdh.h"

extern THREAD_P CmpMemoryMonitor *cmpMemMonitor;

#define CONTROLDBHEAP CTXTHEAP

// -----------------------------------------------------------------------
// global functions
// -----------------------------------------------------------------------

ControlDB * ActiveControlDB()
{
  if (!CmpCommon::context()->controlDB_)
    CmpCommon::context()->initControlDB();
  return CmpCommon::context()->controlDB_;
}

// -----------------------------------------------------------------------
// member functions for class ControlDB
// -----------------------------------------------------------------------

#define REMOVEAT(list,i)	{			\
				  delete list[i];	\
				  list.removeAt(i);	\
				}

ControlDB::ControlDB()
  :cqdList_(CONTROLDBHEAP),
   csList_(CONTROLDBHEAP)
{
  requiredShape_ = NULL;
  requiredShapeWasOnceNonNull_ = FALSE;
  savedRequiredShape_ = NULL;
  outStream_ = NULL;

  ctList_ = new CONTROLDBHEAP LIST(ControlTableOptions *)(CONTROLDBHEAP);
  savedCtList_ = NULL;
}

ControlDB::~ControlDB()
{
  delete requiredShape_;
  delete ctList_;
}

void ControlDB::setRequiredShape(ControlQueryShape *shape)
{
  if (requiredShape_)
    {
      delete requiredShape_->getShape();
      delete requiredShape_;
    }

  if (!shape
	  ||
	  !shape->getShape()
	  ||
      //++MV Edward Bortnikov 02/25/2001
      // CONTROL QUERY SHAPE CUT must produce no query shape
      shape->getShape()->isCutOp()
      //--MV
	  )
    {
      requiredShape_ = NULL;
      return;
    }

  requiredShape_ = (ControlQueryShape *) shape->copyTree(CONTROLDBHEAP);
  requiredShape_->setShape(shape->getShape()->copyTree(CONTROLDBHEAP));
  requiredShapeWasOnceNonNull_ = TRUE;
}


void ControlDB::setHistogramCacheState()
{
  UInt32 size = 0;
  if (CmpCommon::getDefault(CACHE_HISTOGRAMS) == DF_ON)
    size = (ActiveSchemaDB()->getDefaults().getAsLong
            (CACHE_HISTOGRAMS_IN_KB)*1024);
  CURRCONTEXT_HISTCACHE->resizeCache(size);
}

extern char *__progname;

void ControlDB::setControlDefault(ControlQueryDefault *def)
{
  CMPASSERT(def->getAttrEnum() >= 0);

  if (def->getToken() == "")
    {
      // CONTROL QUERY DEFAULT * RESET;
      CMPASSERT(def->reset());
      for (CollIndex i = 0; i < cqdList_.entries(); i++)
      {
        ControlQueryDefault *cqd = cqdList_.at(i);
	const char * name =
	  ActiveSchemaDB()->getDefaults().lookupAttrName(cqd->getAttrEnum());
	NABoolean isResetable = 
	  (NOT ActiveSchemaDB()->getDefaults().isNonResetableAttribute(name));

	if (cqd->getAttrEnum()==ROBUST_QUERY_OPTIMIZATION)
	  resetRobustQueryOptimizationCQDs();

	if (cqd->getAttrEnum()==CACHE_HISTOGRAMS ||
            cqd->getAttrEnum()==CACHE_HISTOGRAMS_IN_KB)
        {
          setHistogramCacheState();
        }

	if (isResetable)
	  {
	    delete cqd;

	    cqdList_.removeAt(i);
	    i--; // because we just removed this pos, and now continue the loop
	  }
      }
      //      cqdList_.clear();
      return;
    }

  NABoolean removeCat = FALSE;
  if (def->getAttrEnum() == SCHEMA)
    {
      ComSchemaName nam(def->getValue());
      if (!nam.getCatalogNamePart().isEmpty()) 
	removeCat = TRUE;
    }

  // Remove attr in list if it matches me
  // (also remove CATALOG attr if I am SCHEMA)
  for (CollIndex i = 0; i < cqdList_.entries(); i++)
    if ((cqdList_[i]->getAttrEnum() == def->getAttrEnum()) ||
        (removeCat && cqdList_[i]->getAttrEnum() == CATALOG))
      {
	ControlQueryDefault *cqd=cqdList_.at(i);
	delete cqd;
	cqdList_.removeAt(i);
	if (!removeCat) 
	  break;
	i--; // because we just removed this pos, and now continue the loop
      }
  
  // return if this default is to be reset. We have already removed
  // it from CQD list.
  if (def->reset()) {
    switch (def->getAttrEnum()) {
    case ROBUST_QUERY_OPTIMIZATION:
      resetRobustQueryOptimizationCQDs();
      break;
    case MVQR_REWRITE_LEVEL:
      resetMVQRCQDs(); 
      break;
    case CACHE_HISTOGRAMS_TRACE_OUTPUT_FILE:
      {
        const char* fname = ActiveSchemaDB()->getDefaults().
          getValue(CACHE_HISTOGRAMS_TRACE_OUTPUT_FILE);
        fname = (fname && stricmp(fname,"") != 0) ? fname : NULL;
        CURRCONTEXT_HISTCACHE->traceTablesFinalize();
        // close any old file descriptor
        CURRCONTEXT_HISTCACHE->closeTraceFile();
        // open new file descriptor
        if (fname)
        {
          CURRCONTEXT_HISTCACHE->openTraceFile(fname);
          FILE *fd = CURRCONTEXT_HISTCACHE->getTraceFileDesc();
          if (fd) 
            fprintf(fd, "query:CQD CACHE_HISTOGRAMS_TRACE_OUTPUT_FILE \'%s\';\n",fname);
        }
        break;
      }
    case CACHE_HISTOGRAMS_MONITOR_OUTPUT_FILE:
      {
        const char* fname = ActiveSchemaDB()->getDefaults().
          getValue(CACHE_HISTOGRAMS_MONITOR_OUTPUT_FILE);
        fname = (fname && stricmp(fname,"") != 0) ? fname : NULL;
        CURRCONTEXT_HISTCACHE->monitor();
        // close any old file descriptor
        CURRCONTEXT_HISTCACHE->closeMonitorFile();
        // open new file descriptor
        if (fname)
        {
          CURRCONTEXT_HISTCACHE->openMonitorFile(fname);
          FILE *fd = CURRCONTEXT_HISTCACHE->getMonitorFileDesc();
          if (fd) 
            fprintf(fd, "query:CQD CACHE_HISTOGRAMS_MONITOR_OUTPUT_FILE \'%s\';\n",fname);
        }
        break;
      }
    case CACHE_HISTOGRAMS:
    case CACHE_HISTOGRAMS_IN_KB:
      setHistogramCacheState();
      break;
    }

    // for embedded compilers, keep a copy of the reset CQD so it can be passed to a cmp Context
    // we switch to. This is needed in case the CQD was changed earlier and the changed value
    // was passed on to the same cmp Context in a previous switch. 
    if ((IdentifyMyself::GetMyName() == I_AM_EMBEDDED_SQL_COMPILER) &&
        (def->getValue() != ""))  // internal reset not through the CLI
    {
       ControlQueryDefault * defCopy = (ControlQueryDefault *)
       def->copyTree(CONTROLDBHEAP);
       cqdList_.insert(defCopy);
    }

    return;
  }

  if (def->getAttrEnum() == NSK_DBG_LOG_FILE)
  {
    DefaultConstants attrEnum = def->getAttrEnum();
    const char * optDbgLog =
      ActiveSchemaDB()->getDefaults().getValue(attrEnum);

    char* logFileName = NULL;
    Lng32 pid = getpid();

    Lng32 len = strlen(optDbgLog);
    logFileName = new(CmpCommon::statementHeap()) char[len+128];
    memset(logFileName, 0, len+128);

    strcpy(logFileName, optDbgLog);
    strcat(logFileName, ".");
    strcat(logFileName, __progname);
    strcat(logFileName, ".");

    str_itoa(pid, logFileName+strlen(logFileName));

    CURRCONTEXT_OPTDEBUG->openLog(logFileName);

  }

  if (def->getAttrEnum() == NSK_DBG_COMPILE_INSTANCE)
  {
      const char * compileInstance =
        ActiveSchemaDB()->getDefaults().getValue(NSK_DBG_COMPILE_INSTANCE);

      CURRCONTEXT_OPTDEBUG->setCompileInstanceClass(compileInstance);
  }
  
  if (def->getAttrEnum() == MEMORY_MONITOR ||
      def->getAttrEnum() == MEMORY_MONITOR_IN_DETAIL ||
      def->getAttrEnum() == MEMORY_MONITOR_LOGFILE ||
      def->getAttrEnum() == MEMORY_MONITOR_LOG_INSTANTLY)
  {
    DefaultConstants attrEnum = def->getAttrEnum();
    const char *mmCqd = ActiveSchemaDB()->getDefaults().getValue(attrEnum);
    if (def->getAttrEnum() == MEMORY_MONITOR)
      cmpMemMonitor->setIsMemMonitor((mmCqd && stricmp(mmCqd,"ON") == 0) ? TRUE : FALSE);
    if (def->getAttrEnum() == MEMORY_MONITOR_IN_DETAIL)
      cmpMemMonitor->setIsMemMonitorInDetail((mmCqd && stricmp(mmCqd,"ON") == 0) ? TRUE : FALSE);
    if (def->getAttrEnum() == MEMORY_MONITOR_LOGFILE) {
       
       if (mmCqd && stricmp(mmCqd,"NONE") != 0) {

          pid_t pid = getpid();

          NAString x(mmCqd);
          x.append(".");

          x.append(__progname);
          x.append(".");

          char buf[30];
          str_itoa((ULng32)pid, buf);

          x.append(buf);

          cmpMemMonitor->setLogFilename(x.data());
       } else
          cmpMemMonitor->setLogFilename(NULL);
    }
    if (def->getAttrEnum() == MEMORY_MONITOR_LOG_INSTANTLY)
      cmpMemMonitor->setIsLogInstantly((mmCqd && stricmp(mmCqd,"ON") == 0) ? TRUE : FALSE);
  }
  
  ControlQueryDefault * defCopy = (ControlQueryDefault *)
  def->copyTree(CONTROLDBHEAP);

  // Now append.  Thus the list is strictly in sequential order, which is good
  // (because we might have
  //  SET SCHEMA 'A.B';
  //  SET CATALOG 'C';		-- i.e. default schema now C.B
  // and need to display both in CmpDescribeControl).
  cqdList_.insert(defCopy);
  Lng32 metadata_cache_size =0;

  NAString val(STMTHEAP);
  switch (def->getAttrEnum()) {
    // Setting this default to 'ON' allows users to specify their own syskey.
    // Internally, the syskey is treated as a user key. Invalidate the
    // NATable cache (a.k.a. metadata cache) for this default.
  case OVERRIDE_SYSKEY:
    // Basically flushing the NaTable cache.
     ActiveSchemaDB()->getNATableDB()->setCachingOFF();
     // Turning it back on, so that the successive queries'
     // NaTables are cached.
     ActiveSchemaDB()->getNATableDB()->setCachingON();
    break;
  case TRAF_TABLE_SNAPSHOT_SCAN:
    if (CmpCommon::getDefault(TRAF_TABLE_SNAPSHOT_SCAN) == DF_LATEST)
    {
       //when the user sets TRAF_TABLE_SNAPSHOT_SCAN to LATEST
       //we flush the metadata and then we set the caching back to on so that metadata
       //get cached again. If newer snapshots are created after setting the cqd they
       //won't be seen if they are already cached unless the user issue a command/cqd
       //to invalidate or flush the cache. One way for doing that can be to issue
       //"cqd TRAF_TABLE_SNAPSHOT_SCAN 'latest';" again
       ActiveSchemaDB()->getNATableDB()->setCachingOFF();
       ActiveSchemaDB()->getNATableDB()->setCachingON();
    }
    break;
   //need to flush histogram cache, if we change HIST_MC_STATS_NEEDED
   case HIST_MC_STATS_NEEDED:
    if(CURRCONTEXT_HISTCACHE)
      CURRCONTEXT_HISTCACHE->invalidateCache();
    break;
  case CACHE_HISTOGRAMS_TRACE_OUTPUT_FILE:
    {
      const char* fname = ActiveSchemaDB()->getDefaults().
        getValue(CACHE_HISTOGRAMS_TRACE_OUTPUT_FILE);
      fname = (fname && stricmp(fname,"") != 0) ? fname : NULL;
      CURRCONTEXT_HISTCACHE->traceTablesFinalize();
      // close any old file descriptor
      CURRCONTEXT_HISTCACHE->closeTraceFile();
      // open new file descriptor
      if (fname)
      {
        CURRCONTEXT_HISTCACHE->openTraceFile(fname);
        FILE *fd = CURRCONTEXT_HISTCACHE->getTraceFileDesc();
        if (fd) 
          fprintf(fd, "query:CQD CACHE_HISTOGRAMS_TRACE_OUTPUT_FILE \'%s\';\n",fname);
      }
      break;
    }
  case CACHE_HISTOGRAMS_MONITOR_OUTPUT_FILE:
    {
      const char* fname = ActiveSchemaDB()->getDefaults().
        getValue(CACHE_HISTOGRAMS_MONITOR_OUTPUT_FILE);
      fname = (fname && stricmp(fname,"") != 0) ? fname : NULL;
      CURRCONTEXT_HISTCACHE->monitor();
      // close any old file descriptor
      CURRCONTEXT_HISTCACHE->closeMonitorFile();
      // open new file descriptor
      if (fname)
      {
        CURRCONTEXT_HISTCACHE->openMonitorFile(fname);
        FILE *fd = CURRCONTEXT_HISTCACHE->getMonitorFileDesc();
        if (fd) 
          fprintf(fd, "query:CQD CACHE_HISTOGRAMS_MONITOR_OUTPUT_FILE \'%s\';\n",fname);
      }
      break;
    }
  case METADATA_CACHE_SIZE:
    metadata_cache_size = getDefaultAsLong(METADATA_CACHE_SIZE)*1024*1024;
    ActiveSchemaDB()->getNATableDB()->resizeCache(metadata_cache_size);
    break;
  case PCODE_EXPR_CACHE_SIZE:
    {
      //
      // Increase OR decrease the size of the PCode Expression Cache
      // for the current Context.
      //
      Int32 newsiz  = getDefaultAsLong( PCODE_EXPR_CACHE_SIZE );
      Int32 currSiz = CURROPTPCODECACHE->getMaxSize() ;

      // Note: If new size is negative or the same size, just leave cache alone
      if ( ( newsiz >= 0 ) && ( newsiz != currSiz ) )
      {
         CURROPTPCODECACHE->resizeCache( newsiz );
         if ( newsiz < currSiz )
            CURROPTPCODECACHE->clearStats(); // Do this after resizeCache(...)
      }
    }
    break;
  case QUERY_CACHE:
    {
      ULng32 newsiz = getDefaultInK(QUERY_CACHE);
      const NAArray<CmpContextInfo *> & cmpCtxInfo = 
              GetCliGlobals()->currContext()->getCmpContextInfo();
      for (short i = 0; i < cmpCtxInfo.entries(); i++)
      {
        QueryCache * qc = cmpCtxInfo[i]->getCmpContext()->getQueryCache();
        if (newsiz <= 0) qc->clearStats();
        qc->resizeCache(newsiz);
      }
    }
    break;

  case QUERY_CACHE_STATISTICS: // is now a no-op
    break;
  case QUERY_CACHE_AVERAGE_PLAN_SIZE:
    CURRENTQCACHE->setAvgPlanSz
      (getDefaultInK(QUERY_CACHE_AVERAGE_PLAN_SIZE));
    break;
  case QUERY_CACHE_MAX_VICTIMS:
    CURRENTQCACHE->setMaxVictims(getDefaultAsLong(QUERY_CACHE_MAX_VICTIMS));
    break;
  case HQC_LOG_FILE:
    CURRENTQCACHE->invalidateHQCLogging();
    break;
  case HQC_MAX_VALUES_PER_KEY:
    CURRENTQCACHE->setHQCMaxValuesPerKey(getDefaultAsLong(HQC_MAX_VALUES_PER_KEY));
    break;
  case QUERY_CACHE_REQUIRED_PREFIX_KEYS:
  case QUERY_CACHE_STATISTICS_FILE:
    // the above CQD changes do not affect compiler plan quality
    break;
  case ROBUST_QUERY_OPTIMIZATION:
    setRobustQueryOptimizationCQDs();
    break;
  case MVQR_REWRITE_LEVEL:
    setMVQRCQDs();
    break;
  case CACHE_HISTOGRAMS:
  case CACHE_HISTOGRAMS_IN_KB:
    setHistogramCacheState();
    break;
  case DEFAULT_SCHEMA_ACCESS_ONLY:
    if ((CmpCommon::getDefault(DEFAULT_SCHEMA_ACCESS_ONLY)==DF_ON) &&
        (CmpCommon::getDefault(DEFAULT_SCHEMA_NAMETYPE)==DF_USER))
    {
      // update SCHEMA in CQD list
      NAString schName = "";
      CmpCommon::getDefault(SCHEMA, schName, 0);
      NAString cqdSchema("CONTROL QUERY DEFAULT SCHEMA ", STMTHEAP);
      cqdSchema += "'";
      cqdSchema += schName;
      cqdSchema += "';";
      ControlQueryDefault *cqdS = 
        new(STMTHEAP) ControlQueryDefault(cqdSchema,
                                          CharInfo::UTF8
                                          , "SCHEMA", schName, FALSE, 0, STMTHEAP, 0);
      cqdS->setAttrEnum(SCHEMA);
      setControlDefault(cqdS);
    }
    break;
  default:
    break;
  }
}

void ControlDB::do_one_CQD(const char *attrName,
                           const char *attrValue,
                           NABoolean reset,
                           NADefaults::Provenance masterOrigin)
{
  // make sure cqd name is good
  NADefaults &defs = ActiveSchemaDB()->getDefaults();
  DefaultConstants cqdEnum = defs.lookupAttrName(attrName);
  if (cqdEnum < 0) 
    return; // cqd name is bad

  // was it explicitly set by some user action?
  NADefaults::Provenance 
    cqdOrigin = defs.getProvenance(cqdEnum);
  if (cqdOrigin >= NADefaults::READ_FROM_SQL_TABLE && 
      cqdOrigin >= masterOrigin) { // yes, it was. 
    return; // so, leave it alone. 
  } 
  // no, it is not user-specified.
  // (re)set it accordingly
 // put together cqd statement
  NAString text("CONTROL QUERY DEFAULT ", 200, STMTHEAP),
    name(attrName, STMTHEAP), value(attrValue, STMTHEAP);
  text += attrName; 
  if (reset) {
    text += " RESET;";
  } else {
    text += " '"; text += attrValue; text += "';";
  }
  ControlQueryDefault *cqdP = new(STMTHEAP) ControlQueryDefault
    (text,
     CharInfo::UTF8,
     name, value, FALSE, 0, STMTHEAP, reset);
  // "execute" cqd statement
  NADefaults::Provenance prevState = defs.getState();
  defs.setState(NADefaults::DERIVED);
  cqdP->setAttrEnum(defs.validateAndInsert(attrName, value, reset));
  setControlDefault(cqdP);
  defs.setState(prevState);
}

void ControlDB::do_one_RQO_CQD(const char *attrName,
                               const char *attrValue,
                               NABoolean reset)
{
  do_one_CQD(attrName, attrValue, reset,
             ActiveSchemaDB()->getDefaults().getProvenance
             (ROBUST_QUERY_OPTIMIZATION));
}

void ControlDB::doRobustQueryOptimizationCQDs(RQOsetting set)
{
  // ROBUST_QUERY_OPTIMIZATION is an external master CQD that sets its 
  // internal CQDs to max, high, min, system, or reset
  switch (set) {
  case RQO_MAX:
    do_one_RQO_CQD("RISK_PREMIUM_NJ"             , "5.0", FALSE);
    do_one_RQO_CQD("RISK_PREMIUM_SERIAL"         , "2.0", FALSE);
    do_one_RQO_CQD("PARTITIONING_SCHEME_SHARING" , "2"  , FALSE);
    do_one_RQO_CQD("ROBUST_HJ_TO_NJ_FUDGE_FACTOR", "1.0", FALSE);
    do_one_RQO_CQD("ROBUST_SORTGROUPBY"          , "2"  , FALSE);
    do_one_RQO_CQD("RISK_PREMIUM_MJ"             , "2.0", FALSE);
    break;
  case RQO_HIGH:
    do_one_RQO_CQD("RISK_PREMIUM_NJ"             , "2.5", FALSE);
    do_one_RQO_CQD("RISK_PREMIUM_SERIAL"         , "1.5", FALSE);
    do_one_RQO_CQD("PARTITIONING_SCHEME_SHARING" , "1"  , FALSE);
    do_one_RQO_CQD("ROBUST_HJ_TO_NJ_FUDGE_FACTOR", "3.0", FALSE);
    do_one_RQO_CQD("ROBUST_SORTGROUPBY"          , "2"  , FALSE);
    do_one_RQO_CQD("RISK_PREMIUM_MJ"             , "1.5", FALSE);
    break;
  case RQO_MIN:
    do_one_RQO_CQD("RISK_PREMIUM_NJ"             , "1.0", FALSE);
    do_one_RQO_CQD("RISK_PREMIUM_SERIAL"         , "1.0", FALSE);
    do_one_RQO_CQD("PARTITIONING_SCHEME_SHARING" , "0"  , FALSE);
    do_one_RQO_CQD("ROBUST_HJ_TO_NJ_FUDGE_FACTOR", "0.0", FALSE);
    do_one_RQO_CQD("ROBUST_SORTGROUPBY"          , "0"  , FALSE);
    do_one_RQO_CQD("RISK_PREMIUM_MJ"             , "1.0", FALSE);
    break;
  case RQO_SYS:
    do_one_RQO_CQD("RISK_PREMIUM_NJ"             , "SYSTEM", FALSE);
    do_one_RQO_CQD("RISK_PREMIUM_SERIAL"         , "SYSTEM", FALSE);
    do_one_RQO_CQD("PARTITIONING_SCHEME_SHARING" , "SYSTEM", FALSE);
    do_one_RQO_CQD("ROBUST_HJ_TO_NJ_FUDGE_FACTOR", "SYSTEM", FALSE);
    do_one_RQO_CQD("ROBUST_SORTGROUPBY"          , "SYSTEM", FALSE);
    do_one_RQO_CQD("RISK_PREMIUM_MJ"             , "SYSTEM", FALSE);
    break;
  case RQO_RESET:
    do_one_RQO_CQD("RISK_PREMIUM_NJ"             , "", TRUE);
    do_one_RQO_CQD("RISK_PREMIUM_SERIAL"         , "", TRUE);
    do_one_RQO_CQD("PARTITIONING_SCHEME_SHARING" , "", TRUE);
    do_one_RQO_CQD("ROBUST_HJ_TO_NJ_FUDGE_FACTOR", "", TRUE);
    do_one_RQO_CQD("ROBUST_SORTGROUPBY"          , "", TRUE);
    do_one_RQO_CQD("RISK_PREMIUM_MJ"             , "", TRUE);
    break;
  }    
}

void ControlDB::resetRobustQueryOptimizationCQDs()
{
  // ROBUST_QUERY_OPTIMIZATION reset is an 
  // external master CQD that resets its internal CQDs
  doRobustQueryOptimizationCQDs(RQO_RESET);
}

void ControlDB::setRobustQueryOptimizationCQDs()
{
  // ROBUST_QUERY_OPTIMIZATION is an 
  // external master CQD that sets following internal CQDs
  //                              robust_query_optimization
  //                              MINIMUM  SYSTEM  MAXIMUM
  // risk_premium_NJ                 1.0   system    5.0
  // risk_premium_SERIAL             1.0   system    2.0
  // partitioning_scheme_sharing     0     system    2
  // robust_hj_to_nj_fudge_factor    0.0   system    1.0
  // robust_sortgroupby              0     system    2
  // risk_premium_MJ                 1.0   system    2.0
  // NB: We want the robust_query_optimization master CQD to behave like so:
  //     1) controlled internal CQDs appear to have been set thru normal means.
  //     2) user can set, reset these internal CQDs as they choose.
  //     3) user-specified setttings of internal CQDs are honored (ie, left
  //        alone) by master CQD.
  DefaultToken rqo = CmpCommon::getDefault(ROBUST_QUERY_OPTIMIZATION);
  if (rqo == DF_MINIMUM)
    doRobustQueryOptimizationCQDs(RQO_MIN);
  else if (rqo == DF_MAXIMUM)
    doRobustQueryOptimizationCQDs(RQO_MAX);
  else if (rqo == DF_SYSTEM)
    doRobustQueryOptimizationCQDs(RQO_SYS);
  else if (rqo == DF_HIGH)
    doRobustQueryOptimizationCQDs(RQO_HIGH);
}

void ControlDB::do_one_MVQR_CQD(const char *attrName,
                                const char *attrValue,
                                NABoolean reset)
{
  do_one_CQD(attrName, attrValue, reset,
             ActiveSchemaDB()->getDefaults().getProvenance
             (MVQR_REWRITE_LEVEL));
}

void ControlDB::doMVQRCQDs(Lng32 level)
{
  // MVQR_REWRITE_LEVEL is a master CQD that sets its subordinate CQDs
  switch (level) {
  case 0: // off
    do_one_MVQR_CQD("MULTI_JOIN_THRESHOLD"        , "3"  , FALSE);
    break;
  case 1: case 2: case 3: case 4: // on
    do_one_MVQR_CQD("MULTI_JOIN_THRESHOLD"        , "2"  , FALSE);
    break;
  case -1: // reset
    do_one_MVQR_CQD("MULTI_JOIN_THRESHOLD"        , "", TRUE);
    break;
  }
}

void ControlDB::resetMVQRCQDs()
{
  doMVQRCQDs(-1); // reset
}

void ControlDB::setMVQRCQDs()
{
  // MVQR_REWRITE_LEVEL is a
  // master CQD that sets following subordinate CQDs
  //                              MVQR_REWRITE_LEVEL
  //                                 0     SYSTEM    1
  // mvqr_rewrite_level              0     system    1
  // multi_join_threshold            3     system    2
  Lng32 level = CmpCommon::getDefaultLong(MVQR_REWRITE_LEVEL);
  doMVQRCQDs(level);
}

////////////////////////////////////////////////////////////////////////////
//
// Implementation of CONTROL TABLE statement.
//
// NOTE: The tableName passed in had better be the external name,
//	 fully qualified, via
//	   getExtFileName(), ansiName(), getXxxAsAnsiString()
//	 or else mismatches/false-nonmatches may result in the case
//	 of DELIMITED identifiers in the table name.
//
////////////////////////////////////////////////////////////////////////////
static const ControlTableOptions::CTTokens controlTableTokens[] = {
  {"IF_LOCKED",		 ControlTableOptions::IF_LOCKED},
  {"MDAM",		 ControlTableOptions::MDAM},
  {"NOWAIT",		 ControlTableOptions::NOWAIT},
  {"PRIORITY",		 ControlTableOptions::PRIORITY},
  {"PRIORITY_DELTA",	 ControlTableOptions::PRIORITY_DELTA},
  {"SIMILARITY_CHECK",	 ControlTableOptions::SIMILARITY_CHECK},
  {"TABLELOCK",		 ControlTableOptions::TABLELOCK},
  {"TIMEOUT",		 ControlTableOptions::TIMEOUT}
};

static const size_t numControlTableTokens =
  sizeof(controlTableTokens) / sizeof(ControlTableOptions::CTTokens);

ControlTableOptions * ControlDB::getControlTableOption(
			 const NAString &tableName,
			 CollIndex &i)
{
  for (i = 0; i < getCTList().entries(); i++)
    {
      if (getCTList()[i]->tableName() == tableName)
	{
	  return getCTList()[i];
	}
    }

  return NULL; // not found
}

void ControlDB::resetTableValue(const NAString &tableName, const NAString &token)
{
  ControlTableOptions * cto = NULL;
  if (tableName == "*")
    {
      if (token == "")
	{
	  // CONTROL TABLE * RESET;  -- reset everything.
          for (CollIndex i = 0; i < getCTList().entries(); i++)
          {
            cto = getCTList().at(i);
            delete cto;
          }
	  getCTList().clear();
	}
      else
	{
	  // CONTROL TABLE * tok RESET;
	  // CONTROL TABLE * tok 'val';
	  for (CollIndex i = 0; i < getCTList().entries(); i++)
	    {
	      cto = getCTList()[i];
	      cto->remove(token);
	    }
	}
    }
  else
    {
      CollIndex index;
      cto = getControlTableOption(tableName, index);
      if (cto)
	{
	  if (token == "")
	    {
	      // CONTROL TABLE t RESET;  -- remove entry for this tableName.
	      REMOVEAT(getCTList(), index);
	    }
	  else
	    {
	      // CONTROL TABLE t tok RESET;
	      cto->remove(token);
	    }
	}
    }
}

CollIndex ControlDB::isValidToken(const NAString &token)
{
  for (CollIndex index = 0; index < numControlTableTokens; index++)
    {
      if (token == controlTableTokens[index].token_)
	return index;
    }

  return NULL_COLL_INDEX;
}

// Returns TRUE, if a CT value specified is valid for that option.
NABoolean ControlDB::validate(ControlTable *ct)
{
  // Put value in canonical form, first of all.
  NAString &value = (NAString &)ct->getValue();		// cast away constness
  TrimNAStringSpace(value);
  value.toUpper();

  if (value == "" || value == "RESET")
    if (!ct->reset()) ct->reset() = 1;			// 0->1 but not 2->1 !

  const NAString &token = ct->getToken();
  if (token == "") return TRUE;				// wildcard valid

  CollIndex index = isValidToken(token);
  NABoolean valid = (index != NULL_COLL_INDEX);

  if (valid) {

    if (ct->reset()) return TRUE;

    switch (controlTableTokens[index].const_)
      {
      case ControlTableOptions::IF_LOCKED:
	{
	  if ((value == "RETURN") || (value == "WAIT"))
	    valid = TRUE;
	}
      break;

      case ControlTableOptions::MDAM:
	{
	  if ((value == "OFF") || (value == "ON") || (value == "ENABLE"))
	    valid = TRUE;
	}
      break;

      case ControlTableOptions::NOWAIT:
	{
	  if ((value == "ON") || (value == "OFF"))
	    valid = TRUE;
	}
      break;

      case ControlTableOptions::PRIORITY:
	{
	  valid = TRUE;
	  for (size_t i = 0; (i < value.length() && valid); i++)
	    {
	      if (value.data()[i] < '0' ||
		  value.data()[i] > '9')
		valid = FALSE;
	    }
	  if (valid)
	    {
	      Int32 priority = atoi(value.data());

	      // priority must be between 1 and 199.
	      if (priority < 1 || priority > 199)
          valid = FALSE;

	    }
	}
      break;

      case ControlTableOptions::PRIORITY_DELTA:
	{
	  valid = TRUE;
	  NABoolean negative = FALSE;
	  if (value.data()[0] == '-')
	    {
	      negative = TRUE;
	    }
	  for (size_t i = 0 + (negative ? 1 : 0);
	       (i < value.length() && valid); i++)
	    {
	      if (value.data()[i] < '0' ||
		  value.data()[i] > '9')
		valid = FALSE;
	    }
	  if (valid)
	    {
	      Int32 priority = atoi(&value.data()[(negative ? 1 : 0)]);
	      if (negative)
		priority = - priority;
	      // priority must be between 1 and 199.
	      if (priority < -199 || priority > 199)
		valid = FALSE;
	    }
	}
      break;

      case ControlTableOptions::SIMILARITY_CHECK:
	{
	  if ((value == "ON") || (value == "OFF"))
	    valid = TRUE;
	}
      break;

      case ControlTableOptions::TABLELOCK:
	{
	  if ((value == "ON") || (value == "OFF") || (value == "ENABLE"))
	    valid = TRUE;
	}
      break;

      case ControlTableOptions::TIMEOUT:
	{
	  valid = TRUE;
	  if ( value == "-1" ) break;   // 11-8-2000
	  for (size_t i = 0; (i < value.length() && valid); i++)
	    {
	      if (value.data()[i] < '0' ||
		  value.data()[i] > '9')
		valid = FALSE;
	    }
	}
      break;

      default:
	CMPASSERT(FALSE);
	break;

    } // switch

  } // isValidToken

  if (! valid)
    {
      *CmpCommon::diags() << DgSqlCode(-2051)
	<< DgString0("CONTROL TABLE")
	<< DgString1(token)
	<< DgString2(value);
    }

  return valid;
}

NABoolean ControlDB::setControlTableValue(ControlTable *ct)
{
  if (! validate(ct)) return FALSE;

  // Do this AFTER validate() has altered the value string
  const NAString &value = ct->getValue();
  const NAString &token = ct->getToken();

  // CONTROL TABLE * tok 'v'  -- name here will be "*"
  // CONTROL TABLE "*" t 'v'  -- name here will be "\"*\""  (it's an Ansi name!)
  NAString tableName(CmpCommon::statementHeap());

  if (ct->getTableName().isFabricated())
     tableName = "*";
  else
    //ct-bug-10-030102-3803 -Begin
    tableName = ct->getTableName().getUgivenName();
   //ct-bug-10-030102-3803 -End

  if (ct->reset())
    {
      // CONTROL TABLE * RESET; 	(equiv:)  CONTROL TABLE * * RESET;
      // CONTROL TABLE t RESET; 	(equiv:)  CONTROL TABLE t * RESET;
      // CONTROL TABLE * tok RESET;
      // CONTROL TABLE t tok RESET;
      // and all the above forms twice more,
      // replacing RESET by value string of '' and 'reset'.
      resetTableValue(tableName, token);
      return TRUE;
    }

  if (token == "")
    {
      // CONTROL TABLE t * 'val';  -- is allowed by Parser for all them 'reset'
      // things preceding, but here we flag it as illegal.
      ct->mutateToken() = "*";
      NABoolean starValid = validate(ct);
      CMPASSERT(!starValid);			// it better not be valid
      return FALSE;
    }

  if (tableName == "*")
    {
      // CONTROL TABLE * tok 'val';  -- erase all prior <C.T. t tok 'val';>
      resetTableValue(tableName, token);
    }

  CollIndex index;
  ControlTableOptions * cto = getControlTableOption(tableName, index);
  if (NOT cto)
    {
      // this table control option doesn't exist. Create an entry for it.
      cto = new CONTROLDBHEAP ControlTableOptions(tableName);

      getCTList().insert(cto);
    }

  cto->addTokenAndValue(token, value);

  return TRUE;
}

const NAString * ControlDB::getControlTableValue(const NAString &tableName,
						 const NAString &token)
{
  const NAString * controlValue = NULL;
  if (isValidToken(token) == NULL_COLL_INDEX)
    {
      CMPASSERT(FALSE);
      return NULL;
    }

  // Return the 'value' for 'token'.  First search using tableName.
  CollIndex index;
  ControlTableOptions * cto = getControlTableOption(tableName, index);
  if (! cto)
    {
      // Didn't find in the tableName entry.  Look up in the "*" entry.
      if (tableName != "*")
	{
	  NAString star("*",STMTHEAP);
	  controlValue = getControlTableValue(star, token);
	}
    }
  else
    controlValue = cto->getValue(token);

  if (controlValue == NULL)
    {

       //ct-bug-10-030102-3803 -Begin
       // Check if we have a C.T * set on this token.
       if (tableName != "*")
	{
          NAString star("*",STMTHEAP);
          controlValue = getControlTableValue(star, token);
	}

       if(controlValue == NULL) // Still no match, Set System defaults.
       {
       //ct-bug-10-030102-3803 -End
          // see if this value exists in the CQD table.
          NAString defVal(CmpCommon::statementHeap());
          DefaultConstants dc = NADefaults::lookupAttrName(token, FALSE);
          if (dc >= 0) // valid
	  {
	     CmpCommon::getDefault(dc, defVal, FALSE /*do not return err/warn*/);
	     if (! defVal.isNull())
	     controlValue = new STMTHEAP NAString(defVal, STMTHEAP);
	  }
       //ct-bug-10-030102-3803 -Begin
       }
       //ct-bug-10-030102-3803 -End
    }

  return controlValue;
}

ControlTableOptions::ControlTableOptions()
  : tableName_(CONTROLDBHEAP)
{
  tokens_ = NULL;
  values_ = NULL;
}

ControlTableOptions::ControlTableOptions(const NAString &tableName)
  : tableName_(tableName, CONTROLDBHEAP)
{
  tokens_ = new CONTROLDBHEAP LIST(NAString*)(CONTROLDBHEAP);
  values_ = new CONTROLDBHEAP LIST(NAString*)(CONTROLDBHEAP);
}

ControlTableOptions::~ControlTableOptions()
{
  CollIndex i=0;
  NAString *ns=NULL;

  for (i = 0; i < tokens_->entries(); i++)
  {
    ns = tokens_->at(i);
    delete ns;
  }
  delete tokens_;

  for (i = 0; i < values_->entries(); i++)
  {
    ns = values_->at(i);
    delete ns;
  }
  delete values_;
}

void ControlTableOptions::remove(const NAString &token)
{
  CMPASSERT(tokens_->entries() == values_->entries());

  CollIndex i;
  CollIndex index = NULL_COLL_INDEX;

  for (i = 0; i < tokens_->entries(); i++)
  {
    NAString *ns = tokens_->at(i);

    // compareTo method returns zero if two strings are equal
    if (!ns->compareTo(token) )
    {
      index = i;
      break;
    }
  }

  if (index != NULL_COLL_INDEX)
    {
      NAString *ns=tokens_->at(index);
      delete ns;
      tokens_->removeAt(index);
      ns=values_->at(index);
      delete ns;
      values_->removeAt(index);
    }
}

const NAString * ControlTableOptions::getValue(const NAString &token)
{
  CMPASSERT(tokens_->entries() == values_->entries());

  CollIndex i;
  CollIndex index = NULL_COLL_INDEX;

  for (i = 0; i < tokens_->entries(); i++)
  {
    NAString *ns = tokens_->at(i);

    // CompareTo returns zero if two strings are equal
    if (!ns->compareTo(token) )
    {
      index = i;
      break;
    }
  }

  if (index != NULL_COLL_INDEX)
    return (*values_)[index];
  else
    return NULL;
}

void ControlTableOptions::addTokenAndValue(const NAString &token,
					   const NAString &value)
{
  remove(token);
  NAString *ctToken = new CONTROLDBHEAP NAString(token, CONTROLDBHEAP);
  tokens_->insert(ctToken);
  NAString *ctValue = new CONTROLDBHEAP NAString(value, CONTROLDBHEAP);
  values_->insert(ctValue);
}

const NAString &ControlTableOptions::getToken(CollIndex index)
{
  CMPASSERT((index >= 0) && (index < tokens_->entries()));

  return *((*tokens_)[index]);
}

const NAString &ControlTableOptions::getValue(CollIndex index)
{
  CMPASSERT((index >= 0) && (index < values_->entries()));

  return *((*values_)[index]);
}




////////////////////////////////////////////////////////////////////////////
//
// Implementation of CONTROL SESSION statement.
//
////////////////////////////////////////////////////////////////////////////

// PRIVATE methods

void ControlDB::resetSessionValue(const NAString &token)
{
  ControlSessionOption * cso = NULL;
  if (token == "")
    {
      // CONTROL SESSION * RESET;  -- reset everything.
       for (CollIndex i = 0; i < csList_.entries(); i++)
       {
         cso = csList_.at(i);
         delete cso;
       }
      csList_.clear();
    }
  else
    {
      CollIndex index;
      cso = getControlSessionOption(token, index);
      if (cso)
	REMOVEAT(csList_, index);
    }
}

ControlSessionOption * ControlDB::getControlSessionOption
				     (const NAString &token, CollIndex &i)
{
  // return the 'value' for 'token'.
  for (i = 0; i < csList_.entries(); i++)
    {
      ControlSessionOption * cso = csList_[i];
      if (cso->getToken() == token)
	return cso;
    }

  return NULL;
}


// PUBLIC methods

NABoolean ControlDB::validate(ControlSession *cs)
{
  // Put things into canonical form, then validate.

  // Must cast away constness here, for these Trim/toUpper's

  NAString &token = (NAString &)cs->getToken();
  TrimNAStringSpace(token);
  token.toUpper();

  NAString &value = (NAString &)cs->getValue();
  TrimNAStringSpace(value);
  // but NOT:  value.toUpper();	   // values can be any case

  NABoolean valid = TRUE;

  if (value == "")		// value=="RESET" *is* a valid Session value
    {
      if (!cs->reset()) cs->reset() = 1;		// 0->1 but not 2->1 !
    }
  else if (token == "")
    {
      valid = FALSE;		// CONTROL SESSION '' 'nonblank';  -- error
    }
  else
    {
      // Token must begin with [A-Z_$=], continue with [A-Z0-9_$=.] only.
      const char *str = token.data();
      if (!isalpha(*str) && !strchr("_$=", *str))
	valid = FALSE;
      while (*++str && valid)
        {
	  if (!isalnum(*str) && !strchr("_$=.", *str))
	    valid = FALSE;
	}
    }

  if (! valid)
    {
      token.prepend("'");
      token.append("'");
      *CmpCommon::diags() << DgSqlCode(-2051)
	<< DgString0("CONTROL SESSION")
	<< DgString1(token)
	<< DgString2(value);
    }

  return valid;
}

NABoolean ControlDB::setControlSessionValue(ControlSession *cs)
{
  if (! validate(cs)) return FALSE;

  const NAString &token = cs->getToken();
  const NAString &value = cs->getValue();

  if (cs->reset())
    {
      resetSessionValue(token);
      return TRUE;
    }

  CollIndex index;
  ControlSessionOption * cso = getControlSessionOption(token, index);
  if (NOT cso)
    {
      // this Session control option doesn't exist. Create an entry for it.
      cso = new CONTROLDBHEAP ControlSessionOption();

      csList_.insert(cso);
    }

  cso->addTokenAndValue(token, value);

  return TRUE;
}

const NAString * ControlDB::getControlSessionValue(const NAString &token)
{
  // return the 'value' for 'token'.
  CollIndex index;
  NAString t(token, CmpCommon::statementHeap());
  t.toUpper();

  ControlSessionOption * cso = getControlSessionOption(t, index);
  if (cso)
    return &cso->getValue();
  else
    return NULL;
}

///////////////////////////////////////////////////////////////////////
// Packed form:
//
//    num of Tablenames              (long)
//      num of CTO for a table       (long)
//      Name of Table                (char*, null terminated)
//        Token                      (char*, null terminated)
//        Value                      (char*, null terminated)
//        ...
//
///////////////////////////////////////////////////////////////////////
Lng32 ControlDB::packedLengthControlTableOptions()
{
  Lng32 size = 0;

  if (getCTList().entries() > 0)
    {
      size = sizeof(Lng32);
      for (CollIndex i = 0; i < getCTList().entries(); i++)
	{
	  size += sizeof(Lng32);

	  ControlTableOptions *cto = getCTList()[i];
	  size += strlen(cto->tableName().data()) + 1;

	  for (CollIndex j = 0; j < cto->numEntries(); j++)
	    {
	      size += strlen(cto->getToken(j).data()) + 1
		+ strlen(cto->getValue(j).data()) + 1;
	    } // j

	  size = ROUND8(size);
	} // i
    }

  return size;
}

// see ControlDB::packedLengthControlTableOptions() for packed layout
Lng32 ControlDB::packControlTableOptionsToBuffer(char * buffer)
{
  Lng32 curPos = 0;

  ULng32 tempSize = 0;

  if (getCTList().entries() > 0)
    {
      tempSize = getCTList().entries();
      str_cpy_all(&buffer[curPos], (char*)&tempSize, sizeof(Lng32));
      curPos = sizeof(Lng32);
      for (CollIndex i = 0; i < getCTList().entries(); i++)
	{
	  ControlTableOptions *cto = getCTList()[i];

	  tempSize = cto->numEntries();
	  str_cpy_all(&buffer[curPos], (char*)&tempSize, sizeof(Lng32));
	  curPos += sizeof(Lng32);

	  strcpy(&buffer[curPos], cto->tableName().data());
	  curPos += strlen(cto->tableName().data()) + 1;

	  for (CollIndex j = 0; j < cto->numEntries(); j++)
	    {
	      strcpy(&buffer[curPos], cto->getToken(j).data());
	      curPos += strlen(cto->getToken(j).data()) + 1;

	      strcpy(&buffer[curPos], cto->getValue(j).data());
	      curPos += strlen(cto->getValue(j).data()) + 1;

	    } // j

	  curPos = ROUND8(curPos);
	} // i
    }

  return curPos;
}

// see ControlDB::packedLengthControlTableOptions() for packed layout
Lng32 ControlDB::unpackControlTableOptionsFromBuffer(char * buffer)
{
  if (!ctList_)
    ctList_ = new CONTROLDBHEAP LIST(ControlTableOptions *)(CONTROLDBHEAP);

  Lng32 numEntries;
  Lng32 curPos = 0;
  str_cpy_all((char*)&numEntries, buffer, sizeof(Lng32));
  curPos += sizeof(Lng32);
  for (Int32 i = 0; i < numEntries; i++)
    {
      Lng32 numCTO;
      str_cpy_all((char*)&numCTO, &buffer[curPos], sizeof(Lng32));
      curPos += sizeof(Lng32);

      char * tableName = &buffer[curPos];
      ControlTableOptions * cto =
	new CONTROLDBHEAP ControlTableOptions(tableName);
      getCTList().insert(cto);
      curPos += strlen(&buffer[curPos]) + 1;

      for (Int32 j = 0; j < numCTO; j++)
	{
	  char * token = &buffer[curPos];
	  curPos += strlen(&buffer[curPos]) + 1;
	  char * value = &buffer[curPos];
	  curPos += strlen(&buffer[curPos]) + 1;

	  cto->addTokenAndValue(token, value);
	}
      curPos = ROUND8(curPos);
    }

  return curPos;
}

NABoolean ControlDB::isSameCTO(char * buffer, Lng32 bufLen)
{
  return FALSE;
}

Lng32 ControlDB::saveCurrentCTO()
{
  if (savedCtList_)
    delete savedCtList_;

  savedCtList_ = ctList_;

  ctList_ = new CONTROLDBHEAP LIST(ControlTableOptions *)(CONTROLDBHEAP);

  return 0;
}

Lng32 ControlDB::restoreCurrentCTO()
{
  if (ctList_)
    delete ctList_;

  ctList_ = savedCtList_;

  savedCtList_ = NULL;

  return 0;
}

Lng32 ControlDB::packedLengthControlQueryShape()
{
  Lng32 size = 0;

  if (getRequiredShape())
    size = (Lng32)getRequiredShape()->getShapeText().length() + 1;

  return size;
}

Lng32 ControlDB::packControlQueryShapeToBuffer(char * buffer)
{
  Lng32 size = 0;

  if (getRequiredShape())
    {
      size = (Lng32)getRequiredShape()->getShapeText().length() + 1;

      strcpy(buffer, getRequiredShape()->getShapeText().data());
    }

  return size;
}

Lng32 ControlDB::unpackControlQueryShapeFromBuffer(char * buffer)
{

  return 0;
}

Lng32 ControlDB::saveCurrentCQS()
{
  if (savedRequiredShape_)
    delete savedRequiredShape_;

  savedRequiredShape_ = requiredShape_;

  requiredShape_ = NULL;

  return 0;
}

Lng32 ControlDB::restoreCurrentCQS()
{
  if (requiredShape_)
    delete requiredShape_;

  requiredShape_ = savedRequiredShape_;

  savedRequiredShape_ = NULL;

  return 0;
}

NABoolean ControlDB::isSameCQS(char * buffer, Lng32 bufLen)
{
  if ((getRequiredShape()) &&
      (bufLen > 0) &&
      (strcmp(getRequiredShape()->getShapeText().data(), buffer) == 0))
    return TRUE;

  return FALSE;
}

ControlSessionOption::ControlSessionOption()
{
  token_ = NULL;
  value_ = NULL;
}

ControlSessionOption::~ControlSessionOption()
{
  delete token_;
  delete value_;
}

void ControlSessionOption::addTokenAndValue(const NAString &token,
					    const NAString &value)
{
  delete token_;
  delete value_;

  token_ = new CONTROLDBHEAP NAString(token, CONTROLDBHEAP);
  value_ = new CONTROLDBHEAP NAString(value, CONTROLDBHEAP);
}

// ===========================================================================

// ---------------------------------------------------------------------------
// This code is called by SqlParser.y.
// It was moved out of there because that file was getting too big --
// the c89 compiler was choking on it (ugen assertion failure).
// ---------------------------------------------------------------------------

// argument checks for CONTROL QUERY SHAPE syntax
static NABoolean badSingleArg(const NAString &fname, ExprNodePtrList *args,
			      ComDiagsArea *diags)
{
  if (args->entries() != 1 OR
      args->at(0)->castToRelExpr() == NULL)
    {
      *diags << DgSqlCode(-3113) <<
	DgString0(NAString("One relational argument expected for ") + fname);
      return TRUE;
    }
  return FALSE;
}

static NABoolean badTwoArgs(const NAString &fname, ExprNodePtrList *args,
			    ComDiagsArea *diags)
{
  if (args->entries() != 2 OR
      args->at(0)->castToRelExpr() == NULL OR
      args->at(1)->castToRelExpr() == NULL)
    {
      *diags << DgSqlCode(-3113) <<
	DgString0(NAString("Two relational arguments expected for ") + fname);
      return TRUE;
    }
  return FALSE;
}

// decode one relational expression for CONTROL QUERY SHAPE
ExprNode *DecodeShapeSyntax(const NAString &fname,
			    ExprNodePtrList *args,
			    ComDiagsArea *diags,
			    CollHeap *heap)
{
  ExprNode * result = NULL;
  enum { FORCED_EXCHANGE,
	 FORCED_JOIN,
         FORCED_OTHER } forcedType = FORCED_OTHER;
  OperatorTypeEnum wildcard;
  ExchangeForceWildCard::forcedExchEnum whichExch;

  // **** NOTE: this procedure basically adds functions that could
  // equally well be handled by parsing rules. The reason for doing
  // it inside this procedure is to avoid mixing the CONTROL QUERY SHAPE
  // syntax details with the rest of the SQL parser. Once CONTROL QUERY
  // shape has become an accepted part of our product this procedure
  // could be changed. Error messages should then be parameterized.

  // supports scan, scan(table), scan(table, index),
  // join, tsj, groupby, materialize, exchange, union, mvi, root,
  // mj, hj, sort_groupby, hash_groupby, sort, esp_exchange, pa, papa
  // at this time and supports type1, type2 and <num of esps> as third
  // and fourth arguments of join operators (join, tsj, hj, mj).
  // In addition to the short names (a deprecated feature), the syntax
  // now includes identifiers that are equal to those printed by EXPLAIN.

  // support is also added to handle scan(list of scan options) where
  // scan options are passed as ForcedScanWildCard objects in the list
  // *args. support for scan(table) and scan(table, index) remains.
  // Also have support for MDAM_COLUMNS(columns list) were column list
  // is a list of sparse, dense, or system options passed as ConstValues.

  if (fname == "SCAN" OR
      fname == "FILE_SCAN" OR
      fname == "INDEX_SCAN")
    {
      Int32 numArgs = args->entries();
      Int32 firstNonStringArg=0;
      NAString tableName(CmpCommon::statementHeap());
      NAString indexName(CmpCommon::statementHeap());
      NABoolean dummyNegate = FALSE;
      ItemExpr *itm;

      // first we would like to find the first non String argument
      // this is because we like to handle the shortcut cases
      // scan('t1',...) and scan('t1','i1',...).

      NABoolean exitFlag = FALSE;
      Int32 i;

      for (i=0; i<numArgs; i++)
	{
	  itm = args->at(i)->castToItemExpr();
	  if (itm && itm->castToConstValue(dummyNegate))
	    {
	      const NAType *nat = (itm->castToConstValue(dummyNegate))->getType();
	      if (nat->getTypeQualifier() != NA_CHARACTER_TYPE)
		exitFlag = TRUE;
	    }
	  else
	    exitFlag = TRUE;
	  if (exitFlag) break;
	}
      firstNonStringArg = i;

      if (firstNonStringArg >= 3)
	{
	  // no more than two literals can be passed
	  *diags << DgSqlCode(-3113) <<
	       DgString0("More than 2 string arguments in scan(...) statement.");
	  return NULL;
	}

      if ((numArgs >= 1)&&(firstNonStringArg >= 1))
	{
	  // first argument is a string specifying the table name
	  itm = args->at(0)->castToItemExpr();
	  if (itm == NULL OR
	      itm->castToConstValue(dummyNegate) == NULL)
	    {
	      *diags << DgSqlCode(-3113) <<
		DgString0("Character constant for table name expected");
	      return NULL;
	    }
	  tableName = NAString(
	       (char *) itm->castToConstValue(dummyNegate)->getConstValue(),
	       (size_t) itm->castToConstValue(dummyNegate)->getStorageSize());
	  // sorry, no delimited identifiers
	  tableName.toUpper();
	}
      if ((numArgs >= 2)&&(firstNonStringArg >= 2))
	{
	  // second argument is the index name
	  itm = args->at(1)->castToItemExpr();
	  if (itm == NULL OR
	      itm->castToConstValue(dummyNegate) == NULL)
	    {
	      *diags << DgSqlCode(-3113) <<
		DgString0("Character constant for index name expected");
	      return NULL;
	    }
	  indexName = NAString(
	       (char *) itm->castToConstValue(dummyNegate)->getConstValue(),
	       (size_t) itm->castToConstValue(dummyNegate)->getStorageSize());
	  // sorry, no delimited identifiers
	  indexName.toUpper();
	}

      if (indexName != "")
	result = new (heap) ScanForceWildCard(tableName,indexName);
      else if(tableName != "")
	result = new (heap) ScanForceWildCard(tableName);
      else
	result = new (heap) ScanForceWildCard();

      // Now update result with other scan options using
      // mergeScanOptions() functions
      for (i=firstNonStringArg; i<numArgs; i++)
	{
	  if (args->at(i)->getOperatorType()!=REL_FORCE_ANY_SCAN)
	    {
	      *diags << DgSqlCode(-3113) <<
		DgString0("Illegal scan(...) argument.");
	      return NULL;
	    }
	  ScanForceWildCard* tmpScan = (ScanForceWildCard*)args->at(i);
	  if (!((ScanForceWildCard*)result)->mergeScanOptions(*tmpScan))
	    {
	      // This can only happen if conflicting scan options were given
	      *diags << DgSqlCode(-3113) <<
		DgString0("Conflicting scan options.");
	      return NULL;
	    }
	}

      if (((ScanForceWildCard*)result)->doesThisCoflictMasterSwitch())
	{
	  *diags << DgSqlCode(-3113) <<
	    DgString0("MDAM options Conflicting With Master Switch.");
	  return NULL;
	}
      ((ScanForceWildCard*)result)->prepare();
    }

  else if (fname == "MDAM_COLUMNS")
    {
      result = new (heap) ScanForceWildCard();
      NABoolean dummyNegate = FALSE;
      Int32 numColumns = args->entries();
      ItemExpr *itm;

      ScanForceWildCard::scanOptionEnum* columnAlgorithms
	= new (heap) ScanForceWildCard::scanOptionEnum[numColumns];

      for (Int32 i=0; i<numColumns; i++)
	{
	  itm = args->at(i)->castToItemExpr();
	  if (itm == NULL OR itm->castToConstValue(dummyNegate) == NULL)
	    {
	      *diags << DgSqlCode(-3113) <<
		DgString0("Illegal MDAM_COLUMNS (...) argument");
	      return NULL;
	    }
	  const NAType *nat = itm->castToConstValue(dummyNegate)->getType();
	  if (nat->getTypeQualifier() != NA_NUMERIC_TYPE)
	    {
	      *diags << DgSqlCode(-3113) <<
		DgString0("Illegal MDAM_COLUMNS (...) argument");
	      return NULL;
	    }
	  short arg =
	    itm->castToConstValue(dummyNegate)->getExactNumericValue();

	  if (arg == _SYSTEM_)
	    {
	      columnAlgorithms[i] = ScanForceWildCard::COLUMN_SYSTEM;
	    }
	  else if (arg == _SPARSE_)
	    {
	      columnAlgorithms[i] = ScanForceWildCard::COLUMN_SPARSE;
	    }
	  else if (arg == _DENSE_)
	    {
	      columnAlgorithms[i] = ScanForceWildCard::COLUMN_DENSE;
	    }
	  else
	    {
	      *diags << DgSqlCode(-3113) <<
		DgString0("Illegal MDAM_COLUMNS (...) argument.");
	      return NULL;
	    }
	}
      ((ScanForceWildCard*)result)->
	setColumnOptions(numColumns, columnAlgorithms,
			 ScanForceWildCard::MDAM_COLUMNS_NO_MORE);
    }

  else if (fname == "JOIN")
    {
      forcedType = FORCED_JOIN;
      wildcard = REL_FORCE_JOIN;
    }
  else if (fname == "TSJ" OR
	   fname == "NESTED_JOIN")
    {
      forcedType = FORCED_JOIN;
      wildcard = REL_FORCE_NESTED_JOIN;
    }
  else if (fname == "GROUPBY")
    {
      if (badSingleArg(fname,args,diags))
	return NULL;
      result = new (heap) WildCardOp(
	   REL_ANY_GROUP,
	   0,
	   args->at(0)->castToRelExpr());
    }
  else if (fname == "UNION")
    {
      if (badTwoArgs(fname,args,diags))
	return NULL;

      result = new (heap) MergeUnion(
	   args->at(0)->castToRelExpr(),
	   args->at(1)->castToRelExpr());
    }
  else if (fname == "MVI" OR
	   fname == "EXPR")
    {
      if (badSingleArg(fname,args,diags))
	return NULL;

      result = new (heap) MapValueIds(
	   args->at(0)->castToRelExpr());
    }
  else if (fname == "FAST_EXTRACT" ||
           fname == "HIVE_INSERT")
    {
      if (badSingleArg(fname,args,diags))
	return NULL;
      result = new (heap) WildCardOp(
	   REL_ANY_EXTRACT,
	   0,
	   args->at(0)->castToRelExpr());
    }
  else if (fname == "SORT")
    {
      if (badSingleArg(fname,args,diags))
	return NULL;

      result = new (heap) Sort(
	   args->at(0)->castToRelExpr());
    }
  else if (fname == "EXCHANGE")
    {
      forcedType = FORCED_EXCHANGE;
      wildcard   = REL_FORCE_EXCHANGE;
      whichExch  = ExchangeForceWildCard::ANY_EXCH;
    }
  else if (fname == "PA" OR
	   fname == "PARTITION_ACCESS")
    {
      forcedType = FORCED_EXCHANGE;
      wildcard   = REL_FORCE_EXCHANGE;
      whichExch  = ExchangeForceWildCard::FORCED_PA;
    }
  else if (fname == "PAPA" OR
	   fname == "SPLIT_TOP_PA")
    {
      forcedType = FORCED_EXCHANGE;
      wildcard   = REL_FORCE_EXCHANGE;
      whichExch  = ExchangeForceWildCard::FORCED_PAPA;
    }
  else if (fname == "ESP_EXCHANGE" OR
	   fname == "REPARTITION")
    {
      forcedType = FORCED_EXCHANGE;
      wildcard   = REL_FORCE_EXCHANGE;
      whichExch  = ExchangeForceWildCard::FORCED_ESP_EXCHANGE;
    }
  else if (fname == "MJ" OR
	   fname == "MERGE_JOIN")
    {
      forcedType = FORCED_JOIN;
      wildcard = REL_FORCE_MERGE_JOIN;
    }

  // previous HJ (hybrid_hash_join) is now split into three
  // HJ (hash_join), type unknown, the nextSubstitute will pick
  // HHJ (hybrid_hash_join) and OHJ (ordered_hash_join)
  else if (fname == "HJ" OR
	   fname == "HASH_JOIN")
    {
      forcedType = FORCED_JOIN;
      wildcard = REL_FORCE_HASH_JOIN;
    }

  else if (fname == "HHJ" OR
	   fname == "HYBRID_HASH_JOIN")
    {
      forcedType = FORCED_JOIN;
      wildcard = REL_FORCE_HYBRID_HASH_JOIN;
    }
  else if (fname == "OHJ" OR
	   fname == "ORDERED_HASH_JOIN")
    {
      forcedType = FORCED_JOIN;
      wildcard = REL_FORCE_ORDERED_HASH_JOIN;
    }

  // Ordered cross product type for star schema change.
  else if (fname == "OCP" OR
	   fname == "ORDERED_CROSS_PRODUCT")
    {
      forcedType = FORCED_JOIN;
      wildcard = REL_FORCE_ORDERED_CROSS_PRODUCT;
    }

  else if (fname == "SORT_GROUPBY")
    {
      if (badSingleArg(fname,args,diags))
	return NULL;

      result = new (heap) SortGroupBy(
	   args->at(0)->castToRelExpr());
    }
  else if (fname == "HG" OR
	   fname == "HASH_GROUPBY")
    {
      if (badSingleArg(fname,args,diags))
	return NULL;

      result = new (heap) HashGroupBy(
	   args->at(0)->castToRelExpr());
    }
  else if (fname == "SHORTCUT_GROUPBY")
    {
       if (badSingleArg(fname,args,diags))
	return NULL;

      result = new (heap) ShortCutGroupBy(
	   args->at(0)->castToRelExpr());
    }
  else if (fname == "TRANSPOSE")
    {
      if (badSingleArg(fname,args,diags))
        return NULL;

      result = new (heap) Transpose(NULL,NULL,
                                            args->at(0)->castToRelExpr());
    }
  else if (fname == "SEQUENCE")
    {
      if (badSingleArg(fname,args,diags))
        return NULL;

      result = new (heap) RelSequence(args->at(0)->castToRelExpr());
    }
  else if (fname == "UNPACK")
    {
      if (badSingleArg(fname,args,diags))
        return NULL;

      result = new (heap) UnPackRows(0, NULL, NULL, NULL,
				     args->at(0)->castToRelExpr(), 
				     NULL_VALUE_ID);
    }
  else if (fname == "PACK")
    {
      if (badSingleArg(fname,args,diags))
        return NULL;

      result = new (heap) Pack(0, args->at(0)->castToRelExpr());
    }
  else if (fname == "SAMPLE")
    {
      if (badSingleArg(fname,args,diags))
	return NULL;

      result = new (heap) RelSample(args->at(0)->castToRelExpr(),
                                            RelSample::ANY,
                                            NULL);
    }
  else if (fname == "ISOLATED_SCALAR_UDF")
    {
      Int32 numArgs = args->entries();
      if (numArgs > 2)
      {
        // No more than two arguments allowed.
        *diags << DgSqlCode(-3113) <<
             DgString0("Expected no more than 2 arguments in isolated_scalar_udf(...) statement.");
        return NULL;
      }

      // Result is a wildcard for UDF.
      result = new (heap) UDFForceWildCard(REL_FORCE_ANY_SCALAR_UDF);

      // Update result with the 'isolated_scalar_udf' options given
      // in the CQS using mergeUDFOptions() function.
      for (Int32 i=0; i<numArgs; i++)
      {
        if (args->at(i)->getOperatorType()!=REL_FORCE_ANY_SCALAR_UDF)
        {
          *diags << DgSqlCode(-3113) <<
            DgString0("Illegal isolated_scalar_udf argument.");
          return NULL;
        }

        UDFForceWildCard *tmpUDF = (UDFForceWildCard*)args->at(i);
        if (!((UDFForceWildCard*)result)->mergeUDFOptions(*tmpUDF))
        {
          // This can only happen if conflicting udf options were given
         *diags << DgSqlCode(-3113) <<
            DgString0("Conflicting isolated_scalar_udf options.");
          return NULL;
        }
      }
      // If action name argument given, then must also specify function name.
      if (((UDFForceWildCard*)result)->getFunctionName() == "" && 
          ((UDFForceWildCard*)result)->getActionName() != "")
        {
         *diags << DgSqlCode(-3113) <<
            DgString0("Missing isolated_scalar_udf SCALAR_UDF option.");
          return NULL;
        }
    }
  else if (fname == "TMUDF")
    {
      Int32 numArgs = args->entries();
      OperatorTypeEnum op = REL_ANY_LEAF_TABLE_MAPPING_UDF;
      RelExpr *child0 = NULL;
      RelExpr *child1 = NULL;

      if (numArgs == 1)
        {
          if (badSingleArg(fname,args,diags))
            return NULL;
          op = REL_ANY_UNARY_TABLE_MAPPING_UDF;
          child0 = args->at(0)->castToRelExpr();
        }
      else if (numArgs == 2)
        {
          if (badTwoArgs(fname,args,diags))
            return NULL;
          op = REL_ANY_BINARY_TABLE_MAPPING_UDF;
          child0 = args->at(0)->castToRelExpr();
          child1 = args->at(1)->castToRelExpr();
        }
      else if (numArgs > 2)
        {
          *diags << DgSqlCode(-3113) <<
	    DgString0("TMUDF operator must have 0, 1 or 2 arguments.");
          return NULL;
        }

      result = new (heap) WildCardOp(
	   op,
	   0,
	   child0,
           child1);
    }
  else
    {
      NAString e(fname, CmpCommon::statementHeap());
      e += " is not a valid operator for CONTROL QUERY SHAPE";
      *diags << DgSqlCode(-3113) << DgString0(e);
      return NULL;
    }

  if (result == NULL AND forcedType == FORCED_JOIN)
    {
      // common code for all forced joins, build a relexpr, interpret
      // additional parameters, and do an error check
      if (args->entries() < 2 OR args->entries() > 4)
	{
	  *diags << DgSqlCode(-3113) <<
	    DgString0("Join force operators must have 2, 3, or 4 arguments.");
	  return NULL;
	}

      JoinForceWildCard::forcedPlanEnum whichPlan =
	JoinForceWildCard::ANY_PLAN;
      Int32 numOfEsps = 0;

      if (args->entries() > 2)
	{
	  for (Int32 i = 2; i < (Int32)args->entries(); i++)
	    {
	      NABoolean dummyNegate = FALSE;
	      if (NOT (args->at(i)->castToItemExpr() AND
		       args->at(i)->castToItemExpr()->castToConstValue(dummyNegate)))
		{
		  *diags << DgSqlCode(-3113) <<
		    DgString0("Additional join arguments must be TYPE1, TYPE2 or a number.");
		  return NULL;
		}

	      ConstValue *cv =
		args->at(i)->castToItemExpr()->castToConstValue(dummyNegate);
	      const NAType *nat = cv->getType();

	      if (nat->getTypeQualifier() == NA_CHARACTER_TYPE)
		{
		  NAString planNum ((char *) cv->getConstValue(),
                                    (size_t) cv->getStorageSize(),
									STMTHEAP);
		  planNum.toUpper();

		  if (planNum == "PLAN0")
		    whichPlan = JoinForceWildCard::FORCED_PLAN0;
		  else if (planNum == "PLAN1")
		    whichPlan = JoinForceWildCard::FORCED_PLAN1;
		  else if (planNum == "PLAN2")
		    whichPlan = JoinForceWildCard::FORCED_PLAN2;
		  else if (planNum == "PLAN3")
		    whichPlan = JoinForceWildCard::FORCED_PLAN3;
		  else if (planNum == "TYPE1")
		    whichPlan = JoinForceWildCard::FORCED_TYPE1;
		  else if (planNum == "TYPE2")
		    whichPlan = JoinForceWildCard::FORCED_TYPE2;
                  else if (planNum == "INDEXJOIN")
                    whichPlan = JoinForceWildCard::FORCED_INDEXJOIN;
		  else
		    {
		      *diags << DgSqlCode(-3113) <<
			DgString0("Expected TYPE1 or TYPE2.");
		      return NULL;
		    }
		}
	      else if (nat->getTypeQualifier() == NA_NUMERIC_TYPE)
		{
		  if (cv->getStorageSize() > 2)
		    {
		      *diags << DgSqlCode(-3113) <<
			DgString0("Number of ESPs (short int) expected.");
		      return NULL;
		    }
		  numOfEsps = cv->getExactNumericValue();
		}
	      else
		{
		  *diags << DgSqlCode(-3113) <<
		    DgString0("Additional join arguments must be string or integer.");
		  return NULL;
		}
	    }
	}

      if (NOT (args->at(0)->castToRelExpr() AND
	       args->at(1)->castToRelExpr()))
	{
	  *diags << DgSqlCode(-3113) <<
	    DgString0("Join needs two arguments representing a query.");
	  return NULL;
	}

      result = new (heap)
	JoinForceWildCard(wildcard,
			  args->at(0)->castToRelExpr(),
			  args->at(1)->castToRelExpr(),
			  whichPlan,
			  numOfEsps);
    }
  else if (result == NULL AND forcedType == FORCED_EXCHANGE)
    {
      // -----------------------------------------------------------------
      // process forced exchange and additional arguments that specify the
      // form of logical partitioning and number of ESPs below the exchange
      // -----------------------------------------------------------------

      ExchangeForceWildCard::forcedLogPartEnum whichLogPart =
	ExchangeForceWildCard::ANY_LOGPART;
      Lng32 numOfEsps = -1;

      // check the first argument
      if (args->entries() < 1 OR
	  args->at(0)->castToRelExpr() == NULL)
	{
	  *diags << DgSqlCode(-3113) << DgString0(
	       "Shape as first argument expected for exchange operator");
	  return NULL;
	}

      if (args->entries() > 1)
	{
	  for (Int32 i = 1; i < (Int32)args->entries(); i++)
	    {
	      NABoolean dummyNegate = FALSE;
	      if (NOT (args->at(i)->castToItemExpr() AND
		       args->at(i)->castToItemExpr()->
		       castToConstValue(dummyNegate)))
		{
		  *diags << DgSqlCode(-3113) <<
		    DgString0("Additional exchange arguments must be 'GROUP', 'SPLIT', 'REPART' or a number (of ESPs).");
		  return NULL;
		}

	      ConstValue *cv =
		args->at(i)->castToItemExpr()->castToConstValue(dummyNegate);
	      const NAType *nat = cv->getType();

	      if (nat->getTypeQualifier() == NA_CHARACTER_TYPE)
		{
		  NAString logPart( (char *) cv->getConstValue(),
			            (size_t) cv->getStorageSize(),
                                    CmpCommon::statementHeap() );
		  logPart.toUpper();

		  if (logPart == "GROUP")
		    whichLogPart = ExchangeForceWildCard::FORCED_GROUP;
		  else if (logPart == "SPLIT")
		    whichLogPart = ExchangeForceWildCard::FORCED_SPLIT;
		  else if (logPart == "REPART")
		    whichLogPart = ExchangeForceWildCard::FORCED_REPART;
		  else
		    {
		      *diags << DgSqlCode(-3113) <<
			DgString0("Expected GROUP, SPLIT, or REPART.");
		      return NULL;
		    }
		}
	      else if (nat->getTypeQualifier() == NA_NUMERIC_TYPE)
		{
		  if (cv->getStorageSize() > 2)
		    {
		      *diags << DgSqlCode(-3113) <<
			DgString0("Number of ESPs (short int) expected.");
		      return NULL;
		    }
		  numOfEsps = cv->getExactNumericValue();
		}
	      else
		{
		  *diags << DgSqlCode(-3113) <<
		    DgString0("Additional join arguments must be string or integer.");
		  return NULL;
		}
	    }
	}

      // can't force a number of streams for a generic EXCHANGE or a PA node
      if ((whichExch == ExchangeForceWildCard::ANY_EXCH OR
	   whichExch == ExchangeForceWildCard::FORCED_PA) AND
	  numOfEsps > 0)
	{
	  *diags << DgSqlCode(-3113) << DgString0(
	       "Number of ESPs can not be specified for EXCHANGE or PARTITION_ACCESS.");
	  return NULL;
	}

      // can't force logical partitioning for EXCHANGE or ESP_EXCHANGE
      if ((whichExch == ExchangeForceWildCard::ANY_EXCH OR
	   whichExch == ExchangeForceWildCard::FORCED_ESP_EXCHANGE) AND
	  whichLogPart != ExchangeForceWildCard::ANY_LOGPART)
	{
	  *diags << DgSqlCode(-3113) << DgString0(
	       "GROUP, SPLIT, or REPART are allowed for SPLIT_TOP_PA and PARTITION_ACCESS only.");
	  return NULL;
	}

      result = new (heap)  ExchangeForceWildCard(
	   args->at(0)->castToRelExpr(),
	   whichExch,
	   whichLogPart,
	   numOfEsps);
    }

  return result;
}
