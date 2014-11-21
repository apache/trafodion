/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 1994-2014 Hewlett-Packard Development Company, L.P.
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
#ifndef RELEXEUTIL_H
#define RELEXEUTIL_H
/* -*-C++-*-
******************************************************************************
*
* File:         RelExeUtil.h
* Description:  Miscellaneous operators used as part of ExeUtil operation
* Created:      10/17/2008
* Language:     C++
*
*
*
*
******************************************************************************
*/

#include "ObjectNames.h"
#include "RelExpr.h"
#include "SQLCLIdev.h"
#include "OptUtilIncludes.h"
#include "BinderUtils.h"
#include "StmtNode.h"
#include "charinfo.h"
#include "RelFastTransport.h"
//#include "LateBindInfo.h"


// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class GenericUtilExpr;
class ExeUtilExpr;

NABoolean ExeUtilReplicate_checkReplicateCQD(Int32 userID);

// -----------------------------------------------------------------------
// This class is the base class for sql statements which are processed and
// evaluated at runtime. They could be DDL statements, or other stmts
// which are evaluated using other cli calls(like LOAD).
// They could be evaluated by executor(ex, LOAD) or mxcmp(ex, DDL).
// This class is different than RelInternalSP which is evaluated by
// making open/fetch/close calls to stored proc functions in mxcmp.
// -----------------------------------------------------------------------
class GenericUtilExpr : public RelExpr
{
public:
  GenericUtilExpr(char * stmtText,
		  CharInfo::CharSet stmtTextCharSet,
		  ExprNode * exprNode,
		  RelExpr * child,
		  OperatorTypeEnum otype,
		  CollHeap *oHeap = CmpCommon::statementHeap())
    : RelExpr(otype, child, NULL, oHeap),
      stmtTextCharSet_(stmtTextCharSet),
      heap_(oHeap),
      exprNode_(exprNode),
      xnNeeded_(TRUE)
  {
    setNonCacheable();
    if (stmtText)
      {
	stmtText_ = new(oHeap) char[strlen(stmtText)+1];
	strcpy(stmtText_, stmtText);
      }
    else
      stmtText_ = NULL;
  };

  virtual RelExpr * bindNode(BindWA *bindWAPtr);

  // method to do code generation
  RelExpr * preCodeGen(Generator * generator,
		       const ValueIdSet & externalInputs,
		       ValueIdSet &pulledNewInputs);
  virtual short codeGen(Generator*);

  // The set of values that I can potentially produce as output.
  virtual void getPotentialOutputValues(ValueIdSet & vs) const;

  // cost functions
  virtual PhysicalProperty *synthPhysicalProperty(const Context *context,
						  const Lng32     planNumber);

  // this is both logical and physical node
  virtual NABoolean isLogical() const{return TRUE;};
  virtual NABoolean isPhysical() const{return TRUE;};

  // various PC methods

  // get the degree of this node 
  virtual Int32 getArity() const { return 0; };
  virtual RelExpr * copyTopNode(RelExpr *derivedNode = NULL,
				CollHeap* outHeap = 0);

  // method required for traversing an ExprNode tree
  // access a child of an ExprNode
  //virtual ExprNode * getChild(long index);

  ExprNode * getExprNode(){return exprNode_;};

  //virtual void addLocalExpr(LIST(ExprNode *) &xlist,
  //		    LIST(NAString) &llist) const;
  NABoolean &xnNeeded() { return xnNeeded_;}

  char * getStmtText()
  {
    return stmtText_;
  }

  CharInfo::CharSet getStmtTextCharSet() const
  {
    return stmtTextCharSet_;
  }

  void setStmtText(char * stmtText, CharInfo::CharSet stmtTextCS)
  {
    if (stmtText_)
      NADELETEBASIC(stmtText_, heap_);

    if (stmtText)
      {
	stmtText_ = new(heap_) char[strlen(stmtText)+1];
	strcpy(stmtText_, stmtText);
      }
    else
      stmtText_ = NULL;
    stmtTextCharSet_ = stmtTextCS;
  }

  virtual NABoolean dontUseCache() { return FALSE; }
private:
  // the parse tree version of the statement, if needed
  ExprNode * exprNode_;

  // the string version of the statement.
  char *     stmtText_;
  CharInfo::CharSet stmtTextCharSet_;

  CollHeap * heap_;

  // should master exe start a xn before executing this request?
  NABoolean xnNeeded_;
}; // class GenericUtilExpr

// -----------------------------------------------------------------------
// The DDLExpr class is used to represent a DDL query that is treated
// like a DML. It points to the actual parse tree representing the DDL
// query.
// -----------------------------------------------------------------------
class DDLExpr : public GenericUtilExpr
{
public:
  DDLExpr(ExprNode * ddlNode,
	  char * ddlStmtText,
	  CharInfo::CharSet ddlStmtTextCharSet,
	  NABoolean forShowddlExplain = FALSE,
	  NABoolean internalShowddlExplain = FALSE,
	  NABoolean noLabelStats = FALSE,
	  QualifiedName * explObjName = NULL,
	  double numExplRows = 0,
	  CollHeap *oHeap = CmpCommon::statementHeap())
    : GenericUtilExpr(ddlStmtText, ddlStmtTextCharSet, ddlNode, NULL, REL_DDL, oHeap),
    mpRequest_ (FALSE),
    specialDDL_(FALSE),
    ddlObjNATable_(NULL),
    forShowddlExplain_(forShowddlExplain),
    internalShowddlExplain_(internalShowddlExplain),
    noLabelStats_(noLabelStats),
    numExplRows_(numExplRows),
    isCreate_(FALSE), isCreateLike_(FALSE), isVolatile_(FALSE), 
    isDrop_(FALSE), isAlter_(FALSE),
    isTable_(FALSE), isIndex_(FALSE), isMV_(FALSE), isView_(FALSE),
      isLibrary_(FALSE), isRoutine_(FALSE),
    isUstat_(FALSE),
    isHbase_(FALSE),
    isNative_(FALSE),
    initHbase_(FALSE),
    dropHbase_(FALSE),
    updateVersion_(FALSE),
    purgedataHbase_(FALSE),
    initAuthorization_(FALSE),
    dropAuthorization_(FALSE),
    addSeqTable_(FALSE),
    flags_(0)
  {
    if (explObjName)
      explObjName_ = *explObjName;
  };

 DDLExpr(NABoolean initHbase, NABoolean dropHbase,
	 NABoolean createMDviews, NABoolean dropMDviews,
         NABoolean initAuthorization, NABoolean dropAuthorization,
	 NABoolean addSeqTable, NABoolean updateVersion,
	 char * ddlStmtText,
	 CharInfo::CharSet ddlStmtTextCharSet,
	  CollHeap *oHeap = CmpCommon::statementHeap())
   : GenericUtilExpr(ddlStmtText, ddlStmtTextCharSet, NULL, NULL, REL_DDL, oHeap),
    mpRequest_ (FALSE),
    specialDDL_(FALSE),
    ddlObjNATable_(NULL),
    forShowddlExplain_(FALSE),
    internalShowddlExplain_(FALSE),
    noLabelStats_(FALSE),
    numExplRows_(0),
    isCreate_(FALSE), isCreateLike_(FALSE), isVolatile_(FALSE), 
    isDrop_(FALSE), isAlter_(FALSE),
    isTable_(FALSE), isIndex_(FALSE), isMV_(FALSE), isView_(FALSE),
    isLibrary_(FALSE), isRoutine_(FALSE),
    isUstat_(FALSE),
    isHbase_(FALSE),
    isNative_(FALSE),
    initHbase_(initHbase), 
    dropHbase_(dropHbase),
    updateVersion_(updateVersion),
    purgedataHbase_(FALSE),
    initAuthorization_(initAuthorization),
    dropAuthorization_(dropAuthorization),
    addSeqTable_(addSeqTable),
    flags_(0)
  {
    if (createMDviews)
      setCreateMDViews(TRUE);
    
    if (dropMDviews)
      setDropMDViews(TRUE);
  };

 DDLExpr(NABoolean purgedataHbase,
	 CorrName &purgedataTableName,
	 char * ddlStmtText,
	 CharInfo::CharSet ddlStmtTextCharSet,
	 CollHeap *oHeap = CmpCommon::statementHeap())
   : GenericUtilExpr(ddlStmtText, ddlStmtTextCharSet, NULL, NULL, REL_DDL, oHeap),
    mpRequest_ (FALSE),
    specialDDL_(FALSE),
    ddlObjNATable_(NULL),
    forShowddlExplain_(FALSE),
    internalShowddlExplain_(FALSE),
    noLabelStats_(FALSE),
    numExplRows_(0),
    isCreate_(FALSE), isCreateLike_(FALSE), isVolatile_(FALSE), 
    isDrop_(FALSE), isAlter_(FALSE),
    isTable_(FALSE), isIndex_(FALSE), isMV_(FALSE), isView_(FALSE),
    isUstat_(FALSE),
    isHbase_(FALSE),
    isNative_(FALSE),
    initHbase_(FALSE), 
    dropHbase_(FALSE),
    updateVersion_(FALSE),
    purgedataHbase_(purgedataHbase),
    initAuthorization_(FALSE),
    dropAuthorization_(FALSE),
    addSeqTable_(FALSE),
    flags_(0)
  {
    purgedataTableName_ = purgedataTableName;
    qualObjName_ = purgedataTableName.getQualifiedNameObj();
  };

  virtual RelExpr * copyTopNode(RelExpr *derivedNode = NULL,
				CollHeap* outHeap = 0);
  virtual const NAString getText() const;

  virtual void unparse(NAString &result,
		       PhaseEnum /* phase */,
		       UnparseFormatEnum /* form */,
		       TableDesc * tabId = NULL) const;

  virtual RelExpr * preCodeGen(Generator * generator,
			       const ValueIdSet & externalInputs,
			       ValueIdSet &pulledNewInputs);

  virtual RelExpr * bindNode(BindWA *bindWAPtr);

  // method to do code generation
  virtual short codeGen(Generator*);

  ExplainTuple *addSpecificExplainInfo(ExplainTupleMaster *explainTuple, 
				       ComTdb * tdb, 
				       Generator *generator);

  ExprNode * getDDLNode(){return getExprNode();};

  NABoolean &mpRequest() { return mpRequest_;}

  char * getDDLStmtText()
  {
    return getStmtText();
  }

  Lng32 getDDLStmtTextCharSet() const
  {
    return getStmtTextCharSet();
  }

  CorrName &purgedataTableName() { return purgedataTableName_; }

  NABoolean &specialDDL() { return specialDDL_;}
  NABoolean &isUstat() { return isUstat_; }

  NABoolean forShowddlExplain() { return forShowddlExplain_; }
  NABoolean internalShowddlExplain() { return internalShowddlExplain_; }
  NABoolean noLabelStats() { return noLabelStats_; }
  void setNoLabelStats(NABoolean v) { noLabelStats_ = v; }

  QualifiedName &explObjName() { return explObjName_; }

  double numExplRows() { return numExplRows_; }

  virtual NABoolean dontUseCache() 
  { return (forShowddlExplain() ? FALSE : TRUE); }

  NABoolean initHbase() { return initHbase_; }
  NABoolean dropHbase() { return dropHbase_; }
  NABoolean updateVersion() { return updateVersion_; }
  NABoolean purgedataHbase() { return purgedataHbase_; }
  NABoolean initAuthorization() { return initAuthorization_; }
  NABoolean dropAuthorization() { return dropAuthorization_; }
  NABoolean addSeqTable() { return addSeqTable_; }

  NAString getQualObjName() { return qualObjName_.getQualifiedNameAsString(); }

  void setCreateMDViews(NABoolean v)
  {(v ? flags_ |= CREATE_MD_VIEWS : flags_ &= ~CREATE_MD_VIEWS); }
  NABoolean createMDViews() { return (flags_ & CREATE_MD_VIEWS) != 0;}

