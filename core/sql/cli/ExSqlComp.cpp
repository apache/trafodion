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
 * File:         ExSqlComp.C
 * Description:  This file contains the implementation of ExSqlComp class for
 *               executor to create arkcmp process and send requests 
 *               for compilation and SQLCAT related tasks.
 *
 * Created:      06/21/96
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

#include <ctype.h>
#include "Platform.h"
#include "cextdecs/cextdecs.h"

#include "ex_stdh.h"			// TEMP, for ISP testing

#include "Platform.h"


#include "ExSqlComp.h"
#include "cli_stdh.h"
#include "CmpErrors.h"
#include "CmpMessage.h"
#include "ComDiags.h"
#include "ShowSchema.h"			// GetControlDefaults class
#include "StmtCompilationMode.h"

#include "ComTdb.h"
#include "ExControlArea.h"
#include "ComTransInfo.h"
#include "ex_tcb.h"
#include "ex_stored_proc.h"
#include "ex_transaction.h"
#include "sql_id.h"
#include "ComRtUtils.h"

#include "Ipc.h"
#include "PortProcessCalls.cpp"

#include "seabed/fs.h"
#include "seabed/ms.h"

// -----------------------------------------------------------------------
// Diagnostics error listings for ExSqlComp
// -----------------------------------------------------------------------

// -----------------------------------------------------------------------
// Internal helper routines for ExSqlComp
// -----------------------------------------------------------------------

inline 
NABoolean ExSqlComp::error(Lng32 no)
{
  *diagArea_ << DgSqlCode(no);
  return TRUE;
}

void ExSqlComp::clearDiags()
{
  if (diagArea_)
    diagArea_->clear();
  else
    diagArea_ = ComDiagsArea::allocate(h_);
}

inline void ExSqlComp::initRequests(Requests& req)
{
  req.message_ = 0;
  req.resendCount_ = 0;
  req.waited_ = TRUE;
  req.ioStatus_ = INIT;
}

ExSqlComp::ReturnStatus ExSqlComp::changePriority(IpcPriority priority,
						  NABoolean isDelta)
{
  ReturnStatus ret = SUCCESS;
  if (! server_)
    return ret;

  short rc = server_->castToIpcGuardianServer()->changePriority(priority, isDelta);
  if (rc != 0)
    {
      ret = ERROR;
    }

  return ret;
}

ExSqlComp::ReturnStatus ExSqlComp::createServer() 
{
  ReturnStatus ret = SUCCESS;

  if (sc_) {
    delete sc_;
    sc_ = NULL;
  };

  if (!(sc_=new(h_) IpcServerClass(env_, IPC_SQLCOMP_SERVER, allocMethod_,
                                   compilerVersion_,nodeName_)))
    {
      //
      *diagArea_ << DgSqlCode(- CLI_OUT_OF_MEMORY)
                 << DgString0("IpcServerClass");
      ret = ERROR;
      
    }
  else
    {
      IpcPriority priority = IPC_PRIORITY_DONT_CARE;
      
      if (cliGlobals_->currContext()->getSessionDefaults()->getMxcmpPriority() > 0)
	priority = cliGlobals_->currContext()->getSessionDefaults()->
	  getMxcmpPriority();
      else if (cliGlobals_->currContext()->getSessionDefaults()->
	       getMxcmpPriorityDelta() != 0)
	priority = 
	  //	  env_->getMyProcessPriority() + 
	  cliGlobals_->myPriority() + 
	  cliGlobals_->currContext()->getSessionDefaults()->
	  getMxcmpPriorityDelta();
      if ((priority > 200) ||
	  (priority < 1))
	
	priority = IPC_PRIORITY_DONT_CARE;
	
      ComDiagsArea* diags = 0;
      if ( !( server_ = sc_->allocateServerProcess(&diags, h_,nodeName_,
						   IPC_CPU_DONT_CARE,
						   priority,
						   1, TRUE, TRUE, 2, NULL, NULL, FALSE, NULL
						   ) ) )
	ret = ERROR;
      if (diags) 
	{
	  diagArea_->mergeAfter(*diags);
	  diags->deAllocate();
	  ret = ERROR;
	}
      //Server process allocations may have changed the define context
      cliGlobals_->currContext()->checkAndSetCurrentDefineContext();
    }
  //  
  if (ret == ERROR)
  {
	if ((!diagArea_->contains(-2013)) && (!diagArea_->contains(-2012))) // avoid generating redundant error
	  error(arkcmpErrorServer);
	if (getenv("DEBUG_SERVER"))
	  MessageBox(NULL, "ExSqlComp:createServer", "error ", MB_OK|MB_ICONINFORMATION);
  }
  

  return ret;
}

ExSqlComp::ReturnStatus ExSqlComp::establishConnection()
{
  ReturnStatus ret = SUCCESS;

  if (sqlcompMessage_) {
    delete sqlcompMessage_;
    sqlcompMessage_ = NULL;
  };
    if ( !(sqlcompMessage_ = new(h_) CmpMessageStream (env_, this) ))
    {
      *diagArea_ << DgSqlCode(- CLI_OUT_OF_MEMORY)
                 << DgString0("CmpMessageStreaam");
      ret = ERROR;
    }
  else
  {
    sqlcompMessage_->addRecipient(server_->getControlConnection());
    CmpMessageConnectionType connectionType(connectionType_, h_);
    (*sqlcompMessage_) << connectionType;
    sqlcompMessage_->setWaited(TRUE);  // set to waited as default
    Requests tempRequests = outstandingSendBuffers_;
    outstandingSendBuffers_.message_ = NULL;
    outstandingSendBuffers_.resendCount_ = 99; // do not resend.
    sqlcompMessage_->send(TRUE);  // always waited when establishing connection
    outstandingSendBuffers_ = tempRequests;
    if (badConnection_)
      ret = ERROR;
  }
    //
  if (ret == ERROR)
    error(arkcmpErrorConnection);
  
  return ret;
}

ExSqlComp::ReturnStatus ExSqlComp::startSqlcomp(void)
{ 
  ReturnStatus ret = SUCCESS;

  badConnection_ = FALSE;
  breakReceived_ = FALSE;

  // all the connection and control processing are done in waited mode.
  if ( (ret=createServer()) == ERROR )
    return ret;
  if ( (ret=establishConnection()) == ERROR )
    return ret;

  if ( (ret=resendControls()) == ERROR )
    return ret;

  // on NT, the environment is not shipped to mxcmp when the process
  // is created as an NSK lite process.
  // Send it now.
  if ( (ret=refreshEnvs()) == ERROR )
    return ret;
 
  return ret;
}    
  
