/***********************************************************************
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
#ifndef RELMISC_H
#define RELMISC_H
/* -*-C++-*-
******************************************************************************
*
* File:         RelMisc.h
* Description:  Miscellaneous relational operators
*               (both physical and logical operators)
* Created:      4/28/94
* Language:     C++
*
*
*
*
******************************************************************************
*/

//#include "exp_clause_derived.h"
#include "ObjectNames.h"
#include "RelExpr.h"
#include "SQLCLIdev.h"
#include "OptUtilIncludes.h"
#include "BinderUtils.h"
#include "StmtNode.h"
#include "LateBindInfo.h"
#include "SequenceGeneratorAttributes.h"
#include "ComSecurityKey.h"
#include "CmpStatement.h"

class TrafDesc;

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class RelRoot;
class Tuple;
class TupleList;
class Filter;
class Rename;
class RenameTable;
class RenameReference;
class BeforeTrigger;
class Refresh;
class MapValueIds;
class Pack;
class PhyPack;
class FirstN;
class ExtractSource;
class SequenceGenerator;
class Rowset;
class RowsetRowwise;
class RowsetInto;
class RowsetFor;
class ControlRunningQuery;
class CommonSubExprRef;
class Union;


  // The following are physical operators
  // class Tuple is both a logical and a physical operator!!!


// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
class LogicalProperty;
class BindWA;
class Generator;
class SearchKey;
class RaiseError;
class ItemExprList;
class UpdateColumns;
class QuerySimilarityInfo;
class HostArraysWA;
class DeltaDefinition;
class PipelineDef;
class IncrementalRefreshOption;
class DeltaDefinitionPtrList;
class NRowsClause;
class PipelineClause;
class MVInfo;
class MvRefreshBuilder;
class MvBindContext;
class RangePartitioningFunction;
class ElemDDLNode;
class ElemProxyColDef;
class ItemExprList;
class RtmdCompileTimeObj;
class HbaseAccessOptions;
class Union;

// TreeStore struct
struct TreeStore : public NABasicObject
{
public:
	RelExpr * rep; //rel expr ptr
	Int32 cindex; //compressed index
	//constructor
	TreeStore(RelExpr * r, Int32 c){rep=r; cindex=c;}
};

// -----------------------------------------------------------------------
// A RelRoot operator returns a computed expression for each of its
// child rows. A typical application for the RelRoot node is the SELECT
// list of an SQL query, like SELECT a+1, b+c, d from t; A RelRoot node
// also keeps a list of input variables. Both computed expressions and
// input variables are lists rather than sets.
// -----------------------------------------------------------------------

class RelRoot : public RelExpr
{
public:

  // constructor
  RelRoot(RelExpr *child,
	  OperatorTypeEnum otype = REL_ROOT,
	  ItemExpr *compExpr = NULL,
	  ItemExpr *orderBy = NULL,
	  ItemExpr *updateCol = NULL,
	  RelExpr *reqdShape = NULL,
	  CollHeap *oHeap = CmpCommon::statementHeap());

  RelRoot(RelExpr *child,
	  TransMode::AccessType at,
	  LockMode lm,
	  OperatorTypeEnum otype = REL_ROOT,
	  ItemExpr *compExpr = NULL,
	  ItemExpr *orderBy = NULL,
	  ItemExpr *updateCol = NULL,
	  RelExpr *reqdShape = NULL,
	  CollHeap *oHeap = CmpCommon::statementHeap());

  RelRoot(const RelRoot & other);

  // virtual destructor
  virtual ~RelRoot();

  // get the degree of this node (it is a unary op).
  virtual Int32 getArity() const;

  void setFirstNRowsParam(ItemExpr *firstNRowsParam)   
  { firstNRowsParam_ = firstNRowsParam; }
  ItemExpr * getFirstNRowsParam() 			{ return firstNRowsParam_; }

  // access (get and set) the count of output variables for this (dynamic) query
  Int32 &outputVarCnt()			{ return outputVarCnt_; }
  NABoolean outputVarCntValid() const	{ return outputVarCnt_ >= 0; }

  // get and set the input variables as a parse tree
  void addInputVarTree(ItemExpr *inputVar);
  ItemExpr * removeInputVarTree();

  // add input variable to the top of tree. As of R1.5 called only for
  // ODBC initiated dynamic rowset queries.
  void addAtTopOfInputVarTree(ItemExpr *inputVar);

  // get and set the output variables as a parse tree
  ItemExpr *& outputVarTree() { return outputVarTree_; }
  void addOutputVarTree(ItemExpr *outputVar);
  ItemExpr * removeOutputVarTree();

  ItemExpr *& assignmentStTree() {return assignmentStTree_; }
  void addAssignmentStTree(ItemExpr *inputOutputVar);
  ItemExpr * removeAssignmentStTree();
  ItemExpr *& assignList() { return assignList_; }

  // get and set the compute predicate as a parse tree
  ItemExpr * getCompExprTree() const	{ return compExprTree_; }
  void addCompExprTree(ItemExpr *compExpr);
  ItemExpr * removeCompExprTree();

  // get and set the order by list as a parse tree
  void addOrderByTree(ItemExpr *orderBy);
  ItemExpr * removeOrderByTree();
  NABoolean hasOrderBy()	{ return orderByTree_ || reqdOrder_.entries(); }
  ItemExpr * getOrderByTree() const { return orderByTree_; }

  // get and set the where predicate as a parse tree
  ItemExpr * getPredExprTree() const	{ return predExprTree_; }
  void addPredExprTree(ItemExpr *predExpr);
  ItemExpr * removePredExprTree();

  // partitioning type, TMUDF only
  TMUDFInputPartReq getPartReqType() {return partReqType_;}
  void setPartReqType(TMUDFInputPartReq val) {partReqType_= val;}

  // get and set the partition By set as a parse tree, TMUDF only
  void addPartitionByTree(ItemExpr *partBy);
  ItemExpr * removePartitionByTree();
  NABoolean hasPartitionBy()	{ return partitionByTree_ || partArrangement_.entries() ; }
  ItemExpr * getPartitionByTree() const { return partitionByTree_; }

  // get and set the update column list as a parse tree
  void addUpdateColTree(ItemExpr *updateCol);
  ItemExpr * removeUpdateColTree();

  // return a (short-lived) read/write reference to the input/output lists
  ValueIdList & compExpr() 		{ return compExpr_; }
  const ValueIdList & compExpr() const 	{ return compExpr_; }
  void setCompExpr(const ValueIdList& c)	{ compExpr_ = c; }

  ValueIdList & assignmentStVars() { return assignmentStVars_; }
  void setAssignmentStVars(const ValueIdList &c) {assignmentStVars_ = c; }
  ValueIdList & inputVars() { return inputVars_; }
  const ValueIdList & inputVars() const { return inputVars_; }
  ValueIdList & outputVars() { return outputVars_; }
  const ValueIdList & outputVars() const { return outputVars_; }

  ValueIdList & reqdOrder() 		{ return reqdOrder_; }
  const ValueIdList & reqdOrder() const { return reqdOrder_; }

  // TMUDF only
  ValueIdSet & partitionArrangement() 		{ return partArrangement_; }
  const ValueIdSet & partitionArrangement() const { return partArrangement_; }
  ValueIdList & updateCol() 		{ return updateCol_; }
  const ValueIdList & updateCol() const { return updateCol_; }

  void setReqdShape(RelExpr *shape) 	{ reqdShape_ = shape; }
  RelExpr *getReqdShape() const 	{ return reqdShape_; }
  ValueIdList &pkeyList() 		{ return pkeyList_; }
  ItemExpr *&currOfCursorName()		{ return currOfCursorName_; }
  NABoolean &updatableSelect() 		{ return updatableSelect_; }
  NABoolean &updateCurrentOf() 		{ return updateCurrentOf_; }
  NABoolean &rollbackOnError()		{ return rollbackOnError_; }

  void processRownum(BindWA * bindWA);

  // a virtual function for performing name binding within the query tree
  virtual RelExpr * bindNode(BindWA *bindWAPtr);

  // bind time method that rewrites subqueries with [first n] + ORDER BY 
  // into OLAP subqueries
  RelExpr * rewriteFirstNOrderBySubquery(BindWA *bindWAPtr);

  // a virtual function to get addressability to the list of output values
  virtual ItemExpr * selectList();

  // Each operator supports a (virtual) method for transforming its
  // scalar expressions to a canonical form
  virtual void transformNode(NormWA & normWARef,
			     ExprGroupId & locationOfPointerToMe);

  // a method used during subquery transformation for pulling up predicates
  // towards the root of the transformed subquery tree
  virtual void pullUpPreds();

  // a method used for recomputing the outer references (external dataflow
  // input values) that are still referenced by each operator in the
  // subquery tree after the preidcate pull up is complete.
  virtual void recomputeOuterReferences();

  // Each operator supports a (virtual) method for rewriting its
  // value expressions.
  virtual void rewriteNode(NormWA & normWARef);

  // Each operator supports a (virtual) method for performing
  // predicate pushdown and computing a "minimal" set of
  // characteristic input and characteristic output values.
  virtual RelExpr * normalizeNode(NormWA & normWARef);

  // subqueries are unnested in this method
  virtual RelExpr * semanticQueryOptimizeNode(NormWA & normWARef);
  RelExpr * inlineTempTablesForCSEs(NormWA & normWARef);

  // Method to push down predicates from a RelRoot node into the
  // children
  virtual
  void pushdownCoveredExpr(const ValueIdSet & outputExprOnOperator,
                           const ValueIdSet & newExternalInputs,
                           ValueIdSet& predOnOperator,
			   const ValueIdSet * nonPredExprOnOperator = NULL,
                           Lng32 childId = (-MAX_REL_ARITY) );

  // The set of values that I can potentially produce as output.
  virtual void getPotentialOutputValues(ValueIdSet & vs) const;

  void setRootFlag(NABoolean trueRootFlag) 	{ trueRoot_ = trueRootFlag; }
  NABoolean isTrueRoot() const 			{ return trueRoot_; }

  void setSubRoot(NABoolean subRoot) 	{ subRoot_ = subRoot; }
  NABoolean isSubRoot() const 			{ return subRoot_; }

  virtual HashValue topHash();
  virtual NABoolean duplicateMatch(const RelExpr & other) const;
  virtual RelExpr * copyTopNode(RelExpr *derivedNode = NULL,
				CollHeap* outHeap = 0);

  // synthesize logical properties
  virtual void synthLogProp(NormWA * normWAPtr = NULL);
  virtual void synthEstLogProp(const EstLogPropSharedPtr& inputEstLogProp);

  // optimizer functions
  virtual CostMethod* costMethod() const;
  virtual Context* createContextForAChild(Context* myContext,
                     PlanWorkSpace* pws,
                     Lng32& childIndex);

  virtual NABoolean currentPlanIsAcceptable(Lng32 planNo,
            const ReqdPhysicalProperty* const rppForMe) const;

  virtual PhysicalProperty *synthPhysicalProperty(const Context *context,
                                                  const Lng32     planNumber,
                                                  PlanWorkSpace  *pws);

  // method to do code generation
  RelExpr * preCodeGen(Generator * generator,
		       const ValueIdSet & externalInputs,
		       ValueIdSet &pulledNewInputs);
  virtual short codeGen(Generator*);

  // add all the expressions that are local to this
  // node to an existing list of expressions (used by GUI tool)
  virtual void addLocalExpr(LIST(ExprNode *) &xlist,
			    LIST(NAString) &llist) const;

  virtual void computeMyRequiredResources(RequiredResources & reqResources,
                                      EstLogPropSharedPtr & inLP);

  // get a printable string that identifies the operator
  virtual const NAString getText() const;

  // set display on/off.
  void            setDisplayTree(NABoolean val) {displayTree_ = val;}
  NABoolean       getDisplayTree() const 	{return displayTree_;}
  void            setExeDisplay(NABoolean val)  {exeDisplay_ = val;}
  NABoolean       getExeDisplay() const 	{return exeDisplay_;}

  ExplainTuple *addSpecificExplainInfo(ExplainTupleMaster *explainTuple,
					      ComTdb * tdb,
					      Generator *generator);


  StmtLevelAccessOptions& accessOptions() 	{ return accessOptions_; }
  NABoolean &readOnlyTransIsOK()		{ return readOnlyTransIsOK_; }

  NABoolean isUpdatableCursor();
  NABoolean isUpdatableView(NABoolean &isInsertable) const;

  LIST(OptSqlTableOpenInfo *) &getViewStoiList() 	{ return viewStoiList_; }
  LIST(OptSqlTableOpenInfo *) &getDDLStoiList() 	{ return ddlStoiList_; }
  LIST(OptUdrOpenInfo *) &getUdrStoiList() 	{ return stoiUdrList_; }
  LIST(OptUDFInfo *) &getUDFList() 	        { return udfList_; }
  ComSecurityKeySet  &getSecurityKeySet()       { return securityKeySet_; }

  NABoolean &needFirstSortedRows() 		{ return needFirstSortedRows_; }

  //++ Privs
  NABoolean checkPrivileges(BindWA* bindWA);
  void findKeyAndInsertInOutputList( ComSecurityKeySet KeysForTab
                                   , const uint32_t userHashValue
                                   , const PrivType which
                                   , BindWA* bindWA );

  //++ MVs
  NABoolean hasMvBindContext() const;
  MvBindContext * getMvBindContext() const;
  void setMvBindContext(MvBindContext * pMvBindContext);

  // -- MVs
  void setRootOfInternalRefresh() { isRootOfInternalRefresh_ = TRUE; }
  NABoolean isRootOfInternalRefresh() const { return isRootOfInternalRefresh_; }
  void setMVQRqueryNonCacheable() { isQueryNonCacheable_ = TRUE; }
  NABoolean isMVQRqueryNonCacheable() const { return isQueryNonCacheable_;}

  // -- Triggers
  void setEmptySelectList() { isEmptySelectList_ = TRUE; }
  NABoolean isEmptySelectList() const { return isEmptySelectList_; }

  void setDontOpenNewScope() { isDontOpenNewScope_ = TRUE; }
  NABoolean isDontOpenNewScope() const { return isDontOpenNewScope_; }

  //  NABoolean &doOltpQueryOptimization() 	{ return doOltpQueryOptimization_; }

  OperatorType &childOperType() 		{ return childOperType_;};
  ItemExpr *getInputVarTree() 			{ return inputVarTree_; }
  ItemExpr *getOutputVarTree() 			{ return outputVarTree_; }

