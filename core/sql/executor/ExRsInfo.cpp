/******************************************************************************
*
* File:         ExRsInfo.cpp
* Description:  A container class for Executor Result Set Info
*
* Created:      October 2005
* Language:     C++
*
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
*
******************************************************************************
*/

/*
 This class is a container to hold the following CALL statement attributes
 including:

   A pointer to the most recently used ExUdrServer instance, pushed in 
   by the UDR TCB

   A copy of the most recently used UDR server's process ID structure,
   pushed in by the UDR TCB

   A copy of the most recently used UDR handle, pushed in 
   by the UDR TCB

   Bookkeeping fields allowing us to detect when all result sets for a
   given CALL execution are closed:
   - number of RSs returned by the CALL
   - number of those RSs closed so far

   A txState_ field to track whether a transaction has been downloaded
   to the server. Methods are also provided to download, temporarily
   suspend, and release the transaction.

   A collection of RsInfo objects, each containing information related
   to a unique RS proxy statement including:
   - A pointer to the RS proxy statement instance. 
   - The index of the RS proxy statement.
   - A flag indicating whether an open has been attempted on this RS
     proxy statement.
   - A flag indicating whether this RS proxy statement has already been
     closed.
   - A flag indicating whether this RS proxy statement has been
     successfully prepared.
   - The RS proxy syntax string returned in the INVOKE reply.

 Instances of this class are allocated by
   ex_exe_stmt_globals::getResultSetInfo()

*/

#include "ExRsInfo.h"
#include "NAMemory.h"
#include "ComSmallDefs.h"
#include "ex_ex.h"
#include "UdrExeIpc.h"
#include "Statement.h"
#include "ExUdrServer.h"
#include "ex_exe_stmt_globals.h"
#include "ExUdrClientIpc.h"

#ifdef _DEBUG
#include <stdarg.h>
#define ExRsDebug0(s) ExRsPrintf((s))
#define ExRsDebug1(s,a1) ExRsPrintf((s),(a1))
#define ExRsDebug2(s,a1,a2) ExRsPrintf((s),(a1),(a2))
#define ExRsDebug3(s,a1,a2,a3) ExRsPrintf((s),(a1),(a2),(a3))
#define ExRsDebug4(s,a1,a2,a3,a4) ExRsPrintf((s),(a1),(a2),(a3),(a4))
#define ExRsDebug5(s,a1,a2,a3,a4,a5) ExRsPrintf((s),(a1),(a2),(a3),(a4),(a5))
#define DisplayList() displayList()
#else
#define ExRsDebug0(s)
#define ExRsDebug1(s,a1)
#define ExRsDebug2(s,a1,a2)
#define ExRsDebug3(s,a1,a2,a3)
#define ExRsDebug4(s,a1,a2,a3,a4)
#define ExRsDebug5(s,a1,a2,a3,a4,a5)
#define DisplayList()
#endif


#define exrsinfo_assert(p, msg) if (!(p)) { NAAbort( __FILE__ , __LINE__ , msg); NAExit(-1);};
/*
 **********************************************************************
 * RsInfo functions
 **********************************************************************
*/
RsInfo::RsInfo()
  : statement_(NULL),
    openAttempted_(FALSE),
    closeAttempted_(FALSE),
    prepared_(FALSE),
    proxySyntax_(NULL)
{
}
RsInfo::~RsInfo()
{
}
/*
 **********************************************************************
 * ExRsInfo functions
 **********************************************************************
*/
ExRsInfo::ExRsInfo()
  : rsInfoArray_(collHeap()),
    txState_(IDLE),
    enterTxStream_(NULL),
    exitOrSuspendStream_(NULL),
    numReturnedByLastCall_(0),
    numClosedSinceLastCall_(0)
{
#ifdef _DEBUG
  // We enable the debug_ flag if either the UDR_DEBUG or UDR_RS_DEBUG
  // env var is set. We enable the txDebug_ flag if either debug_ is
  // enabled or the UDR_TX_DEBUG env var is set.
  debug_ = FALSE;
  txDebug_ = FALSE;
  if (getenv("UDR_RS_DEBUG") || getenv("UDR_DEBUG"))
  {
    debug_ = TRUE;
    txDebug_ = TRUE;
  }
  if (getenv("UDR_TX_DEBUG"))
  {
    txDebug_ = TRUE;
  }
#endif

  ExRsDebug1("[BEGIN constructor] %p", this);
  ExRsDebug1("[END constructor] %p", this);
}// ExRsInfo::ExRsInfo

