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
 * File:         CmpConnection.C
 * Description:  The Ipc classes for arkcmp communicating with executor.
 *               The implementaion of ExCmpMessage, CmpIpcEnvironment and
 *               CmpGuaControlConnection classes           
 *
 * Created:      09/05/96
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

#define   SQLPARSERGLOBALS_NADEFAULTS   // should precede all other #include's
#define   SQLPARSERGLOBALS_FLAGS
#define   SQLPARSERGLOBALS_NADEFAULTS_SET
#include "SqlParserGlobals.h"
#include "SqlParserGlobalsCmn.h"

#include <iostream>
#include "Ipc.h"
#include "CmpCommon.h"
#include "CmpConnection.h"
#include "CmpMessage.h"
#include "CmpStatement.h"
#include "CmpErrLog.h"
#include "NewDel.h"
#include "opt.h"
#include "NAExit.h"
#include "QCache.h"
#include "CompException.h"
#include "CostMethod.h"
#include "NAExecTrans.h"

extern THREAD_P jmp_buf ExportJmpBuf;
extern THREAD_P jmp_buf CmpInternalErrorJmpBuf;

// This is a global variable used per process to identify whether this 
// arkcmp process is spawned for internal stored procedure execution.
// This value is set before the CmpContext is instantiated, so it can't
// be put into CmpContext class.
THREAD_P NABoolean CmpMainISPConnection = FALSE;

ostream &operator<<(ostream &dest, const ComDiagsArea& da);

void TODOEMSABORT( const char* msg )
{
  // TODOEMSABORT is to be replaced later when arkcmp provides EMS support.
  // It is only used in main program or CmpConnection when the IPC
  // mechanism is not setup yet. At this period, if there is need to
  // abort the program, some error reporting will be done.
  // Once IPC is setup, CMPABORT should be used if there is need to
  // abort the program. The errors will be sent back to executor and
  // arkcmp exits gracefully.

  // This should be implemented in the future to report error log to
  // EMS, since the IPC mechanism is broken in this kind of error,
  // there is no way for arkcmp to return error information back to
  // executor through ComDiags. Currently, arkcmp just exit.
  cerr << msg << endl;
}

// This is a last resort, stuff will still scroll past if
// an error occurs during UNATTENDED compilation,
// when no alert user to right there eyeballing the screen
// This function is only necessary if arkcmp is executing
// in its own window, i.e. not from a cmd shell.
void ArkcmpDelayExit()
{
    #define DEFAULT_DELAY_SECS 10
    ULng32 secs = DEFAULT_DELAY_SECS;
    char *ev = getenv("SQL_ERROR_SLEEP");   // scripts: set to 0 (off), 60, etc
    if (ev)
      {
        char *tmp;
	secs = strtoul(ev, &tmp, 10/*decimal*/);
	if (tmp == ev) secs = DEFAULT_DELAY_SECS;	// bad env var value
      }
    while (secs--) Sleep(1000);
}

// myNAExit(status) does mxcmp finalization before calling NAExit(status)
static void myNAExit(Int32 status)
{
  CURRENTQCACHE->finalize("Dynamic ");
  NAExit(status);
}

void ArkcmpErrorMessageBox(const char *msg,
			   ArkcmpErrorSeverity sev,
			   NABoolean doExit,
			   NABoolean doDelay,
			   NABoolean doCerr)
{
  if (doCerr) cerr << msg << endl << flush;

    // This functionality is only necessary if arkcmp is executing
    // in its own window, i.e. not from a cmd shell.
		//	if (##running in our own window, not from a cmd shell##)
		//	{

    static Int32 recursing = 0;
    // We will popup a msgbox if not out of memory
    NABoolean doMsgbox = (sev != NOMEM_SEV && !recursing);
    recursing++;

    if (doMsgbox)
      {
      // Scripts (e.g. runregr, sqlit) set this env var to 0 (meaning off),
      // which is the default!
      char *ev = getenv("SQL_ERROR_MSGBOX");
      if (!ev || ev[0] == '0')
        doMsgbox = FALSE;
      }

    if (doMsgbox)
      {
	UINT mbflags = MB_ICONERROR;
	if (sev == WARNING_SEV)   mbflags = MB_ICONWARNING;
	else if (sev == INFO_SEV) mbflags = MB_ICONINFORMATION;
	mbflags |=  MB_OK |  MB_SETFOREGROUND | MB_TOPMOST;
	MessageBox(NULL, msg, "tdm_arkcmp", mbflags);
      }

    else if (doDelay && recursing < 3)
      ArkcmpDelayExit();

    recursing--;
    		//	} // ##running in own window, not invoked from shell

  if (doExit)
    {
      TODOEMSABORT(msg);
      myNAExit(1);
    }
}


