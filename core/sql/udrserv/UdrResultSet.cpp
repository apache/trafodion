/* -*-C++-*- */
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
/****************************************************************************
*
* File:         UdrResultSet.cpp
* Description:  This file contains method definitaions for
*               UdrResultSet and TmpBuffer classes.
* Created:      10/20/2005
* Language:     C++
**
*****************************************************************************
*/
#include "sqlcli.h"
#include "UdrResultSet.h"
#include "sql_buffer.h"
#include "spinfo.h"
#include "UdrDebug.h"
#include "ComSizeDefs.h"
#include "LmRoutine.h"
#include "exp_expr.h"
#include "ExpError.h"
#include "udrglobals.h"
#include "udrutil.h"
#include "SQLCLIdev.h"
#include "ExpError.h"
#include "udrdecs.h"
#include "UdrFFDC.h"
#include "ComRtUtils.h"

extern NABoolean allocateReplyRow(UdrGlobals *UdrGlob,
                                  SqlBuffer &replyBuffer,
                                  queue_index parentIndex,
                                  Int32 replyRowLen,
                                  char *&newReplyRow,
                                  ControlInfo *&newControlInfo,
                                  ex_queue::up_status upStatus);

extern NABoolean allocateErrorRow(UdrGlobals *UdrGlob,
                                  SqlBuffer &replyBuffer,
                                  queue_index parentIndex,
                                  NABoolean setDiagsFlag);

extern NABoolean allocateEODRow(UdrGlobals *UdrGlob,
                                SqlBuffer &replyBuffer,
                                queue_index parentIndex);

// Utility method to deallocate descriptor items created in
// UdrResultSet::generateProxySyntax()
static void
deleteDescItems(ComUInt32 numCols, SQLDESC_ITEM *desc_items, NAMemory *heapPtr)
{
  ComUInt32 index = 0;
  // Deallocate string values in desc_items and desc_items themselves
  for (index = 0; index < numCols; index++)
  {
    ComUInt32 startingPoint = index * NUMDESC_ITEMS;

    NADELETEBASIC(desc_items[startingPoint+0].string_val, heapPtr);
    NADELETEBASIC(desc_items[startingPoint+1].string_val, heapPtr);
    NADELETEBASIC(desc_items[startingPoint+2].string_val, heapPtr);
    NADELETEBASIC(desc_items[startingPoint+3].string_val, heapPtr);
    NADELETEBASIC(desc_items[startingPoint+4].string_val, heapPtr);
    NADELETEBASIC(desc_items[startingPoint+5].string_val, heapPtr);
  }

  heapPtr->deallocateMemory(desc_items);

  return;
}

//////////////////////////////////////////////////////////////////////////////
//
//  TmpBuffer methods
//
//////////////////////////////////////////////////////////////////////////////
char *TmpBuffer::getNextRow(ComDiagsArea* &rowDiags,
                            NABoolean singleRowFetchEnabled)
{
  char * retptr = NULL;

  if (moreRowsToCopy())
  {
    retptr = buffer_ + (index_ * fetchRowSize_);

    Int32 rowNumber = index_ + 1;
    while(cliDiags_->getNumber(DgSqlCode::WARNING_))
    {
      // We stop checking the conditions when we hit a condition whose
      // row number is more than the current row number. Until then, we
      // copy the conditions from cliDiags_ to rowDiags and delete the
      // conditions from cliDiags_.
      // Note that getWarningEntry() expects 1-based index value whereas
      // deleteWarning() takes 0-based values.
      const ComCondition *condFromCli = cliDiags_->getWarningEntry(1);
      Lng32 rowNumberFromCli = condFromCli->getRowNumber();
      if (rowNumberFromCli > rowNumber)
      {
        break;
      }
      else
      {
        // In single row fetch case, we have only one row in this
        // object and all the warnings are related to this row.
        if (rowNumberFromCli == rowNumber || singleRowFetchEnabled)
        {
          if (rowDiags == NULL)
            rowDiags = ComDiagsArea::allocate(heap_);

          UDR_ASSERT(rowDiags, "Diags Area for row warning is not present.");
          ComCondition *cond = rowDiags->makeNewCondition();
          *cond = *condFromCli;
          rowDiags->acceptNewCondition();
        }

        cliDiags_->deleteWarning(0);
        continue;
      }
    }

    index_++;

    // Reset pointers if needed
    if (numRows_ == index_)
      numRows_ = index_ = 0;
  }

  return retptr;
}

//////////////////////////////////////////////////////////////////////////////
//
// UdrResultSet Constructor/Destructor definitions
//
//////////////////////////////////////////////////////////////////////////////

// If errors occur, parameter 'd' will be populated with errors. Callers
// need to check for diagnostics
// This constructor is used on Windows Platform
UdrResultSet::UdrResultSet(SPInfo *spInfo, SQLSTMT_ID *stmt, ComDiagsArea &d)
  : spInfo_(spInfo),
    state_(RS_INITIATED),
    lmResultSet_(NULL),
    proxySyntax_(NULL),
    rsHandle_(INVALID_RS_HANDLE),
    rsColumnDesc_(NULL),
    rsColDescForLM_(NULL),
    numColumns_(0),
    exeRowSize_(0),
    bufferSize_(0),
    rowsToFetchFromJDBC_(0),
    stmt_id_(stmt),
    output_desc_(NULL),
    quad_fields_(NULL),
    parentQueueIndex_(0),
    needToCopyErrorRow_(FALSE),
    needToCopyEODRow_(FALSE),
    singleRowFetchEnabled_(FALSE),
    tmpBuffer_(NULL)
{
  UDR_ASSERT(stmt_id_ != NULL, "Result set statement id not available");

  generateProxySyntax(d);

  UDR_DEBUG1("UdrResultSet %p created.", this);
  UDR_DEBUG1("  SPInfo %p", spInfo_);
  UDR_DEBUG1("  LmResultSet %p", lmResultSet_);
  UDR_DEBUG1("  Stmt name %s", stmt_id_->identifier);
  UDR_DEBUG1("  Proxy syntax \"%s\"",
             (proxySyntax_ ? proxySyntax_->data() : ""));
}

// If errors occur, parameter 'd' will be populated with errors. Callers
// need to check for diagnostics
// This constructor is used on NSK Platform
UdrResultSet::UdrResultSet(SPInfo *spInfo, LmResultSet *lmRS, ComDiagsArea &d)
  : spInfo_(spInfo),
    state_(RS_INITIATED),
    lmResultSet_(lmRS),
    proxySyntax_(NULL),
    rsHandle_(INVALID_RS_HANDLE),
    rsColumnDesc_(NULL),
    rsColDescForLM_(NULL),
    numColumns_(0),
    exeRowSize_(0),
    bufferSize_(0),
    rowsToFetchFromJDBC_(0),
    stmt_id_(NULL),
    output_desc_(NULL),
    quad_fields_(NULL),
    parentQueueIndex_(0),
    needToCopyErrorRow_(FALSE),
    needToCopyEODRow_(FALSE),
    singleRowFetchEnabled_(FALSE),
    tmpBuffer_(NULL)
{
  UDR_ASSERT(lmResultSet_, "LM Result Set object is not set");

  if (lmResultSet_ && lmResultSet_->isCliStmtAvailable())
  {
    stmt_id_ = copyStatementID((SQLSTMT_ID *)lmResultSet_->getStmtID());
    UDR_ASSERT(stmt_id_ != NULL, "Result set statement id not available");
  }

  generateProxySyntax(d);

  UDR_DEBUG1("UdrResultSet %p created.", this);
  UDR_DEBUG1("  SPInfo %p", spInfo_);
  UDR_DEBUG1("  LmResultSet %p", lmResultSet_);
  if (stmt_id_)
    UDR_DEBUG1("  Stmt name %s", stmt_id_->identifier);
  UDR_DEBUG1("  Proxy syntax \"%s\"",
             (proxySyntax_ ? proxySyntax_->data() : ""));
}