/*
  For every RS entry deallocates memory used in the field of the entry
  and then deallocates the entry.
*/
ExRsInfo::~ExRsInfo()
{
  ExRsDebug1("[BEGIN destructor] %p", this);

  //For all RS entries in the collection deallocate
  //RS entry proxy syntax and then the RS entry
  ULng32 e = getNumEntries();
  for (ULng32 i = 1; i <= e; i++)
  {
    if(rsInfoArray_.used(i))
    {
      RsInfo * rsInfo = rsInfoArray_[i];
      if(rsInfo)
      {
        char * proxySyntax = rsInfo->getProxySyntax();
        if(proxySyntax)
        {
          collHeap()->deallocateMemory(proxySyntax);
        }
        delete rsInfo;
      }
    }
  }

  if (enterTxStream_)
    enterTxStream_->setExRsInfo(NULL);

  if (exitOrSuspendStream_)
    exitOrSuspendStream_->setExRsInfo(NULL);

  ExRsDebug1("[END destructor] %p", this);
}// ExRsInfo::~ExRsInfo

/*
  Overwrites proxy syntax in the RS entry at the given index. If
  the given index is unused then records a new RS entry with
  the given proxy sytax at the given index. Calling this function
  with index < 1 triggers assert with "invalid index". Calling 
  this function with proxySyntax = NULL triggers assert with 
  "invalid proxy syntax pointer". 
*/
void ExRsInfo::populate(ULng32 index, const char *proxySyntax)
{
  ExRsDebug2("[BEGIN populate] index=%u, proxySyntax=%s", 
    index, proxySyntax);
  exrsinfo_assert(index > 0, "ExRsInfo::populate() - invalid index");
  exrsinfo_assert(proxySyntax, 
    "ExRsInfo::populate() - invalid proxy syntax pointer");

  //Copy the proxy syntax
  Lng32 psLen = (Lng32) str_len(proxySyntax);
  char * ps = (char *) collHeap()->allocateMemory(psLen+1);
  str_cpy_all(ps, proxySyntax, psLen);
  ps[psLen] = 0;

  RsInfo * rsInfo;
  if(rsInfoArray_.used(index))
  {
    rsInfo = rsInfoArray_[index];
    exrsinfo_assert(rsInfo, "ExRsInfo::populate() - invalid rsInfo pointer");
    
    ExRsDebug1(" Entry %u exists", index);
    char * proxySyntax = rsInfo->getProxySyntax();
    if(proxySyntax)
      collHeap()->deallocateMemory(proxySyntax);
    rsInfo->setProxySyntax(ps);
  }
  else
  {
    rsInfo = new (collHeap()) RsInfo();
    rsInfo->setProxySyntax(ps);
    rsInfoArray_.insertAt(index,rsInfo);
  }

  numReturnedByLastCall_++;

  ExRsDebug0("[END populate]");
}// ExRsInfo::populate

/*
  Records the given RS proxy statement pointer in RS entry at
  the given index. If the index is unused creates a new RS entry, 
  sets the field statement and inserts the new entry into the 
  collection. Calling this function with index < 1 triggers assert
  with "invalid index". Calling this function with statement = NULL
  triggers assert with "invalid statement pointer". 
  
*/
void ExRsInfo::bind(ULng32 index, Statement *statement)
{
  ExRsDebug2("[BEGIN bind] index=%u, statement=%p", 
    index, statement);
  exrsinfo_assert(index > 0, "ExRsInfo::bind() - invalid index");
  exrsinfo_assert(statement, 
      "ExRsInfo::bind() - invalid statement pointer");

  if(rsInfoArray_.used(index))
  {
    RsInfo * rsInfo = rsInfoArray_[index];
    exrsinfo_assert(rsInfo, "ExRsInfo::bind() - invalid rsInfo pointer");
    
    ExRsDebug2(" Entry %u exists and has stmt pointer %p",
               index, rsInfo->getStatement());
    rsInfo->setStatement(statement);
    rsInfo->setPrepared(FALSE);
  }
  else
  {
    ExRsDebug2(" About to add index=%u, statement=%p",
               index, statement);
    RsInfo *rsInfo = new (collHeap()) RsInfo();
    rsInfo->setStatement(statement);
    rsInfoArray_.insertAt(index,rsInfo);
  }

  ExRsDebug0("[END bind]");
}// ExRsInfo::bind

