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
* File:         Statement.C
* Description:  Methods for class Statement.
*
* Created:      7/10/95
* Language:     C++
*
*
*
*
*****************************************************************************
*/

#define _XOPEN_SOURCE_EXTENDED 1
#include "cli_stdh.h"
#undef _XOPEN_SOURCE_EXTENDED       

#include "ComCextdecs.h"
#include "ex_stdh.h"
#include "ex_exe_stmt_globals.h"
#include "ComTdb.h"
#include "ex_tcb.h"
#include "ex_root.h"
#include "ExStats.h"
#include "ExSqlComp.h"
#include "ex_transaction.h"
#include "ex_frag_rt.h"
#include "ComDiags.h"
#include "NAMemory.h"
#include "LateBindInfo.h"
#include "sql_buffer.h"
#include "ex_control.h"
#include "Descriptor.h"  // For call to Descriptor::getCharDataFromCharHostVar(). 
#include "exp_clause_derived.h"
#include "sql_id.h"
#include "ex_error.h"
#include "ComRtUtils.h"
#include "ComDistribution.h"
#include "Cli.h"
#include "Int64.h"
#include "ComSqlId.h"
#include "CmpErrors.h"

#include "TriggerEnable.h" // triggers
#include "ComSmallDefs.h" // MV
#include "ComMvAttributeBitmap.h" // MV

#include "logmxevent.h"

#include "ExpLOBinterface.h"

#include "ExUdrServer.h"
#include "wstr.h"
#include "QueryText.h"
#include <wchar.h>

#include "ComAnsiNamePart.h"
#include "ExRsInfo.h"

#include "arkcmp_proc.h"
#include "CmpContext.h"

#include "HiveClient_JNI.h"

// Printf-style tracing macros for the debug build. The macros are
// no-ops in the release build.
#ifdef _DEBUG
#include <stdarg.h>
#define StmtDebug0(s) StmtPrintf((s))
#define StmtDebug1(s,a1) StmtPrintf((s),(a1))
#define StmtDebug2(s,a1,a2) StmtPrintf((s),(a1),(a2))
#define StmtDebug3(s,a1,a2,a3) StmtPrintf((s),(a1),(a2),(a3))
#define StmtDebug4(s,a1,a2,a3,a4) StmtPrintf((s),(a1),(a2),(a3),(a4))
#define StmtDebug5(s,a1,a2,a3,a4,a5) StmtPrintf((s),(a1),(a2),(a3),(a4),(a5))
const char *TransIdToText(Int64 transId)
{
  static char text[256];
  short actualLen;
  short error = TRANSIDTOTEXT(transId, text, 255, &actualLen);
  if (error)
    str_sprintf(text, "(error %d)", (Int32) error);
  else
    text[actualLen] = 0;
  return &text[0];
}
#else
#define StmtDebug0(s)
#define StmtDebug1(s,a1)
#define StmtDebug2(s,a1,a2)
#define StmtDebug3(s,a1,a2,a3)
#define StmtDebug4(s,a1,a2,a3,a4)
#define StmtDebug5(s,a1,a2,a3,a4,a5)
#endif

const char *RetcodeToString(RETCODE r)
{
  switch (r)
  {
    case SUCCESS: return "SUCCESS";
    case SQL_EOF: return "SQL_EOF";
    case ERROR: return "ERROR";
    case WARNING: return "WARNING";
    case NOT_FINISHED: return "NOT_FINISHED";
    default: return ComRtGetUnknownString((Int32) r);
  }
}

class Dealloc
{
  NAHeap *heap_;
  void * addr_;
public:
  Dealloc() : heap_(NULL), addr_(NULL) {}
  ~Dealloc() { if (addr_ != NULL) heap_->deallocateMemory(addr_); }
  NAHeap * setHeap(NAHeap *heap) { return heap_ = heap; }
  void * getAddr(void *addr) { return addr_ = addr; }
};

////////////////////////////////////////////////////////////////////
// class StatementInfo
////////////////////////////////////////////////////////////////////
StatementInfo::StatementInfo()
{
  statement_  = NULL;
  inputDesc_  = NULL;
  outputDesc_ = NULL;
  hashValue_  = 0;
  flags_      = 0;
};

StatementInfo::~StatementInfo()
{
  statement_  = NULL;
  inputDesc_  = NULL;
  outputDesc_ = NULL;
  hashValue_  = 0;
  flags_      = 0;
};

// This function is an internal test mechanism for SPJ result sets
static const char *getProxySyntaxFromEnvironment(CliGlobals *cliGlobals,
                                                 Lng32 rsIndex)
{
  char *stmtText = NULL;


  return stmtText;
}

////////////////////////////////////////////////////////////////////
// class Statement
////////////////////////////////////////////////////////////////////
Statement::Statement(SQLSTMT_ID * statement_id_,
		     CliGlobals * cliGlobals,
                     StatementType stmt_type_,
                     char * cn, Module *module)
  : stmt_state(INITIAL_),
    cliGlobals_(cliGlobals),
    context_(cliGlobals->currContext()),
    clonedFrom_(NULL),
    parentCall_(NULL),
    heap_("Statement Heap",
          context_->exHeap(), 
	  8096  /* 20480 */ /* block size */, 
	  0     /* upperLimit */),
    prevCloseStatement_(NULL),
    nextCloseStatement_(NULL),
    closeSequence_((stmt_type_ == STATIC_STMT) ? 0 : -1)
  , holdable_(SQLCLIDEV_NONHOLDABLE)
  , statementIndex_(0)
  , module_(module)
  , inputArrayMaxsize_(0)   // compile time info for dynamic rowsets
  , rowsetAtomicity_(UNSPECIFIED_)
  , notAtomicFailureLimit_(0)
  , anyTransWasStartedByMe_(FALSE)
  , autoCommitCleared_(FALSE)
  , savedRoVal_(TransMode::READ_WRITE_ )
  , savedRbVal_ (TransMode::ROLLBACK_MODE_NOT_SPECIFIED_ )
  , savedAiVal_( -1 )	       
  , udrSecurity_ (NULL)
  , versionOnEntry_ (COM_VERS_UNKNOWN)
  , versionToUse_ (COM_VERS_UNKNOWN)
  , fetchErrorCode_ (VERSION_NO_ERROR)
  , mxcmpErrorCode_ (VERSION_NO_ERROR)
  , stmtStats_(NULL)
  , extractConsumerQueryTemplate_(NULL)
  , stmtInfo_(NULL)
  , aqrStmtInfo_(NULL)
  , state_(INITIAL_STATE_)  
  , compileStatsArea_(NULL) 
  , spaceObject_(new(context_->exHeap())
    Space(Space::EXECUTOR_SPACE, TRUE, (char *)"Stmt Space"))
  , space_(*spaceObject_)
  , aqrInitialExeStartTime_(-1)
  , compileEndTime_(-1)
{
  cliLevel_ = context_->getNumOfCliCalls();

#ifdef _DEBUG
  stmtDebug_ = FALSE;
  stmtListDebug_ = FALSE;
  Lng32 numCliCalls = context_->getNumOfCliCalls();

  // We have two printf-style trace mechanisms in the debug build
  //
  // 1. A detailed account of all significant events in the life
  //    of this statement. Enabled by the STMT_DEBUG environment
  //    variable.
  //
  // 2. An account of when this is statement is added to or removed
  //    from any of the context's statement lists. Enabled by the
  //    STMTLIST_DEBUG or STMT_DEBUG environment variable. The code
  //    for this trace is in the ContextCli class. All we do here in
  //    the Statement class is carry a flag indicating whether events
  //    for this instance should be reported.

  // Each trace is enabled on a per-instance basis. We check the
  // environment variables every time through the Statement
  // constructor. Also note that we do NOT enable either trace for
  // statements created internally by SQL/MX code. For those
  // statements, the numCliCalls variable will be greater than one.

  // The commands to create trace output are the StmtDebugX macros
  // above in this source file. In the release build the macros
  // evaluate to a noop.

  if (numCliCalls == 1)
  {
    char *envVar;
    envVar = getenv("STMT_DEBUG");
    if (envVar && envVar[0])
    {
      stmtDebug_ = TRUE;
      stmtListDebug_ = TRUE;
    }
    else
    {
      envVar = getenv("STMTLIST_DEBUG");
      if (envVar && envVar[0])
      {
        stmtListDebug_ = TRUE;
      }
    }
  }
#endif

  StmtDebug1("[BEGIN constructor] %p", this);
  StmtDebug2("  Context address %p, context handle %d",
             context_, (Lng32) context_->getContextHandle());

  clonedStatements = new(&heap_) Queue(&heap_);
  // for now a statement space is allocated from the statement heap 
  space_.setParent(&heap_);

  // Set up a space object which might be used during unpacking to allocate
  // additional space for potential upgrading of objects in the plan. This
  // space is derived from heap_, and therefore goes away with it when the
  // statement is destroyed.
  // 
  unpackSpace_ = new(&heap_) Space();
  unpackSpace_->setType(Space::EXECUTOR_SPACE);
  unpackSpace_->setParent(&heap_);

  stmt_type = stmt_type_;

  // Allocate statement-id
  statement_id = (SQLSTMT_ID *)(heap_.allocateMemory(sizeof(SQLSTMT_ID)));
  init_SQLCLI_OBJ_ID(statement_id);

  SQLMODULE_ID* new_module =
       (SQLMODULE_ID *)(heap_.allocateMemory(sizeof(SQLMODULE_ID)));
  init_SQLMODULE_ID(new_module);

  statement_id->module = new_module;
  statement_id->identifier = 0;
  statement_id->handle = 0;
  statement_id->name_mode = statement_id_->name_mode;

  const SQLMODULE_ID* old_module = statement_id_-> module; 

  if(statement_id_->module->module_name)
    {
      Lng32 old_module_nm_len = (Lng32) getModNameLen(old_module);

      char * mn = (char *)(heap_.allocateMemory(old_module_nm_len + 1));
      str_cpy_all(mn,
             statement_id_->module->module_name, old_module_nm_len);

      new_module->module_name_len = old_module_nm_len;
      mn[old_module_nm_len] = 0;
      new_module->module_name = mn;
      StmtDebug1("  Module name: \"%s\"", mn);
    }

  if(statement_id_->identifier)
    {
      Lng32 stmt_name_len = (Lng32) getIdLen(statement_id_);
      char * id =
        (char *)(heap_.allocateMemory(stmt_name_len + 1));

      str_cpy_all(id, statement_id_->identifier, 
		  stmt_name_len);
      statement_id->identifier_len = stmt_name_len;
      id[stmt_name_len] = 0;
      statement_id->identifier = id;
    }
  
  if (context_->aqrInfo() && 
      context_->aqrInfo()->aqrStmtInfo() && 
      (context_->aqrInfo()->aqrStmtInfo()->getSavedStmtHandle() != 0))      
    statement_id->handle=context_->aqrInfo()->aqrStmtInfo()->getSavedStmtHandle();
  else  
    statement_id->handle=(void*)getContext()->getNextStatementHandle();
  StmtDebug1("  New statement handle: %p", statement_id->handle);
    
  default_input_desc = 0;
  default_output_desc = 0;
  
  cursor_name_ = NULL;

  switch (statement_id->name_mode)
  {
  case stmt_handle:
    // nothing to handle for now...
    break;

  case stmt_name:
    // nothing to handle for now...
    break;

  case cursor_name:
    if (cn)
      setCursorName(cn);
    else
      setCursorName(statement_id->identifier);
    break;

  case stmt_via_desc:
  case curs_via_desc:
    {
    SQLDESC_ID tmpDescId;
    if (statement_id->name_mode == stmt_via_desc)
    {
   	init_SQLDESC_ID(&tmpDescId, 
		SQLCLI_CURRENT_VERSION,
		stmt_via_desc,
		statement_id->module,
		statement_id->identifier,
		0,
		SQLCHARSETSTRING_ISO88591,
		(Lng32) getIdLen(statement_id)
    		);
     }
    else
       {
	init_SQLDESC_ID(&tmpDescId,
        SQLCLI_CURRENT_VERSION,
        curs_via_desc,
        statement_id->module,
        statement_id->identifier,
        0,
        SQLCHARSETSTRING_ISO88591,
        (Lng32) getIdLen(statement_id)
        );
	}

    // get the value of the name from the descriptor
    SQLCLI_OBJ_ID* tmpObjId = Descriptor::GetNameViaDesc(&tmpDescId,
           context_,heap_);

    // now that we have used the statement_id->identifier to get the
    // descriptor, we can get rid of it as it won't be used again...
    // NOTE: init_SQLDESC_ID() doesn't make a copy of the identifier so
    //  it isn't safe to free statement_id->identifier until after
    //  the call to GetNameViaDesc().
    if (statement_id->identifier)
    {
      heap_.deallocateMemory((char*)statement_id->identifier);
      statement_id->identifier = 0;
    }

    if (tmpObjId) 
    {
      statement_id->identifier_len = (Lng32) getIdLen(tmpObjId);
      char * id =
        (char *)(heap_.allocateMemory(statement_id->identifier_len + 1));
      if (tmpObjId->identifier) {
        str_cpy_all(id, tmpObjId->identifier,
                    statement_id->identifier_len);
        id[statement_id->identifier_len] = 0;
        // fix memory leak (genesis case 10-981230-3244) 
        // caused by not freeing a non-null tmpString.
        heap_.deallocateMemory((char*)tmpObjId->identifier);
	statement_id->identifier = id;
      }
    
    else
     {
       statement_id->identifier =0;
       statement_id->identifier_len = 0;
     } 
      heap_.deallocateMemory(tmpObjId);
    }

    if(statement_id->identifier == 0)
    {
        // probably an error
    }

    if (statement_id_->name_mode == curs_via_desc)
    {
    // now that the cursor name is set... we don't look in the descriptor
    // again.  The name_mode is now cursor_name *NOT* curs_via_desc.

        setCursorName(statement_id->identifier);
        statement_id->name_mode = cursor_name;
    }
    else
    {
    // now that the statement name is set... we don't look in the descriptor
    // again.  The name_mode is now stmt_name *NOT* stmt_via_desc.

        cursor_name_ = 0;
        statement_id->name_mode = stmt_name;
    }
    }
    break;

  default:
    // error
    break;
  }

#ifdef _DEBUG
  switch (statement_id->name_mode)
  {
    case stmt_handle:
      StmtDebug0("  Name mode: stmt_handle");
      break;

    case stmt_name:
      StmtDebug0("  Name mode: stmt_name");
      StmtDebug1("  Statement name: \"%s\"", statement_id->identifier);
      break;

    case cursor_name:
      StmtDebug0("  Name mode: cursor_name");
      StmtDebug1("  Cursor name: \"%s\"", statement_id->identifier);
      break;

    default:
      StmtDebug1("  *** UNKNOWN NAME MODE %d",
                 (Lng32) statement_id->name_mode);
      break;
  }
#endif

  if (statement_id_->version > SQLCLI_STATEMENT_VERSION_1)
    statement_id->tag = statement_id_->tag; 
  else
    statement_id->tag = 0;
  
  currentOfCursorStatement_ = NULL;

  source_str = 0;
  source_length = 0;
  charset_ = SQLCHARSETCODE_ISO88591;

  schemaName_ = NULL;
  schemaNameLength_ = 0;

  recompControlInfo_ = NULL;
  recompControlInfoLen_ = 0;

  root_tdb = 0;
  root_tcb = 0;
  root_tdb_size = 0;

  lnil_ = NULL;
  inputData_ = NULL;
  inputDatalen_ = 0;


  uniqueStmtId_ = NULL;
  uniqueStmtIdLen_ = 0;
  parentQid_ = NULL;
  parentQidSystem_[0] = '\0';

  if (stmt_type_ == STATIC_STMT)
    // STATIC statement. These are already 'prepared'.
    // They are in Close Cursor state.
    setState(CLOSE_);
  
  flags_ = 0;

  setComputeBulkMoveInfo(TRUE);

  statementGlobals_ = 
    new(&heap_) ExMasterStmtGlobals(0, /* fixup will add the real number */
                                    cliGlobals_,
                                    this,
                                    1 /* create gui scheduler */,
                                    &space_,
                                    &heap_);

  // stored the type in statementGlobals
  if(stmt_type_ == STATIC_STMT){
    statementGlobals_->setStmtType(ExExeStmtGlobals::STATIC);
  }

  if (stmt_type == DYNAMIC_STMT)
    aqrStmtInfo_ = new(&heap_) AQRStatementInfo(&heap_);
  else
    aqrStmtInfo_ = NULL;

  StmtDebug1("[END constructor] %p", this);
  childQueryId_ = NULL;
  childQueryIdLen_ = 0;
  childQueryCostInfo_ = NULL;
  childQueryCompStatsInfo_ = NULL;
} // Statement::Statement

Statement::~Statement()
{
  StmtDebug1("[BEGIN destructor] %p", this);

  dealloc();
  if (compileStatsArea_ != NULL)
  {
    NADELETE(compileStatsArea_, ExStatisticsArea, compileStatsArea_->getHeap());
    compileStatsArea_ = NULL;
  }
  if (statement_id)
    {
      if (statement_id->module)
	{
	  if (statement_id->module->module_name)
	    {
	      heap_.deallocateMemory((char*)statement_id->module->module_name);
	    }
	  heap_.deallocateMemory((char*)statement_id->module);
	  statement_id->module = 0;
	}
      if (statement_id->identifier)
	{
	  heap_.deallocateMemory((char*)statement_id->identifier);
	  statement_id->identifier = 0;
	}
      heap_.deallocateMemory(statement_id);
      statement_id = 0;
    }
  
  if (cursor_name_)
    {
      heap_.deallocateMemory(cursor_name_);
      cursor_name_ = 0;
    }
  
  if (source_str)
    {
      NADELETEBASIC(source_str, context_->exHeap());

      //      heap_.deallocateMemory(source_str);
      source_str = NULL;
    }
  
  // Release the TDB tree. assignRootTdb() does take into account
  // statement cloning and will not actually deallocate memory
  // occupied by the TDB tree if this statement is a clone.
  assignRootTdb(NULL);

  // Delete cloned statements
  // Note: Context should be used to deallocate cloned statements
  //       if the original statement being cloned from is deallocated...
  clonedStatements->position();
  Statement *clone = (Statement *) clonedStatements->getNext();
  for (; clone != NULL; clone = (Statement *) clonedStatements->getNext())
  {
    delete clone;
  }
  
  NADELETE(clonedStatements, Queue, &heap_); 
  
  // Delete child stored procedure result set statements. Note that
  // the context has already removed these child statements from its
  // statement lists before calling this Statement destructor.
  ExRsInfo *rsInfo = getResultSetInfo();
  if (rsInfo)
  {
    ULng32 numChildren = rsInfo->getNumEntries();
    for (ULng32 i = 1; i <= numChildren; i++)
    {
      Statement *rsChild = NULL;
      const char *proxySyntax = NULL;
      NABoolean open = FALSE;
      NABoolean closed = FALSE;
      NABoolean prepared = FALSE;
      NABoolean found = rsInfo->getRsInfo(i, rsChild, proxySyntax,
                                          open, closed, prepared); 
      if (found && rsChild)
        delete rsChild;
    }

    // All the child statements have gone away and this statement is
    // about to go away. We can destroy the ExRsInfo object that was
    // storing information about the children.
    statementGlobals_->deleteResultSetInfo();
  }

  // If this is a stored procedure result set, inform the parent that
  // this child is going away
  if (parentCall_)
  {
    ExRsInfo *rsInfo = parentCall_->getResultSetInfo();
    if (rsInfo)
    {
      StmtDebug1("  About to unbind proxy statement %p ", this);
      rsInfo->unbind(this);
    }
  }

  if (default_input_desc)
    {
      delete default_input_desc;    
      default_input_desc = 0;
    }
  
  if (default_output_desc)
    {
      delete default_output_desc;
      default_output_desc = 0;
    }

  if ( udrSecurity_ )
    {
      if ( udrSecurity_->entries ())
	{
	  for ( UInt32 i=0; i < udrSecurity_->entries (); i++)
	    {
	      delete (*udrSecurity_)[i];
	    }
	    udrSecurity_->clear ();
        } // if udrSecurity_->entries ()
  
      delete udrSecurity_;
      udrSecurity_ = 0;
    } // if udrSecurity_

  StmtDebug1("[END destructor] %p", this);

  if (cliGlobals_->getStatsGlobals() != NULL)
  {
    if (stmtStats_ != NULL)
    {
      int error = cliGlobals_->getStatsGlobals()->getStatsSemaphore(cliGlobals_->getSemId(),
                    cliGlobals_->myPin());
      if (stmtStats_->getMasterStats() != NULL)
	{
	  stmtStats_->getMasterStats()->setStmtState(Statement::DEALLOCATED_);
	  stmtStats_->getMasterStats()->setEndTimes(! stmtStats_->aqrInProgress());
	}
      cliGlobals_->getStatsGlobals()->removeQuery(cliGlobals_->myPin(), stmtStats_);
      cliGlobals_->getStatsGlobals()->releaseStatsSemaphore(cliGlobals_->getSemId(),cliGlobals_->myPin());
    }
  }
  else
  {
    if (stmtStats_ != NULL)
    {
      NADELETE(stmtStats_, StmtStats, stmtStats_->getHeap());
    }
  }
  stmtStats_ = NULL;
  if (childQueryId_ != NULL)
  {
    NADELETEBASIC(childQueryId_, &heap_);
    childQueryId_ = NULL;
  }
  if (childQueryCostInfo_ != NULL)
  {
    NADELETEBASIC(childQueryCostInfo_, &heap_);
    childQueryCostInfo_ = NULL;
  }
  if (childQueryCompStatsInfo_ != NULL)
  {   
    NADELETEBASIC(childQueryCompStatsInfo_, &heap_);
    childQueryCompStatsInfo_ = NULL;
  }
  if (spaceObject_ != NULL)
  {
    delete spaceObject_;
    spaceObject_ = NULL;
  }
} // Statement::~Statement

NABoolean Statement::updateInProgress ()
{
  // if there are incomplete insert, delete, update or prepare operations,
  // return TRUE, otherwise return FALSE.
  if (!isPubsubHoldable() 
      )
  {
   if (((getState() == Statement::OPEN_ || 
        getState() == Statement::FETCH_ ||
        getState() == Statement::RELEASE_TRANS_) &&
	(getRootTdb() &&
        (getRootTdb()->updDelInsertQuery() ||
	 getRootTdb()->isEmbeddedUpdateOrDelete()) )
        ) )
     return TRUE;
   }
  return FALSE;
}  

// releaseTransaction is called by Statement::close, Statement::dealloc,
// ContextCli::releaseAllTransactionalRequests and 
// ContextCli::removeFromOpenstatementList.

RETCODE Statement::releaseTransaction(NABoolean allWorkRequests,
				      NABoolean alwaysSendReleaseMsg,
                                      NABoolean statementRemainsOpen)
{
  
  StmtDebug2("[BEGIN releaseTransaction] %p, allWorkRequests = %s",
             this, (allWorkRequests ? "TRUE" : "FALSE"));
  
  // do not send new DML request. 
  statementGlobals_->setNoNewRequest(TRUE);

  if (!statementRemainsOpen               &&  // is holdable cursor?
      root_tcb                            &&
      root_tcb->needsDeregister())
    root_tcb->deregisterCB();

  Int64 cbWaitStartTime = NA_JulianTimestamp();

  statementGlobals_->setNoNewRequest(FALSE);
    
  ExRtFragTable *fragTable = statementGlobals_->getRtFragTable();
  if (fragTable)
    {
      if (root_tcb && root_tcb->fatalErrorOccurred())
        {
          // We can easily get deadlocks from the ESPs at this
          // point, due to producer ESPs' ex_split_bottom_tcb::
          // workState_ set to WAIT_FOR_MORE_REQUESTORS --
          // see solution 10-070108-1544.  This entire statement
          // will soon be deallocated, so place all IpcConnections
          // with pending I/O into the error state.
          fragTable->abandonPendingIOs();
        }
      else
        {
          // For the ESPs, send out the word that we want to get out
          // of the transaction (maybe just temporarily)
	  fragTable->releaseTransaction(allWorkRequests,
					alwaysSendReleaseMsg);
	  
          // Note that for the cancel broker, we have already sent the 
          // finish message -- see the call to deregisterCB in this 
          // method above.
	  
          // wait until the messages have come back
          // (that's all messages if allWorkRequests is set to TRUE,
          // and only the transactional msges if it is FALSE)

	  // how long should we wait for release work reply from esps?
	  // upon timeout we will abort all outstanding I/Os.
	  // some facts below:
	  //
	  //   - default timeout is 15 minutes
	  //   - minimum wait timeout is 5 minutes
	  //   - timeout = -1 means wait indefinitely. but every 2 minutes
	  //     write a timeout message to EMS log.
	  //   - timeout = -2 means that we time out after 2 minutes, then
	  //     abend up to 8 esps that are not responding so we can collect
	  //     saveabend files for debug purpose.
	  //
	  bool waitForever = false;
	  bool abendEsps = false;
	  IpcTimeout timeout = context_->getSessionDefaults()->
                                                getEspReleaseWorkTimeout();
	  if (timeout <= 0)
	    {
	      if (timeout == -1)
		waitForever = true;
	      else if (timeout == -2)
		abendEsps = true;

	      timeout = 2*6000; // 2 minutes
	    }
	  else if (timeout < 5*60)
	    {
	      // minimum timeout is 5 minutes
	      timeout = 5*6000;
	    }
	  else
	    {
	      timeout *= 100;
	    }

          NABoolean hasTimedout;
          while (fragTable->hasOutstandingTransactionalMsges() ||
		 (allWorkRequests && fragTable->hasOutstandingWorkRequests()))
            {
	      if (fragTable->getState() == ExRtFragTable::ERROR)
		{
		  // some error occured
		  context_->diags() << DgSqlCode(-EXE_INTERNAL_ERROR);
		  ComDiagsArea *diagsPtr = statementGlobals_->getDiagsArea();
		  if (diagsPtr)
		    context_->diags().mergeAfter(*diagsPtr);

		  // abort all outstanding I/Os
		  fragTable->abandonPendingIOs();
		  break;
		}

              // wait for an I/O to complete, we know there are some
              // transaction requests that should complete...
              statementGlobals_->getIpcEnvironment()->getAllConnections()->
	        waitOnAll(timeout, FALSE, &hasTimedout);

              if (hasTimedout)
                {
                  // timed out waiting for esp reply. something is wrong.
                  char errMsg[200];
                  str_sprintf(errMsg, "Statement still has %d connections with I/O outstanding - %d trans msgs and %d work reqs.",
			      statementGlobals_->getIpcEnvironment()
                              ->getAllConnections()->getNumPendingIOs(),
			      fragTable->numOutstandingTransactionalMsges(),
			      fragTable->numOutstandingWorkRequests());

		  if (!waitForever)
		    {
		      context_->diags() << DgSqlCode(-EXE_RELEASE_WORK_TIMEOUT)
					<< DgString0(errMsg);

		      if (abendEsps)
			{
			  // abend up to 8 esps so we can collect saveabend
			  // files for debug purpose.
			  CollIndex numEsps = 8;
			  GuaProcessHandle phandles[8];
			  statementGlobals_->getIpcEnvironment()->getAllConnections()->
			    fillInListOfPendingPhandles(phandles, numEsps);

			  char logMsg[300];
			  char *initMsg= (char *)" Executor abended non-responding ESPs ";
			  memset(logMsg, '\0', sizeof logMsg);
			  str_cat(logMsg, initMsg, logMsg);
			  for (CollIndex i = 0; i < numEsps; i++)
			    {
                              // abend esp with saveabend file
                              phandles[i].dumpAndStop(true, true);
			      if (i > 0)
				str_cat(logMsg, ", ", logMsg);
			      phandles[i].toAscii(logMsg + str_len(logMsg), 300 - str_len(logMsg));
			    }
			  str_cat(logMsg, ".", logMsg);

			  context_->diags() << DgString1(logMsg);
			} // if (abendEsps)

		      // abort all outstanding I/Os
		      fragTable->abandonPendingIOs();
		      break;
		    } // if (!waitForever)
		  else
		    {
		      // wait forever. log a warning msg and go back to wait.
		      char logMsg[500];
		      const char *initMsg="Query looping while waiting for release work reply from ESPs ";
		      logMsg[0] = '\0';
		      str_cat(logMsg, initMsg, logMsg);
		      // pins of waited processes are concatenated to logMsg
		      // each pin should be 28 bytes or less
		      statementGlobals_->getIpcEnvironment()->getAllConnections()->
			fillInListOfPendingPins(logMsg + str_len(initMsg),
						300 - str_len(initMsg), 8);
		      str_cat(logMsg, ". ", logMsg);
		      str_cat(logMsg, errMsg, logMsg);
		      SQLMXLoggingArea::logExecRtInfo("Statement.cpp", 0, logMsg, 0);
		    }
                } // if (hasTimedout)
	      else
		{
		  // we did not timeout because some I/O completed first.
		  // now do some work on the fragment dir.
		  if (fragTable->getState() != ExRtFragTable::ERROR)
		    {
		      fragTable->workOnRequests();
		    }
		  else
		    {
		      // some error occured
		      context_->diags() << DgSqlCode(-EXE_INTERNAL_ERROR);
		      ComDiagsArea *diagsPtr = statementGlobals_->getDiagsArea();
		      if (diagsPtr)
			context_->diags().mergeAfter(*diagsPtr);

		      // abort all outstanding I/Os
		      fragTable->abandonPendingIOs();
		      break;
		    }
		}
            } // while (..)
        } // if (root_tcb && ..)
    } // if (fragTable)

  // In most case, the wait loop above will ensure that cancel broker messages
  // are now complete.  However, if there is no fragTable, or there was
  // fatalErrorOccurred or if in the wait loop some problem happened that
  // caused the loop to exit early, then here we must wait for cancel broker
  // to reply to start  and finish message.
  if (root_tcb && !statementRemainsOpen)
    root_tcb->cbMessageWait(cbWaitStartTime);

  // If there is an error either in Context diagsArea or
  // in StatementGlobals_ diagsArea, do not update the end time
  ComDiagsArea *diagsArea = context_->getDiagsArea();
  if (diagsArea->mainSQLCODE() >= 0 && stmtStats_ != NULL &&
    stmtStats_->getMasterStats() != NULL && (! stmtStats_->aqrInProgress())
      && stmtStats_->getMasterStats()->getExeEndTime() == -1)
  {
    Int64 exeEndTime = NA_JulianTimestamp();
    stmtStats_->getMasterStats()->setExeEndTime(exeEndTime);
    stmtStats_->getMasterStats()->setElapsedEndTime(exeEndTime);
  }

  // If this is a CALL statement that can produce result sets (as
  // indicated by a non-NULL return value from getResultSetInfo())
  // then we may need to tell the UDR server to release this
  // transaction.
  ExRsInfo *rsInfo = getResultSetInfo();
  if (rsInfo)
  {
    StmtDebug0("  About to call ExRsInfo::suspendUdrTx()...");
    rsInfo->suspendUdrTx(*statementGlobals_);
    StmtDebug0("  Done");
  }

  // Make sure there are no outstanding transactional UDR requests
  completeUdrRequests(allWorkRequests);

  
  // Ideally, we would want to set this state for all cursors.
  // But we found that setting it for all results in an unnecessary call to
  // removeFromOpenStatementList() when the state is changed from RELEASE_TRANS_ 
  // to CLOSE_ even when the cursor was already closed earlier.
  if (isAnsiHoldable())
    setState(RELEASE_TRANS_);
  else
  if (isPubsubHoldable())
  {
    if (! getRootTdb()->getPsholdUpdateBeforeFetch())
      setState(RELEASE_TRANS_);
  }

  StmtDebug1("[END releaseTransaction] %p", this);
  return SUCCESS;
}

