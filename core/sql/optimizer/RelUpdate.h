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
#ifndef RELUPDATE_H
#define RELUPDATE_H
/* -*-C++-*-
******************************************************************************
*
* File:         RelUpdate.h
* Description:  Relational insert, update, delete, and their generic
*               superclass (both physical and logical operators)
* Created:      4/28/94
* Language:     C++
*
*
*
*
******************************************************************************
*/


#include "ComTransInfo.h"
#include "OptUtilIncludes.h"
#include "ObjectNames.h"
#include "RelExpr.h"
#include "SearchKey.h"
#include "RelJoin.h"
#include "RelScan.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------

// The following are logical operators
class GenericUpdate;
class Insert;
class LeafInsert;
class Update;
class MergeUpdate;
class Delete;
class MergeDelete;
class LeafDelete;
class HbaseDelete;
class HiveInsert;
class HbaseInsert;
class HBaseBulkLoadPrep;


// The following are physical operators
class InsertCursor;
class UpdateCursor;
class HbaseUpdate;
class DeleteCursor;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
class BindWA;
class ColReference;
class LogicalProperty;
class ModifiedFieldMap;
class NormWA;
class TableDesc;
class TriggerList;
class UpdateColumns;
class TriggersTempTable;
class BeforeAndAfterTriggers;
class MVInfoForDML;

// -----------------------------------------------------------------------
// generic update operation (insert, update, or delete)
// -----------------------------------------------------------------------

class GenericUpdate : public RelExpr
{
public:

  enum RowsAffected { DEFAULT_ROWSAFFECTED,             // indeterminate/off
                      COMPUTE_ROWSAFFECTED,             // definitely compute
                      DO_NOT_COMPUTE_ROWSAFFECTED };    // ruled out/off

  enum NoLogType { NORMAL_LOGGING = FALSE,
		   CONSISTENT_NOLOG,
		   INCONSISTENT_NOLOG };

 
GenericUpdate(const CorrName &name, 
                TableDesc *tabId,
                OperatorTypeEnum otype,
                RelExpr *child,
		ItemExpr *newRecExpr,
		ItemExpr *currOfCursorName,
		CollHeap *oHeap)

  : RelExpr(otype,child,NULL,oHeap),
    updatedTableName_(name, oHeap),
    tabId_(tabId),
    executorPredTree_(NULL),
    newRecExprTree_(newRecExpr),
    scanIndexDesc_(NULL),
    indexDesc_(NULL),
    indexNumberArray_(oHeap),
    indexNewRecExprArrays_(oHeap),
    indexBeginKeyPredArray_(oHeap),
    indexEndKeyPredArray_(oHeap),
    pathKeys_(NULL),
    partKeys_(NULL),
    currOfCursorName_(currOfCursorName),
    rowsAffected_(DEFAULT_ROWSAFFECTED),
    stoi_(NULL),
    isNoLogOperation_(NORMAL_LOGGING),
    noFlow_(FALSE),
    avoidHalloweenR2_(0),
    avoidHalloween_(FALSE),
    halloweenCannotUseDP2Locks_(FALSE),
    mtsStatement_(FALSE),
    noRollback_(FALSE),
    isMergeUpdate_(FALSE),
    isMergeDelete_(FALSE),
    subqInUpdateAssign_(NULL),
    updateCKorUniqueIndexKey_(FALSE),
    isIgnoreTriggers_(FALSE),
    hbaseOper_(FALSE),
    uniqueHbaseOper_(FALSE),
    cursorHbaseOper_(FALSE),
    uniqueRowsetHbaseOper_(FALSE),
    canDoCheckAndUpdel_(FALSE),
    noDTMxn_(FALSE),
    useRegionXn_(FALSE),
    noCheck_(FALSE),
    noIMneeded_(FALSE),
    useMVCC_(FALSE),
    useSSCC_(FALSE),
    preconditionTree_(NULL)
  {}

  // copy ctor
  GenericUpdate (const GenericUpdate & orig, CollHeap * h=0) ; // not written

  // virtual destructor
  virtual ~GenericUpdate();

  // append an ascii-version of GenericUpdate into cachewa.qryText_
  virtual void generateCacheKey(CacheWA& cwa) const;

  // is this entire expression cacheable after this phase?
  virtual NABoolean isCacheableExpr(CacheWA& cwa);

  // change literals of a cacheable query into ConstantParameters
  virtual RelExpr* normalizeForCache(CacheWA& cwa, BindWA& bindWA);

  // get the degree of this node (it is a unary op).
  virtual Int32 getArity() const;

  // accessor and mutator functions
  const CorrName &getTableName() const         { return updatedTableName_; }
  CorrName &getTableName()                     { return updatedTableName_; }
  const CorrName *getPtrToTableName() const   { return &updatedTableName_; }
  TableDesc     *getTableDesc() const                     { return tabId_; }
  void          setTableDesc(TableDesc *newId)           { tabId_ = newId; }
  const         IndexDesc *getIndexDesc() const       { return indexDesc_; }
  void          setIndexDesc(const IndexDesc *id)       { indexDesc_ = id; }

  // get and set the update/key expression
  ItemExpr      *&recExprTree()
                { return newRecExprTree_; }
  void          addNewRecExprTree(ItemExpr *expr);
  ItemExpr      *removeNewRecExprTree();

  ValueIdSet    &newRecExpr()                   { return newRecExpr_; }
  ValueIdArray  &newRecExprArray()                  { return newRecExprArray_; }

  // QSTUFF
  ValueIdSet    &newRecBeforeExpr()             { return newRecBeforeExpr_; }
  ValueIdArray  &newRecBeforeExprArray()        { return newRecBeforeExprArray_; }
  // QSTUFF

  void          addKeyExprTree(ItemExpr *expr);
  ItemExpr      *removeKeyExprTree();

  ValueIdList &beginKeyPred()                    { return beginKeyPred_; }
  ValueIdList &endKeyPred()                      { return endKeyPred_;   }

  const ValueIdList &getBeginKeyPred() const     { return beginKeyPred_; }
  const ValueIdList &getEndKeyPred() const         { return endKeyPred_; }

  ValueIdSet &executorPred()                     { return executorPred_; }
  ItemExpr *getExecutorPredTree()            { return executorPredTree_; }
  void setExecutorPredTree(ItemExpr *ept)     { executorPredTree_ = ept; }

  SearchKey *getSearchKey() const                    { return pathKeys_; }
  SearchKey *getPartKey() const                      { return partKeys_; }
  void setSearchKey(SearchKey *pathKey)           { pathKeys_ = pathKey; }
  void setPartKey(SearchKey *partKey)             { partKeys_ = partKey; }

  ValueIdSet    &usedColumns()                    { return usedColumns_; }

  NABoolean isMerge() const { return (isMergeUpdate_ || isMergeDelete_); }

  NABoolean isMergeUpdate() const { return isMergeUpdate_; }
  void setIsMergeUpdate(NABoolean v) { isMergeUpdate_ = v; }

  NABoolean isMergeDelete() const { return isMergeDelete_; }
  void setIsMergeDelete(NABoolean v) { isMergeDelete_ = v; }

  // MVs --
  // was the NOLOG option specified?
  void setNoLogOperation(NABoolean isConsistent = FALSE)
  { isNoLogOperation_ = isConsistent ? CONSISTENT_NOLOG : INCONSISTENT_NOLOG; }
  NoLogType isNoLogOperation() const	     { return isNoLogOperation_; }
  void setNoLogOp(NoLogType val)
  { isNoLogOperation_ = val; }

  // triggers
  void setIgnoreTriggers (NABoolean ignoreTriggers = FALSE)
     {isIgnoreTriggers_ = ignoreTriggers; }
  NABoolean isIgnoreTriggers() const         { return isIgnoreTriggers_; }

  // Accessor for the noFlow flag.
  NABoolean getNoFlow() const { return noFlow_; }
  // Mutator for the noFlow flag.
  void setNoFlow(NABoolean noFlow)
    { noFlow_ = noFlow;  }

  //MTS - Multi Transaction Support
  NABoolean isMtsStatement() const
                         { return mtsStatement_; }
  void setMtsStatement(NABoolean lastRow)
                         { mtsStatement_ = lastRow; }

  //No rollback - Transaction Type support
  NABoolean isNoRollback() const
                         { return noRollback_; }
  void setNoRollbackOperation(NABoolean flag = TRUE)
                         { noRollback_ = flag; }

  NABoolean getUpdateCKorUniqueIndexKey() const
                         { return updateCKorUniqueIndexKey_; }
  void setUpdateCKorUniqueIndexKey(NABoolean val)
                         { updateCKorUniqueIndexKey_ = val; }

  // Index maintenance helpers
  //
  // Records mapping between index and index number used by binder -- needed
  // to index into the various other index arrays in the generator.
  // ##IM: IM_GENERATOR scheme only, to be REMOVED soon!
  //
  ARRAY(IndexDesc *) &indexNumberArray()     { return indexNumberArray_; }

  // Set of assign nodes for each index -- used for insert.
  //
  ARRAY(ValueIdArray)  &indexNewRecExprArrays()
                                        { return indexNewRecExprArrays_; }

  // Expressions for begin and end key for each index
  //
  ARRAY(ValueIdList) &indexBeginKeyPredArray()
                                       { return indexBeginKeyPredArray_; }
  ARRAY(ValueIdList) &indexEndKeyPredArray()
                                         { return indexEndKeyPredArray_; }

  void setNoIMneeded(NABoolean v)
  {
    noIMneeded_ = v;
  }
  NABoolean noIMneeded()
  {
    return noIMneeded_;
  }

  // The generator needs a reference to the original scan columns
  // ##IM: not needed any more (?) -- REMOVE the scanIndexDesc_ member! (?)
  //
  const IndexDesc *getScanIndexDesc() const     { return scanIndexDesc_; }
  void setScanIndexDesc(const IndexDesc *id)      { scanIndexDesc_ = id; }

