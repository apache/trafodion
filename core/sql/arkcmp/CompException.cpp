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

#include "string.h"
#include "CompException.h"
#include "NAInternalError.h"

#include "CmpEHCallBack.h"


BaseException::BaseException(const char *fileName, UInt32 lineNum)
  : lineNum_(lineNum)
{
  if(fileName) {
    strncpy(fileName_, fileName, sizeof(fileName_));
    fileName_[sizeof(fileName_)-1] = 0;
  }
  else {
    fileName_[0] = 0;
  }
}

BaseException::BaseException() : lineNum_(0)
{ fileName_[0] = 0; }

UInt32 BaseException::getLineNum()
{ return lineNum_; }

const char * BaseException::getFileName()
{ return fileName_; }

// Optinally hook the Debugger when something is wrong.
void BaseException::doFFDCDebugger(char * msg)
{
  makeTFDSCall(msg, fileName_, lineNum_);
  hookDebugger();
}

void BaseException::hookDebugger()
{
  static NABoolean firstTime = TRUE;
  static NABoolean invokeDebugger = FALSE;


  if(invokeDebugger)
    hookDebuggerHelper();
}

void BaseException::hookDebuggerHelper()
{
  MessageBox( NULL,
	      "An Internal Error Found, Attach Debugger Now",
	      "NonStop SQL/MX",
	      MB_OK|MB_ICONINFORMATION );
  // DebugBreak() stop the MSDEV when the user is in the MSDEV debugger
  // DebugBreak() terminates MXCMP when the user is not in the MSDEV debugger
  //              and the standard breakpoint exception handler is in place.
  DebugBreak();
}

// UserException Implementation

UserException::UserException(const char *fileName, UInt32 lineNum)
  : BaseException(fileName, lineNum)
{
}

void UserException::throwException()
{
  throw *this;
}

DDLException::DDLException(Int32 sqlcode, const char *fileName, UInt32 lineNum)
: BaseException(fileName, lineNum),
  sqlcode_ (sqlcode)
{
}
void DDLException::throwException()
{
  throw *this;
}
// FatalException Implementation

FatalException::FatalException(const char *msg,
			       const char *fileName,
			       UInt32 lineNum,
			       const char *stackTrace)
  : BaseException(fileName, lineNum)
{
  int len = 0;
  if(msg && (len = strlen(msg))) {
    len = MINOF(len,sizeof(msg_));
    strncpy(msg_, msg, len);
    msg_[len] = 0;
  }
  else {
    msg_[0] = 0;
  }
  
  if(stackTrace && (len = strlen(stackTrace))) {
    len = MINOF(len, sizeof(stackTrace_));
    strncpy(stackTrace_, stackTrace, len);
    stackTrace_[len] = 0;
  }
  else {
    stackTrace_[0] = 0;
  }
}

const char * FatalException::getMsg()
{ return msg_; }

const char * FatalException::getStackTrace()
{ return stackTrace_; }

void FatalException::throwException()
{
  doFFDCDebugger(msg_);
  throw *this;
}

// CmpInternalException Implementation

CmpInternalException::CmpInternalException(const char *msg,
			       const char *fileName,
			       UInt32 lineNum)
  : BaseException(fileName, lineNum)
{
  if(msg) {
    strncpy(msg_, msg, sizeof(msg_));
    msg_[sizeof(msg_)-1] = 0;
  }
  else {
    msg_[0] = 0;
  }
}

const char * CmpInternalException::getMsg()
{ return msg_; }

void CmpInternalException::throwException()
{
  doFFDCDebugger(msg_);
  throw *this;
}

// AssertException Implementation

AssertException::AssertException(const char *condition,
				 const char *fileName,
				 UInt32 lineNum,
				 const char *stackTrace)
  : BaseException(fileName, lineNum)
{
  int len = 0;
  if(condition && (len = strlen(condition))) {
    len = MINOF(len ,sizeof(condition_));
    strncpy(condition_, condition, len);
    condition_[len] = 0;
  }
  else {
    condition_[0] = 0;
  }
  
  if(stackTrace && (len = strlen(stackTrace))) {
    len = MINOF(len, sizeof(stackTrace_));
    strncpy(stackTrace_, stackTrace, len );
    stackTrace_[len] = 0;
  }
  else {
    stackTrace_[0] = 0;
  }
}

