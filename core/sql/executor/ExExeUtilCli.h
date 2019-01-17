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

#ifndef EX_EXE_UTIL_CLI_H
#define EX_EXE_UTIL_CLI_H

class ContextCli;

class OutputInfo
{
 public:
  enum {MAX_OUTPUT_ENTRIES = 100};

  OutputInfo(Lng32 numEntries);
  void dealloc(CollHeap * heap);
  void insert(Lng32 index, char * data);
  void insert(Lng32 index, char * data, Lng32 len);
  void insert(Lng32 index, char * data, Lng32 len,Lng32 type,Lng32 *indOffset = NULL, Lng32 *varOffset = NULL);
  char * get(Lng32 index);
  short get(Lng32 index, char* &data, Lng32 &len);
  short get(Lng32 index, char* &data, Lng32 &len, Lng32 &type,Lng32 *indOffset, Lng32 *varOffset);

 private:
  Lng32 numEntries_;
  char * data_[MAX_OUTPUT_ENTRIES];
  Lng32 len_[MAX_OUTPUT_ENTRIES];
  Lng32 type_[MAX_OUTPUT_ENTRIES];
};

class ExeCliInterface : public NABasicObject
{
private:
  enum {
    NOT_EXEUTIL_INTERNAL_QUERY = 0x0001
  };

 public:
  ExeCliInterface(CollHeap * heap = NULL, Int32 isoMapping = 0,
                  ContextCli * currContext = NULL,
                  const char *parentQid = NULL);

  virtual ~ExeCliInterface();

  Lng32 allocStuff(SQLMODULE_ID * &module,
                  SQLSTMT_ID * &stmt,
                  SQLDESC_ID * &sql_src,
                  SQLDESC_ID * &input_desc,
                  SQLDESC_ID * &output_desc,
                  const char * stmtName = NULL
                  );

  Lng32 deallocStuff(SQLMODULE_ID * &module,
                    SQLSTMT_ID * &stmt,
                    SQLDESC_ID * &sql_src,
                    SQLDESC_ID * &input_desc,
                    SQLDESC_ID * &output_desc);

  Lng32 executeImmediate(const char * stmt,
			 char * outputBuf = NULL,
			 Lng32 * outputBufLen = NULL,
			 NABoolean nullTerminate = TRUE,
			 Int64 * rowsAffected = NULL,
			 NABoolean monitorThis = FALSE,
			 ComDiagsArea **globalDiags = NULL);

  Lng32 executeImmediatePrepare(const char * stmt,
				char * outputBuf = NULL,
				Lng32 * outputBufLen = NULL,
				Int64 * rowsAffected = NULL,
				NABoolean monitorThis = FALSE,
				char * stmtName = NULL);

  Lng32 executeImmediatePrepare2(const char * stmt,
				 char *uniqueStmtId,
				 Lng32 *uniqueStmtIdLen,
				 SQL_QUERY_COST_INFO *query_cost_info,
				 SQL_QUERY_COMPILER_STATS_INFO *comp_stats_info,
				 char * outputBuf = NULL,
				 Lng32 * outputBufLen = NULL,
				 Int64 * rowsAffected = NULL,
				 NABoolean monitorThis = FALSE);

  Lng32 executeImmediateExec(const char * stmt,
                            char * outputBuf = NULL,
                            Lng32 * outputBufLen = NULL,
                            NABoolean nullTerminate = TRUE,
                            Int64 * rowsAffected = NULL,
                            ComDiagsArea **diagsArea = NULL);
 
  Lng32 prepare(const char * stmtStr,
		SQLMODULE_ID * module,
		SQLSTMT_ID * stmt,
		SQLDESC_ID * sql_src,
		SQLDESC_ID * input_desc,
		SQLDESC_ID * output_desc,
		char ** outputBuf,
		Queue * outputVarPtrList = NULL,
		char ** inputBuf = NULL,
		Queue * inputVarPtrList = NULL,
		char *uniqueStmtId = NULL,
		Lng32 *uniqueStmtIdLen = NULL,
		SQL_QUERY_COST_INFO *query_cost_info = NULL,
		SQL_QUERY_COMPILER_STATS_INFO *comp_stats_info = NULL,
		NABoolean monitorThis = FALSE,
		NABoolean doNotCachePlan = FALSE);

  Lng32 setupExplainData(SQLMODULE_ID * module,
                       SQLSTMT_ID * stmt);
  Lng32 setupExplainData();
  char* getExplainDataPtr() { return explainData_;}
  Lng32 getExplainDataLen() { return explainDataLen_; }

  Lng32 exec(char * inputBuf = NULL, Lng32 inputBufLen = 0);
  Lng32 fetch();
  Lng32 close();
  Lng32 dealloc();

  short clearExecFetchClose(char * inputBuf, Lng32 inputBufLen,
			    char * outputBuf = NULL,
			    Lng32 * outputBufLen = 0);