  // Methods that operate on the check constraints.
  const ValueIdList &getCheckConstraints() const
                                             { return checkConstraints_; }
  ValueIdList &checkConstraints()            { return checkConstraints_; }

  // Initialize the characteristic outputs
  void initCharacteristicOutputs() const;

  // a method used for recomputing the outer references (external dataflow
  // input values) that are still referenced by each operator in the
  // subquery tree after the predicate pull up is complete.
  // should never be called on an update
  virtual void recomputeOuterReferences();

  // The set of values that I can potentially produce as output.
  virtual void getPotentialOutputValues(ValueIdSet &vs) const;
  void setPotentialOutputValues(const ValueIdSet &vs)
                                        { potentialOutputs_ = vs; }
  void setPotentialOutputValues(const ValueIdList &vl)
                                        { potentialOutputs_.clear();
                                          potentialOutputs_.insertList(vl);
                                        }
  void addPotentialOutputValues(const ValueIdList &vl)
                                        {
                                          potentialOutputs_.insertList(vl);
                                        }
  //++MV - returns the GenericUpdateOutputFunction's that are in the
  // characteristic outputs
  NABoolean getOutputFunctionsForMV(ValueId &epochValueId, 
                                    OperatorTypeEnum opType) const;

  NABoolean producesOutputs() const     { return !potentialOutputs_.isEmpty(); }

  virtual void getInputValuesFromParentAndChildren(ValueIdSet &vs) const;

  // a virtual function for performing name binding within the query tree
  RelExpr *bindNode(BindWA *bindWA);

  // Index Maintenance in the Binder
  RelExpr *createIMTree(BindWA *bindWA,
                        UpdateColumns *updatedColumns,
                        NABoolean useInternalSyskey);
  RelExpr *createIMNodes(BindWA *bindWA,
                         NABoolean useInternalSyskey,
                         IndexDesc *index,
                         const ValueId &mergeIUDIndicator);
  RelExpr *createUndoTree(BindWA *bindWA,
                          UpdateColumns *updatedColumns,
                          NABoolean useInternalSyskey,
                          NABoolean isImOrRiPresent,
                          NABoolean isOrMvPresent,
                          TriggersTempTable *tempTableObj);
  RelExpr *createUndoNodes(BindWA *bindWA,
                           NABoolean useInternalSyskey,
                           IndexDesc *index);
  
  RelExpr *createUndoIUDLog(BindWA *bindWA) ;
  RelExpr *createUndoTempTable(TriggersTempTable *tempTableObj,
                               BindWA *bindWA);
  void nonvirtual_placeholder_func1();
  void nonvirtual_placeholder_func2();

  // Each operator supports a (virtual) method for transforming its
  // scalar expressions to a canonical form
  virtual void transformNode(NormWA &normWARef,
                             ExprGroupId &locationOfPointerToMe);

  // Each operator supports a (virtual) method for rewriting its
  // value expressions.
  virtual void rewriteNode(NormWA &normWARef);
  
  virtual void pushdownCoveredExpr
			  (const ValueIdSet & outputExprOnOperator,
                           const ValueIdSet & newExternalInputs,
                           ValueIdSet & predicatesOnParent,
			   const ValueIdSet * nonPredExprOnOperator = NULL,
			   Lng32 childIndex = (-MAX_REL_ARITY));

  virtual RelExpr *preCodeGen(Generator * generator,
                              const ValueIdSet & externalInputs,
                              ValueIdSet &pulledNewInputs);

  // -----------------------------------------------------
  // generate CONTROL QUERY SHAPE fragment for this node.
  // -----------------------------------------------------
  virtual short generateShape(CollHeap * space, char * buf, NAString * shapeStr = NULL);

  // compute the columns that are actually referenced by the operation
  void computeUsedCols();

  virtual HashValue topHash();
  virtual NABoolean duplicateMatch(const RelExpr &other) const;
  virtual RelExpr *copyTopNode(RelExpr *derivedNode = NULL,
			       CollHeap *outHeap = CmpCommon::statementHeap());

  // synthesize logical properties
  virtual void synthLogProp(NormWA * normWAPtr = NULL);

  // finish synthesis of logical properties by setting the cardinality
  // attributes
  virtual void finishSynthEstLogProp();

  virtual void synthEstLogProp(const EstLogPropSharedPtr& inputEstLogProp);

  // Generate the write access set.
  virtual SubTreeAccessSet * calcAccessSets(CollHeap *heap);

  // add all the expressions that are local to this
  // node to an existing list of expressions (used by GUI tool)
  virtual void addLocalExpr(LIST(ExprNode *) &xlist,
                            LIST(NAString) &llist) const;

  // get a printable string that describes this operator
  virtual const NAString getText() const;
  const NAString getUpdTableNameText() const;

  StmtLevelAccessOptions &accessOptions()       { return accessOptions_; }

  // adds information about this node to the Explain tree
  virtual ExplainTuple *addSpecificExplainInfo(ExplainTupleMaster *explainTuple,
					      ComTdb * tdb,
					      Generator *generator);

  short addSpecificExplainInfo(ExplainTupleMaster *explainTuple,
			       ComTdb * tdb,
			       Generator *generator,
			       NAString &description);


  NABoolean updateCurrentOf();
  ItemExpr *&currOfCursorName()                 { return currOfCursorName_; }

  RowsAffected &rowsAffected()                  { return rowsAffected_; }
  void finalizeRowsAffected(const GenericUpdate *before)
  {
    rowsAffected_ = (before->rowsAffected_ == DO_NOT_COMPUTE_ROWSAFFECTED)
                    ? DO_NOT_COMPUTE_ROWSAFFECTED : COMPUTE_ROWSAFFECTED;
    // (thus a before value of DEFAULT_ROWSAFFECTED becomes COMPUTE_ ...)
  }

  OptSqlTableOpenInfo *&getRefToStoi()             { return stoi_; }
  OptSqlTableOpenInfo *getOptStoi() const             { return stoi_; }
  void setOptStoi(OptSqlTableOpenInfo *stoi)          { stoi_ = stoi; }

  // access to value id map between updated and selected columns
  ValueIdMap &updateToSelectMap()  { return updateToSelectMap_; }

  virtual NABoolean okToAttemptESPParallelism (
            const Context* myContext, /*IN*/
            PlanWorkSpace* pws, /*IN*/
            Lng32& numOfESPs, /*OUT*/
            float& allowedDeviation, /*OUT*/
            NABoolean& numOfESPsForced /*OUT*/) ;


  // QSTUFF
  // --------------------------------------------------------------------
  // This routine checks whether a table is both read and updated
  // --------------------------------------------------------------------
  virtual rwErrorStatus checkReadWriteConflicts(NormWA & normWARef);

    // access to value id map between updated and selected columns
  ValueIdMap &oldToNewMap()  { return oldToNewMap_; }

  // this method binds both, the set clauses applied to the after
  // image as well as the set clauses applied to the before image
  // the new set on rollback clause allows an application to modify
  // the before image.
  // delete from tab set on rollback x = 1;
  // update tab set x = 1 set on rollback x = 2;

  void bindUpdateExpr(BindWA	    *bindWA,
                      ItemExpr	    *recExpr,
                      ItemExprList  &RecExpr,
                                          RelExpr       *boundView,
                                          Scan          *scanNode,
                                          SET(short)    &stoiColumnSet,
                                          NABoolean     onRollback = FALSE);

  ItemExpr * subqInUpdateAssign() { return subqInUpdateAssign_; }
  void setSubqInUpdateAssign(ItemExpr * re) { subqInUpdateAssign_ = re; }

  ValueIdSet    &mergeInsertRecExpr()       { return mergeInsertRecExpr_; }
  ValueIdArray  &mergeInsertRecExprArray()  { return mergeInsertRecExprArray_;}
  ValueIdSet    &mergeUpdatePred()          { return mergeUpdatePred_; }

  // QSTUFF
  void setTransactionRequired(Generator *generator,
                              NABoolean isNeededForAllFragments = FALSE);

  // Handle the inlining of Triggers, RI and IM.
  RelExpr *handleInlining(BindWA *bindWA, RelExpr *boundExpr);

  // Returns TRUE if need for logging or marking inconsistent.
  NABoolean isMvLoggingRequired();

  virtual PlanPriority computeOperatorPriority
    (const Context* context,
     PlanWorkSpace *pws=NULL,
     Lng32 planNumber=0);

  virtual NABoolean computeRowsAffected() const;

  inline NABoolean avoidHalloween() const
  { return avoidHalloween_; }

  inline void setAvoidHalloween(NABoolean h)
  { avoidHalloween_ = h; }

  inline void setHalloweenCannotUseDP2Locks(NABoolean h)
  { halloweenCannotUseDP2Locks_ = h; }

  inline NABoolean getHalloweenCannotUseDP2Locks() const
  { return halloweenCannotUseDP2Locks_; }

  // Following methods compatible with Neo R2.
  inline NABoolean avoidHalloweenR2() const 
  { return avoidHalloweenR2_ > 0; }

  inline void setAvoidHalloweenR2(UInt32 numHalloweenR2)
  { avoidHalloweenR2_ = numHalloweenR2; }

  inline void resetAvoidHalloweenR2() 
  {
    avoidHalloweenR2_--;
    CMPASSERT(avoidHalloweenR2_ != 0);
  }

  void configTSJforHalloween( Join* j, OperatorTypeEnum o, CostScalar c);

  // Override the RelExpr method 
  virtual RelExpr *normalizeNode(NormWA & normWARef);

  NABoolean checkForMergeRestrictions(BindWA *bindWA);

  //////////////////////////////////////////////////////
  virtual NABoolean pilotAnalysis(QueryAnalysis* qa);
  //////////////////////////////////////////////////////