void Statement::releaseEsps(NABoolean closeAllOpens)
{
  if (!statementGlobals_)
    return;

  ExRtFragTable *fragTable = statementGlobals_->getRtFragTable();
  if (!fragTable)
    return;

  fragTable->releaseEsps(closeAllOpens);
	  
  // wait until all release esp replies have come back
  NABoolean hasTimedout;
  IpcTimeout espReleaseTimeout;
  espReleaseTimeout = context_->getEspReleaseTimeout();
  if (espReleaseTimeout != -1)
    espReleaseTimeout *= 6000;
  while (fragTable->hasOutstandingReleaseEspMsges())
    {
      statementGlobals_->getIpcEnvironment()->getAllConnections()->
        waitOnAll(5*6000, FALSE, &hasTimedout);// Wait up to 5 minutes
      if (hasTimedout)
        {
          // not all replied after 5 minutes, something is wrong
          char msg[300];
          str_sprintf(msg, "Timedout! Still have %d release esp messages and %d I/O outstanding.",
                      fragTable->numOutstandingReleaseEspMsges(),
                      statementGlobals_->getIpcEnvironment()
                      ->getAllConnections()->getNumPendingIOs());
          SQLMXLoggingArea::logExecRtInfo("Statement.cpp", 0, msg, 0);
          // abort remaining release esp msgs
          fragTable->abandonPendingIOs();
        } // if (hasTimedout)
    } // while
}

RETCODE Statement::bindTo(Statement * stmt)
{
  StmtDebug2("[BEGIN bindTo] Binding clone %p to master %p",
             this, stmt);

  root_tdb = stmt->root_tdb;
  clonedFrom_ = stmt;
  setState(CLOSE_); // does this work here????
  setCloned();
  stmt->clonedStatements->insert((void *) this);

  StmtDebug0("[END bindTo]");
  return SUCCESS;
}

bool Statement::isOpen()
{
  switch (stmt_state)
  {
    case OPEN_:
    case FETCH_:
    case EOF_:
    case PREPARE_:
    case RELEASE_TRANS_:
    case INITIAL_:
      return true;
    default:
      return false;
  }
}

RETCODE Statement::close(ComDiagsArea &diagsArea, NABoolean inRollback)
{  
  StmtDebug2("[BEGIN close] %p, stmt state %s", this, stmtState(getState()));
  RETCODE rc = SUCCESS;
  NABoolean readyToReturn = FALSE;

  if (!isOpen())
  {
      // trying to close a statement which is already in closed state
      diagsArea << DgSqlCode(- CLI_STMT_NOT_OPEN);
    rc = ERROR;
    readyToReturn = TRUE;
  }

  NABoolean closingEmbeddedIUDCursorPrematurely = FALSE; 

  if (!readyToReturn)
  {
    if ((stmt_state == OPEN_) && 
        (root_tdb && root_tdb->isEmbeddedIUDWithLast1()))
    {
      closingEmbeddedIUDCursorPrematurely = TRUE;
    }

    // Update stmtGlobals transid with current transid.
    // The stmtGlobals transid is used in sending release msg to remote esps.
    // The context transid can be obsolete if the transaction has been 
    // committed/aborted by the user.
    Int64 transid;
    if (!context_->getTransaction()->getCurrentXnId(&transid))
      getGlobals()->castToExExeStmtGlobals()->getTransid() = transid;
    else 
      getGlobals()->castToExExeStmtGlobals()->getTransid() = (Int64)-1;

  // cancel the down request to my child
    if (root_tcb)
    {
      ComDiagsArea *diagsPtr = NULL;

      // pass flag to cancel to collect diags from queue entries 
      // if stmt_state is OPEN_.  This is needed to pick up diags
      // info from failed DDL operations.
      NABoolean pickUpDiags =
        (   // False if DML, true otherwise
           (root_tdb->getQueryType() < ComTdbRoot::SQL_SELECT_UNIQUE)     ||
           (root_tdb->getQueryType() > ComTdbRoot::SQL_DELETE_NON_UNIQUE)
        );

      Int32 rc1 =
      root_tcb->cancel(statementGlobals_, diagsPtr, pickUpDiags);

      StmtDebug1("  root_tcb->cancel() returned %s",
                 RetcodeToString((RETCODE) rc1));
      if (diagsPtr)
	{
	  diagsArea.mergeAfter(*diagsPtr);
	  diagsPtr->decrRefCount();    
          diagsPtr = NULL;
	}
      if (root_tcb->fatalErrorOccurred())
        {
          dealloc();
          space_.freeBlocks();
          statementGlobals_->reAllocate(1);
          setFixupState(0);
        }
    }
  }

  if (!readyToReturn)
  {
    // If this statement has stored procedure result set children,
    // then close each child. Ignore errors encountered while closing
    // the children.
    ExRsInfo *rsInfo = getResultSetInfo();
    if (rsInfo)
    {
      ULng32 numChildren = rsInfo->getNumEntries();
      StmtDebug0("  About to close all result set proxy statements");
      StmtDebug1("  Num RS children: %d", (Lng32) numChildren);

      ComDiagsArea *diagsFromProxy = NULL;

      for (ULng32 i = 1; i <= numChildren && !readyToReturn; i++)
      {
        Statement *proxyStmt = NULL;
        const char *proxySyntax = NULL;
        NABoolean open = FALSE;
        NABoolean closed = FALSE;
        NABoolean prepared = FALSE;
        NABoolean found = rsInfo->getRsInfo(i, proxyStmt, proxySyntax,
                                            open, closed, prepared);

        if (found && proxyStmt && proxyStmt->stmt_state != CLOSE_)
        {
          if (diagsFromProxy == NULL)
            diagsFromProxy = ComDiagsArea::allocate(&heap_);

          RETCODE rcFromProxy = proxyStmt->close(*diagsFromProxy);
          diagsFromProxy->clear();
        }

      } // for each proxy

      if (diagsFromProxy)
        diagsFromProxy->deAllocate();

      StmtDebug0("  About to call ExRsInfo::exitUdrTx()...");
      rsInfo->exitUdrTx(*statementGlobals_);
      StmtDebug0("  Done");
      rsInfo->reset();
    } // if (rsInfo)
  
    // release all work requests for the statement
    if (context_->getSessionDefaults()->getAltpriEsp())
      releaseTransaction(TRUE, TRUE);
    else
      releaseTransaction(TRUE, FALSE);

    setState(CLOSE_);

    if (closingEmbeddedIUDCursorPrematurely)
    {
      diagsArea << DgSqlCode(- CLI_IUD_IN_PROGRESS) << DgString0(getCursorName()->identifier);
      if (rollbackTransaction(diagsArea))
          return ERROR;
    }

    // stop the transaction, if one was started and if we are not
    // currently rolling back this transaction
    if (!inRollback)
    {
      if ((root_tdb && root_tdb->transactionReqd()) ||
          ((context_->getTransaction()->xnInProgress()) &&
          (context_->getTransaction()->exeStartedXn()) &&
          (context_->getTransaction()->autoCommit()) &&
          (autocommitXn())))
        {
          if (commitTransaction(diagsArea))
          {
            rc = ERROR;
            readyToReturn = TRUE;
          }
        }
    } // !inRollback
  } // !readyToReturn
  updateStatsAreaInContext();
  // Get rid of child query info if there is any
  if (childQueryId_ != NULL)
  {
    NADELETEBASIC(childQueryId_, &heap_);
    childQueryId_ = NULL;
    childQueryIdLen_ = 0;
  }
  if (childQueryCostInfo_ != NULL)
  {
    NADELETEBASIC(childQueryCostInfo_, &heap_);
    childQueryCostInfo_ = NULL;
  }
  if (childQueryCompStatsInfo_ != NULL)
  {
    NADELETEBASIC(childQueryCompStatsInfo_, &heap_);
    childQueryCompStatsInfo_ = NULL;
  }

  // clear transId from statement globals as it is no longer needed
  // this is to avoid reuse the transId when deleting the statement
  statementGlobals_->getTransid() = (Int64)-1;

  StmtDebug2("[END close] %p, result is %s", this,
             RetcodeToString(rc));
  return rc;
}

static inline void diagsTakeOver(ComDiagsArea &diagsArea,
				 CliGlobals *cliGlobals,
                                  short indexIntoCompilerArray)
{
  ComDiagsArea &exec_diags =
    *cliGlobals->getArkcmp(indexIntoCompilerArray)->getDiags();
  if (&diagsArea != &exec_diags)
    {
      if (diagsArea.getFunction() == ComDiagsArea::NULL_FUNCTION)
	diagsArea.setFunction(exec_diags.getFunction());
      diagsArea.mergeAfter(exec_diags);
      exec_diags.clear();
    }
}

NABoolean Statement::isDISPLAY()
{
  QueryText qt(source_str, charset_);
  return qt.isDISPLAY();
}

NABoolean Statement::isExeDebug(char *src, Lng32 charset)
{
  if (!src) {
    return FALSE;
  }
  else {
    if (charset == SQLCHARSETCODE_UCS2) {
          NAWchar* p = (NAWchar*)(src) + 7;
         
          return  p[0] == NAWchar('$') &&
                  p[1] == NAWchar('Z') &&
                  p[2] == NAWchar('Z') &&
                  p[3] == NAWchar('E') &&
                  p[4] == NAWchar('X') &&
                  p[5] == NAWchar('E') &&
                  p[6] == NAWchar('D') &&
                  p[7] == NAWchar('E') &&
                  p[8] == NAWchar('B') &&
                  p[9] == NAWchar('U') &&
                  p[10] == NAWchar('G') ;
    }
    else {
	  return str_cmp(src+7, "$ZZEXEDEBUG", str_len("$ZZEXEDEBUG")) == 0;
    }
  }
}

Int32 Statement::octetLen(char *s, Lng32 charset)
{
  return charset==SQLCHARSETCODE_UCS2 ?  
    na_wcslen((const NAWchar*)s) * 
    CharInfo::maxBytesPerChar((CharInfo::CharSet)charset) : str_len(s);
}

Int32 Statement::octetLenplus1(char *s, Lng32 charset)
{
  return charset==SQLCHARSETCODE_UCS2 ? 
    (na_wcslen((const NAWchar*)s)+1)*
    CharInfo::maxBytesPerChar((CharInfo::CharSet)charset) : str_len(s)+1;
}

Int32 Statement::sourceLenplus1()
{
  return (source_length+1)*CharInfo::maxBytesPerChar((CharInfo::CharSet)charset_);
}

RETCODE Statement::prepareReturn ( const RETCODE retcode)
{
  if (versionToUse_ != versionOnEntry_)
  {
    // We changed the current compiler, reset it
    short index;
    getContext()->setOrStartCompiler(versionOnEntry_, NULL, index);
  }

  // Unconditionally reset version on entry.
  versionOnEntry_ = COM_VERS_UNKNOWN;
  if (retcode != NOT_FINISHED)
  {
    // We're done with the prepare, reset the version we used
    versionToUse_ = COM_VERS_UNKNOWN;
  }

  return retcode;
}

/*
NABoolean Statement::isStandaloneQ()
{
  NABoolean standaloneQ = FALSE;

  // if query from mxcs and the statement name starts with "SQL_CUR_",
  // then it is a standalone query. This is standard and guaranteed
  // mxcs behavior.
  // or if query is from mxci and the statement name starts 
  // with "__SQLCI_", then it is a standalone query.
  if ((getContext()->getSessionDefaults()->getOdbcProcess()) &&
      (statement_id->identifier_len > 0) &&
      (strncmp(statement_id->identifier, "SQL_CUR_", strlen("SQL_CUR_")) == 0))
    {
    standaloneQ = TRUE;
    }
  else if ((getContext()->getSessionDefaults()->getMxciProcess()) &&
	   (statement_id->identifier_len > 0) &&
	   (strncmp(statement_id->identifier, "__SQLCI_DML_", strlen("__SQLCI_DML_")) == 0))
    {
      standaloneQ = TRUE;
    }
  
  return standaloneQ;
}
*/
RETCODE Statement::prepare(char *source, ComDiagsArea &diagsArea,
			   char *passed_gen_code, 
			   ULng32 passed_gen_code_len,
			   Lng32 charset,
			   NABoolean unpackTdbs,
			   ULng32 cliFlags)
{
  StmtDebug1("[BEGIN prepare] %p", this);
  StmtDebug1("  Source: %s",
             source ? source : (source_str ? source_str : "(NULL)"));

  state_ = INITIAL_STATE_;

  if (aqrReprepareNeeded())
    setAqrReprepareNeeded(FALSE);

  if (NOT ((! (cliFlags & PREPARE_AUTO_QUERY_RETRY)) &&
	   (cliFlags & PREPARE_STANDALONE_QUERY) &&
	   source &&
	   (stmt_type == DYNAMIC_STMT) &&
	   unpackTdbs &&
	   (context_->getNumOfCliCalls() == 1)))
    {
      // do not use query text cache for explicitely prepared queries.
      cliFlags |= PREPARE_NO_TEXT_CACHE;
    }
  
  if (context_->aqrInfo())
    context_->aqrInfo()->setXnStartedAtPrepare(FALSE);
  
  RETCODE rc = prepare2(source, diagsArea, 
			passed_gen_code, passed_gen_code_len,
			charset, unpackTdbs, cliFlags);

  StmtDebug2("[END prepare] %p, result is %s", this, RetcodeToString(rc));
  return rc;
}

RETCODE Statement::prepare2(char *source, ComDiagsArea &diagsArea,
                            char *passed_gen_code, 
			    ULng32 passed_gen_code_len,
			    Lng32 charset,
			    NABoolean unpackTdbs,
			    ULng32 cliFlags)
{
  ULng32 fetched_gen_code_len = 0L;
  char *fetched_gen_code = NULL;
  short retcode = SUCCESS;   
  short indexIntoCompilerArray = 0;
     
  // if there is any error using embedded cmpiler and we will switch to regular compiler
  NABoolean canUseEmbeddedArkcmp = ((cliFlags & PREPARE_USE_EMBEDDED_ARKCMP) != 0) ; // This flag 
  // will be set only by the master. If a Prepare call is made from the 
  // compiler(including the embedded compiler), it will use the regular compiler.

  ExSqlComp::OperationStatus status;
  Dealloc dealloc; // DTOR calls NAHeap::deallocateMemory for an object

  //  NABoolean newOperation = TRUE;
  NABoolean newOperation = (NOT ((cliFlags & PREPARE_NOT_A_NEW_OPERATION) != 0));
  NABoolean reComp       = (cliFlags & PREPARE_RECOMP) != 0;
  NABoolean aqRetry      = (cliFlags & PREPARE_AUTO_QUERY_RETRY) != 0;
  NABoolean deCache      = (cliFlags & PREPARE_WITH_DECACHE) != 0;
  NABoolean noTextCache  = (cliFlags & PREPARE_NO_TEXT_CACHE) != 0;
  NABoolean standaloneQuery = (cliFlags & PREPARE_STANDALONE_QUERY) != 0;
  NABoolean doNotCache   = (cliFlags & PREPARE_DONT_CACHE) != 0;
  this->setStandaloneQ(standaloneQuery);

  NABoolean wmsMonitoringNeeded = (cliFlags & PREPARE_MONITOR_THIS_QUERY) !=0;

  this->setWMSMonitorQuery(wmsMonitoringNeeded);
  // Remove compileStatsArea, if it exists
  if (compileStatsArea_  != NULL)
  {
    NADELETE(compileStatsArea_, ExStatisticsArea, compileStatsArea_->getHeap());
    compileStatsArea_ = NULL;
  }
  // We should not do prepares on a cloned statement. Clones share a
  // TDB tree with some other statement and compiles/recompiles should
  // always be done by that other statement.
  ex_assert(!isCloned(),
            "Statement::prepare() should not be called on a clone");

  // We may be doing a prepare for two reasons: 1) as part of a
  // dynamic SQL PREPARE operation or 2) as part of automatic
  // recompilation. The first case includes such things as 
  // EXECUTE IMMEDIATE and internal calls to prepare statements.
  // 
  // The value returned by the noWaitOpPending() member in the Statement 
  // object must be interpreted very carefully here, because its 
  // meaning depends on which of these two cases
  // we are in. In the first case, where we are doing an explicit
  // prepare, noWaitOpPending() TRUE implies that we are trying
  // to start a new explicit operation on a statement that already
  // has a no-wait operation in progress. This is a user error,
  // and is raised as such in this method.
  //
  // In the second case, however, this method has been called
  // from another Statement method to do an automatic recompilation.
  // If noWaitOpPending() == TRUE in this case, it is because the
  // calling method is processing an outstanding no-wait operation.
  // So, for example, we might be attempting an automatic
  // recompilation while redriving a no-wait fetch. This is not
  // a user error; in fact we want the automatic recompilation
  // to succeed.
  //
  // The way to distinguish these two cases is to consider the
  // reComp parameter. If it is FALSE, we know it is the first
  // case. If it is TRUE, we know it is the second.
  //
  // For the moment, we make all automatic recompilations waited,
  // even if we are redriving a no-wait operation. The reason we
  // do this is the Statement object lacks the state, and the
  // calling methods lack the logic, to detect on a redrive that
  // a no-wait automatic recompilation is in effect. This is a 
  // refactoring opportunity for a clever developer in a future 
  // release ;-).
  //
  // Note that since we make automatic recompilations waited,
  // we treat the noWaitOpEnabled_ flag differently in this case
  // as well.
  //
  // In this method, then, instead of testing noWaitOpPending_
  // directly, we use the following flag which takes reComp
  // into account.


  Lng32 rsa = getRowsetAtomicity();
  if (context_->getSessionDefaults()->getRowsetAtomicity() != -1)
    rsa = context_->getSessionDefaults()->getRowsetAtomicity(); //NOT_ATOMIC_;

  if (canUseEmbeddedArkcmp && !context_->isEmbeddedArkcmpInitialized())
    {
      Int32 embeddedArkcmpSetup;
      // embeddedArkcmpSetup = arkcmp_main_entry();
      embeddedArkcmpSetup = context_->switchToCmpContext((Int32)0);
      if (embeddedArkcmpSetup == 0)           
        {
          context_->setEmbeddedArkcmpIsInitialized(TRUE);
        }
      else if (embeddedArkcmpSetup == -2)
        {
          diagsArea << DgSqlCode(-2079);
          return ERROR;
        }
      else
        {
          context_->setEmbeddedArkcmpIsInitialized(FALSE);
          context_->setEmbeddedArkcmpContext(NULL);
        }
    }
  // Set the Global CmpContext from the one saved in the CLI context
  // for proper operation
  if (context_->isEmbeddedArkcmpInitialized() &&
      context_->getEmbeddedArkcmpContext())
    {
      cmpCurrentContext = context_->getEmbeddedArkcmpContext();
    }
    
  if (newOperation)
    assert ((aqRetry && source) ||
	    ((!reComp) || (reComp && (!source))));

  if ((reComp) && (stmt_type == STATIC_STMT))
    {
    }
#ifdef _DEBUG
  if (getenv("TEST_INFO_EVENT"))
    {

      SQLMXLoggingArea::logSQLMXEventForError(0000, "Dummy Error with Dummy SQCode and QID", "DummyQID", FALSE);

      SQLMXLoggingArea::logExecRtInfo("Statement.cpp",999,"Testing info event", 999);
      
      SQLMXLoggingArea::logSQLMXAbortEvent("Statement.cpp",888, "testing abort event");
      SQLMXLoggingArea::logSQLMXAssertionFailureEvent("Statement.cpp",777,"testing assertion failure");
      SQLMXLoggingArea::logSQLMXDebugEvent("debug event" ,69,__LINE__);
      
      SQLMXLoggingArea::logMVRefreshInfoEvent("mv refresh info");
      SQLMXLoggingArea::logMVRefreshErrorEvent("mv refresh error");

      SQLMXLoggingArea::logCliReclaimSpaceEvent(100,200,300,400);

      SQLMXLoggingArea::logCompNQCretryEvent("select * from t1 where a > 10");
      SQLMXLoggingArea::logSortDiskInfo("disk101", 25, 2080);

      const char msgT[] = "SQL compiler: Optimization failed at pass two or higher. " 
                          "Execution will use the plan generated at pass one instead.";

      SQLMXLoggingArea::logSQLMXPredefinedEvent(msgT, LL_WARN);
    }
 #endif
  
  if (passed_gen_code)
    {
      // it is legal for passed_gen_code to be passed in as NULL here!
      copyGenCode(passed_gen_code, passed_gen_code_len, FALSE);

      
    }
  else if ((source) ||               // source is passed in. Preparing a stmt.
	   ((!source) && (reComp)) || // recompiling a statement at runtime.
	    !newOperation)
    {
      if (isExeDebug(source, charset))
	{
	  NADebug();
	  diagsArea << DgSqlCode(- CLI_STMT_NOT_PREPARED);
          // OK to return directly, we haven't done anything to
          // the current compiler settings yet
	  return ERROR;
	}
      
      if (context_->checkAndSetCurrentDefineContext())
	{

	     // define context changed, kill arkcmps, if they are runing.
             for (short i = 0; 
		  i < getContext()->getNumArkcmps();
		  i++)
	       cliGlobals_->getArkcmp(i)->endConnection();

	}

      // Get the version of compiler to use initially. There are several possibilities.
      // a) No particular version compiler is required, we use whatever is the current version.
      // b) The compiler version was explicitly set from outside, using setOrStartCompiler.
      //    This may happen for automatic retry of plan version errors, and for certain DDL
      //    operations.
      // c) The compiler version was requested, using the versionToUse_ data member.
      //    This may happen if a non-retryable statement encountered a plan version error.
      // In all cases, save whatever version was the default for our caller since prepare
      // may automatically set the compiler version further down the road.
      // 
      versionOnEntry_ = context_->getVersionOfCompiler();
      if (newOperation && (versionToUse_ == COM_VERS_UNKNOWN))
      {
        // This is a new prepare, and a particular version was not requested.
        // Use the version from the context.
        indexIntoCompilerArray = context_->getIndexToCompilerArray();  
        versionToUse_ = versionOnEntry_;
      }
      else
      {
        // This is a redrive of a nowait prepare, or a particular version was requested.
        // Use whatever we used originally/whatever is requested.
        getContext()->setOrStartCompiler(versionToUse_, NULL, indexIntoCompilerArray);
      }

      // From this point on, do not return directly as that will mess up the current compiler version.
      // Instead, do 
      //    return prepareReturn (<retcode>);
      short retry = TRUE;
      char * data = NULL;
      ULng32 dataLen = 0;
      ExSqlComp::Operator op;
      while (retry)
	{
	  if (newOperation)
	    {

	      // Build request and send to arkcmp. 
	      if ((reComp) || (aqRetry && deCache))
		{
		  CmpCompileInfo c((reComp ? source_str : source), 
				   (reComp ? sourceLenplus1() : octetLenplus1(source, charset)),
				   (Lng32) (reComp ? charset_ : charset),
				   schemaName_, schemaNameLength_+1, 
				   getInputArrayMaxsize(), (short)rsa);

		  if (aqRetry)
		    c.setAqrPrepare(aqRetry);
		  else if (noTextCache)
		  c.setNoTextCache(noTextCache);
		  c.setDoNotCachePlan(doNotCache);
		  
		  if (standaloneQuery)
		    c.setStandaloneQuery(standaloneQuery);

		  // if this statement was statically compiled with odbc process
		  // on, then set it here. This will be used at auto recomp time.
		  c.setOdbcProcess(odbcProcess());
		  c.setSystemModuleStmt(systemModuleStmt());   
                  c.setAnsiHoldable(isAnsiHoldable());
                  c.setPubsubHoldable(isPubsubHoldable());
              	  dataLen = c.getLength();
		  // Dealloc dtor will deallocate the memory allocated by next statement
		  data = (char *)dealloc.getAddr
		    (new(dealloc.setHeap(&heap_)) char[dataLen]);
		  c.pack(data);
		  
		  // Release the existing TDB tree if one exists
		  assignRootTdb(NULL);
		  
		  op = (getStatementType() == DYNAMIC_STMT) ?
		    EXSQLCOMP::SQLTEXT_RECOMPILE :
		    EXSQLCOMP::SQLTEXT_STATIC_RECOMPILE;
		  
		  
		  // The R1.8 compiler does not understand the above type
		  // EXSQLCOMP::SQLTEXT_STATIC_RECOMPILE. So if we are talking
		  // to an R1.8 compiler then we have to change the type to
		  // EXSQLCOMP::SQLTEXT_STATIC_COMPILE
		  
		  if (cliGlobals_->getArkcmp(indexIntoCompilerArray)->
		      getVersion() == COM_VERS_R1_8 )
		    {
		      op = (getStatementType() == DYNAMIC_STMT) ?
			EXSQLCOMP::SQLTEXT_RECOMPILE :
			EXSQLCOMP::SQLTEXT_STATIC_COMPILE;
		    }
		} // recompiling statement
	      else
		{
		  CmpCompileInfo c(source, octetLenplus1(source, charset), (Lng32) charset,
				   NULL, 0,
				   getInputArrayMaxsize(), (short)rsa);

		  if (aqRetry)
		    c.setAqrPrepare(aqRetry);
		  else if (noTextCache)
		  c.setNoTextCache(noTextCache);
		  c.setDoNotCachePlan(doNotCache);

		  if (standaloneQuery)
		    c.setStandaloneQuery(standaloneQuery);
        	  c.setAnsiHoldable(isAnsiHoldable());
                  c.setPubsubHoldable(isPubsubHoldable());
                  dataLen = c.getLength();
		  // Dealloc dtor will deallocate the memory allocated by next statement
		  data = (char *)dealloc.getAddr
		    (new(dealloc.setHeap(&heap_)) char[dataLen]);
		  c.pack(data);
		  
		  op = EXSQLCOMP::SQLTEXT_COMPILE;
		}
	      
                
	      // send request
	      // request is nowaited if noWaitOpEnabled and not recompile
	      NABoolean waited = TRUE;

              // Use the embedded compiler first
              if (context_->getSessionDefaults()->callEmbeddedArkcmp() && 
		  canUseEmbeddedArkcmp && 
		  context_->isEmbeddedArkcmpInitialized() &&
		  CmpCommon::context() && (CmpCommon::context()->getRecursionLevel() == 0))
		  //!aqRetry  && cliGlobals_->isEmbeddedArkcmpInitialized())
                {
                  Int32 compStatus;
                  ComDiagsArea *da = NULL;

                  // clean up diags area of regular arkcmp, it could contain
                  // old errors from last use
                  if (cliGlobals_->getArkcmp() &&
                      cliGlobals_->getArkcmp()->getDiags())
                     cliGlobals_->getArkcmp()->getDiags()->clear();

                  compStatus = CmpCommon::context()->compileDirect(
                                   (char *)data, dataLen,
                                   // use arkcmp heap to store the plan
                                   // check why indexIntoCompilerArray is used here?
                                   cliGlobals_->getArkcmp(indexIntoCompilerArray)->getHeap(),
                                   charset, op,
                                   fetched_gen_code, fetched_gen_code_len,
                                   context_->getSqlParserFlags(), 
                                   NULL, 0, da);
                  if (da != NULL) 
                  {
                     diagsArea.mergeAfter(*da);
                     da->decrRefCount();
                  }

                  if (compStatus == ExSqlComp::SUCCESS)
                    {
                      // break from the while (retry) loop
                      break;
                    }
                  else
		    {
		      diagsArea << DgSqlCode(- CLI_STMT_NOT_PREPARED);
		      if (diagsArea.getRollbackTransaction())
		     	rollbackTransaction(diagsArea);
		      return prepareReturn (ERROR);
		    }
                }

	      ExSqlComp::ReturnStatus sendStatus = 
		cliGlobals_->getArkcmp(indexIntoCompilerArray)->sendRequest(
		     op, data, dataLen,
		     waited, 
		     0, charset, TRUE);
	      
	      if ( sendStatus == ExSqlComp::ERROR || 
		   sendStatus == ExSqlComp::WARNING )
		{
		  diagsTakeOver(diagsArea, cliGlobals_,indexIntoCompilerArray);
		  if (sendStatus == ExSqlComp::ERROR)
		    {
		      setState(INITIAL_);
#ifndef _DEBUG
                      // For release, if the mxcmp could not reply
                      // to the request, use error -2005, to let the 
                      // user know that a dialout has been generated.
		      diagsArea << DgSqlCode(arkcmpErrorNoDiags);
		      diagsArea << DgString0(source);
#endif
                      diagsArea << DgSqlCode(- CLI_STMT_NOT_PREPARED);
                      if (diagsArea.getRollbackTransaction())
			rollbackTransaction(diagsArea);
		      return prepareReturn (ERROR);
		    }
		  else	//if (sendStatus == ExSqlComp::WARNING)
		    retcode = WARNING;
		}
	    } // if new operation
	  
	  // check for completion of request.
	  status = cliGlobals_->getArkcmp(indexIntoCompilerArray)->status();
	  
	  if ( status != ExSqlComp::FINISHED )
	      {
	      // waited request should complete. Return error.
	      diagsTakeOver(diagsArea, cliGlobals_,indexIntoCompilerArray);
	      diagsArea << DgSqlCode(- CLI_IO_REQUESTS_PENDING) 
			<< DgString0("SQLTEXT_COMPILE");
	      return prepareReturn (ERROR); 
	    }
	  
	  // initialize pointer to returned data(from reply) in private state.
	  ExSqlComp::ReturnStatus replyStatus = 
	    cliGlobals_->getArkcmp(indexIntoCompilerArray)->getReply(
		 fetched_gen_code, 
		 fetched_gen_code_len);
	  
          retry = FALSE;

          if (replyStatus != ExSqlComp::SUCCESS)
            {
              diagsTakeOver (diagsArea, cliGlobals_, indexIntoCompilerArray);
 
              if (replyStatus == ExSqlComp::ERROR)
                {
                  
                  if (retry == FALSE)
                  {
                    // An error other than 25304, or failure to start downrev compiler
                    setState(INITIAL_);
		    
		    if (ABS(diagsArea.mainSQLCODE()) != 4074
			||
			diagsArea.getNumber() != 1)
		      diagsArea << DgSqlCode(- CLI_STMT_NOT_PREPARED);
		    return prepareReturn (ERROR);
		  }
		}
	      else
                {
                  retcode = WARNING;
		}
	    }

	} // while retry
      context_->killIdleMxcmp();
      assignRootTdb((ex_root_tdb *)fetched_gen_code);
      root_tdb_size = (Lng32) fetched_gen_code_len;
    }
  
  if (root_tdb)
    {
      diagsArea.setCost(root_tdb->getCost());
    }
  if (unpackTdbs)
    retcode = (short)unpackAndInit(diagsArea, indexIntoCompilerArray);
  
  return (RETCODE)retcode;
} // Statement::prepare

