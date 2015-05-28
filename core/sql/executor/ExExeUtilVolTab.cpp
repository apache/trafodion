/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 1994-2015 Hewlett-Packard Development Company, L.P.
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
 * File:         ExExeUtilVolTab.cpp
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
#include "ComSizeDefs.h"
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
#include  "ComRtUtils.h"
#include  "ExStats.h"
#include  "seabed/ms.h"

///////////////////////////////////////////////////////////////////
ex_tcb * ExExeUtilLoadVolatileTableTdb::build(ex_globals * glob)
{
  ExExeUtilLoadVolatileTableTcb * exe_util_tcb;

  exe_util_tcb = 
    new(glob->getSpace()) ExExeUtilLoadVolatileTableTcb(*this, glob);

  exe_util_tcb->registerSubtasks();

  return (exe_util_tcb);
}

////////////////////////////////////////////////////////////////
// Constructor for class ExExeUtilLoadVolatileTableTcb
///////////////////////////////////////////////////////////////
ExExeUtilLoadVolatileTableTcb::ExExeUtilLoadVolatileTableTcb(
     const ComTdbExeUtil & exe_util_tdb,
     ex_globals * glob)
     : ExExeUtilTcb( exe_util_tdb, NULL, glob),
       step_(INITIAL_)
{
  // Allocate the private state in each entry of the down queue
  qparent_.down->allocatePstate(this);
}


//////////////////////////////////////////////////////
// work() for ExExeUtilLoadVolatileTableTcb
//////////////////////////////////////////////////////
short ExExeUtilLoadVolatileTableTcb::work()
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

  // Get the globals stucture of the master executor.
  ExExeStmtGlobals *exeGlob = getGlobals()->castToExExeStmtGlobals();
  ExMasterStmtGlobals *masterGlob = exeGlob->castToExMasterStmtGlobals();
  ContextCli *currContext = masterGlob->getStatement()->getContext();

  while (1)
    {
      switch (step_)
	{
	case INITIAL_:
	  {
	    step_ = INSERT_;
	  }
	break;

	case INSERT_:
	  {
	    // let compiler know that this insert statement should be
	    // treated as a regular insert stmt.
	    // 0x20000 is the bit to allow this.
	    // Bit defined in parser/SqlParserGlobalsCmn.h.
	    //	    masterGlob->getStatement()->getContext()->setSqlParserFlags(0x20000); // NO_IMPLICIT_VOLATILE_TABLE_UPD_STATS

	    // issue the insert command 
	    Int64 rowsAffected;

	    // All internal queries issued from CliInterface assume that
	    // they are in ISO_MAPPING.
	    // That causes mxcmp to use the default charset as iso88591
	    // for unprefixed literals.
	    // The insert...select being issued out here contains the user
	    // specified query and any literals in that should be using
	    // the default_charset.
	    // So we send the isoMapping charset instead of the
	    // enum ISO_MAPPING.
	    Int32 savedIsoMapping = 
	      currContext->getSessionDefaults()->getIsoMappingEnum();
	    cliInterface()->setIsoMapping
	      (currContext->getSessionDefaults()->getIsoMappingEnum());
	    cliRC = cliInterface()->executeImmediate(
		 lvtTdb().insertQuery_,
		 NULL, NULL, TRUE,
		 &rowsAffected,TRUE);
	    cliInterface()->setIsoMapping(savedIsoMapping);
	    if (cliRC < 0)
	      {
		ExHandleErrors(qparent_,
			       pentry_down,
			       0,
			       getGlobals(),
			       NULL,
			       (ExeErrorCode)cliRC,
			       NULL,
			       NULL
			       );
		step_ = ERROR_;
		break;
	      }

	    masterGlob->setRowsAffected(rowsAffected);
	    if (rowsAffected > lvtTdb().threshold_)
	      step_ = UPD_STATS_;
	    else
	      step_ = DONE_;
	  }
	break;

	case UPD_STATS_:
	  {
	    // issue the upd stats command 
	    char * usQuery =
	      new(getHeap()) char[strlen(lvtTdb().updStatsQuery_) + 10 + 1];

	    str_sprintf(usQuery, lvtTdb().updStatsQuery_,
			masterGlob->getRowsAffected());

	    cliRC = cliInterface()->executeImmediate(usQuery,NULL,NULL,TRUE,NULL,TRUE);
	    NADELETEBASIC(usQuery, getHeap());
	    if (cliRC < 0)
	      {
		ExHandleErrors(qparent_,
			       pentry_down,
			       0,
			       getGlobals(),
			       NULL,
			       (ExeErrorCode)cliRC,
			       NULL,
			       NULL
			       );
		step_ = ERROR_;
		break;
	      }
	    
	    step_ = DONE_;
	  }
	break;
	
	case DONE_:
	  {
	    // reset special insert processing bit
	    //	    masterGlob->getStatement()->getContext()->resetSqlParserFlags(0x20000); // NO_IMPLICIT_VOLATILE_TABLE_UPD_STATS

	    // Return EOF.
	    ex_queue_entry * up_entry = qparent_.up->getTailEntry();
	    
	    up_entry->upState.parentIndex = 
	      pentry_down->downState.parentIndex;
	    
	    up_entry->upState.setMatchNo(0);
	    up_entry->upState.status = ex_queue::Q_NO_DATA;
	    
	    // insert into parent
	    qparent_.up->insert();
	    
	    step_ = INITIAL_;
	    qparent_.down->removeHead();
	    
	    return WORK_OK;
	  }
	break;

	case ERROR_:
	  {
	    step_ = DONE_;
	  }
	break;


	} // switch
    } // while
  
  return WORK_OK;
}