  inline LIST(ComTimestamp) * getTriggersList() const	{ return triggersList_; }

  // QSTUFF
  // --------------------------------------------------------------------
  // This routine checks whether a table is both read and updated
  // --------------------------------------------------------------------
  virtual rwErrorStatus checkReadWriteConflicts(NormWA & normWARef);

  inline NABoolean avoidHalloween() const 
  { return avoidHalloween_; }

  inline void setAvoidHalloween(NABoolean h)
  { avoidHalloween_ = h; }

  inline NABoolean disableESPParallelism() const 
  { return disableESPParallelism_; }

  inline void setDisableESPParallelism(NABoolean e)
  { disableESPParallelism_ = e; }

  HostArraysWA *getHostArraysArea() { return hostArraysArea_; }
  void setHostArraysArea(HostArraysWA *ha) { hostArraysArea_ = ha; }

  // apply xforRowsetsInTree to RelRoot & return transformed tree
  RelExpr *xformRowsetsInTree(BindWA& wa,
					const ULng32 inputArrayMaxsize = 0,
					const RelExpr::AtomicityType atomicity = RelExpr::UNSPECIFIED_);

  // append an ascii-version of RelRoot into cachewa.qryText_
  virtual void generateCacheKey(CacheWA& cwa) const;

  // is this entire expression cacheable after this phase?
  virtual NABoolean isCacheableExpr(CacheWA& cwa);

  // change literals of a cacheable query into ConstantParameters and
  // save true root into cachewa so we can add ConstantParameters as inputs
  virtual RelExpr* normalizeForCache(CacheWA& cwa, BindWA& bindWA);

  // For defect id: 10-010522-2978

  RelRoot     *getParentForRowsetReqdOrder()
                 { return parentForRowsetReqdOrder_; };
  void         setParentForRowsetReqdOrder(RelRoot * pRR)
                 { parentForRowsetReqdOrder_ = pRR; };

  // End defect id: 10-010522-2978
 
  inline const ItemExprList *getSpOutParams (void)
 
  {
      return spOutParams_;
  }

  NABoolean forceCQS(RelExpr *);

  RelRoot * transformGroupByWithOrdinalPhase1(BindWA *bindWA);
  RelRoot * transformGroupByWithOrdinalPhase2(BindWA *bindWA);
  RelRoot * transformOrderByWithExpr(BindWA *bindWA);
  ItemExpr * processGroupingID(ItemExpr * ie, BindWA *bindWA);

  // MV --
  NABoolean virtual isIncrementalMV() { return getFirstNRows()==-1 && !needFirstSortedRows(); }

  NABoolean downrevCompileNeeded() const
  {
    return (downrevCompileMXV_ != COM_VERS_CURR_PLAN);
  }

  void setDownrevCompileMXV(COM_VERSION mxv)
  { downrevCompileMXV_ = mxv;}
  COM_VERSION getDownrevCompileMXV() { return downrevCompileMXV_;}

  // For parallel extract producer queries
  NABoolean isParallelExtractProducer() const
  { return (numExtractStreams_ > 0 ? TRUE : FALSE); }
  void setNumExtractStreams(ComUInt32 n)
  { numExtractStreams_ = n; }
  ComUInt32 getNumExtractStreams() const
  { return numExtractStreams_; }

  inline NABoolean containsOnStatementMV() const
  { return containsOnStatementMV_; }

  inline void setContainsOnStatementMV(NABoolean h)
  { containsOnStatementMV_ = h; }

  inline NABoolean containsLRU() const
  { return containsLRU_; }

  inline void setContainsLRU(NABoolean h)
  { containsLRU_ = h; }

  // MVQR
  inline NABoolean isAnalyzeOnly()
  { return isAnalyzeOnly_; }
  
  inline void setAnalyzeOnly()
  { isAnalyzeOnly_ = true; }

  inline NABoolean hasMandatoryXP() const
  { return hasMandatoryXP_; }

  inline void setHasMandatoryXP(NABoolean h)
  { hasMandatoryXP_ = h; }

  // olap functions support
  
  void setHasOlapFunctions(NABoolean v) { hasOlapFunctions_ = v; }
  NABoolean getHasOlapFunctions()  { return hasOlapFunctions_; }

  void setHasTDFunctions(NABoolean v) { hasTDFunctions_ = v; }
  NABoolean getHasTDFunctions()  { return hasTDFunctions_; }

  private:

  enum
  {
    HDFS_ACCESS = 0x0001
  };

  void transformTDPartitionOrdinals(BindWA *bindWA);
  void resolveAggregates(BindWA *bindWA);
  void resolveSequenceFunctions(BindWA *bindWA);

  NABoolean isUpdatableBasic(NABoolean isView, NABoolean &isInsertable) const;

  NABoolean checkFirstNRowsNotAllowed(BindWA* bindWA) ;

  // defined in generator/GenRelMisc.cpp
  TrafQuerySimilarityInfo * genSimilarityInfo(Generator *generator);

  // returns TRUE if a GroupByAgg node was added
  NABoolean addOneRowAggregates(BindWA * bindWA, NABoolean forceGroupByAgg);

  inline void setNumBMOs(unsigned short num) { numBMOs_ = num; }
  inline unsigned short getNumBMOs() { return numBMOs_; }

  // set and get the total BMO memory usage of the fragment rooted
  // at the root node
  inline void setBMOsMemoryUsage(CostScalar x) { BMOsMemoryUsage_ = x; }
  inline CostScalar getBMOsMemoryUsage() { return BMOsMemoryUsage_ ; }

  inline void setNBMOsMemoryUsage(CostScalar x) { nBMOsMemoryUsage_ = x; }
  inline CostScalar getNBMOsMemoryUsage() { return nBMOsMemoryUsage_ ; }

  NABoolean hdfsAccess()
    { return (flags_ & HDFS_ACCESS) != 0; }
  void setHdfsAccess(NABoolean v)
    { (v ? flags_ |= HDFS_ACCESS : flags_ &= ~HDFS_ACCESS); }	

  NABoolean   trueRoot_;    	// set in the true root of the query tree
                            	// reset in the roots of all subquery trees
  Int32	      outputVarCnt_;	// -1 if not true root, else no. of output :hv's
  NABoolean   subRoot_;	// true iff this is a subroot (ie, the root of a
  // sql statement that's inside a compound statement)
  ItemExpr    * compExprTree_;
  ItemExpr    * inputVarTree_;
  ItemExpr    * outputVarTree_;
  ItemExpr    * orderByTree_;
  TMUDFInputPartReq partReqType_;      //NONE, REPLICATE, ANY, SPECIFIED
  ItemExpr    * partitionByTree_;  // used by TMUDF only. If this member is set 
                                   // orderBy is for each partition (OLAP style)
  ItemExpr    * updateColTree_;
  ItemExpr    * assignmentStTree_; // Variables in left-hand side of assignment
                                   // statement in Compound Statements
  ItemExpr    * assignList_;       // List of assign nodes. Used by assignment statement;
                                   // contains first and last value id of host variables
                                   // in the statement. Used at code generation time
  // predicate to be evaluated before returning a row. If specified, rows are returned
  // only if this pred passes
  ItemExpr    * predExprTree_;

  ValueIdList compExpr_;    	// select list
  ValueIdList inputVars_;   	// list of input host variables or parameters
  ValueIdList reqdOrder_;   	// ORDER BY list
  ValueIdSet partArrangement_;  // PARTITION BY set for TMUDF
  ValueIdList updateCol_;   	// update column list
  RelExpr     * reqdShape_; 	// forced plan shape of this query
  ValueIdList outputVars_;   	// list of output host variables or parameters
  ValueIdList assignmentStVars_;// Bound variables in left-hand side of
                                // assignment statement in Compund Statements
  ItemExprList *spOutParams_;// List of HVs/DPs in CALL OUT/INOUT parameter list

  // list of all the views open information of this query.
  LIST(OptSqlTableOpenInfo *) viewStoiList_;

  // list of all the table open information of this DDL stmt.
  // used by Trigger and MV
  LIST(OptSqlTableOpenInfo *) ddlStoiList_;

  // list of all routines open in the context of this query.
  LIST(OptUdrOpenInfo *) stoiUdrList_;

  // list of all UDFs open in the context of this query.
  // This list applies only to TRIGGERS.
  LIST(OptUDFInfo *) udfList_;

  // list of ComSecurityKeys used by this query
  // If a privilege is obtained by more than one means, compiler chooses a path
  ComSecurityKeySet  securityKeySet_;

  ////////////////////////////////////////////////////////////////////
  // The fields updatableSelect_, updateCurrentOf_ and pkeyList_ are
  // used to process an 'update where current of query'.
  // Only ONE of the two flags (updatableSelect_ and updateCurrentOf_)
  // could be TRUE.
  ////////////////////////////////////////////////////////////////////

  // this flag indicates that the query below this root node is
  // a 'simple' SELECT cursor declared via "DECLARE CURSOR..." statement.
  // and the base table could be updated/deleted via an
  // "UPDATE...WHERE CURRENT OF..." statement.
  // A select is not 'simple' if there are Aggrs, multiple tables,
  // subqueries in a select statement.
  NABoolean updatableSelect_;

  // this flag, if set, indicates that this is a static update where current
  // of query.
  NABoolean updateCurrentOf_;

  // The next list contains one of the following two lists:
  //  -- if updatableSelect_ is TRUE, then this contains the list of
  //     child SELECT's primary key columns. Used to create a 'row' of
  //     primary keys which are passed to the 'update' query at runtime.
  //
  //  -- if updateCurrentOf_ is TRUE, this list contains the list of
  //     fabricated hostvars. See class GenericUpdate, field pkeyHvarList_.
  ValueIdList pkeyList_;

  // Name of the cursor or a hostvar which will contain the cursor
  // name at runtime. Valid if updateCurrentOf_ is TRUE. Initialized
  // at bind time from the child update/delete node.
  ItemExpr * currOfCursorName_;

  NABoolean  displayTree_; // if set, this tree needs to be displayed.
                           // Set by parser on seeing a DISPLAY command.
  NABoolean  exeDisplay_;  // if set, display query execution in the GUI

  // this flag is set to TRUE if this is an update, delete or insert
  // query. This information is needed at runtime to rollback/abort
  // the transaction in case of an error. What we really need is
  // support for statement atomicity. But until we have that, the
  // only way to have a consistent database in case of an error is
  // to rollback the transaction. This makes us ANSI incompatible.
  NABoolean rollbackOnError_;

  // user specified BROWSE, STABLE or REPEATABLE access type and
  // user specified SHARE or EXCLUSIVE mode
  StmtLevelAccessOptions accessOptions_;
  NABoolean readOnlyTransIsOK_;

  // if TRUE, return first N sorted rows.
  NABoolean needFirstSortedRows_;

  // Tells how many scalar variables this node contains
  Int32 numSimpleVar_;

  // Tells how many host array variables this node contains
  Int32 numHostVar_;

  // if set to TRUE, then message buffers between FS and DP2 could
  // be small. This is set by binder if the true root of query tree
  // is an update, delete, insert-values or select-into query.
  // This is still a hint. The actual buffer size is computed by
  // the generator depending on whether a unique operation is chosen.
  //NABoolean doOltpQueryOptimization_;

  OperatorType childOperType_;

  // points to a class used by RowSets code
  HostArraysWA *hostArraysArea_;

  // -- MVs
  // Is this true root above a Refresh node (before binding).
  // This flag is propagated to the physical node by the optimizer,
  // and later used at generation time.
  NABoolean isRootOfInternalRefresh_;

  // true iff result descriptor has Provided range or residual predicate(s)
  NABoolean isQueryNonCacheable_; // default is FALSE

  //++ MV OZ
  // this context will be passed to the new scope created by the root
  MvBindContext * pMvBindContextForScope_;

  ULng32 flags_;

  // For defect id: 10-010522-2978
  // Points to ORDER BY required to properly support a lower level RelRoot
  // which has an ORDER by and an input host variable array (i.e. Rowset used
  // for input).  The idea is to always force the ordering to be for
  // the entire output from the upper level RelRoot rather than just the
  // output from each of the elements of the input host variable array(s).
  // This is used in the HostArraysWA::modifyTree() method.  It puts the
  // bound identifiers for the ORDER BY into the upper level RelRoot, but
  // avoids prematurely populating the reqdOrder_ member of that upper level
  // RelRoot; in other words, this was added to avoid putting bound
  // identifiers into reqdOrder_ until after it is bound.
  // rowsetReqdOrder_ is populated when the lower level RelRoot
  // is executing RelRoot::bindNode().  These bound variables will be
  // saved here until we reach the upper level RelRoot::bindNode() and
  // the contents are merged into the upper level RelRoot's reqdOrder_.
  // NOTE: This code assumes that there are no modifications to the
  //       RelExpr tree which would move between the parent RelRoot node for
  //       and the RelExprs created below it during HostArraysWA::modifyTree().
  //       Should such modifications be needed, this may need to change to
  //       accomodate the tree structure.
  ValueIdList  rowsetReqdOrder_;

  // This pointer to the parent level RelRoot is initialized to NULL by
  // the constructors and potentially set by the code in
  // HostArraysWA::modifyTree().  It is checked for non-NULL and used during
  // RelRoot::bindNode() to allow the lower level RelRoot to quickly
  // populate the parent level's rowsetReqdOrder_.  This could be
  // changed to be a flag and then have code to search upwards in the
  // tree to find the correct upper level parent if the tree could get
  // rewritten to have another intervening RelRoot which should then
  // become the new target of the parentForRowsetReqdOrder_. (In other words,
  // we would need to find correct upper level parent algorithmically).
  RelRoot     *parentForRowsetReqdOrder_;

  // End defect id: 10-010522-2978

  // -- Triggers
  // This root has no data flowing up.
  NABoolean isEmptySelectList_;
  // Dont open a new BindScope when binding this root.
  NABoolean isDontOpenNewScope_;

  // List of trigger ID's in this RelExpr tree
  LIST(ComTimestamp) *triggersList_;

  // downrevCompileMXV_ contains the version of code which needs to be
  // generated. It is the min MXV of all partitions which we can access.
  // It is set in RelRoot::bindNode.
  // For coyote, we need to generate either roadrunner or R2 fcs code,
  // depending on the min MXV.
  // This will be removed when real versioning support is in.
  COM_VERSION downrevCompileMXV_;