Lng32 Statement::unpackAndInit(ComDiagsArea &diagsArea,
			      short indexIntoCompilerArray)
{
  Lng32 retcode = 0;

  // Do not unpack the root tdb if the statement is from showplan.
  // CmpDescribePlan will unpack it.
  ComTdbRoot *thisTdb = root_tdb;

  if (root_tdb && !thisTdb->isFromShowplan())
    {
      // Here, we have just freshly (re)prepared the plan using the latest
      // version of the compiler. Unpacking should be uneventful (i.e. no
      // object version migration should happen). Therefore, it should not
      // be necessary to reallocate space.
      //
      ComTdb dummyTdb;
      ex_root_tdb *newRoot = (ex_root_tdb *)
	root_tdb->driveUnpack((void *) root_tdb, &dummyTdb, unpackSpace_);
      
      if (newRoot == NULL)  
	{
	  // ERROR during unpacking. 
	  if ((indexIntoCompilerArray >= 0) &&
	      (cliGlobals_->getArkcmp(indexIntoCompilerArray)->getDiags()))
	    diagsArea.mergeAfter(*cliGlobals_->getArkcmp()->getDiags());
	  diagsArea << DgSqlCode(- CLI_STMT_NOT_PREPARED);
          return prepareReturn (ERROR); 
	}

      assignRootTdb(newRoot);
    }


      if ( (versionToUse_ != COM_VERS_COMPILER_VERSION)                    && // Used a downrev compiler
           (returnRecompWarn())                                            && // Will return recomp warnings
           (root_tdb->getQueryType() >= ComTdbRoot::SQL_SELECT_UNIQUE)     && // Query is DML
           (root_tdb->getQueryType() <= ComTdbRoot::SQL_DELETE_NON_UNIQUE)
         )
      {
        // We compiled a DML query with a downrev compiler - save information so that we can 
        // issue warning 25304 if required. We do this rather than issue the warning directly,
        // because we want warning 8576: Query was recompiled, to precede warning 25304 if applicable.
	mxcmpErrorCode_ = VERSION_COMPILER_USED_TO_COMPILE_QUERY;
        mxcmpStartedVersion_ = (COM_VERSION) versionToUse_; 
      }

  // Reset state in this statement, then loop through all clones and
  // make corresponding state changes in each.
  if (root_tdb)
    {
      setComputeBulkMoveInfo(TRUE);
    }
  setFixupState(0);
  setState(CLOSE_);
  setFirstResolveDone(FALSE);
  lnil_ = NULL;
  
  clonedStatements->position();
  Statement * clone = (Statement*)clonedStatements->getNext();
  for (; clone != NULL; clone = (Statement*)clonedStatements->getNext())
    {
      if (clone->root_tdb)
	{
	  clone->setComputeBulkMoveInfo(TRUE);
	}
      clone->setFixupState(0);
      clone->setState(CLOSE_);
      clone->setFirstResolveDone(FALSE);
      clone->lnil_ = NULL;
    }
  
  if (indexIntoCompilerArray >= 0)
    diagsTakeOver(diagsArea, cliGlobals_,indexIntoCompilerArray);
  
  if (root_tdb)
    diagsArea.setCost(root_tdb->getCost());
  StatsGlobals *statsGlobals = cliGlobals_->getStatsGlobals();
  Lng32 fragOffset;
  Lng32 fragLen;
  Lng32 topNodeOffset;
  SessionDefaults *sessionDefaults =
       context_->getSessionDefaults();
  if (statsGlobals != NULL && stmtStats_ != NULL && root_tdb != NULL 
        && getUniqueStmtId() != NULL) 
  {
    ex_root_tdb *rootTdb = getRootTdb();
    //root_tdb is not unpacked for SHOWPLAN and 
    // explain fragment can't be obtained for such prepared queries
    if (!rootTdb->isPacked() && rootTdb->explainInRms() &&
        rootTdb->getFragDir()->getExplainFragDirEntry
                 (fragOffset, fragLen, topNodeOffset) == 0)
    {
      int error = statsGlobals->getStatsSemaphore(cliGlobals_->getSemId(),
            cliGlobals_->myPin());
      stmtStats_->setExplainFrag((void *)(((char *)root_tdb)+fragOffset), fragLen, topNodeOffset);
      statsGlobals->releaseStatsSemaphore(cliGlobals_->getSemId(),cliGlobals_->myPin());
    }
  }
  return prepareReturn ((RETCODE)retcode);
}

RETCODE Statement::closeTables(ComDiagsArea &diagsArea)
{
  // cancel the down request to my child
  ComDiagsArea *diagsPtr = NULL;
  Int32 retcode = root_tcb->cancel(statementGlobals_,diagsPtr);
  StmtDebug1("  root_tcb->cancel() returned %s",
             RetcodeToString((RETCODE) retcode));
  if (diagsPtr)
    {
      diagsArea.mergeAfter(*diagsPtr);
      diagsPtr->decrRefCount();    
      diagsPtr = NULL;
    }

  if (retcode)
    return ERROR;
  
  retcode = root_tcb->closeTables(statementGlobals_,
                                  statementGlobals_->getRtFragTable());
  if (retcode)
    return ERROR;
  
  return SUCCESS;
}

RETCODE Statement::reOpenTables(ComDiagsArea &diagsArea)
{
  closeTables(diagsArea);

  Int32 retcode =
    root_tcb->reOpenTables(
	 statementGlobals_,
	 statementGlobals_->getRtFragTable());
  if (retcode)
    return ERROR;
  
  return SUCCESS;
}

/////////////////////////////////////////////////////////////////////
// Searches for the given cursorName in the statement list. If found,
// and the cursor is an updatable cursor, returns pointer to that
// statement. Else, returns NULL.
/////////////////////////////////////////////////////////////////////
Statement * Statement::getCurrentOfCursorStatement(char * cursorName)
{
  ComDiagsArea &diags = context_->diags();

  if (!cursorName)
     return NULL;

  char nameBuf[ComMAX_1_PART_EXTERNAL_UTF8_NAME_LEN_IN_BYTES + 1 + 16]; // a null terminator + a few extra bytes
  char *pName = nameBuf;
  Lng32 len = str_len(cursorName);

  if (len > sizeof(nameBuf) - 1)
    pName = (char *)heap_.allocateMemory(len + 1);

  str_cpy_all(pName, cursorName, len);
  pName[len] = '\0';
  str_strip_blanks(pName,len);
  // Convert to ansi id format
  if (str_to_ansi_id(cursorName,pName,len))
	{
          diags << DgSqlCode(-CLI_INVALID_SQL_ID)
       << DgString0(pName);
        return NULL;
	}
  HashQueue * cursorList =
    context_->getCursorList();

  Statement *pStmt = NULL;
  Statement * stmt;
  
  cursorList->position(pName, len);
  while (stmt = (Statement *)cursorList->getNext())
    { 
      //char * stmtCursorName = stmt->getCursorName();
      SQLSTMT_ID* stmtCursorName = stmt->getCursorName();

// Need to use length in comparison when the
// new length argument is added for "cursorName".
      if (stmtCursorName &&
          (strcmp(pName, stmtCursorName->identifier) == 0))
        {
          if (stmt->getRootTdb() && (stmt->getRootTdb()->updatableSelect() ))
	    {
	      pStmt = stmt;
	      break;
	    }
          else
            break;
	  
        }
    }

  if (pName != nameBuf)
    heap_.deallocateMemory(pName);
  return pStmt;

}

RETCODE Statement::doHiveTableSimCheck(TrafSimilarityTableInfo *si,
                                       NABoolean &simCheckFailed,
                                       ComDiagsArea &diagsArea)
{
  simCheckFailed = FALSE;

  if ((si->hdfsRootDir() == NULL) || (si->modTS() == -1))
    return SUCCESS;
 
  char *tmpBuf = new (&heap_) char[ComMAX_3_PART_EXTERNAL_UTF8_NAME_LEN_IN_BYTES+6];
  Lng32 numParts = 0;
  char *parts[4];
  Int64 redefTime;

  LateNameInfo::extractParts(si->tableName(), tmpBuf, numParts, parts, FALSE);
  switch (numParts) {
     case 1:
        parts[2] = parts[0];
        parts[1] = (char *)"default";
        parts[0] = (char *)"HIVE";
        break;
     case 2:
        parts[2] = parts[1];
        parts[1] = parts[0];
        parts[0] = (char *)"HIVE";
        break;
     case 3:
        break;
     default:
        diagsArea << DgSqlCode(-24114);
        return ERROR;
  }
  if (stricmp(parts[1], "HIVE") == 0)
     parts[1] = (char *)"default";
  HVC_RetCode hvcRetcode = HiveClient_JNI::getRedefTime(parts[1], parts[2], redefTime);
  if (hvcRetcode == HVC_OK) {
     if (redefTime > si->modTS()) {
        simCheckFailed = TRUE;
        char errStr[strlen(si->tableName()) + 100 + strlen(si->hdfsRootDir())];
        snprintf(errStr,sizeof(errStr), 
               "compiledModTS = %ld, failedModTS = %ld, failedLoc = %s", 
               si->modTS(), redefTime, 
               si->hdfsRootDir());
        diagsArea << DgSqlCode(-EXE_HIVE_DATA_MOD_CHECK_ERROR)
                  << DgString0(errStr);
        NADELETEBASIC(tmpBuf, &heap_);
        return ERROR;
     }
  } else if (hvcRetcode == HVC_DONE) {
      char errBuf[strlen(si->tableName()) + 100 + strlen(si->hdfsRootDir())];
      snprintf(errBuf,sizeof(errBuf), "%s (fileLoc: %s)", si->tableName(), si->hdfsRootDir());
      diagsArea << DgSqlCode(-EXE_TABLE_NOT_FOUND)
                << DgString0(errBuf); 
      NADELETEBASIC(tmpBuf, &heap_);
      return ERROR;             
  } else {
     diagsArea << DgSqlCode(-1192)
          << DgString0("HiveClient_JNI::getRedefTime")
          << DgString1("")
          << DgInt0(hvcRetcode)
          << DgString2(getSqlJniErrorStr());
     NADELETEBASIC(tmpBuf, &heap_);
     return ERROR; 
  } 
  NADELETEBASIC(tmpBuf, &heap_);
  return SUCCESS;
}

RETCODE Statement::doQuerySimilarityCheck(TrafQuerySimilarityInfo * qsi,
					  NABoolean &simCheckFailed,
					  ComDiagsArea &diagsArea
					  )
{
  RETCODE retcode;

  simCheckFailed = FALSE;
  if ((! qsi) ||
      (qsi->disableSimCheck()) ||
      (! qsi->siList()) ||
      (qsi->siList()->numEntries() == 0))
    return SUCCESS;

  qsi->siList()->position();
  for (Lng32 i = 0; i < qsi->siList()->numEntries(); i++)
    {
      TrafSimilarityTableInfo *si = 
        (TrafSimilarityTableInfo *)qsi->siList()->getCurr();

      simCheckFailed = FALSE;
      if (si->isHive())
        {
          retcode = doHiveTableSimCheck(si,simCheckFailed, diagsArea);
          if (retcode == ERROR)
            {
              goto error_return; // diagsArea is set
            }
        }
    } // for

  return SUCCESS;
  
 error_return:
  return ERROR;
}

RETCODE Statement::fixup(CliGlobals * cliGlobals, Descriptor * input_desc,
			 ComDiagsArea &diagsArea, NABoolean &doSimCheck,
                         NABoolean &partitionUnavailable, const NABoolean donePrepare)
{
  Int32 retcode;
  ExMasterStats *masterStats;
 
  if ((stmtStats_) && ((masterStats = stmtStats_->getMasterStats()) != NULL))
     masterStats->setStmtState(STMT_FIXUP_);

  // Initialize this method's output parameters  
  doSimCheck = FALSE;
  partitionUnavailable = FALSE;

  if (fixupState() != 0)
    return ERROR;

  /* fixup the generated code */
  statementGlobals_->setStartAddr((void *)root_tdb);
  statementGlobals_->setCliGlobals(cliGlobals_);

  // Keep timeout data locally for this statement
  //  (The "root build" builds the messages to the ESPs by copying the 
  //   relevant timeout data from the master globals.)
  statementGlobals_->setLocalTimeoutData(root_tdb);

  // if this is part of an auto query retry and esp need to be cleaned up,
  // set that indication in root tcb.
  if ((context_->aqrInfo()) &&
      (context_->aqrInfo()->espCleanup()))
    {
      // set flag for verify ESPs - make sure esp in cache is alive before
      // reuse it.
      statementGlobals_->setVerifyESP();
    }

  root_tcb = (ex_root_tcb *)(root_tdb->build(cliGlobals, statementGlobals_));

  if ((context_->aqrInfo()) &&
      (context_->aqrInfo()->espCleanup()))
    {
      // reset flag for verify ESPs
      statementGlobals_->resetVerifyESP();
    }

  statementGlobals_->takeGlobalDiagsArea(diagsArea);

  if ((diagsArea.mainSQLCODE() < 0) || (!root_tcb))
    {
      if ((diagsArea.contains(-EXE_TIMESTAMP_MISMATCH) &&
           (NOT tsMismatched()))
          // if table not found, primary partition may have moved.
          // so, try a similarity check to verify it.
          || diagsArea.contains(-EXE_TABLE_NOT_FOUND))
	{
	  setTsMismatched(TRUE);
	  doSimCheck = TRUE;
	  return SUCCESS;
	}

      if (diagsArea.contains(-EXE_PARTITION_UNAVAILABLE))
          // if partition availability error, use the catalog manager
          // to attempt to find another partition.
	{
          partitionUnavailable = TRUE;
	  return SUCCESS;
	}

      setTsMismatched(FALSE);
      rollbackTransaction(diagsArea);
      return ERROR;
    }

  if (inputData_)
    {
      root_tcb->setInputData(inputData_);
    }
  
  // QStuff ^^
  if (isPubsubHoldable() || isAnsiHoldable())
    {
      root_tcb->propagateHoldable(TRUE);
    }
  // QStuff __

  retcode = root_tcb->fixup();

  // fixup is done. restore esp priority to its execute priority if master
  // is not changing esp's priority by sending msgs.
  short rc = statementGlobals_->getRtFragTable()->restoreEspPriority();
  if (rc)
    {
    }

  if (retcode)
    {
      statementGlobals_->takeGlobalDiagsArea(diagsArea);

      // do similarity check if timestamp mismatch is returned at
      // fixup time.
      if (((diagsArea.contains(-EXE_TIMESTAMP_MISMATCH)) &&
	   (NOT tsMismatched()))
          || (diagsArea.contains(-EXE_TABLE_NOT_FOUND)))
	{
	  setTsMismatched(TRUE);
	  doSimCheck = TRUE;
	  return SUCCESS;
	}
      
      if (diagsArea.contains(-EXE_PARTITION_UNAVAILABLE))
          // if partition availability error, use the catalog manager
          // to attempt to find another partition.
	{
          partitionUnavailable = TRUE;
	  return SUCCESS;
	}

      setTsMismatched(FALSE);
      rollbackTransaction(diagsArea);
      return ERROR;
    }
  
  setTsMismatched(FALSE);

  // if this is an 'update where current of' query, and
  // the cursor statement was specified as a literal,
  // then find the cursor statement and hook it up here.
  if (root_tdb->updateCurrentOfQuery() &&
      root_tdb->fetchedCursorName())
    {
      currentOfCursorStatement_ =
	getCurrentOfCursorStatement(root_tdb->fetchedCursorName());
      
      if (currentOfCursorStatement_ == NULL)
	{
	  diagsArea << DgSqlCode(- CLI_NON_UPDATABLE_SELECT_CURSOR)
		    << DgString0(root_tdb->fetchedCursorName());
	  return ERROR;
	}
    }

  setFixupState(-1);

  return SUCCESS;
}

static
NABoolean compareTransModes(ex_root_tdb *root_tdb,
			    ExTransaction *currTransaction,
			    ComDiagsArea *diags = NULL,
                            NABoolean isSysModuleStmt = FALSE)
{
  TransMode
    &tmCompile = *root_tdb->getTransMode(),
    &tmRuntime = *currTransaction->getTransMode();

#ifdef _DEBUG
    static NABoolean dbg = !!getenv("TRANSMODE_DEBUG");
    if (dbg)
      cerr << "##tm: exe= " << tmRuntime.display()
           <<      " cmp= " << tmCompile.display()
           <<           " " << tmCompile.stmtLevelAccessOptions()
	   <<           " " << root_tdb->readonlyTransactionOK()
	   << endl;
#endif

  NABoolean recompileDueToRollbackMode = 
	( 
	  ((tmCompile.getRollbackMode() == TransMode::NO_ROLLBACK_IN_IUD_STATEMENT_) &&
	    (!currTransaction->autoCommit()))
	  ||
	  (((tmCompile.getRollbackMode() != TransMode::NO_ROLLBACK_IN_IUD_STATEMENT_) &&
	      (tmCompile.getRollbackMode() != TransMode::NO_ROLLBACK_)) &&
	    (tmRuntime.getRollbackMode() == TransMode::NO_ROLLBACK_))
	 ) ;

  NABoolean recompileDueToMultiCommitMode = !(tmCompile.multiCommitCompatible(tmRuntime));

  if (tmCompile.accessModeCompatible(tmRuntime) ||
      root_tdb->readonlyTransactionOK())
  {
    if (tmCompile.isolationLevelCompatible(tmRuntime) ||
        tmCompile.stmtLevelAccessOptions())
    {
      if( !recompileDueToRollbackMode && !recompileDueToMultiCommitMode)
	 return FALSE;
    }
  }

  // The run-time transaction mode ($0~int0) differs the compile-time ($1~int1).
  if (diags && !isSysModuleStmt)
    *diags << DgSqlCode(- CLI_TRANS_MODE_MISMATCH)
      << DgInt0(tmRuntime.display())
      << DgInt1(tmCompile.display())
      ;
  return TRUE;
}

inline static
void recompileReasonIsTransMode(ex_root_tdb *root_tdb,
				ExTransaction *currTransaction,
			 Lng32 recompileReason[])
{
  recompileReason[0] = CLI_TRANS_MODE_MISMATCH;
  recompileReason[1] = currTransaction->getTransMode()->display();
  recompileReason[2] = root_tdb->getTransMode()->display();
}

RETCODE Statement::error(ComDiagsArea &diagsArea)
{
  // Reset the autocommit flag to the user's view.
  resetAutoCommit();

  // abort the transaction, if auto commit is on or
  // rollbackTransaction flag is set in the diags area or
  // this is a query that would have dirtied the disk(like,
  // insert/update/delete). 
  rollbackTransaction(diagsArea);
  
  // if no other diags, as a last resort emit this rather 
  // cryptic message
  if (diagsArea.getNumber(DgSqlCode::ERROR_) == 0)
    diagsArea << DgSqlCode(- CLI_TCB_EXECUTE_ERROR);
  
  dealloc();
  space_.freeBlocks();
  statementGlobals_->reAllocate(1);
  
  setState(CLOSE_);
  
  setFixupState(0);
  
  return ERROR;
}