UdrResultSet::~UdrResultSet()
{
  deallocateUDRGeneratedFields();

  // Cleanup LmResultSet object
  if (lmResultSet_)
    getSPInfo()->getLMHandle()->cleanupLmResultSet(lmResultSet_);

  lmResultSet_ = NULL;

  deallocateExeGeneratedFields();
} // UdrResultSet::~UdrResultSet

//////////////////////////////////////////////////////////////////////////////
//
// UdrResultSet method definitions
//
//////////////////////////////////////////////////////////////////////////////

void
UdrResultSet::deallocateUDRGeneratedFields()
{
  SQLCTX_HANDLE prevContext = 0;

  if (getContextHandle() != 0)
    SQL_EXEC_SwitchContext((Lng32) getContextHandle(), &prevContext);


  NAMemory *heap = collHeap();

  if (stmt_id_)
  {
    heap->deallocateMemory((void*)stmt_id_->module->module_name);
    heap->deallocateMemory((void*)stmt_id_->module->charset);
    heap->deallocateMemory((void*)stmt_id_->module);
    heap->deallocateMemory((void*)stmt_id_->identifier);
    heap->deallocateMemory((void*)stmt_id_->charset);
    heap->deallocateMemory(stmt_id_);
  }

  if (output_desc_)
  {
    SQL_EXEC_DeallocDesc(output_desc_);
    heap->deallocateMemory((void*)output_desc_->module);
    heap->deallocateMemory(output_desc_);
  }

  heap->deallocateMemory(quad_fields_);
  NADELETE(tmpBuffer_, TmpBuffer, collHeap());
  NADELETE(proxySyntax_, NAString, collHeap());
  heap->deallocateMemory(rsColDescForLM_);

  rowsToFetchFromJDBC_ = 0;
  stmt_id_ = NULL;
  output_desc_ = NULL;
  proxySyntax_ = NULL;
  quad_fields_ = NULL;
  tmpBuffer_ = NULL;
  rsColDescForLM_ = NULL;

  parentQueueIndex_ = 0;
  needToCopyErrorRow_ = FALSE;
  needToCopyEODRow_ = FALSE;
  singleRowFetchEnabled_ = FALSE;

  if (prevContext != 0)
    SQL_EXEC_SwitchContext_Internal(prevContext, NULL,TRUE);

  // Let's clear cli diags if there are any
  SQL_EXEC_ClearDiagnostics(NULL);
  return;
} // UdrResultSet::deallocateUDRGeneratedFields()

// Resets all the fields that come from executor in RS_LOAD message
void
UdrResultSet::deallocateExeGeneratedFields()
{
  rsHandle_ = INVALID_RS_HANDLE;
  numColumns_ = 0;
  exeRowSize_ = 0;
  bufferSize_ = 0;

  deallocateColumnDesc();

} // UdrResultSet::deallocateExeGeneratedFields

const char *
UdrResultSet::stateString()
{
  switch (state_)
  {
    case RS_INITIATED: return "RS_INITIATED";
    case RS_REINITIATED: return "RS_REINITIATED";
    case RS_LOADED: return "RS_LOADED";
    case RS_UNLOADED: return "RS_UNLOADED";
    case RS_FETCH: return "RS_FETCH";
    case RS_FETCH_COMPLETE: return "RS_FETCH_COMPLETE";
    case RS_CLOSED: return "RS_CLOSED";
    case RS_EARLY_CLOSE: return "RS_EARLY_CLOSE";
    default: return ComRtGetUnknownString((Int32) state_);
  }
}

// Reinitiliazes the UdrResultSet object with the new LmResultSet object.
// If lmRS param is NULL, the stmt_id param is used for initialization.
// This method is used on both Windows and NSK platforms. On NSK, lmRS param
// will have a valid value. On Windows, stmt_id will have a valid value.
//
// Returns 0 for success
//         -1 for error
Int32
UdrResultSet::reInit(LmResultSet *lmRS, ComDiagsArea &d, SQLSTMT_ID *stmt_id)
{
  state_ = RS_REINITIATED;

  lmResultSet_ = lmRS;

  if (lmRS != NULL)
  {
    if (lmResultSet_->isCliStmtAvailable())
    {
      // NSK; and RS is using T2 connection
      stmt_id_ = copyStatementID((SQLSTMT_ID*)lmResultSet_->getStmtID());
      UDR_ASSERT(stmt_id_ != NULL, "Result set statement id not available");
    }
  }
  else
  {
    // Windows
    stmt_id_ = stmt_id;
    UDR_ASSERT(stmt_id_ != NULL, "Result set statement id not available");
  }

  if (generateProxySyntax(d) != 0)
    return -1;

  UDR_DEBUG1("UdrResultSet %p reinitialized.", this);
  UDR_DEBUG1("  SPInfo %p", spInfo_);
  UDR_DEBUG1("  LmResultSet %p", lmResultSet_);
  if (stmt_id_)
    UDR_DEBUG1("  Stmt name %s", stmt_id_->identifier);
  UDR_DEBUG1("  Proxy syntax \"%s\"",
             (proxySyntax_ ? proxySyntax_->data() : ""));

  return 0;
} // UdrResultSet::reInit()

// prepareForReinvoke() deallocates all the fields that will be regenerated
// by UDR invocation.
void
UdrResultSet::prepareForReinvoke()
{
  deallocateUDRGeneratedFields();

  // Cleanup LmResultSet object
  if (lmResultSet_)
    getSPInfo()->getLMHandle()->cleanupLmResultSet(lmResultSet_);

  lmResultSet_ = NULL;

  return;
} // UdrResultSet::prepareForReinvoke()


// Loads column description and other result set row details
// with the information from master executor.
NABoolean
UdrResultSet::load(RSHandle handle,
                   ComUInt32 numRSCols,
                   ComUInt32 rowSize,
                   ComUInt32 bufferSize,
                   UdrParameterInfo *columnDesc,
                   ComDiagsArea &d)
{
  // State should be RS_INITIATED or RS_REINITIATED or RS_UNLOADED
  if (!isInited() && !isReInited() && !isUnloaded())
  {
    d << DgSqlCode(-UDR_ERR_INVALID_RS_STATE)
      << DgString0("Load")
      << DgString1(stateString());

    return FALSE;
  }

  rsHandle_ = handle;
  numColumns_ = numRSCols;
  exeRowSize_ = rowSize;
  bufferSize_ = bufferSize;

  populateColumnDesc(columnDesc);

  state_ = RS_LOADED;

  UdrGlobals *udrGlob = getSPInfo()->getUdrGlobals();
  if (udrGlob->verbose_ &&
      udrGlob->traceLevel_ >= TRACE_DETAILS &&
      udrGlob->showRSLoad_)
  {
    ServerDebug("  UdrResultSet %p loaded ", this);
    ServerDebug("    SPInfo %p", spInfo_);
    ServerDebug("    LmResultSet %p", lmResultSet_);
    if (stmt_id_)
      ServerDebug("    Stmt name %s", stmt_id_->identifier);
    ServerDebug("    Proxy syntax \"%s\"",
                (proxySyntax_ ? proxySyntax_->data() : ""));

    UdrParameterInfo *pi;
    for (ComUInt32 ind = 0; ind < numColumns_; ind++)
    {
      ServerDebug(" ");
      ServerDebug("  Result Set column# %d: ", ind);
      pi = &rsColumnDesc_[ind];
      pi->display(UdrTraceFile, 4, pi);
    }
  }

  return TRUE;
}  // UdrResultSet::load()

