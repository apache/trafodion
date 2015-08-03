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
* File:         UdrResultSet.h
* Description:  This file contains UdrResultSet class and
*               TmpBuffer class definitions.
* Created:      10/20/2005
* Language:     C++
**
*****************************************************************************
*/
#ifndef UDRRESULTSET_H
#define UDRRESULTSET_H

#include "udrdefs.h"
#include "NAType.h"
#include "LmResultSet.h"
#include "UdrExeIpc.h"
#include "ComSmallDefs.h"
#include "QueueIndex.h"
#include "udrglobals.h"
#include "LmParameter.h"
// forward references
class SPInfo;
class UdrGlobals;

// The number of different descriptor info that we are interested in
// for each column to generate Proxy Syntax. Right now we are interested in
// SQLDESC_CATALOG_NAME, SQLDESC_SCHEMA_NAME, SQLDESC_TABLE_NAME,
// SQLDESC_NAME, SQLDESC_TEXT_FORMAT, SQLDESC_HEADING, SQLDESC_NULLABLE
const Int32 NUMDESC_ITEMS = 7;

//////////////////////////////////////////////////////////////////////////
//
// TmpBuffer: This class manages the temporary location that's needed
// for UdrResultSet during row fetch from CLI.
// This class keeps track of total number of rows in the buffer and 
// the number that needs to be provided to the caller. This class
// does not provide interface to read previous rows that were read.
// This class also contains the diagnostic conditions related to the
// data rows.
//
//////////////////////////////////////////////////////////////////////////
class TmpBuffer
{
public:
  TmpBuffer(ComUInt32 size, ComUInt32 rowSize, NAMemory *heap)
    : heap_(heap),
      size_(size),
      exeRowSize_(rowSize),
      // When we fetch from the local CLI, we need a row size that is
      // a multiple of 2. This is explained in commentary below.
      fetchRowSize_(rowSize + (rowSize % 2)),
      numRows_(0),
      index_(0)
  {
    // Here's why our CLI fetch size must be even. It's a workaround
    // to a quirk in the CLI. The quirky behavior only happens for
    // rows with VARCHAR columns but we can use the workaround for all
    // rows. 
    // 
    // First note that we use quad fields to do rowset fetching
    // from the CLI (see UdrResultSet::setupQuadFields() for more info
    // on our use of quad fields). The "layout" values in the quad
    // fields structs are supposed to be the length of the entire
    // row. The CLI uses these layout values to compute the address
    // offset between a given column in two consecutive rows. For
    // example, if we want column 1 in row 1 and column 1 in row 2 to
    // be separated by N bytes, then we need layout values of N.
    // 
    // For a VARCHAR column if the layout is not even, the CLI will
    // "add 1" to the layout when computing these offsets. This is not
    // what we want. To avoid the "add 1" quirk we simply never give
    // the CLI an odd value in the layout fields. The "add 1" quirk is
    // in Descriptor::getVarItem() and was supposed to be a fix for
    // COBOL programs only, but is taking effect for all programs.

    if (size_ < fetchRowSize_)
      size_ = fetchRowSize_;

    // Because of the way we setup quadfields, it is possible to overwrite
    // the bytes beyond the allocated bytes if we happen to fetch rows full
    // of the buffer. So we actually allocate space for one extra row beyond
    // size_ bytes. Probably rowwise rowsets will not have this problem and
    // should also yield better performance.
    ComUInt32 nBytes = size_ + fetchRowSize_;
    buffer_ = new(heap) char[nBytes];
    memset(buffer_, 0, nBytes);

    cliDiags_ = ComDiagsArea::allocate(heap_);
  }

  ~TmpBuffer()
  {
    NADELETEBASIC(buffer_, heap_);
    cliDiags_->decrRefCount();
  }

  // Accessors
  ComUInt32 getSize() const { return size_; }
  ComUInt32 getExeRowSize() const { return exeRowSize_; }
  ComUInt32 getFetchRowSize() const { return fetchRowSize_; }
  ComUInt32 getNumRows() const { return numRows_; }
  ComUInt32 getIndex() const { return index_; }
  char *getBufferPtr() const { return buffer_; }
  ComDiagsArea *getDiags() const { return cliDiags_; }

  ComUInt32 getNumRowsPerFetch() const { return size_ / fetchRowSize_; }

  ComUInt32 getNumRowsToCopy() const { return numRows_ - index_; }
  NABoolean moreRowsToCopy() const
  {
    UDR_ASSERT(index_ <= numRows_, "Wrong index_ value in TmpBuffer.");
    return numRows_ != index_;
  }

  char *getNextRow(ComDiagsArea* &rowDiags,
                   NABoolean singleRowFetchEnabled = FALSE);

