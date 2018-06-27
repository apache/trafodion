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
#include "PrivMgrMD.h"


// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class GenericUtilExpr;
class ExeUtilExpr;

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
    xnNeeded_(TRUE),
    virtualTabId_(NULL)
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

  GenericUtilExpr(OperatorTypeEnum otype) 
       : RelExpr(otype, NULL, NULL, NULL)
  {};

  virtual RelExpr * bindNode(BindWA *bindWAPtr);

  // method to do code generation
  RelExpr * preCodeGen(Generator * generator,
		       const ValueIdSet & externalInputs,
		       ValueIdSet &pulledNewInputs);
  virtual short codeGen(Generator*);

  // no return from this method, the atp and atpindex is changed to
  // returned atp (= 0) and returned atp index (last entry of returnedDesc).
  // If noAtpOrIndexChange flag is set, then they are not changed.
  short processOutputRow(Generator * generator,
                         const Int32 work_atp, 
                         const Int32 output_row_atp_index,
                         ex_cri_desc * returnedDesc,
                         NABoolean noAtpOrIndexChange = FALSE);

  // The set of values that I can potentially produce as output.
  virtual void getPotentialOutputValues(ValueIdSet & vs) const;

  // cost functions
  virtual PhysicalProperty *synthPhysicalProperty(const Context *context,
						  const Lng32     planNumber,
                                                  PlanWorkSpace  *pws);

  // this is both logical and physical node
  virtual NABoolean isLogical() const{return TRUE;};
  virtual NABoolean isPhysical() const{return TRUE;};

  virtual NABoolean producesOutput() { return FALSE; }
  virtual const char 	*getVirtualTableName() { return NULL;};
  virtual TrafDesc 	*createVirtualTableDesc() { return NULL;};
  TableDesc * getVirtualTableDesc() const
  {
    return virtualTabId_;
  };

  void setVirtualTableDesc(TableDesc * vtdesc)
  {
    virtualTabId_ = vtdesc;
  };

  // various PC methods

  // get the degree of this node 
  virtual Int32 getArity() const { return 0; };
  virtual NABoolean duplicateMatch(const RelExpr & other) const;
  virtual RelExpr * copyTopNode(RelExpr *derivedNode = NULL,
				CollHeap* outHeap = 0);

  // method required for traversing an ExprNode tree
  // access a child of an ExprNode
  //virtual ExprNode * getChild(long index);

  ExprNode * getExprNode(){return exprNode_;};
  const ExprNode * getExprNode() const {return exprNode_;};

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

 protected:
  // the parse tree version of the statement, if needed
  ExprNode * exprNode_;

  // the string version of the statement.
  char *     stmtText_;
  CharInfo::CharSet stmtTextCharSet_;

  CollHeap * heap_;

  // should master exe start a xn before executing this request?
  NABoolean xnNeeded_;

  // a unique identifer for the virtual table
  TableDesc * virtualTabId_;

}; // class GenericUtilExpr