// Makes a copy of SQLSTMT_ID struct
SQLSTMT_ID *UdrResultSet::copyStatementID(SQLSTMT_ID *srcStmtID,
                                          NABoolean resetStmtInfo)
{
  if (srcStmtID == NULL)
    return NULL;

  // SQLSTMT_ID has three pointers fields: module, identifier and charset.
  // identifier is not null terminated. It's length is stored in
  // identifier_len field. charset is null terminated string.
  SQLSTMT_ID *stmt_id = new (collHeap()) SQLSTMT_ID;

  // Copy all primitive fields
  stmt_id->version = srcStmtID->version;
  stmt_id->name_mode = srcStmtID->name_mode;
  stmt_id->identifier_len = srcStmtID->identifier_len;

  if (resetStmtInfo && stmt_id->name_mode != stmt_handle)
  {
    // We may be copying a SQLSTMT_ID created by the JDBC driver. That
    // structure may be associated with a StatementInfo object inside
    // the CLI that contains pointers to the descriptors JDBC is using
    // for this statement. We don't want to use those same descriptors
    // here in our MXUDR code. We don't even know if they are still
    // valid. We need to break the association between this new
    // SQLSTMT_ID and the StatementInfo. This is done by setting the
    // handle field to 0 for all name modes other than
    // stmt_handle. For the stmt_handle mode, StatementInfo
    // associations are not maintained.
    stmt_id->handle = 0;
  }
  else
  {
    stmt_id->handle = srcStmtID->handle;
  }

  stmt_id->tag = srcStmtID->tag;

  // Make a copy of SQLMODULE_ID
  // SQLMODULE_ID has two pointer fields: module_name (whose length is in
  // module_name_len) and charset (which is null terminated)
  SQLMODULE_ID *module_id = new (collHeap()) SQLMODULE_ID;
  const SQLMODULE_ID *srcModuleID = srcStmtID->module;
  module_id->version = srcModuleID->version;
  module_id->module_name_len = srcModuleID->module_name_len;
  char *module_name = new (collHeap()) char[srcModuleID->module_name_len];
  str_cpy_all(module_name, srcModuleID->module_name,
              srcModuleID->module_name_len);
  module_id->module_name = (const char *) module_name;

  ComUInt32 module_charset_len = str_len(srcModuleID->charset);
  char *module_charset = new (collHeap()) char[module_charset_len + 1];
  str_cpy_all(module_charset, srcModuleID->charset, (Lng32) module_charset_len);
  module_charset[module_charset_len] = '\0';
  module_id->charset = (const char *) module_charset;

  stmt_id->module = module_id;

  // Copy identifier field
  char *identifier = new (collHeap()) char[srcStmtID->identifier_len];
  str_cpy_all(identifier, srcStmtID->identifier, srcStmtID->identifier_len);
  stmt_id->identifier = (const char *) identifier;

  // Copy charset field
  ComUInt32 stmt_charset_len = str_len(srcStmtID->charset);
  char *stmt_charset = new (collHeap()) char[stmt_charset_len + 1];
  str_cpy_all(stmt_charset, srcStmtID->charset, (Lng32) stmt_charset_len);
  stmt_charset[stmt_charset_len] = '\0';
  stmt_id->charset = (const char *) stmt_charset;

  return stmt_id;
} // udrResultSet::copyStatementID

// Sets RS's context and returns the current context handle in oldCtx
// Returns 0 for success and other values for failure
Int32
UdrResultSet::setContext(SQLCTX_HANDLE &oldCtx, ComDiagsArea &d)
{
  // Set the return value, oldCtx, to 0
  oldCtx = 0;

  // No need for switching context when there is no lmResultSet_ object
  // or when there is no context handle for this UdrResultSet.
  // The first thing can happen when there is no JDBC involved in UDR Server
  // such as on Windows platform. In this case, we will have only one context.
  // Second case is when Type 4 driver is used.
  if (lmResultSet_ == NULL || getContextHandle() == 0)
    return 0;

  SQLCTX_HANDLE tmpCtx;
  Int32 result = SQL_EXEC_SwitchContext((Lng32) getContextHandle(),
                                        &tmpCtx);

  if (result < 0)
  {
    d << DgSqlCode(-UDR_ERR_INTERNAL_CLI_ERROR)
      << DgString0("SQL_EXEC_SwitchContext")
      << DgInt0(result);

    return result;
  }

  oldCtx = tmpCtx;
  return 0;
} // UdrResultSet::setContext

Int32
UdrResultSet::resetContext(SQLCTX_HANDLE ctxHandle, ComDiagsArea &d)
{
  if (lmResultSet_ == NULL || getContextHandle() == 0)
    return 0;

  SQLCTX_HANDLE tmpCtxHandle;
  Int32 result = SQL_EXEC_SwitchContext_Internal(ctxHandle, &tmpCtxHandle,TRUE);

  if (result < 0)
  {
    d << DgSqlCode(-UDR_ERR_INTERNAL_CLI_ERROR)
      << DgString0("SQL_EXEC_SwitchContext")
      << DgInt0(result);

    return result;
  }

  UDR_ASSERT(tmpCtxHandle == (SQLCTX_HANDLE) getContextHandle(),
             "Wrong context Handle.");

  return result;
} // UdrResultSet::resetContext