/*
  Removes an RS proxy statement pointer from the first RS entry
  that contains the given statement pointer value. Calling this 
  function with statement = NULL triggers assert with
  "invalid statement pointer". Triggers assert with "unknown statement"
  if the given statement was not found.
*/
void ExRsInfo::unbind(Statement *statement)
{
  ExRsDebug1("[BEGIN unbind] statement=%p", statement);
  exrsinfo_assert(statement, 
      "ExRsInfo::unbind() - invalid statement pointer");
  NABoolean found = FALSE;
  ULng32 e = getNumEntries();
  for (ULng32 i = 1; i <= e; i++)
  {
    if(rsInfoArray_.used(i))
    {
      RsInfo * rsInfo = rsInfoArray_[i];
      if(rsInfo && rsInfo->getStatement() == statement)
      {
        rsInfo->setStatement(NULL);
        rsInfo->setPrepared(FALSE);
        found = TRUE;
        break;
      }
    }
  }

  if (!found)
  {
    ExRsDebug0("  NOT FOUND");
    exrsinfo_assert(found, "ExRsInfo::unbind() - unknown statement");
  }

  ExRsDebug1("[END unbind] found=%s", (found ? "TRUE" : "FALSE"));
}// ExRsInfo::unbind

/*
  Sets the "open attempted" field to TRUE in the RS entry at the
  given index. Calling this function with an index < 1 triggers
  assert with "invalid index". Triggers assert with "unknown index"
  if the given index is unused.
*/
void ExRsInfo::setOpenAttempted(ULng32 index)
{
  ExRsDebug1("[BEGIN setOpenAttempted] index %u", index);
  exrsinfo_assert(index > 0,
      "ExRsInfo::setOpenAttempted() - invalid index");
  NABoolean found = FALSE;
  if(rsInfoArray_.used(index))
  {
    RsInfo * rsInfo = rsInfoArray_[index];
    if(rsInfo)
    {
      rsInfo->setOpenAttempted(TRUE);
      found = TRUE;
    }
  }

  if (!found)
  {
    ExRsDebug0("  NOT FOUND");
    exrsinfo_assert(found, "ExRsInfo::setOpenAttempted() - unknown index");
  }

  ExRsDebug1("[END setOpenAttempted] found=%s", (found ? "TRUE" : "FALSE"));
}// ExRsInfo::setOpenAttempted

/*
  Sets the "close attempted" field to TRUE in the RS entry at the
  given index. Calling this function with an index < 1 triggers
  assert with "invalid index". Triggers assert with "unknown index"
  if the given index is unused.
*/
void ExRsInfo::setCloseAttempted(ULng32 index)
{
  ExRsDebug1("[BEGIN setCloseAttempted] index %u", index);
  exrsinfo_assert(index > 0,
      "ExRsInfo::setCloseAttempted() - invalid index");
  NABoolean found = FALSE;
  if(rsInfoArray_.used(index))
  {
    RsInfo * rsInfo = rsInfoArray_[index];
    if(rsInfo)
    {
      // If this RS has been opened and is now being closed for the
      // first time, then increment the count of closed RSs
      if (rsInfo->getOpenAttempted() && !rsInfo->getCloseAttempted())
        numClosedSinceLastCall_++;

      rsInfo->setCloseAttempted(TRUE);
      found = TRUE;
    }
  }

  if (!found)
  {
    ExRsDebug0("  NOT FOUND");
    exrsinfo_assert(found, "ExRsInfo::setCloseAttempted() - unknown index");
  }

  ExRsDebug1("[END setCloseAttempted] found=%s", (found ? "TRUE" : "FALSE"));
}// ExRsInfo::setCloseAttempted