  short clearExecFetchCloseOpt(char * inputBuf, Lng32 inputBufLen,
			       char * outputBuf = NULL,
			       Lng32 * outputBufLen = 0,
                               Int64 * rowsAffected = NULL);
  
  Lng32 executeImmediateCEFC(const char * stmtStr,
                             char * inputBuf,
                             Lng32 inputBufLen,
			     char * outputBuf,
			     Lng32 * outputBufLen,
			     Int64 * rowsAffected = NULL
			     );

  Lng32 rwrsPrepare(
		    const char * stmStr, Lng32 rs_maxsize,
		    NABoolean monitorThis = FALSE);

  Lng32 rwrsExec(
    char * inputRow,
    Int32 inputRowLen,
    Int64 * rowsAffected);

  Lng32 rwrsClose();

  Lng32 cwrsAllocStuff(SQLMODULE_ID * &module,
    SQLSTMT_ID * &stmt,
    SQLDESC_ID * &sql_src,
    SQLDESC_ID * &input_desc,
    SQLDESC_ID * &output_desc,
    SQLDESC_ID * &rs_input_maxsize_desc,
    const char * stmtName = NULL
  );

  Lng32 cwrsDeallocStuff(
    SQLMODULE_ID * &module,
    SQLSTMT_ID * &stmt,
    SQLDESC_ID * &sql_src,
    SQLDESC_ID * &input_desc,
    SQLDESC_ID * &output_desc,
    SQLDESC_ID * &rs_input_maxsize_desc);

  Lng32 cwrsPrepare(
		    const char * stmtStr,
		    Lng32 rs_maxsize,
		    NABoolean monitorThis = FALSE);

  Lng32 cwrsExec(
    char * inputRow,
    Int32 inputRowLen,
    Int64 * rowsAffected);

  Lng32 cwrsClose(Int64 * rowsAffected);

  Lng32 getPtrAndLen(short entry, char* &ptr, Lng32 &len, short **ind = NULL);
  Lng32 getHeadingAndLen(short entry, char* heading, Lng32 &len);

  Lng32 getNumEntries(Lng32 &numInput, Lng32 &numOutput);
  Lng32 getAttributes(short entry, NABoolean forInput,
                     Lng32 &fsDatatype, Lng32 &length,
                     Lng32 *indOffset, Lng32 *varOffset);
  Lng32 getDataOffsets(short entry, Lng32 forInput,
		       Lng32 *indOffset, Lng32 *varOffset);

  Lng32 getStmtAttr(char * stmtName, Lng32 attrName, 
		    Lng32 * numeric_value, char * string_value);

  short fetchRowsPrologue(const char  * sqlStrBuf, NABoolean noExec = FALSE,
			  NABoolean monitorThis = FALSE,
			  char * stmtName = NULL);
  short fetchRowsEpilogue(const char  * sqlStrBuf, NABoolean noClose = FALSE);

  short initializeInfoList(Queue* &infoList, NABoolean infoListIsOutputInfo);

  short fetchAllRows(Queue * &infoList,
		     const char * query,
		     Lng32 numOutputEntries = 0,
		     NABoolean varcharFormat = FALSE,
		     NABoolean monitorThis = FALSE,
		     NABoolean initInfoList = FALSE);

  short prepareAndExecRowsPrologue(const char  * sqlInitialStrBuf,
                                   char  * sqlSecondaryStrBuf,
                                   Queue * initialOutputVarPtrList,
                                   Queue * continuingOutputVarPtrList,
                                   Int64 &rowsAffected,
				   NABoolean monitorThis = FALSE);

  short execContinuingRows(Queue * secondaryOutputVarPtrs,
                           Int64   &rowsAffected);

  void setOutputPtrsAsInputPtrs(Queue * entry,
                                SQLDESC_ID * target_inputDesc = NULL);

  Lng32 setCharsetTypes();

  Lng32 prepareAndExecRowsEpilogue();

  Lng32 beginWork();
  Lng32 commitWork();
  Lng32 rollbackWork();
  Lng32 autoCommit(NABoolean v); // TRUE, set to ON. FALSE, set to OFF.
  static Lng32 beginXn();
  static Lng32 commitXn();
  static Lng32 rollbackXn();
  static Lng32 statusXn();
  static Lng32 suspendXn();
  static Lng32 resumeXn();

  Lng32 createContext(char* contextHandle); // out buf will return context handle
  Lng32 switchContext(char* contextHandle); // in buf contains context handle
  Lng32 currentContext(char* contextHandle); // out buf will return context handle
  Lng32 deleteContext(char* contextHandle); // in buf contains context handle

  Lng32 retrieveSQLDiagnostics(ComDiagsArea *toDiags);
  ComDiagsArea *allocAndRetrieveSQLDiagnostics(ComDiagsArea *&toDiags);

  CollHeap * getHeap() { return heap_; }

  char * outputBuf() { return outputBuf_; };
  Int32  outputDatalen() { return outputDatalen_; };

  char * inputBuf() { return inputBuf_; };
  Int32  inputDatalen() { return inputDatalen_; };