////////////////////////////////////////////////////////////////////////
// Redefine virtual method allocatePstates, to be used by dynamic queue
// resizing, as well as the initial queue construction.
////////////////////////////////////////////////////////////////////////
ex_tcb_private_state * ExExeUtilLoadVolatileTableTcb::allocatePstates(
     Lng32 &numElems,      // inout, desired/actual elements
     Lng32 &pstateLength)  // out, length of one element
{
  PstateAllocator<ExExeUtilLoadVolatileTablePrivateState> pa;

  return pa.allocatePstates(this, numElems, pstateLength);
}

/////////////////////////////////////////////////////////////////////////////
// Constructor and destructor for ExeUtil_private_state
/////////////////////////////////////////////////////////////////////////////
ExExeUtilLoadVolatileTablePrivateState::ExExeUtilLoadVolatileTablePrivateState()
{
}

ExExeUtilLoadVolatileTablePrivateState::~ExExeUtilLoadVolatileTablePrivateState()
{
};

///////////////////////////////////////////////////////////////////
ex_tcb * ExExeUtilCleanupVolatileTablesTdb::build(ex_globals * glob)
{
  ExExeUtilCleanupVolatileTablesTcb * exe_util_tcb;

  exe_util_tcb = new(glob->getSpace()) ExExeUtilCleanupVolatileTablesTcb(*this, glob);

  exe_util_tcb->registerSubtasks();

  return (exe_util_tcb);
}

static const QueryString getAllNodeNamesQuery[] =
{
  {" select RTRIM('NSK') from "},
  {"   HP_SYSTEM_CATALOG.system_schema.catsys CS "},
  {"   where cs.cat_name = replace(_ucs2'%s', _ucs2'''', _ucs2'''''') "},
  {"        for read uncommitted access "},
  {" ; "}
};

////////////////////////////////////////////////////////////////
// class ExExeUtilVolatileTablesTcb
///////////////////////////////////////////////////////////////
ExExeUtilVolatileTablesTcb::ExExeUtilVolatileTablesTcb(
     const ComTdbExeUtil & exe_util_tdb,
     ex_globals * glob)
     : ExExeUtilTcb( exe_util_tdb, NULL, glob),
       nodeNamesList_(NULL)
{
  // Allocate the private state in each entry of the down queue
  qparent_.down->allocatePstate(this);
}

short ExExeUtilVolatileTablesTcb::getAllNodeNames(char * param1)
{
  Lng32 cliRC = 0;

  ex_queue_entry * pentry_down = qparent_.down->getHeadEntry();
  ExExeUtilPrivateState & pstate =
    *((ExExeUtilPrivateState*) pentry_down->pstate);

  ExExeStmtGlobals *exeGlob = getGlobals()->castToExExeStmtGlobals();
  ExMasterStmtGlobals *masterGlob = exeGlob->castToExMasterStmtGlobals();

  short rc = 0;

  rc = initializeInfoList(nodeNamesList_);
  if (rc)
    return -1;

  if (param1)
    {
      const QueryString * qs;
      Int32 sizeOfqs = 0;
      
      qs = getAllNodeNamesQuery;
      sizeOfqs = sizeof(getAllNodeNamesQuery);
      
      Int32 qryArraySize = sizeOfqs / sizeof(QueryString);
      char * gluedQuery;
      Lng32 gluedQuerySize;
      glueQueryFragments(qryArraySize,  qs,
			 gluedQuery, gluedQuerySize);
      
      Lng32 extraSpace = 2 * 10 /*segment name*/+ ComMAX_1_PART_EXTERNAL_UTF8_NAME_LEN_IN_BYTES/*cat name*/;
      
      char * nodeNamesQuery =
	new(getHeap()) char[gluedQuerySize + extraSpace + 1];
      
      str_sprintf(nodeNamesQuery, gluedQuery, param1, NULL);
      
      if (fetchAllRows(nodeNamesList_, nodeNamesQuery, 1, FALSE, rc) < 0)
	{
	  // Delete new'd characters
	  NADELETEBASIC(gluedQuery, getHeap());
	  gluedQuery = NULL;
	  
	  NADELETEBASIC(nodeNamesQuery, getHeap());
	  
	  return -1;
	}

      // Delete new'd characters
      NADELETEBASIC(gluedQuery, getHeap());
      
      NADELETEBASIC(nodeNamesQuery, getHeap());
    }

  // if no entries were found using the input param1(catalog name),
  // then create an entry using the current myNodeName.
  if (nodeNamesList_->numEntries() == 0)
    {
      OutputInfo * vi = new(getHeap()) OutputInfo(1);
      
      Lng32 len = strlen(masterGlob->getCliGlobals()->myNodeName());

      char * r = new(getHeap()) char[1+len+1];
      r[0] = '\\';
      strcpy(&r[1], masterGlob->getCliGlobals()->myNodeName());
      vi->insert(0, r);

      nodeNamesList_->insert(vi);
    }

  return 0;
}