  UninitializedMvNameList *uninitializedMvList_;

  // Support self-referencing updates and avoid the Halloween problem.
  NABoolean avoidHalloween_;

  // Disable ESP parallel execution for certain kind of queries.
  // Right now being done for Merge statement.
  // Merge is only done on unique predicates.
  // Right now, even with unique predicate, the merge request is sent
  // to each ESP. 
  // That incorrectly inserts rows if row to be updated
  // is not found.
  // This may be temporary if code is fixed so merge request is only
  // sent to the ESP based on the unique merge predicate.
  NABoolean disableESPParallelism_;

  // For a parallel extract producer query, the number of extract
  // streams
  ComUInt32 numExtractStreams_;

  // TRUE, if in a grouped query, all columns specified in the
  // order by clause are also specified in the group by clause.
  // Valid for queries with GROUP BY and ORDER BY clauses.
  NABoolean allOrderByRefsInGby_;
  
  // olap functions support
  NABoolean hasOlapFunctions_;
  NABoolean hasTDFunctions_;
 

  // TRUE, if the IUD statement contains an ON STATMENT MV
  NABoolean containsOnStatementMV_;

  // TRUE, if the IUD statement is an LRU
  NABoolean containsLRU_;

  // MVQR
  // TRUE if the query result should be the query descriptor used for MV Query Reqrite.
  NABoolean isAnalyzeOnly_;

  // TRUE, if query has mandatory cross products
  NABoolean hasMandatoryXP_;

  // Updated and used only in the generator.  Counts BMOs in master fragment.
  unsigned short numBMOs_;

  // Updated and used only in the generator.  Total BMO memory usage in master fragment.
  CostScalar BMOsMemoryUsage_ ;
  CostScalar nBMOsMemoryUsage_ ;

  // flag used to indicate whether CIF is on or off in the explain plan only
  NABoolean isCIFOn_;

  ItemExpr * firstNRowsParam_;
}; // class RelRoot

// -----------------------------------------------------------------------
// The PhysicalRelRoot replaces the logical RelRoot through the
// application of the PhysicalRelRootRule. This transformation is
// designed to present a purely physical verison of an operator
// that is both logical and physical.
// -----------------------------------------------------------------------
class PhysicalRelRoot : public RelRoot
{
public:
  PhysicalRelRoot(RelExpr * childExpr,
		  CollHeap *oHeap = CmpCommon::statementHeap())
    : RelRoot(childExpr, REL_ROOT, NULL, NULL, NULL, NULL, oHeap)   {};
  PhysicalRelRoot(const RelRoot & other) : RelRoot(other)     {};

  virtual NABoolean isLogical() const;
  virtual NABoolean isPhysical() const;

}; // class PhysicalRelRoot

// -----------------------------------------------------------------------
// The Tuple class represents a table with a single row (tuple) in it.
// This is both a logical and a physical operator.
// -----------------------------------------------------------------------
class Tuple : public RelExpr
{
public:

  // constructor
  Tuple(OperatorTypeEnum oper, ItemExpr *tupleExpr = NULL,
	CollHeap *oHeap = CmpCommon::statementHeap())
    : RelExpr(oper,NULL,NULL,oHeap),
      tupleExprTree_(tupleExpr),
	  rejectPredicates_(FALSE)
  {
    id_ = ++idCounter_;
  }

  Tuple(ItemExpr *tupleExpr = NULL,
	CollHeap *oHeap = CmpCommon::statementHeap())
    : RelExpr(REL_TUPLE,NULL,NULL,oHeap),
      tupleExprTree_(tupleExpr),
  	  rejectPredicates_(FALSE)
  {
    id_ = ++idCounter_;
  }

  Tuple(const Tuple & other);

  virtual ~Tuple();

  // get the degree of this node (it is a leaf op).
  virtual Int32 getArity() const;

  // get and set the tuple predicate, add and remove individual predicates
  // from the list of expressions
  ItemExpr * tupleExprTree() const	{ return tupleExprTree_; }
  void addTupleExprTree(ItemExpr *tupleExpr);
  ItemExpr * removeTupleExprTree();

  // return a (short-lived) read/write reference to the tuple expression
  ValueIdList & tupleExpr() { return tupleExpr_; }

  // return a read-only reference to the tuple expression
  const ValueIdList & tupleExpr() const { return tupleExpr_; }

  // a virtual function for performing name binding within the query tree
  RelExpr * bindNode(BindWA *bindWAPtr);

  // MV --
  NABoolean virtual isIncrementalMV() { return TRUE; }

  // Each operator supports a (virtual) method for transforming its
  // scalar expressions to a canonical form
  //
  virtual void transformNode(NormWA & normWARef,
                             ExprGroupId & locationOfPointerToMe);

  // a method used for recomputing the outer references (external dataflow
  // input values) that are still referenced by each operator in the
  // subquery tree after the preidcate pull up is complete.
  virtual void recomputeOuterReferences();

  // Each operator supports a (virtual) method for rewriting its
  // value expressions.
  virtual void rewriteNode(NormWA & normWARef);

  // Each operator supports a (virtual) method for performing
  // predicate pushdown and computing a "minimal" set of
  // characteristic input and characteristic output values.
  virtual RelExpr * normalizeNode(NormWA & normWARef);

  // method to do code generation
  virtual RelExpr * preCodeGen(Generator * generator,
		                       const ValueIdSet & externalInputs,
		                       ValueIdSet &pulledNewInputs);
  virtual short codeGen(Generator*);

  virtual void getPotentialOutputValues(ValueIdSet & vs) const;

  // add all the expressions that are local to this
  // node to an existing list of expressions (used by GUI tool)
  virtual void addLocalExpr(LIST(ExprNode *) &xlist,
			    LIST(NAString) &llist) const;

  virtual HashValue topHash();
  virtual NABoolean duplicateMatch(const RelExpr & other) const;
  virtual RelExpr * copyTopNode(RelExpr *derivedNode = NULL,
				CollHeap* outHeap = 0);

  // synthesize logical properties
  virtual void synthLogProp(NormWA * normWAPtr = NULL);
  virtual void synthEstLogProp(const EstLogPropSharedPtr& inputEstLogProp);

  // cost functions
  virtual CostMethod* costMethod() const;
  virtual PhysicalProperty *synthPhysicalProperty(const Context *context,
                                                  const Lng32     planNumber,
                                                  PlanWorkSpace  *pws);

  NABoolean& rejectPredicates() { return rejectPredicates_; }

  // get a printable string that identifies the operator
  virtual const NAString getText() const;

  // append an ascii-version of Tuple into cachewa.qryText_
  virtual void generateCacheKey(CacheWA& cwa) const;

  // is this entire expression cacheable after this phase?
  virtual NABoolean isCacheableExpr(CacheWA& cwa);

  // change literals of a cacheable query into ConstantParameters
  virtual RelExpr* normalizeForCache(CacheWA& cwa, BindWA& bindWA);

private:

  // an expression with the value of the single tuple
  ItemExpr   * tupleExprTree_;
  ValueIdList tupleExpr_;

  // If TRUE, will not accept predicates pushed from above.
  NABoolean   rejectPredicates_;

  // This id is added to uniquely distinguish tuple expressions
  // We do not want valus(1) and values(1) to be considered the same
  // RelExpr in cascades memo which results in eliminating legitimate
  // Tuple expressions
  Lng32 id_;

  // Static counter used to ensure unique id to each Tuple object
  static THREAD_P Lng32 idCounter_;
}; // class Tuple

// -------------------------------------------------------------------------
// The TupleList class represents a table with multiple rows (tuples) in it.
// This is both a logical and a physical operator.
// -------------------------------------------------------------------------
class TupleList : public Tuple
{
public:

  // constructor
  TupleList(ItemExpr *tupleListExpr,
	CollHeap *oHeap = CmpCommon::statementHeap())
    : Tuple(REL_TUPLE_LIST,tupleListExpr, oHeap), numberOfTuples_(-1), 
      flags_(0)
  {}

  TupleList(const TupleList & other);

  ValueIdList & castToList() { return castToList_; }
  Lng32 numTuples() const { return numberOfTuples_; }

  // a virtual function for performing name binding within the query tree
  RelExpr * bindNode(BindWA *bindWAPtr);

  // Each operator supports a (virtual) method for transforming its
  // scalar expressions to a canonical form
  //
  virtual void transformNode(NormWA & normWARef,
                             ExprGroupId & locationOfPointerToMe);

  // a method used for recomputing the outer references (external dataflow
  // input values) that are still referenced by each operator in the
  // subquery tree after the preidcate pull up is complete.
  virtual void recomputeOuterReferences();

  // Each operator supports a (virtual) method for rewriting its
  // value expressions.
  virtual void rewriteNode(NormWA & normWARef);

  // add all the expressions that are local to this
  // node to an existing list of expressions (used by GUI tool)
  virtual void addLocalExpr(LIST(ExprNode *) &xlist,
			    LIST(NAString) &llist) const;

  virtual RelExpr * copyTopNode(RelExpr *derivedNode = NULL,
				CollHeap* outHeap = 0);

  // synthesize logical properties
  virtual void synthLogProp(NormWA * normWAPtr = NULL);
  virtual void synthEstLogProp(const EstLogPropSharedPtr& inputEstLogProp);

  // method to do code generation
  virtual short codeGen(Generator*);

  // get a printable string that identifies the operator
  virtual const NAString getText() const;

  virtual NABoolean isLogical() const { return TRUE; };
  virtual NABoolean isPhysical() const { return FALSE; };

  // set vidlist = ith tuple of this tuplelist and return TRUE
  RelExpr* getTuple(BindWA *bindWA, ValueIdList& vidList, CollIndex i);

  NABoolean createdForInList() { return (flags_ & CREATED_FOR_IN_LIST) != 0; }
  void setCreatedForInList(NABoolean v)
  { (v ? flags_ |= CREATED_FOR_IN_LIST : flags_ &= ~CREATED_FOR_IN_LIST); }

  NABoolean hiveTextInsert() { return (flags_ & HIVE_TEXT_INSERT) != 0; }
  void setHiveTextInsert(NABoolean v)
  { (v ? flags_ |= HIVE_TEXT_INSERT : flags_ &= ~HIVE_TEXT_INSERT); }

  NABoolean castTo() { return (flags_ & CAST_TO) != 0; }
  void setCastTo(NABoolean v)
  { (v ? flags_ |= CAST_TO : flags_ &= ~CAST_TO); }

private:
  enum Flags
    {
      CREATED_FOR_IN_LIST   = 0x0001,
      HIVE_TEXT_INSERT      = 0x0002,
      CAST_TO               = 0x0004
    };

  // set needsFixup to TRUE iff tuplelist needs INFER_CHARSET fixup
  RelExpr* needsCharSetFixup(BindWA *bindWA,
                             CollIndex arity,
                             CollIndex nTuples,
                             NAList<NABoolean> &strNeedsFixup,
                             NABoolean &needsFixup);

  // find fixable strings' inferredCharTypes
  RelExpr* pushDownCharType(BindWA *bindWA,
                            enum CharInfo::CharSet cs,
                            NAList<const CharType*> &inferredCharType,
                            NAList<NABoolean> &strNeedsFixup,
                            CollIndex arity,
                            CollIndex nTuples);

  // do INFER_CHARSET fixup
  RelExpr* doInferCharSetFixup(BindWA *bindWA,
                               enum CharInfo::CharSet cs,
                               CollIndex arity,
                               CollIndex nTuples);

  // cached number of tuples in the tuple list
  Lng32 numberOfTuples_;

  // list containing the value ids whose type the tuples are to be
  // converted before returning. Used (currently) if this list will be
  // inserted into a table.
  ValueIdList castToList_;

  Int32 flags_;
}; // class TupleList

// -----------------------------------------------------------------------
// The PhysicalTuple replaces the logical Tuple through the
// application of the PhysicalTupleRule. This transformation is
// designed to present a purely physical verison of an operator
// that is both logical and physical.
// -----------------------------------------------------------------------
class PhysicalTuple : public Tuple
{
public:
  PhysicalTuple(const Tuple & other) : Tuple(other) {};

  PhysicalTuple(CollHeap *oHeap = CmpCommon::statementHeap())
    : Tuple(REL_TUPLE, NULL, oHeap) {};

  virtual NABoolean isLogical() const;
  virtual NABoolean isPhysical() const;
  virtual RelExpr* preCodeGen(Generator*, const ValueIdSet&, ValueIdSet &);

}; // class PhysicalTuple

// -----------------------------------------------------------------------
// The PhysicalTupleList replaces the logical TupleList through the
// application of the PhysicalTupleListRule. This transformation is
// designed to present a purely physical verison of an operator.
// -----------------------------------------------------------------------
class PhysicalTupleList : public TupleList
{
public:
  PhysicalTupleList(const TupleList & other) : TupleList(other) {};

  PhysicalTupleList(CollHeap *oHeap = CmpCommon::statementHeap())
    : TupleList(NULL, oHeap) {};

  virtual NABoolean isLogical() const { return FALSE; }
  virtual NABoolean isPhysical() const { return TRUE; }
  virtual RelExpr* preCodeGen(Generator*, const ValueIdSet&, ValueIdSet &);

}; // class PhysicalTupleList

// -----------------------------------------------------------------------
// The Filter is introduced as well as eliminated by the rules.
// It is a placeholder for predicates that are pushed down.
// -----------------------------------------------------------------------
class Filter : public RelExpr
{
public:

  // constructor, destructor
  Filter(RelExpr * queryTree, CollHeap *oHeap = CmpCommon::statementHeap())
    : RelExpr(REL_FILTER, queryTree, NULL, oHeap)
  {
    reattemptPushDown_ = FALSE;
  }

  virtual ~Filter();

  // get the degree of this node (it is a unary op).
  virtual Int32 getArity() const;

  virtual HashValue topHash();
  virtual NABoolean duplicateMatch(const RelExpr & other) const;
  virtual RelExpr * copyTopNode(RelExpr *derivedNode = NULL,
				CollHeap* outHeap = 0);
  virtual void getPotentialOutputValues(ValueIdSet & vs) const;

  virtual RelExpr * normalizeNode(NormWA & normWARef) ;

  // flows compRefOpt constraints up the query tree.
  virtual void processCompRefOptConstraints(NormWA * normWAPtr) ;