// stmtNewHandler_CharSave and stmtNewHandler are used in error
// handling when running out of virtual memory happens after the
// request is received from executor.

// Save 5K bytes of memory for the error handling when running out
// of virtual memory, so the error messages can be sent back to executor.
static char* stmtNewHandler_CharSave = new char[5120];


static void stmtNewHandler()
{
  delete[] stmtNewHandler_CharSave;
  // There are some components are catching all exceptions, e.g.
  // optimizer is catching all exceptions including CMPABORT.
  // To avoid deleting the stmtNewHandler_CharSave more than once,
  // it is set to null after deletion.
  stmtNewHandler_CharSave=0;

  // Log this error to the file indicated by CMP_ERR_LOG_FILE CQD.
  CmpErrLog("Memory allocation failure");

  if ( CmpCommon::context() )
  {
    *(CmpCommon::diags() ) << DgSqlCode(arkcmpErrorOutOfMemory);
    CMPABORT;
  }
  else
  {
    // Must be in the ConnectionType request, the cmpContext is not setup yet;
    TODOEMSABORT("Run out of virtual memory");
    myNAExit(1);
  }
  //return 0;
}
// -----------------------------------------------------------------------
// Methods for ExCmpMessage
// -----------------------------------------------------------------------

ExCmpMessage::ExCmpMessage(IpcEnvironment* ipcEnv) :
     IpcMessageStream(ipcEnv, CmpMessageObj::EXE_CMP_MESSAGE, 
		      EXECMPIPCVERSION, 0, TRUE)
{
  endOfConnection_ = FALSE;
  cmpContext_ = 0;
}

ExCmpMessage::~ExCmpMessage()
{
}

void ExCmpMessage::setCmpContext(CmpContext* cmpContext)
{
  cmpContext_ = cmpContext;
}


inline static void receiveAndSetUp(ExCmpMessage *from, CmpMessageObj &to)
{
  *from >> to;
  Set_SqlParser_Flags(to.getFlags() LAND IPC_COPIABLE_MASK);
}

inline static void clearAndReset(ExCmpMessage *from)
{
  from->clearAllObjects();
  Set_SqlParser_Flags(0);

  // set DEFAULT_CHARSET to the original CHARSET. It may have been
  // changed if an internal query with ISO_MAPPING was sent.
  if (SqlParser_NADefaults_Glob)
    SetSqlParser_DEFAULT_CHARSET(SqlParser_ORIG_DEFAULT_CHARSET);
}

static void sendErrorOnLongJump(CmpStatement* cmpStatement, ExCmpMessage* msg, Int32 errcode, CollHeap* heap)
{
  if (cmpStatement)
  {
    cmpStatement->exceptionRaised();
    if (errcode == MEMALLOC_FAILURE)
    {
      // Log this error to the file indicated by CMP_ERR_LOG_FILE CQD.
      CmpErrLog("Memory allocation failure");
      *cmpStatement->diags() << DgSqlCode(arkcmpErrorOutOfMemory);
    }
    else
    {
      *cmpStatement->diags() << DgSqlCode(arkcmpErrorAssert);
      *cmpStatement->diags() << DgString0("from longjmp(NAAssert)")
                             << DgString1(__FILE__) << DgInt0(__LINE__);
    }
    *msg << *cmpStatement->diags();
    if (cmpStatement->reply())
      *msg << *cmpStatement->reply();
  }
  else
  {
    ComDiagsArea *diags = ComDiagsArea::allocate(heap);
    *diags << DgSqlCode(arkcmpErrorAssert)
	  << DgString0("from longjmp(NAAssert)")
	  << DgString1(__FILE__) << DgInt0(__LINE__);
    *msg << *diags;
    diags->decrRefCount();
  }

  CmpMessageLast last_message;
  *msg << last_message;
  msg->send();
}

extern THREAD_P jmp_buf CmpInternalErrorJmpBuf;