  NABoolean &hbaseOper() { return hbaseOper_; }
  NABoolean &uniqueHbaseOper() { return uniqueHbaseOper_; }
  NABoolean &cursorHbaseOper() { return cursorHbaseOper_; }
  NABoolean &uniqueRowsetHbaseOper() { return uniqueRowsetHbaseOper_; }
  const NABoolean &uniqueRowsetHbaseOper() const { return uniqueRowsetHbaseOper_; }
  NABoolean &canDoCheckAndUpdel() { return canDoCheckAndUpdel_; }

  NABoolean &noDTMxn() { return noDTMxn_; }
  NABoolean &useRegionXn() { return useRegionXn_; }

  NABoolean noCheck() { return noCheck_; }
  void setNoCheck(NABoolean v) { noCheck_ = v; }

  NABoolean &useMVCC() { return useMVCC_; }

  NABoolean &useSSCC() { return useSSCC_; }
 
  NABoolean useMVCCorSSCC() { if (useSSCC_ || useMVCC_)
                                return TRUE;
                               else
                                 return FALSE; 
                            }

  inline ItemExpr * getPreconditionTree() const { return preconditionTree_; }
  inline const ValueIdSet getPrecondition() const { return precondition_; }
  inline void setPreconditionTree(ItemExpr *pc) { preconditionTree_ = pc; }
  inline void setPrecondition(const ValueIdSet pc)
           { precondition_ = pc; exprsInDerivedClasses_ += precondition_; }
  inline const ValueId & getProducedMergeIUDIndicator() const
                                     { return producedMergeIUDIndicator_; }
  inline void setProducedMergeIUDIndicator(const ValueId &v)
                                        { producedMergeIUDIndicator_ = v; }
  inline const ValueId & getReferencedMergeIUDIndicator() const
                                   { return referencedMergeIUDIndicator_; }
  inline void setReferencedMergeIUDIndicator(const ValueId &v)
                                      { referencedMergeIUDIndicator_ = v; }

  virtual ItemExpr * insertValues() { return NULL;}

protected:

  // Here, derived classes can register expressions that are used by
  // them, so that the computation of characteristic inputs can make
  // sure these expressions get the inputs they need.
  ValueIdSet exprsInDerivedClasses_;
  
private:

  // ---------------------------------------------------------------------
  // Triggers --
  // Methods for inlining triggers, RI and IM.
  // ---------------------------------------------------------------------

  // This method is better known as setRETDescForTSJTree()
  RETDesc * createOldAndNewCorrelationNames(BindWA *bindWA,
                                       NABoolean createRETDescOnly=FALSE);


  // helper function to method handleInlining use to check some currently
  // unsupported uses of triggers
  NABoolean checkNonSupportedTriggersUse(BindWA *bindWA, QualifiedName &subjectTable,
                                         ComOperation op, BeforeAndAfterTriggers *allTriggers);

  // Add a "virtual" column to this GenericUpdate node.
  ValueId addVirtualColumn(BindWA *bindWA, ItemExpr *colExpr, const char *colName, CollHeap *heap);

  // Inline IndexMaintainance.
  NABoolean isIMNeeded(UpdateColumns *updatedColumns);
  RelExpr *inlineIM(RelExpr *topNode,
		    BindWA *bindWA,
		    NABoolean isLastTSJ,
		    UpdateColumns *updatedColumns,
		    CollHeap *heap,
		    NABoolean useInternalSyskey,
		    NABoolean rowTriggersPresent);

  // Handle inlining of RI, row triggers and the temp insert.
  RelExpr *inlinePipelinedActions(RelExpr *topNode, BindWA *bindWA,
				  TriggerList *rowTriggers,
				  RefConstraintList *riList,
				  CollHeap *heap);

  // Create the sub-tree that inserts the affected set into the temporary table.
  RelExpr *inlineTempInsert(RelExpr *topNode,
			    BindWA *bindWA,
			    TriggersTempTable& tempTableObj,
			    NABoolean isDrivenByBeforeTriggers,
			    NABoolean isTopMostTSJ,
			    CollHeap *heap);

  // Create the sub-tree that deletes the affected set from the temp table.
  RelExpr *inlineTempDelete(BindWA *bindWA, RelExpr *topNode, TriggersTempTable& tempTableObj, CollHeap *heap);

  // Fix the NEW@ cols according to Update/Insert expressions.
  void fixTentativeRETDesc(BindWA *bindWA, CollHeap *heap);

  // For Update nodes, add the predicate: OLD@.<ci> = NEW@.<ci>
  void addPredicateOnClusteringKey(BindWA *bindWA, RelExpr *tentativeNode, CollHeap *heap);

  // This method replaces the GenericUpdate node with a temporary Rename node
  // that functions as the tentative execution node, and goes away during Transfomation.
  // The actual changes to the subject table are done elsewhere (see inlineEffectiveGU() below).
  RelExpr *createTentativeGU(BindWA *bindWA, CollHeap *heap);

  // Create the left sub-tree of the inlining backbone, when before triggers exist.
  RelExpr *createTentativeSubTree(BindWA *bindWA, TriggerList *triggers, UpdateColumns *updatedColumns, TriggersTempTable& tempTableObj, CollHeap *heap);

  // Inline the GenericUpdate node that affects the base table following
  // the inlining of before triggers. The first method is a pure virtual method
  // implemented by the sub-classes: Insert, Delete and Update.
  // The second method uses the first method in building the EffectiveGU sub-tree.
  virtual RelExpr *createEffectiveGU(BindWA *bindWA,
                                     CollHeap *heap,
				     TriggersTempTable& tempTableObj,
				     GenericUpdate **effectiveGUNode,
				     UpdateColumns *colsToSet=NULL);

  // When the binding gets to the effective GU, don't inline triggers
  // recursively, but generate the NEW@ and OLD@ values if needed.
  RelExpr *bindEffectiveGU(BindWA *bindWA);

  void removeRedundantInputsFromTempInsertTree(BindWA  *bindWA,
                                               RelExpr *tentativeSubtree);

  RelExpr *inlineAfterOnlyBackbone(BindWA *bindWA,
				   TriggersTempTable& tempTableObj,
				   TriggerList *rowTriggers,
				   TriggerList *stmtTriggers,
				   RefConstraintList *riConstraints,
				   NABoolean needIM,
                                   NABoolean isMVLoggingRequired,
  				   UpdateColumns *updatedColumns,
				   CollHeap *heap);
  RelExpr *inlineAfterOnlyBackboneForUndo(BindWA *bindWA,
				   TriggersTempTable& tempTableObj,
				   TriggerList *rowTriggers,
				   TriggerList *stmtTriggers,
				   RefConstraintList *riConstraints,
				   NABoolean needIM,
                                   NABoolean isMVLoggingRequired,
  				   UpdateColumns *updatedColumns,
				   CollHeap *heap);

  RelExpr *inlineBeforeAndAfterBackbone(BindWA *bindWA,
					RelExpr *tentativeSubtree,
					TriggersTempTable& tempTableObj,
					TriggerList *rowTriggers,
					TriggerList *stmtTriggers,
					RefConstraintList *riConstraints,
					NABoolean needIM,
                                        NABoolean isMVLoggingRequired,
					UpdateColumns *updatedColumns,
					CollHeap *heap);

  // Is this backbone cascaded from a row after trigger?
  NABoolean shouldForbidMaterializeNodeHere(BindWA *bindWA);

  // Private functions RI support.
  RelExpr *createRISubtree(BindWA *bindWA,
			   const NATable *naTable,
			   const RefConstraint& refConstraint,
			   CollHeap *heap);

  // Inline Referential Integrity constraints
  RelExpr *inlineRI (BindWA *bindWA,
		     const RefConstraintList *refConstraints,
		     CollHeap *heap);

  // Get the list of referential integrity constraints.
  RefConstraintList *getRIs(BindWA *bindWA, const NATable *naTable);

  // Add to the RETDesc the virtual columns needed for MV logging.
  void prepareForMvLogging(BindWA *bindWA,
			   CollHeap *heap);

  // Create the subtree for inserting rows into the MV log table.
  RelExpr *createMvLogInsert(BindWA *bindWA,
			     CollHeap *heap,
			     UpdateColumns *updatedColumns,
			     NABoolean projectMidRangeRows);

  RelExpr *inlineOnlyRIandIMandMVLogging(BindWA *bindWA,
					 RelExpr *topNode,
					 NABoolean needIM,
					 RefConstraintList *riConstraints,
					 NABoolean isMVLoggingRequired,
					 UpdateColumns *columns,
					 CollHeap *heap);

  // Finish the inlining procedure.
  void InliningFinale(BindWA *bindWA, RelExpr *topNode, RETDesc *origRETDesc);


  // MV
  // Add all the ON STATEMENT MVs that should be refreshed as a result of the
  // current IUD action to the list of triggers
  BeforeAndAfterTriggers *getTriggeredMvs(BindWA *bindWA,
					  BeforeAndAfterTriggers *list,
					  UpdateColumns *columns);

  // MV
  // The actual insertion of the ON STATEMENT MVs to the list of triggers.
  // This method is NOT implemented for GenericUpdate, but do implemented
  // for the derived classes (Insert Update and Delete).
  virtual void insertMvToTriggerList(BeforeAndAfterTriggers *list,
				     BindWA *bindWA,
				     CollHeap *heap,
				     const QualifiedName &mvName,
				     MVInfoForDML *mvInfo,
				     const QualifiedName &subjectTable,
				     UpdateColumns *updateCols = NULL);

  // If we are reading from the same table we are updating, check to
  // see if we can support this potential halloween problem
  //
  NABoolean checkForHalloweenR2(Int32 numScansToFind);

  NABoolean checkForNotAtomicStatement(BindWA *bindWA, Lng32 sqlcode, NAString objname, NAString tabname);

  // name of the table affected by the operation
  CorrName updatedTableName_;

  // a unique identifer for the updated table
  TableDesc *tabId_;

  // a value id map to map key columns from the above TableDesc to
  // the key columns of the select part of the update
  ValueIdMap updateToSelectMap_;