// -----------------------------------------------------------------------
// The DDLExpr class is used to represent a DDL query that is treated
// like a DML. It points to the actual parse tree representing the DDL
// query.
// -----------------------------------------------------------------------
class DDLExpr : public GenericUtilExpr
{
  void setDDLXns(NABoolean v)
  {
    if (v)
      ddlXns_ = (CmpCommon::getDefault(DDL_TRANSACTIONS) == DF_ON);
    else
      ddlXns_ = FALSE;
  }

public:
  DDLExpr(ExprNode * ddlNode,
	  char * ddlStmtText,
	  CharInfo::CharSet ddlStmtTextCharSet,
	  CollHeap *oHeap)
       : GenericUtilExpr(ddlStmtText, ddlStmtTextCharSet, ddlNode, NULL, 
                         REL_DDL, oHeap),
         specialDDL_(FALSE),
         ddlObjNATable_(NULL),
         numExplRows_(0),
         isCreate_(FALSE), isCreateLike_(FALSE), isVolatile_(FALSE), 
         isDrop_(FALSE), isAlter_(FALSE), isCleanup_(FALSE),
         isTable_(FALSE), isIndex_(FALSE), isMV_(FALSE), isView_(FALSE),
         isSchema_(FALSE), isLibrary_(FALSE), isRoutine_(FALSE),
         isUstat_(FALSE),
         isHbase_(FALSE),
         isNative_(FALSE),
         returnStatus_(FALSE),
         flags_(0)
  {
    setDDLXns(TRUE);
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

  void setReturnStatus(NABoolean v) { returnStatus_ = v; }
  virtual NABoolean producesOutput() { return returnStatus_;}
  virtual const char 	*getVirtualTableName();
  virtual TrafDesc 	*createVirtualTableDesc();

  ExprNode * getDDLNode(){return getExprNode();};
  const ExprNode * getDDLNode() const {return getExprNode();};

  char * getDDLStmtText()
  {
    return getStmtText();
  }

  Lng32 getDDLStmtTextCharSet() const
  {
    return getStmtTextCharSet();
  }

  CorrName &purgedataTableName() { return purgedataTableName_; }
  void setPurgedataTableName(CorrName &cn) 
  { 
    purgedataTableName_ = cn; 
    qualObjName_ = cn.getQualifiedNameObj();
  }

  NABoolean &specialDDL() { return specialDDL_;}
  NABoolean &isUstat() { return isUstat_; }

  QualifiedName &explObjName() { return explObjName_; }
  void setExplObjName(QualifiedName &qn) { explObjName_ = qn; }

  double numExplRows() { return numExplRows_; }
  void setNumExplRows(double nr) { numExplRows_ = nr; }

  virtual NABoolean dontUseCache() 
  { return (showddlExplain() ? FALSE : TRUE); }

  short ddlXnsInfo(NABoolean &ddlXns, NABoolean &xnCanBeStarted);

  NAString getQualObjName() { return qualObjName_.getQualifiedNameAsAnsiString(); }

  void setCreateMDViews(NABoolean v)
  {(v ? flags_ |= CREATE_MD_VIEWS : flags_ &= ~CREATE_MD_VIEWS); }
  NABoolean createMDViews() { return (flags_ & CREATE_MD_VIEWS) != 0;}

  void setDropMDViews(NABoolean v)
  {(v ? flags_ |= DROP_MD_VIEWS : flags_ &= ~DROP_MD_VIEWS); }
  NABoolean dropMDViews() { return (flags_ & DROP_MD_VIEWS) != 0;}

  void setGetMDVersion(NABoolean v)
  {(v ? flags_ |= GET_MD_VERSION : flags_ &= ~GET_MD_VERSION); }
  NABoolean getMDVersion() { return (flags_ & GET_MD_VERSION) != 0;}

  void setCreateLibmgr(NABoolean v)
  {(v ? flags_ |= CREATE_LIBMGR : flags_ &= ~CREATE_LIBMGR); }
  NABoolean createLibmgr() { return (flags_ & CREATE_LIBMGR) != 0;}

  void setDropLibmgr(NABoolean v)
  {(v ? flags_ |= DROP_LIBMGR : flags_ &= ~DROP_LIBMGR); }
  NABoolean dropLibmgr() { return (flags_ & DROP_LIBMGR) != 0;}

  void setUpgradeLibmgr(NABoolean v)
  {(v ? flags_ |= UPGRADE_LIBMGR : flags_ &= ~UPGRADE_LIBMGR); }
  NABoolean upgradeLibmgr() { return (flags_ & UPGRADE_LIBMGR) != 0;}

  void setCreateRepos(NABoolean v)
  {(v ? flags_ |= CREATE_REPOS : flags_ &= ~CREATE_REPOS); }
  NABoolean createRepos() { return (flags_ & CREATE_REPOS) != 0;}

  void setDropRepos(NABoolean v)
  {(v ? flags_ |= DROP_REPOS : flags_ &= ~DROP_REPOS); }
  NABoolean dropRepos() { return (flags_ & DROP_REPOS) != 0;}

  void setUpgradeRepos(NABoolean v)
  {(v ? flags_ |= UPGRADE_REPOS : flags_ &= ~UPGRADE_REPOS); }
  NABoolean upgradeRepos() { return (flags_ & UPGRADE_REPOS) != 0;}

  void setCleanupAuth(NABoolean v)
  {(v ? flags_ |= CLEANUP_AUTH : flags_ &= ~CLEANUP_AUTH); }
  NABoolean cleanupAuth() { return (flags_ & CLEANUP_AUTH) != 0;}

  void setInitHbase(NABoolean v)
  {(v ? flags_ |= INIT_HBASE : flags_ &= ~INIT_HBASE); }
  NABoolean initHbase() { return (flags_ & INIT_HBASE) != 0;}

  void setDropHbase(NABoolean v)
  {(v ? flags_ |= DROP_HBASE : flags_ &= ~DROP_HBASE); }
  NABoolean dropHbase() { return (flags_ & DROP_HBASE) != 0;}

  void setInitAuth(NABoolean v)
  {(v ? flags_ |= INIT_AUTH : flags_ &= ~INIT_AUTH); }
  NABoolean initAuth() { return (flags_ & INIT_AUTH) != 0;}

  void setDropAuth(NABoolean v)
  {(v ? flags_ |= DROP_AUTH : flags_ &= ~DROP_AUTH); }
  NABoolean dropAuth() { return (flags_ & DROP_AUTH) != 0;}

  void setUpdateVersion(NABoolean v)
  {(v ? flags_ |= UPDATE_VERSION : flags_ &= ~UPDATE_VERSION); }
  NABoolean updateVersion() { return (flags_ & UPDATE_VERSION) != 0;}

  void setAddSchemaObjects(NABoolean v)
  {(v ? flags_ |= ADD_SCH_OBJS : flags_ &= ~ADD_SCH_OBJS); }
  NABoolean addSchemaObjects() { return (flags_ & ADD_SCH_OBJS) != 0;}

  // minimal: meaningful only when initHbase_ is true; if this is true,
  // means create the metadata tables only (and not repository etc.)
  void setMinimal(NABoolean v)
  {(v ? flags_ |= MINIMAL : flags_ &= ~MINIMAL); }
  NABoolean minimal() { return (flags_ & MINIMAL) != 0;}

  void setPurgedata(NABoolean v)
  {(v ? flags_ |= PURGEDATA : flags_ &= ~PURGEDATA); }
  NABoolean purgedata() { return (flags_ & PURGEDATA) != 0;}

  void setPurgedataIfExists(NABoolean v)
  {(v ? flags_ |= PURGEDATA_IF_EXISTS : flags_ &= ~PURGEDATA_IF_EXISTS); }
  NABoolean purgedataIfExists() { return (flags_ & PURGEDATA_IF_EXISTS) != 0;}

  // this ddlexpr is created for 'showddl <obj>, explain' to
  // explain the object explObjName.
  void setShowddlExplain(NABoolean v)
  {(v ? flags_ |= SHOWDDL_EXPLAIN : flags_ &= ~SHOWDDL_EXPLAIN); }
  NABoolean showddlExplain() { return (flags_ & SHOWDDL_EXPLAIN) != 0;}

  void setShowddlExplainInt(NABoolean v)
  {(v ? flags_ |= SHOWDDL_EXPLAIN_INT : flags_ &= ~SHOWDDL_EXPLAIN_INT); }
  NABoolean showddlExplainInt() { return (flags_ & SHOWDDL_EXPLAIN_INT) != 0;}

  void setNoLabelStats(NABoolean v)
  {(v ? flags_ |= NO_LABEL_STATS : flags_ &= ~NO_LABEL_STATS); }
  NABoolean noLabelStats() { return (flags_ & NO_LABEL_STATS) != 0;}

  NABoolean ddlXns() { return ddlXns_; }

 protected:
  enum Flags
  {
    CREATE_MD_VIEWS         = 0x000001,
    DROP_MD_VIEWS           = 0x000002,
    GET_MD_VERSION          = 0x000004,
    CREATE_REPOS            = 0x000008,
    DROP_REPOS              = 0x000010,
    UPGRADE_REPOS           = 0x000020,
    CLEANUP_AUTH            = 0X000040,
    CREATE_LIBMGR           = 0x000080,
    DROP_LIBMGR             = 0x000100,
    UPGRADE_LIBMGR          = 0x000200,
    INIT_HBASE              = 0x000400,
    DROP_HBASE              = 0x000800,
    INIT_AUTH               = 0x001000,
    DROP_AUTH               = 0x002000,
    UPDATE_VERSION          = 0x004000,
    ADD_SCH_OBJS            = 0x008000,
    MINIMAL                 = 0x010000,
    PURGEDATA               = 0x020000,
    SHOWDDL_EXPLAIN         = 0x040000,
    SHOWDDL_EXPLAIN_INT     = 0x080000,
    NO_LABEL_STATS          = 0x100000,
    PURGEDATA_IF_EXISTS     = 0x200000,
  };

  // see method processSpecialDDL in sqlcomp/parser.cpp
  NABoolean specialDDL_;

  // NATable for the DDL object. Used to generate EXPLAIN information.
  // Set during bindNode phase.
  NATable * ddlObjNATable_;

  QualifiedName explObjName_;
  double numExplRows_;  // number of rows specified by user or actual count

  NABoolean isCreate_;
  NABoolean isCreateLike_;
  NABoolean isVolatile_;
  NABoolean isSchema_;
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

  // if set, this ddl cannot run under a user transaction. 
  // It must run in autocommit mode.
  NABoolean hbaseDDLNoUserXn_;

  // set for create table index/MV only. Used during InMemory table
  // processing.	  
  NAString objName_;

  NABoolean isDrop_;
  NABoolean isAlter_;
  NABoolean isCleanup_;

  CorrName purgedataTableName_;
  QualifiedName qualObjName_;

  // if TRUE, then status is returned during ddl operation.
  // Executor communicates with arkcmp and returns status rows.
  NABoolean returnStatus_;

  UInt32 flags_;

  // if TRUE, ddl transactions are enabled. Actual operation may
  // run under one transaction or multiple transactions.
  // Details in sqlcomp/CmpSeabaseDDL*.cpp.
  NABoolean ddlXns_;
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
    DISPLAY_EXPLAIN_          = 2,
    MAINTAIN_OBJECT_          = 3,
    LOAD_VOLATILE_            = 4,
    CLEANUP_VOLATILE_TABLES_  = 5,
    GET_VOLATILE_INFO_        = 6,
    CREATE_TABLE_AS_          = 7,
    GET_STATISTICS_           = 9,
    LONG_RUNNING_             = 11,
    GET_METADATA_INFO_        = 12,
    GET_VERSION_INFO_         = 13,
    SUSPEND_ACTIVATE_         = 14,
    REGION_STATS_         = 15,
    LOB_INFO_             =16,
    SHOWSET_DEFAULTS_         = 18,
    AQR_                      = 19,
    DISPLAY_EXPLAIN_COMPLEX_  = 20,
    GET_UID_                  = 21,
    POP_IN_MEM_STATS_         = 22,
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
    ORC_FAST_AGGR_            = 37,
    GET_QID_                  = 38,
    HIVE_TRUNCATE_LEGACY_     = 39,
    LOB_UPDATE_UTIL_          = 40,
    HIVE_QUERY_               = 41,
    HIVE_TRUNCATE_            = 45
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
         stoi_(NULL)
  {
  };

  ExeUtilExpr()
       : GenericUtilExpr(REL_EXE_UTIL) {};

  virtual HashValue topHash();
  virtual NABoolean duplicateMatch(const RelExpr & other) const;
  virtual RelExpr * copyTopNode(RelExpr *derivedNode = NULL,
				CollHeap* outHeap = 0);
  virtual const NAString getText() const;

  virtual RelExpr * bindNode(BindWA *bindWAPtr);

  virtual RelExpr * preCodeGen(Generator * generator,
			       const ValueIdSet & externalInputs,
			       ValueIdSet &pulledNewInputs);

  NABoolean pilotAnalysis(QueryAnalysis* qa);

  virtual NABoolean producesOutput() { return FALSE; }
  virtual const char 	*getVirtualTableName();
  virtual TrafDesc 	*createVirtualTableDesc();

  // exeutil statements whose query type need to be returned as 
  // SQL_EXE_UTIL. Set during RelRoot::codeGen in ComTdbRoot class.
  virtual NABoolean isExeUtilQueryType() { return FALSE; }

  virtual NABoolean aqrSupported() { return FALSE; }

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

  NABoolean checkForComponentPriv(SQLOperation operation, BindWA *bindQA);

  void setupStoiForPrivs(SqlTableOpenInfo::AccessFlags privs, BindWA *bindWA);

  OptSqlTableOpenInfo *getOptStoi() const
  {
    return stoi_;
  }

  void setOptStoi(OptSqlTableOpenInfo *stoi)
  {
    stoi_ = stoi;
  }

  virtual NABoolean explainSupported() { return FALSE; }

  virtual NABoolean dontUseCache() { return FALSE; }

protected:
  ExeUtilType type_;

  // name of the table affected by the operation
  CorrName tableName_;

  // a unique identifer for the table specified by tableName_
  TableDesc *tableId_;  

  // for special privilege checks - add a stoi
  OptSqlTableOpenInfo *stoi_;
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
  virtual TrafDesc 	*createVirtualTableDesc();

  virtual NABoolean producesOutput() { return TRUE; }

  NABoolean isOptionE() { return ((flags_ & OPTION_E) != 0); };
  NABoolean isOptionF() { return ((flags_ & OPTION_F) != 0); };
  NABoolean isOptionM() { return ((flags_ & OPTION_M) != 0); };
  NABoolean isOptionN() { return ((flags_ & OPTION_N) != 0); };

  // this option is used to cleanse and return deterministic explain output.
  // when it is set, non-deterministic fields are filtered out and replaced
  // with a deterministic pattern. Fields like cost or num esps, etc.
  // Used during traf regressions run with explain/explain_options_f stmts
  // to cleanse non-deterministic fields, if those fields are not relevant.
  // Filtererd patterns are discussed in executor/ExExeUtilExplain.cpp.
  NABoolean isOptionC() { return ((flags_ & OPTION_C) != 0); };

  // this option will cleanse(optionC()), prune and not return cleansed rows.
  // This is useful to reduce the amount of explain output by eliminating
  // cleansed rows.
  NABoolean isOptionP() { return ((flags_ & OPTION_P) != 0); };

protected:
  enum OpToFlag
  {
    // formatted explain
    OPTION_F      = 0x0001,

    // expert mode explain
    OPTION_E      = 0x0002,

    // machine readable explain
    OPTION_M      = 0x0004,

    // normal full explain
    OPTION_N      = 0x0008,
    
    // cleansed explain
    OPTION_C      = 0x0010,

    // pruned explain
    OPTION_P      = 0x0020
  };

  short setOptionsX();
  short setOptionX(char c, Int32 &numOptions);

  char * moduleName_;
  char * stmtName_;
  char * optionsStr_;

  UInt32 flags_;                                  
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
       : DDLExpr(exprNode, stmtText, stmtTextCharSet, oHeap),
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
       : DDLExpr(exprNode, stmtText, stmtTextCharSet, oHeap)
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

  ExplainTuple *addSpecificExplainInfo(ExplainTupleMaster *explainTuple, 
				       ComTdb * tdb, 
				       Generator *generator);

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

class ExeUtilHiveTruncateLegacy : public ExeUtilExpr
{
public:
  ExeUtilHiveTruncateLegacy(const CorrName &name,
                            ConstStringList * pl,
                            CollHeap *oHeap = CmpCommon::statementHeap())
       : ExeUtilExpr(HIVE_TRUNCATE_LEGACY_, name, NULL, NULL, NULL, 
                     CharInfo::UnknownCharSet, oHeap),
         pl_(pl), suppressModCheck_(FALSE), dropTableOnDealloc_(FALSE),
         noSecurityCheck_(FALSE)
  { }
  
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

  ConstStringList* &partnList() { return pl_; }

  NABoolean getSuppressModCheck() const         { return suppressModCheck_; }
  NABoolean getDropTableOnDealloc() const       { return dropTableOnDealloc_; }
  NABoolean getNoSecurityCheck() const          { return noSecurityCheck_; }

  void setSuppressModCheck(NABoolean v=TRUE)    { suppressModCheck_ = v; }
  void setDropTableOnDealloc(NABoolean v=TRUE)  { dropTableOnDealloc_ = v; }
  void setNoSecurityCheck(NABoolean v)          { noSecurityCheck_ = v; }  
private:
  NAString  hiveTableLocation_;
  NAString hiveHostName_;
  Int32 hiveHdfsPort_;

  // timestamp of hiveTableLocation. 
  Int64 hiveModTS_;

  // list of partitions to be truncated
  ConstStringList * pl_;
  NABoolean suppressModCheck_;
  NABoolean dropTableOnDealloc_;

  // if this truncate node is added internally to process 'insert overwrite'
  // statement, then skip security/privilege checks.
  // Checks will be done when the corresponding insert node is processed.
  NABoolean noSecurityCheck_;
};

///////////////////////////////////////////////////////////
// ExeUtilHiveTruncate
///////////////////////////////////////////////////////////
class ExeUtilHiveTruncate : public ExeUtilExpr
{
public:
  ExeUtilHiveTruncate(CorrName &name,
                      NAString &hiveTableName,
                      NAString &hiveTruncQuery,
                      CollHeap *oHeap = CmpCommon::statementHeap())
       : ExeUtilExpr(HIVE_TRUNCATE_, name, NULL, NULL, NULL, 
                     CharInfo::UnknownCharSet, oHeap),
         hiveTableName_(hiveTableName),
         hiveTruncQuery_(hiveTruncQuery),
         dropTableOnDealloc_(FALSE),
         noSecurityCheck_(FALSE),
         hiveExternalTable_(FALSE),
         ifExists_(FALSE),
         tableNotExists_(FALSE)
  { }

  virtual NABoolean isExeUtilQueryType() { return TRUE; }

  virtual RelExpr * copyTopNode(RelExpr *derivedNode = NULL,
				CollHeap* outHeap = 0);

  virtual RelExpr * bindNode(BindWA *bindWAPtr);

  // method to do code generation
  virtual short codeGen(Generator*);
  
  ExplainTuple *addSpecificExplainInfo(ExplainTupleMaster *explainTuple, 
				       ComTdb * tdb, 
				       Generator *generator);
  
  virtual NABoolean aqrSupported() { return TRUE; }

  const NAString &getHiveTableName() const { return hiveTableName_; }
  const NAString &getHiveTruncQuery() const { return hiveTruncQuery_; }

  NABoolean getDropTableOnDealloc() const       { return dropTableOnDealloc_; }
  NABoolean getNoSecurityCheck() const          { return noSecurityCheck_; }
  NABoolean getHiveExternalTable() const        { return hiveExternalTable_; }
  NABoolean getIfExists() const                 { return ifExists_; }
  NABoolean getTableNotExists()   const         { return tableNotExists_; }

  void setDropTableOnDealloc(NABoolean v=TRUE)  { dropTableOnDealloc_ = v; }
  void setNoSecurityCheck(NABoolean v)          { noSecurityCheck_ = v; }  
  void setHiveExternalTable(NABoolean v)        { hiveExternalTable_ = v; }
  void setIfExists(NABoolean v)                 { ifExists_ = v; }
  void setTableNotExists(NABoolean v)           { tableNotExists_ = v; }

private:

  NAString hiveTableName_;
  NAString hiveTruncQuery_;
  NABoolean dropTableOnDealloc_;

  // if this truncate node is added internally to process 'insert overwrite'
  // statement, then skip security/privilege checks.
  // Checks will be done when the corresponding insert node is processed.
  NABoolean noSecurityCheck_;

  // TRUE: Hive External table. FALSE: Hive Managed table.
  NABoolean hiveExternalTable_;

  // if 'if exist' clause is specified
  NABoolean ifExists_;

  // if table does not exist
  NABoolean tableNotExists_;
};

class ExeUtilHiveQuery : public ExeUtilExpr
{
public:
  enum HiveSourceType
    {
      FROM_STRING,
      FROM_FILE
    };
  ExeUtilHiveQuery(const NAString &hive_query,
                   HiveSourceType type,
                   CollHeap *oHeap = CmpCommon::statementHeap())
       : ExeUtilExpr(HIVE_QUERY_, CorrName("dummyName"), 
                     NULL, NULL, 
                     NULL,
                     CharInfo::UnknownCharSet, oHeap),
         type_(type),
         hiveQuery_(hive_query)
  { }
  virtual NABoolean isExeUtilQueryType() { return TRUE; }
  virtual RelExpr * copyTopNode(RelExpr *derivedNode = NULL,
				CollHeap* outHeap = 0);
  virtual RelExpr * bindNode(BindWA *bindWAPtr);
  virtual short codeGen(Generator*);
  NAString &hiveQuery() { return hiveQuery_; }
  const NAString &hiveQuery() const { return hiveQuery_; }
  HiveSourceType sourceType() { return type_;}
private:
  HiveSourceType type_;
  NAString hiveQuery_;
};
class ExeUtilMaintainObject : public ExeUtilExpr
{
public:
  enum MaintainObjectOptionType
  {
    INITIALIZE_, REINITIALIZE_, DROP_, 
    CREATE_VIEW_, DROP_VIEW_,
    ALL_,
    UPD_STATS_TABLE_, UPD_STATS_MVLOG_,
    UPD_STATS_MVS_, UPD_STATS_MVGROUP_,
    UPD_STATS_ALL_MVS_,
    REFRESH_ALL_MVGROUP_, REFRESH_MVGROUP_, 
    REFRESH_ALL_MVS_, REFRESH_MVS_,
    REFRESH_,
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
  // sl: A single line report of BMO and PERTABLE stats
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
  NABoolean singleLineFormat_;
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
    COMPONENT_OPERATIONS = 12,
    HBASE_OBJECTS_ = 13
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
  
  NABoolean cascade() { return cascade_;}
  void setCascade(NABoolean v) { cascade_ = v; }

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
  NABoolean cascade_;
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

class ExeUtilRegionStats : public ExeUtilExpr 
{
public:
  
  ExeUtilRegionStats(const CorrName &objectName,
                     NABoolean summaryOnly,
                     NABoolean isIndex,
                     NABoolean forDisplay,
                     NABoolean clusterView,
                     RelExpr * child,
                     CollHeap *oHeap = CmpCommon::statementHeap());
  
  ExeUtilRegionStats():
       summaryOnly_(FALSE),
       isIndex_(FALSE),
       displayFormat_(FALSE),
       clusterView_(FALSE)
  {}

  ExeUtilRegionStats(NABoolean clusterView):
       summaryOnly_(FALSE),
       isIndex_(FALSE),
       displayFormat_(FALSE),
       clusterView_(clusterView)
  {}

  virtual RelExpr * bindNode(BindWA *bindWAPtr);

  // a method used for recomputing the outer references (external dataflow
  // input values) that are needed by this operator.
  virtual void recomputeOuterReferences();

  virtual RelExpr * copyTopNode(RelExpr *derivedNode = NULL,
				CollHeap* outHeap = 0);

  // method to do code generation
  virtual short codeGen(Generator*);

  virtual const char 	*getVirtualTableName();
  static const char * getVirtualTableNameStr() 
  { return "EXE_UTIL_REGION_STATS__";}

  virtual TrafDesc 	*createVirtualTableDesc();

  static const char * getVirtualTableClusterViewNameStr() 
  { return "EXE_UTIL_CLUSTER_STATS__"; }

  virtual NABoolean producesOutput() { return TRUE; }

  virtual int getArity() const { return ((child(0) == NULL) ? 0 : 1); }

  virtual NABoolean aqrSupported() { return TRUE; }

private:
  ItemExpr * inputColList_;

  NABoolean summaryOnly_;

  NABoolean isIndex_;

  NABoolean displayFormat_;

  NABoolean clusterView_;

  NABoolean errorInParams_;
};

class ExeUtilLobInfo : public ExeUtilExpr 
{
public:
  
  ExeUtilLobInfo(const CorrName &objectName,
                 NABoolean tableFormat = FALSE,
                     RelExpr * child = NULL,
                     CollHeap *oHeap = CmpCommon::statementHeap());
  
  ExeUtilLobInfo()       
  {}
 
  virtual RelExpr * bindNode(BindWA *bindWAPtr);

  // a method used for recomputing the outer references (external dataflow
  // input values) that are needed by this operator.
  virtual void recomputeOuterReferences();

  virtual RelExpr * copyTopNode(RelExpr *derivedNode = NULL,
				CollHeap* outHeap = 0);

  // method to do code generation
  virtual short codeGen(Generator*);

  virtual const char 	*getVirtualTableName();
  static const char * getVirtualTableNameStr() 
  { return "EXE_UTIL_LOB_INFO__";}
  virtual TrafDesc 	*createVirtualTableDesc();

  virtual NABoolean producesOutput() { return TRUE; }

  virtual int getArity() const { return ((child(0) == NULL) ? 0 : 1); }

 virtual NABoolean aqrSupported() { return TRUE; }

private:
  Int64 objectUID_;
  NABoolean tableFormat_;
  NABoolean errorInParams_;
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
                                                  const Lng32     planNumber,
                                                  PlanWorkSpace  *pws);

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
  virtual TrafDesc 	*createVirtualTableDesc();

private:
  // using this enum from MaintainObject class as it serves the
  // purpose. We could also move this enum to base ExeUtilExpr. TBD.
  ExeUtilMaintainObject::MaintainObjectType type_;

  Int64 uid_;
};

////////////////////////////////////////////////////////////////////
// This class is used to return query id of specified statement.
////////////////////////////////////////////////////////////////////
class ExeUtilGetQID : public ExeUtilExpr
{
public:
  ExeUtilGetQID(NAString &statement,
		CollHeap *oHeap = CmpCommon::statementHeap());
  
  virtual RelExpr * copyTopNode(RelExpr *derivedNode = NULL,
				CollHeap* outHeap = 0);

  virtual NABoolean producesOutput() { return TRUE; }

  //  virtual RelExpr * bindNode(BindWA *bindWAPtr);

  virtual short codeGen(Generator*);

  virtual const char 	*getVirtualTableName();
  virtual TrafDesc 	*createVirtualTableDesc();

private:
  NAString statement_;
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

class ExeUtilLobExtract : public ExeUtilExpr
{
public:
  enum ExtractToType
  {
    TO_FILE_, TO_STRING_, TO_BUFFER_, TO_EXTERNAL_FROM_STRING_,
    TO_EXTERNAL_FROM_FILE_, RETRIEVE_LENGTH_, RETRIEVE_HDFSFILENAME_, 
    RETRIEVE_OFFSET_,NOOP_
  };
  enum ExtractFileActionType
  {
    ERROR_IF_NOT_EXISTS =1, ERROR_IF_EXISTS, TRUNCATE_EXISTING,APPEND_OR_CREATE };
  
  
 ExeUtilLobExtract(ItemExpr * handle, 
		   ExtractToType toType,
		   Int64 bufaddr=0,
		   Int64 extractSizeAddr=0,
		   Int64 intParam = 0,
		   Int64 intParam2 = 0,
		   char * stringParam = NULL,
		   char *stringParam2 = NULL,
		   char *stringParam3 = NULL,
		   RelExpr * childNode = NULL,
		   CollHeap *oHeap = CmpCommon::statementHeap())
   : ExeUtilExpr(LOB_EXTRACT_, CorrName("dummyExtractName"),
		 NULL, childNode, 
		 NULL, CharInfo::UnknownCharSet, oHeap),
    handle_(handle),
    toType_(toType),
    bufAddr_(bufaddr),
    extractSizeAddr_(extractSizeAddr),
    intParam_(intParam),
    intParam2_(intParam2),
    handleInStringFormat_(TRUE),
    withCreate_(TRUE)
    {
      if (stringParam)
	stringParam_ = stringParam;
      if (stringParam2)
	stringParam2_ = stringParam2;
      if (stringParam3)
	stringParam3_ = stringParam3;
    };

  virtual Int32 getArity() const { return (child(0) ? 1 : 0); }
  
  virtual NABoolean isExeUtilQueryType() { return TRUE; }

  //virtual NABoolean producesOutput() { return (toType_ == TO_STRING_ ? TRUE : FALSE); }
  virtual NABoolean producesOutput() { return  TRUE ; }
  virtual RelExpr * copyTopNode(RelExpr *derivedNode = NULL,
				CollHeap* outHeap = 0);
  
  virtual RelExpr * bindNode(BindWA *bindWAPtr);

  virtual void transformNode(NormWA & normWARef,
			     ExprGroupId & locationOfPointerToMe);

  virtual RelExpr * normalizeNode(NormWA & normWARef);

  virtual void pushdownCoveredExpr(const ValueIdSet & outputExpr,
				   const ValueIdSet & newExternalInputs,
				   ValueIdSet & predicatesOnParent,
				   const ValueIdSet * setOfValuesReqdByParent,
				   Lng32 childIndex
				   );
  
  virtual RelExpr * preCodeGen(Generator * generator,
			       const ValueIdSet & externalInputs,
			       ValueIdSet &pulledNewInputs);
  
  // method to do code generation
  virtual short codeGen(Generator*);

  void setHandleInStringFormat(NABoolean v)
  {
    handleInStringFormat_ = v;
  }

  NABoolean &withCreate() { return withCreate_; }

 private:
  //  NAString handle_;
  ItemExpr * handle_;
  ExtractToType toType_;
  
  Int64 bufAddr_;
  Int64 extractSizeAddr_;
  
  Int64 intParam_;   // options for create or size limit
  Int64 intParam2_;// options for file behavior
  NAString stringParam_; // output file name for TO_FILE_.

  NAString stringParam2_;
  NAString stringParam3_;

  NABoolean handleInStringFormat_;

  // create file during load (type TO_EXTERNAL_FROM...)
  NABoolean withCreate_;
};


class ExeUtilLobUpdate : public ExeUtilExpr
{
public:
  enum UpdateFromType
  {
     FROM_BUFFER_, FROM_FILE_, FROM_STRING_,FROM_EXTERNAL_, NOOP_
  };
  enum UpdateActionType
  {
    ERROR_IF_EXISTS_=1, TRUNCATE_EXISTING_,APPEND_, REPLACE_ 
  };
  
  
 ExeUtilLobUpdate(ItemExpr * handle, 
		   UpdateFromType fromType,
		   Int64 bufaddr=0,
		   Int64 updateSize=0,
                  UpdateActionType updateAction=UpdateActionType::ERROR_IF_EXISTS_,
		   RelExpr * childNode = NULL,
		   CollHeap *oHeap = CmpCommon::statementHeap())
   : ExeUtilExpr(LOB_UPDATE_UTIL_, CorrName("dummyUpdateLobName"),
		 NULL, childNode, 
		 NULL, CharInfo::UnknownCharSet, oHeap),
    handle_(handle),
    fromType_(fromType),
    bufAddr_(bufaddr),
     updateSize_(updateSize),
     updateAction_(updateAction)
   
    {
     
    };

  virtual Int32 getArity() const { return (child(0) ? 1 : 0); }
  
  virtual NABoolean isExeUtilQueryType() { return TRUE; }

  //virtual NABoolean producesOutput() { return (toType_ == TO_STRING_ ? TRUE : FALSE); }
  virtual NABoolean producesOutput() { return  TRUE ; }
  virtual RelExpr * copyTopNode(RelExpr *derivedNode = NULL,
				CollHeap* outHeap = 0);
  
  virtual RelExpr * bindNode(BindWA *bindWAPtr);

  virtual void transformNode(NormWA & normWARef,
			     ExprGroupId & locationOfPointerToMe);

  virtual RelExpr * normalizeNode(NormWA & normWARef);

  virtual void pushdownCoveredExpr(const ValueIdSet & outputExpr,
				   const ValueIdSet & newExternalInputs,
				   ValueIdSet & predicatesOnParent,
				   const ValueIdSet * setOfValuesReqdByParent,
				   Lng32 childIndex
				   );
  
  virtual RelExpr * preCodeGen(Generator * generator,
			       const ValueIdSet & externalInputs,
			       ValueIdSet &pulledNewInputs);
  
  // method to do code generation
  virtual short codeGen(Generator*);

  

  

 private:
  //  NAString handle_;
  ItemExpr * handle_;
  UpdateFromType fromType_;
  Int32 updateAction_;
  Int64 bufAddr_;
  Int64 updateSize_;
  
  
};
class ExeUtilLobShowddl : public ExeUtilExpr
{
public:
 ExeUtilLobShowddl(const CorrName &lobTableName,
		   Lng32 sdOptions,
		   CollHeap *oHeap = CmpCommon::statementHeap())
   : ExeUtilExpr(LOB_SHOWDDL_, lobTableName,
		 NULL, NULL, 
		 NULL, CharInfo::UnknownCharSet, oHeap),
    sdOptions_(sdOptions),
    objectUID_(0)
  {
  };

  virtual NABoolean isExeUtilQueryType() { return TRUE; }

  virtual RelExpr * copyTopNode(RelExpr *derivedNode = NULL,
				CollHeap* outHeap = 0);

  virtual NABoolean producesOutput() { return TRUE; }

  virtual RelExpr * bindNode(BindWA *bindWAPtr);

  // method to do code generation
  virtual short codeGen(Generator*);
  
private:
  Lng32 sdOptions_;

  Int64 objectUID_;
};
// sss #endif

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
     corrName_(corrName),
     estRowsAccessed_(-1.0)
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

  virtual ExplainTuple * addSpecificExplainInfo(
       ExplainTupleMaster *explainTuple, ComTdb *tdb, 
       Generator *generator);

  inline const CostScalar getEstRowsAccessed() const 
    { return estRowsAccessed_; }
  inline void setEstRowsAccessed(CostScalar r)  { estRowsAccessed_ = r; }

 private:
  ValueIdSet aggregateExpr_;
  CorrName corrName_;
  CostScalar estRowsAccessed_;
  
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
    UPDATE_STATS_,
    LOG_ERROR_ROWS_,
    STOP_AFTER_N_ERROR_ROWS_,
    NO_DUPLICATE_CHECK_,
    REBUILD_INDEXES_,
    CONSTRAINTS_,
    NO_OUTPUT_,
    INDEX_TABLE_ONLY_,
    UPSERT_USING_LOAD_,
    CONTINUE_ON_ERROR_
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
                   RelExpr *queryExpression,
                   CollHeap *oHeap = CmpCommon::statementHeap())
   : ExeUtilExpr(HBASE_LOAD_, hBaseTableName, exprNode, NULL,
                 stmtText, stmtTextCharSet, oHeap),
    //preLoadCleanup_(FALSE),
    keepHFiles_(FALSE),
    truncateTable_(FALSE),
    updateStats_(FALSE),
    noRollback_(FALSE),
    continueOnError_(FALSE),
    logErrorRows_(FALSE),
    noDuplicates_(TRUE),
    rebuildIndexes_(FALSE),
    constraints_(FALSE),
    noOutput_(FALSE),
    indexTableOnly_(FALSE),
    hasUniqueIndexes_(FALSE),
    upsertUsingLoad_(FALSE),
    pQueryExpression_(queryExpression),
    maxErrorRows_(0)
  {
  };

  virtual const NAString getText() const;

  virtual RelExpr * copyTopNode(RelExpr *derivedNode = NULL,
                                CollHeap* outHeap = 0);

  virtual RelExpr * bindNode(BindWA *bindWAPtr);

  virtual short codeGen(Generator*);

  NABoolean getKeepHFiles() const
  {
    return keepHFiles_;
  }

  void setKeepHFiles(NABoolean keepHFiles)
  {
    keepHFiles_ = keepHFiles;
  }
  NABoolean getContinueOnError() const
  {
    return continueOnError_;
  }

  void setContinueOnError(NABoolean v)
  {
    continueOnError_ = v;
  }

  NABoolean getLogErrorRows() const
  {
    return logErrorRows_;
  }

  void setLogErrorRows(NABoolean v)
  {
    logErrorRows_ = v;
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

  NABoolean getRebuildIndexes() const
  {
   return rebuildIndexes_;
  }

  void setRebuildIndexes(NABoolean indexes)
  {
   rebuildIndexes_ = indexes;
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

  NABoolean getHasUniqueIndexes() const {
   return hasUniqueIndexes_;
 }

 void setHasUniqueIndexes(NABoolean uniqIndex) {
   hasUniqueIndexes_ = uniqIndex;
 }

  NABoolean getUpsertUsingLoad() const
  {
   return upsertUsingLoad_;
  }

 void setUpsertUsingLoad(NABoolean upsertUsingLoad)
 {
   upsertUsingLoad_ = upsertUsingLoad;
 }

 NABoolean getUpdateStats() const
 {
   return updateStats_;
 }

 void setUpdateStats(NABoolean updateStats)
 {
   updateStats_ = updateStats;
 }
 void setMaxErrorRows( UInt32 v)
 {
   maxErrorRows_ = v;
 }
 UInt32 getMaxErrorRows( )
  {
    return maxErrorRows_ ;
  }
 void setLogErrorRowsLocation (char * str)
 {
   logErrorRowsLocation_ = str;
 }

  virtual NABoolean isExeUtilQueryType() { return TRUE; }
  virtual NABoolean producesOutput() { return (noOutput_ ? FALSE : TRUE); }

  RelExpr *getQueryExpression() { return pQueryExpression_; }

  short setOptions(NAList<ExeUtilHBaseBulkLoad::HBaseBulkLoadOption*> *
      hBaseBulkLoadOptionList,
      ComDiagsArea * da);
private:

  //NABoolean preLoadCleanup_;
  NABoolean keepHFiles_;
  NABoolean truncateTable_;
  NABoolean updateStats_;
  NABoolean noRollback_;
  NABoolean continueOnError_;
  NABoolean logErrorRows_;
  NABoolean noDuplicates_;
  NABoolean rebuildIndexes_;
  NABoolean constraints_;
  NABoolean noOutput_;
  //target table is index table
  NABoolean indexTableOnly_;
  // target table has unique indexes
  NABoolean hasUniqueIndexes_;
  NABoolean upsertUsingLoad_;
  RelExpr *pQueryExpression_;
  UInt32     maxErrorRows_;
  NAString  logErrorRowsLocation_;

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

  virtual RelExpr * bindNode(BindWA *bindWA);

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

  enum ScanType
  {
    REGULAR_SCAN_ = 0,
    SNAPSHOT_SCAN_CREATE_ = 1,
    SNAPSHOT_SCAN_EXISTING_ = 2
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
      overwriteMergeFile_(FALSE),
      scanType_(REGULAR_SCAN_),
      snapSuffix_(oHeap)

    {
    };
  ExeUtilHBaseBulkUnLoad(const CorrName &hBaseTableName,
                   ExprNode * exprNode,
                   char * stmtText,
                   NAString * extractLocation,
                   CharInfo::CharSet stmtTextCharSet,
                   RelExpr *queryExpression,
                   CollHeap *oHeap = CmpCommon::statementHeap())
   : ExeUtilExpr(HBASE_UNLOAD_, hBaseTableName, exprNode, NULL,
                 stmtText, stmtTextCharSet, oHeap),
    emptyTarget_(FALSE),
    logErrors_(FALSE),
    noOutput_(FALSE),
    oneFile_(FALSE),
    compressType_(NONE_),
    extractLocation_(*extractLocation, oHeap),
    overwriteMergeFile_(FALSE),
    scanType_(REGULAR_SCAN_),
    snapSuffix_(oHeap),
    pQueryExpression_(queryExpression)
  {
  };

  virtual const NAString getText() const;

  virtual RelExpr * copyTopNode(RelExpr *derivedNode = NULL,
                                CollHeap* outHeap = 0);
  virtual RelExpr * bindNode(BindWA *bindWAPtr);

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

  RelExpr *getQueryExpression() { return pQueryExpression_; }

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
  NAString snapSuffix_;
  ScanType scanType_;
  RelExpr *pQueryExpression_;
};


#endif /* RELEXEUTIL_H */