ExSqlComp::ReturnStatus ExSqlComp::resendRequest() 
{
  // Do not resend the request if it is an ISP request, 
  // If arkcmp exits in the middle of stored procedure execution for
  // any reason, don't start arkcmp again since the execution state can't
  // be retrieved.
  
  IpcMessageType typ = outstandingSendBuffers_.message_ ?
		       outstandingSendBuffers_.message_->getType() :
		       CmpMessageObj::NULL_REQUEST;

  ExTransaction *ta = cliGlobals_->currContext()->getTransaction();
  // removing this check for now. If the user transaction is still active
  // we do not need to report an error.
  /* long sqlCode = (ta->xnInProgress() && !ta->implicitXn()) ?
     arkcmpErrorUserTxnAndArkcmpGone : 0;*/
  Lng32 sqlCode=0;
  //
  if (
#ifdef _DEBUG
      getenv("ARKCMP_NORESEND_DEBUG")		||
#endif
      sqlCode					||
      outstandingSendBuffers_.resendCount_ >= 1	|| 
      currentISPRequest_			||	// an ISP request
      typ == CmpMessageObj::INTERNALSP_REQUEST	||
      typ == CmpMessageObj::INTERNALSP_GETNEXT)
    {
      error(sqlCode ? sqlCode : arkcmpErrorResend);
      badConnection_ = TRUE;
      initRequests(outstandingSendBuffers_);
      currentISPRequest_ = 0;
      outstandingSendBuffers_.ioStatus_ = FINISHED;
      return ERROR;
    }
  
  // if this transaction was 
  // implicitly started by the executor clean it up now. otherwise
  // this executor will get stuck in a repeated 8841 cycle.
  // If autocommit is on it is safe to continue restarting a new Xn and 
  // continuing.
  //If autocommit is off, cleanup but return an error. 
  if (ta->xnInProgress() && ta->implicitXn() && ta->userEndedExeXn()) 
    {
      
      if (!ta->autoCommit())
	{
	   error(-CLI_USER_ENDED_EXE_XN);
	   badConnection_ = TRUE;
	   initRequests(outstandingSendBuffers_);
	   outstandingSendBuffers_.ioStatus_ = FINISHED;
	   ta->cleanupTransaction();
	   return ERROR;
	  
	}
      else
	ta->cleanupTransaction();
    }

  // Start the process again.

  badConnection_ = FALSE;
  breakReceived_ = FALSE;

  // save request.
  // set outstandingSendBuffers_.message_ to 0 so that its memory won't be
  // deallocated in estableConnection().  
  Requests tempRequests = outstandingSendBuffers_;
  outstandingSendBuffers_.message_ = 0;   

  ReturnStatus ret = SUCCESS;
  outstandingSendBuffers_.resendCount_++;
  ret = createServer();
  if (ret == ERROR) return ret;

  Lng32 r = outstandingSendBuffers_.resendCount_;	// save
  ret = establishConnection();
  if (ret == ERROR) return ret;

  // Setup the arkcmp context again.

  ret = resendControls();

  if (ret == ERROR) return ret;

  ret = refreshEnvs();
  if (ret == ERROR) return ret;

  // Send the message again. Use the same waited mode in the request.

  outstandingSendBuffers_.resendCount_ = r;		// restore

  // restore request.
  outstandingSendBuffers_ = tempRequests;
  if (outstandingSendBuffers_.message_)
    ret = sendR(outstandingSendBuffers_.message_,outstandingSendBuffers_.waited_);

  return ret;
}

inline
ExSqlComp::ReturnStatus ExSqlComp::sendR(CmpMessageObj* c, NABoolean w)
{
  ReturnStatus ret;

   // Get the index into the cli compiler array  so that we use the correct
   // arkcmpInitFailed entry and not the default.

  short indexIntoCliCompilerArray = 0;
  indexIntoCliCompilerArray = cliGlobals_->currContext()->getIndexToCompilerArray();

  //
  if (badConnection_)
    {      
      deleteServerStruct();
      badConnection_ = FALSE;
      breakReceived_ = FALSE;
    }
  
  if (!sqlcompMessage_) 
    { 
      ret = startSqlcomp();
      if (ret == ERROR)
         return ret;
    } 
  //

  if(this->isShared() && (cliGlobals_->currContext() != lastContext_))
  {
    ret = resendControls(TRUE /*context switch? */);
    if( ret == ERROR)
    {
	*diagArea_ << DgSqlCode(-CLI_SEND_ARKCMP_CONTROL); 
	return ret;
    }
  }
  

  sqlcompMessage_->clearAllObjects();
  (*sqlcompMessage_) << *c;
  sqlcompMessage_->setWaited(w);  // stored the waited mode

  // send the message.
  Int64 transid = cliGlobals_->currContext()->getTransaction()->getExeXnId(); 
  recentIpcTimestamp_ = NA_JulianTimestamp();
  sqlcompMessage_->send(w, transid);

  if (badConnection_) 
      return ERROR;


  // Any arkcmp initialization failure, we constantly harp on it so user will
  // be cautious.  If arkcmp then crashes and is restarted, render everything
  // unusable (the Executor is still ok, though -- previously PREPAREd stmts
  // can still be run).
  ContextCli::ArkcmpFailMode fm =
    cliGlobals_->currContext()->arkcmpInitFailed(indexIntoCliCompilerArray);
  if (fm == ContextCli::arkcmpWARN_)
    {
      error(+ CLI_ARKCMP_INIT_FAILED);
      return WARNING;
    }
  else if (fm == ContextCli::arkcmpERROR_)
    {
      error(- CLI_ARKCMP_INIT_FAILED);
      return ERROR;
    }

  return SUCCESS;
}

// Wait for request to complete. Called by Statement::releaseTransaction.
void ExSqlComp::completeRequests()
{
  sqlcompMessage_->waitOnMsgStream(IpcInfiniteTimeout);
  recentIpcTimestamp_ = NA_JulianTimestamp();
}

// The return status of ERROR here is only useful for WAITED requests --
// which are the default, e.g. used by refreshEnvs(), resendControls(),
// and the CLI.
inline
ExSqlComp::ReturnStatus ExSqlComp::waitForReply()
{
  sqlcompMessage_->waitOnMsgStream(IpcImmediately);
  recentIpcTimestamp_ = NA_JulianTimestamp();
  return (outstandingSendBuffers_.ioStatus_ == FINISHED) ? SUCCESS : ERROR;
}

ExSqlComp::OperationStatus ExSqlComp::status(Int64 reqId) 
{
  OperationStatus s = FINISHED;
 
  waitForReply();
  s = outstandingSendBuffers_.ioStatus_;

  return s;
}