  // the index descriptor of the updated table (for physical nodes only)
  const IndexDesc *indexDesc_;

  // Expressions to compute the inserted/updated record from the child record.
  //
  // The newRecExpr_ is assumed to be a set of Assign item expressions.
  // It is used for insert and update operations, by Optimizer.
  // It contains values for columns to insert/update.
  //
  // The newRecExprArray_ is an array of Assigns ordered by column position:
  // used for insert and update, by Generator.
  // For an Insert, it will contain Assigns for *all* columns of the target
  // (if not all cols were specified in the insert stmt, the binder will
  // have filled in default vals).  Also, in certain cases, it contains
  // Assigns for syskey columns -- see Binder for details.
  //
  // The usedColumns_ set indicates which columns are needed to
  // perform the operation, this is mainly to support physical operations.
  ItemExpr     *newRecExprTree_;
  ValueIdSet    newRecExpr_;
  ValueIdArray  newRecExprArray_;
  ValueIdSet    usedColumns_;

  // QSTUFF
  ValueIdSet    newRecBeforeExpr_;
  ValueIdArray  newRecBeforeExprArray_;
  ValueIdSet    usedBeforeColumns_;
  // QSTUFF

  ValueIdSet    potentialOutputs_;

  // Index Maintenance in the Generator (old scheme)  ##IM: TO BE REMOVED...
  //
  // Vector Mapping indexDesc to index number -- this is used to index into
  // the various arrays of expressions related to the indexes.
  // ##IM: IM_GENERATOR scheme only, to be REMOVED soon!
  //
  ARRAY(IndexDesc *) indexNumberArray_;

  // The ValueIdSets for the indexes
  //
  ARRAY(ValueIdArray) indexNewRecExprArrays_;

  // Begin and End Key Predicates for the indexes
  //
  ARRAY(ValueIdList) indexBeginKeyPredArray_;
  ARRAY(ValueIdList) indexEndKeyPredArray_;

  // The descriptor for the original scan table.
  //
  const IndexDesc *scanIndexDesc_;      // ##IM: REMOVE?

  // Begin and end key expression trees. Both are used for subset and
  // range update/deletes.
  // For inserts and unique updates/deletes, only beginKeyExpr_ is set.
  ValueIdList beginKeyPred_;
  ValueIdList endKeyPred_;

  // Key objects for the clustering key and the partitioning key
  SearchKey  *pathKeys_;
  SearchKey  *partKeys_;

  // executor predicates
  ValueIdSet executorPred_;
  ItemExpr  *executorPredTree_;

  // SQL/MP check constraints on the table to be updated.
  ValueIdList checkConstraints_;

  // Contains user specified STABLE or REPEATABLE access type
  StmtLevelAccessOptions accessOptions_;

  // The next field containing information to process the 'where current of'
  // clause in an update/delete statement.

  // Name of the cursor or a hostvar which will contain the cursor
  // name at runtime. Set to a non-null value if the 'where current of'
  // clause is specified in the update/delete stmt.
  ItemExpr  *currOfCursorName_;

  // This flag, if COMPUTE_ROWSAFFECTED, indicates that affected rows
  // (inserted, updated, or deleted) are to be computed for this node
  // at runtime.  A flag is needed so that we compute the rows
  // affected for base table ins/upd/del only and not for indexes.
  RowsAffected rowsAffected_;

  // tables open information which is necessary during the open of a sql table.
  OptSqlTableOpenInfo *stoi_;

  // MVs --
  // Is MV logging disabled, and should the table be marked as inconsistent.
  NoLogType isNoLogOperation_;

  // triggers
  NABoolean isIgnoreTriggers_;

  NABoolean noFlow_;

  // QSTUFF
  // this indicates that a on rollback clause was specified in a delete or update
  // statement and that the operator causes updates a before image
  NABoolean updateOnRollback_;

  ValueIdMap oldToNewMap_;

  //MTS - Multi Transaction Support
  // this indicates that the update is part of an MTS query
  // and that only the last row that got updated needs to be returned.
  // However currently the embedded insert for MTS returns all
  // rows, so this flag is used to decide when to set potential outputs
  // and return the appropriate RETDesc.
  NABoolean mtsStatement_;

  //Transaction Type Support
  // this indicates that this IUD is of no Rollback transaction type
  // Allowed only when autocommit is ON and there are no dependent objects
  NABoolean noRollback_;

  // Note that the following comments document the handling of the 
  // Halloween probem as it was implemented .
  // Some restrictions in the R2 implementation have been removed for
  // Neo R2.2.  However, a CQD, R2_HALLOWEEN_SUPPORT can be used to
  // get the old R2 behavior, hence the suffix "R2" on the accessor
  // and mutator methods and the class member variable.

  // If greater than 0, then this Generic Update could potentially run
  // into the Halloween problem.  The source contains a reference to
  // the target. We check for this in binder and issue an error in
  // many cases, but in some cases we allow it and set this flag to
  // TRUE.  If this flag is set, then we must generate a safe plan WRT
  // the Halloween problem.  For the Generic Update operator, this
  // means that the plan should have a SORT operator on the LHS of the
  // Tuple Flow.  This causes the source to be blocked.
  //
  UInt32 avoidHalloweenR2_;

  // Starting with Neo R2.2, a simple boolean is set in the bindNode
  // method.  It is read when configTSJforHalloween is called by 
  // the topMatch methods of TSJFlowRule and TSJRule.  If it is 
  // set to true, configTSJforHalloween will decide whether to protect
  // from Halloween by using the DP2 locks method.
  
  NABoolean avoidHalloween_;

  // Also used in the Neo R2.2 scheme, the next attribute is set to 
  // indicate that the self-referenced table is being access in 
  // modes incompatible with DP2 locks, i.e., SERIALIZABLE or
  // READ UNCOMMITTED ACCESS, or TABLELOCK is requests (the latter
  // is tbd.)  Then is it used in configTSJforHalloween.
  NABoolean halloweenCannotUseDP2Locks_;

  NABoolean isMergeUpdate_;
  NABoolean isMergeDelete_;

  ItemExpr * subqInUpdateAssign_;

  ValueIdSet    mergeInsertRecExpr_;
  ValueIdArray  mergeInsertRecExprArray_;
  ValueIdSet    mergeUpdatePred_;

  // flag to indicate if an insert or delete node is being used implement 
  // part of an update that changes a clustering key or unique index key.
  NABoolean updateCKorUniqueIndexKey_;

  NABoolean hbaseOper_;
  NABoolean uniqueHbaseOper_;
  NABoolean cursorHbaseOper_;
  NABoolean uniqueRowsetHbaseOper_;
  NABoolean canDoCheckAndUpdel_;

  // if set to ON, then this operator is not run as part of an enclosing DTM transaction 
  // nor is a transaction needed to execute it.
  // It is executed using underlying hbase single row transaction consistency.
  NABoolean noDTMxn_;

  // if set to ON, then query is run as part of localized region transaction.
  // No external transaction is started to run it.
  NABoolean useRegionXn_;

  // If set, then for seabase tables, no check of rows existence or non-existence
  // is done during an insert or delete operation. 
  // For insert, row overwrites an existing row, dup error is not returned. (upsert)
  // For delete, rows are deleted if they exist. Caller doesn't get an indication if
  // a row existed or not.
  NABoolean noCheck_;

  // curently used by Delete and Bulk Load Insert only. Other operators
  // have different schemes to avoid IM
  NABoolean noIMneeded_;

  // if set to ON, then this statement will run under MVCC mode
  NABoolean useMVCC_;

  // if set to ON, then this statement will run under SSCC mode
  NABoolean useSSCC_;

  // predicate evaluated before doing the LeafInsert(unique IM)
  // LeafDelete (IM). Only insert/delete if TRUE
  ItemExpr *preconditionTree_;
  ValueIdSet precondition_;

  // For upsert or merge we need an indicator whether the
  // action for a particular row is an insert, update or
  // delete. This ValueId is either NULL_VALUE_ID (meaning
  // no merge or upsert) or it is an expression evaluating
  // to a CHAR(1) CHARACTER SET ISO88591 with these values:
  //   I  this row is being inserted
  //   U  this row is being updated
  //   D  this row is being deleted
  // When we inline index maintenance operations, we generate
  // a tree for an update. This indicator can be used at
  // runtime to suppress index deletion for inserts and to
  // suppress index inserts for deletion. Similar actions
  // can be taken for other inlined code like RI constraints,
  // triggers or MVs (once supported).
  ValueId producedMergeIUDIndicator_;

  // the above variable is used at the producer side, the
  // actual MERGE or Upsert statement. However, we also need
  // to remember it on the consumer side, like an IM insert
  // or delete, this is done here:
  ValueId referencedMergeIUDIndicator_;

};

// -----------------------------------------------------------------------
// The INSERT operator can be used to represent two types of inserts:
//  1) insert specifying a list of constants (use Tuple class as child)
//  2) insert specifying a selection expression (any query as child)
//
// The Insert operator will for each row of its child apply the selection
// predicate (from RelExpr), then calculate a new row by applying
// GenericUpdate::newRecExprArray(), and then insert the result into the
// target table identified by (name,tabId).
// -----------------------------------------------------------------------
class Insert : public GenericUpdate
{
public:

  enum InsertType
  {
    SIMPLE_INSERT,       // One row inserted at a time, data is not buffered.
    VSBB_INSERT_SYSTEM,  // "Smart range lock" - DP2 decides when to turn on
                         // VSBB inserts, and what range to lock.
    VSBB_INSERT_USER,    // "Dumb range lock" - The entire range from the first
                         // row inserted until the next physical row in the
			 // partition is locked.
    VSBB_LOAD,           // SideTree Insert - Buffered inserts into an empty
                         // non-audited key sequenced table. Used with
                         // Load or Create Index queries.
    VSBB_LOAD_AUDITED,   // SideTree Insert - Buffered inserts into an empty
                         // audited key sequenced table. Used with
                         // "insert using load" queries.
    VSBB_LOAD_APPEND,    // Buffered inserts into a non-empty audited key-seq
                         // table
    VSBB_LOAD_NO_DUP_KEY_CHECK, // same as VSBB_LOAD but DP2 doesn't check for
                                // duplicate keys. Used when oneis SURE that
                                // source will not have a duplicate key,
                                // like when loading from a key seq
                                // table to another key seq table
                                // with the same key value.
    VSBB_LOAD_APPEND_NO_DUP_KEY_CHECK,