/*
  Sets the "prepared" field to TRUE in the RS entry at the 
  the given index. Calling this function with an index < 1
  triggers assert with "invalid index". Triggers assert with "unknown 
  index" if the given index is unused.
*/
void ExRsInfo::setPrepared(ULng32 index)
{
  ExRsDebug1("[BEGIN setPrepared] index %u", index);
  exrsinfo_assert(index > 0,
      "ExRsInfo::setPrepared() - invalid index");
  NABoolean found = FALSE;
  if(rsInfoArray_.used(index))
  {
    RsInfo * rsInfo = rsInfoArray_[index];
    if(rsInfo)
    {
      rsInfo->setPrepared(TRUE);
      found = TRUE;
    }
  }

  if (!found)
  {
    ExRsDebug0("  NOT FOUND");
    exrsinfo_assert(found, "ExRsInfo::setPrepared() - unknown index");
  }

  ExRsDebug1("[END setPrepared] found=%s", (found ? "TRUE" : "FALSE"));
}// ExRsInfo::setPrepared

/*
  Sets fields in all RS entries to the "populate" state.
*/
void ExRsInfo::reset()
{
  ExRsDebug0("[BEGIN reset]");
  ExRsDebug0("BEFORE");
  DisplayList();
  ULng32 e = getNumEntries();
  for (ULng32 i = 1; i <= e; i++)
  {
    if(rsInfoArray_.used(i))
    {
      RsInfo * rsInfo = rsInfoArray_[i];
      if(rsInfo)
      {
        rsInfo->setOpenAttempted(FALSE);
        rsInfo->setCloseAttempted(FALSE);
        rsInfo->setPrepared(FALSE);
        char * proxySyntax = rsInfo->getProxySyntax();
        if(proxySyntax)
        {
          collHeap()->deallocateMemory(proxySyntax);
          rsInfo->setProxySyntax(NULL);
        }
      }
    }
  }

  numReturnedByLastCall_ = 0;
  numClosedSinceLastCall_ = 0;

  ExRsDebug0("AFTER");
  DisplayList();
  ExRsDebug0("[END reset]");
}// ExRsInfo::reset

/*
  Retrieves the RS entry in the given index and returns TRUE if the  
  statement field is not NULL, FALSE otherwise. Calling this function
  with an index < 1 triggers assert with "invalid index". 
*/
NABoolean ExRsInfo::statementExists(ULng32 index) const
{
  ExRsDebug1("[BEGIN exists] index %u", index);
  exrsinfo_assert(index > 0, 
      "ExRsInfo::exists() - invalid index");
  NABoolean found = FALSE;
  if(rsInfoArray_.used(index))
  {
    RsInfo * rsInfo = rsInfoArray_[index];
    if(rsInfo && rsInfo->getStatement())
      found = TRUE;
  }
  ExRsDebug1("[END exists] found=%s", (found ? "TRUE" : "FALSE"));
  return found;
}// ExRsInfo::statementExists

/*
  Returns the value in the "open attempted" field of the RS entry 
  at the given index. Calling this function with an index < 1 
  triggers assert with "invalid index". Triggers assert with 
  "unknown index" if the given index is not found. 
*/
NABoolean ExRsInfo::openAttempted(ULng32 index) const
{
  ExRsDebug1("[BEGIN openAttempted] index %u", index);
  exrsinfo_assert(index > 0, 
    "ExRsInfo::openAttempted() - invalid index");
  NABoolean found = FALSE;
  NABoolean openAttempted = FALSE;
  if(rsInfoArray_.used(index))
  {
    RsInfo * rsInfo = rsInfoArray_[index];
    if(rsInfo)
    {
      openAttempted = rsInfo->getOpenAttempted();
      found = TRUE;
    }
  }

  if (!found)
  {
    ExRsDebug0("  NOT FOUND");
    exrsinfo_assert(found, "ExRsInfo::openAttempted() - unknown index");
  }

  ExRsDebug2("[END openAttempted] found=%s, openAttempted=%s", 
             (found ? "TRUE" : "FALSE"), (openAttempted ? "TRUE" : "FALSE"));

  return openAttempted;
}// ExRsInfo::openAttempted