// populates proxySyntax_ field
Int32
UdrResultSet::generateProxySyntax(ComDiagsArea &d)
{
  Lng32 retcode = 0;
  ComUInt32 index = 0;

  if (lmResultSet_ && ! lmResultSet_->isCliStmtAvailable())
  {
    // RS is using T4 connection.
    proxySyntax_ = new (collHeap()) NAString(lmResultSet_->getProxySyntax());
    return retcode;
  }


  if (state_ != RS_INITIATED && state_ != RS_REINITIATED)
  {
    // 'Initiate' is not a request that can be asked by UDR server
    // clients. It's an internal request for UDR server.
    d << DgSqlCode(-UDR_ERR_INVALID_RS_STATE)
      << DgString0("Initiate")
      << DgString1(stateString());

    return -1;
  }

  SQLCTX_HANDLE prevContext = 0;
  if (setContext(prevContext, d) != 0)
    return -1;

  if (stmt_id_ == NULL)
  {
    if (lmResultSet_ == NULL)
      d << DgSqlCode(-UDR_ERR_INTERNAL_ERROR)
        << DgString0("LmResultSet object is not set");
    else
      stmt_id_ = copyStatementID((SQLSTMT_ID*)lmResultSet_->getStmtID());
  }

  UDR_ASSERT(stmt_id_ != NULL, "NULL statement id for result set");

  // Allocate an output descriptor where column information
  // will be stored
  output_desc_ = new (collHeap()) SQLDESC_ID;
  SQLMODULE_ID *outmodule = new (collHeap()) SQLMODULE_ID;
  output_desc_->name_mode = desc_handle;
  output_desc_->identifier = 0;
  output_desc_->handle = 0;
  output_desc_->module = outmodule;
  outmodule->module_name =0;

  retcode = SQL_EXEC_AllocDesc(output_desc_, 500);
  if (retcode < 0)
  {
    d << DgSqlCode(-UDR_ERR_INTERNAL_CLI_ERROR)
      << DgString0("SQL_EXEC_AllocDesc")
      << DgInt0(retcode);

    return -1;
  }

  // Describe Statement.
  retcode = SQL_EXEC_DescribeStmt(stmt_id_, NULL, output_desc_);
  if (retcode < 0)
  {
    d << DgSqlCode(-UDR_ERR_INTERNAL_CLI_ERROR)
      << DgString0("SQL_EXEC_DescribeStmt")
      << DgInt0(retcode);

    return -1;
  }

  // Check how many columns there are in the output_desc
  ComUInt32 numColumns = 0;
  retcode = SQL_EXEC_GetDescEntryCount(output_desc_, (Lng32*) &numColumns);
  if (retcode < 0)
  {
    d << DgSqlCode(-UDR_ERR_INTERNAL_CLI_ERROR)
      << DgString0("SQL_EXEC_GetDescEntryCount")
      << DgInt0(retcode);

    return -1;
  }

  // The CLI statement does not have any output columns. This situation
  // may be rare but quite possible on windows because of our hack to
  // produce RS on windows.
  if (numColumns <= 0)
  {
    d << DgSqlCode(-UDR_ERR_INVALID_RS_COLUMN_COUNT)
      << DgInt0(numColumns);

    return -1;
  }

  // We want NUMDESC_ITEMS number of different information
  // for each column. desc_items will have room for
  // (numColumns * NUMDESC_ITEMS) items. The information is arranged
  // as  (C1.I1), (C1.I2), ... (C1.Im),
  //     (C2.I1), (C2.I2), ... (C2.Im)
  //     ... ...
  //     (Cn.I1), (Cn.I2), ... (Cn.Im) and so on
  // for numColumns = n and NUMDESC_ITEMS = m.
  SQLDESC_ITEM *desc_items =
    new (collHeap()) SQLDESC_ITEM[numColumns * NUMDESC_ITEMS];

  for (index = 0; index < numColumns; index++)
  {
    ComUInt32 startingPoint = index * NUMDESC_ITEMS;

    // We are allocating ComMAX_ANSI_IDENTIFIER_EXTERNAL_LEN size of
    // sqldesc_catalog_name, sqldesc_schema_name, sqldesc_table_name
    // sqldesc_name, sqldesc_text_format and sqldesc_heading.
    // TBD: Not sure if it is okay for sqldesc_heading
    char *sqldesc_catalog_name =
      new (collHeap()) char[ComMAX_ANSI_IDENTIFIER_EXTERNAL_LEN];
    desc_items[startingPoint+0].item_id = SQLDESC_CATALOG_NAME;
    desc_items[startingPoint+0].entry = (Lng32) index + 1;
    desc_items[startingPoint+0].num_val_or_len =
      ComMAX_ANSI_IDENTIFIER_EXTERNAL_LEN;
    desc_items[startingPoint+0].string_val = sqldesc_catalog_name;

    char *sqldesc_schema_name =
      new (collHeap()) char[ComMAX_ANSI_IDENTIFIER_EXTERNAL_LEN];
    desc_items[startingPoint+1].item_id = SQLDESC_SCHEMA_NAME;
    desc_items[startingPoint+1].entry = (Lng32)index + 1;
    desc_items[startingPoint+1].num_val_or_len =
      ComMAX_ANSI_IDENTIFIER_EXTERNAL_LEN;
    desc_items[startingPoint+1].string_val = sqldesc_schema_name;

    char *sqldesc_table_name =
      new (collHeap()) char[ComMAX_ANSI_IDENTIFIER_EXTERNAL_LEN];
    desc_items[startingPoint+2].item_id = SQLDESC_TABLE_NAME;
    desc_items[startingPoint+2].entry = (Lng32) index + 1;
    desc_items[startingPoint+2].num_val_or_len =
      ComMAX_ANSI_IDENTIFIER_EXTERNAL_LEN;
    desc_items[startingPoint+2].string_val = sqldesc_table_name;

    char *sqldesc_column_name =
      new (collHeap()) char[ComMAX_ANSI_IDENTIFIER_EXTERNAL_LEN];
    desc_items[startingPoint+3].item_id = SQLDESC_NAME;
    desc_items[startingPoint+3].entry = (Lng32) index + 1;
    desc_items[startingPoint+3].num_val_or_len =
      ComMAX_ANSI_IDENTIFIER_EXTERNAL_LEN;
    desc_items[startingPoint+3].string_val = sqldesc_column_name;

    char *sqldesc_text_format =
      new (collHeap()) char[ComMAX_ANSI_IDENTIFIER_EXTERNAL_LEN];
    desc_items[startingPoint+4].item_id = SQLDESC_TEXT_FORMAT;
    desc_items[startingPoint+4].entry = (Lng32) index + 1;
    desc_items[startingPoint+4].num_val_or_len =
      ComMAX_ANSI_IDENTIFIER_EXTERNAL_LEN;
    desc_items[startingPoint+4].string_val = sqldesc_text_format;

    char *sqldesc_heading =
      new (collHeap()) char[ComMAX_ANSI_IDENTIFIER_EXTERNAL_LEN];
    desc_items[startingPoint+5].item_id = SQLDESC_HEADING;
    desc_items[startingPoint+5].entry = (Lng32) index + 1;
    desc_items[startingPoint+5].num_val_or_len =
      ComMAX_ANSI_IDENTIFIER_EXTERNAL_LEN;
    desc_items[startingPoint+5].string_val = sqldesc_heading;

    desc_items[startingPoint+6].item_id = SQLDESC_NULLABLE;
    desc_items[startingPoint+6].entry = (Lng32) index + 1;
  }

  retcode = SQL_EXEC_GetDescItems2(output_desc_,
                                   (Lng32) numColumns * NUMDESC_ITEMS,
                                   desc_items);
  if (retcode < 0)
  {
    d << DgSqlCode(-UDR_ERR_INTERNAL_CLI_ERROR)
      << DgString0("SQL_EXEC_GetDescItems2")
      << DgInt0(retcode);

    deleteDescItems(numColumns, desc_items, collHeap());
    return -1;
  }

  // Now put together Proxy Syntax
  proxySyntax_ = new (collHeap()) NAString();
  NAString &proxy = *proxySyntax_;
  proxy += "SELECT * FROM TABLE ( SP_RESULT_SET ( ";

  for (index = 0; index < numColumns; index++)
  {
    ComUInt32 startingPoint = index * NUMDESC_ITEMS;

    SQLDESC_ITEM &catalog_name = desc_items[startingPoint+0];
    catalog_name.string_val[catalog_name.num_val_or_len] = '\0';

    SQLDESC_ITEM &schema_name = desc_items[startingPoint+1];
    schema_name.string_val[schema_name.num_val_or_len] = '\0';

    SQLDESC_ITEM &table_name = desc_items[startingPoint+2];
    table_name.string_val[table_name.num_val_or_len] = '\0';

    SQLDESC_ITEM &column_name = desc_items[startingPoint+3];
    column_name.string_val[column_name.num_val_or_len] = '\0';

    SQLDESC_ITEM &text_format = desc_items[startingPoint+4];
    text_format.string_val[text_format.num_val_or_len] = '\0';

    SQLDESC_ITEM &heading = desc_items[startingPoint+5];
    heading.string_val[heading.num_val_or_len] = '\0';

    SQLDESC_ITEM &null_value = desc_items[startingPoint+6];

    // To take care of any delimited identifiers,
    // convert the internal names of catalog, schema, table and
    // columns to external format by calling ToAnsiIdentifier
    NAString quote("\""), dot("."), space(" ");
    if (catalog_name.num_val_or_len != 0)
      proxy +=  ToAnsiIdentifier(catalog_name.string_val) + dot;

    if (schema_name.num_val_or_len != 0)
      proxy +=  ToAnsiIdentifier(schema_name.string_val) + dot;

    if (table_name.num_val_or_len != 0)
      proxy +=  ToAnsiIdentifier(table_name.string_val)  + dot;

    // Just in case, if column name is null, set '(EXPR)' as
    // column name
    if (column_name.num_val_or_len != 0)
      proxy += ToAnsiIdentifier(column_name.string_val) + space;
    else
      proxy += quote + "(EXPR)" + quote + space;

    proxy += text_format.string_val + space;

    NAString quotedHeading;
    if (heading.num_val_or_len > 0)
      ToQuotedString(quotedHeading, heading.string_val);
    proxy += ((heading.num_val_or_len > 0) ? ("HEADING " + quotedHeading)
	                                   : "");
    proxy += space;
    proxy += (null_value.num_val_or_len ? "" : "NOT NULL");
    proxy += ((index == numColumns - 1) ?  " ) )" : ", ");
  }

  // Restore the previous context
  if (prevContext != 0)
  {
    if (resetContext(prevContext, d) != 0)
    {
      deleteDescItems(numColumns, desc_items, collHeap());
      return -1;
    }
  }

  deleteDescItems(numColumns, desc_items, collHeap());

  return 0;
} // UdrResultSet::generateProxySyntax