  // Mutators
  void setNumRows(ComUInt32 numRows) { numRows_ = numRows; }

private:
  NAMemory *heap_;         // Heap where buffer_ is allocated
  char *buffer_;           // Place where CLI rows are copied temporarily
  ComUInt32 size_;         // Buffer size
  ComUInt32 exeRowSize_;   // Row size returned to executor
  ComUInt32 fetchRowSize_; // Row size fetched from the CLI
  ComUInt32 numRows_;      // Number of rows in this buffer
  ComUInt32 index_;        // Index of first row that was not copied from
                           // this buffer.

  ComDiagsArea *cliDiags_; // CLI diags corresponding to the rows in this
                           // buffer
};

//////////////////////////////////////////////////////////////////////////
//
// UdrResultSet: A representation of Result Set in UDR Server layer.
//
//////////////////////////////////////////////////////////////////////////
class UdrResultSet : public NABasicObject
{
public:
  enum RSState
  {
    RS_INITIATED = 1,   // RS initialized after UDR invoke.
                        //  The column desc is not set yet when RS is in
                        //  this state.
    RS_REINITIATED,     // RS re-initialized. 
                        //  Column desc is set and may be used for fetches
                        //  depending on the next message from master.
    RS_LOADED,          // Column desc populated
    RS_UNLOADED,        // Column desc is destroyed
    RS_FETCH,           // Rows being fetched (by either
                        //  RS INVOKE or CONTINUE requests)
    RS_FETCH_COMPLETE,  // FETCH is compete
    RS_EARLY_CLOSE,     // An RS CLOSE request arrived before
                        //  EOD was returned
    RS_CLOSED           // RS closed
  };

  // Constructor to create UdrResultSet objects during UDR_INVOKE
  // phase.
  UdrResultSet(SPInfo *spInfo, LmResultSet *lmRS, ComDiagsArea &d);

  // Constructor that uses SQLSTMT_ID object directly. This constructor
  // will be used when JDBC is not available on the platform. In this case,
  // SPJ method may not be able to return actual result sets
  // and LM will not create LmResultSet objects. Instead the callers create
  // CLI statements directly.
  UdrResultSet(SPInfo *spInfo, SQLSTMT_ID *stmt_id, ComDiagsArea &d);

  ~UdrResultSet();

  // Accessors
  SPInfo *getSPInfo() const { return spInfo_; }
  RSState getState() const { return state_; }
  const char * stateString();
  NABoolean isInited() const { return state_ == RS_INITIATED; }
  NABoolean isReInited() const { return state_ == RS_REINITIATED; }
  NABoolean isLoaded() const { return state_ == RS_LOADED; }
  NABoolean isUnloaded() const { return state_ == RS_UNLOADED; }
  NABoolean isFetchStarted() const { return state_ == RS_FETCH; }
  NABoolean isFetchComplete() const { return state_ == RS_FETCH_COMPLETE; }
  NABoolean isClosed() const { return state_ == RS_CLOSED; }
  NABoolean isEarlyClose() const { return state_ == RS_EARLY_CLOSE; }

  RSHandle getRSHandle() const { return rsHandle_; }
  const char *getProxySyntax() const { return proxySyntax_->data(); }
  const LmResultSet *getLmResultSet() const { return lmResultSet_; }

  ComUInt32 getContextHandle() const 
  { return lmResultSet_ ? lmResultSet_->getCtxHandle() : 0; }
  SQLSTMT_ID *getStatementHandle() const { return stmt_id_; }
 
  ComUInt32 getExeRowSize() const { return exeRowSize_; }
  ComUInt32 getBufferSize() const { return bufferSize_; }

  // mutators
  void setState(RSState state) { state_ = state; }
  // Reinitialize the object. columnDesc_ will not be changed.
  Int32 reInit(LmResultSet *lmRS, ComDiagsArea &d, SQLSTMT_ID *stmt_id=NULL);

  // Prepare for reinvoke
  // Deallocates and run cleanup methods on the non-reusable fields
  void prepareForReinvoke();

  // Load method.
  // A 'load' on UdrResultSet refers to setting the columnDesc_ field
  NABoolean load(RSHandle handle,
                 ComUInt32 numRSCols,
                 ComUInt32 rowSize,
                 ComUInt32 bufferSize,
                 UdrParameterInfo *columnDesc,
                 ComDiagsArea &d);

  // Fetch methods
  Lng32 fetchRows(UdrGlobals *udrGlob,
                 SqlBuffer *request,
                 SqlBuffer *reply,
                 ComDiagsArea &d,
                 NAList<ComDiagsArea*> *rowDiagsList);
  void fetchRowsFromCLI(UdrGlobals *udrGlob,
                        SqlBuffer *reply,
                        ComUInt32 &numBufferedRows,
                        ComDiagsArea &d,
                        NAList<ComDiagsArea*> *rowDiagsList);

