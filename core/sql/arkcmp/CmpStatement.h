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
 * File:         CmpStatement.h
 * Description:  This file contains the declaration of the routines to process
 *               requests from executor.
 *               
 *               
 * Created:      06/24/96
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */



#ifndef CmpStatement_H
#define CmpStatement_H

#include "CmpMessage.h"
#include "CmpContext.h"
#include "ComDiags.h"
#include "CmpErrors.h"
#include "OperTypeEnum.h"

// forward declaration
class CmpStoredProc;
class CmpMain;
namespace tmudr {
  class UDRInvocationInfo;
  class UDRPlanInfo;
}
class DDLExpr;
class ExprNode;
class QueryAnalysis;
class CostMethod;
class NAMemory;
class CompilationStats;
class OptGlobals;
class CqsWA;
class CommonSubExprRef;
class ValueIdList;
class ValueIdSet;
class RelExpr;
class CSEInfo;

// contents
class CmpStatement;
class CmpStatementISP;
class CmpStatementISPGetNext;

typedef NASimpleArray<NAString*>                   NAStringList;

class CmpStatement : public NABasicObject
{
public:
  enum ReturnStatus 
    { CmpStatement_SUCCESS, CmpStatement_ERROR };

  // constructor  
  // memoryType parameter is ignored 
  CmpStatement(
      CmpContext*,
      NAMemory *outHeap = NULL);
  
  // requests process
  ReturnStatus process(const CmpMessageObj&);
  ReturnStatus process(const CmpMessageEnvs&);
  ReturnStatus process(const CmpMessageSQLText&);
  ReturnStatus process(const CmpMessageCompileStmt&);
  ReturnStatus process(const CmpMessageDDL&);
  ReturnStatus process(const CmpMessageDescribe&);
  ReturnStatus process(const CmpMessageUpdateHist&);
  ReturnStatus process(const CmpMessageSetTrans&);
  ReturnStatus process(const CmpMessageDDLNATableInvalidate&);
  ReturnStatus process(const CmpMessageDatabaseUser&);
  ReturnStatus process(const CmpMessageEndSession&);
  ReturnStatus process(const CmpMessageDDLwithStatus&);

  // retrieve the reply
  CmpMessageReply* reply() { return reply_; }

  // retrieve the diags
  ComDiagsArea* diags() { return context_->diags(); }

  // retrieve the envs
  ProcessEnv* envs() { return context_->envs(); }      
 
  // methods for prvStatement
  CmpStatement* prvCmpStatement() { return prvCmpStatement_; }
  void setPrvCmpStatement(CmpStatement* s) { prvCmpStatement_ = s; }

  // retrieve the NAMemory*
  NAHeap* heap() { return heap_; }

  // get user sql query
  char* userSqlText() const { return sqlTextStr_; }
  Lng32 userSqlTextLen() const { return sqlTextLen_; }
  Lng32 userSqlTextCharSet() const { return sqlTextCharSet_; }

  // set user sql query
  void setUserSqlText(char * sqlTextStr)
  {
    sqlTextStr_ = sqlTextStr;
  }
  void setSqlTextLen(Lng32 txtLen) { sqlTextLen_ = txtLen; }
  void setSqlTextCharSet(Lng32 charSet) { sqlTextCharSet_ = charSet; }

  void setSMDRecompile(NABoolean TorF ) {isSMDRecompile_ = TorF;}
  // set the exceptionRaised_ flag;
  void exceptionRaised();
  NABoolean exception() const { return exceptionRaised_; }

  // member functions for stored procedure handles
  const CmpStoredProc* storedProc() const { return storedProc_; }
  CmpStoredProc* setStoredProc(CmpStoredProc* p);

  // this method will return the deletion status of CmpStatement, 
  // if TRUE, means this CmpStatement is ready to be deleted.
  // if FALSE, means this CmpStatement should be kept around
  // for more requests coming from executor.
  virtual NABoolean readyToDie() { return TRUE; }
  virtual CmpStatementISP* ISPStatement() { return 0; }

  // destructor
  virtual ~CmpStatement();

  NABoolean error(Lng32 no=arkcmpErrorNoDiags, const char* s=0);

  //is this compilation a re-compilation
  inline NABoolean recompiling(){return recompiling_;};
  