// Allocates and populates rsColumnDesc_ & rsColDescForLM_ fields
void
UdrResultSet::populateColumnDesc(UdrParameterInfo *columnDesc)
{
  // Store UdrParameterInfo objects
  rsColumnDesc_ = (UdrParameterInfo*)
    collHeap()->allocateMemory(numColumns_ * sizeof(UdrParameterInfo));
  memset(rsColumnDesc_, 0, numColumns_ * sizeof(UdrParameterInfo));

  rsColDescForLM_ = (LmParameter *)
    collHeap()->allocateMemory(numColumns_ * sizeof(LmParameter));
  memset(rsColDescForLM_, 0, numColumns_ * sizeof(LmParameter));

  for (ComUInt32 index = 0; index < numColumns_; index++)
  {
    rsColumnDesc_[index] = columnDesc[index];

    UdrParameterInfo *udrColumn = &rsColumnDesc_[index];
    LmParameter *p = &rsColDescForLM_[index];

    // Determine the encoding charset. We will use =_SQL_MX_ISO_MAPPING
    // define value if the param's charset is ISO88591.
    CharInfo::CharSet charset =
      (CharInfo::CharSet) udrColumn->getEncodingCharSet();
    if (charset == CharInfo::ISO88591)
      charset = spInfo_->getUdrGlobals()->getIsoMapping();

    p->init((ComFSDataType) udrColumn->getFSType(),
            udrColumn->getPrec(),
            udrColumn->getScale(),
            charset,
            (CharInfo::Collation) udrColumn->getCollation(),
            COM_OUTPUT_COLUMN,
            FALSE, // objMap
            RS_NONE,
            0, 0, 0, 0, 0, 0, // input lengths and offsets
            udrColumn->getDataOffset(),
            udrColumn->getDataLength(),
            udrColumn->getNullIndicatorOffset(),
            udrColumn->getNullIndicatorLength(),
            udrColumn->getVCLenIndOffset(),
            udrColumn->getVCIndicatorLength(),
            NULL); // parameter name
  }

} // UdrResultSet::populateColumnDesc

// Deallocates rsColumnDesc_ field
void
UdrResultSet::deallocateColumnDesc()
{
  collHeap()->deallocateMemory(rsColumnDesc_);
  rsColumnDesc_ = NULL;
} // udrResultSet::deallocateColumnDesc

// Fetch rows from JDBC(if needed) & CLI and populate rows in
// the replyBuffer.
//
// Returns the number of rows that were copied into replyBuffer excluding
// ERROR and EOD rows.
//
// Note that errors from SQL_EXEC_FETCH are considered to be user errors,
// so an ERROR/EOD rows will be added to replyBuffer and the number of
// rows copied so far will be returned.
//
//
Lng32
UdrResultSet::fetchRows(UdrGlobals *udrGlob,
                        SqlBuffer *requestBuffer,
                        SqlBuffer *replyBuffer,
                        ComDiagsArea &mainDiags,
                        NAList<ComDiagsArea*> *rowDiagsList)
{
  if (state_ != RS_LOADED &&
      state_ != RS_FETCH &&
      state_ != RS_REINITIATED &&
      state_ != RS_EARLY_CLOSE)
  {
    mainDiags << DgSqlCode(-UDR_ERR_INVALID_RS_STATE)
              << DgString0("Fetch or Continue")
              << DgString1(stateString());

    return -1;
  }

  SQLCTX_HANDLE prevContext = 0;
  if (setContext(prevContext, mainDiags) != 0)
    return -1;

  NABoolean eofEncountered = FALSE;
  // We need to set up_state.parentIndex correctly. The request
  // contains an empty row and will contain down_state.parentIndex.
  // We need to copy that into our rows.
  // We will have valid requestBuffer only for FETCH requests. CONTINUE
  // requests will not have requestBuffer. So save parentIndex for later
  // use with CONTINUE messages.
  if (requestBuffer)
  {
    down_state downState;
    tupp requestRow;
    Lng32 retcode = requestBuffer->moveOutSendOrReplyData
      (TRUE,                // [IN] sending? (vs. replying)
       &downState,          // [OUT] queue state
       requestRow,          // [OUT] new data tupp_descriptor
       NULL,                // [OUT] new ControlInfo area
       NULL,                // [OUT] new diags tupp_descriptor
       NULL                 // [OUT] new stats area
       );

    parentQueueIndex_ = downState.parentIndex;
  }

  if (state_ == RS_EARLY_CLOSE)
  {
    // An RS CLOSE request arrived at some point. We need to return
    // EOD to the executor and call the close() method.
    NABoolean ok = copyEODRowIntoSqlBuffer(replyBuffer, parentQueueIndex_);

    // Something went wrong internally if there's no room for an EOD
    // entry in this reply buffer
    UDR_ASSERT(ok, "Failed to add EOD to reply buffer");

    // Transition to the CLOSED state and return. The return value is
    // the number of rows written into the reply buffer.
    close(&mainDiags);
    return 0;
  }

  state_ = RS_FETCH;

  // Allocate databuffer with bufferSize_ bytes
  if (tmpBuffer_ == NULL)
    tmpBuffer_ = new (collHeap()) TmpBuffer(bufferSize_, exeRowSize_,
                                            collHeap());

  // The local variable numBufferedRows is incremented when ever a row
  // is copied in the sql buffer (replyBuffer) means copied directly
  // into sql Buffer by fetchFromJDBC() or fetchFromCLI().
  ComUInt32 numBufferedRows = 0;

  // Check if we have any rows that were fetched earlier and
  // not yet sent to caller.

  if (tmpBuffer_->moreRowsToCopy())
  {
    copyRowsIntoSqlBuffer(replyBuffer,
                          parentQueueIndex_,
                          numBufferedRows,
                          rowDiagsList);
  }

  // By this time make sure that tmpBuffer_ is empty.
  // If we got out from copyRowsIntoSqlBuffer() function and
  // tmpBuffer_ is not empty, then it means SQL buffer is full.
  // In that case, don't fetch any more rows from JDBC or CLI.
  if (!(tmpBuffer_->moreRowsToCopy()))
  {
    // Now check if we have to fetch any special rows.
    // For SPJs, special rows mean those rows that were cached
    // in JDBC buffer.
    if (lmResultSet_->moreSpecialRows())
    {
      fetchRowsFromJDBC(udrGlob,
                      replyBuffer,
                      numBufferedRows,   // in/out value
                      eofEncountered,
                      mainDiags,
                      rowDiagsList);
    }
    // Fetch rows from CLI only when JDBC row access was successful
    // and eof is not encountered and there is space in the sql buffer(given
    // by no rows in tmpBuffer_).
    // If CLI statement is closed which is possible
    // in case of unique fetch row, fetchRowsFromJDBC already has added EOD Row
    // in sql Buffer and set eofEncountered to TRUE.
    if (mainDiags.getNumber(DgSqlCode::ERROR_) == 0 && !eofEncountered &&
        !(tmpBuffer_->moreRowsToCopy()))
    {
      fetchRowsFromCLI(udrGlob,
                     replyBuffer,
                     numBufferedRows,    // in/out value
                     mainDiags,
                     rowDiagsList);
    }
  }
  // Restore the previous context
  if (prevContext != 0)
  {
    if (resetContext(prevContext, mainDiags) != 0)
      return -1;
  }

  return numBufferedRows;
} // udrResultSet::fetchRows

// Fetches rows that are cached in JDBC buffers.
// Note  8/4/06:
// Right now the function fetchSpecialRows() in LmResultSetJava.cpp
// return one row at a time from a ResultSet. The fetchRowsFromJDBC() code
// is based on this fact. So if in the future it returns more than one row,
// then this function code will be required to change.