  // synthesize logical properties
  virtual void synthLogProp(NormWA * normWAPtr = NULL);
  virtual void synthEstLogProp(const EstLogPropSharedPtr& inputEstLogProp);

  virtual const NAString getText() const;

  void setReattemptPushDown()                { reattemptPushDown_ = TRUE; }
  void resetReattemptPushDown()             { reattemptPushDown_ = FALSE; }
  NABoolean reattemptPushDown()              { return reattemptPushDown_; }

////////////////////////////////////
  virtual void primeGroupAnalysis();
////////////////////////////////////

private:

  // Indicator of whether this filter node carries predicates which we want
  // to re-attempt to push down. That is, they were not successfully pushed
  // down last time we tried, but we want to retry this again, due to new
  // conditions (like elimination of a group by). This flag is here as part
  // of a temporary fix. 
  //
  NABoolean reattemptPushDown_;

}; // class Filter

// -----------------------------------------------------------------------
// The Rename class is an abstract class for name mapping operations.
// Derived from it: RenameTable, and RenameReference and BeforeTrigger.
// Rename objects pass name mapping information from the parser to the binder,
// or are used internally in the binder. These nodes disappear during
// transformation, therefore the normalization method below just ASSERT.
// -----------------------------------------------------------------------
class Rename : public RelExpr
{
public:

  // constructors, destructor
  Rename(RelExpr *child, OperatorTypeEnum otype = REL_RENAME,
	  CollHeap *oHeap = CmpCommon::statementHeap() )
    : RelExpr(otype, child, NULL, oHeap)
  {}

  virtual ~Rename();

  // get the degree of this node (it is a unary op).
  virtual Int32 getArity() const;

  // Each operator supports a (virtual) method for transforming its
  // scalar expressions to a canonical form
  virtual void transformNode(NormWA & normWARef,
			     ExprGroupId & locationOfPointerToMe);

  // Make sure Rename nodes dissappear during Transformation.
  virtual RelExpr * normalizeNode(NormWA & normWARef)
	{ CMPASSERT(FALSE); return this; }

  virtual HashValue topHash();
  virtual NABoolean duplicateMatch(const RelExpr & other) const;

  // get a printable string that identifies the operator
  // Pure virtual - implemented by sub-classes.
  virtual const NAString getText() const = 0;

}; // class Rename

// -----------------------------------------------------------------------
// The RenameTable operator is used by Parser and Binder to allow name
// mapping of external (ANSI) SQL table and column names.
// -----------------------------------------------------------------------
class RenameTable : public Rename
{
public:

  // constructors, destructor
  RenameTable(RelExpr *child,
	      const NAString &corrName,
	      ItemExpr *colNames = NULL,
	      CollHeap *oHeap = CmpCommon::statementHeap(),
              const NABoolean isView = FALSE)
    : Rename(child, REL_RENAME_TABLE, oHeap),
      newTableName_(corrName, FALSE/*not fabricated, but name IS corr only*/, oHeap),
      newColNamesTree_(colNames),
      isView_(isView),
      viewNATable_(NULL)
  {}

  RenameTable(NABoolean /*just to make it obvious a different ctor is called*/,
	      RelExpr *child,
	      const CorrName &corrName,
	      ItemExpr *colNames = NULL,
	      CollHeap *oHeap = CmpCommon::statementHeap(),
              const NABoolean isView = FALSE)
    : Rename(child, REL_RENAME_TABLE, oHeap),
      newTableName_(corrName, oHeap),	// (non-standard) copy ctor
      newColNamesTree_(colNames),
      isView_(isView),
      viewNATable_(NULL)
  {
    CMPASSERT(!newTableName_.isFabricated());
    // This next code is wrong, doesn't work for bindView()!
    //
    // if (newTableName_.getCorrNameAsString().isNull()) {
    //   newTableName_.setCorrName(
    //     newTableName_.getQualifiedNameObj().getObjectName());
    //   CMPASSERT(!newTableName_.getCorrNameAsString().isNull());
    // }
  }

  // copy ctor
  RenameTable (const RenameTable & orig,
               CollHeap * h=CmpCommon::statementHeap()) ; // not written

  virtual ~RenameTable();

  // get the table and column names
  const CorrName &getTableName() const { return newTableName_; }
  ItemExpr *removeColNameTree();

  // is this object created for a view?
  NABoolean isView() const { return isView_; }

  // a virtual function for performing name binding within the query tree
  RelExpr * bindNode(BindWA *bindWAPtr);

  // MV --
  NABoolean virtual isIncrementalMV() { return TRUE; }
  void virtual collectMVInformation(MVInfoForDDL *mvInfo,
									NABoolean isNormalized);

  // add all the expressions that are local to this
  // node to an existing list of expressions (used by GUI tool)
  virtual void addLocalExpr(LIST(ExprNode *) &xlist,
			    LIST(NAString) &llist) const;

  virtual RelExpr * copyTopNode(RelExpr *derivedNode = NULL,
				CollHeap* outHeap = 0);

  // get a printable string that identifies the operator
  virtual const NAString getText() const;

  const NATable * getViewNATable() {return viewNATable_ ;}
  void setViewNATable(const NATable * val) {viewNATable_ = val;}


private:

  // the new user-specified name of the table
  CorrName newTableName_;

  // the column name list specified in the rename clause
  ItemExpr *newColNamesTree_;

  // is this object created for a view?
  NABoolean isView_;

  // NATable for view
  const NATable * viewNATable_ ;
}; // class RenameTable

// -----------------------------------------------------------------------
// The RenameReference node implements the REFERENCING clause of trigger
// definitions. A list of TableRefName objects is passed from the parser
// to the binder.
// -----------------------------------------------------------------------
class RenameReference : public Rename
{
public:

  // constructors, destructor
  RenameReference(RelExpr *child,
		  TableRefList& tableReferences,
	      CollHeap *oHeap = CmpCommon::statementHeap() )
    : Rename(child, REL_RENAME_REFERENCE, oHeap),
	  tableReferences_(tableReferences)
  {}

  virtual ~RenameReference();

  // a virtual function for performing name binding within the query tree
  RelExpr * bindNode(BindWA *bindWAPtr);

  // utility method for binding.
  void prepareRETDescWithTableRefs(BindWA *bindWA);

  virtual RelExpr * copyTopNode(RelExpr *derivedNode = NULL,
				CollHeap* outHeap = 0);

  // add all the expressions that are local to this
  // node to an existing list of expressions (used by GUI tool)
  virtual void addLocalExpr(LIST(ExprNode *) &xlist,
			    LIST(NAString) &llist) const;

  // get the table references list
  TableRefList& getRefList()
	{ return tableReferences_; }

  // get a printable string that identifies the operator
  virtual const NAString getText() const;

private:

  TableRefList tableReferences_;
}; // class RenameReference

// -----------------------------------------------------------------------
// The BeforeTrigger class implements a before trigger.
// -----------------------------------------------------------------------
class BeforeTrigger : public Rename
{
public:
  // constructors, destructor
  BeforeTrigger(TableRefList& tableReferences,
				ItemExpr     *whenClause,
				ItemExprList *setList,
				CollHeap *oHeap = CmpCommon::statementHeap() )
    : Rename(NULL, REL_BEFORE_TRIGGER, oHeap),
	  tableReferences_(tableReferences),
	  whenClause_(whenClause),
	  signal_(NULL),
	  setList_(setList),
	  isSignal_(FALSE),
	  parentTSJ_(NULL)
  { }

  BeforeTrigger(TableRefList& tableReferences,
				ItemExpr     *whenClause,
				RaiseError   *signal,
				CollHeap *oHeap = CmpCommon::statementHeap() )
    : Rename(NULL, REL_BEFORE_TRIGGER, oHeap),
	  tableReferences_(tableReferences),
	  whenClause_(whenClause),
	  signal_(signal),
	  setList_(NULL),
	  isSignal_(TRUE),
	  parentTSJ_(NULL)
  { }

  virtual ~BeforeTrigger();

  // get the degree of this node (it is a unary op).
  virtual Int32 getArity() const;

  // a virtual function for performing name binding within the query tree
  RelExpr * bindNode(BindWA *bindWAPtr);

  ItemExpr *getWhenClause() { return whenClause_; }
  void setWhenClause(ItemExpr *exp) { whenClause_ = exp; }

  void bindSetClause(BindWA *bindWA, RETDesc *origRETDesc, CollHeap *heap);
  void bindSignalClause(BindWA *bindWA, RETDesc *origRETDesc, CollHeap *heap);

  // Fix the inputs before calling the inherited method.
  virtual void transformNode(NormWA & normWARef,
			     ExprGroupId & locationOfPointerToMe);

  virtual RelExpr * copyTopNode(RelExpr *derivedNode = NULL,
				CollHeap* outHeap = 0);

  // add all the expressions that are local to this
  // node to an existing list of expressions (used by GUI tool)
  virtual void addLocalExpr(LIST(ExprNode *) &xlist,
			    LIST(NAString) &llist) const;

  // get the table references list
  TableRefList& getRefList()
	{ return tableReferences_; }

  // get a printable string that identifies the operator
  virtual const NAString getText() const;

  // Find the position and name of a Assign target column.
  Lng32 getTargetColumn(CollIndex i, ColRefName* targetColName, const NATable *naTable);

  // Do DDL semantic checks on the SET clause.
  void doSetSemanticChecks(BindWA *bindWA, RETDesc *origRETDesc);

  // Add the positions of all Assign target columns to ColsToSet.
  void addUpdatedColumns(UpdateColumns *colsToSet, const NATable *naTable);

  void setParentTSJ(RelExpr *parent) { parentTSJ_ = parent; }

private:

  TableRefList tableReferences_;
  ItemExpr     *whenClause_;
  RaiseError   *signal_;
  ItemExprList *setList_;
  NABoolean     isSignal_;
  RelExpr	   *parentTSJ_;
}; // class BeforeTrigger

// -----------------------------------------------------------------------
// The BinderOnlyNode class is an abstract class for nodes that implement
// commands that transform themselves during binding to more complex
// RelExpr trees. Derived from it are Refresh and MvLog.
// Their bindNode() method constructs a complex RelExpr tree that
// replaces the node itself in the tree. This is why the transformNode()
// method is implemented as an assert(FALSE).
// -----------------------------------------------------------------------
class BinderOnlyNode : public RelExpr
{
public:

  // constructors, destructor
  BinderOnlyNode(OperatorTypeEnum otype = REL_BINDER_ONLY,
	  CollHeap *oHeap = CmpCommon::statementHeap() )
    : RelExpr(otype, NULL, NULL, oHeap)
  {}

  virtual ~BinderOnlyNode()
  {};

  // Since the destructor is never really called, at least clean up
  // internal data before disappearing. Should be called before returning
  // from bindNode().
  virtual void cleanupBeforeSelfDestruct() = 0;

  // get the degree of this node (it is a unary op).
  virtual Int32 getArity() const { return 0; }

  // This is where all the action is.
  // Pure virtual - implemented by sub-classes.
  virtual RelExpr *bindNode(BindWA *bindWA) = 0;

  virtual void transformNode(NormWA & normWARef,
			     ExprGroupId & locationOfPointerToMe)
  { CMPASSERT(FALSE);}

}; // class BinderAnimal

// -----------------------------------------------------------------------
// The map value ids operator is used to create a link between
// logically equivalent expressions that have incompatible
// characteristic outputs and therefore cannot be in the same group.
// -----------------------------------------------------------------------
class MapValueIds : public RelExpr
{
public:

  // constructor and destructor
  MapValueIds(RelExpr  *child = NULL,
	      CollHeap *oHeap = CmpCommon::statementHeap())
    : RelExpr(REL_MAP_VALUEIDS,child,NULL,oHeap), 
      includesFavoriteMV_(FALSE),
      cseRef_(NULL)                                                    {}

  MapValueIds(RelExpr *child,
	      const ValueIdSet &identity,
	      CollHeap *oHeap = CmpCommon::statementHeap())
    : RelExpr(REL_MAP_VALUEIDS,child,NULL,oHeap),
      map_(identity), includesFavoriteMV_(FALSE),
      cseRef_(NULL)                                                    {}

  MapValueIds(RelExpr *child,
	      const ValueIdMap &map,
	      CollHeap *oHeap = CmpCommon::statementHeap())
    : RelExpr(REL_MAP_VALUEIDS,child,NULL,oHeap),
    map_(map), includesFavoriteMV_(FALSE),
      cseRef_(NULL)                                                    {}

  MapValueIds(const MapValueIds & other)
    : RelExpr(REL_MAP_VALUEIDS, other.child(0)),
      map_(other.map_),
      valuesNeededForVEGRewrite_(other.valuesNeededForVEGRewrite_),
      includesFavoriteMV_(other.includesFavoriteMV_),
      cseRef_(other.cseRef_)                                           {}

  virtual ~MapValueIds();

  // various PC methods
  virtual Int32 getArity() const;
  virtual void addLocalExpr(LIST(ExprNode *) &xlist,
			    LIST(NAString) &llist) const;
  virtual HashValue topHash();
  virtual NABoolean duplicateMatch(const RelExpr & other) const;
  virtual RelExpr * copyTopNode(RelExpr *derivedNode = NULL,
				CollHeap* outHeap = 0);
  virtual const NAString getText() const;

  virtual void synthLogProp(NormWA * normWAPtr = NULL);
  virtual void synthEstLogProp(const EstLogPropSharedPtr& inputEstLogProp);

  virtual CostMethod* costMethod() const;
  virtual Context* createContextForAChild(Context* myContext,
                     PlanWorkSpace* pws,
                     Lng32& childIndex);
  virtual PhysicalProperty *synthPhysicalProperty(const Context *context,
                                                  const Lng32     planNumber,
                                                  PlanWorkSpace  *pws);