void ExCmpMessage::actOnReceive(IpcConnection* )
{
 
  if (getState()==ERROR_STATE) 
    {
      ArkcmpFatalError(ARKCMP_ERROR_PREFIX
      		"ExCmpMessage::actOnReceive, error from receiving message.");
    }

  CmpStatement* volatile cmpStatement=0;
  CollHeap * volatile ipcHeap = environment_->getHeap();
  jmp_buf oldBuf;
  memcpy (&oldBuf, ExportJmpBufPtr, sizeof(jmp_buf));

  
  Int32 jRc = setjmp(ExportJmpBuf);
  if (jRc)
    {
      // The jmp_buf has to be set back to the old one here,
      // so if there is an NAAssert in the IPC routines, it will
      // jump back to the one in main program to avoid infinite loop.
      memcpy( ExportJmpBufPtr, &oldBuf, sizeof(jmp_buf));
      clearAndReset(this);
      CostMethod::cleanUpAllCostMethods();
      sendErrorOnLongJump(cmpStatement, this, jRc, ipcHeap);
      if ( cmpStatement && cmpStatement->readyToDie() )
        delete cmpStatement;
      return;
    }

  ExportJmpBufPtr = &ExportJmpBuf;
  
  jmp_buf oldBuf2;
  memcpy (&oldBuf2, &CmpInternalErrorJmpBuf, sizeof(jmp_buf));
  Int32 jRc2 = setjmp(CmpInternalErrorJmpBuf);
  if(jRc2)
  {
    clearAndReset(this);
    if (stmtNewHandler_CharSave)
    {
      delete stmtNewHandler_CharSave;
      stmtNewHandler_CharSave = 0;
    }
    CmpCommon::context()->freeReservedMemory();
    CostMethod::cleanUpAllCostMethods();
    sendErrorOnLongJump(cmpStatement, this, jRc2, ipcHeap);
    if (jRc2 == MEMALLOC_FAILURE)
      ArkcmpFatalError(ARKCMP_ERROR_PREFIX "Out of virtual memory.", NOMEM_SEV);
    else
      ArkcmpFatalError(ARKCMP_ERROR_PREFIX "Fatal error from longjmp");
  }

  CmpInternalErrorJmpBufPtr = &CmpInternalErrorJmpBuf;
  
#if (defined(_DEBUG) || defined(NSK_MEMDEBUG))
  NewHandler_NSK newHandler(stmtNewHandler);
#endif
  
  try 
    {
      IpcMessageObjType typ;
      // For the requests with the following message type the parent qid may not be passed
      // CmpMessageDescribe
      // CmpMessageUpdateHist
      // CmpMessageSetTrans
      // CmpMessageEndSession
      // Reset the parent qid and the requests that has parent qid will set it later
      if (CmpCommon::context() && CmpCommon::context()->sqlSession())
        CmpCommon::context()->sqlSession()->setParentQid(NULL);

      switch (typ=getNextObjType()) 
      {
      case CmpMessageObj::SQLTEXT_RECOMPILE :
      case CmpMessageObj::SQLTEXT_COMPILE :
        {
          // The number of NAMemory objects that reside in SYSTEM_MEMORY
          // is currently restricted to one at a time--see explanation in
          // NAMemory.h.
          cmpStatement = new CTXTHEAP CmpStatement(cmpContext_);
          CmpMessageSQLText sqltext(NULL,0,CTXTHEAP,SQLCHARSETCODE_UNKNOWN,
                                    (CmpMessageObj::MessageTypeEnum)typ);
          receiveAndSetUp(this, sqltext);
          cmpStatement->process(sqltext);
          break;
        }
				  	    
      case CmpMessageObj::SQLTEXT_STATIC_RECOMPILE :
      case CmpMessageObj::SQLTEXT_STATIC_COMPILE :
        {
          cmpStatement = new CTXTHEAP CmpStatement(cmpContext_);
          CmpMessageCompileStmt compilestmt(NULL,
                                            0,
                                            (CmpMessageObj::MessageTypeEnum)typ,
                                            CTXTHEAP) ;
	  receiveAndSetUp(this, compilestmt);
          cmpStatement->process(compilestmt);
          break;
        }
      break;
	      
      case (CmpMessageObj::EXIT_CONNECTION) :
        setEnd();
        break;
	
      case (CmpMessageObj::ENVS_REFRESH) :
        {
          cmpStatement = new CTXTHEAP CmpStatement(cmpContext_);
      
          CmpMessageEnvs env_message(CmpMessageEnvs::NONE, NULL, TRUE, CTXTHEAP);
	  receiveAndSetUp(this, env_message);
          cmpStatement->process(env_message);
          CmpCommon::context()->setSecondaryMxcmp();
          break;
        } // end of case (CmpMessageObj::ENVS_REFRESH) 

      case (CmpMessageObj::DDL) :
        {
          cmpStatement = new CTXTHEAP CmpStatement(cmpContext_);
          CmpMessageDDL statement(NULL, 0, CTXTHEAP);
	  receiveAndSetUp(this, statement);
          cmpStatement->process(statement);
          break;
        } // end of case (CmpMessageObj::DDL)

      case (CmpMessageObj::DESCRIBE) :
        {
          cmpStatement = new CTXTHEAP CmpStatement(cmpContext_);
          CmpMessageDescribe statement(NULL, 0, CTXTHEAP);
	  receiveAndSetUp(this, statement);
          cmpStatement->process(statement);
          break;
        } // end of case (CmpMessageObj::DESCRIBE)   

      case (CmpMessageObj::UPDATE_HIST_STAT) :
        {
          cmpStatement = new CTXTHEAP CmpStatement(cmpContext_);
          CmpMessageUpdateHist statement(NULL, 0, CTXTHEAP);
	  receiveAndSetUp(this, statement);
          cmpStatement->process(statement);
          break;
        } // end of case (CmpMessageObj::UPDATE_HIST_STAT)   

      case (CmpMessageObj::SET_TRANS) :
        {
          cmpStatement = new CTXTHEAP CmpStatement(cmpContext_);
          CmpMessageSetTrans statement(NULL,0,CTXTHEAP);
	  receiveAndSetUp(this, statement);
          cmpStatement->process(statement);
          break;
        } // end of case (CmpMessageObj::SET_TRANS)

        case (CmpMessageObj::DATABASE_USER):
        {
          cmpStatement = new CTXTHEAP CmpStatement(cmpContext_);
          CmpMessageDatabaseUser statement(NULL, 0, CTXTHEAP);
	  receiveAndSetUp(this, statement);
          cmpStatement->process(statement);
          break;
        } // end of case (CmpMessageObj::DATABASE_USER)

      case (CmpMessageObj::DDL_WITH_STATUS) :
        {
          cmpStatement = new CTXTHEAP CmpStatement(cmpContext_);
          CmpMessageDDLwithStatus statement(NULL, 0, CTXTHEAP);
	  receiveAndSetUp(this, statement);
          cmpStatement->process(statement);
          break;
        } // end of case (CmpMessageObj::DDL_WITH_STATUS)

      case (CmpMessageObj::END_SESSION) :
        {
          cmpStatement = new CTXTHEAP CmpStatement(cmpContext_);
          CmpMessageEndSession statement(NULL,0,CTXTHEAP);
	  receiveAndSetUp(this, statement);
          cmpStatement->process(statement);
          break;
        } // end of case (CmpMessageObj::END_SESSION)

      case (CmpMessageObj::INTERNALSP_REQUEST) :
        {
          CmpStatementISP* ispStatement = new CTXTHEAP CmpStatementISP(cmpContext_);
          cmpStatement = ispStatement;
          // The request is dynamically allocated here instead of using 
	  // stack variable, because the contents of this request will be 
	  // referenced later in INTERNALSP_GETNEXT request for performance
	  // ( so we can avoid some data movement ). CmpStatementISP
          // owns the CmpISPDateObject which in turns own the 
	  // CmpMessageISPRequest. it should be
          // deleted in the destructor of CmpMessageISPRequest. 
          CmpMessageISPRequest* ispRequest = new CmpMessageISPRequest();
	  receiveAndSetUp(this, *ispRequest);
          ispStatement->process(*ispRequest);
          break;  
        }// end of case(CmpMessageObj::INTERNAL_REQUEST)

      case (CmpMessageObj::INTERNALSP_GETNEXT) :
        {
          CmpMessageISPGetNext ispGetNext;
	  receiveAndSetUp(this, ispGetNext);
          CmpStatementISP* ispStatement = 
            getISPStatement(ispGetNext.ispRequest());
          if (!ispStatement)
          {
            // There must be a previous ispStatement, otherwise it is an 
            // internal error. Instantiate a dummy CmpStatement here, just
            // to place the error information.
            cmpStatement = new CTXTHEAP CmpStatement(cmpContext_);
            CMPASSERT(FALSE);
          }
          cmpStatement = ispStatement;
          cmpContext_->setCurrentStatement(cmpStatement);
          ispStatement->process(ispGetNext);
          break;
        }
      case (CmpMessageObj::CONNECTION_TYPE) :
        {
          CmpMessageConnectionType connectionType;
	  receiveAndSetUp(this, connectionType);
          CmpMainISPConnection = 
            (connectionType.connectionType()==CmpMessageConnectionType::ISP);
          break;
        }
      default:
        break;      
      } // end of switch (message1.getNextObjType())
    clearAndReset(this);
  }
  catch(CmpInternalException& exInternal)
    {
      clearAndReset(this);
      if (cmpStatement)
      {
	cmpStatement->error(arkcmpErrorAssert, exInternal.getMsg());
        cmpStatement->exceptionRaised();
      }
      else
      {
	ComDiagsArea *diags = ComDiagsArea::allocate(ipcHeap);
        *diags << DgSqlCode(arkcmpErrorAssert)
          << DgInt0(0) << DgString0("from CMPASSERT")
          << DgString1("CmpConnection::actOnReceive,EH_INTERNAL_EXCEPTION");
        *this << *diags;
	diags->decrRefCount();
      }
    }
  catch(EHBreakException & x)
    {
      clearAndReset(this);
      if ( !CmpCommon::diags()->getNumber() )
        *CmpCommon::diags() << DgSqlCode(arkcmpErrorAssert) << DgString0("EHBreakException") << DgString1(x.getFileName()) << DgInt0(x.getLineNum());
      cerr  << *CmpCommon::diags(); 
      *this << *CmpCommon::diags();
      CmpMessageLast last_message;
      *this << last_message;
      send();
      ArkcmpFatalError(ARKCMP_ERROR_PREFIX "actOnReceive exception.");
    }
  catch(...)
    {
      clearAndReset(this);
      if (cmpStatement)
      {
	cmpStatement->error(arkcmpErrorNoDiags, "Unknown Exception");
        cmpStatement->exceptionRaised();
      }
      else
      {
	ComDiagsArea *diags = ComDiagsArea::allocate(ipcHeap);
        *diags << DgSqlCode(arkcmpErrorAssert)
	      << DgString0("from longjmp(CmpConnection::actOnReceive)")
	      << DgString1(__FILE__) << DgInt0(__LINE__);
        *this << *diags;
	diags->decrRefCount();
      }
    }

  // reset the Buf back to the original one.
  memcpy (ExportJmpBufPtr, &oldBuf, sizeof(jmp_buf));
  memcpy (&CmpInternalErrorJmpBuf, &oldBuf, sizeof(jmp_buf));
  clearAndReset(this);
  
  if (cmpStatement)
  {
    ComDiagsArea *diags = cmpStatement->diags();
    if (diags->getNumber() > 0)
       *this << *cmpStatement->diags();
    if (cmpStatement->reply()) 
      *this << *cmpStatement->reply();
  }

  CmpMessageLast last_message;
  *this << last_message;

  send();

  if ( cmpStatement && cmpStatement->readyToDie() )
    delete cmpStatement;

  if ( !stmtNewHandler_CharSave )
    // Some components (e.g. optimizer) might catch all exceptions and
    // handle errors differently. In the case of running out of virtual
    // memory error, the program should exit right after sending the reply.
    myNAExit(-1);
}