RETCODE Statement::execute(CliGlobals * cliGlobals, Descriptor * input_desc,
			   ComDiagsArea &diagsArea, ExecState  execute_state,
			   NABoolean fixupOnly, ULng32 cliflags)
{
  StmtDebug2("[BEGIN execute] %p, stmt_state %s", this, stmtState(getState()));

  RETCODE retcode = SUCCESS;
  NABoolean schemaFileLabelTSChecked = 0;

  NABoolean recompWarn;
  NABoolean partitionUnavailable = FALSE;
  NABoolean partitionAvailabilityChecked = FALSE;

  // This boolean indicates whether ANSI to Guardian name mappings
  // have failed following a "partition not available" error. The
  // first time we encounter such a failure we attempt an automatic
  // recompile and set this variable to TRUE. If the same failure is
  // encountered again after the recompile then we return an error to
  // the user.
  NABoolean partitionNameLookupsFailed = FALSE;

  // This boolean indicates whether a prepare has been done as part 
  // of this execution. If that is the case, visibility checks can
  // be skipped because the prepare will have done that already.
  NABoolean donePrepare = FALSE;

  Lng32 recompileReason[3];
  recompileReason[0] = recompileReason[1] = recompileReason[2] = 0;
  Int64 reCompileTime = (Int64)0;
  NABoolean reExecute=FALSE;
  ExMasterStats *masterStats = NULL;

  if (state_ != FIXUP_DONE_)
    state_ = execute_state;
  
  // To ensure correct handling of the implicit transactions (and
  // autocommit clearing and restoring) which happen when the CLI
  // makes recursive calls via CatMapAnsiNameToGuardianName,
  // CatMapGetCatalogVisibility, RTMD fetches, etc, please be sure to
  // call Statement::commitImplicitTransAndResetTmodes before
  // leaving this function.  So it is best not to code a "return" from
  // inside this while loop, but instead, set the retcode local
  // variable, and set the readyToReturnVariable to TRUE, then break
  // from the switch statement.

  NABoolean readyToReturn = FALSE;
  if (stmtStats_ != NULL) 
     masterStats = stmtStats_->getMasterStats();
  while (readyToReturn == FALSE)  
    {
#ifdef _DEBUG
      if (getenv("SHOW_STATE"))
	{
	  char buf[40];

	  switch (state_)
	    {
	    case INITIAL_STATE_: strcpy(buf, "INITIAL_STATE_"); break;
	    case DO_SIM_CHECK_: strcpy(buf, "DO_SIM_CHECK_"); break;
	    case CHECK_DYNAMIC_SETTINGS_: strcpy(buf, "CHECK_DYNAMIC_SETTINGS_"); break;
	    case FIXUP_: strcpy(buf, "FIXUP_"); break;
	    case FIXUP_DONE_: strcpy(buf, "FIXUP_DONE_"); break;
	    case EXECUTE_: strcpy(buf, "EXECUTE_"); break;
	    case ERROR_: strcpy(buf, "ERROR_"); break;
	    case ERROR_RETURN_: strcpy(buf, "ERROR_RETURN_"); break;
	    default: strcpy(buf, "Unknown state!"); break;
	    }
	  cout << "State " << buf << endl;
	}
#endif

      switch (state_)
	{
	case INITIAL_STATE_:
	  {
            // Reclaim Statements if it is available only when the 
            // parent statements are executed
            if (context_->getNumOfCliCalls() == 1)
            {
              // Reclaim Statements if it is available
              context_->reclaimStatements();
            }
            if (masterStats != NULL)
            {
              Int64 jts = NA_JulianTimestamp();
	      if (NOT masterStats->isPrepAndExec() && (!fixupOnly))
	      {
	        masterStats->setElapsedStartTime(jts);
	      }
              if (! stmtStats_->aqrInProgress())
                aqrInitialExeStartTime_ = -1;
              if (! fixupOnly)
                 masterStats->setExeStartTime(aqrInitialExeStartTime_ == -1 ? 
                    jts : aqrInitialExeStartTime_);
	      masterStats->initBeforeExecute(jts);
              if (! stmtStats_->aqrInProgress())
                masterStats->resetAqrInfo();
	      }

	    if (stmt_state != CLOSE_)
	      {
		// Report an error because we are trying to execute a
		// statement which is already in an open state, or one
		// that was never prepared. One exception is a stored
		// procedure result set in the INITIAL_ state. It can
		// be described and executed without the application
		// first having done a prepare.
                if (!(parentCall_ && stmt_state == INITIAL_))
                {
		//ADebug();
		diagsArea << DgSqlCode(- CLI_STMT_NOT_CLOSE);
		state_ = ERROR_;
		statementGlobals_->setCancelState(CLI_CANCEL_TCB_READY);
		break;
	      }
              }

	    if (aqrReprepareNeeded())
	      {
		diagsArea << DgSqlCode(-EXE_USER_PREPARE_NEEDED);
		state_ = ERROR_;
		break;
	      }

	    if (!root_tdb &&
		!allocated() &&
		(statement_id->name_mode == cursor_name) &&
		(statement_id->identifier) &&
		(!isEqualByName((SQLSTMT_ID *)statement_id, cursor_name_))
		)
	      {
		SQLSTMT_ID tmpStmtId = *(SQLSTMT_ID *)statement_id;
		tmpStmtId.name_mode = stmt_name;
		Statement * bindToStmt =
		  context_->getStatement(&tmpStmtId);
		if (!bindToStmt)
		  {
		    diagsArea << DgSqlCode(-CLI_STMT_NOT_EXISTS);
		    state_ = ERROR_;
		    break;
		  }
		bindTo(bindToStmt);
	      }

            // Two cases to consider for CALL statements
            // a) This is a CALL statement. We need to reset the state
            //    of all child result sets before doing this execute. 
            //    The state should have already been reset when the
            //    CALL was closed but we do it again to be safe.
            // b) This is a stored procedure result set. We may need to
            //    to trigger an internal prepare before doing this
            //    execute. We also need to prevent a result set from
            //    being opened multiple times following a single 
            //    execution of the CALL.
            ExRsInfo *rsInfo = getResultSetInfo();
            if (rsInfo)
            {
              // Case a)
              rsInfo->reset();
            }
            if (parentCall_)
            {
              // Case b)
              ExRsInfo *parentRsInfo = parentCall_->getResultSetInfo();
              ex_assert(parentRsInfo, "No parent RS info available");

              ULng32 myIndex = parentRsInfo->getIndex(this);
              NABoolean openWasAttempted =
                parentRsInfo->openAttempted(myIndex);
              
              if (!openWasAttempted)
              {
                parentRsInfo->setOpenAttempted(myIndex);
              }
              else
              {
                diagsArea << DgSqlCode(-EXE_UDR_RS_REOPEN_NOT_ALLOWED)
                          << DgInt0((Lng32) myIndex);
                state_ = ERROR_;
                break;
              }
              
              RETCODE proxyRetcode = rsProxyPrepare(*parentRsInfo, myIndex,
                                                    diagsArea);
              if (proxyRetcode == ERROR)
              {
                state_ = ERROR_;
                break;
              }
            }
  
	    // Recompile if this stmt did not get compiled at 
	    // compilation time,
	    // or if the current transaction mode is different than the 
	    // mode specified at compile time.
	    if (! root_tdb)
	      {
		// A special case:
		// if DISPLAY query and no root_tdb, return. In this
		// case user does not want to execute the query.
		if (isDISPLAY()) 
		  {
			// change state so fetch can 'succeed' with an EOF.
			stmt_state = EOF_;
                        retcode = SUCCESS;
                        readyToReturn = TRUE;
                        break;
		  }

		// AQR has been enabled.
		if ((allocated()) &&
		    (context_->getSessionDefaults()->getAqrType() != 0))
		  {
		    // return error. AQR will handle recompile and retry.
		    diagsArea << DgSqlCode(-8583);
		    state_ = ERROR_;
		    break;
		  }

		state_ = ERROR_;
		break;
	      }
	    else if ((root_tdb->transactionReqd() || root_tdb->isLRUOperation())
		     &&
		     compareTransModes(root_tdb, context_->getTransaction(),
				       (root_tdb->aqrEnabled() ? &diagsArea : NULL))
		     &&
              !systemModuleStmt() )
	      {
		if (root_tdb->aqrEnabled())
		  {
		    // return error. AQR will handle recompile and retry.
		    state_ = ERROR_;
		  }
		else
		  {
                    state_ = ERROR_;
		  }
		break;
	      }

              if (root_tdb->inMemoryObjectDefn())
	      {
		// trying to executed a query which refers to an inMemory
		// object definition.
		// Return error.
                diagsArea << DgSqlCode(-CLI_CANNOT_EXECUTE_IN_MEM_DEFN);
                state_ = ERROR_;
                break;
	      }

            // if statistics were previously returned for this statement
            // (could happen for multiple executions of the same prepared
            //  or an embedded static statement), then re-initizalize the stat
            // area.	    
            ExStatisticsArea *statsArea = getStatsArea();
            if (statsArea != NULL)
            {
              StatsGlobals *statsGlobals = cliGlobals->getStatsGlobals();
              if (statsGlobals != NULL)
              {
                int error = statsGlobals->getStatsSemaphore(cliGlobals->getSemId(),
						      cliGlobals->myPin());
                statsArea->initEntries();
                statsArea->restoreDop();
                statsGlobals->releaseStatsSemaphore(cliGlobals->getSemId(),cliGlobals->myPin());
              }
              else
              {
                statsArea->initEntries();
                statsArea->restoreDop();
              }
            }
            if (stmtStats_ != NULL)
              stmtStats_->setAqrInProgress(FALSE);
            // LRU cannot run in a user transaction
            if (root_tdb && root_tdb->isLRUOperation() && context_->getTransaction()->xnInProgress())
	      {
                diagsArea << DgSqlCode(-EXE_CANT_BEGIN_USER_TRANS_WITH_LRU);
		retcode = ERROR;
	        state_ = ERROR_RETURN_;
                break;
	      }
	    
            // Start a transaction, if needed and one not already running.
	    //			if(!context_->getSuppressAutoXactStartFlag())
	    if ((NOT fixupOnly) ||
		(cliflags & PREPARE_STANDALONE_QUERY))
	      {
		if (beginTransaction(diagsArea))
		  {
		    retcode = ERROR;
		    state_ = ERROR_RETURN_;
		    break;
		  }
	      }

	    setTsMismatched(FALSE);
            state_ = DO_SIM_CHECK_;
	  }
	break;

        case DO_SIM_CHECK_:
          {
            if ((! root_tdb) || (! root_tdb->querySimilarityInfo()))
              {
                state_ = CHECK_DYNAMIC_SETTINGS_;
                break;
              }

            NABoolean simCheckFailed = FALSE;
            retcode =
              doQuerySimilarityCheck(root_tdb->querySimilarityInfo(),
                                     simCheckFailed, diagsArea);
            if (retcode == ERROR)
              {
                state_ = ERROR_;
                break;
              }
            
            state_ = CHECK_DYNAMIC_SETTINGS_;
          }
        break;

        case CHECK_DYNAMIC_SETTINGS_:
        {
          if (fixupState())
          {
            // If this fixed up statement was affected by SET TIMEOUT
            // or by a change in UDR runtime options then we want to
            // make sure it gets fixed up again
            if (statementGlobals_->timeoutSettingChanged() ||
                statementGlobals_->udrRuntimeOptionsChanged())
            {
              setFixupState(0);
            }
          }

          if (!fixupState())
            state_ = FIXUP_;
          else
            state_ = EXECUTE_;

        }
        break;
	
	case FIXUP_:
	  {
	    // We want to ignore any errors that occur as part of dealloc 
	    // when called from here.
            // So, we mark the DiagsArea before making the call to dealloc(),
	    // and then rewind
            // back to there afterwards.
            Lng32 oldDiagsAreaMark = diagsArea.mark();
            Lng32 oldGlobalDiagsAreaMark = 0;
            if (statementGlobals_->getDiagsArea())
	      {
		oldGlobalDiagsAreaMark = statementGlobals_->getDiagsArea()->mark();
	      }
            
            if (dealloc())
	      {
                //Leave the diagsArea as it is in case of error during the dealloc().
		state_ = ERROR_;
		break;
	      }
	    
            // Rewind to ignore all errors that occurred during a successful dealloc()
            if (statementGlobals_->getDiagsArea())
	      {
		statementGlobals_->getDiagsArea()->rewind(oldGlobalDiagsAreaMark, TRUE);
	      }
            diagsArea.rewind(oldDiagsAreaMark, TRUE);
	    space_.freeBlocks();
	    
	    statementGlobals_->reAllocate(1);
	    
	    setState(CLOSE_);
	    
	    setFixupState(0);

            StmtDebug1("[BEGIN fixup] %p", this);

	    NABoolean doSimCheck = FALSE;
	    retcode =
	      fixup(cliGlobals, input_desc, diagsArea, doSimCheck,
                      partitionUnavailable, donePrepare);


            StmtDebug2("[END fixup] %p, result is %s", this,
                       RetcodeToString(retcode));

	    if (((fixupOnly) ||
		 (root_tdb->aqrEnabled())) &&
		((retcode == ERROR) ||
		 (diagsArea.mainSQLCODE() < 0) ||
		 (doSimCheck)))
	      {
		state_ = ERROR_;
		break;
	      }

	    if ((retcode == ERROR) || 
                (partitionUnavailable && partitionAvailabilityChecked))
	      {
                state_ = ERROR_;
		break;
	      }

	    if (doSimCheck || partitionUnavailable)
            {
	      state_ = DO_SIM_CHECK_;
            }
	    else
            {
              state_ = EXECUTE_;

              if (fixupOnly)
		{		   		  
		  if  (NOT(cliflags & PREPARE_STANDALONE_QUERY))
		    {
		      // if an explicitly prepared query, commit any
		      // transaction that was started during fixup
		      // stage. We don't want to hold onto this Xn
		      // between this stmt's prepare and its execute.
		      // Later, when this query is executed, a
		      // transaction would be started.

                      // Wait for UDR transactional replies before
                      // attempting to commit
                      NABoolean allRequests = FALSE;
                      completeUdrRequests(allRequests);

		      // The following call will only reset autoCommit
		      // which is needed for the commitTransaction()
		      // to actually commit.
		      resetAutoCommit(); 

		      // now call commitTransaction since autoCommit has been 
		      // reset
		      if (commitTransaction(diagsArea))
			{
			  state_ = ERROR_;
			  retcode = ERROR;
			  readyToReturn = TRUE;
			  break;
			}
		      retcode = SUCCESS;
		      readyToReturn = TRUE;
		      state_ = FIXUP_DONE_;
		    
		    }
		  else
		    {
		      retcode = SUCCESS;
		      readyToReturn = TRUE;
		      state_ = INITIAL_STATE_;
		    }
                  
		} // if (fixupOnly && (state_ != ERROR_))
            } // if (doSimCheck || partitionUnavailable) else ...
            
            // Transition to the ERROR_ state if errors were recorded
            // in statement globals. Also set readyToReturn to FALSE
            // so we don't return out of this method before processing
            // the state transition.
            if (state_ != ERROR_)
            {
              ComDiagsArea *stmtGlobDiags = statementGlobals_->getDiagsArea();
              if (stmtGlobDiags && stmtGlobDiags->mainSQLCODE() < 0)
              {
                statementGlobals_->takeGlobalDiagsArea(diagsArea);
                state_ = ERROR_;
                readyToReturn = FALSE;
              }
            }
            
	  } // case FIXUP_
	break;
	
	case FIXUP_DONE_:
	  {
	    // Begin a transaction before going to EXECUTE_ state since
	    // the transaction may have been committed
	    if (beginTransaction(diagsArea))
	      {
		retcode = ERROR;
		state_ = ERROR_RETURN_;
		break;
	      }
	    if (masterStats != NULL)
	      {
	       Int64 jts = NA_JulianTimestamp();
                if (NOT masterStats->isPrepAndExec())
	        {
                  masterStats->setElapsedStartTime(jts);
                }
                masterStats->setExeStartTime(aqrInitialExeStartTime_ == -1
                     ? jts : aqrInitialExeStartTime_);
              }
	    state_ = EXECUTE_;
	  }
	  break;

	case EXECUTE_:
	  {
	    if (masterStats != NULL)
	      {
		masterStats->
		  setFixupEndTime(NA_JulianTimestamp());
                if (!masterStats->getValidDDL())
                  {
                    diagsArea << DgSqlCode(-CLI_DDL_REDEFINED);
                    state_ = ERROR_;
                    break;
                  }
                if (!masterStats->getValidPrivs())
                  {
                    diagsArea << DgSqlCode(-CLI_INVALID_QUERY_PRIVS);
                    state_ = ERROR_;
                    break;
                  }
              }
            // In case of master, the unused memory quota needs to be reset
            // with every statement execution. 
            statementGlobals_->resetMemoryQuota();
	    /* execute it */
            if( root_tdb )
            {            
                // check if we have triggers
                if (root_tdb->getTriggersCount() > 0)
                {
                  retcode =
                    getTriggersStatus(root_tdb->stoiStoiList(), diagsArea);
                  if (retcode == ERROR)
                  {
                    state_ = ERROR_;
                    break;                
                  }
                }

                // check for statements with uninitialized mvs          
                char * pMvName;
                if( doesUninitializedMvExist( &pMvName, diagsArea ) )
                {                                
                    diagsArea << DgSqlCode(- CLI_MV_EXECUTE_UNINITIALIZED)
                              << DgTableName( (const char *)pMvName);
                    state_ = ERROR_;
                    break;                
                }
            } 
          
            // in case this is a holdable cursor propagate flag to master executor
            // leaf nodes
            if (isPubsubHoldable() &&
		!root_tdb->isEmbeddedUpdateOrDelete() &&
		!root_tdb->isStreamScan())
	      {
                // Holdable cursors are only supported for streaming cursors
		// and destructive cursors because there are some strange
		// side-effects (locks disappear after commit) that we don't
		// want to inflict on non-Publich/Subscribe functionality at
		// this time. There are no source-code changes needed to allow
		// holdable cursors in these other cases though.
                diagsArea << DgSqlCode(-CLI_CURSOR_CANNOT_BE_HOLDABLE);
                state_ = ERROR_;
                break;
              }
            // BertBert ^^
           
	    ComDiagsArea* diagsPtr = NULL;
	    
	    // if this is an 'update where current of' query, and
	    // the cursor statement name was specified via a hvar,
	    // then find the cursor statement and hook it up here.
	    // In this case, fetched statement has to be searched
	    // before each execute because the hvar value may 
	    // change between calls to Exec.
	    // Also validate that the tablename specified in the upd/del
	    // stmt is the same as the one in the declare cursor stmt.
	    if (root_tdb->updateCurrentOfQuery())
	      {
		if (root_tdb->fetchedCursorName() == NULL) // spec via hvar
		  {
		    // find the hvar that contains the cursor name in the 
		    // input desc list
		    Lng32 cursor_name = 0;
		    input_desc->getDescItem(root_tdb->fetchedCursorHvar(),
					    SQLDESC_VAR_PTR, &cursor_name, 
					    0, 0, 0, 0);
                    char * cursor_name_copy = NULL;
		    if (cursor_name == 0)
		      {
			diagsArea << DgSqlCode(-CLI_TCB_EXECUTE_ERROR);
			state_ = ERROR_;
			break;
		      }
		    else
		      {
                        Lng32 string_length = 0;
                        input_desc->getDescItem(root_tdb->fetchedCursorHvar(),
						SQLDESC_LENGTH, &string_length,
						0, 0, 0, 0);
                        cursor_name_copy = 
                        Descriptor::getCharDataFromCharHostVar
                          (diagsArea,
			   heap_,
                           (char *)((long)cursor_name), 
                           string_length,
			   "CURSOR NAME", 
                           input_desc,
                           root_tdb->fetchedCursorHvar() - 1,  // Pass a zero-based index.
                           REC_BYTE_V_ANSI);
		      }
		    
		    currentOfCursorStatement_ =
		      getCurrentOfCursorStatement((char *)cursor_name_copy);
		    if (cursor_name_copy) heap_.deallocateMemory(cursor_name_copy);

		    if (currentOfCursorStatement_ == NULL)
		      {
			diagsArea << DgSqlCode(- CLI_NON_UPDATABLE_SELECT_CURSOR)
				  << DgString0((char *)((long)cursor_name));
			state_ = ERROR_;
			break;
		      }
		  }

		// make sure that the table name in the update/del stmt is the
		// same as the tablename specified in the cursor.
		ex_root_tdb *cursorTdb = currentOfCursorStatement_->getRootTdb();
		Int16 cursorTableNameLen = 
		  str_len( cursorTdb->getLateNameInfoList()->
						    getLateNameInfo(cursorTdb->baseTablenamePosition()).
						    lastUsedAnsiName());
		
		Int16 updelTableNameLen = 
		  str_len( root_tdb->getLateNameInfoList()->
			   getLateNameInfo(root_tdb->baseTablenamePosition()).
			   lastUsedAnsiName());
		if ( (cursorTableNameLen != updelTableNameLen ) || 

		     (str_cmp(root_tdb->getLateNameInfoList()->
			   getLateNameInfo(root_tdb->baseTablenamePosition()).
			   lastUsedAnsiName(), 
			   cursorTdb->getLateNameInfoList()->
			   getLateNameInfo(cursorTdb->baseTablenamePosition()).
                           lastUsedAnsiName(), 
			   updelTableNameLen
			   ) != 0) 
		    )
		  {
		    diagsArea << DgSqlCode(- CLI_NON_CURSOR_UPDEL_TABLE)
			      << DgString0(root_tdb->fetchedCursorName());
		    retcode = ERROR;
		    state_ = ERROR_RETURN_;
		    //                    readyToReturn = TRUE;
                    break;
		  }
	      }	   
 
	    // Get the row of primary key values from the referenced cursor and
	    // move it to my root tcb and ensure the update columns are valid.
	    if (root_tdb->updateCurrentOfQuery())
	      {
		if (currentOfCursorStatement_ == NULL)
		  {
		    diagsArea << DgSqlCode(- CLI_NON_UPDATABLE_SELECT_CURSOR);
		    state_ = ERROR_;
		    break;
		  }
		
		if (currentOfCursorStatement_->getState() != FETCH_)
		  {
		    // Must have fetched a row to do an update...current of.
		    // Add a better error message here. TBD.
		    diagsArea << DgSqlCode(- EXE_CURSOR_NOT_FETCHED);
		    state_ = ERROR_;
		    break;
		  }

		if (currentOfCursorStatement_->isDeletedCursor())
		  {
		    // Must have fetched a row to do an update...current of.
		    // Cannot update or delete a row that was previously
		    // deleted using this cursor.
		    diagsArea << DgSqlCode(- EXE_CURSOR_NOT_FETCHED);
		    state_ = ERROR_;
		    break;
		  }

		char * pkeyRow = 
		  currentOfCursorStatement_->getRootTcb()->getPkeyRow();
		if (pkeyRow == NULL)
		  {
		    // Add a better error message here. TBD.
		    diagsArea << DgSqlCode(- CLI_TCB_EXECUTE_ERROR);
		    state_ = ERROR_;
		    break;
		  }
		
		root_tcb->inputPkeyRow(pkeyRow);
		
		if (!currentOfCursorStatement_->root_tdb->isUpdateCol(root_tdb))
		  {
		    diagsArea << DgSqlCode(- CLI_INVALID_UPDATE_COLUMN);
		    state_ = ERROR_;
		    break;
		  }
	      }

	     // NOT_ATOMIC_FAILURE_LIMIT was specified through the statement attribute
	     // override any value we get from the CQD
	     if ((getNotAtomicFailureLimit() != 0) && 
	         (root_tdb->isNonFatalErrorTolerated())) {
	      root_tdb->setNotAtomicFailureLimit(getNotAtomicFailureLimit());
	     }
	    
	    // nothing happened yet.
            statementGlobals_->setGlobDiagsArea(0);
	    statementGlobals_->setRowsAffected(0);
	    diagsArea.setRowCount(0);
	    
            // Now that the statement is completely name-resolved, 
            // catalog-visiblity-checked, fixed up, compiled, recompiled, 
            // etc, etc, see if we started an implicit transaction to
            // perform any of these tasks and if so, commit it so that 
            // the statement can run without a trans (since if *we*
            // started the trans then this must be a READ UNCOMMITTED
            // statement.  Also, reset the autocommit setting if we 
            // had to temporarily turn it off.
            commitImplicitTransAndResetTmodes();


	    // before executing the statement, change master priority to
	    // be the same as ESP priority.
            SessionDefaults *sessionDefaults =
              statementGlobals_->getContext()->getSessionDefaults();
	    if (sessionDefaults->getAltpriMaster() ||
		sessionDefaults->getAltpriMasterSeqExe())
	      {
		// if session default altpri_master is set, then:
		//   -- always altpr parallel queries
		//   -- altpri sequential queries if ALTPRI_MASTER is set in 
		//      root tdb
		ExRtFragTable *fragTable = statementGlobals_->getRtFragTable();
                if (sessionDefaults->getAltpriMasterSeqExe() ||
		    (fragTable &&
                     fragTable->getState() != ExRtFragTable::NO_ESPS_USED))
		  {
		    IpcPriority myPriority = statementGlobals_->getMyProcessPriority();
		    IpcPriority espPriority;
		    if (sessionDefaults->getEspPriority() > 0)
		      espPriority = sessionDefaults->getEspPriority();
		    else if (sessionDefaults->getEspPriorityDelta() != 0)
		      espPriority = myPriority +
                        sessionDefaults->getEspPriorityDelta();
		    else
		      espPriority = myPriority;
		    
		    if ((espPriority > 200) ||
			(espPriority < 1))
		      espPriority = myPriority;
		    
		    // change master priority to be the same as ESP.
		    // Do this only for root cli level
		    if ((context_->getNumOfCliCalls() == 1) &&
			(myPriority != espPriority))
		      {
			ComRtSetProcessPriority(espPriority, FALSE);
			statementGlobals_->getCliGlobals()->setPriorityChanged(TRUE);
		      }
		  }
              }
	    NABoolean parentIsCanceled = updateChildQid();
	    if (parentIsCanceled)
	    {
	      diagsArea << DgSqlCode(-EXE_CANCELED);
	      state_ = ERROR_;
	      break;
	    }
	    //decide if this query needs to be monitored by WMS
	    NABoolean monitorThisQuery = FALSE;
	    // If there is no parent qid then filter out certain query types before monitoring
	    if (!getParentQid())
	    {
	    	if ((root_tdb->getWmsMonitorQuery()) &&
	    			(root_tdb->getQueryType() != SQL_CONTROL) &&
	    			(root_tdb->getQueryType() != SQL_SET_TRANSACTION) &&
	    			(root_tdb->getQueryType() != SQL_SET_CATALOG) &&
	    			(root_tdb->getQueryType() != SQL_SET_SCHEMA) &&
	    			(root_tdb->getSubqueryType() != SQL_DESCRIBE_QUERY) &&
	    			(root_tdb->getQueryType() != SQL_SELECT_UNIQUE) &&
	    			(root_tdb->getQueryType() != SQL_INSERT_UNIQUE) &&
	    			(root_tdb->getQueryType() != SQL_UPDATE_UNIQUE) &&
	    			(root_tdb->getQueryType() != SQL_DELETE_UNIQUE) &&
	    			(root_tdb->getQueryType() != SQL_OTHER) &&
		                (stmt_type != STATIC_STMT) &&
		                (getUniqueStmtId())
	    	)
	    	{
	    		monitorThisQuery = TRUE;
			if (stmtStats_)
			  stmtStats_->setWMSMonitoredCliQuery(TRUE);
	    	}
	    }
	    else
	    {
	    	//There is a parent qid associated with this query

	    	// If the query is a call statement don't monitor. mxosrvr
	    	//will monitor all call statements as well as DML statements
	    	// issued by SPJ body.
	    	if ((root_tdb->getQueryType() == SQL_CALL_NO_RESULT_SETS) ||
	    			(root_tdb->getQueryType() == SQL_CALL_WITH_RESULT_SETS) ||
	    			(root_tdb->getQueryType() == SQL_SP_RESULT_SET))
	    	{
	    		monitorThisQuery = FALSE;
	    	}
	    	// If the CQD WMS_CHILD_MONITOR_QUERY is TRUE

	    	// For ExeUtil queries ,also check if the prepare flag is set
	    	// and only then monitor it.
	    	else if (root_tdb->getQueryType() == SQL_EXE_UTIL  )
	    	{
	    		if (root_tdb->getWmsChildMonitorQuery() && wmsMonitorQuery())
			  {
	    			monitorThisQuery = TRUE;
				if (stmtStats_)
				  stmtStats_->setWMSMonitoredCliQuery(TRUE);
			  }
	    	}
	    	else
	    		// These are queries issued by internal callers.
	    		//The prepare flag is not set for these so simply
	    		// filter out simple queries and monitor it.
	    		if ( root_tdb->getWmsChildMonitorQuery() &&
	    				(root_tdb->getQueryType() != SQL_CONTROL) &&
	    				(root_tdb->getQueryType() != SQL_SET_TRANSACTION) &&
	    				(root_tdb->getQueryType() != SQL_SET_CATALOG) &&
	    				(root_tdb->getQueryType() != SQL_SET_SCHEMA) &&
	    				(root_tdb->getSubqueryType() != SQL_DESCRIBE_QUERY) &&
	    				(root_tdb->getQueryType() != SQL_SELECT_UNIQUE) &&
	    				(root_tdb->getQueryType() != SQL_INSERT_UNIQUE) &&
	    				(root_tdb->getQueryType() != SQL_UPDATE_UNIQUE) &&
	    				(root_tdb->getQueryType() != SQL_DELETE_UNIQUE) &&
	    				(root_tdb->getQueryType() != SQL_OTHER) &&
			                (stmt_type != STATIC_STMT) &&
			                (getUniqueStmtId())
			     )
			  {
			    monitorThisQuery = TRUE;
			    if (stmtStats_)
			      stmtStats_->setWMSMonitoredCliQuery(TRUE);
			  }
	    }
	      
	    if (monitorThisQuery)
	      {
		SQL_QUERY_COST_INFO query_cost_info;
		SQL_QUERY_COMPILER_STATS_INFO query_comp_stats_info;
		query_cost_info.cpuTime   = 0;
		query_cost_info.ioTime    = 0;
		query_cost_info.msgTime   = 0;
		query_cost_info.idleTime  = 0;
		query_cost_info.totalTime = 0;
		query_cost_info.cardinality = 0;
		query_cost_info.estimatedTotalMem  = 0;
		query_cost_info.resourceUsage = 0;
                query_cost_info.maxCpuUsage = 0;
		if (getRootTdb())
		  {
		    if (getRootTdb()->getQueryCostInfo())
		      {
                        getRootTdb()->getQueryCostInfo()->translateToExternalFormat(&query_cost_info);
		      }
		
		  }
		else
		  query_cost_info.totalTime = getRootTdb()->getCost();

	      
		if (getRootTdb())
		  {
		    CompilerStatsInfo *cmpStatsInfo = 
		      getRootTdb()->getCompilerStatsInfo();

		    if (cmpStatsInfo)
		      {
			short xnNeeded = (transactionReqd() ? 1 : 0);
			cmpStatsInfo->translateToExternalFormat(&query_comp_stats_info,xnNeeded);
			// CompilationStatsData. 
			CompilationStatsData *cmpData = 
			  getRootTdb()->getCompilationStatsData();


			SQL_COMPILATION_STATS_DATA *query_cmp_data = 
			  &query_comp_stats_info.compilationStats;
  
			if( cmpData )
			  {
                           Int64 cmpStartTime = -1;
                           Int64 cmpEndTime = NA_JulianTimestamp();
                           if (masterStats != NULL)
                              cmpStartTime = masterStats->getCompStartTime();
			    cmpData->translateToExternalFormat(query_cmp_data,
                                        cmpStartTime, cmpEndTime);
                            setCompileEndTime(cmpEndTime);
			  }   
		      }

		  }
	      }	

            // done deciding if this query needs to be monitored and 
            // registered with WMS.
            // now execute it.
            if (masterStats != NULL)
              {
                masterStats->setIsBlocking();
                masterStats->setStmtState(STMT_EXECUTE_);
              }
            
            Int32 rc = root_tcb->execute(cliGlobals, statementGlobals_,
                                         input_desc, diagsPtr, reExecute);
            
            if (masterStats != NULL)
              masterStats->setNotBlocking();
            if (rc < 0)
              retcode = ERROR;
            // "diagsPtr" is modified by the foregoing call.
            // If "diagsPtr" is NULL, there are no diags to merge
            // into "diagsArea".  Otherwise, "diagsPtr" is not NULL and does
            // point to a diags area from which we: 1) avoid the SQL function
            // 2) copy the ComCondition objects over.  Then we decrement
            // the reference count to indicate we're done with that
            // ComDiagsArea.
	    
            if (diagsPtr)
              {
                diagsArea.mergeAfter(*diagsPtr);
                diagsPtr->decrRefCount();    
                diagsPtr = NULL;
              }
	    
            if (retcode == ERROR)
              {
                root_tcb->cancel(statementGlobals_,diagsPtr);
                state_ = ERROR_;
                break;
              }
            else
              {
                if (retcode == 0 && diagsArea.mainSQLCODE() > 0)
                  // It's a warning. So return 1. 
                  retcode = (RETCODE)1;
                setState(OPEN_);
                readyToReturn = TRUE;
                break;
              }
          }
        break;
          
	case RE_EXECUTE_:                
	  {
	    // if input descriptor was passed in at execute time, recreate 
	    // the input data
	    // Setting the boolean reExecute to true, will cause the
	    // inputData from the previous execution to be used since we
	    // have no input_desc anymore at this stage. 
	    if (inputData_)
	      heap_.deallocateMemory (inputData_) ;
	    inputData_ = NULL;
	    char * inputData;
	    root_tcb->getInputData(inputData, inputDatalen_);
	    if (inputData)
	      {
		inputData_ = (char*) heap_.allocateMemory (inputDatalen_) ;
		str_cpy_all(inputData_, inputData, (Lng32) inputDatalen_);
	      }
	    
	    setFixupState(0);
	    reExecute=TRUE;
            state_ = CHECK_DYNAMIC_SETTINGS_;
	  }
	break;

	case ERROR_:
	  {
	    retcode = error(diagsArea);
	    state_ = ERROR_RETURN_;
	  }
	break;
	
	case ERROR_RETURN_:
	  {
            readyToReturn = TRUE;
	  }
	break;
	
	default:
	  state_ = ERROR_;
	  break;

	} // switch
    } // while return 

  commitImplicitTransAndResetTmodes();

  StmtDebug2("[END execute] %p, result is %s", this,
             RetcodeToString(retcode));
  return retcode;

} // Statement::execute diags