// --------------------------------------------------------------------------
// Parse the info fetched from Describe::bindNode().  Genesis 10-981211-5986.
//
static NABoolean pairLenTxt(Int32 &len, const char *&txt, const char *&cqd)
{
  len = 0;
  for (Int32 i=3; i--; cqd++)
    {
      //      if (isdigit(*cqd))      	len = (10 * len) + *cqd - '0';
      //      else if (isspace(*cqd)) 	len = (10 * len);
      if (isDigit8859_1((unsigned char)*cqd)) len = (10 * len) + *cqd - '0';
      else if (isSpace8859_1((unsigned char)*cqd)) 	len = (10 * len);
      else			return TRUE;			// error
    }
  txt = cqd;
  cqd += len;
  return FALSE;
}


void ExSqlComp::appendControls(ExControlArea *dest, ExControlArea *src){
  Queue *srcList = src->getControlList();
  ExControlEntry *ctl = NULL;

  for (srcList->position(); ctl = (ExControlEntry *)srcList->getNext(); )
    {
      
      dest->addControl(ctl->type(), ctl->getReset(), 
		       ctl->getSqlText(), ctl->getSqlTextLen(),
		       ctl->getValue(1), ctl->getLen(1), 
		       ctl->getValue(2), ctl->getLen(2), 
		       ctl->getValue(3), ctl->getLen(3),
                       ctl->getActionType(),
		       ctl->getResendType(),
                       ctl->isNonResettable());
    }
}
//
static ExSqlComp::ReturnStatus saveControls(ExControlArea *ca, const char *cqd)
{
	      #ifdef _DEBUG
	      if (getenv("SQLCOMP_DEBUG")) {
		size_t len = str_len(cqd);
		cerr << len << "\t";
		if (len < 800)
		  cerr << cqd << endl;
		else {
		  char *cqd_ = (char *)cqd;
		  char a = cqd_[800];
		  cqd_[800] = '\0';
		  cerr << cqd << endl;
		  cqd_[800] = a;
		}
		cerr << endl;
	      }
	      #endif

  ExSqlComp::ReturnStatus ret = ExSqlComp::SUCCESS;
  char sqlText[2200];
  Int32 lenP = str_len(ca->getText(DEFAULT_));		// len of prefix:
  str_cpy_all(sqlText, ca->getText(DEFAULT_), lenP);	// "CQD"
  sqlText[lenP++] = ' ';				// "CQD "

  while (*cqd)
    {
      Int32 lenN, lenV, lenX = lenP;
      const char *nam, *val;

      if (pairLenTxt(lenN, nam, cqd) ||
          pairLenTxt(lenV, val, cqd) ||
	  lenN == 0)			   // name can't be blank; value can be.
        { ret = ExSqlComp::ERROR; break; }

      str_cpy_all(&sqlText[lenX], nam, lenN);
      lenX += lenN;
      sqlText[lenX++] = ' ';				// "CQD nam "

      sqlText[lenX++] = '\'';
      str_cpy_all(&sqlText[lenX], val, lenV);
      lenX += lenV;
      sqlText[lenX++] = '\'';
      sqlText[lenX++] = ';';				// "CQD nam 'val';"

      //ca->addControllAreaOnly(DEFAULT_, -1, sqlText, lenX, nam, lenN, val, lenV, 0, 0, ComTdbControl::NONE_,
      ca->addControl(DEFAULT_, -1, sqlText, lenX, nam, lenN, val, lenV, 0, 0, ComTdbControl::NONE_,
	  ExControlEntry::UPON_CMP_CRASH);
    }

  static const char cqdResetReset[] = "* RESET RESET;";
  str_cpy_all(&sqlText[lenP], cqdResetReset, sizeof(cqdResetReset));
  lenP += sizeof(cqdResetReset);
  ca->addControl(DEFAULT_, 2, sqlText, lenP,
    0, 0, 0, 0, 0, 0, ComTdbControl::NONE_,
    ExControlEntry::UPON_CMP_CRASH);		// "CQD * RESET RESET;"

  return ret;
}


ExSqlComp::ReturnStatus ExSqlComp::resetAllDefaults(){
  const char * buf[] = {"control query default * reset", "control query shape off", 
    "control table * reset", "control session * reset"};

  for(Int32 i=0; i<sizeof(buf)/sizeof(char *); i++){
    Lng32 len = str_len(buf[i])+1;
    CmpCompileInfo c((char *)buf[i], len,
                     (Lng32)SQLCHARSETCODE_UTF8
                     , NULL, 0, 0, 0);
    size_t dataLen = c.getLength();
    char * data = new(h_) char[dataLen];
    c.pack(data);

    ReturnStatus ret = sendRequest(EXSQLCOMP::SQLTEXT_STATIC_COMPILE,
		      data,dataLen);
      
    
    h_->deallocateMemory(data);
    if(ret == ERROR) return ret;
  }
  return SUCCESS;
}