short ExExeUtilVolatileTablesTcb::isCreatorProcessObsolete
(char * schemaName, NABoolean includesCat)
{
  Lng32 retcode = 0;

  // find process start time, node name, cpu and pin of creator process.
  short segmentNum;
  short cpu;
  pid_t pin;
  Int64 schemaNameCreateTime = 0;
  
  Lng32 currPos = 0;

  if (includesCat)
    {
      // schemaName is of the form:  <CAT>.<SCHEMA>
      // Skip the <CAT> part.
      while (schemaName[currPos] != '.')
	currPos++;
      currPos++;
    }

  currPos += 
    strlen(COM_VOLATILE_SCHEMA_PREFIX); // + strlen(COM_SESSION_ID_PREFIX);

  Int64 segmentNum_l;
  Int64 cpu_l;
  Int64 pin_l;
  Int64 sessionUniqNum;
  Lng32 userNameLen = 0;
  Lng32 userSessionNameLen = 0;
  ComSqlId::extractSqlSessionIdAttrs
    (&schemaName[currPos],
     -1, //(strlen(schemaName) - currPos),
     segmentNum_l,
     cpu_l,
     pin_l,
     schemaNameCreateTime,
     sessionUniqNum,
     userNameLen, NULL,
     userSessionNameLen, NULL);
  segmentNum = (short)segmentNum_l;
  cpu = (short)cpu_l;
  pin = (pid_t)pin_l;

  // see if process exists. If it exists, check if it is the same
  // process that is specified in the schemaName.
  short errorDetail = 0;
  Int64 procCreateTime = 0;
  retcode = ComRtGetProcessCreateTime(&cpu, &pin, &segmentNum,
				      procCreateTime,
				      errorDetail);
  if (retcode == XZFIL_ERR_OK)
  {
     // process specified in schema name exists.
     if (schemaNameCreateTime != procCreateTime)
	// but is a different process. Schema's process is obsolete.
	return -1;
     else
	// schema's process is still alive.
	return 0;
  }
  else
  { 
     if (retcode == XZFIL_ERR_NOSUCHDEV)
        return -1;
     else
        return 0;
  }
}

////////////////////////////////////////////////////////////////////////
// Redefine virtual method allocatePstates, to be used by dynamic queue
// resizing, as well as the initial queue construction.
////////////////////////////////////////////////////////////////////////
ex_tcb_private_state * ExExeUtilVolatileTablesTcb::allocatePstates(
     Lng32 &numElems,      // inout, desired/actual elements
     Lng32 &pstateLength)  // out, length of one element
{
  PstateAllocator<ExExeUtilVolatileTablesPrivateState> pa;

  return pa.allocatePstates(this, numElems, pstateLength);
}

/////////////////////////////////////////////////////////////////////////////
// Constructor and destructor for ExeUtil_private_state
/////////////////////////////////////////////////////////////////////////////
ExExeUtilVolatileTablesPrivateState::ExExeUtilVolatileTablesPrivateState()
{
}

ExExeUtilVolatileTablesPrivateState::~ExExeUtilVolatileTablesPrivateState()
{
};


////////////////////////////////////////////////////////////////
// class ExExeUtilCleanupVolatileTablesTcb
///////////////////////////////////////////////////////////////
ExExeUtilCleanupVolatileTablesTcb::ExExeUtilCleanupVolatileTablesTcb(
     const ComTdbExeUtil & exe_util_tdb,
     ex_globals * glob)
     : ExExeUtilVolatileTablesTcb( exe_util_tdb, glob),
       step_(INITIAL_),
       schemaNamesList_(NULL),
       schemaQuery_(NULL)
{
}

//////////////////////////////////////////////////////
// work() for ExExeUtilCleanupVolatileTablesTcb
//////////////////////////////////////////////////////
static const QueryString schemaCleanupQuery[] =
{
  {" select translate(rtrim(C.cat_name) || _ucs2'.' || S.schema_name using ucs2toutf8) from "},
  {" HP_SYSTEM_CATALOG.system_schema.schemata S, "},
  {" HP_SYSTEM_CATALOG.system_schema.catsys C "},
  {" where S.cat_uid = C.cat_uid "},
  {"       and S.current_operation = 'VS'"},
  {" for skip conflict access "}
};