/*
  Returns the value in the "close attempted" field of the RS entry 
  at the given index. Calling this function with an index < 1 
  triggers assert with "invalid index". Triggers assert with 
  "unknown index" if the given index is not found. 
*/
NABoolean ExRsInfo::closeAttempted(ULng32 index) const
{
  ExRsDebug1("[BEGIN closeAttempted] index %u", index);
  exrsinfo_assert(index > 0, 
    "ExRsInfo::closeAttempted() - invalid index");
  NABoolean found = FALSE;
  NABoolean closeAttempted = FALSE;
  if(rsInfoArray_.used(index))
  {
    RsInfo * rsInfo = rsInfoArray_[index];
    if(rsInfo)
    {
      closeAttempted = rsInfo->getCloseAttempted();
      found = TRUE;
    }
  }

  if (!found)
  {
    ExRsDebug0("  NOT FOUND");
    exrsinfo_assert(found, "ExRsInfo::closeAttempted() - unknown index");
  }

  ExRsDebug2("[END closeAttempted] found=%s, closeAttempted=%s", 
             (found ? "TRUE" : "FALSE"), (closeAttempted ? "TRUE" : "FALSE"));

  return closeAttempted;
}// ExRsInfo::closeAttempted

/*
  Returns the value in the "prepared" field of an RS entry at
  the given index. Calling this function with an index < 1 triggers
  assert with "invalid index". Triggers assert with "unknown index"
  if the given index is unused. 
*/
NABoolean ExRsInfo::isPrepared(ULng32 index) const
{
  ExRsDebug1("[BEGIN isPrepared] index %u", index);
  exrsinfo_assert(index > 0, 
    "ExRsInfo::isPrepared() - invalid index");
  NABoolean found = FALSE;
  NABoolean prepared = FALSE;
  if(rsInfoArray_.used(index))
  {
    RsInfo * rsInfo = rsInfoArray_[index];
    if(rsInfo)
    {
      prepared = rsInfo->isPrepared();
      found = TRUE;
    }
  }

  if (!found)
  {
    ExRsDebug0("  NOT FOUND");
    exrsinfo_assert(found, "ExRsInfo::isPrepared() - unknown index");
  }

  ExRsDebug2("[END isPrepared] found=%s, prepared=%s", 
             (found ? "TRUE" : "FALSE"), (prepared ? "TRUE" : "FALSE"));

  return prepared;
}// ExRsInfo::isPrepared

/* 
  Returns fields describing one result set entry. Returns TRUE if a
  result set does exist at the specified position, FALSE
  otherwise. Positions are one-based. Calling this method with
  positions ranging from 1 to getNumEntries() allows users of this
  class to iterate through every result set in the collection.
*/
NABoolean ExRsInfo::getRsInfo(ULng32 position,    // IN
                              Statement *&statement,     // OUT
                              const char *&proxySyntax,  // OUT
                              NABoolean &openAttempted,  // OUT
                              NABoolean &closeAttempted, // OUT
                              NABoolean &isPrepared)     // OUT
 const
{
  ExRsDebug1("[BEGIN getRsInfo] position=%u", position);
  exrsinfo_assert(position > 0, 
    "ExRsInfo::getRsInfo() - invalid position");
  NABoolean found= FALSE;
  if(rsInfoArray_.used(position))
  {
    RsInfo * rsInfo = rsInfoArray_[position];
    if(rsInfo)
    {
      statement = rsInfo->getStatement();
      proxySyntax = rsInfo->getProxySyntax();
      openAttempted = rsInfo->getOpenAttempted();
      closeAttempted = rsInfo->getCloseAttempted();
      isPrepared = rsInfo->isPrepared();
      found = TRUE;
    }
  }
  ExRsDebug1("[END getRsInfo] found=%s", (found ? "TRUE" : "FALSE"));
  return found;
}// ExRsInfo::getRsInfo

/*
  Returns the value in the index field of an RS entry where
  the statement field matches the given statement. Calling this
  function with an empty statement pointer triggers assert with 
  "invalid statement".Triggers assert with "unknown statement" if the
  given statement is not found. 
*/
ULng32 ExRsInfo::getIndex(Statement *statement) const
{
  ExRsDebug1("[BEGIN getIndex] statement=%p", statement);
  exrsinfo_assert(statement, "ExRsInfo::getIndex() - invalid statement");
  NABoolean found = FALSE;
  ULng32 index = 0;
  ULng32 e = getNumEntries();
  for (ULng32 i = 1; i <= e; i++)
  {
    if(rsInfoArray_.used(i))
    {
      RsInfo * rsInfo = rsInfoArray_[i];
      if(rsInfo && rsInfo->getStatement() == statement)
      {
        index = i;
        found = TRUE;
        break;
      }
    }
  }

  if (!found)
  {
    ExRsDebug0("  NOT FOUND");
    exrsinfo_assert(found, "ExRsInfo::getIndex() - unknown statement");
  }

  ExRsDebug1("[END getIndex] found=%s", (found ? "TRUE" : "FALSE"));
  return index;
}// ExRsInfo::getIndex