ExSqlComp::ReturnStatus ExSqlComp::resendControls(NABoolean ctxSw)   // Genesis 10-981211-5986
{
  // If we are already resending the controls, then we must return.
  // Otherwise, we will resend the controls and set the resendControls_ state
  // to TRUE. When we are done, we will reset the resendingControls_ state to
  // FALSE. This is all because resendControls() call sendRequest()
  // that call resendControls() again.
  // * sigh *, if we could design the call with two-layered pattern.
  // The lower layer should avoid calling the upper layer. 
  if (resendingControls_)
    return SUCCESS;

  resendingControls_ = TRUE;

  // ##DLL-linkage problem ...
  // ## Perhaps we need to copy the global IdentifyMyself to exe-glob-ctxt ...
  // cerr << "## resendControls: I am " << IdentifyMyself::getMyName() << endl;

  ReturnStatus ret = SUCCESS;

  NABoolean e = doRefreshEnvironment_;				// save
  CmpMessageObj* t = outstandingSendBuffers_.message_;		// save
  outstandingSendBuffers_.message_= 0;

  // Get the index into the cli compiler array  so that we use the correct
  // arkcmpInitFailed entry and not the default.

  ContextCli *ctxt = cliGlobals_->currContext();
  short indexIntoCliCompilerArray = ctxt->getIndexToCompilerArray();

  //
  if (ctxt->arkcmpInitFailed(indexIntoCliCompilerArray))
  {
    ctxt->arkcmpInitFailed(indexIntoCliCompilerArray) =
      ContextCli::arkcmpERROR_;
    error(- CLI_ARKCMP_INIT_FAILED);

    ret = ERROR;
  }
  

  if (ret != ERROR)
  {
    // The message contains the following:
    //   (auth state and user ID are delimited by commas)
    //     authorization state (0 - off, 1 - on)
    //     integer user ID
    //     database user name
    // See CmpStatement::process (CmpMessageDatabaseUser) for more details
    Int32 *userID = ctxt->getDatabaseUserID();
    Int32 userAsInt = *userID;
    CmpContext *cmpCntxt = CmpCommon::context();
    NABoolean authOn = cmpCntxt ? cmpCntxt->isAuthorizationEnabled() : FALSE;

    char userMessage [MAX_AUTHID_AS_STRING_LEN + 1 + MAX_USERNAME_LEN + 1 + 2];
    str_sprintf(userMessage, "%d,%d,%s", authOn, userAsInt, ctxt->getDatabaseUserName());

#ifdef _DEBUG
    NABoolean doDebug = (getenv("DBUSER_DEBUG") ? TRUE : FALSE);
    if (doDebug)
    {
      printf("[DBUSER:%d] Sending CMP user credentials through ExSqlComp::resendControls,  %s\n",
             (int) getpid(), userMessage);
      fflush(stdout);
    }
#endif
    
    ret = sendRequest(EXSQLCOMP::DATABASE_USER,
                      (const char *) &userMessage,
                      (ULng32) sizeof(userMessage));
  }

  ComDiagsArea *loopDiags = NULL;
  ExControlArea *ca = ctxt->getControlArea();
  Queue *q = ca->getControlList();

  if (ret != ERROR)
  {
    if (q->isEmpty())
    {
      ret=resetAllDefaults(); 
      // 
      if (ret == ERROR)
      {
        resendingControls_ = FALSE;
        lastContext_ = 0;
        
        return ERROR;
      }
      
      
      ExControlArea *sharedCtrl = cliGlobals_->getSharedControl();
      Queue *sharedCtrlList = sharedCtrl->getControlList();
      if (sharedCtrlList->isEmpty())
      {
        // First time any arkcmp was started.  Our caller is sendR().
        // We ask arkcmp for all its initial (read-from-Defaults-table)
        // defaults in the "externalized" subset, adding these to the
        // Executor ControlArea.  Thus, if any values in the persistent
        // Defaults table get changed (by any user) between now and this
        // arkcmp's crashing, when we restart arkcmp we will feed it
        // these CQDs to reestablish the exact same context (i.e. we'll
        // override the later Defaults-table values).
        doRefreshEnvironment_ = FALSE;
        
        char * buf = (char *)GetControlDefaults::GetExternalizedDefaultsStmt();
        Lng32 len =
          str_len(GetControlDefaults::GetExternalizedDefaultsStmt())+1;
        CmpCompileInfo c(buf, len,
                         (Lng32)SQLCHARSETCODE_UTF8
                         , NULL, 0, 0, 0);
        size_t dataLen = c.getLength();
        char * data = new(h_) char[dataLen];
        c.pack(data);
        
        ReturnStatus ret = sendRequest(EXSQLCOMP::SQLTEXT_STATIC_COMPILE,
                                       data,dataLen);
        
        h_->deallocateMemory(data);
        
        Int32 mainSqlcode = (Int32) diagArea_->mainSQLCODE();
        if (ret != ERROR &&
            ABS(mainSqlcode) == ABS(EXE_INFO_CQD_NAME_VALUE_PAIRS))
        {
          // This dang diagArea error list is [1]-based, not [0]-based...
          const char *cqd = (*diagArea_)[1].getOptionalString(0);
          if (cqd && str_len(cqd))
          {
            ret = saveControls(sharedCtrl, cqd);
          }
          if (ret != ERROR) 
            diagArea_->clear();
        }
        if ((ret == ERROR) && (!breakReceived_))
        {
          ctxt->arkcmpInitFailed(indexIntoCliCompilerArray) =
            ContextCli::arkcmpWARN_;
          error(+ CLI_ARKCMP_INIT_FAILED);
        }
        
        breakReceived_ = FALSE;
      }
      appendControls(ca, sharedCtrl);
      
    } // if (q->isEmpty())	
    
    else
    {
      ret=resetAllDefaults();
      if (ret == ERROR)
      {
        resendingControls_ = FALSE;
        lastContext_ = 0;
        
        return ERROR;
      }
      
      // Start a new arkcmp after earlier one crashed;
      // need to reestablish the new arkcmp's control state.
      // Our caller is resendRequest().
      ExControlEntry *ctl;
      for (q->position(); ctl = (ExControlEntry *)q->getNext(); )
      {
        if (ctxSw && (ctl->getResendType() == ExControlEntry::UPON_CMP_CRASH))
        {
          continue; // bypass controls needed for the crahsed arkcmp.
        }
#ifdef SHARE_ARKCMP_DEBUG
        cerr << "resend " << ctl->getSqlTextLen() << "\t"
             << ctl->getSqlText() << endl;
#endif
#ifdef _DEBUG
        if (getenv("SQLCOMP_DEBUG"))
          cerr << ctl->getSqlTextLen() << "\t" << ctl->getSqlText() << endl;
#endif
        
        // Each time thru loop.  See [##] in sendRequest().
        doRefreshEnvironment_ = FALSE;
        
        // ExControlTcb::work() is where this gets done for the earlier arkcmp
        char * buf = ctl->getSqlText();
        
        // Ignore OSIM-related CQDs
        if (strstr(buf, "CQD") || strstr(buf, "CONTROL QUERY DEFAULT"))
          if (strstr(buf, "OSIM"))
            continue;
        
        Lng32 len = ctl->getSqlTextLen()+1;
	CmpCompileInfo c(buf, len,
                         (Lng32)SQLCHARSETCODE_UTF8
                         , NULL, 0, 0, 0);
        size_t dataLen = c.getLength();
        char * data = new(h_) char[dataLen];
        c.pack(data);
        
        if (strstr(buf, "CQD") || strstr(buf, "CONTROL QUERY DEFAULT"))
        {
          if (strstr(buf, "REPLICATE_ALLOW_ROLES"))
          {
            // allow setOnce cqds.
            cliGlobals_->currContext()->setSqlParserFlags(0x400000);
          }
        }
        
        ReturnStatus ret = sendRequest(EXSQLCOMP::SQLTEXT_STATIC_COMPILE,
                                       data,dataLen);
        
        if (strstr(buf, "CQD") || strstr(buf, "CONTROL QUERY DEFAULT"))
        {
          if (strstr(buf, "REPLICATE_ALLOW_ROLES"))
          {
            // allow setOnce cqds.
            cliGlobals_->currContext()->resetSqlParserFlags(0x400000);
          }
        }
        
        h_->deallocateMemory(data);
        
        if (ret != ERROR)
          if (waitForReply() == ERROR)
            ret = ERROR;
        
        if (ret != ERROR)
        {
	  //
          if ( 
              ((*diagArea_).contains(-2050) || 
               (*diagArea_).contains(-2055))  &&
              (compilerVersion_ < COM_VERS_COMPILER_VERSION)
              )
          {
            diagArea_->clear();
          }
          else
          {
            if (diagArea_->getNumber() > 0)
            {
               if (loopDiags == NULL)
                   loopDiags = ComDiagsArea::allocate(h_);
               loopDiags->mergeAfter(*diagArea_);
               diagArea_->clear();
            }
          }
          ret = SUCCESS;
        }
        else
          break;
        
      } // for each control
      
      // ## CmpDescribe sendAllControls() should probably send this too
      // ## to avoid Executor sending prepared stmts back to be rebound...
      if (ret != ERROR)
      {
        ExTransaction *ta = ctxt->getTransaction();
        ReturnStatus ret = sendRequest(EXSQLCOMP::SET_TRANS,
                                       (char *)ta->getUserTransMode(),
                                       sizeof(TransMode));
        
        if (ret != ERROR)
          if (waitForReply() == ERROR)
            ret = ERROR;
      }
      
      if ((ret == ERROR) && (!breakReceived_))
      {
        ctxt->arkcmpInitFailed(indexIntoCliCompilerArray) =
          ContextCli::arkcmpERROR_;
        error(- CLI_ARKCMP_INIT_FAILED);
      }
      
      breakReceived_ = FALSE;
      
    } // control list is NOT empty
  } // if (ret != ERROR)
  //
  if (ret != SUCCESS || diagArea_->getNumber() || loopDiags != NULL )
  {
    if (loopDiags != NULL)
    {
       diagArea_->mergeAfter(*loopDiags);
       loopDiags->decrRefCount();
    }
    if (ret != ERROR)
      ret = diagArea_->getNumber(DgSqlCode::ERROR_) ? ERROR : WARNING;
    if (ret == ERROR)
      *diagArea_ << DgSqlCode(- CLI_SEND_REQUEST_ERROR)
                 << DgString0("resendControls");
  }  
  
  doRefreshEnvironment_ = e;					// restore
  outstandingSendBuffers_.message_ = t;				// restore
  
  if (ret == SUCCESS)
  {
    lastContext_ = cliGlobals_->currContext();
  }
  else
  {
    lastContext_ = 0;
  }
  
  resendingControls_ = FALSE;

  return ret;
}