  void setDropMDViews(NABoolean v)
  {(v ? flags_ |= DROP_MD_VIEWS : flags_ &= ~DROP_MD_VIEWS); }
  NABoolean dropMDViews() { return (flags_ & DROP_MD_VIEWS) != 0;}

  void setGetMDVersion(NABoolean v)
  {(v ? flags_ |= GET_MD_VERSION : flags_ &= ~GET_MD_VERSION); }
  NABoolean getMDVersion() { return (flags_ & GET_MD_VERSION) != 0;}

 protected:
  enum Flags
  {
    CREATE_MD_VIEWS      = 0x0001,
    DROP_MD_VIEWS          = 0x0002,
    GET_MD_VERSION        = 0x0004
  };

  // is this a DDL operation to work on SQL/MP tables?
  NABoolean mpRequest_;

  // see method processSpecialDDL in sqlcomp/parser.cpp
  NABoolean specialDDL_;

  // NATable for the DDL object. Used to generate EXPLAIN information.
  // Set during bindNode phase.
  NATable * ddlObjNATable_;

  // this ddlexpr is created for 'showddl <obj>, explain' to
  // explain the object explObjName.
  NABoolean forShowddlExplain_;
  NABoolean internalShowddlExplain_;
  NABoolean noLabelStats_;
  QualifiedName explObjName_;
  double numExplRows_;  // number of rows specified by user or actual count

  NABoolean isCreate_;
  NABoolean isCreateLike_;
  NABoolean isVolatile_;
  NABoolean isTable_;
  NABoolean isIndex_;
  NABoolean isMV_;
  NABoolean isView_;
  NABoolean isLibrary_;
  NABoolean isRoutine_;
  NABoolean isUstat_;    // specialDDL_ for Update Statistics

  NABoolean isHbase_; // a trafodion or hbase operation
  NABoolean isNative_; // an operation directly on a native hbase table, like
                                  // creating an hbase table from trafodion interface.
  NABoolean initHbase_;	  
  NABoolean dropHbase_;	
  NABoolean updateVersion_;
  NABoolean purgedataHbase_;
  NABoolean initAuthorization_;
  NABoolean dropAuthorization_;
  NABoolean addSeqTable_;

  // if set, this ddl cannot run under a user transaction. It must run in autocommit
  // mode.
  NABoolean hbaseDDLNoUserXn_;

  // set for create table index/MV only. Used during InMemory table
  // processing.	  
  NAString objName_;

  NABoolean isDrop_;
  NABoolean isAlter_;

  CorrName purgedataTableName_;
  QualifiedName qualObjName_;

  UInt32 flags_;
};

// -----------------------------------------------------------------------
// The ExeUtilExpr class is used to represent queries which are evaluated
// by executor at runtime to implement various operations.
//
// If type_ is LOAD, then this query is
// used to load the target table using sidetree inserts.
// At runtime, the table is made unaudited, data is loaded and the table
// is made audited.
// -----------------------------------------------------------------------
class ExeUtilExpr : public GenericUtilExpr
{
public:
  enum ExeUtilType
  {
    LOAD_                     = 0,
    REORG_                    = 1,
    DISPLAY_EXPLAIN_          = 2,
    MAINTAIN_OBJECT_          = 3,
    LOAD_VOLATILE_            = 4,
    CLEANUP_VOLATILE_TABLES_  = 5,
    GET_VOLATILE_INFO_        = 6,
    CREATE_TABLE_AS_          = 7,
    FAST_DELETE_              = 8,
    GET_STATISTICS_           = 9,
    USER_LOAD_                = 10,
    LONG_RUNNING_             = 11,
    GET_METADATA_INFO_        = 12,
    GET_VERSION_INFO_         = 13,
    SUSPEND_ACTIVATE_         = 14,
    SHOWSET_DEFAULTS_         = 18,
    AQR_                      = 19,
    DISPLAY_EXPLAIN_COMPLEX_  = 20,
    GET_UID_                  = 21,
    POP_IN_MEM_STATS_         = 22,
    REPLICATE_                = 23,
    ST_INSERT_                = 24,
    GET_ERROR_INFO_           = 25,
    LOB_EXTRACT_              = 26,
    LOB_SHOWDDL_              = 27,
    HBASE_DDL_                = 28,
    HBASE_COPROC_AGGR_        = 29,
    WNR_INSERT_               = 30,
    METADATA_UPGRADE_         = 31,
    HBASE_LOAD_               = 32,
    HBASE_LOAD_TASK_          = 33,
    AUTHORIZATION_            = 34,
    HBASE_UNLOAD_             = 35,
    HBASE_UNLOAD_TASK_        = 36,
    ORC_FAST_AGGR_            = 37

  };

  ExeUtilExpr(ExeUtilType type,
	      const CorrName &name,
	      ExprNode * exprNode,
	      RelExpr * child,
	      char * stmtText,
	      CharInfo::CharSet stmtTextCharSet,
	      CollHeap *oHeap = CmpCommon::statementHeap())
    : GenericUtilExpr(stmtText, stmtTextCharSet, exprNode, child, REL_EXE_UTIL, oHeap),
	 type_(type),
	 tableName_(name, oHeap),
	 tableId_(NULL),
	 virtualTabId_(NULL)
  {
  };

  virtual RelExpr * copyTopNode(RelExpr *derivedNode = NULL,
				CollHeap* outHeap = 0);
  virtual const NAString getText() const;

  virtual RelExpr * bindNode(BindWA *bindWAPtr);

  virtual RelExpr * preCodeGen(Generator * generator,
			       const ValueIdSet & externalInputs,
			       ValueIdSet &pulledNewInputs);

  NABoolean pilotAnalysis(QueryAnalysis* qa);

  virtual void getPotentialOutputValues(ValueIdSet &vs) const;

  virtual NABoolean producesOutput() { return FALSE; }

  // exeutil statements whose query type need to be returned as 
  // SQL_EXE_UTIL. Set during RelRoot::codeGen in ComTdbRoot class.
  virtual NABoolean isExeUtilQueryType() { return FALSE; }

  virtual NABoolean aqrSupported() { return FALSE; }

  virtual const char 	*getVirtualTableName();
  virtual desc_struct 	*createVirtualTableDesc();
  TableDesc * getVirtualTableDesc() const
  {
    return virtualTabId_;
  };

  void setVirtualTableDesc(TableDesc * vtdesc)
  {
    virtualTabId_ = vtdesc;
  };

  virtual void unparse(NAString &result,
		       PhaseEnum /* phase */,
		       UnparseFormatEnum /* form */,
		       TableDesc * tabId = NULL) const;

  ExeUtilType getExeUtilType() { return type_; }

  const CorrName &getTableName() const         { return tableName_; }
  CorrName &getTableName()                     { return tableName_; }

  TableDesc     *getUtilTableDesc() const { return tableId_; }
  void          setUtilTableDesc(TableDesc *newId)  
  { tableId_ = newId; }

  virtual NABoolean explainSupported() { return FALSE; }

  virtual NABoolean dontUseCache() { return FALSE; }

protected:
  ExeUtilType type_;

  // name of the table affected by the operation
  CorrName tableName_;

  // a unique identifer for the table specified by tableName_
  TableDesc *tableId_;  

  // a unique identifer for the virtual table
  TableDesc * virtualTabId_;

};

class ExeUtilDisplayExplain : public ExeUtilExpr
{
public:
  ExeUtilDisplayExplain(ExeUtilType opType,
			char * stmtText,
			CharInfo::CharSet stmtTextCharSet,
			char * moduleName = NULL,
			char * stmtName = NULL,
			char * optionsStr = NULL,
			ExprNode * exprNode = NULL,
			CollHeap *oHeap = CmpCommon::statementHeap());

  virtual RelExpr * copyTopNode(RelExpr *derivedNode = NULL,
				CollHeap* outHeap = 0);

  virtual RelExpr * bindNode(BindWA *bindWAPtr);

  // method to do code generation
  virtual short codeGen(Generator*);

  virtual const char 	*getVirtualTableName();
  virtual desc_struct 	*createVirtualTableDesc();

  virtual NABoolean producesOutput() { return TRUE; }

protected:
  short setOptionsX();

  char * moduleName_;
  char * stmtName_;
  char * optionsStr_;

  char optionX_;                // explain [options 'x'] storage
};

class ExeUtilDisplayExplainComplex : public ExeUtilDisplayExplain
{
public:
  enum ComplexExplainType
  {
    CREATE_TABLE_,
    CREATE_INDEX_,
    CREATE_MV_,
    CREATE_TABLE_AS
  };

  ExeUtilDisplayExplainComplex(char * stmtText,
			       CharInfo::CharSet stmtTextCharSet,
			       char * optionsStr = NULL,
			       ExprNode * exprNode = NULL,
			       CollHeap *oHeap = CmpCommon::statementHeap());

  virtual RelExpr * copyTopNode(RelExpr *derivedNode = NULL,
				CollHeap* outHeap = 0);

  virtual RelExpr * bindNode(BindWA *bindWAPtr);

  // method to do code generation
  virtual short codeGen(Generator*);

private:
  ComplexExplainType type_;

  ////////////////////////////////////////////////////////////////
  // CREATE TABLE:
  //  qry1_  ==> create table in memory...
  //  qry2_  ==> explain create table...
  //
  // CREATE INDEX:
  //  qry1_  ==> create index in memory...
  //  qry2_  ==> explain create index...
  //
  // CREATE TABLE AS:
  //  qry1_  ==> create table in memory...
  //  qry2_  ==> explain create table...
  //  qry3_  ==> explain insert using vsbb...
  //  qry4_  ==> explain insert using sideinserts...
  //
  ////////////////////////////////////////////////////////////////
  NAString qry1_;
  NAString qry2_;
  NAString qry3_;
  NAString qry4_;

  NAString objectName_;

  // CREATE of a volatile table/index.
  NABoolean isVolatile_;
};

class ExeUtilLoad : public ExeUtilExpr
{
public:
  ExeUtilLoad(const CorrName &name,
	      ExprNode * exprNode,
	      char * stmtText,
	      CharInfo::CharSet stmtTextCharSet,
	      CollHeap *oHeap = CmpCommon::statementHeap())
       : ExeUtilExpr(LOAD_, name, exprNode, NULL, stmtText, stmtTextCharSet, oHeap)
  {
  };

  virtual RelExpr * copyTopNode(RelExpr *derivedNode = NULL,
				CollHeap* outHeap = 0);

  // method to do code generation
  virtual short codeGen(Generator*);

private:
};

class ExeUtilUserLoad : public ExeUtilExpr
{
public:
  enum UserLoadOptionType
  {
    OLD_LOAD_,
    WITH_SORT_,
    NOT_ATOMIC_,
    EXCEPTION_TABLE_,
    EXCEPTION_ROWS_PERCENTAGE_,
    EXCEPTION_ROWS_NUMBER_,
    LOAD_ID_,
    INGEST_MASTER_,
    INGEST_SOURCE_
  };

  class UserLoadOption
  {
    friend class ExeUtilUserLoad;
  public:
    UserLoadOption(UserLoadOptionType option, Int64 numericVal,
		   void * stringVal)
	 : option_(option), numericVal_(numericVal), stringVal_(stringVal)
    {}

  private:
    UserLoadOptionType option_;
    Int64   numericVal_;
    void * stringVal_;
  };

  ExeUtilUserLoad(const CorrName &name,
		  RelExpr * child,
		  ItemExpr * eodExpr,
		  ItemExpr * partnNumExpr,
		  NAList<UserLoadOption*> * userLoadOptionsList,
		  CollHeap *oHeap = CmpCommon::statementHeap());

  virtual RelExpr * copyTopNode(RelExpr *derivedNode = NULL,
				CollHeap* outHeap = 0);

  virtual RelExpr * bindNode(BindWA *bindWAPtr);

  virtual Int32 getArity() const { return 1; };

  virtual RelExpr * preCodeGen(Generator * generator,
			       const ValueIdSet & externalInputs,
			       ValueIdSet &pulledNewInputs);

  // method to do code generation
  virtual short codeGen(Generator*);