short ExExeUtilCleanupVolatileTablesTcb::work()
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

  // Get the globals stucture of the master executor.
  ExExeStmtGlobals *exeGlob = getGlobals()->castToExExeStmtGlobals();
  ExMasterStmtGlobals *masterGlob = exeGlob->castToExMasterStmtGlobals();

  while (1)
    {
      switch (step_)
	{
	case INITIAL_:
	  {
	    step_ = FETCH_SCHEMA_NAMES_;
	  }
	break;

	case FETCH_SCHEMA_NAMES_:
	  {
	    Int32 schema_qry_array_size = 
	      sizeof(schemaCleanupQuery) 
	      / sizeof(QueryString);
	    
	    const QueryString * schemaCleanupQueryString = schemaCleanupQuery;
	    
	    char * gluedQuery;
	    Lng32 gluedQuerySize;
	    glueQueryFragments(schema_qry_array_size, 
			       schemaCleanupQueryString,
			       gluedQuery, gluedQuerySize);
	    
	    schemaQuery_ =
	      new(getHeap()) char[gluedQuerySize + 10 + 1];

	    str_sprintf(schemaQuery_, gluedQuery);
            NADELETEBASIC(gluedQuery, getMyHeap());

	    if (initializeInfoList(schemaNamesList_))
	      {
		step_ = ERROR_;
		break;
	      }

	    if (fetchAllRows(schemaNamesList_, schemaQuery_, 1, FALSE, retcode) < 0)
	      {
		/*
		ExHandleErrors(qparent_,
			       pentry_down,
			       0,
			       getGlobals(),
			       NULL,
			       (ExeErrorCode)retcode,
			       NULL,
			       NULL
			       );
			       */

		// Delete new'd characters
		NADELETEBASIC(schemaQuery_, getHeap());
		schemaQuery_ = NULL;

		step_ = ERROR_;
		break;
	      }

	    // Delete new'd characters
	    NADELETEBASIC(schemaQuery_, getHeap());
	    schemaQuery_ = NULL;

	    step_ = START_CLEANUP_;
	  }
	break;

	case START_CLEANUP_:
	  {
	    schemaNamesList_->position();
	    
	    someSchemasCouldNotBeDropped_ = FALSE;

	    if (masterGlob->getStatement()->getContext()->
		getTransaction()->xnInProgress())
	      {
		// cannot have a transaction running.
		// Return error.
		ExHandleErrors(qparent_,
			       pentry_down,
			       0,
			       getGlobals(),
			       NULL,
			       (ExeErrorCode)-EXE_BEGIN_TRANSACTION_ERROR,
			       NULL,
			       NULL
			       );
		step_ = ERROR_;
		break;
	      }

	    step_ = CHECK_FOR_OBSOLETE_CREATOR_PROCESS_;
	  }
	break;

	case CHECK_FOR_OBSOLETE_CREATOR_PROCESS_:
	  {
	    if (schemaNamesList_->atEnd())
	      {
		step_ = END_CLEANUP_;
		break;
	      }

	    OutputInfo * vi = (OutputInfo*)schemaNamesList_->getCurr();
	    char * schemaName = vi->get(0);
	    if ((cvtTdb().cleanupAllTables()) ||
		(isCreatorProcessObsolete(schemaName, TRUE)))
	      {
		// schema is obsolete, drop it.
		// Or we need to cleanup all schemas, active or obsolete.
		step_ = BEGIN_WORK_;
	      }
	    else
	      {
		schemaNamesList_->advance();
	      }
	  }
	break;

	case BEGIN_WORK_:
	  {
	    cliInterface()->beginWork();
	    step_ = DO_CLEANUP_;
	  }
	break;

	case DO_CLEANUP_:
	  {
	    OutputInfo * vi = (OutputInfo*)schemaNamesList_->getCurr();
	    char * schemaName = vi->get(0);
	    retcode =
	      dropVolatileSchema(masterGlob->getStatement()->getContext(),
				 schemaName, getHeap());
	    if (retcode < 0)
	      {
		// clear diags and move on to next schema.
		// Remember that an error was returned, we will
		// return a warning at the end.
		SQL_EXEC_ClearDiagnostics(NULL);
		retcode = 0;

		someSchemasCouldNotBeDropped_ = TRUE;
	      }

	    schemaNamesList_->advance();
	    step_ = COMMIT_WORK_;
	  }
	break;

	case COMMIT_WORK_:
	  {
	    cliInterface()->commitWork();
	    step_ = CHECK_FOR_OBSOLETE_CREATOR_PROCESS_;
	  }
	break;

	case END_CLEANUP_:
	  {
	    /*	    if (someSchemasCouldNotBeDropped_)
	      {
		// add a warning to indicate that some schemas were not
		// dropped.
		ExHandleErrors(qparent_,
			       pentry_down,
			       0,
			       getGlobals(),
			       NULL,
			       (ExeErrorCode)1069,
			       NULL,
			       NULL
			       );
	      }
	      */
	    step_ = DONE_;
	  }
	break;

	case ERROR_:
	  {
	    step_ = DONE_;
	  }
	break;

	case DONE_:
	  {
	    // Return EOF.
	    ex_queue_entry * up_entry = qparent_.up->getTailEntry();
	    
	    up_entry->upState.parentIndex = 
	      pentry_down->downState.parentIndex;
	    
	    up_entry->upState.setMatchNo(0);
	    up_entry->upState.status = ex_queue::Q_NO_DATA;
	    
	    // insert into parent
	    qparent_.up->insert();
	    
	    step_ = INITIAL_;
	    qparent_.down->removeHead();
	    
	    return WORK_OK;
	  }
	break;


	} // switch
    } // while
  
  return WORK_OK;
}