//inline
ExSqlComp::ReturnStatus ExSqlComp::refreshEnvs()
{
  ReturnStatus ret = SUCCESS;
  
  CmpMessageObj* t = outstandingSendBuffers_.message_;	// save
  outstandingSendBuffers_.message_= 0;
  
  CmpMessageEnvs envs(CmpMessageEnvs::EXGLOBALS,
		      cliGlobals_->getEnvVars(),
		      FALSE,
		      getHeap());
  
  ret = sendR(&envs);
      
  if (ret != ERROR)
    if (waitForReply() == ERROR)
      ret = ERROR;
  
  if (ret == ERROR)
    *diagArea_ << DgSqlCode(- CLI_SEND_REQUEST_ERROR)
	       << DgString0("SETENV");
  
  outstandingSendBuffers_.message_ = t;			// restore
  
  if (ret == ERROR) return ret;

  doRefreshEnvironment_ = FALSE;  
  return ret;
}

// -----------------------------------------------------------------------
// Constructors of ExSqlComp
// -----------------------------------------------------------------------

ExSqlComp::ExSqlComp(void* ex_environment,
		     CollHeap *h,
		     CliGlobals *cliGlobals,
		     ExStoredProcTcb *storedProcTcb,
                     short version,
                     char *nodeName ,
                     IpcEnvironment *env):
isShared_(FALSE), lastContext_(NULL), resendingControls_(FALSE), nodeName_(NULL)
{
  h_ = h;

  allocMethod_ = IPC_ALLOC_DONT_CARE;

  cliGlobals_ = cliGlobals;
  env_ = env;
  sc_ = 0;
  sqlcompMessage_ = 0;
  storedProcTcb_ = storedProcTcb;
  compilerVersion_ = version;
  exEnvironment_ = ex_environment;
  doRefreshEnvironment_ = TRUE;
  initRequests(outstandingSendBuffers_);
  badConnection_ = FALSE;
  breakReceived_ = FALSE;
  currentISPRequest_ = 0;
  connectionType_ = CmpMessageConnectionType::DMLDDL;
  if (nodeName)
   {
     nodeName_ = new (h_) char[strlen(nodeName)+1];
     strcpy(nodeName_,nodeName);
   }
  server_ = 0;

  diagArea_ = ComDiagsArea::allocate(h_);  
  recentIpcTimestamp_ = -1; 
}

ExSqlComp::~ExSqlComp()
{
  if (server_)
    {
      // release an existing server. This will close the connection
      // to mxcmp. Do this for NSK only as for some reason, which am
      // not going to debug right now, this doesn't work on NT platform.

      // send exit message to MXCMP if NT platform.
      if ( sqlcompMessage_ )
	{
	  CmpMessageExit exitmsg;
	  sqlcompMessage_->clearAllObjects();
	  (*sqlcompMessage_) << exitmsg;
	  sqlcompMessage_->setWaited(TRUE);
	  sqlcompMessage_->send(TRUE);
	}

    }

  delete sqlcompMessage_;

  NADELETE(sc_, IpcServerClass, h_);
  NADELETEBASIC(nodeName_, h_);
  nodeName_ = NULL;

  if (diagArea_)
    diagArea_->deAllocate();
}
//
void ExSqlComp::endConnection()
{
  if ( sqlcompMessage_ )
    {
      CmpMessageExit exitmsg;
      sqlcompMessage_->clearAllObjects();
      (*sqlcompMessage_) << exitmsg;
      sqlcompMessage_->setWaited(TRUE);
      sqlcompMessage_->send(TRUE);
    }
 
  delete sqlcompMessage_;
  
  sqlcompMessage_ = NULL;
  doRefreshEnvironment_ = TRUE;
}

inline NABoolean ExSqlComp::getEnvironment(char*&, ULng32&) 
{
  // TODO : use environment in executor globals.
  return TRUE;
}

ExSqlComp::ReturnStatus ExSqlComp::preSendRequest(NABoolean doRefreshEnvs)
{
  ReturnStatus ret = SUCCESS;

  // currently, only one outstanding I/O is supported.
  assert( !outstandingSendBuffers_.message_);
  // make sure all the replys from currentISPRequest_ have been fetched already.
  assert( currentISPRequest_ == 0 );

  initRequests(outstandingSendBuffers_);

  clearDiags();
  retval_ = SUCCESS;

  //  if ( doRefreshEnvs && doRefreshEnvironment_ ) 
  //    {
  //     doRefreshEnvironment_ = FALSE;
  //  ret = refreshEnvs();
  //  }
  if (doRefreshEnvs)
    ret = refreshEnvs();  // this starts the server.
  else
    {
      // Start mxcmp if not started.
      // This is needed for nowaited prepare.
      if (!sqlcompMessage_) 
        ret = startSqlcomp();
     }

  return ret;
}