  void clearGlobalDiags();

  Int32 getIsoMapping() { return isoMapping_; };
  void setIsoMapping(Int32 isoMapping) { isoMapping_ = isoMapping; };

  Lng32 GetRowsAffected(Int64 *rowsAffected);

  Lng32 holdAndSetCQD(const char * defaultName, const char * defaultValue, ComDiagsArea *globalDiags = NULL);

  Lng32 restoreCQD(const char * defaultName, ComDiagsArea *globalDiags = NULL);

  Lng32 getCQDval(const char * defaultName,
		  char * val,
		  ComDiagsArea *globalDiags = NULL);

  void setNotExeUtilInternalQuery(NABoolean v)
    {(v ? flags_ |= NOT_EXEUTIL_INTERNAL_QUERY : flags_ &= ~NOT_EXEUTIL_INTERNAL_QUERY); };
  NABoolean notExeUtilInternalQuery() { return (flags_ & NOT_EXEUTIL_INTERNAL_QUERY) != 0; };

  Lng32 setCQS(const char * shape, ComDiagsArea *globalDiags = NULL);
  Lng32 resetCQS(ComDiagsArea *globalDiags = NULL);

  // methods for routine invocation
  Lng32 getRoutine(
       /* IN */     const char   *serializedInvocationInfo,
       /* IN */     Int32         invocationInfoLen,
       /* IN */     const char   *serializedPlanInfo,
       /* IN */     Int32         planInfoLen,
       /* IN */     Int32         language,
       /* IN */     Int32         paramStyle,
       /* IN */     const char   *externalName,
       /* IN */     const char   *containerName,
       /* IN */     const char   *externalPath,
       /* IN */     const char   *librarySqlName,
       /* OUT */    Int32        *handle,
       /* IN/OUT */ ComDiagsArea *diags);

  Lng32 invokeRoutine(
       /* IN */     Int32         handle,
       /* IN */     Int32         phaseEnumAsInt,
       /* IN */     const char   *serializedInvocationInfo,
       /* IN */     Int32         invocationInfoLen,
       /* OUT */    Int32        *invocationInfoLenOut,
       /* IN */     const char   *serializedPlanInfo,
       /* IN */     Int32         planInfoLen,
       /* IN */     Int32         planNum,
       /* OUT */    Int32        *planInfoLenOut,
       /* IN */     char         *inputRow,
       /* IN */     Int32         inputRowLen,
       /* OUT */    char         *outputRow,
       /* IN */     Int32         outputRowLen,
       /* IN/OUT */ ComDiagsArea *diags);

  Lng32 getRoutineInvocationInfo(
       /* IN */     Int32         handle,
       /* IN/OUT */ char         *serializedInvocationInfo,
       /* IN */     Int32         invocationInfoMaxLen,
       /* OUT */    Int32        *invocationInfoLenOut,
       /* IN/OUT */ char         *serializedPlanInfo,
       /* IN */     Int32         planInfoMaxLen,
       /* IN */     Int32         planNum,
       /* OUT */    Int32        *planInfoLenOut,
       /* IN/OUT */ ComDiagsArea *diags);

  Lng32 putRoutine(
       /* IN */     Int32         handle,
       /* IN/OUT */ ComDiagsArea *diags);

private:
  struct Attrs
    {
    Lng32 fsDatatype_;
    Lng32 nullFlag_;
    Lng32 length_;
    Lng32 varOffset_;
    Lng32 indOffset_;
    };

  SQLMODULE_ID * module_;
  SQLSTMT_ID   * stmt_;
  SQLDESC_ID   * sql_src_;
  SQLDESC_ID   * input_desc_;
  SQLDESC_ID   * output_desc_;
  char         * outputBuf_;
  Int32          isoMapping_;
  Int32          outputDatalen_;

  char * explainData_;
  Int32 explainDataLen_;

  Int32          numInputEntries_;
  Int32          numOutputEntries_;
  struct Attrs * inputAttrs_;
  struct Attrs * outputAttrs_;

  SQLDESC_ID   * rs_input_maxsize_desc_;
  Int32          rs_maxsize_;

  char         * inputBuf_;
  Int32          inputDatalen_;

  SQLMODULE_ID * moduleWithCK_;
  SQLSTMT_ID   * stmtWithCK_;
  SQLDESC_ID   * sql_src_withCK_;
  SQLDESC_ID   * input_desc_withCK_;
  SQLDESC_ID   * output_desc_withCK_;
  char         * outputBuf_withCK_;

  // variables to process rowwise rowset
  Int32 rsMaxsize_; // max number of of rows in a rowset
  char * rsInputBuffer_; // rwrs buffer passed to sql/cli
  Int32 currRSrow_; // current number of rows in the rsInputBuffer_

  Int32 numQuadFields_;
  struct SQLCLI_QUAD_FIELDS * quadFields_;

  CollHeap * heap_;

  ContextCli * currContext_;
  const char * parentQid_;

  Lng32 flags_;
};


#endif