void
UdrResultSet::fetchRowsFromJDBC(UdrGlobals *udrGlob,
                                SqlBuffer *replyBuffer,
                                ComUInt32 &numBufferedRows,
                                NABoolean &eofEncountered,
                                ComDiagsArea &mainDiags,
                                NAList<ComDiagsArea*> *rowDiagsList)
{
  eofEncountered = FALSE;



  NABoolean done = FALSE;
  while (lmResultSet_->moreSpecialRows())
  {
    // At this time tempBuffer_ should be empty
    ComDiagsArea *diagsArea = tmpBuffer_->getDiags();

    // Right now, Function fetchSpecialRows() processes one row at a time
    // and returns 1, if it succeeds. If in future it processes more than
    // one row and returns more than 1, then this function will require
    // changes too.
    Lng32 numRows =
    lmResultSet_->fetchSpecialRows(tmpBuffer_->getBufferPtr(),
		                   rsColDescForLM_, numColumns_,
                                   mainDiags, diagsArea);
    if (numRows == -1)
    {
      needToCopyErrorRow_ = TRUE;
      needToCopyEODRow_ = TRUE;

      if (copyErrorRowIntoSqlBuffer(replyBuffer, parentQueueIndex_, mainDiags))
         needToCopyErrorRow_ = FALSE;
      else
         done = TRUE;

      // Write EOD row only after writing ERROR row, if we have to write one.
      // Change state_ only when EOD row is written.
      if (done != TRUE && copyEODRowIntoSqlBuffer(replyBuffer, parentQueueIndex_))
        {
          needToCopyEODRow_ = FALSE;
          state_ = RS_FETCH_COMPLETE;
          eofEncountered = TRUE;
        }
      done = TRUE;
    }
    else
      UDR_ASSERT((numRows == 1) || (numRows == 0), "Language Manager returned more than one row, or found an exception or found other errors." );

    if (done != TRUE && !lmResultSet_->isCliStmtAvailable() && numRows == 0)
    {
      // There are no more rows to read from T4 driver.
      needToCopyEODRow_ = TRUE;
      if (copyEODRowIntoSqlBuffer(replyBuffer, parentQueueIndex_))
      {
        needToCopyEODRow_ = FALSE;
        state_ = RS_FETCH_COMPLETE;
        eofEncountered = TRUE;
      }

      done = TRUE;
    }

    if (done == TRUE) {
      return;
    }

    // By this time tempBuffer_ should not be empty.
    tmpBuffer_->setNumRows(numRows);

    // Copy rows from TmpBuffer into replyBuffer
    if (tmpBuffer_->moreRowsToCopy())
    {
      ComUInt32 numRowsCopied = 0;
      copyRowsIntoSqlBuffer(replyBuffer,
                            parentQueueIndex_,
                            numRowsCopied,
                            rowDiagsList);
      if (numRowsCopied == 0)
        return;  // there is no space in buffer
      else
        numBufferedRows += numRowsCopied;
    }
  }


  // If CLI statement is closed, copy end of data Row into sqlBuffer
  // and set eofEncounter TRUE, so that we will not be fetching from CLI.
  // e.g. If there is an unique row fetch (e.g select with where on
  // primary key) jdbc gets 1 row, and at that time CLI statement gets closed.

  if (lmResultSet_->isCliStmtAvailable() && lmResultSet_->isCLIStmtClosed())
  {
      if (copyEODRowIntoSqlBuffer(replyBuffer, parentQueueIndex_))
      {
        needToCopyEODRow_ = FALSE;
        state_ = RS_FETCH_COMPLETE;
        eofEncountered = TRUE;
      }
  }


} // udrResultSet::fetchRowsFromJDBC


// Sets up the quad fields for multiple row fetching from CLI.
// Quad fields contain four fields.
//   1. Data ptr, 2. data size
//   3. Null indicator ptr and 4. Null indicator size
// We need to setup a quadfield for each output column. CLI uses data size
// to advance the pointer to copy next row value.
//
// For example: C1R1 is copied at DataPtr
//              C1R2 is copied at DataPtr+data size
//              C1R3 is copied at DataPtr+(2 * data size)  and so on
// Similar things happen for Null indicator.
//
// setupQuadFields() sets up quadfields in such way that CLI copies all
// columns of a row next to each other and also all the rows are copied
// next to each other.
//
// Note:
// 1. tmpBuffer_ is set up to hold buffersize/rowsize number of rows.
//    These number of rows may not fit into given SqlBuffer because of
//    SqlBuffer's overhead.
// 2. It is possible for CLI to return fewer than requested number of rows.
//    So in fetchRowsFromCLI(), we make multiple attempts to fill tmpBuffer_
//    (and in turn SqlBuffer).
//
// returns FALSE for errors
//         TRUE  for success
NABoolean
UdrResultSet::setupQuadFields(ComDiagsArea &d)
{
  // We can reuse the quad_fields_ that were setup earlier.
  if (quad_fields_ != NULL)
    return TRUE;

  // The Quad_Fields point to memory locations in tmpBuffer_
  ComUInt32 nBytes = numColumns_ * sizeof(SQLCLI_QUAD_FIELDS);
  if (nBytes > 0)
  {
    quad_fields_ = (SQLCLI_QUAD_FIELDS *) collHeap()->allocateMemory(nBytes);
    memset(quad_fields_, 0, nBytes);
  }

  for (ComUInt32 index = 0; index < numColumns_; index++)
  {
    // Next we want to compute the offset of the data region for this
    // column. There are two cases to consider:
    //
    // a. VARCHAR columns. The CLI returns VARCHAR length indicators
    // with the data. We setup quad fields so that
    // - the beginning of the data region is the offset of the
    //   VARCHAR length indicator
    // - the "quad field layout" does not include the size of the
    //   length prefix. This may be non-intuitive but it's what the
    //   CLI expects.
    //
    // b. Non-VARCHAR columns. Just use the data offset that was
    // provided in the RS LOAD message.

    const UdrParameterInfo &paramInfo = rsColumnDesc_[index];
    ComUInt32 vcIndLen = paramInfo.getVCIndicatorLength();
    ComUInt32 dataOffset = paramInfo.getDataOffset();
    ComUInt32 dataLayout = tmpBuffer_->getFetchRowSize();
    ComUInt32 nullIndLayout = dataLayout;
    const char *buf = tmpBuffer_->getBufferPtr();

    if (vcIndLen > 0)
    {
      ComUInt32 vcLenOffset = paramInfo.getVCLenIndOffset();
      UDR_ASSERT((vcLenOffset + vcIndLen) == dataOffset,
                 "VARCHAR indicator must immediately precede data");
      UDR_ASSERT(vcIndLen % 2 == 0, "VARCHAR indicator len must be even");
      dataOffset = vcLenOffset;
      dataLayout -= vcIndLen;
    }

    quad_fields_[index].var_ptr = (void *) (buf + dataOffset);
    quad_fields_[index].var_layout = (Lng32) dataLayout;

    if (paramInfo.isNullable())
    {
      ComUInt32 nullIndOffset = paramInfo.getNullIndicatorOffset();
      quad_fields_[index].ind_ptr = (void *) (buf + nullIndOffset);
      quad_fields_[index].ind_layout = (Lng32) nullIndLayout;
    }
  }

  Lng32 rowsPerFetch = (Lng32) tmpBuffer_->getNumRowsPerFetch();
#ifdef UDR_DEBUG
  char * e = getenv("UDR_RS_FETCH_SIZE");
  if (e && e[0])
  {
    Lng32 fetchSize = atol(e);
    if (fetchSize > 0 && fetchSize < rowsPerFetch)
      rowsPerFetch = fetchSize;
  }
#endif

  singleRowFetchEnabled_ = (rowsPerFetch == 1) ? TRUE : FALSE;

  UDR_DEBUG2("Fetch size for Result Set %p is set to %d.",
             this, rowsPerFetch);

  Lng32 rowset_status[1];
  Int32 retcode = SQL_EXEC_SETROWSETDESCPOINTERS(output_desc_,
                                               rowsPerFetch,
                                               rowset_status,
                                               1,
                                               (Lng32) numColumns_,
                                               quad_fields_);
  if (retcode < 0)
  {
    d << DgSqlCode(-UDR_ERR_INTERNAL_CLI_ERROR)
      << DgString0("SQL_EXEC_SETROWSETDESCPOINTERS")
      << DgInt0(retcode);

    return FALSE;
  }

  return TRUE;
} // udrResultSet::setupQuadFields