    VSBB_INSERT_ANYWHERE, // The data is buffered, but does not need to be
                         // sequential, ordered or very large. No range lock
			 // used during the insert operation.
    UPSERT_INSERT,
    UPSERT_LOAD
  };


  Insert(const CorrName &name,
         TableDesc *tabId,
         OperatorTypeEnum otype = REL_UNARY_INSERT,
         RelExpr *child = NULL,
         ItemExpr *insertCols = NULL,
         ItemExpr *orderBy = NULL,
         CollHeap *oHeap = CmpCommon::statementHeap(),
         InsertType insertType = SIMPLE_INSERT,
         NABoolean createUstatSample = FALSE);

  // copy ctor
  Insert (const Insert &) ; // not written

  // virtual destructor
  virtual ~Insert();

  // accessor functions
  void          addInsertColTree(ItemExpr *expr);
  ItemExpr      *removeInsertColTree();
  ItemExpr      *getInsertColTree();
  void          addOrderByTree(ItemExpr *expr);
  ValueIdList   &reqdOrder()            { return reqdOrder_; }

  // a virtual function for performing name binding within the query tree
  virtual RelExpr *bindNode(BindWA *bindWA);

  // Override the GenericUpdate method to transform an Insert-Tuple pair
  // to a LeafInsert node.
  virtual RelExpr *normalizeNode(NormWA & normWARef);

  // method to do code generation
  virtual short codeGen(Generator *);

  virtual RelExpr *copyTopNode(RelExpr *derivedNode = NULL,
			       CollHeap *outHeap = CmpCommon::statementHeap());

  // get a printable string that identifies the operator
  virtual const NAString getText() const;

  // For round-robin partitioned tables, it is necessary to
  // communicate information between bindNode() and codeGen()
  // regarding the expression that computes the storage key for a new
  // row being inserted, and inputs to this expression (partition
  // number, row position, and total number of partitions).  The value
  // id of each of these is stored in the insert node, accessed by the
  // following functions.
  //
  ValueId &rrKeyExpr()          { return rrKeyExpr_; }
  ValueId &partNumInput()       { return partNumInput_; }
  ValueId &rowPosInput()        { return rowPosInput_; }
  ValueId &totalNumPartsInput() { return totalNumPartsInput_; }

  NABoolean &bufferedInsertsAllowed()     { return bufferedInsertsAllowed_; }

  InsertType &getInsertType() { return insertType_; }
  const InsertType &getInsertType() const { return insertType_; }
  void setInsertType(const InsertType &insType) 
  {
    if ((insType == UPSERT_INSERT) ||
	(insType == UPSERT_LOAD) ||
	(noCheck()))
      {
	if (insType == UPSERT_LOAD)
	  insertType_ = insType;

        isUpsert_ = TRUE;
      }
    else
      insertType_ = insType; 
  }

  NABoolean isInsertSelectQuery() { return insertSelectQuery_; }
  void setInsertSelectQuery(NABoolean v) { insertSelectQuery_ = v; }

  void setTargetUserColPosList(CollIndexList &colPos)
  {
    targetUserColPosList_ = &colPos;
  }
  void setTargetUserColPosList()
  {
    targetUserColPosList_ = NULL;
  }
  NABoolean canBindDefaultSpecification() const
  {
    return (targetUserColPosList_ != NULL);
  }
  const char *getColDefaultValue(BindWA *bindWA,
                                 CollIndex positionInTheColPosList) const;

  NABoolean& insertATuple() { return insert_a_tuple_; };

  // append an ascii-version of Insert into cachewa.qryText_
  virtual void generateCacheKey(CacheWA& cwa) const;

  // is this entire expression cacheable after this phase?
  virtual NABoolean isCacheableExpr(CacheWA& cwa);

  // change literals of a cacheable query into ConstantParameters
  virtual RelExpr* normalizeForCache(CacheWA& cwa, BindWA& bindWA);

  void setNoBeginCommitSTInsert(NABoolean noBeginSTI, NABoolean noCommitSTI);

  NABoolean noBeginSTInsert()  { return noBeginSTInsert_;  }
  NABoolean noCommitSTInsert() { return noCommitSTInsert_; }

  NABoolean &enableTransformToSTI() { return enableTransformToSTI_;}
  NABoolean &enableAqrWnrEmpty()    { return enableAqrWnrEmpty_; }

  NABoolean systemGeneratesIdentityValue()  const
  { return systemGeneratesIdentityValue_;};

  void setSystemGeneratesIdentityValue(NABoolean value) 
  { systemGeneratesIdentityValue_ = value; };

  // Test whether sidetree insert can be performed.
  NABoolean isSideTreeInsertFeasible();

  RelExpr * getBoundView() {return boundView_ ;}
  void setBoundView(RelExpr * val) {boundView_ = val;}

  NABoolean isDataSorted(Insert *bef, const Context *context);

  NABoolean getOverwriteHiveTable()
  {
    return overwriteHiveTable_;
  }
  void setOverwriteHiveTable(NABoolean v)
  {
    overwriteHiveTable_ = v;
  }

  NABoolean isSequenceFile()
  {
    return isSequenceFile_;
  }
  void setSequenceFile(NABoolean sf)
  {
    isSequenceFile_ = sf;
  }

  const NABoolean isUpsert() const { return isUpsert_; }
  void setIsUpsert(NABoolean v) { isUpsert_ = v; }

  const NABoolean isUpsertLoad() const { return (isUpsert_ && (getInsertType() == UPSERT_LOAD)); }

  NABoolean getIsTrafLoadPrep() const
  {
    return isTrafLoadPrep_;
  }

  void setIsTrafLoadPrep(NABoolean isTrafLoadPrep)
  {
    isTrafLoadPrep_ = isTrafLoadPrep;
  }

  NABoolean getCreateUstatSample() const
  {
    return createUstatSample_;
  }

  void setCreateUstatSample(NABoolean val)
  {
    createUstatSample_ = val;
  }

  const ItemExprList &baseColRefs() const       { return *baseColRefs_; }

  void setBaseColRefs(ItemExprList * val) {
    baseColRefs_ = val;
  }

  NABoolean xformedEffUpsert() const
  {
    return xformedEffUpsert_;
  }
  void setXformedEffUpsert(NABoolean x)
  {
    xformedEffUpsert_ = TRUE;
  }
  NABoolean isUpsertThatNeedsTransformation(NABoolean isAlignedRowFormat, NABoolean omittedDefaultCols,NABoolean omittedCurrentDefaultCols, NABoolean &toMerge) const;
  RelExpr* xformUpsertToMerge(BindWA *bindWA) ;
  RelExpr* xformUpsertToEfficientTree(BindWA *bindWA) ;
protected:

  InsertType       insertType_;

  // -- Triggers
  // Create a new Insert node that inserts the affected set into the subject table. Used for Before Triggers.
  virtual RelExpr *createEffectiveGU(BindWA *bindWA,
                                     CollHeap *heap,
				     TriggersTempTable& tempTableObj,
				     GenericUpdate **effectiveGUNode,
				     UpdateColumns *colsToSet=NULL);

  // MV
  // The actual insertion of the ON STATEMENT Mv to the list of triggers
  virtual void insertMvToTriggerList(BeforeAndAfterTriggers *list,
				     BindWA *bindWA,
				     CollHeap *heap,
				     const QualifiedName &mvName,
				     MVInfoForDML *mvInfo,
				     const QualifiedName &subjectTable,
				     UpdateColumns *updateCols = NULL);

  //////////////////////////////////////////////////////
  virtual NABoolean pilotAnalysis(QueryAnalysis* qa);
  //////////////////////////////////////////////////////

  // if true, then this node could be used to insert using sidetree.
  NABoolean enableTransformToSTI_;
  // if true, query will undo a failed insert to an empty table
  // by using purgedata, so that it can AQR.
  NABoolean enableAqrWnrEmpty_;

private:

  ItemExpr        *insertColTree_;
  ItemExpr        *orderByTree_;
  ValueIdList      reqdOrder_;                  // ORDER BY list

  CollIndexList   *targetUserColPosList_;       // used by Binder only, for
                                                // VALUES(..,DEFAULT,..) binding
  NABoolean        bufferedInsertsAllowed_;
  ValueId          rrKeyExpr_;
  ValueId          partNumInput_;
  ValueId          rowPosInput_;
  ValueId          totalNumPartsInput_;

  // used by executing-in-dp2 CS.
  NABoolean insert_a_tuple_;

  // used to indicate if a sidetree insert operation should be
  // initiated and/or committed at runtime.
  // Used to implement multiple sidetree inserts/loads into the same
  // partition where one process begins the sidetree insert operation,
  // others just do inserts and another process commits it.
  // These 3 types of processes could overlap, that is, one process could
  // begin, insert & commit, and others could just insert.
  NABoolean noBeginSTInsert_;
  NABoolean noCommitSTInsert_;

  // identifies if insert-select query. This flag is set in insert::BindNode.
  // The following sample queries will set/not set this flag.
  // insert into t1 values (1,2) ; -- not insert-select
  // insert into t1 values (1,2),(2,3),(4,5); -- not insert-select
  // Insert into t1 select * from (values (1,2)) as T; -- insert-select
  // insert into t1 values((select * from t)); not insert-select
  // Insert into t1 values (?[10],?[10]); ---> not insert-select
  // Insert into t1 select * from (select T1.a, T2.a from (values(1,2)) T1(a,b), (values(2,2)) T2(a,b) where T1.b = T2.b)TT(a,b) ; -- insert-select
  NABoolean  insertSelectQuery_;