RETCODE Statement::fetch(CliGlobals * cliGlobals, Descriptor * output_desc,
			 ComDiagsArea &diagsArea,
                         NABoolean newOperation)
{
  StmtDebug2("[BEGIN fetch] %p, stmt state %s", this, stmtState(getState()));

  Int32 timeout = 0;
  Int32 retcode = SUCCESS;
  if (stmt_state == CLOSE_)
    {
      // trying to fetch from a statement which is in CLOSE state
      diagsArea << DgSqlCode(- CLI_STMT_CLOSE);
      StmtDebug3("[END fetch] %p, result is %s, stmt state %s", this,
                 RetcodeToString(ERROR), stmtState(getState()));
      return ERROR;
    }

  if (stmt_state == EOF_) // already returned EOF once.
  {
    if (output_desc && output_desc->rowwiseRowsetEnabled())
      output_desc->setDescItem(0, SQLDESC_ROWSET_NUM_PROCESSED,
				       0, NULL);
    StmtDebug3("[END fetch] %p, result is %s, stmt state %s", this,
               RetcodeToString(SQL_EOF), stmtState(getState()));
    return SQL_EOF;
  }

  if (!root_tcb)
    {
    // trying to fetch from a statement with a null root_tcb;
    // probably, a bug elsewhere. For example, sqlc translates
    //   exec sql prepare S1 from :buf;
    // into
    //   if ((SQLCODE = SQL_EXEC_ClearDiagnostics(&__SQL_id0))  < 0
    //   ||(SQLCODE = SQL_EXEC_SetDescPointers
    //       (&__SQL_id1,1,1,&(buf[0]),NULL)) < 0
    //   ||(SQLCODE = SQL_EXEC_AllocStmt(&__SQL_id0,NULL)) < 0
    //   ||(SQLCODE = SQL_EXEC_Prepare(&__SQL_id0,&__SQL_id1)) < 0);
    // Note that Prepare() will be short-circuited if any of the preceding
    // CLI calls fail silently and the result can be a null root_tcb which
    // will crash arkcmp unless we bulletproof against FETCH S1!
    diagsArea << DgSqlCode(- CLI_INTERR_NULL_TCB);
    StmtDebug3("[END fetch] %p, result is %s, stmt state %s", this,
               RetcodeToString(ERROR), stmtState(getState()));
    return ERROR;
  }
 
  if (newOperation)
  {
    if ((isAnsiHoldable() || isPubsubHoldable()) && transactionReqd())
    {
      if (! context_->getTransaction()->xnInProgress())
      {
        if (beginTransaction(diagsArea))
          return ERROR;
      }
    }
  }
  else
  {
    if ((isAnsiHoldable() || isPubsubHoldable()) && transactionReqd())
    {
      if (! context_->getTransaction()->xnInProgress())
      {
        ex_assert(0, "Transaction is Not Valid");
      }
    }
  }
  timeout = -1 ;

  // Check for suspended query.  The other check called in the 
  // Scheduler::work.  We do it here simply because some users
  // expect if the query is suspended while the client is not 
  // blocked in SQL, the next fetch should not return any row.
  // Without this check, a row could be returned if it was ready,
  // since it would not be needed to call the Scheduler::work(). 
  statementGlobals_->getScheduler()->checkSuspendAndLimit();

  /*fetch row*/
  ComDiagsArea* diagsPtr = NULL;
  NABoolean closeCursorOnError = TRUE;
  ExMasterStats *masterStats =
    (stmtStats_ ? stmtStats_->getMasterStats() : NULL);
  if (masterStats)
    masterStats->setIsBlocking();
  if (output_desc && output_desc->rowwiseRowsetEnabled())
    {
      NABoolean eodSeen=FALSE;
      retcode = root_tcb->fetchMultiple(cliGlobals, statementGlobals_, 
					output_desc,
					diagsPtr, timeout, newOperation,
					closeCursorOnError,eodSeen);
      if ((retcode >0) && (eodSeen))
	{
	  // If a warning is returned along with EOD,we have to return the 
	  // warning retcode to MXCS otherwise they will not retrieve the diags
	  // But remember that eod was returned so we don't go through fetch 
	  // again when we are redriven
	  setState(EOF_);
	}
 StmtDebug1("  root_tcb->fetchMultiple() returned %s",
            RetcodeToString((RETCODE) retcode));

    }
  else
    {
      retcode = root_tcb->fetch(cliGlobals, statementGlobals_, output_desc,
				diagsPtr, timeout, newOperation,
				closeCursorOnError);
      if (output_desc && output_desc->rowwiseRowset())
	{
	  if ((retcode >= 0) &&
	      (retcode != 100))
	    {
	      output_desc->setDescItem(0, SQLDESC_ROWSET_NUM_PROCESSED,
				       1, NULL);
	    }
	  else
	    {
	      output_desc->setDescItem(0, SQLDESC_ROWSET_NUM_PROCESSED,
				       0, NULL);
	    }
	}

      StmtDebug1("  root_tcb->fetch() returned %s",
           RetcodeToString((RETCODE) retcode));

    }

  if (masterStats)
    masterStats->setNotBlocking();

  // "diagsPtr" is modified by the foregoing call.
  // If "diagsPtr" is NULL, there are no diags to merge
  // into "diagsArea".  Otherwise, "diagsPtr" is not NULL and does
  // point to a diags area from which we: 1) avoid the SQL function
  // 2) copy the ComCondition objects over.  Then we decrement
  // the reference count to indicate we're done with that ComDiagsArea.

  if (diagsPtr)
    {
      /*      if (diagsPtr->getNumber(DgSqlCode::WARNING_) > 0 &&
          diagsPtr->getNumber(DgSqlCode::ERROR_) == 0 &&
          retcode == ERROR &&
          diagsPtr->mainSQLCODE() >= 20000 &&
          diagsPtr->mainSQLCODE() <= 20999)
      {
        // Don't know why root_tcb->fetch returns retcode -1 (ERROR)
        // when the SQL/MX Utility internal stored procedure issues
        // warning messages but executes successfully otherwise
        // (i.e., error messages was issued).  Need to set retcode
        // to SQL_EOF so this routine does not issue the unwanted 
        // CLI_TCB_FETCH_ERROR message.  Cannot set retcode to either
        // SUCCESS or WARNING because mxci will issue the following
        // rows right after the warning messages if retcode is set
        // to SUCCESS or WARNING:
        //
        // RECOVERSTATUS
        // -------------
        //
        //     134936872
        //
        // If retcode is set to SQL_EOF, only the warning messages
        // will be printed by mxci.  Note that error/warning numbers
        // range between 20000 and 20999 are used by the MODIFY utility
        // and several other SQL/MX utilities (e.g., DUP, PURGEDATA).
        //
        retcode = SQL_EOF;
	}*/
      diagsArea.mergeAfter(*diagsPtr);
      diagsPtr->decrRefCount();    
      diagsPtr = NULL;
    }
  
  if (retcode < 0)
    {
      if (stmtStats_ != NULL && stmtStats_->getMasterStats() != NULL)
      {
        stmtStats_->getMasterStats()->setRowsAffected(statementGlobals_->getRowsAffected());
      }
      updateStatsAreaInContext();

      // Error case.
      
      // cancel the down request to my child.
      // Do it only if closeCursorOnError is set to TRUE, the cursor
      // remains open otherwise.

      if (closeCursorOnError)
	{
	  // We must do this to clean up the messages from the queues
	  // (Else there are messages left over and these are incorrectly
	  // read if the statement is re-executed.)
	  diagsPtr = NULL;
          if (root_tcb)
            // We could have deallocated the root tcb earlier
            retcode = root_tcb->cancel(statementGlobals_,diagsPtr);
            StmtDebug1("  root_tcb->cancel() returned %s",
                       RetcodeToString((RETCODE) retcode));

	  if (diagsPtr)
	    {
	      diagsArea.mergeAfter(*diagsPtr);
	      diagsPtr->decrRefCount();    
              diagsPtr = NULL;
	    }

          setState(CLOSE_);
	}

      // rollback savepoints, if they are being done.
      NABoolean doXnRollback = FALSE;
      rollbackSavepoint(diagsArea, doXnRollback);
      
      // abort the transaction, if auto commit is on or
      // rollbackTransaction flag is set in the diags area or
      // this is a query that would have dirtied the disk(like,
      // insert/update/delete). 
      if (rollbackTransaction(diagsArea, doXnRollback))
      {
        StmtDebug3("[END fetch] %p, result is %s, stmt state %s", this,
                   RetcodeToString(ERROR), stmtState(getState()));
        return ERROR;
      }

      // If fatal error, we need to start over again
      if (root_tcb && root_tcb->fatalErrorOccurred())
      {
        dealloc();
        space_.freeBlocks();
        statementGlobals_->reAllocate(1);
        setFixupState(0);
      }
      
      if (diagsArea.getNumber(DgSqlCode::ERROR_) == 0)
	{
	  // add an error indicating no error in diags. 
	  // We will aqr retry on this error.
	  diagsArea << DgSqlCode(-CLI_NO_ERROR_IN_DIAGS);
	}

      StmtDebug3("[END fetch] %p, result is %s, stmt state %s", this,
                 RetcodeToString(ERROR), stmtState(getState()));
      return ERROR;
    }

  if ( !newOperation 
       )
    {
      if (handleUpdDelCurrentOf(diagsArea) == SQL_EOF)
	  retcode = SQL_EOF;
    }

  if ((retcode != NOT_FINISHED) && (retcode != SQL_EOF) && 
      (getRootTdb()->isEmbeddedIUDWithLast1()))
      {
      	// Add the affected row count that was directly placed into
	// the master globals to those that are already in the diags
	// area.
	diagsArea.setRowCount(statementGlobals_->getRowsAffected());
      }

  if (retcode == WARNING)
    {
    StmtDebug3("[END fetch] %p, result is %s, stmt state %s", this,
		 RetcodeToString(WARNING), stmtState(getState()));
    return WARNING;
  }
  else if (retcode == SQL_EOF)
  {
    StmtDebug1("  [EOF] Fetched SQL_EOF for statement %p", this);
    
    if (!(getRootTdb()->isEmbeddedIUDWithLast1()))
      {
	// Add the affected row count that was directly placed into
	// the master globals to those that are already in the diags
	// area. Make both row counts (diags and statementGlobals_) the same.
	diagsArea.addRowCount(statementGlobals_->getRowsAffected());
	statementGlobals_->setRowsAffected(diagsArea.getRowCount());
      }
    if (stmtStats_ != NULL && stmtStats_->getMasterStats() != NULL)
    {
      stmtStats_->getMasterStats()->setRowsAffected(statementGlobals_->getRowsAffected());
    }
    updateStatsAreaInContext();
    
    setState(FETCH_);
    
    // If this is a stored procedure result set, do bookkeeping for
    // this EOF and if this is EOF for the final result set, then put
    // the parent CALL in an EOF state (which has the side effect of
    // ending an AUTOCOMMIT transaction that may be associated with
    // the CALL).
    if (parentCall_)
    {
      StmtDebug1("  [EOF] Parent CALL %p", parentCall_);

      ExRsInfo *parentRsInfo = parentCall_->getResultSetInfo();
      ex_assert(parentRsInfo, "No parent RS info available");
      
      ULng32 rsIndex = parentRsInfo->getIndex(this);
      parentRsInfo->setCloseAttempted(rsIndex);
      StmtDebug2("  [EOF] RS index %u, Num closed since CALL %u",
                 rsIndex, parentRsInfo->getNumClosedSinceLastCall());

      if (parentRsInfo->allResultsAreClosed())
      {
        RETCODE closeResult = parentCall_->close(diagsArea);
        StmtDebug1("  [EOF] parentCall_->close() returned %s",
                   RetcodeToString(closeResult));
        if (closeResult != ERROR)
        {
          parentCall_->setState(EOF_);
        }
        else
        {
          StmtDebug3("[END fetch] %p, result is %s, stmt state %s", this,
			     RetcodeToString(ERROR), stmtState(getState()));
          return ERROR;
        }
      }
    }
    
    StmtDebug0("  [EOF] About to call commitTransaction()...");
    
    // end the transaction, if auto commit is on.
    if (commitTransaction(diagsArea))
    {
      StmtDebug3("[END fetch] %p, result is %s, stmt state %s", this,
		     RetcodeToString(ERROR), stmtState(getState()));
      // In case there is a commit conflict, we need to reset the rowcount 
      // since none of the rows would have got committed. 
      diagsArea.setRowCount(0);
      stmtStats_->getMasterStats()->setRowsAffected(0);
      return ERROR;
    }

    StmtDebug0("  [EOF] Done. commitTransaction() was successful");

    setState(EOF_);
    
    if (output_desc)
    {
      // move EOF condition to diags area.
      diagsArea << DgSqlCode(SQL_EOF);
    }
    
    StmtDebug3("[END fetch] %p, result is %s, stmt state %s", this,
		 RetcodeToString(SQL_EOF), stmtState(getState()));
    return SQL_EOF;
  }

 
  else
    {
      // change priority back to fixup state, if asked for.
      if ((context_->getSessionDefaults()->getAltpriFirstFetch()) &&
	  (context_->getNumOfCliCalls() == 1) &&
	  (cliGlobals->priorityChanged()))
	{
	  ComRtSetProcessPriority(cliGlobals->myPriority(), FALSE);
	  cliGlobals->setPriorityChanged(FALSE);
	}

      setState(FETCH_);

      if (getRootTdb()->updatableSelect())
	resetDeletedCursor();

    StmtDebug3("[END fetch] %p, result is %s, stmt state %s", this,
		 RetcodeToString(SUCCESS), stmtState(getState()));
    return SUCCESS;
  }

} // Statement::fetch()

short Statement::handleUpdDelCurrentOf(ComDiagsArea &diags)
{
  short retcode = SUCCESS;

  // if this is an update, delete, or insert query and no rows were
  // affected and there are no warnings, return SQL_EOF. 
  // If there is a warning, return the warning.
  // This is an ANSI requirement.
  if (noRowsAffected(diags))
    {
      // if this is an "update/delete where current of" operator
      // and no rows were returned,
      // raise a warning. This is done since an absence of this
      // row at this point indicates that someone deleted this
      // row between the time it was fetched (using the
      // FETCH <cursor> INTO...  statement) and now.
      if ((isDeleteCurrentOf()) || (isUpdateCurrentOf()))
	{
	  diags << DgSqlCode(EXE_CURSOR_UPDATE_CONFLICT);
	}
	  
      retcode = ((diags.getNumber(DgSqlCode::WARNING_) > 0) ? 0 : SQL_EOF);
      // move EOF warning to diags area.
      if (retcode == SQL_EOF)
	diags << DgSqlCode(SQL_EOF);

    }
  else
    {
      // if delete where current of, mark the cursor that this fetched
      // row was deleted. This is to prevent another cursor delete/update
      // (ANSI positioned delete/update) on this deleted row.
      if (isDeleteCurrentOf())
	{
	  currentOfCursorStatement()->setDeletedCursor();
	}
    }
  return retcode;
}
  
void Statement::updateTModeValues()
{
  // Get the current transmode and update the values
  // for transaction typing.
  TransMode * cmpTransMode = root_tdb->getTransMode();
  TransMode * runTransMode = context_->getTransaction()->getTransMode();

  // In the unlikely event that we don't have a runtime transMode
  // just use the compiletimeTransMode.
  if (!runTransMode)
    runTransMode = cmpTransMode ;

  short roval = runTransMode->getAccessMode();
  short rbval = runTransMode->getRollbackMode();
  Lng32  aival = runTransMode->getAutoAbortIntervalInSeconds();

  // if AccessMode is not set at runtime then the the compile time 
  // setting is applied. Note that roval has an effect on the
  // transaction only if its value is READ_ONLY or READ_ONLY_SPECIFIED_BY_USER
  if ((roval != TransMode::READ_ONLY_) &&
      (roval != TransMode::READ_ONLY_SPECIFIED_BY_USER_))
	roval = cmpTransMode->getAccessMode();

  //if No rollback is specified through an IUD statement then it gets precedence
  // in compareTransMode we check that autocommit is on at runtime for this setting
  if (cmpTransMode->getRollbackMode() == TransMode::NO_ROLLBACK_IN_IUD_STATEMENT_)
    rbval = TransMode::NO_ROLLBACK_ ;

  // if AutoAbortInterval is not set at runtime then the the compile time 
  // setting is applied.
  if (aival == -1)
    aival = cmpTransMode->getAutoAbortIntervalInSeconds();
        
  context_->getTransaction()->updateROVal(roval);
  context_->getTransaction()->updateRBVal(rbval);
  context_->getTransaction()->updateAIVal(aival);

}


RETCODE Statement::doOltExecute(CliGlobals *cliGlobals,
				Descriptor * input_desc, 
				Descriptor * output_desc,
				ComDiagsArea &diagsArea,
				NABoolean &doNormalExecute,
				NABoolean &reExecute)
{
  Int32 retcode;

  if (!root_tdb)
    {
      diagsArea << DgSqlCode(-CLI_STMT_NOT_EXISTS);
      return error(diagsArea);
    }

  if (stmt_state != CLOSE_)
    {
      // trying to execute a statement which is already in open
      // state
      diagsArea << DgSqlCode(- CLI_STMT_NOT_CLOSE);
      return ERROR;
    }
  
  ExMasterStats * masterStats = NULL;
  if (root_tdb && getStmtStats() && 
      (NULL != (masterStats = getStmtStats()->getMasterStats())))
    {
      if (!masterStats->getValidDDL())
        {
          diagsArea << DgSqlCode(-CLI_DDL_REDEFINED);
          return ERROR;
        }
      if (!masterStats->getValidPrivs())
        {
          diagsArea << DgSqlCode(-CLI_INVALID_QUERY_PRIVS);
          return ERROR;
        }
      if (root_tdb->qCacheInfoIsClass() && root_tdb->qcInfo())
        masterStats->setCompilerCacheHit(
                            root_tdb->qcInfo()->cacheWasHit());
     Int64 jts = NA_JulianTimestamp();
     if (NOT masterStats->isPrepAndExec())
	 masterStats-> setElapsedStartTime(jts);
     masterStats->setFixupStartTime(-1);
     masterStats->setFreeupStartTime(-1);
     masterStats->setExeStartTime(aqrInitialExeStartTime_ == -1 ? jts :
                               aqrInitialExeStartTime_);
     masterStats->setSqlErrorCode(0); 
     masterStats->setStmtState(STMT_EXECUTE_);
   }

  // Get the current transmode and update the values
  // for transaction typing.  Code is here so values
  // will be updated for BEGIN WORK statements.
  updateTModeValues();

  // Start a transaction, if needed and one not already running.
  if (root_tdb->transactionReqd())
    {
      // A user started transaction must be present for olt execution.
      // if no user started transaction, then return and do normal
      // execution.
      if ((! context_->getTransaction()->xnInProgress()) ||
	  ((context_->getTransaction()->exeStartedXn()) &&
	   (context_->getTransaction()->implicitXn())))
	{
	  doNormalExecute = TRUE;
	  reExecute = FALSE;
	  return SUCCESS;
	}
      else
	{
          if (compareTransModes(root_tdb, context_->getTransaction(),
				(root_tdb->aqrEnabled() ? &diagsArea : NULL))
              && !systemModuleStmt() )
            {
              if (root_tdb->aqrEnabled())
                {
                  // return error. AQR will handle recompile and retry.
                  return(ERROR);
                }
              else
                {
                  doNormalExecute = TRUE;
                  reExecute = FALSE;
                  return SUCCESS;
                }
            }
          
          // move the transid from executor globals to statement globals,
	  // if a transaction is running.
	  statementGlobals_->getTransid() =
	    context_->getTransaction()->getExeXnId();

	  if (root_tdb->getUpdSavepointOnError())
	    {
	      // get a savepoint id which will be sent to DP2.
	      context_->getTransaction()->generateSavepointId();
	      
	      statementGlobals_->getSavepointId() =
		context_->getTransaction()->getSavepointId();
	    }
	}
    }
  
  ExStatisticsArea *statsArea = getStatsArea();
  if (statsArea != NULL)
    {
      StatsGlobals *statsGlobals = cliGlobals->getStatsGlobals();
      if (statsGlobals != NULL)
      {
        int error = statsGlobals->getStatsSemaphore(cliGlobals->getSemId(),
					      cliGlobals->myPin());
        statsArea->initEntries();
        statsArea->restoreDop();
        statsGlobals->releaseStatsSemaphore(cliGlobals->getSemId(),cliGlobals->myPin());
      }
      else
      {
        statsArea->initEntries();
        statsArea->restoreDop();
      }
   }

  // do similarity check 
  if (root_tdb && root_tdb->querySimilarityInfo())
    {
      NABoolean simCheckFailed = FALSE;
      retcode =
        doQuerySimilarityCheck(root_tdb->querySimilarityInfo(),
                               simCheckFailed, diagsArea);
      
      if (retcode == ERROR)
        {
          return ERROR;
        }
    }

  ComDiagsArea* diagsPtr = NULL;
  if (! fixupState()) // first time
    {
     if ((stmtStats_) && (stmtStats_->getMasterStats()))
      {
	stmtStats_->getMasterStats()->
	  setFixupStartTime(NA_JulianTimestamp());
      }
      if (dealloc())
        goto retError;
      
      space_.freeBlocks();
      
      statementGlobals_->reAllocate(1);
      
      setState(CLOSE_);
      
      setFixupState(0);

      NABoolean doSimCheck = FALSE;
      NABoolean partitionUnavailable = FALSE;
      retcode = fixup(cliGlobals, input_desc, diagsArea, doSimCheck, partitionUnavailable, FALSE);
      if (doSimCheck || partitionUnavailable)
	{
	  commitImplicitTransAndResetTmodes();
 
          // Redrive the execution if fixup said so.
          // It is OK to clear the diags area, because
          // normal execution will perform the visibility check again.
          diagsArea.clear();

	  setTsMismatched(FALSE);

	  doNormalExecute = TRUE;
	  reExecute = FALSE;
	  return SUCCESS;
	}
      if ((stmtStats_) && (stmtStats_->getMasterStats()))
      {
	stmtStats_->getMasterStats()->
	  setFixupEndTime(NA_JulianTimestamp());
      }

      commitImplicitTransAndResetTmodes();
      if (retcode == ERROR)
        goto retError;
     
    }

  /* execute it */

  // nothing happened yet.
  statementGlobals_->setGlobDiagsArea(0);
  statementGlobals_->setRowsAffected(0);
  setState(OPEN_);
  updateChildQid();
  if (masterStats)
     masterStats->setIsBlocking();
  retcode = root_tcb->oltExecute(statementGlobals_, input_desc, 
				 output_desc, diagsPtr);
  StmtDebug1("  root_tcb->oltExecute() returned %s",
             RetcodeToString((RETCODE) retcode));

  if (masterStats)
     masterStats->setNotBlocking(); 
  if (diagsPtr)
    {      
      NABoolean lostOpen = diagsPtr->contains(-EXE_LOST_OPEN);

      if (lostOpen)
        {
          NABoolean retryableStatement = root_tdb->retryableStmt();
          
          if (retryableStatement)
            {
	      retcode = root_tcb->cancel(statementGlobals_,diagsPtr);
	      StmtDebug1("  root_tcb->cancel() returned %s",
			 RetcodeToString((RETCODE) retcode));
	      
              setState(CLOSE_);
	      doNormalExecute = TRUE;
	      reExecute = TRUE;
	      return SUCCESS;
            }
          else
            {
              // lost the open on a non-retryable statement;
              // force a fresh fix-up on next execution.
              setFixupState(0);
            }
	}
      
      diagsArea.mergeAfter(*diagsPtr);
      diagsPtr->decrRefCount();    
      diagsPtr = NULL;
    }

  if (retcode >= 0 && root_tcb->anyCbMessages())
  {
    Int32 cancelRetcode = root_tcb->cancel(statementGlobals_,diagsPtr);
    StmtDebug1("  root_tcb->cancel() returned %s",
             RetcodeToString((RETCODE) cancelRetcode));

    setState(CLOSE_);
    releaseTransaction(TRUE);
  }

  updateStatsAreaInContext();
  if (retcode < 0)
    {
      // rollback savepoints, if they are being done.
      NABoolean doXnRollback = FALSE;
      rollbackSavepoint(diagsArea, doXnRollback);

      retcode = root_tcb->cancel(statementGlobals_,diagsPtr);
      StmtDebug1("  root_tcb->cancel() returned %s",
                 RetcodeToString((RETCODE) retcode));

      setState(CLOSE_);

      // abort the transaction, if auto commit is on or
      // rollbackTransaction flag is set in the diags area or
      // this is a query that would have dirtied the disk(like,
      // insert/update/delete). 
      if (rollbackTransaction(diagsArea, doXnRollback))
	return ERROR;
 
      // if no other diags, as a last resort emit this rather 
      // cryptic message
      if (diagsArea.getNumber(DgSqlCode::ERROR_) == 0)
	diagsArea << DgSqlCode(- CLI_TCB_EXECUTE_ERROR);
      
      return ERROR;
      //      return error(diagsArea);
    }
  else if (retcode == SUCCESS)
    return SUCCESS;
  else if (retcode == WARNING)
    return WARNING; 
  else
    {
      diagsArea.setRowCount(statementGlobals_->getRowsAffected());
      if (getStmtStats() && (getStmtStats()->getMasterStats()))
          stmtStats_->getMasterStats()->setRowsAffected(statementGlobals_->getRowsAffected());
      if ((statementGlobals_->getRowsAffected() > 0) ||
	  (root_tdb->thereIsACompoundStatement()))
	return SUCCESS;
      else 
	{
	  if ((output_desc) ||
	      (root_tdb && (root_tdb->updDelInsertQuery())))
	    {
	      diagsArea << DgSqlCode(SQL_EOF);
	      return SQL_EOF;
	    }
	  else
	    return SUCCESS;
	}
    }
retError:
    return error(diagsArea);
}

Lng32 Statement::cancel()
{  
  StmtDebug2("[BEGIN cancel] %p, stmt state %s", this, stmtState(getState()));

  CancelState s = statementGlobals_->getCancelState();
  Lng32 retcode = 0;

  switch (s)
  {
  case CLI_CANCEL_TCB_INVALID:
    // 
    statementGlobals_->setCancelState(CLI_CANCEL_REQUESTED);
    break;

  case CLI_CANCEL_TCB_READY:
    // request to cancel down the tcb tree. 
    root_tcb->requestCancel();
    break;
  
  case CLI_CANCEL_DISABLE:
    retcode = -CLI_CANCEL_REJECTED;
    break;

  default:
    // CLI_CANCEL_REQUESTED
    // ignore since an outstanding cancel request exists.
    break;
  }

  StmtDebug3("[END cancel] %p, result is %s, stmt state %s",
             this, RetcodeToString((RETCODE) retcode), stmtState(getState()));
  return retcode;
}

void Statement::releaseStats()
{
  ExStatisticsArea *myStats = getStatsArea();
  ExStatisticsArea *myOrigStatsArea = getOrigStatsArea();
  ExStatisticsArea *ctxStats = context_->getStats();
  ExStatisticsArea *newStats;
  StatsGlobals *statsGlobals = cliGlobals_->getStatsGlobals();
  if (ctxStats && (ctxStats == myStats) && 
        ((Int32)myStats->getCollectStatsType() != SQLCLI_NO_STATS))
  {
    if (statsGlobals != NULL && stmtStats_ != NULL &&
        (Int32)myStats->getCollectStatsType() != SQLCLI_ALL_STATS) 
    {
      int error = statsGlobals->getStatsSemaphore(cliGlobals_->getSemId(),
                            cliGlobals_->myPin());
      // Make sure the ex_globals doesn't delete this stats area
      // in case of dynamic statements, since stmtStats is also
      // pointing to the same area
      getGlobals()->setStatsArea(NULL);
      // Context is pointing to it. Remove query shouldn't
      // deallocate it
      stmtStats_->setStmtStatsUsed(TRUE);
      context_->setStatsArea(myStats, FALSE, TRUE, FALSE);
      // Set the StmtStats that can be used to reset the used flag
      context_->setPrevStmtStats(stmtStats_);
      statsGlobals->releaseStatsSemaphore(cliGlobals_->getSemId(),
                                cliGlobals_->myPin());
    }
    else
    {
      // Resources for this statement are being deallocated. If the
      // context is pointing to my stats area, make a new copy of the
      // stats area and attach it to context.
      newStats = new (context_->exHeap())
      ExStatisticsArea(context_->exHeap(), 0,
                       myStats->getCollectStatsType());
      newStats->setStatsEnabled(myStats->statsEnabled());
      newStats->merge(myStats);
      if (newStats->getMasterStats() == NULL && stmtStats_ != NULL 
                   && stmtStats_->getMasterStats() != NULL)
      {
        ExMasterStats * ems = new(context_->exHeap())
              ExMasterStats(context_->exHeap());
        ems->copyContents(stmtStats_->getMasterStats());
        newStats->setMasterStats(ems);
      }
      if (stmtStats_ != NULL)
         stmtStats_->setStatsArea(NULL);
      // Attach the new stats area to the context
      context_->setStatsArea(newStats, TRUE, FALSE, TRUE);
    }
  }
  else
  if (myOrigStatsArea != NULL)
  {
    // Make sure the ex_globals doesn't delete this stats area
    // in case of dynamic statements, since stmtStats is also
    // pointing to the same area
    if (stmtStats_ != NULL && myOrigStatsArea != NULL &&
          (Int32)myOrigStatsArea->getCollectStatsType() != SQLCLI_ALL_STATS) 
       getGlobals()->setStatsArea(NULL);
  }
}

RETCODE Statement::releaseTcbs(NABoolean closeAllOpens)
{

  if (root_tcb)
  {
  releaseStats();
    // The root tcb takes care of deleting itself and of deleting
    // the statement globals.
    if (statementGlobals_)
    {
      ExRtFragTable *ft = statementGlobals_->getRtFragTable();
      
      // finish any transactional messages outstanding
      // for this statement, since the statement won't
      // be found anymore on the context's statement list
      // Should do this before releaseEsps, so that they 
      // will not be sending work or continue messages to 
      // each other after the deallocation of fragment 
      // instances has begun (see Soln 10-060504-6268).
      releaseTransaction(TRUE);

      // release all esps next
      releaseEsps(closeAllOpens);

      // Deallocate TCB trees for all stored procedure result set
      // children. The UDR leaf nodes in the result set TCB tree are
      // not valid once the UDR leaf node in this CALL statement
      // goes away, so those result set TCBs must be deallocated first.
      ExRsInfo *rsInfo = getResultSetInfo();
      if (rsInfo)
      {
        StmtDebug0("  About to call ExRsInfo::reset()");
        rsInfo->reset();
        
        ULng32 numChildren = rsInfo->getNumEntries();
        StmtDebug1("  Num RS children: %d", (Lng32) numChildren);
        
        for (ULng32 i = 1; i <= numChildren; i++)
        {
          Statement *rsChild = NULL;
          const char *proxySyntax = NULL;
          NABoolean open = FALSE;
          NABoolean closed = FALSE;
          NABoolean prepared = FALSE;
          
          NABoolean found = rsInfo->getRsInfo(i, rsChild, proxySyntax,
                                              open, closed, prepared); 
          
          if (found && rsChild)
          {
            StmtDebug1("  About to call dealloc() on RS %p", rsChild);
            rsChild->dealloc();
          }
          
        } // for each RS child
      } // if (rsInfo)

      // This call to deallocAndDelete() will trigger destructor calls
      // for all TCBs
      Int32 retcode = root_tcb->deallocAndDelete(statementGlobals_,ft);

      if (retcode < 0)
      {
        context_->diags() << DgSqlCode(- EXE_INTERNAL_ERROR);
        return ERROR;
      }
      
      root_tcb = NULL;
      
      //
      // Deallocating the TCB tree may have caused more messages
      // to be sent. To be safe, we once again wait for completion
      // of transactional messages.
      //
      releaseTransaction(TRUE);
      
    } // if (statementGlobals_)
    
  } // if (root_tcb)
  
  root_tcb = NULL;
  return SUCCESS;
}

RETCODE Statement::dealloc(NABoolean closeAllOpens)
{
  StmtDebug2("[BEGIN dealloc] %p, stmt state %s", this, stmtState(getState()));

  RETCODE result = SUCCESS;

  // stop the transaction, if once was started
  if ((context_->aqrInfo()->xnStartedAtPrepare()) &&
      (context_->getTransaction()->xnInProgress()) &&
      (context_->getTransaction()->exeStartedXn()) &&
      (context_->getTransaction()->autoCommit()) &&
      (autocommitXn()))
    {
      context_->aqrInfo()->setXnStartedAtPrepare(FALSE);
      
      if (commitTransaction(context_->diags()))
	{
	  // ignore error and proceed to deallocate
	}
    }
  
  context_->removeFromStatementWithEspsList(this);
  
  result = releaseTcbs(closeAllOpens);
  
  if (result == SUCCESS)
  {
    setState(CLOSE_);
    setFixupState(0);
    setComputeBulkMoveInfo(TRUE);
    
    // deallocate all cloned statements
    clonedStatements->position();
    Statement * clone = (Statement*)clonedStatements->getNext();
    for (; clone != NULL; clone = (Statement*)clonedStatements->getNext())
    {
      StmtDebug1("  About to call dealloc on clone %p", clone);
      clone->dealloc();
    }
    
    context_->removeFromCloseStatementList(this, FALSE);

    if (extractConsumerQueryTemplate_)
    {
      heap_.deallocateMemory(extractConsumerQueryTemplate_);
      extractConsumerQueryTemplate_ = NULL;
    }
  } // if (result == SUCCESS)

  StmtDebug2("[END dealloc] %p, stmt state %s", this, stmtState(getState()));
  return result;
}

static Lng32 getMaxParamIdx(ex_root_tdb *rootTdb){
  Lng32 maxParamIdx = 0;
  if(rootTdb->inputExpr()){
    maxParamIdx = rootTdb->inputExpr()->getMaxParamIdx();
  }
  
  if(rootTdb->outputExpr()){
    Lng32 outputParamIdx = rootTdb->outputExpr()->getMaxParamIdx();
    if(outputParamIdx > maxParamIdx)
      maxParamIdx = outputParamIdx;
  }
  return maxParamIdx;
}