void
UdrResultSet::fetchRowsFromCLI(UdrGlobals *udrGlob,
                               SqlBuffer *replyBuffer,
                               ComUInt32 &numBufferedRows,
                               ComDiagsArea &mainDiags,
                               NAList<ComDiagsArea*> *rowDiagsList)
{
  Lng32 retcode = 0;
  const char *moduleName = "fetchRowsFromCLI";

  // First, setup quad fields to point to TmpBuffer so that CLI copies
  // row values into TmpBuffer
  if (setupQuadFields(mainDiags) == FALSE)
    return;

  // If JDBC used nowait for this stmt, let's disassociate
  // the stmt from the QFO file. UDR Server uses waited
  // mode to fetch rows from RS stmt.
  if (stmt_id_->tag != 0)
  {
    retcode = SQL_EXEC_DisassocFileNumber(stmt_id_);
    if (retcode < 0)
    {
      mainDiags << DgSqlCode(-UDR_ERR_INTERNAL_CLI_ERROR)
                << DgString0("SQL_EXEC_DisassocFileNumber")
                << DgInt0(retcode);

      needToCopyErrorRow_ = needToCopyEODRow_ = TRUE;
    }

    stmt_id_->tag = 0;
  }

  // Check if we have any rows that were fetched earlier and
  // not yet sent to caller.
  if (tmpBuffer_->moreRowsToCopy())
  {
    copyRowsIntoSqlBuffer(replyBuffer,
                          parentQueueIndex_,
                          numBufferedRows,
                          rowDiagsList);
  }

  // Fetch more rows and copy them to SqlBuffer.
  // Looks like there was a bug in CLI code where it returns fewer
  // than requested rows even though there are so many rows. So we call
  // FETCH in loop until the reply buffer is full, error occurred or
  // EOD reached.
  //
  // The following loop is executed with the following conditions
  //  - EOD is not reached (either normally or abnormally)
  //  - tmpBuffer_ is empty
  while (! needToCopyEODRow_ &&
         ! tmpBuffer_->moreRowsToCopy())
  {
    retcode = SQL_EXEC_ClearDiagnostics(stmt_id_);
    if (retcode < 0)
    {
      mainDiags << DgSqlCode(-UDR_ERR_INTERNAL_CLI_ERROR)
                << DgString0("SQL_EXEC_ClearDiagnostics")
                << DgInt0(retcode);

      needToCopyErrorRow_ = needToCopyEODRow_ = TRUE;
      continue;
    }

    // Start fetching rows into tmpBuffer_ (pointed to by output_desc_)
    retcode = SQL_EXEC_Fetch(stmt_id_, output_desc_, 0, 0);

    if (retcode < 0)
    {
      needToCopyErrorRow_ = needToCopyEODRow_ = TRUE;
    }

    if (retcode == 100)
    {
      needToCopyErrorRow_ = FALSE;
      needToCopyEODRow_ = TRUE;
    }

    if (retcode != 0 && retcode != 100)
    {
      // There were errors or warnings, let's get CLI diags into tmpBuffer_.
      NAMemory *h = collHeap();
      ULng32 bufSize = 2048;
      char *bufForPackedDiags = (char *) h->allocateMemory(bufSize);
      ULng32 bufSizeNeeded = 0;
      Lng32 messageObjType = 0;
      Lng32 messageObjVersion = 0;
      Lng32 fetchRetcode = retcode;

      retcode = SQL_EXEC_GetPackedDiagnostics_Internal(
        /*OUT*/   bufForPackedDiags,
        /*IN*/    bufSize,
        /*OUT*/  &bufSizeNeeded,
        /*OUT*/  &messageObjType,
        /*OUT*/  &messageObjVersion);

      if (retcode >= 0 && bufSizeNeeded > bufSize)
      {
        h->deallocateMemory(bufForPackedDiags);
        bufSize = bufSizeNeeded;
        bufForPackedDiags = (char *) h->allocateMemory(bufSize);
        retcode = SQL_EXEC_GetPackedDiagnostics_Internal(
          /*OUT*/   bufForPackedDiags,
          /*IN*/    bufSize,
          /*OUT*/  &bufSizeNeeded,
          /*OUT*/  &messageObjType,
          /*OUT*/  &messageObjVersion);
      }

      if (retcode >= 0)
      {
        ComDiagsArea &d = *(tmpBuffer_->getDiags());
        d.unpackObj(messageObjType,
                    messageObjVersion,
                    TRUE,
                    bufSizeNeeded,
                    bufForPackedDiags);

        h->deallocateMemory(bufForPackedDiags);
        bufForPackedDiags = NULL;

	if (fetchRetcode < 0 && d.getNumber(DgSqlCode::ERROR_) == 0)
	{
	  // There are cases where Fetch returns error but no error
	  // conditions exists in CLI. These situations are addressed
          // here by raising internal error.
          mainDiags << DgSqlCode(-UDR_ERR_INTERNAL_CLI_ERROR)
                    << DgString0("SQL_EXEC_Fetch")
                    << DgInt0(fetchRetcode);
          needToCopyErrorRow_ = needToCopyEODRow_ = TRUE;
	  continue;
	}
      }
      else
      {
        h->deallocateMemory(bufForPackedDiags);
        bufForPackedDiags = NULL;

        mainDiags << DgSqlCode(-UDR_ERR_INTERNAL_CLI_ERROR)
                  << DgString0("SQL_EXEC_MergeDiagnostics_Internal")
                  << DgInt0(retcode);

        SQL_EXEC_ClearDiagnostics(stmt_id_);
        needToCopyErrorRow_ = needToCopyEODRow_ = TRUE;
        continue;
      }
    }

    retcode = SQL_EXEC_ClearDiagnostics(stmt_id_);
    if (retcode < 0)
    {
      mainDiags << DgSqlCode(-UDR_ERR_INTERNAL_CLI_ERROR)
                << DgString0("SQL_EXEC_ClearDiagnostics")
                << DgInt0(retcode);

      needToCopyErrorRow_ = needToCopyEODRow_ = TRUE;
      continue;
    }

    // Find the number of rows fetched.
    Lng32 numRowsFetched = 0;
    retcode = SQL_EXEC_GetDescItem(output_desc_, 1,
                                   SQLDESC_ROWSET_NUM_PROCESSED,
                                   &numRowsFetched, 0, 0, 0, 0);
    if (retcode < 0)
    {
      mainDiags << DgSqlCode(-UDR_ERR_INTERNAL_CLI_ERROR)
                << DgString0("SQL_EXEC_GetDescItem")
                << DgInt0(retcode);

      SQL_EXEC_ClearDiagnostics(stmt_id_);

      needToCopyErrorRow_ = needToCopyEODRow_ = TRUE;
      continue;
    }

    tmpBuffer_->setNumRows((ComUInt32) numRowsFetched);

    if (numRowsFetched > 0 &&
        udrGlob->verbose_ &&
        udrGlob->traceLevel_ >= TRACE_DATA_AREAS &&
        udrGlob->showInvoke_)
    {
      ComUInt32 numRows = tmpBuffer_->getNumRows();
      ComUInt32 rowLen = tmpBuffer_->getExeRowSize();
      const char *tmpBufData = tmpBuffer_->getBufferPtr();
      ComUInt32 tmpBufLen = tmpBuffer_->getSize();

      ServerDebug("");
      ServerDebug("[UdrServ (%s)] CLI fetch buffer", moduleName);
      ServerDebug("[UdrServ (%s)] N rows %ld, exe row size %u, buf size %u",
                  moduleName, numRows, rowLen, tmpBufLen);

      // Display either the full buffer, or N+1 rows, whichever region
      // is smaller
      ComUInt32 displayBytes = (numRows + 1) * rowLen;
      if (displayBytes > tmpBufLen)
        displayBytes = tmpBufLen;

      dumpBuffer((unsigned char *) tmpBufData, displayBytes);
    }

    // Now copy as many rows as we can fit in replyBuffer from tmpBuffer_
    ComUInt32 numRowsCopied = 0;
    copyRowsIntoSqlBuffer(replyBuffer,
                             parentQueueIndex_,
                             numRowsCopied,
                             rowDiagsList);
    numBufferedRows += numRowsCopied;

  } // while loop

  // Even if we have called copyCLIRowsIntoSqlBuffer() above, we may
  // still have some rows in tmpBuffer_ that need to be copied to replyBuffer.
  // This is possible because there may not be enough space in
  // replyBuffer for all the rows read. So write ERROR/EOD row only if no
  // more rows in tmpBuffer_.
  if (! tmpBuffer_->moreRowsToCopy())
  {
    // Note that it is possible that there is no space for ERROR/EOD row.
    // That is not a problem and we write these rows next time.
    if (needToCopyErrorRow_)
    {
      if (copyErrorRowIntoSqlBuffer(replyBuffer, parentQueueIndex_, mainDiags))
        needToCopyErrorRow_ = FALSE;
    }

    // Write EOD row only after writing ERROR row, if we have to write one.
    // Change state_ only when EOD row is written.
    if (needToCopyEODRow_ && !needToCopyErrorRow_)
    {
      if (copyEODRowIntoSqlBuffer(replyBuffer, parentQueueIndex_))
      {
        needToCopyEODRow_ = FALSE;
        state_ = RS_FETCH_COMPLETE;
      }
    }
  }

  // return the number of rows (not including Error & EOD rows) in SqlBuffer.
  return;

} // udrResultSet::fetchRowsFromCLI