ExSqlComp::ReturnStatus 
ExSqlComp::sendRequest(CmpMessageObj* request, NABoolean waited)
{
  ReturnStatus ret = SUCCESS;
  outstandingSendBuffers_.ioStatus_ = ExSqlComp::PENDING;
  outstandingSendBuffers_.message_ = request;
  outstandingSendBuffers_.waited_ = waited;
  outstandingSendBuffers_.requestId_ = request->id();
  ret = sendR(request, waited);
  if (ret != ERROR)
    if (waitForReply() == ERROR)
      if (waited)
        ret = ERROR;
  
  return ret;
}

// The server fielding these requests is ExCmpMessage::actOnReceive
// in arkcmp/CmpConnection.cpp.
// 
ExSqlComp::ReturnStatus ExSqlComp::sendRequest (Operator op,
						const char* const_input_data,
						ULng32 size,
					        NABoolean waited,
						Int64* id,
						Lng32 charset,
						NABoolean resendFlg,
                                                const char *parentQid,
                                                Lng32 parentQidLen)
{
  ReturnStatus ret;

  char *input_data = (char *)const_input_data;
  
  // if this request is going to a downrev compiler(pre-2300),
  // change charset to ISO88591. 
  // Downrev compilers prior to V2300 do not understand ISO_MAPPING.
  //
  // When we move to v2400 and beyond, this code need to correctly
  // figure out which charsets are not understood by the downrev
  // compiler where this msg is being sent to.
  //
  if (getVersion() < COM_VERS_2300)
    {
      charset = SQLCHARSETCODE_ISO88591;
    }
  
  if ( ( ret = preSendRequest(FALSE) ) == ERROR )
    {
      if ( resendFlg && badConnection_ )
	{
	  // retry once.
	  ret = preSendRequest(FALSE);
	}
      if (ret == ERROR)
	return ret;
    }
  
  
  // send out the request, or process the request in one process mode 
  CmpMessageRequest* request = NULL;
  
  if (charset == SQLCHARSETCODE_UNKNOWN)
  {
    charset = CharInfo::UTF8;
  }
  
  switch (op)
    {
    case EXSQLCOMP::ENVS_REFRESH :
      // TODO : this part needs to be reconstructed 
      ret = refreshEnvs();
      break;
      
    case EXSQLCOMP::SQLTEXT_RECOMPILE:
    case EXSQLCOMP::SQLTEXT_COMPILE :   // we might be static or dynamic mode
      request=new(h_) 
	CmpMessageSQLText(input_data,(CmpMsgBufLenType)size,h_,charset,op);
      
      break;
      
    case EXSQLCOMP::SQLTEXT_STATIC_RECOMPILE : 
    case EXSQLCOMP::SQLTEXT_STATIC_COMPILE : // force to static compilation mode
      request=new(h_) 
	CmpMessageCompileStmt(input_data,(CmpMsgBufLenType)size,op,h_, charset);
      break;
      
    case EXSQLCOMP::UPDATE_HIST_STAT:
      request = new(h_)
	CmpMessageUpdateHist(input_data,(CmpMsgBufLenType)size,h_,charset);
      break;
      
    case EXSQLCOMP::PROCESSDDL :
      request = new(h_) 
	CmpMessageDDL(input_data,(CmpMsgBufLenType)size,h_,charset, parentQid, parentQidLen);
      break;
      
    case EXSQLCOMP::DESCRIBE :
      request = new(h_) 
	CmpMessageDescribe(input_data,(CmpMsgBufLenType)size,h_,charset);
      break;
      
    case EXSQLCOMP::SET_TRANS :
      request = new(h_) 
	CmpMessageSetTrans(input_data,(CmpMsgBufLenType)size,h_);
      break;
      
    case EXSQLCOMP::DDL_NATABLE_INVALIDATE :
      request = new(h_) 
	CmpMessageDDLNATableInvalidate(input_data,(CmpMsgBufLenType)size,h_);
      break;
      
    case EXSQLCOMP::DATABASE_USER :
      request = new(h_) 
	CmpMessageDatabaseUser(input_data,(CmpMsgBufLenType)size,h_);
      break;
      
    case EXSQLCOMP::END_SESSION :
      request = new(h_)CmpMessageEndSession(input_data, 
					    (CmpMsgBufLenType)size, h_);
      break;
      
    case EXSQLCOMP::DDL_WITH_STATUS :
      request = new(h_)CmpMessageDDLwithStatus(input_data, 
					   (CmpMsgBufLenType)size, h_);
      break;
      
    default :
      assert(FALSE);
      break;	  
    }
  
  if (request)
    {
      request->setFlags(cliGlobals_->currContext()->getSqlParserFlags()); 
      
      // If we are talking to a downrev compiler take care of the following.
      //
      if ( compilerVersion_ < COM_VERS_COMPILER_VERSION)
	{
	  // if the structure of any of the above message op types have changed from
	  // one release to another, redefine the foll virtual method :
	  // migrateToNewVersionto do the
	  // translation to the down rev structure.
	  // By default, it just sets the version. 
	  request->migrateToVersion(compilerVersion_);
	  
	  
	}
      
      // send the request.
      ret = sendRequest(request, waited);
      if ((ret == ERROR) && badConnection_)
        {
          if (resendFlg)
	    {
	      outstandingSendBuffers_.ioStatus_ = ExSqlComp::PENDING;
	      outstandingSendBuffers_.message_ = request;
	      outstandingSendBuffers_.waited_ = waited;
	      outstandingSendBuffers_.requestId_ = request->id();
	      badConnection_ = FALSE;
	      
	      clearDiags();
	      ret = resendRequest();
	      
	      if (ret != ERROR)
		{
		  if (id) 
		    *id = request->id();
		}
	      else
		{
		  //
		  // The second retry failed. Reset outstandingSendBuffers.
		  outstandingSendBuffers_.ioStatus_ = ExSqlComp::FINISHED;
		  outstandingSendBuffers_.resendCount_ = 0;
		  outstandingSendBuffers_.message_ = 0;
		  badConnection_ = FALSE;
		  
		}
	      
	    } // if (resendFlg)
	  
          else
	    {
	      // The first send attempt failed and the ExSqlComp caller
	      // does not want us to retry. Reset outstandingSendBuffers_.
	      outstandingSendBuffers_.ioStatus_ = ExSqlComp::FINISHED;
	      outstandingSendBuffers_.resendCount_ = 0;
	      outstandingSendBuffers_.message_ = 0;
	      badConnection_ = FALSE;
	      
	    } // if (resendFlg) else ...
          
        } // if ((ret == ERROR) && badConnection_)
    } // if (request)
  
  // For now [##], always reset this for next time in.
  // We should really do refreshEnvs only when envs actually changed
  // (in sqlci, only when a SET/RESET DEFINE occurred).
  doRefreshEnvironment_ = TRUE;

  return ret;  
}