/*
  Ensures enough space for the maximum number of RS entries.

  If the user of this object calls setNumEntries(N) to set the max,
  and we decide to resize the NAArray, the NAArray may actually raise
  the physical limit internally and getNumEntries() may return a value
  higher than N.
*/
void ExRsInfo::setNumEntries(ComUInt32 maxEntries)
{
  if(getNumEntries() < maxEntries)
    rsInfoArray_.resize(maxEntries + 1);
}

/*
  Records a copy of the most recently used IpcProcessId
*/
void ExRsInfo::setIpcProcessId(const IpcProcessId &ipcProcessId)
{
#ifdef _DEBUG
  char buf[300];
  ipcProcessId.toAscii(buf, 300);
  ExRsDebug1("[BEGIN setIpcProcessId] ipcProcessId=%s", buf);
#endif
  ipcProcessId_ = ipcProcessId;
  ExRsDebug0("[END setIpcProcessId]");
}// ExRsInfo::setIpcProcessId

#ifdef _DEBUG
void ExRsInfo::displayList() const  
{
  ULng32 e = getNumEntries();
  if(e > 0)
  {
    for (ULng32 i = 1; i <= e; i++)
    {
      if(rsInfoArray_.used(i))
      {
        NABoolean openAttempted = rsInfoArray_[i]->getOpenAttempted();
        NABoolean closeAttempted = rsInfoArray_[i]->getCloseAttempted();
        NABoolean prepared = rsInfoArray_[i]->isPrepared();

        ExRsDebug1("  rsEntry[%u]", i);
        ExRsDebug1("    statement=%p", rsInfoArray_[i]->getStatement());
        ExRsDebug1("    openAttempted=%s", openAttempted ? "TRUE" : "FALSE");
        ExRsDebug1("    closeAttempted=%s", closeAttempted ? "TRUE" : "FALSE");
        ExRsDebug1("    prepared=%s", (prepared ? "TRUE" : "FALSE"));
        ExRsDebug1("    proxySyntax=%s", rsInfoArray_[i]->getProxySyntax());
      }
      else
        ExRsDebug0("    unused RS entry");
    }
  }
  else
    ExRsDebug0("  list is empty");
}// ExRsInfo::displayList()

void ExRsInfo::ExRsPrintf(const char *formatString, ...) const
{
  if (!debugEnabled())
    return;

  FILE *f = stdout;
  va_list args;
  va_start(args, formatString);
  fprintf(f, "[RS] ");
  vfprintf(f, formatString, args);
  fprintf(f, "\n");
  fflush(f);
}// ExRsInfo::ExRsPrintf
#endif

// This method brings the UDR server into the transaction for this
// statement by sending an ENTER TX request. The stmtGlobals object
// passed in will have its diags area populated if IPC errors are
// encountered.
void ExRsInfo::enterUdrTx(ExExeStmtGlobals &stmtGlobals)
{
#ifdef _DEBUG
  NABoolean oldDebug = debug_;
  if (txDebug_)
    debug_ = TRUE;
  ExRsDebug3("[BEGIN enterUdrTx] %p, state %s, handle "
             INT64_PRINTF_SPEC,
             this, getTxStateString(txState_), udrHandle_);
#endif
  
  if (txState_ != ACTIVE)
  {
    sendTxMessage(stmtGlobals, ENTER);
  }
  
#ifdef _DEBUG
  ExRsDebug2("[END enterUdrTx] %p, state %s",
             this, getTxStateString(txState_));
  debug_ = oldDebug;
#endif
}