  void fetchRowsFromJDBC(UdrGlobals *udrGlob,
                         SqlBuffer *reply,
                         ComUInt32 &numBufferedRows,
                         NABoolean &eofEncountered,
                         ComDiagsArea &d,
                         NAList<ComDiagsArea*> *rowDiagsList);

  // Unload Result Set. This destroys the column desc. Executor has
  // to send an RS_LOAD message, if it wants to use this RS 
  void unload();

  // Close Result Set. After closing an RS, the same RS can be
  // reopened by UDR INVOKE.
  void close(ComDiagsArea *diags);

  // This method can be called when an RS CLOSE request arrives. If
  // the current state is not FETCH then this is a no-op. In the FETCH
  // state, this method transitions the instance to the EARLY_CLOSE
  // state. A subsequent RS CONTINUE request returns EOD to the
  // executor and transitions to the CLOSE state.
  void processRSClose(ComDiagsArea *diags);

private:
  // Deallocates the fields that are generated by Udr Server
  // This method is called from desstructor and also during preparation
  // for reinvocation
  void deallocateUDRGeneratedFields();

  // Deallocates the fields that are downloaded from Executor. This
  // method is called during destruction and unload processing.
  void deallocateExeGeneratedFields();

  // Makes a copy of SQLSTMT_ID struct pointed to by stmt_id. If
  // resetStmtInfo is TRUE and the name mode is not stmt_handle, then
  // the stmt_handle field is set to zero. This breaks any association
  // with an existing StatementInfo structure inside the CLI.
  SQLSTMT_ID *copyStatementID(SQLSTMT_ID *stmt_id,
                              NABoolean resetStmtInfo = TRUE);

  // Context setting methods
  Int32 setContext(SQLCTX_HANDLE &oldCtx, ComDiagsArea &d);
  Int32 resetContext(SQLCTX_HANDLE ctxHandle, ComDiagsArea &d);

  // Populates proxySyntax_ field
  Int32 generateProxySyntax(ComDiagsArea &d);

  // Allocates and populates rsColumnDesc_ & rsColDescForLM_ fields
  void populateColumnDesc(UdrParameterInfo *paramInfo);

  // Dealloactes rsColumnDesc_ field
  void deallocateColumnDesc();

  // Sets up Quad fields for CLI rowset fetching
  NABoolean setupQuadFields(ComDiagsArea &d);

  // Methods that copy rows into SqlBuffer
  void copyRowsIntoSqlBuffer(SqlBuffer *replyBuffer,
                             queue_index queueIndex,
                             ComUInt32 &numRowsCopied,
                             NAList<ComDiagsArea*> *rowDiagsList);
  NABoolean copyErrorRowIntoSqlBuffer(SqlBuffer *replyBuffer,
                                      queue_index queueIndex,
                                      ComDiagsArea &diags);
  NABoolean copyEODRowIntoSqlBuffer(SqlBuffer *replyBuffer,
                                    queue_index queueIndex);

  char *fixDataRow(char *rowPtr, ComDiagsArea *rowDiags);

  // The following information travels from executor
  RSHandle rsHandle_;               // RS Handle
  UdrParameterInfo *rsColumnDesc_;  // Column descriptions
  ComUInt32 numColumns_;            // number of columns in this RS
  ComUInt32 exeRowSize_;            // Row size for rows returned to executor
  ComUInt32 bufferSize_;            // SqlBuffer Size

  // The following information is computed by UDR server
  SPInfo *spInfo_;                  // Owning SPInfo
  RSState state_;                   // current state
  LmResultSet *lmResultSet_;        // LmResultSet
  NAString *proxySyntax_;           // proxy syntax for this RS

  // The following information is generated and managed by
  // UDR server during proxy syntax creation and also for row fetching.
  ComUInt32 rowsToFetchFromJDBC_;   // Number of rows from JDBC
  SQLSTMT_ID *stmt_id_;             // Used for producing proxy syntax and
                                    // CLI rowset fetching
  SQLDESC_ID *output_desc_;         // Used for producing proxy syntax and
                                    // CLI rowset fetching
  SQLCLI_QUAD_FIELDS *quad_fields_; // Used for CLI rowset fetching
  queue_index parentQueueIndex_;    // Queue index to be used by RS rows
                                    // This index is same as the index seen
                                    // in down queue.
  NABoolean needToCopyErrorRow_;    // Error during fetch.
  NABoolean needToCopyEODRow_;      // EOD reached.
  NABoolean singleRowFetchEnabled_; // Fetch a single row at a time, useful
                                    // for debugging
  LmParameter *rsColDescForLM_;     // Col desc for LM use
  TmpBuffer *tmpBuffer_;            // Place for temporary storage
};

#endif