  void setEodExpr(ItemExpr * ee) 
  { 
    if (eodExpr_ == NULL)
      eodExpr_ = ee; 
  };

  ItemExpr *rwrsInputSizeExpr()      { return rwrsInputSizeExpr_;}
  ItemExpr *rwrsMaxInputRowlenExpr() { return rwrsMaxInputRowlenExpr_;}
  ItemExpr *rwrsBufferAddrExpr()     { return rwrsBufferAddrExpr_;}
  ValueIdList &rwrsInputParamVids()  { return rwrsInputParamVids_; }

  void setRwrsInputSizeExpr(ItemExpr * v)      {rwrsInputSizeExpr_ = v;}
  void setRwrsMaxInputRowlenExpr(ItemExpr * v) {rwrsMaxInputRowlenExpr_ = v;}
  void setRwrsBufferAddrExpr(ItemExpr * v)     {rwrsBufferAddrExpr_ = v;}

  CorrName excpTabNam() { return excpTabNam_; }

  void setDoFastLoad(NABoolean v)
  {(v ? flags_ |= DO_FAST_LOAD : flags_ &= ~DO_FAST_LOAD); }
  NABoolean doFastLoad() { return (flags_ & DO_FAST_LOAD) != 0;}

  void setDoSortFromTop(NABoolean v)
  {(v ? flags_ |= SORT_FROM_TOP : flags_ &= ~SORT_FROM_TOP); }
  NABoolean sortFromTop() { return (flags_ & SORT_FROM_TOP) != 0;}

  void setIngestMaster(NABoolean v)
  {(v ? flags_ |= INGEST_MASTER : flags_ &= ~INGEST_MASTER); }
  NABoolean ingestMaster() { return (flags_ & INGEST_MASTER) != 0;}

  void setIngestSource(NABoolean v)
  {(v ? flags_ |= INGEST_SOURCE : flags_ &= ~INGEST_SOURCE); }
  NABoolean ingestSource() { return (flags_ & INGEST_SOURCE) != 0;}

private:
  enum Flags
  {
    DO_FAST_LOAD      = 0x0001,
    SORT_FROM_TOP     = 0x0002,
    INGEST_MASTER     = 0x0004,
    INGEST_SOURCE     = 0x0008
  };

  ItemExpr * eodExpr_;
  ItemExpr * partnNumExpr_;

  ULng32 flags_;

  // if the rows are packed rowwise
  Lng32 maxRowsetSize_;
  ItemExpr *rwrsInputSizeExpr_;
  ItemExpr *rwrsMaxInputRowlenExpr_;
  ItemExpr *rwrsBufferAddrExpr_;

  // value ids of values which are returned by this operator.
  ValueIdList rwrsInputParamVids_;

  // name of exception table where error rows are to be inserted.
  CorrName excpTabNam_;
  NAString excpTabInsertStmt_;
  Lng32 excpRowsPercentage_;
  Lng32 excpRowsNumber_;
  Int64 loadId_;

  TableDesc * excpTabDesc_;

  Lng32 ingestNumSourceProcesses_;
  NAString ingestTargetProcessIds_;
};

///////////////////////////////////////////////////////////////////////////
// this class implements sidetree insert.
// It converts a regular insert...select into a sidetree insert...select
// if certain conditions are met.
// See Insert::bindNode for the conditions.
///////////////////////////////////////////////////////////////////////////
class ExeUtilSidetreeInsert : public ExeUtilExpr
{
public:
  ExeUtilSidetreeInsert(const CorrName &name,
			RelExpr * child,
			char * stmtText,
			CharInfo::CharSet stmtTextCharSet,
			CollHeap *oHeap = CmpCommon::statementHeap());

  virtual RelExpr * copyTopNode(RelExpr *derivedNode = NULL,
				CollHeap* outHeap = 0);

  virtual RelExpr * bindNode(BindWA *bindWAPtr);

  virtual Int32 getArity() const { return 1; };

  virtual RelExpr * preCodeGen(Generator * generator,
			       const ValueIdSet & externalInputs,
			       ValueIdSet &pulledNewInputs);

  // method to do code generation
  virtual short codeGen(Generator*);

private:
};

///////////////////////////////////////////////////////////////////////////
// This class handles a WITH NO ROLLBACK insert so that it can be 
// AQR'd.
///////////////////////////////////////////////////////////////////////////
class ExeUtilWnrInsert :  public ExeUtilExpr
{
public: 
  ExeUtilWnrInsert(const CorrName &name,
                   RelExpr * child,
                   CollHeap *oHeap = CmpCommon::statementHeap());

  virtual RelExpr * copyTopNode(RelExpr *derivedNode = NULL,
                                CollHeap* outHeap = 0);

  virtual RelExpr * bindNode(BindWA *bindWAPtr);

  virtual Int32 getArity() const { return 1; };

  virtual RelExpr * preCodeGen(Generator * generator,
                               const ValueIdSet & externalInputs,
                               ValueIdSet &pulledNewInputs);

  // method to do code generation
  virtual short codeGen(Generator*);

  virtual ExplainTuple *addSpecificExplainInfo( 
                                  ExplainTupleMaster *explainTuple, 
                                  ComTdb *tdb, Generator *generator);

  virtual NABoolean aqrSupported() { return TRUE; }

private:
};

class ExeUtilLoadVolatileTable : public ExeUtilExpr
{
public:
  ExeUtilLoadVolatileTable(const CorrName &name,
			   ExprNode * exprNode,
			   CollHeap *oHeap = CmpCommon::statementHeap())
       : ExeUtilExpr ( LOAD_VOLATILE_, name, exprNode, NULL
                     , NULL                           // in - char * stmt
                     , CharInfo::UnknownCharSet       // in - CharInfo::CharSet stmtCharSet
                     , oHeap                          // in - CollHeap * heap
                     ),
	 insertQuery_(NULL), updStatsQuery_(NULL)
  {
  };

  virtual RelExpr * copyTopNode(RelExpr *derivedNode = NULL,
				CollHeap* outHeap = 0);

  virtual RelExpr* bindNode(BindWA* bindWA);

  // method to do code generation
  virtual short codeGen(Generator*);

private:
  NAString * insertQuery_;
  NAString * updStatsQuery_;
};

class ExeUtilProcessVolatileTable : public DDLExpr
{
public:
  ExeUtilProcessVolatileTable(ExprNode * exprNode,
			      char * stmtText,
			      CharInfo::CharSet stmtTextCharSet,
			      CollHeap *oHeap = CmpCommon::statementHeap())
       : DDLExpr(exprNode, stmtText, stmtTextCharSet, FALSE, FALSE, FALSE, NULL, 0, oHeap),
	 isCreate_(FALSE),
	 isTable_(FALSE),
	 isIndex_(FALSE),
	 isSchema_(FALSE)
  {
  };

  virtual RelExpr * copyTopNode(RelExpr *derivedNode = NULL,
				CollHeap* outHeap = 0);

  virtual const NAString getText() const;

  virtual void unparse(NAString &result,
		       PhaseEnum /* phase */,
		       UnparseFormatEnum /* form */,
		       TableDesc * tabId = NULL) const;

  virtual RelExpr* bindNode(BindWA* bindWA);

  // method to do code generation
  virtual short codeGen(Generator*);

private:
  QualifiedName volTabName_;
  NABoolean isCreate_;
  NABoolean isTable_;
  NABoolean isIndex_;
  NABoolean isSchema_;
};

// this class is used to create the text for a user load exception table
// and set that text in the DDLExpr node.
// After bind stage, this class is handled as DDLExpr.
class ExeUtilProcessExceptionTable : public DDLExpr
{
public:
  ExeUtilProcessExceptionTable(ExprNode * exprNode,
			       char * stmtText,
			       CharInfo::CharSet stmtTextCharSet,
			       CollHeap *oHeap = CmpCommon::statementHeap())
       : DDLExpr(exprNode, stmtText, stmtTextCharSet, FALSE, FALSE, FALSE, NULL, 0, oHeap)
  {
  };

  virtual RelExpr * copyTopNode(RelExpr *derivedNode = NULL,
				CollHeap* outHeap = 0);

  virtual const NAString getText() const;

  virtual RelExpr* bindNode(BindWA* bindWA);

private:
};

class ExeUtilCleanupVolatileTables : public ExeUtilExpr
{
public:
  enum CleanupType
  {
    OBSOLETE_TABLES_IN_DEFAULT_CAT,
    OBSOLETE_TABLES_IN_ALL_CATS,
    OBSOLETE_TABLES_IN_SPECIFIED_CAT,
    ALL_TABLES_IN_ALL_CATS
  };

  ExeUtilCleanupVolatileTables(CleanupType type = OBSOLETE_TABLES_IN_DEFAULT_CAT,
			       const NAString &catName = "",
			       CollHeap *oHeap = CmpCommon::statementHeap())
       : ExeUtilExpr ( CLEANUP_VOLATILE_TABLES_
                     , CorrName("dummyName"), NULL, NULL
                     , NULL                             // in - char * stmt
                     , CharInfo::UnknownCharSet         // in - CharInfo::CharSet stmtCharSet
                     , oHeap                            // in - CollHeap * heap
                     ),
	 type_(type), catName_(catName, oHeap)
  {
  };

  virtual RelExpr * copyTopNode(RelExpr *derivedNode = NULL,
				CollHeap* outHeap = 0);

  virtual RelExpr* bindNode(BindWA* bindWA);

  // method to do code generation
  virtual short codeGen(Generator*);

  virtual NABoolean isExeUtilQueryType() { return TRUE; }

private:
  CleanupType type_;
  NAString catName_;
};

class ExeUtilGetErrorInfo : public ExeUtilExpr
{
public:
  ExeUtilGetErrorInfo(Lng32 errNum,
		      CollHeap *oHeap = CmpCommon::statementHeap())
    : ExeUtilExpr ( GET_ERROR_INFO_
		    , CorrName("dummyName"), NULL, NULL
		    , NULL                             // in - char * stmt
		    , CharInfo::UnknownCharSet         // in - CharInfo::CharSet stmtCharSet
		    , oHeap                            // in - CollHeap * heap
		    ),
    errNum_(errNum)
  {
  };

  virtual RelExpr * copyTopNode(RelExpr *derivedNode = NULL,
				CollHeap* outHeap = 0);

  virtual NABoolean producesOutput() { return TRUE; }

  // method to do code generation
  virtual short codeGen(Generator*);

private:
  Lng32 errNum_;
};

class ExeUtilGetVolatileInfo : public ExeUtilExpr
{
public:
  enum InfoType
  {
    ALL_SCHEMAS,
    ALL_TABLES,
    ALL_TABLES_IN_A_SESSION
  };

  ExeUtilGetVolatileInfo(InfoType type,
			 const NAString &sessionId = "",
			 CollHeap *oHeap = CmpCommon::statementHeap())
       : ExeUtilExpr ( GET_VOLATILE_INFO_
                     , CorrName("dummyName"), NULL, NULL
                     , NULL                             // in - char * stmt
                     , CharInfo::UnknownCharSet         // in - CharInfo::CharSet stmtCharSet
                     , oHeap                            // in - CollHeap * heap
                     ),
	 type_(type), sessionId_(sessionId, oHeap)
  {
  };

  virtual RelExpr * copyTopNode(RelExpr *derivedNode = NULL,
				CollHeap* outHeap = 0);

  virtual NABoolean producesOutput() { return TRUE; }

  // method to do code generation
  virtual short codeGen(Generator*);

private:
  InfoType type_;
  NAString sessionId_;
};

class ExeUtilCreateTableAs : public ExeUtilExpr
{
friend class ExeUtilDisplayExplainComplex;
public:
  ExeUtilCreateTableAs(const CorrName &name,
		       ExprNode * exprNode,
		       char * stmtText,
		       CharInfo::CharSet stmtTextCharSet,
		       CollHeap *oHeap = CmpCommon::statementHeap())
       : ExeUtilExpr(CREATE_TABLE_AS_, name, exprNode, NULL, stmtText, stmtTextCharSet, oHeap),
	 ctQuery_(oHeap), siQuery_(oHeap), viQuery_(oHeap), usQuery_(oHeap),
  loadIfExists_(FALSE), noLoad_(FALSE), deleteData_(FALSE), isVolatile_(FALSE)
  {
  };