  // is this a special case of SMD query recompilation
  inline NABoolean isSMDRecompile() {return isSMDRecompile_;}
  //is this statement a DDL statement
  inline NABoolean isDDL(){return isDDL_;}

  QueryAnalysis* getQueryAnalysis() { return queryAnalysis_; };
  QueryAnalysis* initQueryAnalysis();

  void prepareForCompilationRetry();
  Int32 getNumOfCompilationRetries() const { return numOfCompilationRetries_; }

  // statement shape rewrite
  CqsWA* getCqsWA()   { return cqsWA_; }
  void   initCqsWA();  
  void   clearCqsWA(); 

  CostMethod* getCostMethodHead() { return CostMethodHead_; };
  void setCostMethodHead(CostMethod* x) { CostMethodHead_ = x; };

  OperatorTypeEnum getItemExprOrigOpTypeBeingBound() const 
   { return ItemExprOrigOpTypeBeingBound_; };

  void setItemExprOrigOpTypeBeingBound(OperatorTypeEnum x) 
    { ItemExprOrigOpTypeBeingBound_ = x; };

  Int32& getItemExprOrigOpTypeCounter() { return ItemExprOrigOpTypeCounter_; };
  //void setItemExprOrigOpTypeCounter(Int32 x) { ItemExprOrigOpTypeCounter_ = x; };

  char * getLocalizedTextBuf() { return localizedTextBuf_ ; }
  void   setLocalizedTextBuf( char * newBuf ) { localizedTextBuf_ = newBuf ; }

  size_t getLocalizedTextBufSize() { return localizedTextBufSize_ ; }
  void   setLocalizedTextBufSize( size_t newSiz ) { localizedTextBufSize_ = newSiz ; }

  // queue of literals from SQL statement
  NAStringList& getParserStmtLiteralList() { return parserStmtLiteralList_; };

  // return the CompilationStats object that is recording the compilation stats for this statement
  CompilationStats* getCompilationStats () { return compStats_; }
      
  // optimizer globals
  OptGlobals*  getOptGlobals()  { return optGlobals_; }
 
  TaskMonitor* getSimpleFSOMonPtr() { return simpleFSOTaskMonitor_;}
  TaskMonitor* getComplexFSOMonPtr()  { return complexFSOTaskMonitor_; }

  // controls for the compiler graphical debugging tool
  NABoolean displayGraph () { return displayGraph_; }
  void setDisplayGraph (NABoolean val) { displayGraph_ = val; }
  void clearDisplayGraph () { displayGraph_ = FALSE; }
  
  // objects allocated from system heap, delete when done with the statement
  void addUDRInvocationInfoToDelete(tmudr::UDRInvocationInfo *deleteThisAfterCompilation);
  void addUDRPlanInfoToDelete(tmudr::UDRPlanInfo *deleteThisAfterCompilation);

  // help in diagnosing failed compilation with TMUDFs
  NABoolean getTMUDFRefusedRequirements() const { return detailsOnRefusedRequirements_ != NULL; }
  const LIST(const NAString *) *getDetailsOnRefusedRequirements() const
                                            { return detailsOnRefusedRequirements_; }
  void setTMUDFRefusedRequirements(const char *details);

  short getDDLExprAndNode(char * sqlStr, Lng32 inputCS,
                          DDLExpr* &ddlExpr, ExprNode* &ddlNode);

  CSEInfo *getCSEInfo(const char *cseName) const;
  CSEInfo *getCSEInfoForMainQuery() const;
  static Int32 getCSEIdForMainQuery() { return 0; }
  CSEInfo *getCSEInfoById(Int32 cseId) const;
  const LIST(CSEInfo *) *getCSEInfoList() const { return cses_; }
  void addCSEInfo(CSEInfo *info);

  // context global empty input logical property
  EstLogPropSharedPtr* getGEILP() { return &emptyInLogProp_; }
    
protected:
  // CmpStatement(const CmpStatement&); please remove this line
  CmpStatement& operator=(const CmpStatement&);

  // internal helper routines

  // The CmpContext this CmpStatement belongs to
  CmpContext* context_;
  // The previous statement. To maintain a stack of CmpStatement.
  CmpStatement* prvCmpStatement_;

  // The statement heap
  NAHeap* heap_;