void  ExCmpMessage::actOnSend(IpcConnection* )

{
  if (getState() == ERROR_STATE)
    {
      ArkcmpFatalError(ARKCMP_ERROR_PREFIX 
      		"Error from sending reply back to executor.");
    }
  
}

CmpStatementISP* ExCmpMessage::getISPStatement(Int64 id)
{
  NAList<CmpStatement*> statements = cmpContext_->statements();
  CmpStatementISP* ispStatement;
  for ( CollIndex i = 0; i < statements.entries(); i++)
    if ( statements[i] && ( ispStatement = (statements[i])->ISPStatement() ) 
      && ispStatement->ISPReqId() == id )
      return ispStatement;
  return 0;
}
  
// -----------------------------------------------------------------------
// Methods for CmpIpcEnvironment
// -----------------------------------------------------------------------

void CmpIpcEnvironment::initControl(IpcServerAllocationMethod allocMethod,
				    Int32 sockArg,
				    Int32 portArg) 
{
  switch (allocMethod) 
    {      
      case IPC_LAUNCH_GUARDIAN_PROCESS:
      case IPC_SPAWN_OSS_PROCESS:
      {	
	GuaReceiveControlConnection *cc = 
	  new(this) CmpGuaControlConnection(this);
	setControlConnection(cc);
	cc->waitForMaster();
	break;
      }
      case IPC_INETD:
      case IPC_POSIX_FORK_EXEC:
        setControlConnection(new (this) SockControlConnection(this));
	break;
	// copied from ex_esp_main.C  ( 03/18/97 )
      case IPC_LAUNCH_NT_PROCESS:
	break;
      default :
        {
          ArkcmpFatalError(ARKCMP_ERROR_PREFIX "Invalid connection method.");
        }	
      break;
    }

}

// -----------------------------------------------------------------------
// Implementation of CmpGuaControlConnection, handles system messages
// -----------------------------------------------------------------------


CmpGuaControlConnection::CmpGuaControlConnection(IpcEnvironment* env, 
						 short receiveDepth) :
			 GuaReceiveControlConnection(env, receiveDepth) 
{
}

CmpGuaControlConnection::~CmpGuaControlConnection()
{
}