  // accessor functions
  ValueIdMap & getMap() { return map_; }
  const ValueIdMap & getMap2()  const { return map_; }
  inline void remapTopValue(const ValueId & newTopValue,
			    const ValueId & bottomValue)
                          { map_.remapTopValue(newTopValue,bottomValue); }
  inline void remapBottomValue(const ValueId & topValue,
			       const ValueId & newBottomValue)
                       { map_.remapBottomValue(topValue,newBottomValue); }
  inline void addMapEntry(const ValueId & newTopValue,
			  const ValueId & newBottomValue)
                         { map_.addMapEntry(newTopValue,newBottomValue); }
  inline void clear()                                    { map_.clear(); }
  inline const ValueIdSet & valuesForVEGRewrite()
                                    { return valuesNeededForVEGRewrite_; }
  inline void addValueForVEGRewrite(const ValueId & v)
                                      { valuesNeededForVEGRewrite_ += v; }
  inline void addValuesForVEGRewrite(const ValueIdSet & v)
                                      { valuesNeededForVEGRewrite_ += v; }
  inline void clearValuesForVEGRewrite()
                                   { valuesNeededForVEGRewrite_.clear(); }
  void addSameMapEntries(const ValueIdSet & newTopBottomValues);
  void setCSERef(CommonSubExprRef *cse)                 { cseRef_ = cse; }

  // Method to compute child's characteristic outputs
  virtual
  void pushdownCoveredExpr(const ValueIdSet & outputExprOnOperator,
                           const ValueIdSet & newExternalInputs,
                           ValueIdSet& predOnOperator,
			   const ValueIdSet * nonPredExprOnOperator = NULL,
                           Lng32 childId = (-MAX_REL_ARITY) );

  // The set of values that I can potentially produce as output.
  virtual void getPotentialOutputValues(ValueIdSet & vs) const;

  // method to do code generation
  virtual RelExpr * preCodeGen(Generator * generator,
			       const ValueIdSet & externalInputs,
			       ValueIdSet &pulledNewInputs);
  virtual short codeGen(Generator*);

  // -----------------------------------------------------------------------
  // Performs mapping on the partitioning function, from the mapValueIds
  // node to the child.
  // -----------------------------------------------------------------------
  virtual PartitioningFunction* mapPartitioningFunction(
                          const PartitioningFunction* partFunc,
                          NABoolean rewriteForChild0) ;

  // for mv query rewrite
  void setIncludesFavoriteMV (NABoolean value)
          {includesFavoriteMV_ = value;}

  NABoolean includesFavoriteMV () const
          {return includesFavoriteMV_;}

private:

  ValueIdMap map_;
  ValueIdSet valuesNeededForVEGRewrite_;
  NABoolean  includesFavoriteMV_;
  NABoolean  replaceVEGUsingList_;
  CommonSubExprRef *cseRef_;
};

// -----------------------------------------------------------------------
// The PhysicalMapValueIds replaces the logical MapValueIds through the
// application of the PhysicalMapValueIdsRule. This transformation is
// designed to present a purely physical version of an operator
// that is both logical and physical.
// -----------------------------------------------------------------------
class PhysicalMapValueIds : public MapValueIds
{
public:
  PhysicalMapValueIds(RelExpr  *childExpr,
		      CollHeap *oHeap = CmpCommon::statementHeap())
    : MapValueIds(childExpr,oHeap)  {};

  PhysicalMapValueIds(const MapValueIds & other) : MapValueIds(other)     {};

  virtual NABoolean isLogical() const;
  virtual NABoolean isPhysical() const;

  virtual PlanPriority computeOperatorPriority
    (const Context* context,
     PlanWorkSpace *pws=NULL,
     Lng32 planNumber=0);

}; // class PhysicalMapValueIds

// -----------------------------------------------------------------------
// The FirstN class is used to return the first N rows that are returned
// by its child. At runtime, it sends a Get_N to its child and returns the
// N rows returned by its child to its parent. It doesn't do anything
// else on those returned row.
// This class could be put anywhere in query tree. It eliminates the need
// for a runtime operator to know if it needs to look at its compile time
// first N rows entry or its down queue GET_N entry before it could return
// the N rows. With this node, the runtime operators only need to process
// the GET_N down queue correctly.
// -----------------------------------------------------------------------
class FirstN : public RelExpr
{
public:
 FirstN(RelExpr * child,
        Int64 firstNRows,
        NABoolean isFirstN,
        ItemExpr * firstNRowsParam = NULL,
        CollHeap *oHeap = CmpCommon::statementHeap())
   : RelExpr(REL_FIRST_N, child, NULL, oHeap),
    firstNRows_(firstNRows),
    firstNRowsParam_(firstNRowsParam),
    canExecuteInDp2_(FALSE),
    isFirstN_(isFirstN)
    {
      setNonCacheable();
    };
  
  // sets the canExecuteInDp2 flag for the [LAST 1] operator
  // of an MTS delete and calls the base class implementation of bindNode.
  virtual RelExpr* bindNode(BindWA* bindWA);

  // takes care of any ordering requirement on the child
  virtual Context* createContextForAChild(Context* myContext,
                     PlanWorkSpace* pws,
                     Lng32& childIndex);

  //
  // Physical properties implemented in OptPhysRelExpr.cpp
  //
  PhysicalProperty *synthPhysicalProperty(const Context* myContext,
					  const Lng32     planNumber,
                                          PlanWorkSpace  *pws);

  virtual RelExpr * preCodeGen(Generator * generator,
			       const ValueIdSet & externalInputs,
			       ValueIdSet &pulledNewInputs);

  // method to do code generation
  virtual short codeGen(Generator*);

  // this is both logical and physical node
  virtual NABoolean isLogical() const{return TRUE;};
  virtual NABoolean isPhysical() const{return TRUE;};

  // various PC methods

  // get the degree of this node
  virtual Int32 getArity() const{return 1;};
  virtual RelExpr * copyTopNode(RelExpr *derivedNode = NULL,
				CollHeap* outHeap = 0);
  virtual const NAString getText() const;

  void setFirstNRows(Int64 firstNRows) 		{ firstNRows_ = firstNRows; }
  Int64 getFirstNRows() 			{ return firstNRows_; }

  void setFirstNRowsParam(ItemExpr *firstNRowsParam)   
  { firstNRowsParam_ = firstNRowsParam; }
  ItemExpr * getFirstNRowsParam() 			{ return firstNRowsParam_; }

  void setCanExecuteInDp2(NABoolean flag) 	{ canExecuteInDp2_ = flag; }
  NABoolean canExecuteInDp2() const             { return canExecuteInDp2_; }
  virtual NABoolean computeRowsAffected()   const ;

  NABoolean isFirstN()                          { return isFirstN_; }

  ValueIdList & reqdOrder()                     { return reqdOrder_; }
  ValueIdList & reqdOrderInSubquery()           { return reqdOrderInSubquery_; }

private:
  // Otherwise, return firstNRows_ at runtime.
  Int64 firstNRows_;
  ItemExpr * firstNRowsParam_;
  NABoolean canExecuteInDp2_;
  NABoolean isFirstN_;  // TRUE if [first n], FALSE if [any n] or [last n]

  // Optional ORDER BY to force ordering before applying First N; populated
  // at normalizeNode time.
  ValueIdList reqdOrder_;
  ValueIdList reqdOrderInSubquery_;

}; // class FirstN


// Class Transpose ----------------------------------------------------
// The Transpose RelExpr is a logical RelExpr node used to implement the
// TRANSPOSE operator. It is implemented by the physical RelExpr node
// PhysTranspose. (It would be possible to develop an implementation rule
// which implemented this node as a tree of union nodes.) The data members
// of the class are:
//
//   transValsTree_: This ItemExpr tree contains a list of pairs which is
//   NULL terminated (for ease of processing).  Each pair contains in child(0),
//   a list of transpose items for a given transpose set and in child(1), a
//   list of ColReferences to the new value columns associated with this
//   transpose set. A transpose item is a list of value expressions.
//   This pointer is set to NULL by bindNode.
//
//   keyCol_: Contains the ColReference to the key column.
//   This data member is constructed by the parser.
//   This ItemExpr tree is set to NULL in the Binder (Transpose::BindNode()).
//
//   transUnionVector_: This is a vector of ValueIdLists. There is one entry
//   for each transpose set, plus one entry for the key values. Each entry
//   contains a list of ValueIdUnion Nodes. The first entry contains a list
//   with one ValueIdUnion node. This node is for the Const. Values (1 - N)
//   representing the Key Values. The other entries contain lists of
//   ValueIdUnion nodes for the Transposed Values. Each of these entries of
//   the vector represent a transpose set.  If the transpose set contains a
//   list of values, then there will be only one ValueIdUnion node in the
//   list.  If the transpose set contains a list of lists of values, then
//   there will be as many ValueIdUnion nodes as there are items in the
//   sublists. (see example below.)
//   transUnionVector_ is generated in bindNode().
//
//   transUnionVectorSize_: This is the number of entries in transUnionVector_.
//
class Transpose : public RelExpr
{
public:

  // The constructor
  //
  // Inputs:
  //
  //  transExprs: Contains a list of pairs, each pair contains
  //  in child(0), a list of expressions for a given transpose group
  //  and in child(1), the name associated with this transpose group.
  //  Default value: NULL
  //
  //  keyCol: Contains the ColReference to the key column.
  //  Default value: NULL
  //
  //  child: The child RelExpr node of this node.
  //  Default Value NULL
  //
  //  Constructs a Transpose Node.
  //
  //  Called by Parser (yyparse()), CreateImplementationRules(), Transpose::
  //  copyTopNode(), and the PhysTranspose constructor.
  Transpose(ItemExpr *transExprs = NULL,
		   ItemExpr *keyCol = NULL,
		   RelExpr *child = NULL,
		   CollHeap *oHeap = CmpCommon::statementHeap())

  : RelExpr(REL_TRANSPOSE,child,NULL,oHeap),
    transValsTree_(transExprs),
    keyCol_(keyCol),
    transUnionVectorSize_(0),
    transUnionVector_(0)
  {
    setNonCacheable();
  }

  // The destructor
  //
  virtual ~Transpose();

  // Transpose has one child.
  //
  virtual Int32 getArity() const {return 1;};

  // Return a pointer to the transpose Values tree.
  //
  inline const ItemExpr *transValsTree() const {return transValsTree_;};

  // Return a pointer to the transpose Values tree after
  // setting it to NULL.
  //
  const ItemExpr *removeTransValsTree();

  // Return a pointer to the keyCol ColReference node.
  //
  inline const ItemExpr *keyCol() const {return keyCol_;};

  // Return a pointer to the keyCol ColReference node after setting
  // it to NULL.
  //
  const ItemExpr *removeKeyCol();

  // return a read/write reference to the transpose Union Vector.
  //
  inline ValueIdList *&transUnionVector()
  {
    return transUnionVector_;
  };

  // return a read-only reference to the transpose Union Vector.
  //
  inline const ValueIdList *transUnionVector() const
  {
    return transUnionVector_;
  };

  // return the size of the transpose union vector.
  //
  inline CollIndex transUnionVectorSize() const
  {
    return transUnionVectorSize_;
  };

  inline void setTransUnionVectorSize(CollIndex size)
  {
    transUnionVectorSize_ = size;
  };

  // This method is used in case there are expressions in the Transpose column
  // list. It traverses through the expression to get the column under them
  // If it is a unary expression, it gets the column directly below the expression.
  // If the expression has two children, it goes through both the children
  // to see which one of them has a higher UEC. It returns the ValueId of that
  // column. This column is later used to determine the UEC of the final
  // transpose column.

  ValueId getSourceColFromExprForUec(ValueId sourceValId, const ColStatDescList & childColStatsList);

  // a virtual function for performing name binding within the query tree
  // Transpose::bindNode() generates a list of ValueIdUnion nodes from the
  // transValsTree constructed by the parser.
  //
  RelExpr * bindNode(BindWA *bindWAPtr);

  // Each operator supports a (virtual) method for transforming its
  // scalar expressions to a canonical form
  //
  virtual void transformNode(NormWA & normWARef,
			     ExprGroupId & locationOfPointerToMe);

  // a method used during subquery transformation for pulling up predicates
  // towards the root of the transformed subquery tree
  //
  // The default implementation is adequate for Transpose
  //virtual void pullUpPreds();

  // a method used for recomputing the outer references (external dataflow
  // input values) that are still referenced by each operator in the
  // subquery tree after the predicate pull up is complete.
  //
  virtual void recomputeOuterReferences();

  // Each operator supports a (virtual) method for rewriting its
  // value expressions.
  //
  virtual void rewriteNode(NormWA & normWARef);

  // Each operator supports a (virtual) method for performing
  // predicate pushdown and computing a "minimal" set of
  // characteristic input and characteristic output values.
  //
  // The default implementation is adequate for Transpose
  //virtual RelExpr * normalizeNode(NormWA & normWARef);

  // Method to push down predicates from a Transpose node into the
  // children
  //
  virtual
  void pushdownCoveredExpr(const ValueIdSet & outputExprOnOperator,
                           const ValueIdSet & newExternalInputs,
                           ValueIdSet& predOnOperator,
			   const ValueIdSet * nonPredExprOnOperator = NULL,
                           Lng32 childId = (-MAX_REL_ARITY) );

  // Return a the set of potential output values of this node.
  // For transpose, this is the potential outputs of the child,
  // plus the key column and all the value columns generated by
  // transpose.
  //
  virtual void getPotentialOutputValues(ValueIdSet &vs) const;

  // add all the expressions that are local to this
  // node to an existing list of expressions (used by GUI tool and Explain)
  //
  virtual void addLocalExpr(LIST(ExprNode *) &xlist,
			    LIST(NAString) &llist) const;

  // Compute a hash value for a chain of derived RelExpr nodes.
  // Used by the Cascade engine as a quick way to determine if
  // two nodes are identical.
  // Can produce false positives, but should not produce false
  // negatives.
  //
  virtual HashValue topHash();

  // A more thorough method to compare two RelExpr nodes.
  // Used by the Cascades engine.
  //
  virtual NABoolean duplicateMatch(const RelExpr & other) const;

  // Copy a chain of derived nodes (Calls RelExpr::copyTopNode).
  // Needs to copy all relevant fields.
  // Used by the Cascades engine.
  //
  virtual RelExpr *copyTopNode(RelExpr *derivedNode = NULL,
			       CollHeap *outHeap = NULL);

  // synthesize logical properties
  //
  virtual void synthLogProp(NormWA * normWAPtr = NULL);
  virtual void synthEstLogProp(const EstLogPropSharedPtr& inputEstLogProp);

  // get a printable string that identifies the operator
  //
  virtual const NAString getText() const {return "TRANSPOSE";};

private:

  // This ItemExpr tree contains a list of pairs which is
  // NULL terminated (for ease of processing).  Each pair contains in child(0),
  // a list of transpose items for a given transpose set and in child(1), a
  // list of ColReferences to the new value columns associated with this
  // transpose set. A transpose item is a list of value expressions.
  // This pointer is set to NULL by bindNode.
  //
  ItemExpr   *transValsTree_;