short ExExeUtilCleanupVolatileTablesTcb::dropVolatileSchema
(ContextCli * currContext,
 char * schemaName,
 CollHeap * heap)
{
  ExeCliInterface cliInterface(heap, 0, currContext);

  char * dropSchema = NULL;

  if (schemaName)
    {
      dropSchema = 
	new(heap) char[strlen("DROP VOLATILE SCHEMA CLEANUP CASCADE; ") 
		      + strlen(schemaName) + 1];
      strcpy(dropSchema, "DROP VOLATILE SCHEMA ");
      strcat(dropSchema, schemaName);
      strcat(dropSchema, " CLEANUP CASCADE;");
    }
  else
    {
      dropSchema
	= new(heap) char[strlen("DROP IMPLICIT VOLATILE SCHEMA CLEANUP CASCADE; ") + 1];
      strcpy(dropSchema, "DROP IMPLICIT VOLATILE SCHEMA CLEANUP CASCADE;");
    }
  
  // let compiler know that volatile schema could be dropped.
  // 0x8000 is the bit to allow this.
  // Bit defined in parser/SqlParserGlobalsCmn.h.
  //  currContext->setSqlParserFlags(0x8000); // ALLOW_VOLATILE_SCHEMA_CREATION

  // issue the drop schema command 
  Lng32 cliRC = cliInterface.executeImmediate(dropSchema);

  // reset volatile schema bit
  //  currContext->resetSqlParserFlags(0x8000); // ALLOW_VOLATILE_SCHEMA_CREATION

  NADELETEBASIC(dropSchema, heap);

  if (cliRC < 0)
    {
      SQL_EXEC_ClearDiagnostics(NULL);
      return (short)0; //cliRC;
    }

  return 0;
}

short ExExeUtilCleanupVolatileTablesTcb::dropVolatileTables
(ContextCli * currContext,
 CollHeap * heap)
{
  HashQueue * volTabList = currContext->getVolTabList();

  if ((! volTabList) ||
      (volTabList->numEntries() == 0))
    return 0;

  ExeCliInterface cliInterface(heap, 0, currContext);

  char * dropSchema
    = new(heap) char[strlen("DROP IMPLICIT VOLATILE SCHEMA TABLES CLEANUP CASCADE; ") + 1];
  strcpy(dropSchema, "DROP IMPLICIT VOLATILE SCHEMA TABLES CLEANUP CASCADE;");
  
  // let compiler know that volatile schema could be dropped.
  // 0x8000 is the bit to allow this.
  // Bit defined in parser/SqlParserGlobalsCmn.h.
  //currContext->setSqlParserFlags(0x8000); // ALLOW_VOLATILE_SCHEMA_CREATION

  // issue the drop schema command 
  Lng32 cliRC = cliInterface.executeImmediate(dropSchema);

  // reset volatile schema bit
  //currContext->resetSqlParserFlags(0x8000); // ALLOW_VOLATILE_SCHEMA_CREATION

  NADELETEBASIC(dropSchema, heap);

  char * sendCQD 
    = new(heap) char[strlen("CONTROL QUERY DEFAULT VOLATILE_SCHEMA_IN_USE 'FALSE';") + 1];
  strcpy(sendCQD, "CONTROL QUERY DEFAULT VOLATILE_SCHEMA_IN_USE 'FALSE';");
  cliInterface.executeImmediate(sendCQD);
  NADELETEBASIC(sendCQD, heap);

  if (cliRC < 0)
    {
      SQL_EXEC_ClearDiagnostics(NULL);
      return (short)0; //cliRC;
    }

  return 0;
}

///////////////////////////////////////////////////////////////////
// class ExExeUtilGetVolatileInfoTdb
///////////////////////////////////////////////////////////////
ex_tcb * ExExeUtilGetVolatileInfoTdb::build(ex_globals * glob)
{
  ExExeUtilGetVolatileInfoTcb * exe_util_tcb;

  exe_util_tcb = new(glob->getSpace()) ExExeUtilGetVolatileInfoTcb(*this, glob);

  exe_util_tcb->registerSubtasks();

  return (exe_util_tcb);
}