// This method temporarily brings the UDR server out of the
// transaction for this statement by sending a SUSPEND TX request. The
// stmtGlobals object passed in will have its diags area populated if
// IPC errors are encountered.
void ExRsInfo::suspendUdrTx(ExExeStmtGlobals &stmtGlobals)
{
#ifdef _DEBUG
  NABoolean oldDebug = debug_;
  if (txDebug_)
    debug_ = TRUE;
  ExRsDebug3("[BEGIN suspendUdrTx] %p, state %s, handle "
             INT64_PRINTF_SPEC,
             this, getTxStateString(txState_), udrHandle_);
#endif

  if (txState_ == ACTIVE)
  {
    sendTxMessage(stmtGlobals, SUSPEND);
  }

#ifdef _DEBUG
  ExRsDebug2("[END suspendUdrTx] %p, state %s",
             this, getTxStateString(txState_));
  debug_ = oldDebug;
#endif
}

// This method brings the UDR server out of the transaction for this
// statement by sending an EXIT TX request. The stmtGlobals object
// passed in will have its diags area populated if IPC errors are
// encountered.
void ExRsInfo::exitUdrTx(ExExeStmtGlobals &stmtGlobals)
{
#ifdef _DEBUG
  NABoolean oldDebug = debug_;
  if (txDebug_)
    debug_ = TRUE;
  ExRsDebug3("[BEGIN exitUdrTx] %p, state %s, handle "
             INT64_PRINTF_SPEC,
             this, getTxStateString(txState_), udrHandle_);
#endif

  if (txState_ != IDLE)
  {
    // Before sending UdrExitTxMsg, need to make sure that the txState_ is  
    // ACTIVE. The enterUdrTx() will send UdrENTERTxMsg in non ACTIVE case.
    // This change is required for Solution 10-070125-2076.

    enterUdrTx(stmtGlobals);
    sendTxMessage(stmtGlobals, EXIT);
  }

#ifdef _DEBUG
  ExRsDebug2("[END exitUdrTx] %p, state %s",
             this, getTxStateString(txState_));
  debug_ = oldDebug;
#endif
}