  virtual RelExpr * copyTopNode(RelExpr *derivedNode = NULL,
				CollHeap* outHeap = 0);

  virtual RelExpr* bindNode(BindWA* bindWA);

  // method to do code generation
  virtual short codeGen(Generator*);

  virtual NABoolean explainSupported() { return TRUE; }

  virtual NABoolean producesOutput() { 
    if (CmpCommon::getDefault(REDRIVE_CTAS) == DF_OFF)
      return FALSE; 
    else
      return TRUE;
  }

private:
  NAString ctQuery_; // create table
  NAString siQuery_; // sidetree insert
  NAString viQuery_; // vsbb insert
  NAString usQuery_; // update stats

  // if the table already exists, do no return an error. 
  // Only do the insert...select part.
  NABoolean loadIfExists_;

  // do not do the insert...select part, only create the table.
  NABoolean noLoad_;

  // "create volatile table as" stmt
  NABoolean isVolatile_;

  // delete data first before loading the table.
  // Used if loadIfExists_ has been specified.
  NABoolean deleteData_;
};

class ExeUtilFastDelete : public ExeUtilExpr
{
public:
  ExeUtilFastDelete(const CorrName &name,
		    ExprNode * exprNode,
		    char * stmtText,
		    CharInfo::CharSet stmtTextCharSet,
		    NABoolean doPurgedataCat = FALSE,
		    NABoolean noLog = FALSE,
		    NABoolean ignoreTrigger = FALSE,
		    NABoolean isPurgedata = FALSE,
		    CollHeap *oHeap = CmpCommon::statementHeap(),
		    NABoolean isHiveTable = FALSE,
		    NAString * hiveTableLocation = NULL,
                    NAString * hiveHostName = NULL,
                    Int32 hiveHdfsPort = 0)
       : ExeUtilExpr(FAST_DELETE_, name, exprNode, NULL, stmtText, stmtTextCharSet, oHeap),
         doPurgedataCat_(doPurgedataCat),
         noLog_(noLog), ignoreTrigger_(ignoreTrigger),
         isPurgedata_(isPurgedata),
         doParallelDelete_(FALSE),
         doParallelDeleteIfXn_(FALSE),
         offlineTable_(FALSE),
         doLabelPurgedata_(FALSE),
         numLOBs_(0),
         isHiveTable_(isHiveTable)
  {
    if (isHiveTable )
      {
        CMPASSERT(hiveTableLocation != NULL);
        hiveTableLocation_ = *hiveTableLocation;
        if (hiveHostName)
          hiveHostName_ = *hiveHostName;
        hiveHdfsPort_ = hiveHdfsPort;
      }
  };

  virtual NABoolean isExeUtilQueryType() { return TRUE; }

  virtual RelExpr * copyTopNode(RelExpr *derivedNode = NULL,
				CollHeap* outHeap = 0);

  virtual RelExpr * bindNode(BindWA *bindWAPtr);

  virtual RelExpr * preCodeGen(Generator * generator,
			       const ValueIdSet & externalInputs,
			       ValueIdSet &pulledNewInputs);

  // method to do code generation
  virtual short codeGen(Generator*);
  
  virtual NABoolean aqrSupported() { return TRUE; }


  NABoolean isHiveTable()
  {
    return isHiveTable_;
  }

  const NAString &getHiveTableLocation() const
  {
    return hiveTableLocation_;
  }

  const NAString &getHiveHostName() const
  {
    return hiveHostName_;
  }

  const Int32 getHiveHdfsPort() const
  {
    return hiveHdfsPort_;
  }

private:
  NABoolean doPurgedataCat_;

  NABoolean noLog_;
  NABoolean ignoreTrigger_;

  NABoolean isPurgedata_;

  // do regular parallel delete at runtime. Start a Xn, if oen doesn't
  // exist.
  NABoolean doParallelDelete_;

  // do regular parallel delete if doParallelDelete is not chosen and
  // there is a transaction running at runtime.
  // If this is FALSE, then regular purgedata is invoked. 
  NABoolean doParallelDeleteIfXn_;

  NABoolean offlineTable_;

  // use the new parallel label purgedata operation.
  NABoolean doLabelPurgedata_;
  
  // if there are LOB columns.
  Lng32 numLOBs_; // number of LOB columns
  NAList<short> lobNumArray_; // array of shorts. Each short is the lob num

  NABoolean isHiveTable_;
  NAString  hiveTableLocation_;
  NAString hiveHostName_;
  Int32 hiveHdfsPort_;
};

class ExeUtilReorg : public ExeUtilExpr
{
public:
  enum ReorgOptionType
  {
    PARTITION_,
    CONCURRENCY_,
    RATE_, DSLACK_, ISLACK_, SLACK_, DELAY_, NO_DEALLOC_,
    PRIORITY_, PRIORITY_DELTA_,
    DISPLAY_, NO_OUTPUT_,
    RETURN_SUMMARY_, RETURN_DETAIL_OUTPUT_,
    GET_STATUS_,
    SUSPEND_, NEW_REORG_,
    CHECK_, REORG_IF_NEEDED_, COMPACT_, OVERRIDE_, STOP_, COMPRESS_,
    WHERE_,
    GET_TABLES_,
    UPDATE_DB_, USE_DB_,
    RETURN_FAST_, MAX_TABLES_,
    GENERATE_MAINTAIN_COMMANDS_, MAX_MAINTAIN_TABLES_, 
    SHOW_MAINTAIN_COMMANDS_, RUN_MAINTAIN_COMMANDS_,
    GENERATE_CHECK_COMMANDS_, CONCURRENT_CHECK_SESSIONS_,
    CONTINUE_ON_ERROR_, STOP_ON_ERROR,
    SYSTEM_OBJECTS_ONLY_, DEBUG_OUTPUT_,
    INITIALIZE_DB_, REINITIALIZE_DB_, 
    DROP_DB_, CLEANUP_REORG_DB_,
    CREATE_DB_VIEW_, DROP_DB_VIEW_,
    VERIFY_
  };

  enum ReorgObjectType
  {
    TABLE_, INDEX_, MV_, SCHEMA_, CATALOG_, NOOP_, INVALID_
  };

  class ReorgOption
  {
    friend class ExeUtilReorg;
  public:
    ReorgOption(ReorgOptionType option, 
      Long numericVal,
      char * stringVal,
      Lng32 numericVal2 = 0,
      char * stringVal2 = NULL)
      : option_(option), 
      numericVal_(numericVal), stringVal_(stringVal),
      numericVal2_(numericVal2), stringVal2_(stringVal2)

    {}

  private:
    ReorgOptionType option_;
    Long   numericVal_;
    char * stringVal_;
    Lng32   numericVal2_;
    char * stringVal2_;
  };

  ExeUtilReorg(enum ReorgObjectType type,
	       const CorrName &name,
	       NAList<ReorgOption*> * reorgOptionsList,
	       QualNamePtrList * additionalTables,
	       CollHeap *oHeap = CmpCommon::statementHeap());

  virtual RelExpr * copyTopNode(RelExpr *derivedNode = NULL,
				CollHeap* outHeap = 0);

  virtual NABoolean producesOutput() { return TRUE; }
  virtual NABoolean isExeUtilQueryType() { return TRUE; }

  virtual RelExpr * bindNode(BindWA *bindWAPtr);

  // method to do code generation
  virtual short codeGen(Generator*);

  void setParams(Lng32 concurrency,
		 Lng32 firstPartn, Lng32 lastPartn,
		 Lng32 rate, Lng32 dslack, Lng32 islack, Lng32 delay,
		 NABoolean noDealloc,
		 Lng32 priority,
		 NAString &partnName,
		 NABoolean displayOnly, NABoolean noOutputMsg,
		 NABoolean returnSummary, NABoolean returnDetailOutput,
		 NABoolean doCheck,
		 NABoolean doReorg,
		 NABoolean reorgIfNeeded,
		 NABoolean getStatus,
		 NABoolean suspend,
                 NABoolean stop,
		 NABoolean newReorg,
		 NABoolean doCompact,
		 NABoolean doOverride,
                 Lng32 compressionType,
                 NABoolean updateReorgDB,
                 NABoolean useReorgDB,
                 NABoolean returnFast,
                 Lng32 rowsetSize,
                 Lng32 firstTable,
                 Lng32 lastTable,
                 NAString wherePredStr,
                 NABoolean generateMaintainCommands,
		 NABoolean showMaintainCommands,
		 NABoolean runMaintainCommands,
                 Lng32 maxMaintainTables,
                 NABoolean reorgCheckAll,
                 NABoolean generateCheckCommands,
                 Lng32 concurrentCheckSessions,
                 NABoolean continueOnError,
                 NABoolean systemObjectsOnly,
		 NABoolean debugOutput,
                 NABoolean verify);

   LIST(OptSqlTableOpenInfo *) &getStoiList()  { return stoiList_; }

private:
  ReorgObjectType type_;
  Lng32 concurrency_;
  Lng32 firstPartn_;
  Lng32 lastPartn_;
  Lng32 rate_;
  Lng32 dslack_;
  Lng32 islack_;
  Lng32 delay_;
  Lng32 priority_;
  NABoolean noDealloc_;
  NAString partnName_;
  NABoolean displayOnly_;
  NABoolean noOutputMsg_;

  NABoolean returnSummary_;
  NABoolean returnDetailOutput_;

  NABoolean doCheck_;
  NABoolean doReorg_;
  NABoolean reorgIfNeeded_;

  NABoolean getStatus_;
  NABoolean suspend_;
  NABoolean stop_;
  NABoolean newReorg_;

  NABoolean doCompact_;
  NABoolean doOverride_;

  NABoolean updateDBspecified_;
  NABoolean updateReorgDB_;
  NABoolean useReorgDB_;

  NABoolean returnFast_;
  
  Lng32 rowsetSize_;
  NABoolean generateMaintainCommands_;
  Lng32 maxMaintainTables_;
  NABoolean generateCheckCommands_;
  Lng32 concurrentCheckSessions_;
  NABoolean showMaintainCommands_;
  NABoolean runMaintainCommands_;

  NABoolean reorgCheckAll_;
  
  void * wherePred_;
  NAString wherePredStr_;
  
  Lng32 firstTable_;
  Lng32 lastTable_;
  
  Lng32 compressionType_;

  Int64 statusAfterTS_;

  NABoolean continueOnError_;
  
  NAString statusSummaryOptionsStr_;
  Lng32 statusSummaryOptions_;

  NABoolean systemObjectsOnly_; // only reorg check system objects(schemas, table)

  NABoolean debugOutput_;

  NABoolean initialize_;
  NABoolean reinitialize_;
  NABoolean drop_;
  NABoolean createView_;
  NABoolean dropView_;

  NABoolean cleanupReorgDB_;
  Int64 cleanupToTS_;

  UInt32     maxTables_;
  
  // additional tables that are to be reorged alongwith the primary table.
  QualNamePtrList additionalTables_;

  TableDescList * additionalTablesDescs_;

  LIST(OptSqlTableOpenInfo *) stoiList_;  // open infos
  NABoolean verify_;
};