////////////////////////////////////////////////////////////////
// class ExExeUtilGetVolatileInfoTcb
///////////////////////////////////////////////////////////////
ExExeUtilGetVolatileInfoTcb::ExExeUtilGetVolatileInfoTcb(
     const ComTdbExeUtil & exe_util_tdb,
     ex_globals * glob)
     : ExExeUtilVolatileTablesTcb( exe_util_tdb, glob),
       step_(INITIAL_),
       prevInfo_(NULL),
       infoQuery_(NULL)
{
}

//////////////////////////////////////////////////////
// class ExExeUtilGetVolatileInfoTcb::work
//////////////////////////////////////////////////////

static const QueryString getAllSchemasQuery[] =
{
  {" select translate(rtrim(schema_name) using ucs2toutf8) from "},
  {" HP_SYSTEM_CATALOG.system_schema.schemata S, "},
  {" HP_SYSTEM_CATALOG.system_schema.catsys C "},
  {" where S.cat_uid = C.cat_uid "},
  {"       and S.current_operation = 'VS'"},
  {" order by 1 "},
  {" for skip conflict access "}
};

static const QueryString getAllTablesQuery[] =
{
  {" select translate(rtrim(S.schema_name) using ucs2toutf8), "}, 
  {"        object_type, "},
  {"        translate(O.object_name using ucs2toutf8) "},
  {"  from  HP_SYSTEM_CATALOG.system_schema.schemata S,  "},
  {"        HP_SYSTEM_CATALOG.system_schema.catsys C, "},
  {"        %s.HP_DEFINITION_SCHEMA.objects O "},
  {"  where S.cat_uid = C.cat_uid and "},
  {"        O.schema_uid = S.schema_uid and "},
  {"        S.current_operation = 'VS' and "},
  {"        O.object_name_space in ('TA', 'IX') and "},
  {"        O.object_security_class = 'UT' "},
  {"        for skip conflict access "},
};

static const QueryString getAllTablesInASessionQuery[] =
{
  {" select translate(S.schema_name using ucs2toutf8), "}, 
  {"        object_type, "},
  {"        translate(O.object_name using ucs2toutf8) "},
  {"  from  HP_SYSTEM_CATALOG.system_schema.schemata S,  "},
  {"        HP_SYSTEM_CATALOG.system_schema.catsys C, "},
  {"        %s.HP_DEFINITION_SCHEMA.objects O "},
  {"  where S.cat_uid = C.cat_uid and "},
  {"        O.schema_uid = S.schema_uid and "},
  {"        S.current_operation = 'VS' and "},
  {"        O.object_name_space in ('TA', 'IX') and "},
  {"        O.object_security_class = 'UT' and "},
  {"        S.schema_name like 'VOLATILE_SCHEMA_' || trim(substr('%s', 1, 42)) || '%%' "},
  {"        for skip conflict access "},
};

static const QueryString getAllSeabaseSchemasQuery[] =
{
  {" select O.schema_name from "},
  {" SEABASE.\"_MD_\".OBJECTS O "},
  {" where O.schema_name like 'VOLATILE_SCHEMA_%%' "},
  {"           and (O.object_type = 'BT' or O.object_type = 'IX') "},
  {" order by 1 "},
  {" for read uncommitted access "}
};

static const QueryString getAllSeabaseTablesQuery[] =
{
  {" select O.schema_name, O.object_type, O.object_name from "},
  {" SEABASE.\"_MD_\".OBJECTS O "},
  {" where O.schema_name like 'VOLATILE_SCHEMA_%%' "},
  {"           and (O.object_type = 'BT' or O.object_type = 'IX') "},
  {" for read uncommitted access "}
};

static const QueryString getAllSeabaseTablesInASessionQuery[] =
{
  {" select O.schema_name, O.object_type, O.object_name from "},
  {" SEABASE.\"_MD_\".OBJECTS O "},
  {" where O.schema_name like 'VOLATILE_SCHEMA_%%' "},
  {"           and (O.object_type = 'BT' or O.object_type = 'IX') "},
  {" for read uncommitted access "}
};