  // Contains the ColReference to the key column.
  // This ItemExpr tree is set to NULL in the Binder (Transpose::BindNode()).
  //
  ItemExpr *keyCol_;

  // This is the number of entries in transUnionVector_ (see below).
  //
  CollIndex transUnionVectorSize_;

  // This is a vector of ValueIdLists. There is one entry
  // for each transpose set, plus one entry for the key values. Each entry
  // contains a list of ValueIdUnion Nodes. The first entry contains a list
  // with one ValueIdUnion node. This node is for the Const. Values (1 - N)
  // representing the Key Values. The other entries contain lists of
  // ValueIdUnion nodes for the Transposed Values. Each of these entries of
  // the vector represent a transpose set.  If the transpose set contains a
  // list of values, then there will be only one ValueIdUnion node in the
  // list.  If the transpose set contains a list of lists of values, then
  // there will be as many ValueIdUnion nodes as there are items in the
  // sublists. transUnionVector_ is generated in bindNode().
  //
  ValueIdList *transUnionVector_;

}; // class Transpose

// Class PhysTranspose ----------------------------------------------------
// The PhysTranspose node replaces the logical Transpose node through the
// application of the PhysTransposeRule. This transformation is
// designed to present a purely physical verison of this operator
// that is both a logical and physical node. The PhysTranspose node
// does not add any data members. It adds a few virtual methods.
// -----------------------------------------------------------------------
class PhysTranspose : public Transpose
{
public:

  // The constructor
  //
  PhysTranspose(RelExpr *child = NULL,
		       CollHeap *oHeap = CmpCommon::statementHeap())
    : Transpose(NULL,NULL,child,oHeap) {};

  // The destructor.
  //
  virtual ~PhysTranspose();

  // methods to do code generation of the physical node.
  //
  virtual RelExpr * preCodeGen(Generator * generator,
			       const ValueIdSet & externalInputs,
			       ValueIdSet &pulledNewInputs);
  virtual short codeGen(Generator*);


  // generate CONTROL QUERY SHAPE fragment for this node.
  //
  virtual short generateShape(CollHeap * space, char * buf, NAString * shapeStr = NULL);

  // Copy a chain of derived nodes (Calls Transpose::copyTopNode).
  // Needs to copy all relevant fields (in this case no fields
  // need to be copied)
  // Used by the Cascades engine.
  //
  virtual RelExpr * copyTopNode(RelExpr *derivedNode = NULL,
				CollHeap *outHeap = NULL);

  // cost functions
  //
  virtual PhysicalProperty *synthPhysicalProperty(const Context *context,
                                                  const Lng32     planNumber,
                                                  PlanWorkSpace  *pws);
  virtual CostMethod* costMethod() const;

  // Redefine these virtual methods to declare this node as a
  // physical node.
  //
  virtual NABoolean isLogical() const {return FALSE;};
  virtual NABoolean isPhysical() const {return TRUE;};

}; // class PhysTranspose

// -----------------------------------------------------------------------
// The Pack node is used to implement packing of several logical rows into
// one physical row on a column by column basis. The no of logical rows
// packed into one physical row is determined by the packing factor (pf).
// The packed table have the same no of columns as the original table. The
// type of the column in the packed table is varchar(len * pf) where len
// is the length (in bytes) of the corresponding column in the original
// table. For example, if the input table has columns (x int, y int), and
// the packing factor is 2 the output table will be (xpacked varchar(8),
// ypacked varchar(8)). {x|y}packed will contain the values of {x|y} in 2
// contiguous rows of the original table packed together.
// -----------------------------------------------------------------------
class Pack : public RelExpr
{
public:

  // ---------------------------------------------------------------------
  // Constructor/Destructor/Accessors/Mutators.
  // ---------------------------------------------------------------------

  // Constructor.
  Pack(ULng32 packingFactor = 0,
       RelExpr* child = NULL,
       ItemExpr* packingExprTree = NULL,
       CollHeap* oHeap = CmpCommon::statementHeap());

  // Destructor.
  virtual ~Pack();

  // No of children.
  virtual Int32 getArity() const                               { return 1; }

  // Returns a (short-lived) r/w ref to packing factor in different forms.
  ULng32& packingFactorLong()        { return packingFactorLong_; }
  ULng32 packingFactorLong() const   { return packingFactorLong_; }
  inline ValueIdSet& packingFactor()            { return packingFactor_; }
  inline const ValueIdSet& packingFactor() const
                                                { return packingFactor_; }

  // Returns a pointer to the packing factor item expr (& set it to NULL).
  inline const ItemExpr* packingFactorTree() const
                                            { return packingFactorTree_; }
  ItemExpr* removePackingFactorTree();

  // Returns a pointer to the packing expression tree  (& set it to NULL).
  inline const ItemExpr* packingExprTree() const
                                              { return packingExprTree_; }
  ItemExpr* removePackingExprTree();

  // Returns a (short-lived) r/w ref to the packing expression.
  inline ValueIdList& packingExpr()               { return packingExpr_; }
  inline const ValueIdList& packingExpr() const   { return packingExpr_; }

  // ---------------------------------------------------------------------
  // Some helper methods.
  // ---------------------------------------------------------------------

  // Retrieve into vidset the sub-expressions in the packing expression.
  void getNonPackedExpr(ValueIdSet& vidset);

  // ---------------------------------------------------------------------
  // Methods to support Binding.
  // ---------------------------------------------------------------------

  // It creates the RETDesc and packingExpr_ to be made of packed columns.
  virtual RelExpr* bindNode(BindWA* bindWA);

  // ---------------------------------------------------------------------
  // Methods to support Transformation.
  // ---------------------------------------------------------------------

  // Re-defined to transform the packingExpr_ which might contain a subq.
  virtual void transformNode(NormWA& normWA, ExprGroupId& locPtrToMe);

  // Refined to disallow predicates pullup since they are on unpacked col.
  virtual void pullUpPreds();

  // Refined to add in the ConstValue object needed for the packing factor.
  virtual void recomputeOuterReferences();

  // ---------------------------------------------------------------------
  // Methods to support Normalization.
  // ---------------------------------------------------------------------

  // Refined to rewrite sub-expressions in packingExpr_ to VEG if needed.
  virtual void rewriteNode(NormWA& normWA);

  virtual Context* createContextForAChild(Context* myContext,
                                          PlanWorkSpace* pws,
                                          Lng32& childIndex);

  // Refined to add in the sub-expression for each expr in packingExpr_ to
  // nonPredExprOnOperator.
  virtual void pushdownCoveredExpr(const ValueIdSet & outputExprOnOperator,
                                   const ValueIdSet& newExternalInputs,
                                   ValueIdSet& predOnOperator,
				   const ValueIdSet * nonPredExprOnOperator = NULL,
                                   Lng32 childId = (-MAX_REL_ARITY));

  // Refined to return the packing expression.
  virtual void getPotentialOutputValues(ValueIdSet& vs) const;

  // ---------------------------------------------------------------------
  // Methods to support Optimization.
  // ---------------------------------------------------------------------

  // ---------------------------------------------------------------------
  // Compute hash value for a chain of derived RelExpr nodes. Used by
  // Cascades as a quick way to determine if two nodes are identical.
  // ---------------------------------------------------------------------
  virtual HashValue topHash();

  // Exact method for Cascades to decide whether the two operators match.
  virtual NABoolean duplicateMatch(const RelExpr& other) const;

  // Used by Cascades to make a deep copy of the node.
  virtual RelExpr* copyTopNode(RelExpr* derivedNode = NULL,
                               CollHeap* outHeap = NULL);

  // Logical properties. Not yet handled in this prototype.
  virtual void synthLogProp(NormWA * normWAPtr = NULL);
  virtual void synthEstLogProp(const EstLogPropSharedPtr& inputEstLogProp);

  // ---------------------------------------------------------------------
  // Methods to support GUI/debugging.
  // ---------------------------------------------------------------------

  // Refined to display the packing expression.
  virtual void addLocalExpr(LIST(ExprNode*)& xlist,
                            LIST(NAString)& llist) const;

  // Refined to return the string describing this Pack node.
  virtual const NAString getText() const;

  // Methods for requiredOrder_
  inline const ValueIdList & requiredOrder() const { return requiredOrder_; }

  inline void setRequiredOrder(const ValueIdList &reqOrder)
   { requiredOrder_ = reqOrder; }

protected:

  inline ValueIdList & requiredOrder() { return requiredOrder_; }

private:

  // ---------------------------------------------------------------------
  // Packing factor is the no of logical rows which get packed into a
  // physical row. This member provides a more convenient way of getting
  // the packing factor (as opposed to through the ItemExpr) when it is
  // a constant. Not valid when equals zero.
  // ---------------------------------------------------------------------
  ULng32 packingFactorLong_;

  // ---------------------------------------------------------------------
  // This is the item expression tree for the packing factor. Right now,
  // the tree must point to a ConstValue.
  // ---------------------------------------------------------------------
  ItemExpr* packingFactorTree_;

  // ---------------------------------------------------------------------
  // This set contains ValueId for the packing factor tree. It is a set
  // but not just a ValueId because this facilitates expression rewriting.
  // ---------------------------------------------------------------------
  ValueIdSet packingFactor_;

  // ---------------------------------------------------------------------
  // The packing expression tree contains a list of values to the packed,
  // each topped by the packing function (PackFunc).
  // ---------------------------------------------------------------------
  ItemExpr* packingExprTree_;

  // ---------------------------------------------------------------------
  // ValueId's of the list of values in packingExprTree_.
  // ---------------------------------------------------------------------
  ValueIdList packingExpr_;

  // Return a pointer to the required order tree after
  // setting it to NULL.
  //
  ItemExpr *removeRequiredOrderTree();

  // A bound list of columns representing the required sort order
  // for this node.
  ValueIdList requiredOrder_;

}; // class Pack

// -----------------------------------------------------------------------
// An implementation rule changes the logical Pack node into the physical
// PhyPack node.
// -----------------------------------------------------------------------
class PhyPack : public Pack
{
public:

  // Constructor.
  PhyPack(ULng32 packingFactor = 0,
          RelExpr* child = NULL,
          CollHeap* oHeap = CmpCommon::statementHeap())
   : Pack(packingFactor,child,NULL,oHeap)                               {}

  // Destructor.
  virtual ~PhyPack();

  virtual NABoolean isLogical() const                    { return FALSE; }
  virtual NABoolean isPhysical() const                    { return TRUE; }

  // ---------------------------------------------------------------------
  // Methods to support Optimization.
  // ---------------------------------------------------------------------
  virtual RelExpr* copyTopNode(RelExpr* derivedNode = NULL,
                               CollHeap* outHeap = NULL);

  // ---------------------------------------------------------------------
  // Pre-Code Generation.
  // ---------------------------------------------------------------------
  virtual RelExpr* preCodeGen(Generator * generator,
			      const ValueIdSet & externalInputs,
			      ValueIdSet &pulledNewInputs);

  // generate CONTROL QUERY SHAPE fragment for this node.
  //
  virtual short generateShape(CollHeap * space, char * buf, NAString * shapeStr = NULL);

  // ---------------------------------------------------------------------
  // Code Generation.
  // ---------------------------------------------------------------------
  virtual short codeGen(Generator* generator);

  // ---------------------------------------------------------------------
  // Methods on Physical Properties and Costing.
  // ---------------------------------------------------------------------
  virtual PhysicalProperty* synthPhysicalProperty(const Context* context,
                                                  const Lng32     planNumber,
                                                  PlanWorkSpace  *pws);
  virtual CostMethod* costMethod() const;

}; // class PhyPack

class Rowset : public RelExpr
{
public:

  // ---------------------------------------------------------------------
  // Constructor/Destructor/Accessors/Mutators.
  // ---------------------------------------------------------------------

  // constructor
  Rowset(ItemExpr *inputHostvars,
         ItemExpr *indexExpr = NULL,
         ItemExpr *sizeExpr = NULL,
	 RelExpr  *childExpr = NULL,
         CollHeap *oHeap = CmpCommon::statementHeap());

  virtual ~Rowset();
  virtual RelExpr * copyTopNode(RelExpr *derivedNode = NULL,
                                CollHeap* outHeap = 0);
  virtual Int32 getArity () const;
  virtual const NAString getText() const;
  virtual RelExpr* bindNode(BindWA* bindWA);
  // returns the name of the exposed index of the Rowset
  virtual const NAString getIndexName () const;

protected:
  // an expression with the value of the single tuple
  ItemExpr *inputHostvars_;     // Host variable arrays composing the rowset
  ItemExpr *indexExpr_;         // The exposed index of the Rowset
  ItemExpr *sizeExpr_;          // RowSet size cannot be greater than declared
                                // dimensions of arrays composing rowset.
                                // If NULL, it would take smallest of declared
                                // dimensions
  RelExpr  *transformRelexpr_;  // New transformed tree
}; // class Rowset

class RowsetRowwise : public Rowset
{
public:

  // ---------------------------------------------------------------------
  // Constructor/Destructor/Accessors/Mutators.
  // ---------------------------------------------------------------------

  // constructor
  RowsetRowwise(RelExpr *childExpr = NULL,
		CollHeap *oHeap = CmpCommon::statementHeap());

  virtual RelExpr * copyTopNode(RelExpr *derivedNode = NULL,
                                CollHeap* outHeap = 0);
  virtual const NAString getText() const;
  virtual RelExpr* bindNode(BindWA* bindWA);
  virtual Int32 getArity () const;

private:
}; // class RowsetRowwise

class RowsetInto :  public RelExpr
{
public:

  // ---------------------------------------------------------------------
  // Constructor/Destructor/Accessors/Mutators.
  // ---------------------------------------------------------------------

  // constructor
  RowsetInto(RelExpr *child,
             ItemExpr *outputHostvars,
             ItemExpr *sizeExpr = NULL,
             CollHeap *oHeap = CmpCommon::statementHeap());

  virtual ~RowsetInto();
  virtual RelExpr * copyTopNode(RelExpr *derivedNode = NULL,
                                CollHeap* outHeap = 0);
  virtual Int32 getArity () const;
  virtual const NAString getText() const;
  virtual RelExpr* bindNode(BindWA* bindWA);

  inline const ValueIdList & requiredOrder() const { return requiredOrder_; }