// A private method to do the IPC for TX requests
void ExRsInfo::sendTxMessage(ExExeStmtGlobals &stmtGlobals, TxMsgType t)
{
  exrsinfo_assert(udrServer_, "Cannot send msgs without a valid ExUdrServer");
  
  IpcEnvironment *ipcEnv = stmtGlobals.getIpcEnvironment();
  exrsinfo_assert(ipcEnv, "Cannot send msgs without a valid IPC environment");
  
  NAMemory *h = ipcEnv->getHeap();
  exrsinfo_assert(h, "Cannot send msgs without a valid IPC heap");
  
  // We will only send this message if the server is still running and
  // has the same process ID as the one recorded earlier. That might
  // not be the case if the connection has gone away or reached an
  // error state.
  //
  // If we don't detect a process ID match then there is no point in
  // sending more messages and we change the TX state of this instance
  // to IDLE.
  //
  // Error reporting for these scenarios is handled by other layers
  // (TCBs and stream callbacks).

  IpcConnection *conn = stmtGlobals.castToExMasterStmtGlobals()
                         ->getUdrConnection();
  ExRsDebug1("  IPC Connection is %p", conn);

  if (conn == NULL)
  {
    ExRsDebug0("  Connection is NULL, no msg will be sent");

    txState_ = IDLE;
    return;
  }

  const IpcProcessId &otherEnd = conn->getOtherEnd();

  if (!(ipcProcessId_ == otherEnd) ||
      (udrServer_->getState() == ExUdrServer::EX_UDR_BROKEN))
  {
#ifdef _DEBUG
    if (!(ipcProcessId_ == otherEnd))
    {
      char buf1[300];
      char buf2[300];
      ipcProcessId_.toAscii(buf1, 300);
      otherEnd.toAscii(buf2, 300);
      ExRsDebug0("  IPC process ID mismatch, no msg will be sent");
      ExRsDebug2("  old %s, new %s", buf1, buf2);
    }
    else
    {
      ExRsDebug0("  ExUdrServer is in BROKEN state, no msg will be sent");
    }
#endif

    // If we fail to send an EXIT or SUSPEND request, abandon the
    // outstanding ENTER request
    if (t == EXIT || t == SUSPEND)
    {
      exrsinfo_assert(enterTxStream_,
                      "Cannot send EXIT or SUSPEND when ENTER stream is NULL");

      ExRsDebug1("  About to abandon I/O on ENTER TX stream %p",
                 enterTxStream_);
      enterTxStream_->abandonPendingIOs();
    }

    txState_ = IDLE;
    return;
  }

  if (conn->getState() == IpcConnection::ERROR_STATE)
  {
    ExRsDebug0("  Connection encountered errors, no msg will be sent");

    txState_ = IDLE;
    return;
  }

  UdrMessageObj *msg = NULL;
  TxState nextState = IDLE;
  UdrIpcObjectType msgType = UDR_IPC_INVALID;

  switch (t)
  {
    case ENTER:
    {
      msg = new (h) UdrEnterTxMsg(h);
      nextState = ACTIVE;
      msgType = UDR_MSG_ENTER_TX;
    }
    break;
    
    case SUSPEND:
    {
      msg = new (h) UdrSuspendTxMsg(h);
      nextState = SUSPENDED;
      msgType = UDR_MSG_SUSPEND_TX;
    }
    break;
    
    case EXIT:
    {
      msg = new (h) UdrExitTxMsg(h);
      nextState = IDLE;
      msgType = UDR_MSG_EXIT_TX;
    }
    break;
    
    default:
    {
      exrsinfo_assert(FALSE, "Invalid ExRsInfo::TxMsgType value");
    }
    break;
  }
  
  msg->setHandle(udrHandle_);
  
  // Send Enter Tx with transaction. Exit Tx and Suspend Tx messages
  // do not carry transaction
  Int64 &stmtTransId = stmtGlobals.getTransid();
  Int64 transIdToSend = -1; // -1 is an invalid transid
  if (t == ENTER)
    transIdToSend = stmtTransId;

  NABoolean isTransactional = (transIdToSend == -1 ? FALSE : TRUE);

  UdrClientControlStream *s = new (h)
    UdrClientControlStream(ipcEnv,
                           NULL,          // TCB pointer for callbacks
                           &stmtGlobals,  // Stmt glob pointer for callbacks
                           FALSE,         // Keep diags for caller?
                           isTransactional);

  ExRsDebug1("  Created new control stream %p", s);

  s->setMessageType(msgType);
  s->setExRsInfo(this);

  if (t == ENTER)
  {
    exrsinfo_assert(enterTxStream_ == NULL,
                    "Cannot send ENTER when stream is not NULL");
    ExRsDebug1("  Setting ENTER TX stream to %p", s);
    enterTxStream_ = s;
  }
  else if (t == EXIT || t == SUSPEND)
  {
    exrsinfo_assert(exitOrSuspendStream_ == NULL,
                    "Cannot send EXIT or SUSPEND when stream is not NULL");
    ExRsDebug1("  Setting EXIT or SUSPEND stream to %p", s);
    exitOrSuspendStream_ = s;
  }

#ifdef _DEBUG
  if (debug_)
    s->setTraceFile(stdout);
#endif

  s->addRecipient(conn);
  *s << *msg;

  if (isTransactional)
    stmtGlobals.incrementUdrTxMsgsOut();
  else
    stmtGlobals.incrementUdrNonTxMsgsOut();

  NABoolean sendWaited = FALSE;
  s->send(sendWaited, transIdToSend);

  ExRsDebug2("  %s sent on control stream %p", getTxMsgTypeString(t), s);
  ExRsDebug1("  N UDR msgs outstanding for this statement: %d",
             (Lng32) stmtGlobals.numUdrMsgsOut());
  
  msg->decrRefCount();
  txState_ = nextState;
}

void ExRsInfo::reportCompletion(UdrClientControlStream *s)
{
  ExRsDebug2("[BEGIN reportCompletion] %p, state %s",
             this, getTxStateString(txState_));
  ExRsDebug1("  stream pointer %p", s);
  ExRsDebug2("  ENTER stream %p, EXIT/SUSPEND stream %p",
             enterTxStream_, exitOrSuspendStream_);

  if (s)
  {
    if (s == enterTxStream_)
    {
      enterTxStream_ = NULL;
      
      // txState_ might be ACTIVE if an IPC error occurred while sending
      // the ENTER. Now that we know the ENTER has completed we can
      // transition to IDLE.
      if (txState_ == ACTIVE)
      {
        ExRsDebug0("  Setting txState_ to IDLE");
        txState_= IDLE;
      }
    }
    
    if (s == exitOrSuspendStream_)
    {
      exitOrSuspendStream_ = NULL;
    }
  }

  ExRsDebug2("[END reportCompletion] %p, state %s",
             this, getTxStateString(txState_));
}