short ExExeUtilGetVolatileInfoTcb::work()
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

  // Get the globals stucture of the master executor.
  ExExeStmtGlobals *exeGlob = getGlobals()->castToExExeStmtGlobals();
  ExMasterStmtGlobals *masterGlob = exeGlob->castToExMasterStmtGlobals();

  while (1)
    {
      switch (step_)
	{
	case INITIAL_:
	  {
	    step_ = GET_SCHEMA_VERSION_;
	  }
	break;

	case GET_SCHEMA_VERSION_:
	  {
	    char * param1 = gviTdb().param1_;
	    char * param1PlusNodeName = NULL;
	    char * currNodeName = masterGlob->getCliGlobals()->myNodeName();
	    if (param1 && gviTdb().vtCatSpecified())
	      {
		param1PlusNodeName = new(getHeap()) 
		  char[strlen(param1) + 1 + strlen(currNodeName) + 1];
		strcpy(param1PlusNodeName, param1);
		strcat(param1PlusNodeName, "_");
		strcat(param1PlusNodeName, currNodeName);
	      }
	    
	    if (setSchemaVersion(
		 (gviTdb().vtCatSpecified() ? param1PlusNodeName : param1)))
	      {
		if (param1PlusNodeName)
		  NADELETEBASIC(param1PlusNodeName, getHeap());
		
		step_ = ERROR_;
		break;
	      }
	    
	    if (param1PlusNodeName)
	      NADELETEBASIC(param1PlusNodeName, getHeap());
	
	    step_ = GET_ALL_NODE_NAMES_;
	  }
	break;

	case GET_ALL_NODE_NAMES_:
	  {
	    // if only volatile schemas are to be retirved, then that is
	    // available in the schemata table of the local segment. No
	    // need to get all node names.
	    // Also, if 'volatile_catalog' was not specified, then get
	    // volatile tables of the local segment.
	    if (getAllNodeNames(
		 ((gviTdb().allSchemas() ||
		   (NOT gviTdb().vtCatSpecified())) ? NULL : (char *)"NEO")))
	      step_ = ERROR_;
	    else
	      {
		nodeNamesList_->position();

		if (nodeNamesList_->numEntries() == 0)
		  step_ = ERROR_;
		else
		  step_ = APPEND_NEXT_QUERY_FRAGMENT_;
	      }
	  }
	break;

	case APPEND_NEXT_QUERY_FRAGMENT_:
	  {
	    OutputInfo * vi = (OutputInfo*)nodeNamesList_->getCurr();
	    char * currNodeName = &(vi->get(0))[1]; // skip leading backslash

	    Int32 info_qry_array_size = -1;
	    const QueryString * infoQueryString = NULL;

	    // extra space to be allocated to fill with "%s" fillers
	    // in the query text.
	    Lng32 extraSpace = 0;

	    if (gviTdb().allSchemas())
	      {
		info_qry_array_size = sizeof(getAllSchemasQuery) 
		  / sizeof(QueryString);
		
		infoQueryString = getAllSchemasQuery;

		extraSpace = 2 * 10; // segment name
	      }
	    else if (gviTdb().allTables())
	      {
		info_qry_array_size = sizeof(getAllTablesQuery) 
		  / sizeof(QueryString);
		
		infoQueryString = getAllTablesQuery;

		extraSpace = 2 * 10 /*segment name*/+ ComMAX_1_PART_EXTERNAL_UTF8_NAME_LEN_IN_BYTES/*cat name*/ 
		  + 1/*separator*/ + 10/*segment name */
		   + getSchemaVersionLen();
	      }
	    else if (gviTdb().allTablesInASession())
	      {
		info_qry_array_size = sizeof(getAllTablesInASessionQuery) 
		  / sizeof(QueryString);
		
		infoQueryString = getAllTablesInASessionQuery;

		extraSpace = 2 * 10 /*segment name*/+ ComMAX_1_PART_EXTERNAL_UTF8_NAME_LEN_IN_BYTES/*cat name*/
		  + 1/*separator*/ + 10/*segment name */
		  + 128/*session id*/ + getSchemaVersionLen();
	      }

	    char * gluedQuery;
	    Lng32 gluedQuerySize;
	    glueQueryFragments(info_qry_array_size, 
			       infoQueryString,
			       gluedQuery, gluedQuerySize);
	    
	    char * iq =
	      new(getHeap()) char[gluedQuerySize + extraSpace + 1];

	    char * param1 = gviTdb().param1_;
	    char * param2 = gviTdb().param2_;

	    char * param1PlusNodeName = NULL;
	    if (param1 && gviTdb().vtCatSpecified())
	      {
		param1PlusNodeName = new(getHeap()) 
		  char[strlen(param1) + 1 + strlen(currNodeName) + 1];
		strcpy(param1PlusNodeName, param1);
		strcat(param1PlusNodeName, "_");
		strcat(param1PlusNodeName, currNodeName);
	      }

	    str_sprintf(iq, gluedQuery, 
			currNodeName, currNodeName,
			(param1PlusNodeName ? param1PlusNodeName : param1), 
			getSchemaVersion(), param2);

	    // Delete new'd characters
	    NADELETEBASIC(gluedQuery, getHeap());
	    gluedQuery = NULL;

	    Lng32 newLen = 0;
	    if (infoQuery_ == NULL)
	      {
		newLen = strlen(iq);
	      }
	    else
	      {
		newLen = strlen(infoQuery_) + strlen(iq);
	      }

	    nodeNamesList_->advance();
	    if (NOT nodeNamesList_->atEnd())
	      {
		newLen += strlen(" union ");
	      }
	    else
	      {
		if ((gviTdb().allTables()) ||
		    (gviTdb().allTablesInASession()))
		  {
		    newLen += strlen(" order by 1,2 desc ");
		  }

		newLen += 1; // for trailing semicolon
	      }
	    
	    newLen++; // for null terminator

	    char * newIq = new(getHeap()) char[newLen];
	    
	    if (infoQuery_ == NULL)
	      {
		strcpy(newIq, iq);
		
	      }
	    else
	      {
		strcpy(newIq, infoQuery_);
		strcat(newIq, iq);

		NADELETEBASIC(infoQuery_, getHeap());
	      }

	    if (NOT nodeNamesList_->atEnd())
	      {
		strcat(newIq, " UNION ");
	      }
	    else
	      {
		if ((gviTdb().allTables()) ||
		    (gviTdb().allTablesInASession()))
		  {
		    strcat(newIq, " order by 1,2 desc ");
		  }

		strcat(newIq, ";");
	      }

	    infoQuery_ = newIq;

            NADELETEBASIC(iq, getHeap());

	    if (nodeNamesList_->atEnd())
	      step_ = FETCH_ALL_ROWS_;
	  }
	break;

	case FETCH_ALL_ROWS_:
	  {
	    Lng32 numOutputEntries = 0;

	    if (gviTdb().allSchemas())
	      {
		numOutputEntries = 1;
	      }
	    else if (gviTdb().allTables())
	      {
		numOutputEntries = 3;
	      }
	    else if (gviTdb().allTablesInASession())
	      {
		numOutputEntries = 3;
	      }

	    if (initializeInfoList(infoList_))
	      {
		step_ = ERROR_;
		break;
	      }

	    if (fetchAllRows(infoList_, infoQuery_, numOutputEntries, FALSE, retcode) < 0)
	      {
		step_ = ERROR_;

		NADELETEBASIC(infoQuery_, getHeap());
		infoQuery_ = NULL;

		break;
	      }

            NADELETEBASIC(infoQuery_, getHeap());
	    infoQuery_ = NULL;

	    infoList_->position();
	    
	    if (gviTdb().allSchemas())
	      {
		step_ = RETURN_ALL_SCHEMAS_;
	      }
	    else if (gviTdb().allTables())
	      {
		step_ = RETURN_ALL_TABLES_;
	      }
	    else if (gviTdb().allTablesInASession())
	      {
		step_ = RETURN_TABLES_IN_A_SESSION_;
	      }
	    else
	      step_ = ERROR_;
	  }
	break;

	case RETURN_ALL_SCHEMAS_:
	  {
	    if (infoList_->atEnd())
	      {
		step_ = DONE_;
		break;
	      }

	    if (qparent_.up->isFull())
	      return WORK_OK;

	    char outBuf[400+ComMAX_3_PART_EXTERNAL_UTF8_NAME_LEN_IN_BYTES];

	    OutputInfo * vi = (OutputInfo*)infoList_->getCurr();
	    char * schemaName = vi->get(0);

	    char state[10];
	    if (isCreatorProcessObsolete(schemaName, FALSE))
	      strcpy(state, "Obsolete");
	    else
	      strcpy(state, "Active  ");
	    
	    str_sprintf(outBuf, "Schema(%8s): %s", state, schemaName);
	    moveRowToUpQueue(outBuf);

	    infoList_->advance();
	  }
	break;

	case RETURN_ALL_TABLES_:
	case RETURN_TABLES_IN_A_SESSION_:
	  {
	    if (infoList_->atEnd())
	      {
		step_ = DONE_;
		break;
	      }

	    if ((qparent_.up->getSize() - qparent_.up->getLength()) < 4)
	      return WORK_CALL_AGAIN;

	    char outBuf[400+ComMAX_3_PART_EXTERNAL_UTF8_NAME_LEN_IN_BYTES];

	    OutputInfo * vi = (OutputInfo*)infoList_->getCurr();

	    char * schemaName = vi->get(0);
            if ((! prevInfo_) ||
		(strcmp(prevInfo_->get(0), schemaName) != 0))
	      {
		char state[10];
		if (isCreatorProcessObsolete(schemaName, FALSE))
		  strcpy(state, "Obsolete");
		else
		  strcpy(state, "Active  ");

		str_sprintf(outBuf, "Schema(%8s): %s", state, schemaName);
		moveRowToUpQueue(outBuf);

		prevInfo_ = (OutputInfo*)infoList_->getCurr();
	      }

	    char objectType[6];
	    if (strcmp(vi->get(1), "BT") == 0)
	      strcpy(objectType, "Table");
	    else if (strcmp(vi->get(1), "IX") == 0)
	      strcpy(objectType, "Index");
	    
	    str_sprintf(outBuf, "  %5s: %s",
			objectType, vi->get(2));
	    moveRowToUpQueue(outBuf);
	    
	    infoList_->advance();
	  }
	break;

	case ERROR_:
	  {
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
	    
	    step_ = INITIAL_;
	    qparent_.down->removeHead();
	    
	    return WORK_OK;
	  }
	break;


	} // switch
    } // while
  
  return WORK_OK;
}