class ExeUtilReplicate : public ExeUtilExpr
{
public:
  enum ReplicateOptionType
  {
    SOURCE_OBJECT_, SOURCE_SCHEMA_,
    SOURCE_OBJ_IN_LIST_,
    LIKE_PATTERN_, NOT_LIKE_PATTERN_,
    IN_LIST_, NOT_IN_LIST_,
    TARGET_OBJECT_, PURGEDATA_TARGET_,
    TARGET_SYSTEM_, 
    SOURCE_IMPLICIT_UNIQUE_INDEX_POSITION_,
    VALIDATE_TGT_DDL_, NO_VALIDATE_TGT_DDL_,
    COPY_DDL_, COPY_DATA_, COPY_STATISTICS_,
    STATISTICS_, TGT_STATISTICS_, TGT_DDL_,
    VALIDATE_DATA_,
    PRIORITY_, PRIORITY_DELTA_,
    CONCURRENCY_, RATE_, DELAY_,
    GET_STATUS_,
    SUSPEND_, RESUME_, ABORT_,
    COMPRESS_,
    TGT_OBJS_ONLINE_, TGT_OBJS_OFFLINE_,
    TGT_OBJS_UNAUDITED_, TGT_OBJS_AUDITED_,
    TGT_CAT_NAME_,
    PARENT_QID_,
    TGT_RETURN_PARTITION_DETAILS_,
    TRANSFORM_, NO_TRANSFORM_, TGT_ACTIONS_,
    GET_DETAILS_,
    INITIALIZE_, REINITIALIZE_, DROP_,
    ADD_CONFIG_, REMOVE_CONFIG_,
    DEBUG_TARGET_, FORCE_ERROR_,
    DISABLE_SCHEMA_CREATE_,
    NUM_SRC_PARTNS_,
    RECOVER_, ROLE_NAME_,
    NO_VALIDATE_ROLE_,
    VALIDATE_PRIVILEGES_,
    CLEANUP_,
    VERSION_,
    INCREMENTAL_,
    AUTHORIZATION_,
    COPY_BOTH_DATA_AND_STATISTICS_,
    COMPRESSION_TYPE_,
    DISK_POOL_,
    TARGET_SCHEMA_,
    VALIDATE_DDL_,
    VALIDATE_AND_PURGEDATA_,
    WAITED_STARTUP_
  };

  enum ReplicateObjectType
  {
    TABLE_, INDEX_, MV_
  };

  class ReplicateOption
  {
    friend class ExeUtilReplicate;
  public:
    ReplicateOption(ReplicateOptionType option, 
		    Lng32 numericVal,
		    void * stringVal,
		    Lng32 numericVal2 = 0,
		    char * stringVal2 = NULL)
	 : option_(option), 
	   numericVal_(numericVal), stringVal_(stringVal),
	   numericVal2_(numericVal2), stringVal2_(stringVal2)
    {}

  private:
    ReplicateOptionType option_;
    Lng32   numericVal_;
    void * stringVal_;
    Lng32   numericVal2_;
    void * stringVal2_;
  };

  ExeUtilReplicate(NAList<ReplicateOption*> * replicateOptionsList,
		   CollHeap *oHeap = CmpCommon::statementHeap());

  virtual RelExpr * copyTopNode(RelExpr *derivedNode = NULL,
				CollHeap* outHeap = 0);

  virtual NABoolean producesOutput() { return TRUE; }
  virtual NABoolean isExeUtilQueryType() { return TRUE; }

  virtual RelExpr * bindNode(BindWA *bindWAPtr);

  // method to do code generation
  virtual short codeGen(Generator*);

private:
  void setErrorInParams(NABoolean v, const char * reason);

  NAString reason_;

  // source object name
  CorrName sourceName_;
  ReplicateObjectType sourceType_;

  // target object name
  CorrName targetName_;
  ReplicateObjectType targetType_;
  
  // source schema name
  SchemaName sourceSchema_;
  
  // target schema name
  SchemaName targetSchema_;

  NAString srcCatName_;
  NAString tgtCatName_;

  Lng32 srcImpUnqIxPos_;

  NAString likePattern_;
  NABoolean notLikePattern_;
  NAString escChar_;
  NAString inList_;
  NABoolean notInList_;

  // target system
  NAString targetSystem_;
  //  NABoolean tgtIpAddr_;

  NABoolean purgedataTgt_;
  NABoolean incremental_;

  NAString ddlInputStr_;

  NABoolean srcObjsInList_;
  //  ConstStringList srcInList_;
  QualNamePtrList srcInList_;

  Lng32 concurrency_;
  Lng32 rate_;
  Lng32 priority_;
  Lng32 priorityDelta_;
  Lng32 delay_;

  Int64 srcObjectUid_;

  NABoolean compress_;
  NABoolean compressSpecified_;

  NABoolean getStatus_;
  NABoolean suspend_;
  NABoolean resume_;
  NABoolean abort_;
  NAString  controlQueryId_;

  // validate that source and target DDL is the same
  NABoolean validateTgtDDL_;

  NABoolean validateDDL_;
  NABoolean validateAndPurgedata_;
  NABoolean validateDDLSpecified_; 

  NABoolean copyData_;
  NABoolean validateData_;
  NABoolean copyBothDataAndStats_;

  NABoolean onlySpecified_;

  NABoolean tgtObjsOnline_;
  NABoolean tgtObjsOffline_;

  NABoolean tgtObjsAudited_;
  NABoolean tgtObjsUnaudited_;

  NABoolean returnPartnDetails_;

  NABoolean transform_;
  NABoolean noTransform_;
  
  NABoolean tgtActions_;

  NABoolean initialize_;
  NABoolean reinitialize_;
  NABoolean drop_;
  
  NABoolean addConfig_;
  NABoolean removeConfig_;
  NAString ipAddr_;
  Int32    portNum_;

  NABoolean getDetails_;
  NAString formatOptions_;

  NABoolean debugTarget_;

  NAString forceError_;

  NABoolean disableSchemaCreate_;

  Int32 numSrcPartns_;

  Lng32 recoverType_;

  NAString roleName_;
  NABoolean noValidateRole_;

  NABoolean statistics_;
  Lng32 tgtStatsType_;

  NABoolean copyDDL_;
  Lng32 tgtDDLType_;

  NABoolean validatePrivs_;

  // set if CLEANUP option is specified.
  // From/To are the start times of the replicate queries
  // that need to be removed from replicate status tables.
  NABoolean cleanup_;
  Int64 cleanupFrom_;
  Int64 cleanupTo_;

  NABoolean errorInParams_;
  NABoolean SQTargetType_;
  NABoolean authids_;
  NABoolean compressionTypeSpecified_;
  NABoolean diskPoolSpecified_;
  Lng32 version_;
  Lng32 compressionType_;
  Lng32 diskPool_;
  NABoolean waitedStartup_;
};

class ExeUtilMaintainObject : public ExeUtilExpr
{
public:
  enum MaintainObjectOptionType
  {
    INITIALIZE_, REINITIALIZE_, DROP_, 
    CREATE_VIEW_, DROP_VIEW_,
    ALL_,
    REORG_TABLE_, REORG_INDEX_,
    UPD_STATS_TABLE_, UPD_STATS_MVLOG_,
    UPD_STATS_MVS_, UPD_STATS_MVGROUP_,
    UPD_STATS_ALL_MVS_,
    REFRESH_ALL_MVGROUP_, REFRESH_MVGROUP_, 
    REFRESH_ALL_MVS_, REFRESH_MVS_,
    REORG_MVGROUP_,
    REORG_MVS_, REORG_MVS_INDEX_,
    REORG_, REFRESH_,
    ENABLE_, DISABLE_, RESET_,
    CONTINUE_ON_ERROR_,
    GET_STATUS_, GET_DETAILS_,
    RETURN_SUMMARY_, RETURN_DETAIL_OUTPUT_,
    NO_OUTPUT_, MAX_TABLES_,
    DISPLAY_, DISPLAY_DETAIL_,
    CLEAN_MAINTAIN_CIT_,
    RUN_, IF_NEEDED_,GET_SCHEMA_LABEL_STATS_,GET_LABEL_STATS_, GET_TABLE_LABEL_STATS_, GET_INDEX_LABEL_STATS_, GET_LABELSTATS_INC_INDEXES_, GET_LABELSTATS_INC_INTERNAL_, GET_LABELSTATS_INC_RELATED_
  };

  class MaintainObjectOption
  {
    friend class ExeUtilMaintainObject;
  public:
    MaintainObjectOption(MaintainObjectOptionType option,
			 Lng32 numericVal1,
			 const char * stringVal1,
			 Lng32 numericVal2 = 0,
			 const char * stringVal2 = NULL)
	 : option_(option), 
	   numericVal1_(numericVal1), stringVal1_(stringVal1),
	   numericVal2_(numericVal2), stringVal2_(stringVal2)
    {}

  private:
    MaintainObjectOptionType option_;
    Lng32   numericVal1_;
    const char * stringVal1_;
    Lng32   numericVal2_;
    const char * stringVal2_;
  };

  enum MaintainObjectType
  {
    TABLE_, INDEX_, MV_, MVGROUP_, MV_INDEX_, MV_LOG_, 
    TABLES_, CATALOG_, SCHEMA_, DATABASE_, CLEAN_MAINTAIN_, NOOP_
  };

  ExeUtilMaintainObject(enum MaintainObjectType type,
			const CorrName &name,
			QualNamePtrList * multiTablesNames,
			NAList<MaintainObjectOption*> *MaintainObjectOptionsList,
                        CollHeap *oHeap = CmpCommon::statementHeap());  

  virtual RelExpr * copyTopNode(RelExpr *derivedNode = NULL,
				CollHeap* outHeap = 0);

  virtual NABoolean producesOutput() { return (noOutput_ ? FALSE : TRUE); }
  virtual NABoolean isExeUtilQueryType() { return TRUE; }

  virtual RelExpr * bindNode(BindWA *bindWAPtr);

  // method to do code generation
  virtual short codeGen(Generator*);

  void setParams(NABoolean all,
		 NABoolean reorgTable,
		 NABoolean reorgIndex,
		 NABoolean updStatsTable,
		 NABoolean updStatsMvlog,
		 NABoolean updStatsMvs,
		 NABoolean updStatsMvgroup,
                 NABoolean updStatsAllMvs,
		 NABoolean refreshAllMvgroup,
		 NABoolean refreshMvgroup,
		 NABoolean refreshAllMvs,
		 NABoolean refreshMvs,
		 NABoolean reorgMvgroup,
		 NABoolean reorgMvs,
		 NABoolean reorgMvsIndex,
		 NABoolean cleanMaintainCIT,
		 NABoolean continueOnError,
		 NABoolean returnSummary,
		 NABoolean returnDetailOutput,
		 NABoolean noOutput,
		 NABoolean display,
		 NABoolean displayDetail,
		 NABoolean getLabelStats,
		 NABoolean getTableLabelStats,
		 NABoolean getIndexLabelStats,
		 NABoolean getLabelStatsIncIndexes,
		 NABoolean getLabelStatsIncInternal,
		 NABoolean getLabelStatsIncRelated,
		 NABoolean getSchemaLabelStats);

  void setOptionsParams(const NAString &reorgTableOptions,
			const NAString &reorgIndexOptions,
			const NAString &updStatsTableOptions,
			const NAString &updStatsMvlogOptions,
			const NAString &updStatsMvsOptions,
			const NAString &updStatsMvgroupOptions,
			const NAString &refreshMvgroupOptions,
			const NAString &refreshMvsOptions,
			const NAString &reorgMvgroupOptions,
			const NAString &reorgMvsOptions,
			const NAString &reorgMvsIndexOptions,
			const NAString &cleanMaintainCITOptions);

  void setControlParams(NABoolean disableReorgTable,
			NABoolean enableReorgTable,
			NABoolean disableReorgIndex,
			NABoolean enableReorgIndex,
			NABoolean disableUpdStatsTable,
			NABoolean enableUpdStatsTable,
			NABoolean disableUpdStatsMvs,
			NABoolean enableUpdStatsMvs,
 			NABoolean disableRefreshMvs,
			NABoolean enableRefreshMvs,
			NABoolean disableReorgMvs,
			NABoolean enableReorgMvs,
			NABoolean resetReorgTable,
			NABoolean resetUpdStatsTable,
			NABoolean resetUpdStatsMvs,
			NABoolean resetRefreshMvs,
			NABoolean resetReorgMvs,
 			NABoolean resetReorgIndex,
			NABoolean enableUpdStatsMvslog,
 			NABoolean disableUpdStatsMvslog,
 			NABoolean resetUpdStatsMvslog,
			NABoolean enableReorgMvsIndex,
			NABoolean disableReorgMvsIndex,
			NABoolean resetReorgMvsIndex,
			NABoolean enableRefreshMvgroup,
			NABoolean disableRefreshMvgroup,
			NABoolean resetRefreshMvgroup,
 			NABoolean enableReorgMvgroup,
			NABoolean disableReorgMvgroup,
			NABoolean resetReorgMvgroup,
			NABoolean enableUpdStatsMvgroup,
			NABoolean disableUpdStatsMvgroup,
			NABoolean resetUpdStatsMvgroup,
			NABoolean enableTableLabelStats,
			NABoolean disableTableLabelStats,
			NABoolean resetTableLabelStats,
			NABoolean enableIndexLabelStats,
			NABoolean disableIndexLabelStats,
			NABoolean resetIndexLabelStats
);