  inline void setRequiredOrder(const ValueIdList &reqOrder)
   { requiredOrder_ = reqOrder; }

  inline void setRequiredOrderTree(ItemExpr *reqOrderTree)
  { requiredOrderTree_ = reqOrderTree; }

  inline const ItemExpr * requiredOrderTree() const { return requiredOrderTree_; }

  ItemExpr *getRowsetArrays() {return outputHostvars_;};

protected:

  inline ValueIdList & requiredOrder() { return requiredOrder_; }


private:

  // Return a pointer to the required order tree after
  // setting it to NULL.
  //
  ItemExpr *removeRequiredOrderTree();

  // an expression with the value of the single tuple
  ItemExpr *outputHostvars_;    // Host variable arrays composing the rowset
  ItemExpr *sizeExpr_;          // RowSet size cannot be greater than declared
                                // dimensions of arrays composing rowset.
                                // If NULL, it would take smallest of declared
                                // dimensions
  RelExpr  *transformRelexpr_;  // New transformed tree

  // An ItemExpr list of columns representing the reqired sort order
  // for this node.
  //
  ItemExpr *requiredOrderTree_;

  // The bound version of requiredOrderTree_.
  //

  ValueIdList requiredOrder_;

}; // class RowsetInto

class RowsetFor : public RelExpr
{
public:
  RowsetFor(RelExpr *child,
            ItemExpr *inputSizeExpr,
            ItemExpr *outputSizeExpr = NULL,
            ItemExpr *indexExpr = NULL,
	    ItemExpr *maxSizeExpr = NULL,
	    ItemExpr *maxInputRowlen = NULL,
	    ItemExpr *rwrsBuffer = NULL,
	    ItemExpr *partnNum = NULL,
            CollHeap *oHeap = CmpCommon::statementHeap());

  virtual ~RowsetFor();
  virtual RelExpr * copyTopNode(RelExpr *derivedNode = NULL,
                                CollHeap* outHeap = 0);
  virtual Int32 getArity () const;
  virtual const NAString getText() const;
  virtual RelExpr* bindNode(BindWA* bindWA);

  ItemExpr * getInputSize()  {return inputSizeExpr_;}
  ItemExpr * getOutputSize() {return outputSizeExpr_;}
  ItemExpr * getIndexExpr()  {return indexExpr_;}

  // next fields are for rowwise rowset
  ItemExpr * getMaxSizeExpr() {return maxSizeExpr_; }
  ItemExpr * getMaxInputRowlen() { return maxInputRowlen_; }
  ItemExpr * getRwrsBuffer() {return rwrsBuffer_;}
  ItemExpr * partnNum() {return partnNum_;}

  NABoolean  &rowwiseRowset() { return rowwiseRowset_; }

  void setBufferAttributes(NABoolean packedFormat, 
			   NABoolean compressed,
			   NABoolean dcompressInMaster, 
			   NABoolean compressInMaster,
			   NABoolean partnNumInBuffer)
  {
    packedFormat_ = packedFormat;
    compressed_ = compressed;
    dcompressInMaster_ = dcompressInMaster;
    compressInMaster_ = compressInMaster;
  }
  void getBufferAttributes(NABoolean &packedFormat,
			   NABoolean &compressed,
			   NABoolean &dcompressInMaster, 
			   NABoolean &compressInMaster,
			   NABoolean &partnNumInBuffer)
    {
      packedFormat = packedFormat_;
      compressed = compressed_;
      dcompressInMaster = dcompressInMaster_;
      compressInMaster = compressInMaster_;
      partnNumInBuffer = partnNumInBuffer_;
    }

private:
  ItemExpr *inputSizeExpr_;     // RowSet size cannot be greater than declared
                                // dimensions of arrays composing rowset.
                                // If NULL, it would take smallest of declared
                                // dimensions
  ItemExpr *outputSizeExpr_;    // RowSet size cannot be greater than declared
                                // dimensions of arrays composing rowset.
                                // If NULL, it would take smallest of declared
                                // dimensions
  ItemExpr *indexExpr_;         // The exposed index of the Rowset

  ItemExpr *maxSizeExpr_;       // Input rowwise rowset max size.

  ItemExpr *maxInputRowlen_;    // max length of each input row in the
                                // input rowwise rowset buffer.
  ItemExpr *rwrsBuffer_;        // Contains the address of rowwise-rowset
                                // buffer passed in at runtime to cli.
  ItemExpr *partnNum_;          // partition number where this buffer need
                                // to be shipped to.
  NABoolean rowwiseRowset_;     // layout of input values is rowwise.

  // rows are packed sql/mp style in the buffer. All columns are packed
  // next to each other with no fillers or varchar/null optimizations.
  NABoolean packedFormat_;

  // input buffer is compressed by caller.
  NABoolean compressed_;

  // Next member valid only if compressed_ is TRUE;
  // if TRUE, dcompress the buffer in master before doing other processing.
  // if FALSE, ship the compressed buffer to eid and decompress it there.
  //    This option is only valid if partnNum is specified.
  NABoolean dcompressInMaster_;

  // Next field is valid if buffer is not compressed_.
  // Next field is valid only if partnNum is specified.
  // If TRUE, master exe need to compress the buffer before shipping to eid.
  NABoolean compressInMaster_;

  // TRUE, if partition number is sent in at runtime as part of input buffer,
  // instead of dynamic parameter partnNum_
  NABoolean partnNumInBuffer_;
}; // class RowsetFor


OptSqlTableOpenInfo *setupStoi(OptSqlTableOpenInfo *&optStoi_,
                               BindWA *bindWA,
                               const RelExpr *re,
                               const NATable *naTable,
                               const CorrName &corrName,
                               NABoolean noSecurityCheck = FALSE);

/////////////////////////////////////////////////////////////////////////////
// Suspend, reactivate or cancel a query.
//
//  The Suspend and Activate functions are handled by 
// ExeUtilSuspend.
/////////////////////////////////////////////////////////////////////////////

class ControlRunningQuery : public RelExpr
{
public:

  enum Action {
    Cancel,
    Suspend,
    Activate
    };

  enum ForceOption {
    Safe,
    Force,
    CancelOrActivateIsAlwaysSafe
    };

  enum QuerySpec {
    ControlQid,
    ControlPname,
    ControlNidPid
    };


  ControlRunningQuery(NAString &qid_or_pid,
              QuerySpec qs,
              Action action,
              ForceOption forceOption,
              CollHeap *oHeap = CmpCommon::statementHeap())
       : RelExpr(REL_CONTROL_RUNNING_QUERY, NULL, NULL, oHeap),
         queryId_("")
       , pname_("")
       , nid_(-1)
       , pid_(-1)
       , action_(action)
       , forced_(forceOption)
       , qs_(qs)
  {
    switch (qs)
    {
      case ControlQid:
        queryId_ = qid_or_pid; break;
      case ControlPname:
        pname_   = qid_or_pid; break;
      default:
        CMPASSERT(0);
    }
    comment_ = "";
  };

  ControlRunningQuery(int nid, int pid,
              QuerySpec qs,
              Action action,
              ForceOption forceOption,
              CollHeap *oHeap = CmpCommon::statementHeap())
       : RelExpr(REL_CONTROL_RUNNING_QUERY, NULL, NULL, oHeap),
         queryId_("")
       , pname_("")
       , nid_(nid)
       , pid_(pid)
       , action_(action)
       , forced_(forceOption)
       , qs_(qs)
  {
    CMPASSERT(qs == ControlNidPid);
    comment_ = "";
  };

  virtual RelExpr * copyTopNode(RelExpr *derivedNode = NULL,
				CollHeap* outHeap = 0);

  virtual RelExpr * bindNode(BindWA *bindWAPtr);

  virtual PhysicalProperty *synthPhysicalProperty(const Context *context,
                                                  const Lng32     planNumber,
                                                  PlanWorkSpace  *pws);

  // method to do code generation
  virtual short codeGen(Generator*);

  virtual NABoolean isLogical() const;
  virtual NABoolean isPhysical() const;
  virtual Int32 getArity() const;
  virtual const NAString getText() const;
  virtual ExplainTuple *addSpecificExplainInfo(ExplainTupleMaster *, 
                                               ComTdb *, Generator *);

  bool isUserAuthorized(BindWA *bindWA);
  void setComment(NAString &comment);

private:
  NAString queryId_;
  NAString pname_;
  int nid_;
  int pid_;
  Action action_;
  ForceOption forced_;
  QuerySpec qs_;
  NAString comment_;
};

// forward reference
class CountedCSEInfo;

// Container for the common info about a common subexpression. This is
// a helper class for class CommonSubExprRef below.

// This class is stored in the CmpStatement object, but it is defined
// here because it is closely related to the CommonSubExprRef class
// and because it contains ValueIdSet and ValueIdList data members
// which should not be used by CmpStatement classes directly.

// Note about acronyms:
// CTE: Common Table Expression syntax (WITH clause) in SQL
// CSE: Common SubExpression (could be WITH clause or
//      another type of common subexpression)
class CSEInfo : public NABasicObject
{
public:

  enum CSEAnalysisOutcome
    {
      UNKNOWN_ANALYSIS,    // analysis not yet done
      ELIMINATED_IN_BINDER,// single-consumer CSE, eliminated in binder
      EXPAND,              // expand the common subexpression
      CREATE_TEMP,         // materialize CSE as temp, then read the temp
      TEMP,                // read the temp created by someone else
      ERROR                // error occurred, diags are set
      // possible future analysis outcomes:
      // - decide outcome in the optimizer
      // - pipeline results from a single producer to multiple consumers
      //
      // Note: Right now, all consumers of a CSE have the same
      //       outcome; that may change in the future,
      //       e.g. we may have 5 consumers that can use sharing
      //       very well and one that isn't suitable for sharing
      //       (maybe because it reads different data).
    };

  enum CSETempTableType
    {
      UNKNOWN_TEMP_TABLE,  // temp table type not yet determined
      HIVE_TEMP_TABLE,     // use a Hive delimited table
      VOLATILE_TEMP_TABLE  // use a Trafodion volatile table
    };

  CSEInfo(const char *name,
          NAMemory *mem) :
       name_(name, mem),
       cseId_(-1),
       childCSEs_(mem),
       consumers_(mem),
       alternativeConsumers_(mem),
       numLexicalRefs_(0),
       neededColumns_(mem),
       cseTreeKeyColumns_(mem),
       idOfAnalyzingConsumer_(-1),
       analysisOutcome_(UNKNOWN_ANALYSIS),
       tempTableType_(UNKNOWN_TEMP_TABLE),
       tempTableName_(mem),
       tempTableDDL_(mem),
       tempNATable_(NULL),
       insertIntoTemp_(NULL)
  {}

  const NAString &getName() const                            { return name_; }
  Int32 getCSEId() const                                    { return cseId_; }
  const LIST(CountedCSEInfo) &getChildCSEs() const      { return childCSEs_; }
  const CollIndex getNumConsumers() const     { return consumers_.entries(); }
  CommonSubExprRef *getConsumer(CollIndex i) const   { return consumers_[i]; }
  Int32 getNumLexicalRefs() const                  { return numLexicalRefs_; }
  Int32 getTotalNumRefs(Int32 restrictToSingleConsumer = -1) const;

  Int32 getIdOfAnalyzingConsumer() const    { return idOfAnalyzingConsumer_; }
  CSEAnalysisOutcome getAnalysisOutcome(Int32 id) const;
  NABoolean isShared(Int32 id) const
                                   { return analysisOutcome_ == CREATE_TEMP; }
  NABoolean usesATempTable() const         { return insertIntoTemp_ != NULL; }
  CSETempTableType getTempTableType() const         { return tempTableType_; }
  const NABitVector &getNeededColumns() const       { return neededColumns_; }
  const ValueIdSet &getCommonPredicates() const  { return commonPredicates_; }
  const ValueIdSet &getVEGRefsWithDifferingConstants() const
                                    { return vegRefsWithDifferingConstants_; }
  const ValueIdSet &getVEGRefsWithDifferingInputs() const
                                       { return vegRefsWithDifferingInputs_; }
  const NABitVector &getCSETreeKeyColumns() const
                                                { return cseTreeKeyColumns_; }
  const QualifiedName &getTempTableName() const     { return tempTableName_; }
  const NAString &getTempTableDDL() const            { return tempTableDDL_; }
  const NATable *getTempNATable() const               { return tempNATable_; }
  RelExpr *getInsertIntoTemp() const               { return insertIntoTemp_; }

  void setCSEId(Int32 id)                                     { cseId_ = id; }
  Int32 addChildCSE(CSEInfo *childInfo, NABoolean addLexicalRef);
  void addCSERef(CommonSubExprRef *cse);
  void eliminate()               { analysisOutcome_ == ELIMINATED_IN_BINDER; }
  void registerAnAlternativeConsumer(CommonSubExprRef *c)
                                          { alternativeConsumers_.insert(c); }
  void replaceConsumerWithAnAlternative(CommonSubExprRef *c);
  void setIdOfAnalyzingConsumer(Int32 id)     { idOfAnalyzingConsumer_ = id; }
  void setAnalysisOutcome(CSEAnalysisOutcome outcome)
                                               { analysisOutcome_ = outcome; }
  void setTempTableType(CSETempTableType t)            { tempTableType_ = t; }

  void setNeededColumns(const NABitVector &v)          { neededColumns_ = v; }
  void setCommonPredicates(const ValueIdSet &s)     { commonPredicates_ = s; }
  void addCommonPredicates(const ValueIdSet &s)    { commonPredicates_ += s; }
  void addVEGRefsWithDifferingConstants(const ValueIdSet &s)
                                      { vegRefsWithDifferingConstants_ += s; }
  void addVEGRefsWithDifferingInputs(const ValueIdSet &s)
                                         { vegRefsWithDifferingInputs_ += s; }
  void addCSEKeyColumn(CollIndex c)               { cseTreeKeyColumns_ += c; }
  void setTempTableName(const QualifiedName &n)        { tempTableName_ = n; }
  void setTempTableDDL(const char *s)                   { tempTableDDL_ = s; }
  void setTempNATable(const NATable *nat)              { tempNATable_ = nat; }
  void setInsertIntoTemp(RelExpr *r)                   {insertIntoTemp_ = r; }

private:
  // name of the Common Subexpression
  NAString name_;

  // id of this CSE within the statement
  Int32 cseId_;