RETCODE Statement::describe(Descriptor * desc, Lng32 what_desc,
			    ComDiagsArea &diagsArea)
{
  // One special case is for stored procedure result set proxy
  // statements. CLI callers do not prepare them. We do internal
  // prepares whenever the CLI caller describes or executes them.
  if (parentCall_)
  {
    ExRsInfo *rsInfo = parentCall_->getResultSetInfo();
    if (rsInfo)
    {
      ULng32 rsIndex = rsInfo->getIndex(this);
      RETCODE proxyRetcode = rsProxyPrepare(*rsInfo, rsIndex, diagsArea);
      if (proxyRetcode == ERROR)
        return ERROR;
    }
  }

  if (!root_tdb)
    return SUCCESS;

  InputOutputExpr *expr;
  enum ex_expr::exp_return_type expRType;

  Int32 retcode = 0; // be optimistic

  // A special case for DESCRIBE is when the SQL statement is a CALL
  // statement and the CLI caller wants to use a "wide" descriptor for
  // the CALL.
  if (root_tdb->hasCallStmtExpressions() && desc->isDescTypeWide())
  {
    // First populate the caller's descriptor with information from
    // the INPUT expression, the overlay information from the OUTPUT
    // expression into the same descriptor.

    Lng32 eNumEntries = getMaxParamIdx(root_tdb);
    if (desc->getMaxEntryCount() < eNumEntries){
      diagsArea << DgSqlCode(CLI_TDB_DESCRIBE_ERROR);
      return ERROR;
    }
    else if (desc->getUsedEntryCount() < eNumEntries) {
      desc->dealloc();
      desc->alloc(eNumEntries);
    }

    // The code in expr->describe sets the entry count in case of a Call stmt
    // only if the new count is greater than the old count. This
    // causes the old count to be returned during describe, if the 
    // same descriptor was reused and the number of entries in the new
    // query is less than the old count. 
    // Setting the entry count to 0 before calling describe.
    desc->setUsedEntryCount(0);

    if (expr = root_tdb->inputExpr())
    {
      UInt32 flags = 0;
      expr->setIsODBC(flags, root_tdb->odbcQuery());
      expr->setInternalFormatIO(flags, 
				context_->getSessionDefaults()->
				getInternalFormatIO());
      expr->setIsRWRS(flags, (root_tdb->getRWRSInfo() != NULL));
      expr->setIsDBTR(flags,
		      context_->getSessionDefaults()->getDbtrProcess());

      expRType = expr->describeInput(desc, NULL, flags);
      if(expRType != ex_expr::EXPR_OK)
	retcode = -1;
    }
    
    if ((retcode == 0) && (expr = root_tdb->outputExpr()))
    {
      UInt32 flags = 0;
      expr->setIsODBC(flags, root_tdb->odbcQuery());
      expr->setInternalFormatIO(flags, 
				context_->getSessionDefaults()->
				getInternalFormatIO());
      expr->setIsDBTR(flags,
		      context_->getSessionDefaults()->getDbtrProcess());
      expRType = expr->describeOutput(desc, flags);
      if(expRType != ex_expr::EXPR_OK)
	retcode = -1;
    }
  }
  else{
    if (what_desc == SQLWHAT_INPUT_DESC)
      {
      /* describe input */
      // retcode = root_tdb->describe(desc, 0);
      expr = root_tdb->inputExpr();
      if (expr == NULL)
	{
	  // set the entry count to zero. Otherwise desc will contain the
	  // entry count of the last describe.
	  desc->setUsedEntryCount(0);
	  return SUCCESS;
	}

      if (desc->getMaxEntryCount() < expr->getNumEntries())
	{
	  desc->setMaxEntryCount(expr->getNumEntries());
	}

      if (desc->getUsedEntryCount() < expr->getNumEntries())
	{
	  desc->dealloc();
	  desc->alloc(expr->getNumEntries());
	}
      
      UInt32 flags = 0;
      expr->setIsODBC(flags, root_tdb->odbcQuery());
      expr->setInternalFormatIO(flags, 
				context_->getSessionDefaults()->
				getInternalFormatIO());
      expr->setIsRWRS(flags, (root_tdb->getRWRSInfo() != NULL));
      expr->setIsDBTR(flags,
		      context_->getSessionDefaults()->getDbtrProcess());
      if (expr->describeInput(desc, root_tdb->getRWRSInfo(), flags) 
	  != ex_expr::EXPR_OK)
	retcode = -1;
      }
    else
      {
	/* describe output */
	// retcode = root_tdb->describe(desc, -1);
	expr = root_tdb->outputExpr();
	if (expr == NULL)
	  {
	    // set the entry count to zero. Otherwise desc will contain the
	    // entry count of the last describe.
	    desc->setUsedEntryCount(0);
	    return SUCCESS;
	  }
	
	if (desc->getMaxEntryCount() < expr->getNumEntries())
	  {
	    desc->setMaxEntryCount(expr->getNumEntries());
	  }

	if (desc->getUsedEntryCount() < expr->getNumEntries())
	  {
	    desc->dealloc();
	    desc->alloc(expr->getNumEntries());
	  }
	
	UInt32 flags = 0;
	expr->setIsODBC(flags, root_tdb->odbcQuery());
	expr->setInternalFormatIO(flags, 
				  context_->getSessionDefaults()->
				  getInternalFormatIO());
	expr->setIsDBTR(flags,
			context_->getSessionDefaults()->getDbtrProcess());
	if (expr->describeOutput(desc, flags) != ex_expr::EXPR_OK)
	  retcode = -1;
      }
  }
  if (retcode)
  {
    diagsArea << DgSqlCode(- CLI_TDB_DESCRIBE_ERROR);
    return ERROR;
  }

  return SUCCESS;
}

void Statement::copyGenCode(char * gen_code, ULng32 gen_code_len,
			    NABoolean unpackTDBs)
{
  StmtDebug3("[BEGIN copyGenCode] this %p, gen_code %p, len %u",
             this, gen_code, gen_code_len);

  ex_assert(!isCloned(),
            "copyGenCode() should not be called on a cloned statement");

  // Release the TDB tree if one exists
  assignRootTdb(NULL);

  // do some basic sanity checking
  if ((gen_code == 0) || (gen_code_len <= 0))
  {
    //    gen_code = 0;
    //    gen_code_len = 0;
    return;
  }
  
  CollHeap * h = cliGlobals_->getArkcmp()->getHeap();
  char *newTdbTree = (char *) (new (h) char[gen_code_len]);
  str_cpy_all(newTdbTree, gen_code, (Lng32) gen_code_len);
  
  assignRootTdb((ex_root_tdb *) newTdbTree);
  root_tdb_size = (Lng32) gen_code_len;
  
  if (unpackTDBs)
  {
  // Set up the reallocator for use when object version migration occurs.
  //
  ComTdb dummyTdb;
  ex_root_tdb *newRoot = (ex_root_tdb *)
    root_tdb->driveUnpack((void *)root_tdb,&dummyTdb, unpackSpace_);

  assignRootTdb(newRoot);
  
  if (root_tdb == NULL)
  {
    // ERROR during unpacking. Most likely case is verison-unsupported.
    // How do I report an error here ???
    //
    // diagsArea << DgSqlCode(- CLI_STMT_NOT_PREPARED);
    // return ERROR;
    //
    // Alternative: add code to drive a recompilation. Invalidate the plan.
    //
  }

  if ((stmt_type == STATIC_STMT) && (root_tdb))
    setRecompWarn(root_tdb->recompWarn());
    
  } // if (unpackTDBs)
  
  StmtDebug1("[END copyGenCode] this %p", this);
}

void Statement::copyInSourceStr(char * in_source_str_, Lng32 src_len_in_octets,
			 Lng32 charset)
{
  charset_ = charset;
  NAHeap * heap = context_->exHeap();
  if (source_str)
    NADELETEBASIC(source_str, heap);

  // do some basic sanity checking
  if ((in_source_str_ == 0) || (src_len_in_octets <= 0))
  {
     source_str = 0;
     source_length = 0;
     return;
  }

  Int32 maxBytesPerChar = CharInfo::maxBytesPerChar((CharInfo::CharSet)charset);
  source_str = new (heap) char[src_len_in_octets+maxBytesPerChar];
  str_cpy_all(source_str, in_source_str_, src_len_in_octets);
  if (charset == SQLCHARSETCODE_UCS2)
    ((NAWchar*)source_str)[src_len_in_octets/maxBytesPerChar] = 0;
  else
    source_str[src_len_in_octets] = '\0';
  source_length = src_len_in_octets;
}

void Statement::copyOutSourceStr(char * out_source_str, 
				 Lng32 &out_src_len_in_octets) // INOUT
{
  // do some basic sanity checking
  if ((out_source_str == NULL) || (out_src_len_in_octets <= 0) ||
      (out_src_len_in_octets < source_length))
    {
      out_src_len_in_octets = 0;
      return;
    }

  Int32 maxBytesPerChar = CharInfo::maxBytesPerChar((CharInfo::CharSet)charset_);
  str_cpy_all(out_source_str, source_str, source_length);
  if (charset_ == SQLCHARSETCODE_UCS2)
    ((NAWchar*)out_source_str)[source_length/maxBytesPerChar] = 0;
  else
    out_source_str[source_length] = '\0';

  out_src_len_in_octets = source_length;
}

void Statement::copySchemaName(char * schemaName, Lng32 schemaNameLength)
{
  if (schemaName_)
    NADELETEBASIC(schemaName_, (&heap_));

  // do some basic sanity checking
  if ((schemaName == 0) || (schemaNameLength <= 0))
  {
     schemaName_ = 0;
     schemaNameLength_ = 0;
     return;
  }

  schemaName_ = new (&heap_) char[schemaNameLength + 1];
  str_cpy_all(schemaName_, schemaName, schemaNameLength);
  schemaName_[schemaNameLength] = '\0';
  schemaNameLength_ = schemaNameLength;
}

void Statement::copyRecompControlInfo(char * basePtr,
				      char * recompControlInfo, 
				      Lng32 recompControlInfoLength)
{
}


void Statement::setCursorName(const char * cn)
{
  // as a side effect of this method the statement is added
  // to the cursorList of the current context. If the cursorName
  // changes, the statement is removed from the cursorList and
  // then re-inserted with the new name. This is neccessary,
  // because the name is used for hashing into the cursorList.

  StmtDebug2("  Setting cursor name for statement %p to \"%s\"",
             this, (cn ? cn : "(NULL)"));

 ComDiagsArea &diags = context_->diags();

  Lng32 nameLength = 0;
  char *copy_of_cn;
 

  if (cursor_name_) {

// Need to use length in comparison when 
// the length argument is added for "cn".

    // if we are changing it to the same name, save some work...
    if (cn && !strcmp(cn, cursor_name_->identifier))
       return;
    context_->removeCursor(cursor_name_, statement_id->module);
    heap_.deallocateMemory(cursor_name_);
  }

  if (cn) {
    nameLength = str_len(cn);
    copy_of_cn = new (&heap_) char [nameLength+1];
    str_cpy_all(copy_of_cn,cn,nameLength);
    copy_of_cn[nameLength] = '\0';

    cursor_name_ =  (SQLSTMT_ID *)(heap_.allocateMemory(sizeof(SQLSTMT_ID)));
    init_SQLCLI_OBJ_ID(cursor_name_, SQLCLI_CURRENT_VERSION,
             cursor_name, 0, copy_of_cn, 0,
           SQLCHARSETSTRING_ISO88591, str_len(cn));

    StmtDebug1("  Adding statement %p to the cursor list", this);
    context_->addToCursorList(*this);
  }
  else
     cursor_name_ = 0;
}

short Statement::transactionReqd()
{
  return ((short) (root_tdb && root_tdb->transactionReqd()));
}

Int64 Statement::getRowsAffected()
{
  return statementGlobals_->getRowsAffected();
}

//////////////////////
// PRIVATE methods
//////////////////////


// starts a transaction, if one has not already been started
// and if needed.
short Statement::beginTransaction(ComDiagsArea &diagsArea)
{

  // Get the current transmode and update the values
  // for transaction typing.  Code is here so values
  // will be updated for BEGIN WORK statements.
  updateTModeValues();

  // implicit xns for ddl stmts will be started and committed/aborted
  // in arkcmp if auto commit is on. Otherwise, it will be started here.
  if ((root_tdb->transactionReqd()) &&
      ((NOT root_tdb->ddlQuery()) ||
       (NOT context_->getTransaction()->autoCommit())))
    {
      // the trans mode at compile time of this query must be the
      // same as the transaction mode at execution time.
      if (!systemModuleStmt() && 
            compareTransModes(
	    root_tdb,
	    context_->getTransaction(),
	    &diagsArea, systemModuleStmt()))
	return ERROR;
      if (context_->getTransaction()->userEndedExeXn())
      {
        if (!diagsArea.contains(-CLI_USER_ENDED_EXE_XN))
            diagsArea << DgSqlCode(- CLI_USER_ENDED_EXE_XN);
        return ERROR;
      }
      if (! context_->getTransaction()->xnInProgress())
        {
	  // if no auto begin, then do not start the transaction.
	  if (context_->getTransaction()->getTransMode()->getAutoBeginOff())
	    {
	      // return error. A transaction is needed, one is not
	      // running and autobegin has been disabled.
	      diagsArea << DgSqlCode(- CLI_AUTO_BEGIN_TRANSACTION_ERROR);
              return ERROR;
	    }

          StmtDebug1("  About to BEGIN TX for statement %p...", this);
	  
          // start a transaction since one is not already running.
          short taRetcode =
            context_->getTransaction()->beginTransaction();

          StmtDebug1("  Return code is %d", (Lng32) taRetcode);

          if (taRetcode != 0)
            {
	      diagsArea.mergeAfter(*context_->getTransaction()->getDiagsArea());
	      //          diagsArea << DgSqlCode(- CLI_BEGIN_TRANSACTION_ERROR);
              return ERROR;
            }

          context_->getTransaction()->implicitXn() = TRUE;

	  // this statement started a transaction in autocommit mode.
	  // It means that this xn will only be committed when this
	  // statement ends execution.
	  if (context_->getTransaction()->autoCommit())
          {
	    setAutocommitXn(TRUE);
            StmtDebug0("  AUTOCOMMIT transaction successfully started");
          }
	  else
          {
	    setAutocommitXn(FALSE);
            StmtDebug0("  Transaction successfully started");
          }

        } // if (! context_->getTransaction()->xnInProgress())

      // move the transid from executor globals to statement globals,
      // if a transaction is running.
      statementGlobals_->getTransid() =
        context_->getTransaction()->getExeXnId();
      
      StmtDebug2("  Statement %p now has trans ID %s", this,
                 TransIdToText(statementGlobals_->getTransid()));
	        
      // We might have suspended work on this or a previous transaction
      // during the last CLOSE operation.  Indicate to the root TCB that
      // we are willing to work on a transaction again and that it is
      // ok to send out transactional requests (to DP2 and to ESPs)
      if (statementGlobals_ && statementGlobals_->getRtFragTable())
        {
          statementGlobals_->getRtFragTable()->continueWithTransaction();
        }

      if (root_tdb->getUpdSavepointOnError())
	{
	  // get a savepoint id which will be sent to DP2.
	  context_->getTransaction()->generateSavepointId();

	  statementGlobals_->getSavepointId() =
	    context_->getTransaction()->getSavepointId();
	}
    } // if (root_tdb->transactionReqd())

    else
    {
      // Browse access cursors or the load phase of index will
      // have the transaction passed (to ESPs?) if one exists
      // in its statement globals
      if (context_->getTransaction()->xnInProgress())
        {
          statementGlobals_->getTransid() =
            context_->getTransaction()->getExeXnId();
        }
    }
    if (stmtStats_ && stmtStats_->getMasterStats() != NULL)
        stmtStats_->getMasterStats()->setTransId(statementGlobals_->getTransid());
  return 0;
}
  
// ends(commits) transaction, if one is running and auto commit is on.
short Statement::commitTransaction(ComDiagsArea &diagsArea)
{
  if (context_->getTransaction()->userEndedExeXn())
    {
      if (!diagsArea.contains(-CLI_USER_ENDED_EXE_XN))
         diagsArea << DgSqlCode(- CLI_USER_ENDED_EXE_XN);
      return ERROR;
    }

  if ((context_->getTransaction()->xnInProgress()) &&
      (context_->getTransaction()->exeStartedXn()) &&
      (context_->getTransaction()->autoCommit()) &&
      (autocommitXn()))
    {
      if (context_->aqrInfo())
	context_->aqrInfo()->setXnStartedAtPrepare(FALSE);

      // get current context and close all statements
      // started under the current transaction
      context_->closeAllCursors(ContextCli::CLOSE_ALL, ContextCli::CLOSE_CURR_XN);
      // Capture any errors that happened and return eg. transaction 
      // related errors that happen during 
      // Statement::close-> ExTransaction::commitTransaction that get called 
      // in ::closeAllCursors
      if (diagsArea.mainSQLCODE() <0)
        {
          return ERROR;
        }
      
      // if transaction is still active(it may have been committed at
      // close cursor time if auto commit is on), commit it.
      if (context_->getTransaction()->xnInProgress())
	{
	  StmtDebug2("  About to COMMIT, stmt %p, tx %s...", this,
		     TransIdToText(statementGlobals_->getTransid()));
	  // do waited commit for DDL queries
	  short taRetcode = context_->commitTransaction();
	  
          StmtDebug1("  Return code is %d", (Lng32) taRetcode);
      
	  setAutocommitXn(FALSE);

	  if (taRetcode != 0)
	    {
              // If there are diagnostics in the statement globals then add
              // them to the caller's diags area first. They may contain
              // information about something that went wrong before the
              // COMMIT was attempted. Then add information about the COMMIT
              // failure that just occurred.
              statementGlobals_->takeGlobalDiagsArea(diagsArea);
              diagsArea.mergeAfter(*context_->getTransaction()->getDiagsArea());
              return ERROR;
            }
          StmtDebug0("  COMMIT was successful");
        }
    }
  else
    {
      StmtDebug0("  No AUTOCOMMIT transaction for this stmt");
    }
  
  return 0;
}

short Statement::rollbackSavepoint(ComDiagsArea & diagsArea,
				   NABoolean &rollbackXn)
{
  rollbackXn = FALSE;
  
  NABoolean rollbackSP = FALSE;

  if (context_->getTransaction()->xnInProgress())
    {
      if ((context_->getTransaction()->exeStartedXn()) &&
	  (context_->getTransaction()->autoCommit()))
	{
	  rollbackSP = FALSE;
	}
      else if (diagsArea.getRollbackTransaction())
	rollbackSP = FALSE;
      else if (root_tdb && 
	       (root_tdb->transactionReqd() != 0))
	{
	  if (root_tdb->getUpdAbortOnError())
	    rollbackSP = FALSE;
	  else if (root_tdb->getUpdPartialOnError())
	    rollbackSP = FALSE;
	  else if (root_tdb->getUpdSavepointOnError())
	    rollbackSP = TRUE;
	  else if ((NOT root_tdb->getUpdErrorOnError()) &&
		   (NOT diagsArea.getNoRollbackTransaction()))
	    rollbackSP = FALSE;
	}
    }

  if (rollbackSP)
    {
      short retcode = root_tcb->rollbackSavepoint();
      if (retcode)
	{
	  diagsArea.mergeAfter(*statementGlobals_->getDiagsArea());
	  diagsArea << DgSqlCode(-CLI_SAVEPOINT_ROLLBACK_FAILED);
	  rollbackXn = TRUE;
	}
      else
	{
	  // do not abort this transaction.
	  rollbackXn = FALSE;
	  diagsArea << DgSqlCode(CLI_SAVEPOINT_ROLLBACK_DONE);
	  // clr count of affected rows, CLI_SAVEPOINT_ROLLBACK_DONE.
	  statementGlobals_->setRowsAffected(0);
	  diagsArea.setRowCount(0);
	}
    }

  return 0;
}

// abort (rollback) transaction, if one is running
short Statement::rollbackTransaction(ComDiagsArea & diagsArea,
				     NABoolean doXnRollback)
{
  ContextCli::CloseCursorType closeCursorType;

  if (context_->getTransaction()->userEndedExeXn())
    {
      if (!diagsArea.contains(-CLI_USER_ENDED_EXE_XN))
         diagsArea << DgSqlCode(- CLI_USER_ENDED_EXE_XN);
      return ERROR;
    }

  if (context_->getTransaction()->xnInProgress())
    {
      if (context_->aqrInfo())
	context_->aqrInfo()->setXnStartedAtPrepare(FALSE);

      NABoolean rollbackXn = FALSE;
      NABoolean autoCommitRollback = FALSE;
      if ((context_->getTransaction()->exeStartedXn()) &&
	  (context_->getTransaction()->autoCommit()))
	{
	  rollbackXn = TRUE;
	  autoCommitRollback = TRUE;
	}
      else if (diagsArea.getRollbackTransaction())
	rollbackXn = TRUE;
      else if (root_tdb && 
	       (root_tdb->transactionReqd() != 0))
	{
	  if (root_tdb->getUpdAbortOnError())
	    rollbackXn = TRUE;
	  else if (root_tdb->getUpdPartialOnError())
	    rollbackXn = FALSE;
	  else if ((NOT root_tdb->getUpdSavepointOnError()) &&
		   ((NOT root_tdb->getUpdErrorOnError()) &&
		    (NOT diagsArea.getNoRollbackTransaction())))
	    rollbackXn = TRUE;
	  else if (doXnRollback)
	    rollbackXn = TRUE;

	  // If aqr has been enabled and this query returned a 
	  // special error due to a concurrent ddl operation,
	  // then do not abort the transaction. This query will be retried
	  // and if this error persists, then the Xn will be aborted.
	  if (root_tdb->aqrEnabled() &&
	      ((diagsArea.mainSQLCODE() == -EXE_TIMESTAMP_MISMATCH)  ||
               (diagsArea.mainSQLCODE() == -CLI_INVALID_QUERY_PRIVS) ||
               (diagsArea.mainSQLCODE() == -CLI_DDL_REDEFINED)       ||
               (diagsArea.mainSQLCODE() == -EXE_SCHEMA_SECURITY_CHANGED)))
	    {
	      rollbackXn = FALSE;
	    }

          // If aqr has been enabled and this query returned a lost open
          // error due to a concurrent ddl operation, allow transaction
          // rollback if IUD.  IUD operations may have updated a subset of 
          // partitions before the lost open error was raised.  For example,
          // an earlier execution updated partitions 2, 3, and 4.  A ddl
          // operation now blows away these opens.  The re-exec updates 
          // partition #1, and the lost open error is raised for 2, 3, 
          // and 4.  The result can be data corruption unless we roll 
          // back the transaction.  See solution 10-100604-0887.
          //
          // But if not IUD, This query will be retried and if this error
          // persists, then the Xn will be aborted.
	  if (root_tdb->aqrEnabled() &&
	      (diagsArea.mainSQLCODE() == -EXE_LOST_OPEN) &&
	      (NOT root_tdb->updDelInsertQuery()))
	    {
	      rollbackXn = FALSE;
	    }
	}
      if (root_tdb && root_tdb->getPsholdCloseOnRollback())
        closeCursorType = ContextCli::CLOSE_ALL_INCLUDING_ANSI_PUBSUB_HOLDABLE;
      else
        closeCursorType = ContextCli::CLOSE_ALL_INCLUDING_ANSI_HOLDABLE;
      if (rollbackXn)
	{
	  short taRetcode;

          // clr count of affected rows.
          // ALM CR 6903 -- although some rows may have been affected
          // if an IUD used NO ROLLBACK, we don' currently have an accurate
          // way to count these, due to the way EXE cancels sessions
          // during error handling and the way the TSE does not dispatch
          // exe-in-dp2 sessions when raising errors such as lock timeouts.
          // So the fix for 6903 is to consistently return 0 rows affected.
          // In the future, we might enhance this.
	  statementGlobals_->setRowsAffected(0);
	  diagsArea.setRowCount(0);

	  NABoolean implicitTran = 
	    context_->getTransaction()->implicitXn();
          // get current context and close all statements
          // started under the current transaction
          context_->closeAllCursors(closeCursorType, ContextCli::CLOSE_CURR_XN,0,TRUE);

	  if (! context_->getTransaction()->xnInProgress())
	    return 0;

	  if (NOT autoCommitRollback)
	    {
	      //////////////////////////////////////////////////////////////
	      // An error occurred while doing an upd/ins/del stmt.
	      // We really need to roll back stmt, but until stmt atomicity
	      // support is not in, we must abort Xn.
	      // RollbackStatement ALWAYS rollbacks Xn. RollbackTransaction
	      // only rollbacks if executor started the Xn.
	      //////////////////////////////////////////////////////////////
	      if (context_->getTransaction()->exeStartedXn())
            {
              StmtDebug1("  About to rollback statement %p...", this);
		taRetcode = context_->getTransaction()
		  ->rollbackStatement();
            }
	      else
		{
		  // doom the transaction
              StmtDebug1("  About to DOOM TX for statement %p...", this);
		  taRetcode = context_->getTransaction()
		    ->doomTransaction();
		}
	    }
	  else
	    {
            StmtDebug1("  About to ROLLBACK TX for statement %p...", this);
	      taRetcode = context_->getTransaction()
		->rollbackTransactionWaited();
	    }

          StmtDebug1("  Return code is %d", (Lng32) taRetcode);

	  // release all outstanding I/Os with transactions, just like
	  // we do when we commit the transaction
	  context_->releaseAllTransactionalRequests();

          // fix 10-040526-4462. We need to force the transid in the context_
          // and the statement global to be in sync. Otherwise, the obsolete transid
          // in the statement global will be passed to the ESP during releaseESPs() 
          // calls. For any remote ESP, the bad transid will be rejected by the TMF
          // (inside the body of FS2_transid_to_buffer()) and the releasing operation 
          // on that ESP will fail, including the termination of the remote ESP. 
          // In some cases, the master will hung.
          //
          // After setting the transid in the statement global to -1 here, we just
          // bypass the call to FS2_transid_to_buffer(). Since remote ESPs caches
          // the transid, the cleanup operation will still use the "good" transid.
          // Note here the transid is obsolete from TMF point of view, but still  
          // valid (good) in terms of SQL/MX data structures (e.g., transid as the key
          // for table lookup).
          //
          // Note the transid in statement global should be to -1 after calling 
          // context::releaseAllTransactionalREquests() because that method calls 
          // statement::releaseTransaction() which uses the transid.
          statementGlobals_->getTransid() = (Int64)-1;

	  if (taRetcode != 0)
	    {
	      diagsArea.mergeAfter(*context_->
				   getTransaction()->getDiagsArea());
	      return ERROR;
	    }
	  else
	    {
              // Fix for solution 10-090908-4444 requires an extra
              // diags condition, error 8839.  To avoid changing lots
              // of expected files, don't do this in the regression
              // test environment.
	      if (! implicitTran)
		{
                  diagsArea << DgSqlCode(- CLI_VALIDATE_TRANSACTION_ERROR);
                  return ERROR;
                }
              else 
                {
                  if (context_->aqrInfo())
                    context_->aqrInfo()->setAbortedTransWasImplicit();
                }
            }
        }
      else if ((root_tdb) &&
	       (root_tdb->getUpdPartialOnError()))
	{
	  diagsArea << DgSqlCode(CLI_PARTIAL_UPDATED_DATA);

	  // Add the affected row count that was directly placed into
	  // the master globals to those that are already in the diags
	  // area. Make both row counts (diags and statementGlobals_)the same.
	  diagsArea.addRowCount(statementGlobals_->getRowsAffected());
	  statementGlobals_->setRowsAffected(diagsArea.getRowCount());
	}
    }

  return 0;
}

static
const NAWchar* get_name_mode(Lng32 name_mode)
{
  switch (name_mode)
  {
  case stmt_handle:
    return L"stmt_handle";
    break;

  case stmt_name:
    return L"stmt_name";
    break;

  case cursor_name:
    return L"cursor_name";
    break;

  case stmt_via_desc:
    return L"stmt_via_desc";

  case curs_via_desc:
    return L"curs_via_desc";
    break;

  default:
    return L"unknown";
    break;
  }
}

void Statement::dump(ostream * outstream)
{
  *outstream << "Statement:      " << this << '\n';

  *outstream << "Module:         ";
  if ((statement_id->module) && (statement_id->module->module_name))
//    *outstream << getModNameInLocale(statement_id->module);
    *outstream << (char *)(statement_id->module->module_name);  
  *outstream << '\n';

  *outstream << "Statement Name: "; 
  if (statement_id->identifier)
//    *outstream << getIdInLocale((SQLCLI_OBJ_ID*)statement_id);
    *outstream << (char *)(statement_id->identifier);
  *outstream << '\n';
  
  *outstream << "Cursor Name:    ";
  if (cursor_name_)
//    *outstream << getIdInLocale((SQLCLI_OBJ_ID*)cursor_name_);
    *outstream << (char *)(cursor_name_);
  *outstream << '\n';

  *outstream << "Name Mode:      " << get_name_mode(statement_id->name_mode) << '\n';

  *outstream << "Type:           " << (allocated() ? "DYNAMIC" : "STATIC") << '\n';

  *outstream << "Heap (total):   " << heap_.getTotalSize() << '\n';
  *outstream << "Heap (alloc):   " << heap_.getAllocSize() << '\n';
  *outstream << "Space (total):  " << space_.getAllocatedSpaceSize() << '\n';

  if (source_str)
    *outstream << source_str << '\n';

  *outstream << "----------------------------------------------------\n";
}

void Statement::setState(State newState) {
  // change the state of the statement. As a side effect the statement
  // might be removed or added from/to the openStatementList()
  switch (stmt_state) {
  case OPEN_:
  case EOF_:
  case FETCH_: //gps Add FETCH_ to "transition from" cases. (4/21/99)
  case RELEASE_TRANS_:
    // remove from list if newState is CLOSE_ or DEALLOCATED_
    if ((newState == CLOSE_) || (newState == DEALLOCATED_))
      {
        State originalState = stmt_state;

	// In certain cases, this statement is not added to the open
	// statement list. This is a performance optimization for
	// unique OLT queries. These queries are non-cursor queries
	// that are completed within one CLI call (ClearExecFetchClose).
	// See PerformTasks method in Cli.cpp for details.
	// Do not add them to open stmt list, 
	stmt_state = newState;
	if (NOT oltOpt())
	  context_->removeFromOpenStatementList(statement_id);

        // If this is a stored procedure result set and it is
        // transitioning out of an open state, then we have some
        // bookkeeping to do
        if (parentCall_ && originalState != EOF_)
        {
          ExRsInfo *parentRsInfo = parentCall_->getResultSetInfo();
          if (parentRsInfo)
          {
            StmtDebug2("  setState() changing RS state to %s, this %p",
                       stmtState(getState()), this);
            ULng32 rsIndex = parentRsInfo->getIndex(this);
            parentRsInfo->setCloseAttempted(rsIndex);
            StmtDebug2("  RS index %u, Num closed since CALL %u",
                       rsIndex, parentRsInfo->getNumClosedSinceLastCall());
          }
        }
        
      }
    break;

  case PREPARE_:
    // remove from list if newState is CLOSE_ or DEALLOCATED_ or INITIAL_
    if ((newState == CLOSE_) || (newState == DEALLOCATED_) || (newState == INITIAL_))
      {
	context_->removeFromOpenStatementList(statement_id);
      }
    break;

  case INITIAL_:
  case CLOSE_:
    // add to openStatementList if new state is OPEN_ or PREPARE_
    if ((newState == OPEN_) || (newState == PREPARE_))
      {
	if (NOT oltOpt())
	  context_->addToOpenStatementList(statement_id, this);
	else
	  context_->removeFromCloseStatementList(this, FALSE);
      }
    break;
  }

  if (stmt_state != newState)
  {
    StmtDebug3("*** Statement %p state change: %s -> %s",
               this, stmtState(stmt_state), stmtState(newState));
  }

  stmt_state = newState;

  if (stmtStats_ != NULL) 
  {
    if (stmtStats_->getMasterStats() != NULL)
     stmtStats_->getMasterStats()->setStmtState(newState);
    stmtStats_->setMergeReqd(TRUE);
  }
}

NABoolean Statement::noRowsAffected(ComDiagsArea & diagsArea)
{
  // ANSI says that an EOF is to be returned if no rows were affected
  // by an update/delete/insert operation.
  if ((root_tdb) && 
      (root_tdb->updDelInsertQuery()) &&
      (diagsArea.getRowCount() == 0) &&
      getRowsAffected() == 0)
    return TRUE;
  else
    return FALSE;
}


NABoolean Statement::isSelectInto()
{
  return (getRootTdb() && getRootTdb()->selectIntoQuery());
}

NABoolean Statement::isDeleteCurrentOf()
{
  return (getRootTdb() && getRootTdb()->deleteCurrentOfQuery());
}

NABoolean Statement::isUpdateCurrentOf()
{
  return (getRootTdb() && getRootTdb()->updateCurrentOfQuery());
}
// BertBert VV
NABoolean Statement::isEmbeddedUpdateOrDelete()
{
  return (getRootTdb() && getRootTdb()->isEmbeddedUpdateOrDelete());
}
NABoolean Statement::isStreamScan()
{
  return (getRootTdb() && getRootTdb()->isStreamScan());
}
// BertBert ^^