  void          setMaintainedTableCreateTime(Int64 createTime)
  { maintainedTableCreateTime_ = createTime; }
  Int64 getMaintainedTableCreateTime() { return maintainedTableCreateTime_; }

  void          setParentTableObjectUID(Int64 objectUID)  
  { parentTableObjectUID_ = objectUID; }
  Int64 getParentTableObjectUID() { return parentTableObjectUID_; }

  const char * getParentTableName()  { return parentTableName_.data(); }
  void setParentTableName(char * parent) { parentTableName_ = parent; } 

  UInt32 getParentTableNameLen() { return parentTableNameLen_; }
  void setParentTableNameLen(UInt32 length) { parentTableNameLen_ = length; }

  NATable     *getMvLogTable() const { return mvLogTable_; }
  void          setMvLogTable(NATable *newTable)
  { mvLogTable_ = newTable; }  

  private:
  MaintainObjectType type_;
  Int64 maintainedTableCreateTime_;
  Int64 parentTableObjectUID_;

  NAString parentTableName_;                            
  UInt32 parentTableNameLen_; 

  NABoolean all_;
  NABoolean reorgTable_;
  NABoolean reorgIndex_;
  NABoolean updStatsTable_;
  NABoolean updStatsMvlog_;
  NABoolean updStatsMvs_;
  NABoolean updStatsMvgroup_;
  NABoolean updStatsAllMvs_;
  NABoolean refreshAllMvgroup_;
  NABoolean refreshMvgroup_;
  NABoolean refreshAllMvs_;
  NABoolean refreshMvs_;
  NABoolean reorgMvgroup_;
  NABoolean reorgMvs_;
  NABoolean reorgMvsIndex_;
  NABoolean reorg_;
  NABoolean refresh_;
  NABoolean cleanMaintainCIT_;
  NABoolean getLabelStats_;
  NABoolean getTableLabelStats_;
  NABoolean getIndexLabelStats_;
  NABoolean getLabelStatsIncIndexes_;
  NABoolean getLabelStatsIncInternal_;
  NABoolean getLabelStatsIncRelated_;
  NABoolean getSchemaLabelStats_;

  NAString reorgTableOptions_;
  NAString reorgIndexOptions_;
  NAString updStatsTableOptions_;
  NAString updStatsMvlogOptions_;
  NAString updStatsMvsOptions_;
  NAString updStatsMvgroupOptions_;
  NAString refreshMvgroupOptions_;
  NAString refreshMvsOptions_;
  NAString reorgMvgroupOptions_;
  NAString reorgMvsOptions_;
  NAString reorgMvsIndexOptions_;
  NAString cleanMaintainCITOptions_;
  NAString formatOptions_;

  // set if RUN option is specified.
  Int64 runFrom_;
  Int64 runTo_;

  NABoolean disable_;
  NABoolean disableReorgTable_;
  NABoolean disableReorgIndex_;
  NABoolean disableUpdStatsTable_;
  NABoolean disableUpdStatsMvs_;
  NABoolean disableUpdStatsMvlog_;
  NABoolean disableRefreshMvs_;
  NABoolean disableReorgMvs_;
  NABoolean disableReorgMvsIndex_;
  NABoolean disableRefreshMvgroup_;
  NABoolean disableReorgMvgroup_;
  NABoolean disableUpdStatsMvgroup_;
  NABoolean disableTableLabelStats_;
  NABoolean disableIndexLabelStats_;

  NABoolean enable_;
  NABoolean enableReorgTable_;
  NABoolean enableReorgIndex_;
  NABoolean enableUpdStatsTable_;
  NABoolean enableUpdStatsMvs_;
  NABoolean enableUpdStatsMvlog_;
  NABoolean enableRefreshMvs_;
  NABoolean enableReorgMvs_;
  NABoolean enableReorgMvsIndex_;
  NABoolean enableRefreshMvgroup_;
  NABoolean enableReorgMvgroup_;
  NABoolean enableUpdStatsMvgroup_;
  NABoolean enableTableLabelStats_;
  NABoolean enableIndexLabelStats_;

  NABoolean reset_;
  NABoolean resetReorgTable_;
  NABoolean resetUpdStatsTable_;
  NABoolean resetUpdStatsMvs_;
  NABoolean resetUpdStatsMvlog_;
  NABoolean resetRefreshMvs_;
  NABoolean resetReorgMvs_;
  NABoolean resetReorgIndex_;
  NABoolean resetReorgMvsIndex_;
  NABoolean resetRefreshMvgroup_;
  NABoolean resetReorgMvgroup_;
  NABoolean resetUpdStatsMvgroup_;
  NABoolean resetTableLabelStats_;
  NABoolean resetIndexLabelStats_;

  NABoolean continueOnError_;

  NABoolean returnSummary_;
  NABoolean returnDetailOutput_;
  NABoolean display_;
  NABoolean displayDetail_;
  NABoolean noOutput_;

  NABoolean doTheSpecifiedTask_;

  NABoolean errorInParams_;

  NABoolean getStatus_;
  NABoolean getDetails_;
  NABoolean shortFormat_;
  NABoolean longFormat_;
  NABoolean detailFormat_;
  NABoolean tokenFormat_;
  NABoolean commandFormat_;

  NAString statusSummaryOptionsStr_;

  NABoolean run_;
  NABoolean ifNeeded_;

  UInt32     maxTables_;

  NABoolean initialize_;
  NABoolean reinitialize_;
  NABoolean drop_;
  NABoolean createView_;
  NABoolean dropView_;
  
  NATable *mvLogTable_;

  // multiple tables that are to be maintained together
  QualNamePtrList multiTablesNames_;

  // skipped table during multiple table processing
  QualNamePtrList skippedMultiTablesNames_;

  TableDescList * multiTablesDescs_;

};

class ExeUtilGetStatistics : public ExeUtilExpr
{
public:
  ExeUtilGetStatistics(NAString statementName = "",
		       char * optionsStr = NULL,
		       CollHeap *oHeap = CmpCommon::statementHeap(),
                       short statsReqType = SQLCLI_STATS_REQ_STMT,
                       short statsMergeType = SQLCLI_SAME_STATS,
                       short activeQueryNum = -1); //RtsQueryId::ANY_QUERY_
  
  virtual RelExpr * copyTopNode(RelExpr *derivedNode = NULL,
				CollHeap* outHeap = 0);

  virtual NABoolean producesOutput() { return TRUE; }

  virtual RelExpr * bindNode(BindWA *bindWAPtr);

  // method to do code generation
  RelExpr * preCodeGen(Generator * generator,
		       const ValueIdSet & externalInputs,
		       ValueIdSet &pulledNewInputs);
  virtual short codeGen(Generator*);

protected:
  NAString statementName_;

  // 'cs:es:os:ds:of:tf'
  // cs: Compiler Stats
  // es: Executor Stats
  // os: Other Stats
  // ds: Detailed Stats
  // of: old format (mxci display statistics output)
  // tf: tokenized format, each stats value preceded by a predefined token.
  NAString optionsStr_;

  NABoolean compilerStats_;
  NABoolean executorStats_;
  NABoolean otherStats_;
  NABoolean detailedStats_;

  NABoolean oldFormat_;
  NABoolean shortFormat_;

  NABoolean tokenizedFormat_;

  NABoolean errorInParams_;
  short statsReqType_;
  short statsMergeType_;
  short activeQueryNum_;
};

class ExeUtilGetReorgStatistics : public ExeUtilGetStatistics
{
public:
  ExeUtilGetReorgStatistics(NAString qid = "",
			    char * optionsStr = NULL,
			    CollHeap *oHeap = CmpCommon::statementHeap());
  
  virtual RelExpr * copyTopNode(RelExpr *derivedNode = NULL,
				CollHeap* outHeap = 0);

  virtual short codeGen(Generator*);

private:
};

class ExeUtilGetProcessStatistics : public ExeUtilGetStatistics
{
public:
  ExeUtilGetProcessStatistics(NAString pid = "",
			    char * optionsStr = NULL,
			    CollHeap *oHeap = CmpCommon::statementHeap());
  
  virtual RelExpr * copyTopNode(RelExpr *derivedNode = NULL,
				CollHeap* outHeap = 0);

  virtual short codeGen(Generator*);

private:
};

/////////////////////////////////////////////////////////////////////////////
//
// Syntax:
//  get [<versionOff>] [<allUserSystem>] <infoType> [inOnForClause <objectType> <objectName>] [matchPattern]
//  <versionOf>      version of
//  <allUserSystem>  all|user|system
//  infoType:        tables|indexes|views|mvs|synonyms|schemas 
//  inOnForClause:   in|on|for
//  objectType:      table|view|mv|index|synonym|schema|catalog 
//  objectName:      <cat>|<sch>|<object> 
//  matchPattern:    , match '<pattern>'
//
/////////////////////////////////////////////////////////////////////////////


class ExeUtilGetMetadataInfo : public ExeUtilExpr
{
public:

  enum InfoType
  {
    NO_INFO_TYPE_ = 0, 
    TABLES_       = 1, 
    INDEXES_      = 2, 
    VIEWS_        = 3, 
    MVS_          = 4, 
    MVGROUPS_     = 5, 
    PRIVILEGES_   = 6,
    SYNONYMS_     = 7, 
    SCHEMAS_      = 8,
    COMPONENTS_   = 9,               
    COMPONENT_PRIVILEGES_ = 10,
    INVALID_VIEWS_ = 11,
    COMPONENT_OPERATIONS = 12     
  };

  enum InfoFor
  {
    NO_INFO_FOR_ = 0,
    TABLE_       = 1, 
    INDEX_       = 2, 
    VIEW_        = 3, 
    MV_          = 4, 
    MVGROUP_     = 5, 
    SYNONYM_     = 6, 
    SCHEMA_      = 7, 
    CATALOG_     = 8,
    USER_        = 9,
    COMPONENT_   = 10  // COMPONENT TBD
  };

  ExeUtilGetMetadataInfo(NAString &ausStr,
			 NAString &infoType,
			 NAString &iofStr,
			 NAString &objectType,
			 CorrName &objectName,
			 NAString *pattern,
			 NABoolean returnFullyQualNames,
			 NABoolean getVersion,
			 NAString *param1,
			 CollHeap *oHeap = CmpCommon::statementHeap());
  
  virtual RelExpr * copyTopNode(RelExpr *derivedNode = NULL,
				CollHeap* outHeap = 0);

  virtual NABoolean producesOutput() { return TRUE; }

  virtual RelExpr * bindNode(BindWA *bindWAPtr);

  // method to do code generation
  virtual short codeGen(Generator*);

  NABoolean noHeader() { return noHeader_;}
  void setNoHeader(NABoolean v) { noHeader_ = v; }

  virtual NABoolean aqrSupported() { return TRUE; }

  NABoolean hiveObjects() { return hiveObjs_;}
  void setHiveObjects(NABoolean v) { hiveObjs_ = v; }

  NABoolean hbaseObjects() { return hbaseObjs_;}
  void setHbaseObjects(NABoolean v) { hbaseObjs_ = v; }
  
private:
  NAString ausStr_; // all/user/system objects
  NAString infoType_;
  NAString iofStr_; // in/on/for clause
  NAString objectType_;
  CorrName objectName_;
  NAString pattern_;