  // This heap is used to allocate the reply_ object, since this object
  // will be deleted outside the this routine from executor or arkcmp.
  NAMemory * outHeap_;

  // The reply to be sent back to executor after processing the request in CmpStatement
  CmpMessageReply* reply_;

  // The result of Compile for statements like invoke, get tables, show stats
  // that calls CmpDescribe internally immediately after compilation.

  CmpMessageReply *bound_;

  // The flag to record whether exception has been raised in the
  // statement compilation/execution. This is used to clean up properly once the 
  // exception is raised ( especially when longjmp occurred )
  NABoolean exceptionRaised_;

  // The CmpStoredProc pointer. This is used in execution ( and compilation ) of
  // the stored procedure. The reason to keep this pointer here, instead of locally in 
  // process routine is because :
  // 1. The process routine might be called multiple times for the reply to support 
  //    multiple reply feature for a single stored procedure request. While to the 
  //    implementation of the stored procedure, it still keeps the same handle.
  // 2. In the case of exception, the connection to CmpStoredProc need to be closed. 
  //    It should be done for both compilation and execution of the stored procedure 
  //    request. Since we are using longjmp now, which does not clean up the stack afterwards,
  //    it is very important to clean up this member in exception.
  // Note : Since the storedProc_ might be deleted in this class, the caller who sets this 
  // member should make sure delete storedProc; works. i.e. It has to be allocated correctly, 
  // and once it is deleted, set this member to 0, so it won't be deleted more than once.
  CmpStoredProc* storedProc_;

private:

  // SQL Query in text form; used for debugging purposes
  char * sqlTextStr_;
  Lng32  sqlTextLen_;
  Lng32  sqlTextCharSet_;

  //flag, indicates if this is a recompilation
  NABoolean recompiling_;

  NABoolean isSMDRecompile_;
  //flag, indicates if this is a DDL statment
  NABoolean isDDL_;

  // CompilationStats object that is recording the compilation stats for this statement
  CompilationStats* compStats_;

  // globals used during query optimization
  OptGlobals* optGlobals_;

  // force a shape
  CqsWA*       cqsWA_;

  QueryAnalysis* queryAnalysis_;

  CostMethod* CostMethodHead_; 

  OperatorTypeEnum ItemExprOrigOpTypeBeingBound_;
  Int32 ItemExprOrigOpTypeCounter_;

  NAStringList parserStmtLiteralList_;

  char * localizedTextBuf_ ;
  size_t localizedTextBufSize_ ;

  TaskMonitor* simpleFSOTaskMonitor_;
  TaskMonitor* complexFSOTaskMonitor_;

  // The attribute displayGraph_ is used for sensing whether the user wants
  // to display the query tree during optimization. Certain methods
  // on RelExpr are enabled only when it is set.
  NABoolean displayGraph_;

  // common subexpressions in this statement, there could
  // be multiple, named CSEs, each with one or more references
  LIST(CSEInfo *) *cses_;

  // for error reporting for UDFs, keep a list of requirements the UDF refused
  LIST(const NAString *) *detailsOnRefusedRequirements_;

  // indicates whether we are retrying the compile in
  // CmpMain::sqlcomp(QueryText, ...
  Int32 numOfCompilationRetries_;

  // context global empty input logical property
  EstLogPropSharedPtr emptyInLogProp_;

}; // end of CmpStatement

class CmpStatementISP: public CmpStatement
{
public:
  CmpStatementISP(CmpContext*, NAMemory * outHeap=0);
  virtual ~CmpStatementISP();

  ReturnStatus process(CmpMessageISPRequest&);
  ReturnStatus process(const CmpMessageISPGetNext&);

  virtual NABoolean readyToDie();
  virtual CmpStatementISP* ISPStatement() { return this; }

  // returns TRUE if expecting more data to be fetched from the internalsp in this CmpStatement
  NABoolean moreData();  
  // member to retrieve the ISP reqeust ID which this CmpStatementISP is processing
  Int64 ISPReqId() const { return ISPReqId_; }

private:

  // the execution of the isp for certain ISP request of CmpMessageObject.
  Int64 ISPReqId_;

  CmpStatementISP(const CmpStatementISP&);
  CmpStatementISP& operator=(const CmpStatementISP&);
}; // end of CmpStatementISP


#endif