  // list of other CSEs that are referenced by this one
  LIST(CountedCSEInfo) childCSEs_;

  // list of nodes referring to the common
  // subexpression, their index numbers match
  // the index in this list
  LIST(CommonSubExprRef *) consumers_;

  // We sometimes make copies of a tree, creating alternative
  // consumers. Some notes on these alternative copies: The analyzing
  // consumer (see CommonSubExprRef::analyzeAndPrepareForSharing())
  // and its ancestors are always in the consumers_ list and do not
  // change. As we replace a tree with its copy, we may replace other
  // consumers with their respective copies. The code must be able to
  // deal with this situation, so be careful when making decisions
  // based on a particular consumer obtained from this list.
  LIST(CommonSubExprRef *) alternativeConsumers_;

  // number of lexical refs in the query for this expression
  // (how many time does it appear in the query text)
  Int32 numLexicalRefs_;

  // a common list of columns and predicate to use used for a
  // materialized CSE

  // a list of the actual columns (in terms of the 
  NABitVector neededColumns_;
  ValueIdSet commonPredicates_;

  // VEGies with conflicts between the different
  // consumers
  ValueIdSet vegRefsWithDifferingConstants_;
  ValueIdSet vegRefsWithDifferingInputs_;

  // information for heuristics

  // "key" columns in the child tree, this could include Hive
  // partition columns, "tag" constant columns in a union, and also
  // trailing key columns that may not be significant
  NABitVector cseTreeKeyColumns_;

  // information for the materialization of the CSE
  Int32 idOfAnalyzingConsumer_;
  CSEAnalysisOutcome analysisOutcome_;
  CSETempTableType tempTableType_;
  QualifiedName tempTableName_;
  NAString tempTableDDL_;
  const NATable *tempNATable_;
  RelExpr *insertIntoTemp_;
};

// A CSEInfo and a count (of how many references we have to it)
class CountedCSEInfo
{
public:

  CountedCSEInfo() : info_(NULL), lexicalCount_(-1)                         {}
  CountedCSEInfo(CSEInfo *info, Int32 cnt = 0) :
                                            info_(info), lexicalCount_(cnt) {}
  CountedCSEInfo(const CountedCSEInfo &other) :
                     info_(other.info_), lexicalCount_(other.lexicalCount_) {}
  ~CountedCSEInfo()                                                         {}

  CSEInfo *getInfo() const                                   { return info_; }
  Int32 getLexicalCount() const                      { return lexicalCount_; }

  void incrementLexicalCount()                            { lexicalCount_++; }

private:

  CSEInfo *info_;
  Int32 lexicalCount_;
};

// -----------------------------------------------------------------------
// The CommonSubExprRef class represents a potential common subexpression
// (CSE) in the query tree. The common subexpression has a name, and
// multiple CommonSubExprRef nodes in the tree may refer to the same
// name.  This is a unary operator, the child tree defines the common
// subexpression (with a few caveats, see below). Note that in our
// current design, we keep multiple copies of the common subexpression
// around, in case we don't want to materialize the CSE. This is a
// logical operator, it either needs to be removed or it needs to be
// replaced with a scan of a temp table.
// -----------------------------------------------------------------------
class CommonSubExprRef : public RelExpr
{
public:

  // constructor
  CommonSubExprRef(RelExpr *cse = NULL,
                   const char *internalName = NULL,
                   NAMemory *oHeap = CmpCommon::statementHeap())
       : RelExpr(REL_COMMON_SUBEXPR_REF,cse,NULL,oHeap),
         internalName_(internalName, oHeap),
         id_(-1),
         isAnExpansionOf_(NULL),
         isAnAlternativeOf_(NULL),
         parentCSEId_(-1),
         parentRefId_(-1),
         lexicalRefNumFromParent_(-1),
         hbAccessOptionsFromCTE_(NULL)
  {}

  virtual ~CommonSubExprRef();

  // the name used in the CTE or a generated name
  const NAString &getName() const                    { return internalName_; }
  Int32 getId() const                                          { return id_; }
  Int32 getParentCSEId() const                        { return parentCSEId_; }
  Int32 getParentConsumerId() const                   { return parentRefId_; }
  Int32 getLexicalRefNumFromParent() const {return lexicalRefNumFromParent_; }
  NABoolean isAChildOfTheMainQuery() const;

  virtual Int32 getArity() const;

  // return a read-only reference to the initial list of columns
  const ValueIdList & getColumnList() const            { return columnList_; }
  const ValueIdSet & getNonVEGColumns() const       { return nonVEGColumns_; }

  const ValueIdSet &getPushedPredicates() const  { return pushedPredicates_; }
  const EstLogPropSharedPtr &getEstLogProps() const{ return cseEstLogProps_; }

  void setId(Int32 id)                     { CMPASSERT(id_ == -1); id_ = id; }

  // remember HBase access options (if needed)
  void setHbaseAccessOptions(HbaseAccessOptions *hbo)
                                            { hbAccessOptionsFromCTE_ = hbo; }

  // add this node to the global list of CommonSubExprRefs kept in CmpStatement
  void addToCmpStatement(NABoolean lexicalRef);

  // establish a parent/child relationship between two CommonSubExprRefs
  void addParentRef(CommonSubExprRef *parentRef);

  // is this the first reference to the common subexpression?
  NABoolean isFirstReference() const;

  // CommonSubExprRefs come in different flavors:
  //
  // Lexical ref:     This is a node that got created in a parser
  //                  rule, parsing a reference to a CTE (or, in the
  //                  future, something equivalent, like a view reference).
  // Expanded ref:    Since the parser expands CTE references by making a
  //                  copy on each reference, if that copied tree contains
  //                  child references, an expanded ref is created.
  //                  Lexical and expanded refs shouldn't be treated
  //                  differently, it is somewhat arbitrary which one
  //                  is the lexical one and which ones are expanded.
  // Original ref:    This is the original copy of a lexical or expanded
  //                  ref.
  // Alternative ref: Sometimes we copy a tree after binding, e.g. to
  //                  have a fall-back during the SQO phase. Such copies
  //                  are alternative refs, and only one of them should
  //                  be used in the output of each compilation phase.
  //                  Only one of the alternatives is part of the
  //                  CSEInfo consumer list, the others are stored in
  //                  the alternative consumer list.
  //
  // A given CommonSubExprRef is either a lexical or an expanded ref.
  // The "alternative" flavor is orthogonal to that, both lexical
  // and expanded refs can either be an original or an alternative.
  NABoolean isALexicalRef() const         { return isAnExpansionOf_ == NULL; }
  NABoolean isAnExpandedRef() const       { return isAnExpansionOf_ != NULL; }
  NABoolean isAnOriginalRef() const     { return isAnAlternativeOf_ == NULL; }
  NABoolean isAnAlternativeRef() const  { return isAnAlternativeOf_ != NULL; }
  CommonSubExprRef *getLexicalRef()
                        { return isAnExpansionOf_ ? isAnExpansionOf_ : this; }
  CommonSubExprRef *getOriginalRef()
                    { return isAnAlternativeOf_ ? isAnAlternativeOf_ : this; }

  // a virtual function for performing name binding within the query tree
  virtual RelExpr * bindNode(BindWA *bindWAPtr);

  // normalizer methods
  virtual void transformNode(NormWA & normWARef,
                             ExprGroupId & locationOfPointerToMe);
  virtual void pullUpPreds();
  virtual void pushdownCoveredExpr(
       const ValueIdSet & outputExprOnOperator,
       const ValueIdSet & newExternalInputs,
       ValueIdSet& predOnOperator,
       const ValueIdSet * nonPredNonOutputExprOnOperator = NULL,
       Lng32 childId = (-MAX_REL_ARITY));
  virtual void rewriteNode(NormWA & normWARef);
  virtual RelExpr * semanticQueryOptimizeNode(NormWA & normWARef);
  virtual NABoolean prepareMeForCSESharing(
       const ValueIdSet &outputsToAdd,
       const ValueIdSet &predicatesToRemove,
       const ValueIdSet &commonPredicatesToAdd,
       const ValueIdSet &inputsToRemove,
       ValueIdSet &valuesForVEGRewrite,
       ValueIdSet &keyColumns,
       CSEInfo *info);

  // add all the expressions that are local to this
  // node to an existing list of expressions (used by GUI tool)
  virtual void addLocalExpr(LIST(ExprNode *) &xlist,
			    LIST(NAString) &llist) const;

  virtual HashValue topHash();
  virtual NABoolean duplicateMatch(const RelExpr & other) const;
  virtual RelExpr * copyTopNode(RelExpr *derivedNode = NULL,
				CollHeap* outHeap = 0);

  // synthesize logical properties
  virtual void synthLogProp(NormWA * normWAPtr = NULL);
  virtual void synthEstLogProp(const EstLogPropSharedPtr& inputEstLogProp);

  // get a printable string that identifies the operator
  virtual const NAString getText() const;

  void emitCSEDiagnostics(const char *message,
                          NABoolean forceError = FALSE);

  // for use by the root node for inlining
  static Union *makeUnion(RelExpr *lc, RelExpr *rc, NABoolean blocked);

  static void makeValueIdListFromBitVector(ValueIdList &tgt,
                                           const ValueIdList &src,
                                           const NABitVector &vec);

  // for debugging
  void display();
  static void displayAll(const char *optionalId = NULL);

private:

  // private methods
  // ---------------

  // Methods used in the transformation of a plan with CSEs into
  // one that creates a temp table, populates it, then replaces the
  // CommonSubExprRef nodes with scans of the temp table. These methods
  // are meant to be used in the SQO phase and/or in optimizer rules.

  // - Find a list of columns that satisfies all the CSE consumers,
  // - Find a set of pushed-down predicates common to all consumers
  // - Based on the remaining predicates, determine a STORE BY or
  //   PRIMARY KEY of the temp table
  // - Get an estimate for rows and bytes accessed for each of
  //   the scan nodes
  // - Heuristically decide whether to share the data in a temp table
  CSEInfo::CSEAnalysisOutcome analyzeAndPrepareForSharing(CSEInfo &info);

  // decide which type of temp table to use, store the result in info
  void determineTempTableType(CSEInfo &info);

  // Based on the information obtained in analyzeSharing(), create
  // a volatile table to hold the result of the common subexpression.
  // Note that this table is created at compile time. If the user
  // compiled this statement in a user-defined transaction, the
  // temp table will be created in that transaction. If the user was
  // not in a transaction when this statement was compiled, we
  // will use a separate transaction that will be committed before
  // returning back to the user.
  NABoolean createTempTable(CSEInfo &info);

  RelExpr * createInsertIntoTemp(CSEInfo &info, NormWA & normWARef);

  // Create a scan on the temp table, which can replace this
  // CommonSubExprRef node when we choose to materialize the common
  // subexpression
  RelExpr * createTempScan(CSEInfo &info, NormWA & normWARef);
  RelExpr * getTempScan() const                           { return tempScan_; }

  // data members
  // ------------

  // The name of the CTE (Common Table Expression) in internal
  // format, if the operator was generated from a reference to
  // a CTE. Otherwise, this could be some made-up name.
  NAString internalName_;

  // One or more CommonSubExprRef operators may refer to the same
  // common subexprssion. These references are numbered 0, 1, ...
  Int32 id_;

  // indicate different flavors of CommonSubExprRef nodes and point
  // back to the original node(s), if any
  CommonSubExprRef *isAnExpansionOf_;
  CommonSubExprRef *isAnAlternativeOf_;

  // There are three ids that describe our predecessor in the
  // RelExpr tree and in the lexical directed multigraph of the
  // CSEs:
  // Parent CSE id:
  //     This is the the parent CSE or main query that directly
  //     contains the reference. In the RelExpr tree, this is our
  //     closest ancestor CommonSubExprRef node or the root node
  //     of the tree. It is stored as the integer CSE id here,
  //     we could also store the name. A special id is used for
  //     the main query: CmpStatement::getCSEIdForMainQuery().
  //
  // Parent Ref id:
  //     This is the consumer id of the parent CommonSubExprRef
  //     (or 0 if the ref originates from the main query).
  //
  // Lexical ref num from parent:
  //     This indicates which reference from the parent CSE we
  //     are looking at. The main query or a CSE may refer to
  //     the same child multiple times, and this is the number
  //     indicating this (0...n-1). The id_ data member counts
  //     the total number of references to a CSE; the lexical
  //     ref num from parent counts only the lexical number
  //     of references and only from a particular parent CSE
  //     (or the main query).
  //
  Int32 parentCSEId_;
  Int32 parentRefId_;
  Int32 lexicalRefNumFromParent_;

  // The list of columns produced by the common subexpression.
  // We keep the full list here, even when the characteristic
  // outputs get reduced during the normalization phase. This
  // is a list, since different consumers of the CSE will use
  // different value ids, so the only way to equate columns
  // of different consumers is by position in this list.
  ValueIdList columnList_;

  // Same columns without making VEGRefs. This is needed
  // in preCodeGen. It also includes other potential VEG
  // members if this is the analyzing consumer.
  ValueIdSet nonVEGColumns_;

  // The common inputs (typically parameters). Pushing down
  // new predicates may create additional characteristic
  // inputs for this RelExpr, those are not reflected here.
  // These common inputs should have the same value ids for
  // all consumers.
  ValueIdSet commonInputs_;

  // Predicates that got pulled up during normalization.
  // Usually those should get pushed down again.
  // ValueIdSet pulledPredicates_;

  // Predicates that got pushed into the child tree. These
  // modify our copy of the common subexpression. If we want
  // to use a temp table, we may need to pull out those predicates
  // that are not common between all the users of the CSE
  ValueIdSet pushedPredicates_;

  // Predicates that are not common predicates, pushed into the
  // common temp table. These need to be applied to the temp
  // table, if we decide to read from a temp table.
  ValueIdSet nonSharedPredicates_;

  // We allow hints and access options on references to CTEs.
  // Those are ignored when we expand the CTE, but they can
  // be applied to the scan of the temp node, if we decide to
  // create a temp table for the resulting CSE.
  HbaseAccessOptions *hbAccessOptionsFromCTE_;

  // the estimated logical properties of this common subexpression,
  // set in the analyzing consumer only, since they should be the same
  // for all consumers
  EstLogPropSharedPtr cseEstLogProps_;

  // the temp scan to replace this node after the SQO phase
  RelExpr *tempScan_;

}; // class CommonSubExprRef


#endif /* RELMISC_H */