  NABoolean noHeader_;

  NABoolean returnFullyQualNames_;

  NABoolean getVersion_;

  NAString param1_;

  NABoolean errorInParams_;

  NABoolean hiveObjs_;
  NABoolean hbaseObjs_;
};


/////////////////////////////////////////////////////////////////////////////
// Suspend or activate a query.
//
// Note: on Seaquest Suspend and Activate are handled by the ControlRunningQuery
// RelExpr (see RelMisc.h) and in the executor, by the ComTdbCancel,
// ExCancelTdb and ExCancelTcb classes.
/////////////////////////////////////////////////////////////////////////////
// See ControlRunningQuery

class ExeUtilShowSet : public ExeUtilExpr
{
public:
  enum ShowSessionDefaultType
  {
    ALL_DEFAULTS_,
    EXTERNALIZED_DEFAULTS_,
    SINGLE_DEFAULT_
  };
    
  ExeUtilShowSet(ShowSessionDefaultType type,
		 const NAString &ssdName = "",
		 CollHeap *oHeap = CmpCommon::statementHeap())
       : ExeUtilExpr ( SHOWSET_DEFAULTS_
                     , CorrName("dummyName"), NULL, NULL
                     , (char *)NULL             // in - char * stmt
                     , CharInfo::UnknownCharSet // in - CharInfo::CharSet stmtCharSet
                     , (CollHeap *)oHeap        // in - CollHeap * heap
                     ),
	 type_(type), ssdName_(ssdName, oHeap)
  {
  };
  
  virtual RelExpr * copyTopNode(RelExpr *derivedNode = NULL,
				CollHeap* outHeap = 0);

  virtual short codeGen(Generator*);

  virtual NABoolean producesOutput() { return TRUE; }

private:
  ShowSessionDefaultType type_;

  NAString ssdName_;
};

// auto query retry
class ExeUtilAQR : public ExeUtilExpr
{
public:
  enum AQRTask
  {
    NONE_ = -1, 
    GET_ = 0,
    ADD_, DELETE_, UPDATE_,
    CLEAR_, RESET_
  };

  enum AQROptionType
  {
    SQLCODE_, NSKCODE_, RETRIES_, DELAY_, TYPE_
  };

  class AQROption
  {
    friend class ExeUtilAQR;
  public:
    AQROption(AQROptionType option, Lng32 numericVal,
	      char * stringVal)
	 : option_(option), numericVal_(numericVal), stringVal_(stringVal)
    {}
    
  private:
    AQROptionType option_;
    Lng32   numericVal_;
    char * stringVal_;
  };

  ExeUtilAQR(AQRTask task,
	     NAList<AQROption*> * aqrOptionsList,
	     CollHeap *oHeap = CmpCommon::statementHeap());

  virtual RelExpr * copyTopNode(RelExpr *derivedNode = NULL,
				CollHeap* outHeap = 0);

  virtual RelExpr * bindNode(BindWA *bindWAPtr);

  virtual NABoolean producesOutput() 
  { return (task_ == GET_ ? TRUE : FALSE); }

  // method to do code generation
  virtual short codeGen(Generator*);

private:
  Lng32 sqlcode_;
  Lng32 nskcode_;
  Lng32 retries_;
  Lng32 delay_;
  Lng32 type_;

  AQRTask task_;
};

class ExeUtilLongRunning : public ExeUtilExpr 
{
public:

  enum LongRunningType
    {
      LR_DELETE        = 1,
      LR_UPDATE        = 2,
      LR_INSERT_SELECT = 3,
      LR_END_OF_RANGE  = 99  // Ensure an end of range
    };

  
  ExeUtilLongRunning(const CorrName    &name,
                     const char *     predicate,
                     ULng32    predicateLen,
                     enum LongRunningType type,
		     ULng32    multiCommitSize,
                     CollHeap *oHeap = CmpCommon::statementHeap());

  virtual ~ExeUtilLongRunning();

  // cost functions
  virtual PhysicalProperty *synthPhysicalProperty(const Context *context,
                                                  const Lng32     planNumber);

  virtual RelExpr * bindNode(BindWA *bindWAPtr);

  virtual RelExpr * copyTopNode(RelExpr *derivedNode = NULL,
				CollHeap* outHeap = 0);

  // method to do code generation
  virtual short codeGen(Generator*);

  // get the degree of this node (it is a leaf node).
  virtual Int32 getArity() const { return 0; }

  // Predicate string
  const char * getPredicate() { return predicate_; };
  void setPredicate(char * predicate) { predicate_ = predicate;};

  Int64 getPredicateLen() { return predicateLen_; };

  // LongRunning operation type
  LongRunningType getLongRunningType() { return type_; };
  void setLongRunningType(LongRunningType type) {type_ = type;}; 

  void addPredicateTree(ItemExpr *predicate) { predicateExpr_ = predicate; };

  ULng32 getMultiCommitSize() {return multiCommitSize_;}

  // MV NOMVLOG option for LRU operations
  NABoolean isNoLogOperation() const { return isNoLogOperation_; }
  void setNoLogOperation(NABoolean isNoLoggingOperation = FALSE)
            { isNoLogOperation_ = isNoLoggingOperation; }

  ExplainTuple *addSpecificExplainInfo(ExplainTupleMaster *explainTuple,
					      ComTdb * tdb,
					      Generator *generator);
private:

  NAString constructLRUDeleteStatement(NABoolean withCK);

  NAString getCKColumnsAsSelectList();

  NAString constructKeyRangeComparePredicate();

  char * lruStmt_;
  Int64 lruStmtLen_;

  char * lruStmtWithCK_;
  Int64 lruStmtWithCKLen_;

  char * predicate_;
  Int64 predicateLen_;

  LongRunningType type_;

  // Is not currently used
  ItemExpr * predicateExpr_;

  // The N in the COMMIT EVERY N ROWS. 
  ULng32 multiCommitSize_;

  // MV NOMVLOG option for LRU operations
  NABoolean isNoLogOperation_;

};

////////////////////////////////////////////////////////////////////
// This class is used to return UID for InMemory objects.
// These objects are not in metadata so this information cannot
// be retrieved from there.
// It is currently used to create fake statistics in the histogram
// tables.
////////////////////////////////////////////////////////////////////
class ExeUtilGetUID : public ExeUtilExpr
{
public:
  ExeUtilGetUID(const CorrName &name,
		enum ExeUtilMaintainObject::MaintainObjectType type,
		CollHeap *oHeap = CmpCommon::statementHeap());

  
  virtual RelExpr * copyTopNode(RelExpr *derivedNode = NULL,
				CollHeap* outHeap = 0);

  virtual NABoolean producesOutput() { return TRUE; }

  virtual RelExpr * bindNode(BindWA *bindWAPtr);

  virtual short codeGen(Generator*);

  virtual const char 	*getVirtualTableName();
  virtual desc_struct 	*createVirtualTableDesc();

private:
  // using this enum from MaintainObject class as it serves the
  // purpose. We could also move this enum to base ExeUtilExpr. TBD.
  ExeUtilMaintainObject::MaintainObjectType type_;

  Int64 uid_;
};

//////////////////////////////////////////////////////////////////////////
// This class is used to look for statistics of sourceTable in the 
// histograms tables of the sourceStatsSchema, if specified, and move it to 
// the histogram tables of the schema where the inMem table is created.
// If no sourceStatsSchema is specified, then statistics are retrieved from
// the histogram tables of the schema where sourceTable is created.
//////////////////////////////////////////////////////////////////////////
class ExeUtilPopulateInMemStats : public ExeUtilExpr
{
public:
  ExeUtilPopulateInMemStats(const CorrName &inMemTableName,
			    const CorrName &sourceTableName,
			    const SchemaName * sourceStatsSchemaName,
			    CollHeap *oHeap = CmpCommon::statementHeap());

  
  virtual RelExpr * copyTopNode(RelExpr *derivedNode = NULL,
				CollHeap* outHeap = 0);

  virtual RelExpr * bindNode(BindWA *bindWAPtr);

  virtual short codeGen(Generator*);

  const CorrName & getInMemTableName() const { return inMemTableName_; }
  CorrName & getInMemTableName() { return inMemTableName_; }

  const CorrName & getSourceTableName() const { return sourceTableName_; }
  CorrName & getSourceTableName() { return sourceTableName_; }

  const SchemaName & getSourceStatsSchemaName() const 
  { return sourceStatsSchemaName_; }
  SchemaName & getSourceStatsSchemaName() { return sourceStatsSchemaName_; }

private:
  
  // InMem table whose stats are to be populated
  CorrName inMemTableName_;

  // table UID of the inMem table
  Int64 uid_;

  // source table whose stats are to be used to populate InMem table's stats
  CorrName sourceTableName_;

  // schema where source table's statistics are present.
  // If this is passed in as NULL, then the schema of sourceTableName is used.
  SchemaName sourceStatsSchemaName_;
};


class ExeUtilHbaseDDL : public ExeUtilExpr
{
public:
  enum DDLtype
  {
    INIT_MD_,
    DROP_MD_,
    CREATE_,
    DROP_
  };

 ExeUtilHbaseDDL(const CorrName &hbaseTableName,
		 DDLtype type,
		 ConstStringList * csl,
		 CollHeap *oHeap = CmpCommon::statementHeap())
   : ExeUtilExpr(HBASE_DDL_, hbaseTableName,
		 NULL, NULL, 
		 NULL, CharInfo::UnknownCharSet, oHeap),
    type_(type),
    csl_(csl)
  {
  };

  virtual RelExpr * copyTopNode(RelExpr *derivedNode = NULL,
				CollHeap* outHeap = 0);

  // method to do code generation
  virtual short codeGen(Generator*);
  
private:
  ConstStringList * csl_;
  DDLtype type_;
};

class ExeUtilHbaseCoProcAggr : public ExeUtilExpr
{
 public:
 ExeUtilHbaseCoProcAggr(const CorrName &corrName,
			ValueIdSet &aggregateExpr,
			CollHeap *oHeap = CmpCommon::statementHeap())
   : ExeUtilExpr(HBASE_COPROC_AGGR_, corrName,
		 NULL, NULL, 
		 NULL, CharInfo::UnknownCharSet, oHeap),
     aggregateExpr_(aggregateExpr),
     corrName_(corrName)
  {
  };

  virtual RelExpr * copyTopNode(RelExpr *derivedNode = NULL,
				CollHeap* outHeap = 0);
  
  virtual NABoolean producesOutput() { return TRUE; }
  
  virtual void getPotentialOutputValues(ValueIdSet & outputValues) const;

  virtual RelExpr * bindNode(BindWA *bindWAPtr);

  RelExpr * preCodeGen(Generator * generator,
		       const ValueIdSet & externalInputs,
		       ValueIdSet &pulledNewInputs);
  
  // method to do code generation
  virtual short codeGen(Generator*);

  ValueIdSet &aggregateExpr() { return aggregateExpr_; }
  const ValueIdSet &aggregateExpr() const { return aggregateExpr_; }

  CorrName& getCorrName() { return corrName_; }

 private:
  ValueIdSet aggregateExpr_;
  CorrName corrName_;
  
};

class ExeUtilOrcFastAggr : public ExeUtilExpr
{
 public:
 ExeUtilOrcFastAggr(const CorrName &corrName,
                    ValueIdSet &aggregateExpr,
                    CollHeap *oHeap = CmpCommon::statementHeap())
   : ExeUtilExpr(ORC_FAST_AGGR_, corrName,
		 NULL, NULL, 
		 NULL, CharInfo::UnknownCharSet, oHeap),
    aggregateExpr_(aggregateExpr)
    {
    };
  
  virtual RelExpr * copyTopNode(RelExpr *derivedNode = NULL,
				CollHeap* outHeap = 0);
  
  virtual NABoolean producesOutput() { return TRUE; }
  
  virtual void getPotentialOutputValues(ValueIdSet & outputValues) const;

  virtual RelExpr * bindNode(BindWA *bindWAPtr);