  ItemExpr *removeOrderByTree() {
    ItemExpr * result = orderByTree_;
    orderByTree_ = NULL;
    return result;
  }

  // This flag is set when the system generates the value
  // for the IDENTITY column. This flag is set
  // to false if the user specifies a value for 
  // the IDENTITY column.
  NABoolean systemGeneratesIdentityValue_;

  // RelExpr that represents the view definition after binding
  // The view text is bound twice for inserts. This RelExpr is the
  // contains the version after the second call to bindNode. It is 
  // used to set information in the viewStoi about which view columns 
  // are accessed in the insert statmement. For updates boundView is
  // passed in through bindUpdateExpr method. For Insert since the 
  // relevant information is in Insert::bindNode we are including
  // this RelExpr as a datamember. It is set to NULL after use to
  // avoid any deep copy issue in post binder phases.
  RelExpr * boundView_ ;

  NABoolean overwriteHiveTable_;
  NABoolean isSequenceFile_;

  // for hbase tables. Silently insert duplicate rows. Creates multiple versions
  // within hbase
  NABoolean isUpsert_;

  NABoolean isTrafLoadPrep_;

  // If this is TRUE, a sample table will be created during the bulk load for
  // use by Update Statistics.
  NABoolean createUstatSample_;
  NABoolean xformedEffUpsert_;
  // ColReference list of the base table columns used in creating the
  // newRecExprArray_ during bindNode(). Used only for index maintenance.
  ItemExprList *baseColRefs_;
};

// -----------------------------------------------------------------------
// A logical insert operator of arity 0, used for index maintenance
// -----------------------------------------------------------------------
class LeafInsert : public Insert
{
public:
  LeafInsert(const CorrName &name,
             TableDesc *tabId,
             ItemExprList *afterColumns,
             OperatorTypeEnum otype = REL_LEAF_INSERT,
	     ItemExpr *preconditionTree = NULL,
             CollHeap *oHeap = CmpCommon::statementHeap())
  : Insert(name, tabId, otype, NULL, NULL, NULL, oHeap)
  {setBaseColRefs(afterColumns);setPreconditionTree(preconditionTree);}

  // copy ctor
  LeafInsert (const LeafInsert &) ; // not written

  // virtual destructor
  virtual ~LeafInsert() {}

  // a virtual function for performing name binding within the query tree
  virtual RelExpr *bindNode(BindWA *bindWA);

};

// -----------------------------------------------------------------------
// Update (for each row of the child table, which contains a key of
// the updated table), replace the row in the updated table by a new
// row, calculated by applying newRecExpr on the child row.
// -----------------------------------------------------------------------
class Update : public GenericUpdate
{
public:

  Update(const CorrName &name,
         TableDesc *tabId,
         OperatorTypeEnum otype = REL_UNARY_UPDATE,
         RelExpr *child = NULL,
         ItemExpr *newRecExpr = NULL,
         ItemExpr *currOfCursorName = NULL,
         CollHeap *oHeap = CmpCommon::statementHeap());

  // copy ctor
  Update (const Update &) ; // not written

  // virtual destructor
  virtual ~Update();

  // a virtual function for performing name binding within the query tree
  virtual RelExpr *bindNode(BindWA *bindWA);

  virtual RelExpr *preCodeGen(Generator * generator,
                              const ValueIdSet & externalInputs,
                              ValueIdSet &pulledNewInputs);

  // method to do code generation
  virtual short codeGen(Generator *);

  virtual RelExpr *copyTopNode(RelExpr *derivedNode = NULL,
			       CollHeap *outHeap = CmpCommon::statementHeap());

  // get a printable string that identifies the operator
  virtual const NAString getText() const;

  // -- Triggers
  // Create a new Update node that updates the affected set into the subject table. Used for Before Triggers.
  virtual RelExpr *createEffectiveGU(BindWA *bindWA,
                                     CollHeap *heap,
				     TriggersTempTable& tempTableObj,
				     GenericUpdate **effectiveGUNode,
				     UpdateColumns *colsToSet=NULL);

  // MV
  // The actual insertion of the ON STATEMENT Mv to the list of triggers
  virtual void insertMvToTriggerList(BeforeAndAfterTriggers *list,
				     BindWA *bindWA,
				     CollHeap *heap,
				     const QualifiedName &mvName,
				     MVInfoForDML *mvInfo,
				     const QualifiedName &subjectTable,
				     UpdateColumns *updatedCols = NULL);

  virtual NABoolean isCacheableExpr(CacheWA& cwa);

  inline const CostScalar getEstRowsAccessed() const
  { return estRowsAccessed_; }

private:

  typedef enum {
    IRELEVANT = 0,
    DIRECT,
    INDIRECT
  } MvUpdateType;

  // MV
  // checks the type of the update needed on the ON STATEMENT MV.
  MvUpdateType checkUpdateType(MVInfoForDML *mvInfo,
			       const QualifiedName &subjectTable,
			       UpdateColumns *updatedCols) const;

  // checks if any of the columns being updated either
  // (a) are part of the clustering key of the base table or
  // (b) are part of an unique index
  // this method also raises an error for some unsupported cases.
  NABoolean updatesClusteringKeyOrUniqueIndexKey(BindWA *bindWA);

  // If the previous methof returns TRUE, then this method is called
  // This method changes an Update node into Insert->Root->Delete
  // and calls bindnode on this new sequence of nodes (which also does 
  // inlining for the new insert and delete nodes)
  RelExpr *transformUpdatePrimaryKey(BindWA *bindWA);

  inline void setEstRowsAccessed(CostScalar r)  { estRowsAccessed_ = r; }

  // Estimated number of rows accessed by Update operator.
  CostScalar estRowsAccessed_;

};

// -----------------------------------------------------------------------
// To implement UPSERT functionality. MERGE is the ansi syntax for that.
// Update, if found. Insert, if not found.
// Syntax:
//  MERGE INTO <table> ON <on-expr> [ USING <sel-query> ] 
//    WHEN MATCHED THEN UPDATE SET <colname> = <value>
//    WHEN NOT MATCHED THEN INSERT (<colname>, ...) VALUES (<values), ...)
// -----------------------------------------------------------------------
class MergeUpdate : public Update
{
public:

  MergeUpdate(const CorrName &name,
	      TableDesc *tabId,
              // This is a problem, we shouldn't have two classes
              // that share the same OperatorTypeEnum value, but
              // REL_UNARY_UPDATE is also used by the Update class
              // above.
	      OperatorTypeEnum otype = REL_UNARY_UPDATE,
	      RelExpr *child = NULL,
	      ItemExpr *setExpr = NULL,
	      ItemExpr *insertCols = NULL,
	      ItemExpr *insertValues = NULL,
	      CollHeap *oHeap = CmpCommon::statementHeap(),
	      ItemExpr *where = NULL);
  
  // copy ctor
  MergeUpdate (const MergeUpdate &) ; // not written

  // virtual destructor
  virtual ~MergeUpdate();

  // a virtual function for performing name binding within the query tree
  virtual RelExpr *bindNode(BindWA *bindWA);

  virtual RelExpr *preCodeGen(Generator * generator,
                              const ValueIdSet & externalInputs,
                              ValueIdSet &pulledNewInputs);

  // method to do code generation
  virtual short codeGen(Generator *);

  virtual RelExpr *copyTopNode(RelExpr *derivedNode = NULL,
			       CollHeap *outHeap = CmpCommon::statementHeap());

  // append an ascii-version of Insert into cachewa.qryText_
  virtual void generateCacheKey(CacheWA& cwa) const;

  // is this entire expression cacheable after this phase?
  virtual NABoolean isCacheableExpr(CacheWA& cwa);

  // change literals of a cacheable query into ConstantParameters
  virtual RelExpr* normalizeForCache(CacheWA& cwa, BindWA& bindWA);

  // get a printable string that identifies the operator
  virtual const NAString getText() const;

  ItemExpr * insertCols() {return insertCols_;}
  virtual ItemExpr * insertValues() { return insertValues_;}

  NABoolean xformedUpsert() {return xformedUpsert_;}
  void setXformedUpsert() {xformedUpsert_ = TRUE;}

  NABoolean needsBindScope() const { return needsBindScope_; }
  void setNeedsBindScope(NABoolean b) { needsBindScope_ = b; }

private:
  ItemExpr *insertCols_;
  ItemExpr *insertValues_;
  ItemExpr *where_;
  NABoolean xformedUpsert_;
  NABoolean needsBindScope_;
};


// -----------------------------------------------------------------------
// The Delete operator reads its child, which contains a key of the table
// (name,tabId) to delete from and performs the deletes.
// -----------------------------------------------------------------------
class Delete : public GenericUpdate
{
public:

  Delete(const CorrName &name,
         TableDesc *tabId,
         OperatorTypeEnum otype = REL_UNARY_DELETE,
         RelExpr *child = NULL,
         ItemExpr *newRecExpr = NULL,
         ItemExpr *currOfCursorName = NULL,
	 ConstStringList * csl = NULL,
         CollHeap *oHeap = CmpCommon::statementHeap());

  // copy ctor
  Delete (const Delete &) ; // not written

  // virtual destructor
  virtual ~Delete();

  // a virtual function for performing name binding within the query tree
  virtual RelExpr *bindNode(BindWA *bindWA);

  virtual RelExpr *preCodeGen(Generator * generator,
                              const ValueIdSet & externalInputs,
                              ValueIdSet &pulledNewInputs);

  // method to do code generation
  virtual short codeGen(Generator *);

  virtual RelExpr *copyTopNode(RelExpr *derivedNode = NULL,
			       CollHeap *outHeap = CmpCommon::statementHeap());