AssertException::AssertException(AssertException & e) :
  BaseException(e.getFileName(), e.getLineNum())
{
  int len = 0;
  const char *condition = e.getCondition();
  if(condition && (len = strlen(condition))) {
    len = MINOF(len ,sizeof(condition_));
    strncpy(condition_, condition, len);
    condition_[len] = 0;
  }
  else {
    condition_[0] = 0;
  }
  
  const char *stackTrace = e.getStackTrace();
  if(stackTrace && (len = strlen(stackTrace))) {
    len = MINOF(len, sizeof(stackTrace_));
    strncpy(stackTrace_, stackTrace, len);
    stackTrace_[len] = 0;
  }
  else {
    stackTrace_[0] = 0;
  }
}

const char * AssertException::getCondition()
{ return condition_; }

const char * AssertException::getStackTrace()
{ return stackTrace_; }

void AssertException::throwException()
{
  doFFDCDebugger(condition_);
  throw *this;
}

OsimLogException::OsimLogException(const char * errMsg,
                                   const char * srcFileName,
                                   UInt32 srcLineNum)
  : BaseException (srcFileName, srcLineNum)
{
  errMsg_[0] = 0;
  if(errMsg){
    strcpy(errMsg_,errMsg);
  }
}

void OsimLogException::throwException()
{
  throw *this;
}

// PassOneAssertFatalException Implementation

PassOneAssertFatalException::PassOneAssertFatalException(const char *condition,
							 const char *fileName,
							 UInt32 lineNum)
  : FatalException("Assertion occurred in optimizer pass one",
		   fileName,
		   lineNum)
{
  if(condition) {
    strncpy(condition_, condition, sizeof(condition_));
    condition_[sizeof(condition_)-1] = 0;
  }
  else {
    condition_[0] = 0;
  }
}

void PassOneAssertFatalException::throwException()
{
  doFFDCDebugger(condition_);
  throw *this;
}

// PassOneNoPlanFatalException Implementation

PassOneNoPlanFatalException::PassOneNoPlanFatalException(const char *fileName,
							 UInt32 lineNum)
  : FatalException("Cannot produce a plan in optimizer pass one",
		   fileName,
		   lineNum) {}

void PassOneNoPlanFatalException::throwException()
{
  doFFDCDebugger((char *)getMsg());
  throw *this;
}

// PassOneSkippedPassTwoNoPlanFatalException
PassOneSkippedPassTwoNoPlanFatalException::
PassOneSkippedPassTwoNoPlanFatalException(const char *fileName,
					  UInt32 lineNum) :
  FatalException("Pass one skipped, but cannot produce a plan in pass two",
		 fileName,
		 lineNum) {}

void PassOneSkippedPassTwoNoPlanFatalException::throwException()
{
  doFFDCDebugger((char *)getMsg());
  throw *this;
}

// OptAssertException Implementation
OptAssertException::OptAssertException(AssertException & e, Lng32 taskCount) :
  AssertException(e),
  taskCount_(taskCount) {}

void OptAssertException::throwException()
{
  doFFDCDebugger((char *)getCondition());
  throw *this;
}

// CmpExceptionCallBack Implementation

void CmpExceptionCallBack::throwFatalException(const char *msg,
					       const char *file,
					       UInt32 line,
					       const char *stackTrace)
{ FatalException(msg, file, line, stackTrace).throwException(); }

void CmpExceptionCallBack::throwAssertException(const char *cond,
						const char *file,
						UInt32 line,
						const char *stackTrace)
{ AssertException(cond, file, line, stackTrace).throwException(); }

CmpExceptionCallBack CmpExceptionEnv::eCallBack_;

void CmpExceptionEnv::registerCallBack()
{ NAInternalError::registerExceptionCallBack(&eCallBack_); }

void CmpExceptionEnv::unRegisterCallBack()
{ NAInternalError::unRegisterExceptionCallBack(); }