// stored proc request.
ExSqlComp::ReturnStatus ExSqlComp::sendRequest(
  const char* procName,
  void* inputExpr, ULng32 inputExprSize, // input Expr
  void* outputExpr, ULng32 outputExprSize, // output Expr
  void* keyExpr, ULng32 keyExprSize, // key expr
  void* inputData, ULng32 inputDataSize, // input data
  ULng32 outputRecSize, ULng32 outputTotalSize, // output data
  NABoolean waited, Int64* id,
  const char *parentQid,
  Lng32 parentQidLen)
{
   ReturnStatus ret = ERROR;

   connectionType_ = CmpMessageConnectionType::ISP;
   if ( ( ret = preSendRequest(TRUE) ) == ERROR ) return ret;

   CmpMessageObj* request = new(h_) CmpMessageISPRequest
     ((char *)procName,			//## for now, cast away constness...
     inputExpr, inputExprSize, outputExpr, outputExprSize,
     keyExpr, keyExprSize, inputData, inputDataSize,
     outputRecSize, outputTotalSize, h_, parentQid, parentQidLen);

   if (request)
     {
       request->setFlags(cliGlobals_->currContext()->getSqlParserFlags()); 

       // Save the current request ID because request will be deleted
       // in sendRequest()
       Int64 savedISPRequest = request->id();

       ret = sendRequest(request, waited);
       if (ret != ERROR)
       {
	 currentISPRequest_ = savedISPRequest;
	 if (id) *id = currentISPRequest_;
       }
     }

  // For now [##], always reset this for next time in.
  // We should really do refreshEnvs only when envs actually changed
  // (in sqlci, only when a SET/RESET DEFINE occurred).
   doRefreshEnvironment_ = TRUE;
   return ret;
}

ExSqlComp::ReturnStatus ExSqlComp::getNext(ULng32 bufSize, 
                                           Int64 id,
                                           NABoolean waited,
                                           const char *parentQid,
                                           Lng32 parentQidLen)
{
  ReturnStatus ret = SUCCESS;
  Int64 ispRequest = ( id ) ? id : currentISPRequest_; 
  if ( !ispRequest ) 
    return ERROR;

  // should not call presendRequest here, because the environment should be the
  // same for one ISP execution request sent earlier. So ExSqlComp should not
  // send the refresh environment again to arkcmp.
  clearDiags();
  CmpMessageObj* request = new(h_) CmpMessageISPGetNext
    (bufSize, ispRequest, 0, h_, parentQid, parentQidLen);
  if ( !request )
    return ERROR;
  ret = sendRequest(request, waited);
  return ret;
}
                                           
				     
ExSqlComp::ReturnStatus ExSqlComp::getReply
(char*& reply,ULng32& size, ULng32 maxSize, Int64 reqId,
 NABoolean getDataWithErrReply)
{
  ReturnStatus ret = SUCCESS;

  assert(outstandingSendBuffers_.ioStatus_ == FINISHED);
  outstandingSendBuffers_.ioStatus_ = FETCHED;

  Int64 request = ( reqId ) ? reqId : outstandingSendBuffers_.requestId_;
  if (diagArea_->getNumber(DgSqlCode::ERROR_))
    {
      if (NOT getDataWithErrReply)
	{
	  reply=NULL;
	  retval_ = ERROR;
	  return retval_; 
	}
      
      ret = retval_ = ERROR;
    }

  if (diagArea_->getNumber(DgSqlCode::WARNING_))
    {
      ret = retval_ = WARNING;
    }

  switch (sqlcompMessage_->getNextObjType()) 
    {
      case CmpMessageObj::REPLY_CODE :
	{
	  CmpMessageReplyCode r(h_,0, reply, maxSize, h_);
	  (*sqlcompMessage_) >> r;
          // in the future when there might be more than one outstanding I/O, the
          // reply should be put into the receive buffer list, so it can be retrieved
          // later.
          assert ( r.request() == request );
	  reply = r.takeData();
	  size = r.getSize();
          ret = retval_; 
	  break;
	}

      case CmpMessageObj::REPLY_ISP :
        {
          CmpMessageReplyISP r(h_, 0, reply, maxSize, h_);
          (*sqlcompMessage_) >> r;
          assert ( r.request() == request || r.request() == currentISPRequest_ );
          reply = r.takeData();
          size = r.getSize();
          // There is no WARNING status returned in this case, since the return status 
          // indicates whether there is more data to come or not. In the case of warning
          // the remaining data should still be fetched. The warning information is kept
          // in diagArea_ though.
          retval_ = ret = r.areMore() ? MOREDATA : SUCCESS;
          if ( retval_ == SUCCESS )
            currentISPRequest_ = 0;
          break;
        }
      default :
	break;
    }	     			   	
  return ret;
}

ComDiagsArea* ExSqlComp::getDiags(Int64 ) 
{
  return diagArea_;
}
//
ComDiagsArea* ExSqlComp::takeDiags(Int64)
{
  ComDiagsArea* d = diagArea_;
  diagArea_=0;
  return d;
}

void ExSqlComp::deleteServerStruct() 
{

  delete sqlcompMessage_;

  delete sc_;
  
  sqlcompMessage_ = NULL;
  sc_ = NULL;
}


// -----------------------------------------------------------------------
// Methods for CmpMessageStream 
// -----------------------------------------------------------------------

CmpMessageStream::CmpMessageStream(IpcEnvironment* env, ExSqlComp* sqlcomp) : 
  IpcMessageStream (env, CmpMessageObj::EXE_CMP_MESSAGE,
		    EXECMPIPCVERSION, 0, TRUE)
{
  sqlcomp_ = sqlcomp;
  waited_ = TRUE;
}

void CmpMessageStream::actOnSend(IpcConnection*) 
{
  if (sqlcomp_->storedProcTcb_ )
  {
    // at this point, get a pointer to the ex_stored_proc_tcb, if applicable,
    // and call that TCB's tickleScheduler() method.
    sqlcomp_->storedProcTcb_->tickleScheduler();
  }
  
  if (getState() == ERROR_STATE)
    {
      // Not to resend request here due to a memory corruption bug where 
      // sqlcomp_->sqlcompMessage_ was deleted and referenced later.
      // Setting badConnection_ to true will trigger high layer to resend.
      // sqlcomp_->resendRequest();
      sqlcomp_->badConnection_ = TRUE;    
      return;
    }
  
  if (getState() == BREAK_RECEIVED)
  {
     // received a break signal while waiting on MXCMP. Kill MXCMP, 
     // and return a warning to MXCI, to indicate that the break key was
     // received.
     sqlcomp_->outstandingSendBuffers_.message_ = 0;
     sqlcomp_->outstandingSendBuffers_.ioStatus_ = ExSqlComp::FINISHED;
     sqlcomp_->badConnection_ = TRUE;
     sqlcomp_->breakReceived_ = TRUE;
    NAProcessHandle phandle((SB_Phandle_Type *)
      &(sqlcomp_->server_->getServerId().getPhandle().phandle_));
    Int32 guaRetcode = phandle.decompose();
    if (XZFIL_ERR_OK == guaRetcode)
    {
      msg_mon_stop_process_name(phandle.getPhandleString());
    }
    delete sqlcomp_->sqlcompMessage_;
    sqlcomp_->getDiags() ->setRollbackTransaction(-1);
    sqlcomp_->sqlcompMessage_ = NULL;
    sqlcomp_->doRefreshEnvironment_ = TRUE;
  }
}