  // is this entire expression cacheable after this phase?
  virtual NABoolean isCacheableExpr(CacheWA& cwa);

  // get a printable string that identifies the operator
  virtual const NAString getText() const;

  // -- Triggers
  // Create a new Delete node that deletes the affected set from the subject table. Used for Before Triggers.
  virtual RelExpr *createEffectiveGU(BindWA *bindWA,
                                     CollHeap *heap,
				     TriggersTempTable& tempTableObj,
				     GenericUpdate **effectiveGUNode,
				     UpdateColumns *colsToSet=NULL);

  // MV
  // The actual insertion of the ON STATEMENT Mv to the list of triggers
  virtual void insertMvToTriggerList(BeforeAndAfterTriggers *list,
				     BindWA *bindWA,
				     CollHeap *heap,
				     const QualifiedName &mvName,
				     MVInfoForDML *mvInfo,
				     const QualifiedName &subjectTable,
				     UpdateColumns *updateCols = NULL);

  ConstStringList* &csl() { return csl_; }

  inline const CostScalar getEstRowsAccessed() const
  { return estRowsAccessed_; }

  inline void setEstRowsAccessed(CostScalar r)  { estRowsAccessed_ = r; }

private:
  ConstStringList * csl_;
  // Estimated number of rows accessed by Delete operator.
  CostScalar estRowsAccessed_;
};

// -----------------------------------------------------------------------
// To implement UPSERT functionality. MERGE is the ansi syntax for that.
// Delete, if found. Insert, if not found.
// Syntax:
//  MERGE INTO <table> ON <on-expr> [ USING <sel-query> ] 
//    WHEN MATCHED THEN DELETE
//    WHEN NOT MATCHED THEN INSERT (<colname>, ...) VALUES (<values), ...)
// -----------------------------------------------------------------------
class MergeDelete : public Delete
{
public:

  MergeDelete(const CorrName &name,
	      TableDesc *tabId,
              // This is a problem, we shouldn't have two classes
              // that share the same OperatorTypeEnum value, but
              // REL_UNARY_DELETE is also used by the Delete class
              // above.
	      OperatorTypeEnum otype = REL_UNARY_DELETE,
	      RelExpr *child = NULL,
	      ItemExpr *insertCols = NULL,
	      ItemExpr *insertValues = NULL,
	      CollHeap *oHeap = CmpCommon::statementHeap());
  
  // copy ctor
  MergeDelete (const MergeDelete &) ; // not written

  // virtual destructor
  virtual ~MergeDelete();

  // a virtual function for performing name binding within the query tree
  virtual RelExpr *bindNode(BindWA *bindWA);

  virtual RelExpr *preCodeGen(Generator * generator,
                              const ValueIdSet & externalInputs,
                              ValueIdSet &pulledNewInputs);

  // method to do code generation
  virtual short codeGen(Generator *);

  virtual RelExpr *copyTopNode(RelExpr *derivedNode = NULL,
			       CollHeap *outHeap = CmpCommon::statementHeap());

  // append an ascii-version of Insert into cachewa.qryText_
  virtual void generateCacheKey(CacheWA& cwa) const;

  // is this entire expression cacheable after this phase?
  virtual NABoolean isCacheableExpr(CacheWA& cwa);

  // change literals of a cacheable query into ConstantParameters
  virtual RelExpr* normalizeForCache(CacheWA& cwa, BindWA& bindWA);

  // get a printable string that identifies the operator
  virtual const NAString getText() const;

  ItemExpr * insertCols() {return insertCols_;}
  virtual ItemExpr * insertValues() { return insertValues_;}
private:
  ItemExpr *insertCols_;
  ItemExpr *insertValues_;
};

// -----------------------------------------------------------------------
// A logical delete operator of arity 0, used for index maintenance
// -----------------------------------------------------------------------
class LeafDelete : public Delete
{
public:
  LeafDelete(const CorrName &name,
             TableDesc *tabId,
             ItemExprList *beforeColumns,
	     NABoolean isUndoUniqueIndex = FALSE,
             OperatorTypeEnum otype = REL_LEAF_DELETE,
             ItemExpr *preconditionTree = NULL,
             CollHeap *oHeap = CmpCommon::statementHeap())
    : Delete(name, tabId, otype, NULL, NULL, NULL, NULL, oHeap),
    baseColRefs_(beforeColumns),
    isUndoUniqueIndex_(isUndoUniqueIndex)
    
  {trigTemp_ = NULL; setPreconditionTree(preconditionTree); }

  // copy ctor
  LeafDelete (const LeafDelete &) ; // not written

  // virtual destructor
  virtual ~LeafDelete() {}

  // a virtual function for performing name binding within the query tree
  virtual RelExpr *bindNode(BindWA *bindWA);

  const ItemExprList &baseColRefs() const       { return *baseColRefs_; }
 NABoolean isUndoUniqueIndex() {return isUndoUniqueIndex_;}
  void setUndoUniqueIndex() { isUndoUniqueIndex_ = TRUE;}

  TriggersTempTable *getTrigTemp() {return trigTemp_;}
  void setTrigTemp(TriggersTempTable *tt) { trigTemp_ = tt;}
  void setUpExecPredForUndoUniqueIndex(BindWA *bindWA);
  void setUpExecPredForUndoTempTable(BindWA *bindWA);

private:
  // ColReference list of the old(before) columns used in creating the
  // beginKeyPred during bindNode().
  ItemExprList *baseColRefs_;
  NABoolean isUndoUniqueIndex_;
  TriggersTempTable *trigTemp_;

};

class HbaseDelete : public Delete
{
public:
  HbaseDelete(CorrName &corrName,
	      RelExpr *child = NULL,
	      CollHeap *oHeap = CmpCommon::statementHeap());

  HbaseDelete(CorrName &corrName,
	      TableDesc *tableDesc,
	      CollHeap *oHeap = CmpCommon::statementHeap());
 
 HbaseDelete(CollHeap *oHeap = CmpCommon::statementHeap());
  
  virtual ~HbaseDelete();

  virtual RelExpr *bindNode(BindWA *bindWAPtr);

  // The set of values that I can potentially produce as output.
  virtual void getPotentialOutputValues(ValueIdSet &vs) const;

  // acessors
  //! getVirtualTableName method
  //  returns a const char pointer to the name of the virtual Table
  // should return a CorrName?##
  virtual const char *getVirtualTableName()
  {
    return corrName_.getQualifiedNameObj().getObjectName().data();
  }

  virtual const CorrName &getCorrName() { return corrName_;}

  // get the degree of this node (it is a leaf op).
  virtual Int32 getArity() const;

  virtual NABoolean isLogical() const { return FALSE; };
  virtual NABoolean isPhysical() const { return TRUE; };

  //! preCodeGen method 
  //  method to do preCode generation
  virtual RelExpr * preCodeGen(Generator * generator,
                               const ValueIdSet & externalInputs,
                               ValueIdSet &pulledNewInputs);

  //! codeGen method
  // virtual method used by the generator to generate the node
  virtual short codeGen(Generator*);

  //! copyTopNode
  //  used to create an Almost complete copy of the node
  virtual RelExpr * copyTopNode(RelExpr *derivedNode = NULL,
                                CollHeap* outHeap = 0);

  //! synthPhysicalProperty
  // cost functions
  virtual PhysicalProperty *synthPhysicalProperty(const Context *context,
                                                  const Lng32     planNumber,
                                                  PlanWorkSpace  *pws);

  virtual CostMethod * costMethod() const;

  //! getText method
  //  used to display the name of the node.
  virtual const NAString getText() const;

  NAList<HbaseSearchKey*>& getHbaseSearchKeys() { return listOfSearchKeys_; };
  void addSearchKey(HbaseSearchKey* sk)
    { listOfSearchKeys_.insertAt(listOfSearchKeys_.entries(), sk); };


 ExplainTuple *
   addSpecificExplainInfo(ExplainTupleMaster *explainTuple,
                                    ComTdb * tdb,
                                    Generator *generator);


 virtual void getInputValuesFromParentAndChildren(ValueIdSet &vs) const;

 protected:

  ListOfUniqueRows listOfDelUniqueRows_;
  ListOfRangeRows  listOfDelSubsetRows_;
  //NAList<HbaseRangeRows> listOfDelSubsetRows_;

  CorrName corrName_;

  // The search keys built during optimization.
  NAList<HbaseSearchKey*> listOfSearchKeys_;

  // set of columns that need to be retrieved from hbase. These are used
  // in executor preds, update/merge expressions or returned back as output
  // values.
  ValueIdSet retColRefSet_;

}; // class HbaseDelete

// -----------------------------------------------------------------------
// generic physical insert operator
// -----------------------------------------------------------------------
class InsertCursor : public Insert
{
public:

  InsertCursor(const CorrName &name,
               TableDesc *tabId,
               OperatorTypeEnum otype = REL_INSERT_CURSOR,
               RelExpr *child = NULL,
               CollHeap *oHeap = CmpCommon::statementHeap(),
               InsertType insertType = SIMPLE_INSERT)
  : Insert(name,tabId,otype,child,NULL,NULL,oHeap, insertType) {}

  // copy ctor
  InsertCursor (const InsertCursor &) ; // not written

  // virtual destructor
  virtual ~InsertCursor();

  virtual RelExpr *copyTopNode(RelExpr *derivedNode = NULL,
			       CollHeap *outHeap = CmpCommon::statementHeap());

  virtual NABoolean isLogical() const;
  virtual NABoolean isPhysical() const;

  // cost functions
  virtual PhysicalProperty *synthPhysicalProperty(const Context *context,
                                                  const Lng32    planNumber,
                                                  PlanWorkSpace  *pws);

  // get a printable string that identifies the operator
  const NAString getText() const;

private:

};