RETCODE Statement::setHoldable(ComDiagsArea &diagsArea, NABoolean h)
{
  if (root_tdb == NULL)
    return setAnsiHoldable(diagsArea, h);
  else
    return setPubsubHoldable(diagsArea, h);
}

RETCODE Statement::setPubsubHoldable(ComDiagsArea &diagsArea, NABoolean h)
{
  // tbd -- error for open stmt??
  if (h &&
      root_tdb &&
      !root_tdb->isEmbeddedUpdateOrDelete() &&
      !root_tdb->isStreamScan())
    {
      // Holdable cursors are only supported for streaming cursors
      // and destructive cursors because there are some strange
      // side-effects (locks disappear after commit) that we don't
      // want to inflict on non-Publich/Subscribe functionality at
      // this time. There are no source-code changes needed to allow
      // holdable cursors in these other cases though.
      diagsArea << DgSqlCode(-CLI_CURSOR_CANNOT_BE_HOLDABLE);
      return ERROR;
    }
  if (stmt_state == OPEN_ || stmt_state == FETCH_ || stmt_state == RELEASE_TRANS_)
    {
      // Cannot change holdable attribute while cursor is open.
      diagsArea << DgSqlCode(-CLI_CURSOR_ATTR_CANNOT_BE_SET);
      return ERROR;
    }
  
  if (h)
    holdable_ = SQLCLIDEV_PUBSUB_HOLDABLE;
  else
    holdable_ = SQLCLIDEV_NONHOLDABLE;
  return SUCCESS;
}

RETCODE Statement::setAnsiHoldable(ComDiagsArea &diagsArea, NABoolean h)
{
  // SQL_Exec_SetStmtAttr for setting the holdable cursor can't be called 
  // after the statement is prepared
  if (root_tdb || stmt_state == OPEN_ || stmt_state == FETCH_ || stmt_state == RELEASE_TRANS_)
    {
      // Cannot change holdable attribute while cursor is open.
      diagsArea << DgSqlCode(-CLI_CURSOR_ATTR_CANNOT_BE_SET);
      return ERROR;
    }
  
  if (h)
    holdable_ = SQLCLIDEV_ANSI_HOLDABLE;
  else
    holdable_ = SQLCLIDEV_NONHOLDABLE;
  return SUCCESS;
}

RETCODE Statement::setInputArrayMaxsize(ComDiagsArea &diagsArea, const Lng32 inpArrSize)
{
  if (inpArrSize < 0) {
    diagsArea << DgSqlCode(-CLI_ARRAY_MAXSIZE_INVALID_ENTRY);
    return ERROR;
  }

  inputArrayMaxsize_ = inpArrSize ;
  return SUCCESS ;
}

RETCODE Statement::setRowsetAtomicity(ComDiagsArea &diagsArea, const AtomicityType atomicity)
{
  if ((atomicity != UNSPECIFIED_) && (atomicity != ATOMIC_) && (atomicity != NOT_ATOMIC_)) {
    diagsArea << DgSqlCode(-CLI_INVALID_ATTR_VALUE);
    return ERROR;
  }

  rowsetAtomicity_ = atomicity ;
  return SUCCESS ;
}
  
RETCODE Statement::setNotAtomicFailureLimit(ComDiagsArea &diagsArea, const Lng32 limit)
{
  if (limit < 30) {
    diagsArea << DgSqlCode(-CLI_INVALID_ATTR_VALUE);
    return ERROR;
  }

  notAtomicFailureLimit_ = limit ;
  return SUCCESS ;
}
  
  

void Statement::setOltOpt(NABoolean v)
{
  if (! getRootTdb())
    return;

  if (v && getRootTdb()->doOltQueryOpt())
    flags_ |= OLT_OPT;
  else
    flags_ &= ~OLT_OPT; 
}

// Must be in CLOSE_ state.
// Will cause fixup at open.
Lng32 Statement::releaseSpace()
{
  StmtDebug2("[BEGIN releaseSpace] %p, stmt state %s",
             this, stmtState(getState()));

  if (stmt_state == INITIAL_)
  {
    StmtDebug2("[END releaseSpace] %p, result is %s",
               this, RetcodeToString(SUCCESS));
    return SUCCESS;
  }

  // Bugzilla 1662
  // A statement being reclaimed might be in the EOF_ state. We will
  // not attempt to reclaim these statements. Here's one way an EOF_
  // statement can be encountered:
  // * The statement has an autocommit transaction
  // * End-of-data is fetched
  // * Inside the CLI fetch, we commit the transaction, put the
  //   statement in the CLOSED_ state, and place the statement on
  //   the closed statement list
  // * Then we change the statement state to EOF_ so that a subsequent
  //   SQL_EXEC_CloseStmt will succeed
  // * The statement is now on the closed statement list, is in the
  //   EOF_ state, but is not actually a valid candidate for reclaim
  //   until the CLI caller issues SQL_EXEC_CloseStmt
  if (stmt_state == EOF_)
  {
    StmtDebug2("[END releaseSpace] %p, result is %s",
               this, RetcodeToString(SUCCESS));
    return SUCCESS;
  }

  assert(stmt_state == CLOSE_);
  RETCODE result = releaseTcbs(FALSE);
  
  if (result == SUCCESS)
  {
    setState(CLOSE_); // The state may have changed by a call within releaseTcbs()
    space_.freeBlocks();
    setFixupState(0);  // will fixup at exec()
    // To recalculate the bulk move info when the tcb is re-generated
    setComputeBulkMoveInfo(TRUE);
    if (stmtStats_ != NULL && stmtStats_->getMasterStats() != NULL)
      stmtStats_->getMasterStats()->incReclaimSpaceCount();
  }

  StmtDebug2("[END releaseSpace] %p, result is %s",
             this, RetcodeToString(result));
  return (Lng32) result;
}

NABoolean Statement::isReclaimable()
{
  if (root_tdb != NULL && 
      (root_tdb->getQueryType() == ComTdbRoot::SQL_INSERT_RWRS ||
       root_tdb->cantReclaimQuery()))
    return FALSE;
  else
    return TRUE;
}

NABoolean Statement::returnRecompWarn()
{
  if (stmt_type == STATIC_STMT)
    return recompWarn();
  else if (root_tdb)
    return root_tdb->recompWarn();
  else
    return FALSE;
}

// Wait for completion of UDR requests associated with this
// statement. If allRequests is FALSE then only wait for transactional
// requests.
RETCODE Statement::completeUdrRequests(NABoolean allRequests) const
{

  RETCODE result = SUCCESS;
  while (statementGlobals_ &&
         ((allRequests && statementGlobals_->numUdrMsgsOut() > 0) ||
          (!allRequests && statementGlobals_->numUdrTxMsgsOut() > 0)))
  {
    // First attempt to complete requests on scalar udr server
    IpcConnection *conn = statementGlobals_->getUdrConnection();
    ExUdrServer *udrServ = statementGlobals_->getUdrServer();
    if(conn != NULL && udrServ != NULL)
    {
      StmtDebug1(
      "  About to call ExUdrServer::completeUdrRequests(%p, FALSE)...",
        conn);

      udrServ->completeUdrRequests(conn, FALSE);
    }

    // Next, attempt to complete requests on dedicated udr servers
    // if there are any.
    LIST(ExUdrServer *) udrServList = statementGlobals_->getUdrServersD();
    for (CollIndex i = 0; i < udrServList.entries(); i++)
    {
      ExUdrServer *udrServ = udrServList[i];
      IpcConnection *conn = udrServ->getUdrControlConnection();
      
      StmtDebug1(
        "  About to call ExUdrServer::completeUdrRequests(0x%08x, FALSE)...",
        conn);
      udrServ->completeUdrRequests(conn, FALSE);
    }
    StmtDebug0("  Done");
  }
  
  return result;
}

void Statement::setUniqueStmtId(char * id)
{
  if (uniqueStmtId_)
    {
      // get start & finished messages to cancel broker cleaned up
      // before switching qid.
      if (root_tcb && root_tcb->anyCbMessages())
        releaseTransaction(TRUE);

      NADELETEBASIC(uniqueStmtId_, &heap_);
      uniqueStmtId_ = NULL;
    }

  if (id)
    {
      uniqueStmtId_ = new(&heap_) char[strlen(id)+1];
      strcpy(uniqueStmtId_, id);
      uniqueStmtIdLen_ = strlen(uniqueStmtId_);
    }
  else
    {
      // generate a unique id and set it.
      uniqueStmtId_ = new(&heap_) char[ComSqlId::MAX_QUERY_ID_LEN+1];
      
      char tmpLong[ 20 ];
      if (statement_id->name_mode == stmt_handle)
	str_ltoa((ULong)getStmtHandle(), tmpLong);
      ComSqlId::createSqlQueryId(uniqueStmtId_,
				 ComSqlId::MAX_QUERY_ID_LEN+1,
				 uniqueStmtIdLen_,
				 strlen(context_->getSessionId()),
				 context_->getSessionId(),
				 cliGlobals_->getNextUniqueNumber(),
				 ((statement_id->name_mode == stmt_handle)
				  ? strlen(tmpLong)
				  : strlen(getIdentifier())),
				 (char*)
				 ((statement_id->name_mode == stmt_handle)
				  ? tmpLong
				  : getIdentifier())
				 );
    }
}

// This method should always be used to assign a new value to the
// root_tdb data member. The method will keep the root_tdb pointer for
// a given statement in sync with its clones.
//
// NOTE: we really should have this method keep the root_tdb_size data
// member in sync with root_tdb. But currently root_tdb_size is only
// used by SHOWPLAN code paths and on other paths is not maintained. A
// possible future improvement is to keep root_tdb and root_tdb_size
// in sync on all code paths.

ex_root_tdb * Statement::assignRootTdb(ex_root_tdb *new_root_tdb)
{
  if (root_tdb == new_root_tdb)
    return root_tdb;

  if (root_tdb)
  {
    // dealloc() will release the TCB tree for this statement and will
    // also call dealloc() for any cloned statements
    dealloc();
    
    if (!isCloned())
    {
      // solution 10-040803-8511: root_tdb is not unpacked for SHOWPLAN
      // the lateNameInfoList is still the offset, not valid pointer
      if (!root_tdb->isPacked() && root_tdb->getLateNameInfoList() != NULL)
        for (Int32 i = 0;
             i < (Int32) (root_tdb->getLateNameInfoList()->getNumEntries());
             i++)
          root_tdb->getLateNameInfoList()->
            getLateNameInfo(i).zeroLastUsedAnsiName();     
      CollHeap *h = cliGlobals_->getArkcmp()->getHeap();
      h->deallocateMemory((void *) root_tdb);
      StmtDebug2("  Stmt %p deallocated root TDB %p", this, root_tdb);
    }
  }

  root_tdb = new_root_tdb;
  StmtDebug2("  Stmt %p root TDB is now %p", this, root_tdb);

#ifdef _DEBUG
  Lng32 rs = (Lng32) (root_tdb ? root_tdb->getMaxResultSets() : 0);
  if (rs > 0)
    StmtDebug1("  Max result sets: %d", rs);
#endif

  clonedStatements->position();
  Statement *clone = (Statement *) clonedStatements->getNext();
  for (; clone != NULL; clone = (Statement *) clonedStatements->getNext())
  {
    clone->root_tdb = new_root_tdb;
    StmtDebug2("  Clone %p root TDB is now %p", clone, new_root_tdb);
  }

  return root_tdb;
}

RETCODE Statement::addDescInfoIntoStaticDesc(Descriptor * desc,
					     Lng32 what_desc,
					     ComDiagsArea &diagsArea)
{
  if(!root_tdb){
    return SUCCESS;
  }

  InputOutputExpr *expr;
  enum ex_expr::exp_return_type expRType;

  Int32 retcode = 0; // be optimistic

  // A special case for DESCRIBE is when the SQL statement is a CALL
  // statement and the CLI caller wants to use a "wide" descriptor for
  // the CALL.
  if (root_tdb->hasCallStmtExpressions() && desc->isDescTypeWide())
  {
    // First populate the caller's descriptor with information from
    // the INPUT expression, the overlay information from the OUTPUT
    // expression into the same descriptor.

    if(expr = root_tdb->inputExpr())
    {
      expRType = expr->addDescInfoIntoStaticDesc(desc, TRUE /*input*/);
      if(expRType != ex_expr::EXPR_OK)
	retcode = -1;
    }
    
    if((retcode == 0) && (expr = root_tdb->outputExpr()))
    {
      expRType = expr->addDescInfoIntoStaticDesc(desc, FALSE /*output*/);
      if(expRType != ex_expr::EXPR_OK)
	retcode = -1;
    }
  }
  else{
    NABoolean isInput;
    if (what_desc == SQLWHAT_INPUT_DESC){
      expr = root_tdb->inputExpr();      // input expr -> desc
      isInput = TRUE;
    }
    else{
      expr = root_tdb->outputExpr();	// output expr -> desc
      isInput = FALSE;
    }
    if(expr)
    {
      expRType = expr->addDescInfoIntoStaticDesc(desc, isInput);
      if(expRType != ex_expr::EXPR_OK)
	retcode = -1;
    }
  }
  
  if (retcode)
  {
    diagsArea << DgSqlCode(- CLI_TDB_DESCRIBE_ERROR);
    return ERROR;
  }

  return SUCCESS;
}

NABoolean Statement::containsUdrInteractions() const
{
  return (root_tdb && root_tdb->containsUdrInteractions());
}

//------------------------------------------------------------------------------
// 
// For each stoi that is marked as subject table, get trigger status array from 
// the corresponding rfork. This status array is used to update the status of the
// relevant triggers in the call to updateTriggerStatusPerTable()
//
RETCODE Statement::getTriggersStatus(SqlTableOpenInfoPtr* stoiList, ComDiagsArea &diagsArea)
{
  
  TriggerStatusWA triggerStatusWA(&heap_, root_tcb);
  SqlTableOpenInfo* stoi;
  RETCODE rc = SUCCESS;
  
  for (Int32 i=0; i< root_tcb->getTableCount(); i++)
    {
      stoi=stoiList[i];
      if (stoi->subjectTable())
        {
          
          // save the current diags before entering CLI again
          ComDiagsArea* copyOfDiagsArea = diagsArea.copy(); 
          // remove warning 100 from diags.
          diagsArea.removeFinalCondition100();
          
          // merge diags
          diagsArea.mergeAfter(*copyOfDiagsArea);
          copyOfDiagsArea->decrRefCount();
          copyOfDiagsArea = NULL;
          
          // select the status of the triggers relevant to this statement
          // and update them in the root_tcb->triggerStatusVector_
          triggerStatusWA.updateTriggerStatusPerTable();
          triggerStatusWA.deallocateStatusArray();
        }
    }
  
#ifdef _DEBUG
  if (getenv("SHOW_ENABLE"))
    {
      char int64Str[128];
      cout << "Trigger Ids in TDB:" << endl;
      cout << "-------------------" << endl;
      for (Int32 i=0; i<triggerStatusWA.getTotalTriggersCount(); i++) 
        {
          convertInt64ToAscii(root_tdb->getTriggersList()[i], int64Str);
          cout << i << " : " << int64Str << endl;
        }
      cout << endl;
      
    }
#endif //_DEBUG

  // we cannot check that status was received for all triggers, since
  // triggers may be dropped/added before similarity check is done...
  // if (triggerStatusWA.getTotalTriggersCount() == root_tdb->getTriggersCount())
  return rc;
  
}

// ++ MV -
// isIudTargetTable returns TRUE if the table is the target of an
// insert/update/delete operation, or if the table does not appear in stoi.
NABoolean Statement::isIudTargetTable(char                *tableName, 
				      SqlTableOpenInfoPtr *stoiList)
{
  NABoolean found = FALSE;
  for (Int32 i=0; root_tcb && i < root_tcb->getTableCount(); i++)
    {
      SqlTableOpenInfo *stoi=stoiList[i];

      if (! strcmp(stoi->ansiName(), tableName))
      {
	found = TRUE;
	if (stoi->getInsertAccess() || 
	    stoi->getUpdateAccess() || 
	    stoi->getDeleteAccess())
	return TRUE;
      }
    }

  if (found)
    return FALSE;
  else
    return TRUE;
}

RETCODE Statement::mvSimilarityCheck(char         *table, 
				     ULng32 siMvBitmap, 
				     ULng32 rcbMvBitmap,
				     NABoolean    &simCheckFailed,
				     ComDiagsArea &diagsArea)
{ 
  ComMvAttributeBitmap	siBitmap;
  ComMvAttributeBitmap	rcbBitmap;
  NABoolean iud = isIudTargetTable(table, root_tdb->stoiStoiList());

  siBitmap.initBitmap((ComSInt32)siMvBitmap);
  rcbBitmap.initBitmap((ComSInt32)rcbMvBitmap);

  // if it is INSERT/UPDATE/DELETE statement and table/MV has initialized 
  // ON STATEMENT MV on it - recompile
  if (iud &&
      rcbBitmap.getInitOnStmtMvOnMe())
  {
    if (returnRecompWarn())
      diagsArea << DgSqlCode(EXE_SIM_CHECK_FAILED)
		<< DgString0("Table has on statement MVs on it."); 
    simCheckFailed = TRUE;
    return SUCCESS;
  }

  // if it is INSERT/UPDATE/DELETE statement and table/MV had initialized 
  // ON STATEMENT MV on it - recompile
  if (iud &&
      siBitmap.getInitOnStmtMvOnMe() != rcbBitmap.getInitOnStmtMvOnMe())
  {
    if (returnRecompWarn())
      diagsArea << DgSqlCode(EXE_SIM_CHECK_FAILED)
		<< DgString0("On statement MVs have been removed."); 
    simCheckFailed = TRUE;
    return SUCCESS;
  }

  // if it is INSERT/UPDATE/DELETE statement and table/MV logging required 
  // bit was changed - recompile
  if (iud && siBitmap.getLoggingRequired() != rcbBitmap.getLoggingRequired())
  {
    if (rcbBitmap.getLoggingRequired())
    {
      if (returnRecompWarn())
	diagsArea << DgSqlCode(EXE_SIM_CHECK_FAILED)
	<< DgString0("Logging is required.");
    }
    else
    {
      if (returnRecompWarn())
	diagsArea << DgSqlCode(EXE_SIM_CHECK_FAILED)
		  << DgString0("Logging is not required any more.");
    }

    simCheckFailed = TRUE;
    return SUCCESS;
  }
  
  // if MV enable rewrite bit was change to disable rewrite - recompile
  if (siBitmap.getIsEnableRewrite()  && !rcbBitmap.getIsEnableRewrite())
  {
    if (returnRecompWarn())
      diagsArea << DgSqlCode(EXE_SIM_CHECK_FAILED)
		<< DgString0("MV rewrite was disabled.");
    simCheckFailed = TRUE;
    return SUCCESS;
  }

  // if MV audit bits were changed, but not between the values 
  // AUDIT <--> AUDITONREFRESH - recompile
  ComMvAuditType siMvAuditType = siBitmap.getMvAuditType();
  ComMvAuditType rcbMvAuditType = rcbBitmap.getMvAuditType();
  if (siMvAuditType != rcbMvAuditType &&
      !((siMvAuditType  == COM_MV_NO_AUDIT_ON_REFRESH && 
         rcbMvAuditType == COM_MV_AUDIT)                ||
        (rcbMvAuditType == COM_MV_NO_AUDIT_ON_REFRESH && 
         siMvAuditType  == COM_MV_AUDIT)))
  {
    if (returnRecompWarn())
      diagsArea << DgSqlCode(EXE_SIM_CHECK_FAILED)
		<< DgString0("MV audit status was changed.");
    simCheckFailed = TRUE;
    return SUCCESS;
  }


  // if table insertlog bit was changed - recompile
  if (iud && siBitmap.getIsInsertLog() != rcbBitmap.getIsInsertLog())
  {
    if (returnRecompWarn())
      diagsArea << DgSqlCode(EXE_SIM_CHECK_FAILED)
		<< DgString0("Table INSERTLOG attribute was changed.");
    simCheckFailed = TRUE;
    return SUCCESS;
  }

  // if statement is INSERT/UPDATE/DELETE on table and logging is required 
  // and range log bits was changed, but not between the values 
  // NO RANGELOG <--> MIX RANGELOG - recompile
  ComRangeLogType siRangeLogType = siBitmap.getRangeLogType();
  ComRangeLogType rcbRangeLogType = rcbBitmap.getRangeLogType();
  if (iud                                            && 
      siBitmap.getLoggingRequired()                  &&
      siRangeLogType != rcbRangeLogType              &&
      !((siRangeLogType  == COM_NO_RANGELOG     && 
         rcbRangeLogType == COM_MIXED_RANGELOG)    ||
	(rcbRangeLogType == COM_NO_RANGELOG     && 
	 siRangeLogType  == COM_MIXED_RANGELOG))
      )
  {
    if (returnRecompWarn())    
      diagsArea << DgSqlCode(EXE_SIM_CHECK_FAILED)
		<< DgString0("Table RANGELOG attribute was changed.");
    simCheckFailed = TRUE;
    return SUCCESS;
  }

  // MV is unavailable 
  if (rcbBitmap.getMvStatus() == COM_MVSTATUS_UNAVAILABLE)
  {
    if (returnRecompWarn())
      diagsArea << DgSqlCode(EXE_SIM_CHECK_FAILED)
		<< DgString0("Materialized view is not available.");
      simCheckFailed = TRUE;
      return SUCCESS;
  }

  simCheckFailed = FALSE;
  return SUCCESS;
}

// Discover whether there is already a transaction active
// so that we can avoid starting one when we call CatMapGetCatalogVisibilty,
// CatMapAnsiNameToGuardianName, or other recursive CLI calls.
NABoolean Statement::implicitTransNeeded(void)
{
  const NABoolean noTransInProgress = 
      (context_->getTransaction()->xnInProgress() == FALSE);
  if (noTransInProgress)
    anyTransWasStartedByMe_ = TRUE;
  return noTransInProgress;
}

// If autocommit is on, disable it, prior to making recursive CLI calls.
// Remember if we disabled autoCommit so we can enable before returning 
// from this method.
void Statement::turnOffAutoCommit(void)
{
  if (context_->getTransaction()->autoCommit())
    {
      autoCommitCleared_ = TRUE;
      context_->getTransaction()->disableAutoCommit();
    }
}


// Turn back on autoCommit if we had turned it off.  Maintain the 
// state variable so the statement can be reexecuted.
void Statement::resetAutoCommit(void)
{
  if (autoCommitCleared_)
  {
    autoCommitCleared_ = FALSE;
    context_->getTransaction()->enableAutoCommit();
  }
}
void Statement::saveTmodeValues(void)
{
  ExTransaction  *runTrans = context_->getTransaction();

  if (runTrans)
    {
      savedRoVal_ = runTrans->getROVal();
      savedRbVal_ = runTrans->getRBVal();
      savedAiVal_ = runTrans->getAIVal();
    }
}

void Statement::resetTmodeValues(void)
{
  ExTransaction * runTrans = context_->getTransaction();

  if (runTrans)
    {
      
      runTrans->updateROVal(savedRoVal_);
      runTrans->updateRBVal(savedRbVal_);
      runTrans->updateAIVal(savedAiVal_);
    }
}

// Commit any transaction that we started.  Turn back on autoCommit if we 
// had turned it off.  Maintain the state variables so the statement can be
// reexecuted.
void Statement::commitImplicitTransAndResetTmodes(void)
{
  if (anyTransWasStartedByMe_)
  {
    anyTransWasStartedByMe_ = FALSE;
    if (context_->getTransaction()->xnInProgress())
    {
      // Don't care about any error -- this was a readonly transaction.
      context_->getTransaction()->commitTransaction();
    }
  }
  resetAutoCommit();
  resetTmodeValues();
}


ExStatisticsArea *Statement::getStatsArea()
{
  if (getGlobals())
    return getGlobals()->getStatsArea();
  return NULL;
}

ExStatisticsArea *Statement::getOrigStatsArea()
{
  if (getGlobals())
    return getGlobals()->getOrigStatsArea();
  return NULL;
}

ExStatisticsArea *Statement::getCompileStatsArea()
{
  ExStatisticsArea *stats;
  if ((stats = getStatsArea()) != NULL)
    return stats;
  if (compileStatsArea_ != NULL)
    return compileStatsArea_;
  if (stmtStats_ == NULL || stmtStats_->getMasterStats() == NULL)
    return NULL;
  compileStatsArea_ = new (&heap_) 
    ExStatisticsArea(&heap_, 0, getRootTdb()->getCollectStatsType(),
      getRootTdb()->getCollectStatsType());
  ExMasterStats *masterStats = new(&heap_) ExMasterStats(&heap_);
  masterStats->copyContents(stmtStats_->getMasterStats());
  masterStats->setCollectStatsType(getRootTdb()->getCollectStatsType());
  compileStatsArea_->setMasterStats(masterStats);
  return compileStatsArea_;
}

void Statement::setStmtStats(NABoolean autoRetry)
{
  StatsGlobals *statsGlobals = NULL;
  int error;
  NABoolean stmtStatsRetained = FALSE;
  StmtStats *stmtStats = NULL;
  if (stmtStats_ != NULL)
  {
    if (getRootTdb())
      assignRootTdb (NULL);
  }
  statsGlobals = cliGlobals_->getStatsGlobals();
  if (statsGlobals != NULL)
  {
    error = statsGlobals->getStatsSemaphore(cliGlobals_->getSemId(),
                  cliGlobals_->myPin());
    if (autoRetry)
    {
      if (getUniqueStmtId() != NULL)
      {
        stmtStats = statsGlobals->getMasterStmtStats(getUniqueStmtId(), getUniqueStmtIdLen(), 
              RtsQueryId::ANY_QUERY_);
        ex_assert(stmtStats, "AQR but missing stmtStats.");
        stmtStatsRetained = TRUE;
      }
    }
    else
    {
      // If the same statement handle is used to prepare the query again
      // stmtStats_ would have been set, remove the query in that case
      if (stmtStats_ != NULL)
      {
        if (stmtStats_->getMasterStats() != NULL)
        {
          stmtStats_->getMasterStats()->setStmtState(Statement::DEALLOCATED_);
	  stmtStats_->getMasterStats()->setEndTimes(! stmtStats_->aqrInProgress());
        }
        // Make a copy of stmtStats_ pointer so that in case it is retained we could use 
        // the same stmtStats-
        stmtStats = stmtStats_;
        stmtStatsRetained = statsGlobals->removeQuery(cliGlobals_->myPin(), stmtStats_);
      }
    }
    if (getUniqueStmtId() != NULL)
    {
      if (! stmtStatsRetained)
      {
        stmtStats_ = statsGlobals->addQuery(cliGlobals_->myPin(), 
              getUniqueStmtId(),
              getUniqueStmtIdLen(), (void *)this, (Lng32)-1,
              source_str, source_length, TRUE);
        ex_assert(stmtStats_, "StmtStats_ is null after addQuery");
      }
      else
      {
        ExStatisticsArea *myStats = stmtStats->getStatsArea();
        stmtStats->reuse((void *)this);
        getGlobals()->setStatsArea(NULL);
        if (myStats != NULL && context_->getStats() == myStats)
          context_->setStatsArea(NULL, FALSE, FALSE, FALSE);
        stmtStats_ = stmtStats;
      }
      // update the parentQid
      char *parentQid = getParentQid();
      char *parentQidSystem = getParentQidSystem();
      ULng32 parentQidSystemLen; 
      if (parentQidSystem != NULL)
         parentQidSystemLen = str_len(parentQidSystem);
      else
         parentQidSystemLen = 0; 
      if (parentQid != NULL)
        stmtStats_->setParentQid(parentQid, str_len(parentQid), parentQidSystem, 
                  parentQidSystemLen, cliGlobals_->myCpu(), (short)cliGlobals_->myNodeNumber());
      else
        stmtStats_->setParentQid(NULL, 0, NULL, 0, cliGlobals_->myCpu(), (short)cliGlobals_->myNodeNumber());
    }
    else
      stmtStats_ = NULL;
    statsGlobals->releaseStatsSemaphore(cliGlobals_->getSemId(), cliGlobals_->myPin());
  }
  else
  {
    if (stmtStats_ != NULL)
    {
      NADELETE(stmtStats_, StmtStats, stmtStats_->getHeap());
      stmtStats_ = NULL;
    }   
    // runtime stats not supported on NT platform or runtime stats
    // subsystem not up and running.
    // Allocate ExMasterStats without going thru StatsGlobals.
    stmtStats_ = StatsGlobals::addStmtStats(stmtHeap(),
					cliGlobals_->myPin(), 
					getUniqueStmtId(),
					getUniqueStmtIdLen(),
                                        source_str, source_length);
  }
}

// VO, Plan Versioning Support.
void Statement::issuePlanVersioningWarnings (ComDiagsArea & diagsArea)
{
  if (returnRecompWarn())
  {
    if (fetchErrorCode_ != VERSION_NO_ERROR)
    {
      // A plan versioning error was detected at fetch time
      diagsArea << DgSqlCode (fetchErrorCode_)
                << DgInt0 (fetchPlanVersion_)
                << DgString0 (fetchNode_)
                << DgInt1 (fetchSupportedVersion_);
    }

    if (mxcmpErrorCode_ != VERSION_NO_ERROR)
    {
      // A plan versioning error was detected at prepare time
      diagsArea << DgSqlCode (mxcmpErrorCode_)
                << DgInt0 (mxcmpStartedVersion_)
                << DgInt1 (COM_VERS_COMPILER_VERSION);
    }
  }

  // Unconditionally clear the saved warning codes, even though we
  // may not have reported them - we don't want them hanging around
  // if we recompile for some other reason.
  fetchErrorCode_ = VERSION_NO_ERROR;
  mxcmpErrorCode_ = VERSION_NO_ERROR;
}
const char *Statement::stmtState(State state) 
{
  switch (state)
  {
    case INITIAL_:       return "INITIAL";
    case OPEN_:          return "OPEN";
    case EOF_:           return "EOF";
    case CLOSE_:         return "CLOSE";
    case DEALLOCATED_:   return "DEALLOCATED";
    case FETCH_:         return "FETCH";
    case CLOSE_TABLES_:  return "CLOSE_TABLES";
    case PREPARE_:       return "PREPARE";
    case PROCESS_ENDED_: return "PROCESS_ENDED";
    case RELEASE_TRANS_: return "RELEASE_TRANS";
    case SUSPENDED_:     return "SUSPENDED";
    case STMT_EXECUTE_:  return "EXECUTE";
    case STMT_FIXUP_:    return "FIXUP";
    default:             return ComRtGetUnknownString((Int32) state);
  }
}

ExRsInfo *Statement::getResultSetInfo() const
{
  if (statementGlobals_)
    return statementGlobals_->getResultSetInfo();
  return NULL;
}