void CmpMessageStream::actOnSendAllComplete()
{
  clearAllObjects();
  receive(waited_);
}

void CmpMessageStream::actOnReceive(IpcConnection*) 
{
 
  if ((getState() == ERROR_STATE) || (getState() == BREAK_RECEIVED)) 
    {
      // If the state is ERROR, this could be due to many different
      // scenarios, described below.
      // If the state is BREAK_RECEIVED, this means that we  
      // received a break signal while waiting on MXCMP. Kill MXCMP, 
      // and return to MXCI, or to executor stored proc, 
      // to indicate that the break key was received.

      // Not to resend request due to a memory corruption bug where 
      // sqlcomp_->sqlcompMessage_ was deleted and referenced later.
      
      // Set badConnection_ to true so that the request will be resent
      // at the high level (in sendRequest). It might be arkcmp 
      // crashed in the previous request. But
      // IPCMessageStream won't report the error until
      // receive method is called. If arkcmp dies in 
      // previous query, the following send actually fails
      // (IpcMessageStream still set the state as SENT instead of
      // ERROR ), when the receive method is called, IPCMessageStream
      // then sets the state as ERROR. 
      // 
      // Possible scenarios:
      // . arkcmp dies in previous request, ExSqlComp class should start
      //   another arkcmp.
      // . arkcmp dies in the middle of processing environment setup request, 
      //   this request will be resend one more time. Arkcmp will die again,
      //   resendRequest decides not to send more request and return with 
      //   errors.
      sqlcomp_->badConnection_ = TRUE;
    
      if (getState() == BREAK_RECEIVED) 
         sqlcomp_->breakReceived_ = TRUE;

      IpcMessageType typ = sqlcomp_->outstandingSendBuffers_.message_ ?
		       sqlcomp_->outstandingSendBuffers_.message_->getType() :
		       CmpMessageObj::NULL_REQUEST;

      if ( sqlcomp_->currentISPRequest_  ||	// an ISP request
	   typ == CmpMessageObj::INTERNALSP_REQUEST ||
	   typ == CmpMessageObj::INTERNALSP_GETNEXT)
	{
          if (getState() == ERROR_STATE)
	     sqlcomp_->error(arkcmpErrorResend);
	  sqlcomp_->initRequests(sqlcomp_->outstandingSendBuffers_);
	  sqlcomp_->currentISPRequest_ = 0;
	  sqlcomp_->outstandingSendBuffers_.ioStatus_ = ExSqlComp::FINISHED;
          if (getState() == BREAK_RECEIVED)
            {
              Int32 nid = 0;
              Int32 pid = 0;
              NAProcessHandle phandle((SB_Phandle_Type *)
                &(sqlcomp_->server_->getServerId().getPhandle().phandle_));
              Int32 guaRetcode = phandle.decompose();
              if (XZFIL_ERR_OK == guaRetcode)
              {
                msg_mon_stop_process_name(phandle.getPhandleString());
              }
              delete sqlcomp_->sqlcompMessage_;
              sqlcomp_->sqlcompMessage_ = NULL;
              sqlcomp_->getDiags() ->setRollbackTransaction(-1);
              sqlcomp_->doRefreshEnvironment_ = TRUE;
            }
	}

      if ((getState() != BREAK_RECEIVED) &&
          sqlcomp_->getDiags() &&
          sqlcomp_->getDiags()->getNumber() == 0)
      {
        sqlcomp_->error(-CLI_RECEIVE_ERROR);
      }

      if (sqlcomp_->storedProcTcb_)
	{
	  // and call that TCB's tickleScheduler() method.
	  sqlcomp_->storedProcTcb_->tickleScheduler();
	  return;
	}
    }

  else
  {
    sqlcomp_->outstandingSendBuffers_.ioStatus_ = ExSqlComp::FINISHED;
    sqlcomp_->outstandingSendBuffers_.resendCount_ = 0;
    if (sqlcomp_->outstandingSendBuffers_.message_)
    {
      // This is not an environment setup message, update the I/O status
      sqlcomp_->outstandingSendBuffers_.requestId_ = 
        sqlcomp_->outstandingSendBuffers_.message_->id();
      sqlcomp_->outstandingSendBuffers_.message_->decrRefCount();
      sqlcomp_->outstandingSendBuffers_.message_ = 0;
    }
  
    if (getNextObjType() == IPC_SQL_DIAG_AREA) 
    {
      ComDiagsArea diags(sqlcomp_->getHeap());
      *(this) >> diags;
      if (diags.getNumber()) sqlcomp_->getDiags()->mergeAfter(diags);
    }
  }

  if (sqlcomp_->storedProcTcb_)
  {
    // at this point, get a pointer to the ex_stored_proc_tcb, if applicable,
    // and call that TCB's tickleScheduler() method.
    sqlcomp_->storedProcTcb_->tickleScheduler();
  }
}


#if 0
NABoolean NAExecTrans(Lng32 command, 
                      Int64& transId)
{
  assert(command == 0);

  SQLDESC_ID transid_desc;
  SQLMODULE_ID module;
	
// added for multi charset module names
  init_SQLCLI_OBJ_ID(&transid_desc);
  init_SQLMODULE_ID(&module);

  module.module_name = 0;
  
  transid_desc.module = &module;
  transid_desc.name_mode = desc_handle;
  transid_desc.handle = 0L;
  Int32 rc;
  if ( rc=SQL_EXEC_AllocDesc(&transid_desc, 1) )
    return FALSE;
  
  Int64 transid = transId;
  if ( rc = SQL_EXEC_SetDescItem(&transid_desc, 1, SQLDESC_VAR_PTR,
    (Lng32)&transid, 0) )
    return FALSE;
  
  Lng32 cliCommand = command ? SQLTRANS_SET : SQLTRANS_STATUS;
  rc = SQL_EXEC_Xact(cliCommand, &transid_desc);
  if (rc == 0)
  {
    transId = transid;
  }

  SQL_EXEC_DeallocDesc(&transid_desc);

  return rc == 0;
}
#endif