  RelExpr * preCodeGen(Generator * generator,
		       const ValueIdSet & externalInputs,
		       ValueIdSet &pulledNewInputs);
  
  // method to do code generation
  virtual short codeGen(Generator*);

  ValueIdSet &aggregateExpr() { return aggregateExpr_; }
  const ValueIdSet &aggregateExpr() const { return aggregateExpr_; }

 private:
  ValueIdSet aggregateExpr_;
  
};

class ExeUtilMetadataUpgrade : public ExeUtilExpr
{
public:
 ExeUtilMetadataUpgrade(CollHeap *oHeap = CmpCommon::statementHeap())
   : ExeUtilExpr ( METADATA_UPGRADE_
		   , CorrName("dummyName"), NULL, NULL
		   , NULL                             // in - char * stmt
		   , CharInfo::UnknownCharSet         // in - CharInfo::CharSet stmtCharSet
		   , oHeap                            // in - CollHeap * heap
		   ),
    myFlags_(0)
    {
    };
  
  virtual RelExpr * copyTopNode(RelExpr *derivedNode = NULL,
				CollHeap* outHeap = 0);
  
  virtual NABoolean producesOutput() { return TRUE; }

  virtual NABoolean isExeUtilQueryType() { return TRUE; }
  
  // method to do code generation
  virtual short codeGen(Generator*);
  
  void setGetMDVersion(NABoolean v)
  {(v ? myFlags_ |= GET_MD_VERSION : myFlags_ &= ~GET_MD_VERSION); }
  NABoolean getMDVersion() { return (myFlags_ & GET_MD_VERSION) != 0;}

  void setGetSWVersion(NABoolean v)
  {(v ? myFlags_ |= GET_SW_VERSION : myFlags_ &= ~GET_SW_VERSION); }
  NABoolean getSWVersion() { return (myFlags_ & GET_SW_VERSION) != 0;}

 private:
  enum Flags
  {
    GET_MD_VERSION   = 0x0001,
    GET_SW_VERSION = 0x0002
  };

  UInt32 myFlags_;
};

class ExeUtilHBaseBulkLoad : public ExeUtilExpr
{
public:


  enum HBaseBulkLoadOptionType {
    NO_ROLLBACK_,
    TRUNCATE_TABLE_,
    LOG_ERRORS_,
    STOP_AFTER_N_ERRORS_,
    NO_DUPLICATE_CHECK_,
    NO_POPULATE_INDEXES_,
    CONSTRAINTS_,
    NO_OUTPUT_,
    INDEX_TABLE_ONLY_,
    UPSERT_USING_LOAD_
  };

    class HBaseBulkLoadOption
    {
      friend class ExeUtilHBaseBulkLoad;
    public:
      HBaseBulkLoadOption(HBaseBulkLoadOptionType option, Lng32 numericVal, char * stringVal )
      : option_(option), numericVal_(numericVal), stringVal_(stringVal)
    {
    }  ;

        private:
          HBaseBulkLoadOptionType option_;
          Lng32   numericVal_;
          char * stringVal_;
    };

  ExeUtilHBaseBulkLoad(const CorrName &hBaseTableName,
                   ExprNode * exprNode,
                   char * stmtText,
                   CharInfo::CharSet stmtTextCharSet,
                   CollHeap *oHeap = CmpCommon::statementHeap())
   : ExeUtilExpr(HBASE_LOAD_, hBaseTableName, exprNode, NULL,
                 stmtText, stmtTextCharSet, oHeap),
    //preLoadCleanup_(FALSE),
    keepHFiles_(FALSE),
    truncateTable_(FALSE),
    noRollback_(FALSE),
    logErrors_(FALSE),
    noDuplicates_(TRUE),
    indexes_(TRUE),
    constraints_(FALSE),
    noOutput_(FALSE),
    indexTableOnly_(FALSE),
    upsertUsingLoad_(FALSE)
  {
  };

  virtual const NAString getText() const;

  virtual RelExpr * copyTopNode(RelExpr *derivedNode = NULL,
                                CollHeap* outHeap = 0);

  virtual short codeGen(Generator*);

  NABoolean getKeepHFiles() const
  {
    return keepHFiles_;
  }

  void setKeepHFiles(NABoolean keepHFiles)
  {
    keepHFiles_ = keepHFiles;
  }
  NABoolean getLogErrors() const
  {
    return logErrors_;
  }

  void setLogErrors(NABoolean logErrors)
  {
    logErrors_ = logErrors;
  }

  NABoolean getNoRollback() const
  {
    return noRollback_;
  }

  void setNoRollback(NABoolean noRollback)
  {
    //4489 bulk load option $0~String0 cannot be specified more than once.
    noRollback_ = noRollback;
  }

  NABoolean getTruncateTable() const
  {
    return truncateTable_;
  }

  void setTruncateTable(NABoolean truncateTable)
  {
    truncateTable_ = truncateTable;
  }

  NABoolean getNoDuplicates() const
  {
    return noDuplicates_;
  }

  void setNoDuplicates(NABoolean v)
  {
    noDuplicates_ = v;
  }
  NABoolean getConstraints() const
  {
    return constraints_;
  }

  void setConstraints(NABoolean constraints)
  {
   constraints_ = constraints;
  }

  NABoolean getIndexes() const
  {
   return indexes_;
  }

  void setIndexes(NABoolean indexes)
  {
   indexes_ = indexes;
  }

  NABoolean getNoOutput() const
  {
   return noOutput_;
  }

  void setNoOutput(NABoolean noOutput)
  {
   noOutput_ = noOutput;
  }

  NABoolean getIndexTableOnly() const {
   return indexTableOnly_;
 }

 void setIndexTableOnly(NABoolean indexTableOnly) {
   indexTableOnly_ = indexTableOnly;
 }
  NABoolean getUpsertUsingLoad() const
  {
   return upsertUsingLoad_;
  }

 void setUpsertUsingLoad(NABoolean upsertUsingLoad)
 {
   upsertUsingLoad_ = upsertUsingLoad;
 }

  virtual NABoolean isExeUtilQueryType() { return TRUE; }
  virtual NABoolean producesOutput() { return (noOutput_ ? FALSE : TRUE); }

  short setOptions(NAList<ExeUtilHBaseBulkLoad::HBaseBulkLoadOption*> *
      hBaseBulkLoadOptionList,
      ComDiagsArea * da);
private:

  //NABoolean preLoadCleanup_;
  NABoolean keepHFiles_;
  NABoolean truncateTable_;
  NABoolean noRollback_;
  NABoolean logErrors_;
  NABoolean noDuplicates_;
  NABoolean indexes_;
  NABoolean constraints_;
  NABoolean noOutput_;
  //target table is index table
  NABoolean indexTableOnly_;
  NABoolean upsertUsingLoad_;

};

//hbase bulk load task
class ExeUtilHBaseBulkLoadTask : public ExeUtilExpr
{
public:

  enum TaskType
  {
    NOT_SET_,
    TRUNCATE_TABLE_,
    TAKE_SNAPSHOT, //for recovery???  -- not implemented yet
    PRE_LOAD_CLEANUP_,
    COMPLETE_BULK_LOAD_,
    COMPLETE_BULK_LOAD_N_KEEP_HFILES_,
    EXCEPTION_TABLE_,  //or file  -- not implemented yet
    EXCEPTION_ROWS_PERCENTAGE_, // --not implemented yet
    EXCEPTION_ROWS_NUMBER_,     // -- not implemneted yet
  };
  ExeUtilHBaseBulkLoadTask(const CorrName &hBaseTableName,
                   ExprNode * exprNode,
                   char * stmtText,
                   CharInfo::CharSet stmtTextCharSet,
                   TaskType ttype,
                   CollHeap *oHeap = CmpCommon::statementHeap())
   : ExeUtilExpr(HBASE_LOAD_TASK_, hBaseTableName, exprNode, NULL,
                 stmtText, stmtTextCharSet, oHeap),
    taskType_(ttype)
  {

  };

  virtual const NAString getText() const;

  virtual RelExpr * copyTopNode(RelExpr *derivedNode = NULL,
                                CollHeap* outHeap = 0);

  // method to do code generation
  virtual short codeGen(Generator*);


  virtual NABoolean isExeUtilQueryType() { return TRUE; }

private:

  TaskType taskType_;
};


//------------------------------------------
// Bulk Unload
//-----------------
class ExeUtilHBaseBulkUnLoad : public ExeUtilExpr
{
public:

  enum CompressionType
  {
    NONE_ = 0,
    GZIP_ = 1
  };

    ExeUtilHBaseBulkUnLoad(const CorrName &hBaseTableName,
                     ExprNode * exprNode,
                     char * stmtText,
                     CharInfo::CharSet stmtTextCharSet,
                     CollHeap *oHeap = CmpCommon::statementHeap())
     : ExeUtilExpr(HBASE_UNLOAD_, hBaseTableName, exprNode, NULL,
                   stmtText, stmtTextCharSet, oHeap),
      emptyTarget_(FALSE),
      logErrors_(FALSE),
      noOutput_(FALSE),
      oneFile_(FALSE),
      compressType_(NONE_),
      extractLocation_( oHeap),
      overwriteMergeFile_(FALSE)
    {
    };
  ExeUtilHBaseBulkUnLoad(const CorrName &hBaseTableName,
                   ExprNode * exprNode,
                   char * stmtText,
                   NAString * extractLocation,
                   CharInfo::CharSet stmtTextCharSet,
                   CollHeap *oHeap = CmpCommon::statementHeap())
   : ExeUtilExpr(HBASE_UNLOAD_, hBaseTableName, exprNode, NULL,
                 stmtText, stmtTextCharSet, oHeap),
    emptyTarget_(FALSE),
    logErrors_(FALSE),
    noOutput_(FALSE),
    oneFile_(FALSE),
    compressType_(NONE_),
    extractLocation_(*extractLocation, oHeap),
    overwriteMergeFile_(FALSE)
  {
  };

  virtual const NAString getText() const;

  virtual RelExpr * copyTopNode(RelExpr *derivedNode = NULL,
                                CollHeap* outHeap = 0);
  virtual short codeGen(Generator*);

  ExplainTuple *addSpecificExplainInfo(ExplainTupleMaster *explainTuple, ComTdb * tdb, Generator *generator);

  NABoolean getLogErrors() const
  {
    return logErrors_;
  }

  void setLogErrors(NABoolean logErrors){
    logErrors_ = logErrors;
  }


  NABoolean getEmptyTarget() const  {
    return emptyTarget_;
  }

  void setEmptyTarget(NABoolean emptyTarget)  {
    emptyTarget_ = emptyTarget;
  }
  NABoolean getNoOutput() const  {
   return noOutput_;
  }
  void setNoOutput(NABoolean noOutput) {
   noOutput_ = noOutput;
  }
  NABoolean getCompressType() const {
    return compressType_;
  }
  void setCompressType(CompressionType cType) {
    compressType_ = cType;
  }
  NABoolean getOneFile() const {
    return oneFile_;
  }
  void setOneFile(NABoolean onefile) {
    oneFile_ = onefile;
  }
  virtual NABoolean isExeUtilQueryType() { return TRUE; }
  virtual NABoolean producesOutput() { return (noOutput_ ? FALSE : TRUE); }

  short setOptions(NAList<UnloadOption*> * unlodOptionList,  ComDiagsArea * da);
  void buildWithClause(NAString & withClauseStr, char * str);

  NABoolean getOverwriteMergeFile() const
  {
    return overwriteMergeFile_;
  }

  void setOverwriteMergeFile(NABoolean overwriteMergeFile)
  {
    overwriteMergeFile_ = overwriteMergeFile;
  }

private:
  NABoolean emptyTarget_;
  NABoolean logErrors_;
  NABoolean noOutput_;
  //NABoolean compress_;
  NABoolean oneFile_;
  NAString mergePath_;
  CompressionType compressType_;
  NAString extractLocation_;
  NABoolean overwriteMergeFile_;
};


#endif /* RELEXEUTIL_H */