ExRsInfo *Statement::getOrCreateResultSetInfo()
{
  ex_assert(statementGlobals_, "No statement globals available");
  return statementGlobals_->getResultSetInfo(TRUE);
}

// For stored procedure result set proxy statements, return TRUE if
// the current statement source matches the newSource input string.
NABoolean Statement::rsProxyCompare(const char *newSource) const
{
  NABoolean result = FALSE;

  if (newSource != NULL && source_str != NULL &&
      charset_ == SQLCHARSETCODE_ISO88591 &&
      str_cmp_ne(source_str, newSource) == 0)
  {
    result = TRUE;
  }

  return result;
}

// For stored procedure result set proxy statements, see if a
// prepare of proxy syntax is required and if so, do the internal 
// prepare
RETCODE Statement::rsProxyPrepare(ExRsInfo &rsInfo,          // IN
                                  ULng32 rsIndex,     // IN
                                  ComDiagsArea &diagsArea)   // INOUT
{
  StmtDebug3("[BEGIN rsProxyPrepare] %p, rsIndex %u, stmt state %s",
             this, rsIndex, stmtState(getState()));

  RETCODE result = SUCCESS;

  // These variables will be populated below by the call to
  // getRsInfo()
  NABoolean preparedFlag = FALSE;
  Statement *proxyStmt = NULL;
  NABoolean openFlag = FALSE;
  NABoolean closedFlag = FALSE;

  // We have the ability to acquire proxy strings from the
  // environment. The mechanism is currently disabled. See comments in
  // the getProxySyntaxFromEnvironment() function (in this file) for
  // more info.
  const char *proxySyntax = NULL;

  if (proxySyntax)
  {
    StmtDebug0("  *** WILL USE PROXY SYNTAX FROM AN ENV VAR");
  }
  else
  {
    NABoolean found = rsInfo.getRsInfo(rsIndex, proxyStmt, proxySyntax,
                                       openFlag, closedFlag, preparedFlag);
  }

  StmtDebug2("  rsIndex %u, prepared flag %d", rsIndex, (Int32) preparedFlag);
  StmtDebug1("  proxySyntax [%s]", (proxySyntax ? proxySyntax : "(NULL)"));
  
  if (proxySyntax == NULL)
  {
    diagsArea << DgSqlCode(-EXE_UDR_RS_NOT_AVAILABLE)
              << DgInt0((Int32) rsIndex);
    result = ERROR;
  }
  
  if (result != ERROR)
  {
    if (!preparedFlag)
    {
      NABoolean okToReusePlan = FALSE;
      if (stmt_state != INITIAL_ && root_tdb != NULL)
      {
        // This means we currently have a plan. Next step is to decide
        // if it suitable for reuse.
        if (rsProxyCompare(proxySyntax))
          okToReusePlan = TRUE;
      }

      if (!okToReusePlan)
      {
        // $$$$ PROXY RECOMPILE: We can consider returning a new
        // warning condition when a proxy is prepared. This may help
        // CLI callers avoid doing describes on a proxy after every
        // CALL execution.
        result = prepare((char *) proxySyntax, diagsArea, NULL, 0L);
      }

      if (result != ERROR)
      {
        rsInfo.setPrepared(rsIndex);
      }
      else
      {
        // Add a condition to the diags area indicating it was an
        // internal operation that failed unexpectedly
        diagsArea << DgSqlCode(-EXE_UDR_RS_ALLOC_INTERNAL_ERROR);
      }
    }
  }
  
  StmtDebug3("[END rsProxyPrepare] %p, stmt state %s, retcode %s",
             this, stmtState(getState()), RetcodeToString(result));
  
  return result;
}

// Helper function to convert a char string to quoted char string.
// Also, single quote chars will be replaced with two single quote chars.
static
char* toQuotedString(char *src, Lng32 srcLen, NAHeap *heap)
{
  // Allocate a buffer to fix the resultant string.
  char *target = new (heap) char[2 * srcLen + 3];
  char *targetPtr = target;

  *targetPtr++ = '\'';  // beginning quote

  for (Lng32 pos = 0; pos < srcLen; pos++)
  {
    *targetPtr++ = src[pos];

    if (src[pos] == '\'')
      *targetPtr++ = '\'';
  }
  *targetPtr++ = '\'';  // ending quote
  *targetPtr = '\0';  // NULL terminator

  return target;
}

// getProxySyntax generates the text of a SELECT statement that we
// call "proxy syntax". The two uses of these SELECTs are stored
// procedure result sets and parallel extract consumer queries.
//
// The result set proxy syntax is of the form:
// 
//     SELECT * FROM TABLE ( SP_RESULT_SET ( <col-type-spec>,
//                                           <col-type-spec>, ... ) )
//
// where <col-type-spec> is in the format
//  <cat>.<sch>.<table>.<colname> datatype [HEADING '<heading>'] [NOT NULL]
//
// The extract consumer syntax is of the form:
// 
//     SELECT * FROM TABLE ( EXTRACT_SOURCE ( 'ESP', 'esp-process-handle',
//                                            'security-key', 
//                                            <col-type-spec>,
//                                            <col-type-spec>, ... ) )
//
// The getProxySyntax function takes a prefix and suffix as input and
// generates the string:
//
//    <prefix> <col-type-spec>, ... <suffix>
//
//  In addition to generating proxy syntax, this method also computes
//  the size of the proxy syntax string.
//
//  This method first assumes that 'proxy' with 'maxlength' size
//  is sufficient for holding proxy syntax. If it realizes that
//  'proxy' does not have enough space then the copying stops but the
//  function proceeds to compute the required size
//
//  The size of proxy syntax will be returned in spaceRequired.
//
RETCODE Statement::getProxySyntax(char *proxy, Lng32 maxlength,
                                  Lng32 *spaceRequired,
                                  const char *prefix, const char *suffix)
{
  // In the code below, once we set hasEnoughSpace to FALSE,
  // it will never be changed to TRUE again.
  NABoolean hasEnoughSpace = TRUE;

  // If proxy is NULL, we only compute the size of proxy syntax
  // if spaceRequired is provided.
  if (!proxy) hasEnoughSpace = FALSE;

  if (!proxy && ! spaceRequired)
    return SUCCESS;

  InputOutputExpr *expr = getRootTdb()->outputExpr();
  if (expr == NULL)
  {
    if (spaceRequired) *spaceRequired = 0;
    return SUCCESS;
  }

  SQLMODULE_ID mod_id;
  SQLDESC_ID desc_id;

  mod_id.module_name = 0;
  desc_id.name_mode = desc_handle;
  desc_id.identifier = 0;
  desc_id.handle = 0;
  desc_id.module = &mod_id;

  Descriptor *outdesc = new (&heap_) Descriptor(&desc_id,
                                                expr->getNumEntries(),
                                                context_);
  outdesc->dealloc();
  outdesc->alloc(expr->getNumEntries());

  // Describe the OUTPUT expression
  UInt32 flags = 0;
  expr->setIsODBC(flags, root_tdb->odbcQuery());
  expr->setInternalFormatIO(flags, 
			    context_->getSessionDefaults()->
			    getInternalFormatIO());
  expr->setIsDBTR(flags,
		  context_->getSessionDefaults()->getDbtrProcess());
  expr->describeOutput(outdesc, flags);

  // Now generate proxy syntax string
  char *proxyPtr = proxy;
  Lng32 spaceLeft = maxlength;
  Lng32 spaceNeeded = 0;

  if (prefix == NULL)
    prefix = "";
  if (suffix == NULL)
    suffix = "";

  Lng32 prefixLen = str_len(prefix);
  Lng32 suffixLen = str_len(suffix);

  if (spaceLeft < prefixLen) hasEnoughSpace = FALSE;
  spaceNeeded += prefixLen;
  if (hasEnoughSpace)
  {
    str_cpy_all(proxyPtr, prefix, prefixLen);
    proxyPtr += prefixLen;
    spaceLeft -= prefixLen;
  }

  for (Int32 entry = 1; entry <= expr->getNumEntries(); entry++)
  {
    char *name = NULL;
    Lng32 nameLen;

    // get CATALOG attribute
    outdesc->getDescItemPtr(entry, SQLDESC_CATALOG_NAME, &name, &nameLen);
    if (name != NULL)
    {
      char *catName = ToAnsiIdentifier2(name, nameLen, &heap_);
      Lng32 catLen = str_len(catName);

      // Write CATALOG attribute
      if (spaceLeft < catLen + 1) hasEnoughSpace = FALSE;
      spaceNeeded += (catLen + 1);
      if (hasEnoughSpace)
      {
        str_cpy_all(proxyPtr, catName, catLen);
        str_cpy_all(proxyPtr+catLen, ".", 1);
        proxyPtr += (catLen + 1);
        spaceLeft -= (catLen + 1);
      }
      heap_.deallocateMemory(catName);
    }

    // get SCHEMA attribute
    outdesc->getDescItemPtr(entry, SQLDESC_SCHEMA_NAME, &name, &nameLen);
    if (name != NULL)
    {
      char *schName = ToAnsiIdentifier2(name, nameLen, &heap_);
      Lng32 schLen = str_len(schName);

      // Write SCHEMA attribute
      if (spaceLeft < schLen + 1) hasEnoughSpace = FALSE;
      spaceNeeded += (schLen + 1);
      if (hasEnoughSpace)
      {
        str_cpy_all(proxyPtr, schName, schLen);
        str_cpy_all(proxyPtr+schLen, ".", 1);
        proxyPtr += (schLen + 1);
        spaceLeft -= (schLen + 1);
      }
      heap_.deallocateMemory(schName);
    }

    // get TABLE attribute
    outdesc->getDescItemPtr(entry, SQLDESC_TABLE_NAME, &name, &nameLen);
    if (name != NULL)
    {
      char *tabName = ToAnsiIdentifier2(name, nameLen, &heap_);
      Lng32 tabLen = str_len(tabName);

      // Write TABLE attribute
      if (spaceLeft < tabLen + 1) hasEnoughSpace = FALSE;
      spaceNeeded += (tabLen + 1);
      if (hasEnoughSpace)
      {
        str_cpy_all(proxyPtr, tabName, tabLen);
        str_cpy_all(proxyPtr+tabLen, ".", 1);
        proxyPtr += (tabLen + 1);
        spaceLeft -= (tabLen + 1);
      }

      heap_.deallocateMemory(tabName);
    }

    // get COLUMN Name attribute
    outdesc->getDescItemPtr(entry, SQLDESC_NAME, &name, &nameLen);
    if (name != NULL)
    {
      char *colName = ToAnsiIdentifier2(name, nameLen, &heap_);
      Lng32 colLen = str_len(colName);

      // Write COLUMN attribute
      if (spaceLeft < colLen + 1) hasEnoughSpace = FALSE;
      spaceNeeded += (colLen + 1);
      if (hasEnoughSpace)
      {
        str_cpy_all(proxyPtr, colName, colLen);
        str_cpy_all(proxyPtr+colLen, " ", 1);
        proxyPtr += (colLen + 1);
        spaceLeft -= (colLen + 1);
      }

      heap_.deallocateMemory(colName);
    }

    // get Dataype TEXT FORMAT attribute
    outdesc->getDescItemPtr(entry, SQLDESC_TEXT_FORMAT, &name, &nameLen);
    if (name != NULL)
    {
      // Write Datatype attribute
      if (spaceLeft < nameLen + 1) hasEnoughSpace = FALSE;
      spaceNeeded += nameLen + 1;
      if (hasEnoughSpace)
      {
        str_cpy_all(proxyPtr, name, nameLen);
        str_cpy_all(proxyPtr + nameLen, " ", 1);
        proxyPtr += (nameLen + 1);
        spaceLeft -= (nameLen + 1);
      }
    }

    // get HEADING attribute
    outdesc->getDescItemPtr(entry, SQLDESC_HEADING, &name, &nameLen);
    if ((name != NULL) && (str_len(name) != 0))
    {
      // quotedHeading will be quoted string format i.e., the string will be
      // in single quotes and also any quotes in the string are doubled
      char *quotedHeading = toQuotedString(name, nameLen, &heap_);
      Lng32 quotedHeadLen = str_len(quotedHeading);

      // Write HEADING attribute
      if (spaceLeft < quotedHeadLen + 18) hasEnoughSpace = FALSE;
      spaceNeeded += (quotedHeadLen + 18);
      if (hasEnoughSpace)
      {
        str_cpy_all(proxyPtr, "HEADING _ISO88591", 17);
        str_cpy_all(proxyPtr+17, quotedHeading, quotedHeadLen);
        str_cpy_all(proxyPtr+17+quotedHeadLen, " ", 1);
        proxyPtr += (quotedHeadLen + 18);
        spaceLeft -= (quotedHeadLen + 18);
      }

      heap_.deallocateMemory(quotedHeading);
    }

    // Write "NOT NULL" attribute
    Lng32 nullable;
    outdesc->getDescItem(entry, SQLDESC_NULLABLE, &nullable, NULL, 0, NULL, 0);
    if (nullable == 0)
    {
      if (spaceLeft < 8) hasEnoughSpace = FALSE;
      spaceNeeded += 8;
      if (hasEnoughSpace)
      {
        str_cpy_all(proxyPtr, "NOT NULL", 8);
	proxyPtr += 8;
	spaceLeft -= 8;
      }
    }

    if (entry == expr->getNumEntries())
    {
      // This is last entry, so append the suffix
      if (spaceLeft < suffixLen)
        hasEnoughSpace = FALSE;
      spaceNeeded += suffixLen;
      if (hasEnoughSpace)
      {
        str_cpy_all(proxyPtr, suffix, suffixLen);
	proxyPtr += suffixLen;
	spaceLeft -= suffixLen;
      }
    }
    else
    {
      // Add ','
      if (spaceLeft < 2) hasEnoughSpace = FALSE;
      spaceNeeded += 2;
      if (hasEnoughSpace)
      {
        str_cpy_all(proxyPtr, ", ", 2);
        proxyPtr += 2;
        spaceLeft -= 2;
      }
    }
  }

  // Set the required space size if asked.
  if (spaceRequired) *spaceRequired = spaceNeeded;

  // Clean up
  outdesc->dealloc();
  NADELETE(outdesc, Descriptor, &heap_);

  return SUCCESS;
}

RETCODE Statement::getRSProxySyntax(char *proxy, Lng32 maxlength,
                                    Lng32 *spaceRequired)
{
  return getProxySyntax(proxy, maxlength, spaceRequired,
                        "SELECT * FROM TABLE ( SP_RESULT_SET ( ", " ) )");
}

RETCODE Statement::getExtractConsumerSyntax(char *proxy, Lng32 maxlength,
                                            Lng32 *spaceRequired)
{
  const char *prefix =
    "SELECT * FROM TABLE(EXTRACT_SOURCE('ESP', '%s', '%s', ";
  const char *suffix = ") )";
  return getProxySyntax(proxy, maxlength, spaceRequired, prefix, suffix);
}

#ifdef _DEBUG
void Statement::StmtPrintf(const char *formatString, ...) const
{
  if (!stmtDebug_)
    return;

  FILE *f = stdout;
  va_list args;
  va_start(args, formatString);
  fprintf(f, "[STMT] ");
  vfprintf(f, formatString, args);
  fprintf(f, "\n");
  fflush(f);
}
#endif
/******************************************************************************
    Method: Statement::doesUninitializedMvExist

    Description:       
        Any time the statement is executed, check the list to see if
        any mvs are still uninitialized.  If they are initialized, remove
        them from the list.

        If one uninitialized mv is found, return immediately with error
        since this statement cannot be executed.

    Parameters:
        char **pMvName (OUTPUT)
            - this parameter will be set to the name of the uninitialized
              mv if it is found.

        ComDiagsArea &diagsArea                   
            - diags area for error messages              

    Return:
        - return TRUE if an uninitialized mv is found          
                         
******************************************************************************/
NABoolean 
Statement::doesUninitializedMvExist( char **pMvName,
                                     ComDiagsArea &diagsArea )
{  
   NABoolean bUninitializedMvExists = FALSE; // assume none exist   
  
   UninitializedMvName *uninitializedMvList = NULL;
   const short uninitializedMvCount = root_tdb->uninitializedMvCount();

   uninitializedMvList = root_tdb->uninitializedMvList();

   // if the list is NULL, the return value will show that none exist.
   if( uninitializedMvList )
   {
       // for each name in the uninitializedMvList
       for( short i = 0; i < uninitializedMvCount; i++ )
       {           
           UninitializedMvName currentMv = uninitializedMvList[ i ];

           if( isUninitializedMv( currentMv.getPhysicalName(),
                                  currentMv.getAnsiName(), 
                                  diagsArea ) )
           {           
               // point output to name in root_tdb
               (*pMvName) = uninitializedMvList[ i ].getAnsiName(); 
               bUninitializedMvExists = TRUE;
               break; // exit loop since at least one is found
           } 
           
       }    
   }   
   return bUninitializedMvExists;
}
/******************************************************************************
    Method: Statement::isUninitializedMv

    Description:
        Determine whether a table in the statement is an uninitialized mv.  
        This information is gathered from the ExRcb object.

    Parameters:
        char * physicalName
            - table location.  used to get rfork info
            
        const char *ansiName             
            - table ansi name.  used for error reporting

        ComDiagsArea &diagsArea                   
            - diags area for error messages
              
    Return:
        - return TRUE if this mv exists but is uninitialized.             
                                
******************************************************************************/
NABoolean 
Statement::isUninitializedMv( const char * physicalName,
                              const char * ansiName,
                              ComDiagsArea &diagsArea )
{
    return FALSE;
}

void Statement::buildConsumerQueryTemplate()
{
  if (extractConsumerQueryTemplate_ == NULL)
  {
    Lng32 len = 1000;
    NABoolean done = FALSE;
    Lng32 spaceRequired = 0;
    while (!done)
    {


      // add one for NULL terminator
      extractConsumerQueryTemplate_ = (char *)heap_.allocateMemory(len + 1);

      // Now we call a function to generate the consumer syntax string.
      // The function returns a string WITHOUT a null terminator. The
      // number of bytes required for the full string (not including
      // the null terminator) is returned in the output parameter 
      // spaceRequired.
      RETCODE rc = getExtractConsumerSyntax(extractConsumerQueryTemplate_,
                                            len,
                                            &spaceRequired);
      ex_assert(rc == SUCCESS,
                "Unexpected error from getExtractConsumerSyntax");

      if (spaceRequired > len)
      {
        len = spaceRequired;
        heap_.deallocateMemory(extractConsumerQueryTemplate_);
        extractConsumerQueryTemplate_ = NULL;
      }
      else
      {
        extractConsumerQueryTemplate_[spaceRequired] = 0;
        done = TRUE;
      }
    }
  }
}

Lng32 Statement::getConsumerQueryLen(ULng32 index)
{
  // First build the consumer query template
  buildConsumerQueryTemplate();
  
  // Minimum length will include a null terminator so initialize the
  // result to 1
  Lng32 result = 1;

  const char *tmpl = extractConsumerQueryTemplate_;
  const char *phandle = statementGlobals_->getExtractEspPhandleText(index);
  const char *key = statementGlobals_->getExtractSecurityKey();

  if (phandle == NULL)
    phandle = "";
  if (key == NULL)
    key = "";

  Lng32 reqdLen = str_len(tmpl)
    + str_len(phandle)
    + str_len(key)
    - 4  // to account for the "%s" format specifiers
    + 1; // to account for the null terminator
  result = reqdLen;

  return result;
}

void Statement::getConsumerQuery(ULng32 index, char *buf, Lng32 buflen)
{
  if (buflen < 1)
    return;

  // First build the consumer query template
  buildConsumerQueryTemplate();

  buf[0] = 0;

  const char *tmpl = extractConsumerQueryTemplate_;
  const char *phandle = statementGlobals_->getExtractEspPhandleText(index);
  const char *key = statementGlobals_->getExtractSecurityKey();

  if (phandle == NULL)
    phandle = "";
  if (key == NULL)
    key = "";

  Lng32 reqdLen = str_len(tmpl)
    + str_len(phandle)
    + str_len(key)
    - 4  // to account for the "%s" format specifiers
    + 1; // to account for the null terminator
  ex_assert(buflen >= reqdLen, "Consumer query buffer too small");
  str_sprintf(buf, tmpl, phandle, key);
}

Lng32 Statement::getConsumerCpu(ULng32 index)
{
  Lng32 result = -1;
  short cpu = statementGlobals_->getExtractEspCpu(index);
  Lng32 nodeNumber = statementGlobals_->getExtractEspNodeNumber(index);
  if (cpu >= 0 && nodeNumber >= 0)
  {
    result = (nodeNumber << 8);
    result += (cpu & 0x00ff);
  }
  return result;
}

         
Lng32 Statement::setParentQid(char *queryId)
{
  Lng32 len = 0;
  if (queryId != NULL)
  {
    len = str_len(queryId);
    if (len < ComSqlId::MIN_QUERY_ID_LEN)
      return -CLI_INVALID_ATTR_VALUE;
    if (len < 4 || (len > 4 && str_cmp((const char *)queryId, COM_SESSION_ID_PREFIX, 4) != 0))
      return -CLI_INVALID_ATTR_VALUE;
  }
  if (parentQid_)
    NADELETEBASIC(parentQid_, &heap_);
  if (queryId != NULL)
  {
    parentQid_ = new(&heap_) char[len+1];
    str_cpy_all(parentQid_, queryId, len);
    parentQid_[len] = '\0';
  }
  else
  {
    parentQid_ = NULL;
    len = 0;
  }
  if (stmtStats_ && stmtStats_->getMasterStats() != NULL)
    stmtStats_->getMasterStats()->setParentQid(parentQid_, len);
  return 0;
}

void Statement::setParentQidSystem(char *parentQidSystem)
{
  Lng32 len = 0;
  if (parentQidSystem != NULL)
  {
    len = str_len(parentQidSystem);
    str_cpy_all(parentQidSystem_, parentQidSystem, len);
    parentQidSystem_[len] = '\0';
  }
  else
    parentQidSystem_[0] = '\0'; 
  if (stmtStats_ && stmtStats_->getMasterStats() != NULL)
    stmtStats_->getMasterStats()->setParentQidSystem(parentQidSystem_, len);
}

char *Statement::getParentQid()
{
  char *parentQid;
  if (parentQid_ != NULL)
    parentQid = parentQid_;
  else
  {
    if (context_->getSessionDefaults())
      parentQid = context_->getSessionDefaults()->getParentQid();
    else
      parentQid = NULL;
  }
  return parentQid;
}

char *Statement::getParentQidSystem()
{
  char *parentQidSystem;
  if (parentQidSystem_[0] != '\0')
    parentQidSystem = parentQidSystem_;
  else
  {
    if (context_->getSessionDefaults())
      parentQidSystem = context_->getSessionDefaults()->getParentQidSystem();
    else
      parentQidSystem = NULL;
  }
  return parentQidSystem;
}

Int64 Statement::getExeStartTime()
{
  ExStatisticsArea *statsArea;
  ExMasterStats *masterStats;
  statsArea = getStatsArea();
  
  if (statsArea != NULL && (masterStats = statsArea->getMasterStats()) != NULL)
      return masterStats->getExeStartTime();
  else
      return -1;
}

void Statement::setExeStartTime(Int64 exeStartTime)
{
   if (exeStartTime != -1)
      aqrInitialExeStartTime_ = exeStartTime;
} 

Lng32 Statement::initStrTarget(SQLDESC_ID * sql_source,
                               ContextCli &currContext,
                               ComDiagsArea &diags,
                               StrTarget &strTarget)
{
  if (! sql_source)
    return 0;

  if (sql_source->name_mode == string_data)
    {
      CharInfo::CharSet externalCharset =
              CharInfo::getCharSetEnum(sql_source->charset);
      CharInfo::CharSet internalCharset = CharInfo::UTF8;
      ComDiagsArea *diagsPtr = &diags;


      strTarget.init((char*)sql_source->identifier, 
		     sql_source->identifier_len, 
                     CharInfo::getFSTypeFixedChar(externalCharset),
                     externalCharset,
                     internalCharset,
                     currContext.exCollHeap(),
                     diagsPtr);
    }
  else
    {
      Descriptor * desc = currContext.getDescriptor(sql_source);
      
      /* descriptor must exist */
      if (!desc)
	{
	  diags << DgSqlCode(-CLI_DESC_NOT_EXISTS);
	  return -CLI_DESC_NOT_EXISTS;
	}
      
      strTarget.init(desc, 1);
    }
  
  if (!strTarget.getStr())
    {
      diags << DgSqlCode(-CLI_STMT_NOT_PREPARED);
      return -CLI_STMT_NOT_PREPARED;
    }
  copyInSourceStr(strTarget.getStr(), (octetLen(strTarget.getStr(), strTarget.getIntCharSet())), strTarget.getIntCharSet());
  return 0;
}

NABoolean Statement::updateChildQid()
{
  NABoolean parentIsCanceled = FALSE;
  StatsGlobals *statsGlobals = cliGlobals_->getStatsGlobals();
  
  if (statsGlobals != NULL && uniqueStmtId_ != NULL && parentQid_ != NULL &&
      getStatsArea() != NULL && stmtStats_ != NULL && stmtStats_->updateChildQid())
  {
    int error = statsGlobals->getStatsSemaphore(cliGlobals_->getSemId(),
          cliGlobals_->myPin());
    StmtStats *ss = statsGlobals->getMasterStmtStats(parentQid_, str_len(parentQid_), RtsQueryId::ANY_QUERY_);
    if (ss != NULL)
    {
      ExMasterStats *parentMasterStats = ss->getMasterStats();
      if (parentMasterStats)
      {
        parentMasterStats->setChildQid(uniqueStmtId_, uniqueStmtIdLen_);
        if (parentMasterStats->getCanceledTime() != -1)
          parentIsCanceled = TRUE;
      }
    }
    statsGlobals->releaseStatsSemaphore(cliGlobals_->getSemId(),cliGlobals_->myPin());
  }
  return parentIsCanceled;
}
/*
// make the context's stats point to this statement's stats. 
      // Context points to the most recent executed statement's
      // stat area so later a getStatistics CLI call without a
      // statement ID can return it.
*/
void Statement::updateStatsAreaInContext()
{
  ExStatisticsArea *statsArea;
  ComTdb::CollectStatsType statsType;
  ExMasterStats *masterStats;
  statsArea = getStatsArea();
  if (stmtStats_ != NULL && 
      statsArea && 
      ((statsType = statsArea->getCollectStatsType()) != (Int32)SQLCLIDEV_NO_STATS) && 
      stmt_type == Statement::DYNAMIC_STMT)
  {
    if (statsType == (Int32)SQLCLIDEV_ALL_STATS)
    {
      if ((masterStats = statsArea->getMasterStats()) != NULL)
      {
        NADELETE(masterStats, ExMasterStats, masterStats->getHeap());
        statsArea->setMasterStats(NULL);
      }
    }
    if (statsArea->getMasterStats() == NULL)
	{
	  ExMasterStats * ems = new(context_->exHeap())
	    ExMasterStats(context_->exHeap());
	  if (stmtStats_->getMasterStats())
	    ems->copyContents(stmtStats_->getMasterStats());
          ems->setCollectStatsType(getRootTdb()->getCollectStatsType());
	  statsArea->setMasterStats(ems);
	}
    context_->setStatsArea(getStatsArea(), FALSE, (cliGlobals_->getStatsGlobals() != NULL));
  }
}

Lng32 Statement::setChildQueryInfo(ComDiagsArea *diagsArea, char * uniqueQueryId,
        Lng32 uniqueQueryIdLen,
        SQL_QUERY_COST_INFO *query_cost_info,
        SQL_QUERY_COMPILER_STATS_INFO *comp_stats_info)
{
  NAHeap *heap = stmtHeap();
  
  if (uniqueQueryId == NULL || query_cost_info == NULL || comp_stats_info == NULL)
  {
    if (diagsArea != NULL)
     *diagsArea << DgSqlCode(-CLI_INVALID_ATTR_VALUE);
    return ERROR;
  }
  if (childQueryId_ != NULL)
  {
    NADELETEBASIC(childQueryId_, heap);
  }
  childQueryId_ = new (heap) char[uniqueQueryIdLen+1];
  str_cpy_all(childQueryId_, uniqueQueryId, uniqueQueryIdLen);
  childQueryId_[uniqueQueryIdLen] = '\0';
  childQueryIdLen_ = uniqueQueryIdLen;
  if (childQueryCostInfo_ != NULL)
  {
    NADELETEBASIC(childQueryCostInfo_, heap);
  }
  childQueryCostInfo_ = new (heap) SQL_QUERY_COST_INFO;
  str_cpy_all((char *)childQueryCostInfo_, (const char *)query_cost_info, sizeof(SQL_QUERY_COST_INFO));
  if (childQueryCompStatsInfo_ != NULL)
  {
    NADELETEBASIC(childQueryCompStatsInfo_, heap);
  }
  childQueryCompStatsInfo_ = new (heap) SQL_QUERY_COMPILER_STATS_INFO;
  str_cpy_all((char *)childQueryCompStatsInfo_, (const char *)comp_stats_info, sizeof(SQL_QUERY_COMPILER_STATS_INFO));
  return SUCCESS;
}

Lng32 Statement::getChildQueryInfo(ComDiagsArea &diagsArea, char * uniqueQueryId,
     Lng32 uniqueQueryIdMaxLen,
     Lng32 * uniqueQueryIdLen,
     SQL_QUERY_COST_INFO *query_cost_info,
     SQL_QUERY_COMPILER_STATS_INFO *comp_stats_info)
{
  if (childQueryId_ == NULL)
  {
     diagsArea << DgSqlCode(-EXE_RTS_QID_NOT_FOUND);
     return ERROR;
  }
  if (childQueryIdLen_ > uniqueQueryIdMaxLen || uniqueQueryId == NULL ||
      uniqueQueryIdLen == NULL)
  {
     diagsArea << DgSqlCode(-CLI_BUFFER_TOO_SMALL);
     return ERROR;
  }
  str_cpy_all(uniqueQueryId, (const char *)childQueryId_, childQueryIdLen_);
  *uniqueQueryIdLen = childQueryIdLen_;
  if (childQueryCostInfo_ != NULL && query_cost_info != NULL)
    str_cpy_all((char *)query_cost_info, (const char *)childQueryCostInfo_, 
          sizeof(SQL_QUERY_COST_INFO));
  if (childQueryCompStatsInfo_ != NULL && comp_stats_info != NULL)
    str_cpy_all((char *)childQueryCompStatsInfo_, (const char *)comp_stats_info,
            sizeof(SQL_QUERY_COMPILER_STATS_INFO));
  return SUCCESS;
}