// copyRowsIntoSqlBuffer() knows how to copy a row from tmpBuffer_
// into SqlBuffer. It allocates a row in SqlBuffer and copies a row
// from tmpBuffer_. This method uses the passed in queueIndex value
// for all the rows.
// If there are warnings for a row, a ComDiagsArea object containing
// all the warnings of that row will be inserted into rowDiagsList.
void
UdrResultSet::copyRowsIntoSqlBuffer(SqlBuffer *replyBuffer,
                                       queue_index queueIndex,
                                       ComUInt32 &numRowsCopied,
                                       NAList<ComDiagsArea*> *rowDiagsList)
{
  // First copy the rows themselves along with any associated
  // warnings.
  while (tmpBuffer_->moreRowsToCopy())
  {
    char *rowPtr = NULL;
    ControlInfo *newControlInfo = NULL;

    // Allocate a row in SqlBuffer.
    NABoolean result = allocateReplyRow(getSPInfo()->getUdrGlobals(),
                                        *replyBuffer,
                                        queueIndex,
                                        (Lng32) exeRowSize_,
                                        rowPtr,
                                        newControlInfo,
                                        ex_queue::Q_OK_MMORE);

    if (result)
    {
      ComDiagsArea *rowDiags = NULL;
      char *srcRowPtr = tmpBuffer_->getNextRow(rowDiags,
                                               singleRowFetchEnabled_);
      fixDataRow(srcRowPtr, rowDiags);

      memcpy(rowPtr, srcRowPtr, exeRowSize_);
      numRowsCopied++;

      // Above getNextRow() call might have generated rowDiags pointer.
      // Make sure those diags will be sent back to caller
      if (rowDiags && rowDiags->getNumber(DgSqlCode::WARNING_))
      {
        newControlInfo->setIsExtDiagsAreaPresent(TRUE);
        rowDiagsList->insert(rowDiags);
      }
    }
    else
    {
      // No more space left in the SqlBuffer. That is okay here.
      // We return from here and let the caller come back
      // asking for more.
      return;
    }
  }
} // UdrResultSet::copyRowsIntoSqlBuffer

NABoolean
UdrResultSet::copyErrorRowIntoSqlBuffer(SqlBuffer *replyBuffer,
                                        queue_index queueIndex,
                                        ComDiagsArea &mainDiags)
{
  UdrGlobals *glob = getSPInfo()->getUdrGlobals();
  NABoolean errorRowAllocOK =
    allocateErrorRow(glob, *replyBuffer, queueIndex, TRUE);

    // Add error condition to main diags
  if (errorRowAllocOK)
    mainDiags.mergeAfter(* tmpBuffer_->getDiags());

  return errorRowAllocOK;
} // UdrResultSet::copyErrorRowIntoSqlBuffer

NABoolean
UdrResultSet::copyEODRowIntoSqlBuffer(SqlBuffer *replyBuffer,
                                      queue_index queueIndex)
{
  UdrGlobals *glob = getSPInfo()->getUdrGlobals();
  NABoolean eodRowAllocOK =
      allocateEODRow(glob, *replyBuffer, parentQueueIndex_);

  return eodRowAllocOK;
} // UdrResultSet::copyEODRowIntoSqlBuffer

// fixDataRow() makes any corrections needed for the CLI row
// so that the rows can be copied to SqlBuffer easily.
char *
UdrResultSet::fixDataRow(char *rowPtr, ComDiagsArea * rowDiags)
{
  // Correction for 8402 (String overflow) warning:
  // For string overflow cases, CLI sets the final string size
  // in the NULL indicator bytes. Look for
  //  "if (cliDiags && cliDiags->containsWarning(0, EXE_STRING_OVERFLOW))"
  // in InputOutputExpr::outputValues() for details.
  // We will correct the row so that the null indicator bytes will
  // contain either '0's(not null) or '\377's(null).

  if (rowDiags && rowDiags->containsWarning(0, EXE_STRING_OVERFLOW))
  {
    char *nullIndPtr;
    for (ComUInt32 colIndex=0; colIndex < numColumns_; colIndex++)
    {
      UdrParameterInfo colInfo = rsColumnDesc_[colIndex];

      ComUInt16 nullIndLen = colInfo.getNullIndicatorLength();
      if (colInfo.isNullable())
      {
        nullIndPtr = rowPtr + colInfo.getNullIndicatorOffset();

        for (Int32 i=0; i < nullIndLen; i++)
          if (! (nullIndPtr[i] & NEG_BIT_MASK))
            nullIndPtr[i] = '\0';
      }
    }
  }

  return rowPtr;
} // UdrResultSet::fixDataRow

// UdrResultSet::close() is called during RS_CLOSE message processing.
// The UDR Server and LM resources for this UdrResultSet will be freed
// and state changes to RS_CLOSED.
void
UdrResultSet::close(ComDiagsArea *diags)
{
  // release UDR resources
  deallocateUDRGeneratedFields();

  // Cleanup LmResultSet object
  if (lmResultSet_)
    getSPInfo()->getLMHandle()->cleanupLmResultSet(lmResultSet_, diags);
  lmResultSet_ = NULL;

  setState(RS_CLOSED);
} // UdrResultSet::close

void UdrResultSet::processRSClose(ComDiagsArea *diags)
{
  if (state_ == RS_FETCH)
  {
    // All we do is transition to EARLY_CLOSE and return. Later, a
    // CONTINUE request will arrive and we will return EOD to the
    // executor and call the close() method.
    setState(RS_EARLY_CLOSE);
  }
  else
  {
    // If we are not currently fetching then call the close() method
    close(diags);
  }
}


// Unloading a UdrResultSet means that the Executor is breaking the link
// between RS Proxy TCB and UdrResultSet object.
// After unloading a UdrResultSet, Executor has to send RS_LOAD message
// at this index so that this UdrResultSet would be useful.
void
UdrResultSet::unload()
{
  deallocateExeGeneratedFields();

  setState(RS_UNLOADED);
} // UdrResultSet::unload