// physical Hive Insert
class HiveInsert: public Insert
{
public:
  HiveInsert(const CorrName &name,
               TableDesc *tabId,
               OperatorTypeEnum otype = REL_HIVE_INSERT,
               RelExpr *child = NULL,
               CollHeap *oHeap = CmpCommon::statementHeap(),
               InsertType insertType = SIMPLE_INSERT)
  : Insert(name,tabId,otype,child,NULL,NULL,oHeap, insertType) {};

  // copy ctor
  HiveInsert(const HiveInsert&) ; // not written

  // virtual destructor
  virtual ~HiveInsert() {};

  virtual RelExpr *copyTopNode(RelExpr *derivedNode = NULL,
                               CollHeap *outHeap = CmpCommon::statementHeap());

  virtual NABoolean isLogical() const { return FALSE; };
  virtual NABoolean isPhysical() const { return TRUE; };

  // cost functions
  virtual PhysicalProperty *synthPhysicalProperty(const Context *context,
                                                  const Lng32    planNumber,
                                                  PlanWorkSpace  *pws);

  // get a printable string that identifies the operator
  const NAString getText() const;

  // method to do code generation
  virtual RelExpr *preCodeGen(Generator * generator,
                              const ValueIdSet & externalInputs,
                              ValueIdSet &pulledNewInputs);
  virtual short codeGen(Generator *);

private:

};

// physical Hbase Insert
class HbaseInsert: public Insert
{
public:
  HbaseInsert(const CorrName &name,
               TableDesc *tabId,
               OperatorTypeEnum otype = REL_HBASE_INSERT,
               RelExpr *child = NULL,
               CollHeap *oHeap = CmpCommon::statementHeap(),
               InsertType insertType = SIMPLE_INSERT)
       : Insert(name,tabId,otype,child,NULL,NULL,oHeap, insertType),
         returnRow_(FALSE),
         vsbbInsert_(FALSE)
       {};

  // copy ctor
  HbaseInsert(const HbaseInsert&) ; // not written

  // virtual destructor
  virtual ~HbaseInsert() {};

  virtual RelExpr *copyTopNode(RelExpr *derivedNode = NULL,
                               CollHeap *outHeap = CmpCommon::statementHeap());

  virtual NABoolean isLogical() const { return FALSE; };
  virtual NABoolean isPhysical() const { return TRUE; };

  // cost functions
  virtual PhysicalProperty *synthPhysicalProperty(const Context *context,
                                                  const Lng32    planNumber,
                                                  PlanWorkSpace  *pws);

  // optimizer functions
  virtual CostMethod* costMethod() const;

  // get a printable string that identifies the operator
  const NAString getText() const;

  void setReturnRow(NABoolean val) {returnRow_ = val;}
  NABoolean isReturnRow() {return returnRow_;}

  void setVsbbInsert(NABoolean val) { vsbbInsert_ = val; }
  NABoolean vsbbInsert() const { return vsbbInsert_; } 

  // method to do code generation
  virtual RelExpr *preCodeGen(Generator * generator,
                              const ValueIdSet & externalInputs,
                              ValueIdSet &pulledNewInputs);
  virtual short codeGen(Generator *);

  ValueIdList &lobLoadExpr()                    { return lobLoadExpr_; }

private:

  // used when lob colums are being loaded. Set in GenPreCode.
  ValueIdList lobLoadExpr_;
  NABoolean returnRow_ ; // currently used only for bulk load incremental IM
  NABoolean vsbbInsert_ ;

};


class HBaseBulkLoadPrep: public Insert
{
public:
  HBaseBulkLoadPrep(const CorrName &name,
               TableDesc *tabId,
               OperatorTypeEnum otype = REL_HBASE_BULK_LOAD,
               RelExpr *child = NULL,
               NABoolean createUstatSample = FALSE,
               CollHeap *oHeap = CmpCommon::statementHeap(),
               InsertType insertType = SIMPLE_INSERT)
  : Insert(name,tabId,REL_UNARY_INSERT,child,NULL,NULL,oHeap, insertType, createUstatSample)
    {};

  // copy ctor
  HBaseBulkLoadPrep(const HBaseBulkLoadPrep&) ;

  // virtual destructor
  virtual ~HBaseBulkLoadPrep() {};

  virtual RelExpr * bindNode(BindWA *bindWA);

  virtual RelExpr *copyTopNode(RelExpr *derivedNode = NULL,
                               CollHeap *outHeap = CmpCommon::statementHeap());

  virtual NABoolean isLogical() const { return TRUE; };
  virtual NABoolean isPhysical() const { return FALSE; };

private:

};
// -----------------------------------------------------------------------
// Update with a cursor
// -----------------------------------------------------------------------
class UpdateCursor : public Update
{
public:

  UpdateCursor(const CorrName &name,
               TableDesc *tabId,
               OperatorTypeEnum otype = REL_UPDATE_CURSOR,
               RelExpr *child = NULL,
               CollHeap *oHeap = CmpCommon::statementHeap())
  : Update(name,tabId,otype,child,NULL,NULL,oHeap) {}

  // copy ctor
  UpdateCursor (const UpdateCursor &) ; // not written

  // virtual destructor
  virtual ~UpdateCursor();

  virtual RelExpr *copyTopNode(RelExpr *derivedNode = NULL,
			       CollHeap *outHeap = CmpCommon::statementHeap());

  virtual NABoolean isLogical() const;
  virtual NABoolean isPhysical() const;

  // cost functions
  virtual PhysicalProperty *synthPhysicalProperty(const Context *context,
                                                  const Lng32    planNumber,
                                                  PlanWorkSpace  *pws);

  // method to do code generation
  virtual RelExpr *preCodeGen(Generator * generator,
                              const ValueIdSet & externalInputs,
                              ValueIdSet &pulledNewInputs);
  virtual short codeGen(Generator *);

  // get a printable string that identifies the operator
  const NAString getText() const;

private:

};

class HbaseUpdate : public UpdateCursor
{
public:
  HbaseUpdate(CorrName &corrName,
	      RelExpr *child = NULL,
	      CollHeap *oHeap = CmpCommon::statementHeap());

  HbaseUpdate(CorrName &corrName,
	      TableDesc *tableDesc,
	      CollHeap *oHeap = CmpCommon::statementHeap());
 
 HbaseUpdate(CollHeap *oHeap = CmpCommon::statementHeap());
  
  virtual ~HbaseUpdate();

  virtual RelExpr *bindNode(BindWA *bindWAPtr);

  // The set of values that I can potentially produce as output.
  virtual void getPotentialOutputValues(ValueIdSet &vs) const;

  // acessors
  //! getVirtualTableName method
  //  returns a const char pointer to the name of the virtual Table
  // should return a CorrName?##
  virtual const char *getVirtualTableName()
  {
    return corrName_.getQualifiedNameObj().getObjectName().data();
  }

  virtual const CorrName &getCorrName() { return corrName_;}

  // get the degree of this node (it is a leaf op).
  virtual Int32 getArity() const;

  virtual NABoolean isLogical() const { return FALSE; };
  virtual NABoolean isPhysical() const { return TRUE; };

  //! preCodeGen method 
  //  method to do preCode generation
  virtual RelExpr * preCodeGen(Generator * generator,
                               const ValueIdSet & externalInputs,
                               ValueIdSet &pulledNewInputs);

  //! codeGen method
  // virtual method used by the generator to generate the node
  virtual short codeGen(Generator*);

  //! copyTopNode
  //  used to create an Almost complete copy of the node
  virtual RelExpr * copyTopNode(RelExpr *derivedNode = NULL,
                                CollHeap* outHeap = 0);

  //! synthPhysicalProperty
  // cost functions
  virtual PhysicalProperty *synthPhysicalProperty(const Context *context,
                                                  const Lng32     planNumber,
                                                  PlanWorkSpace  *pws);

  virtual CostMethod * costMethod() const;

  //! getText method
  //  used to display the name of the node.
  virtual const NAString getText() const;

  NAList<HbaseSearchKey*>& getHbaseSearchKeys() { return listOfSearchKeys_; };
  void addSearchKey(HbaseSearchKey* sk)
    { listOfSearchKeys_.insertAt(listOfSearchKeys_.entries(), sk); };


 ExplainTuple *
   addSpecificExplainInfo(ExplainTupleMaster *explainTuple,
                                    ComTdb * tdb,
                                    Generator *generator);


 protected:

  ListOfUniqueRows listOfUpdUniqueRows_;
  ListOfRangeRows  listOfUpdSubsetRows_;

  CorrName corrName_;

  // The search keys built during optimization.
  NAList<HbaseSearchKey*> listOfSearchKeys_;

  // set of columns that need to be retrieved from hbase. These are used
  // in executor preds, update/merge expressions or returned back as output
  // values.
  ValueIdSet retColRefSet_;

}; // class HbaseUpdate

// -----------------------------------------------------------------------
// delete with a cursor
// -----------------------------------------------------------------------
class DeleteCursor : public Delete
{
public:

  DeleteCursor(const CorrName &name,
               TableDesc *tabId,
               OperatorTypeEnum otype = REL_DELETE_CURSOR,
               RelExpr *child = NULL,
               CollHeap *oHeap = CmpCommon::statementHeap())
    : Delete(name,tabId,otype,child,NULL,NULL,NULL,oHeap) {}

  // copy ctor
  DeleteCursor (const DeleteCursor &) ; // not written

  // virtual destructor
  virtual ~DeleteCursor();

  virtual RelExpr *copyTopNode(RelExpr *derivedNode = NULL,
			       CollHeap *outHeap = CmpCommon::statementHeap());

  virtual NABoolean isLogical() const;
  virtual NABoolean isPhysical() const;

  // cost functions
  virtual PhysicalProperty *synthPhysicalProperty(const Context *context,
                                                  const Lng32    planNumber,
                                                  PlanWorkSpace  *pws);

  // method to do code generation
  virtual short codeGen(Generator *);

  // get a printable string that identifies the operator
  const NAString getText() const;

private:

};

#endif /* RELUPDATE_H */
