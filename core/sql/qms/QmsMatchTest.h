// **********************************************************************
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
// **********************************************************************

// ***********************************************************************
//
// File:         MatchOutput.h
// Description:  
//               
//               
//               
//
// Created:      08/26/08
// ***********************************************************************

#include "QRSharedPtr.h"

// Forward declarations
class RewriteInstructionsItem;
class compositeMatchingResults;
class MatchTest;
class   MatchOutput;
class   MatchRangePredicates;
class   MatchResidualPredicates;
class   MatchGroupingColumns;
class   MatchJoinPreds;
class   MatchOuterJoins;
class AggregateCollectorVisitor;

#ifdef _MEMSHAREDPTR
typedef QRIntrusiveSharedPtr<RewriteInstructionsItem>	  RewriteInstructionsItemPtr;
typedef QRIntrusiveSharedPtr<compositeMatchingResults>	  compositeMatchingResultsPtr;            
typedef QRIntrusiveSharedPtr<MatchOutput>		  MatchOutputPtr;            
typedef QRIntrusiveSharedPtr<MatchRangePredicates>	  MatchRangePredicatesPtr;            
typedef QRIntrusiveSharedPtr<MatchResidualPredicates>	  MatchResidualPredicatesPtr;            
typedef QRIntrusiveSharedPtr<MatchGroupingColumns>	  MatchGroupingColumnsPtr;            
typedef QRIntrusiveSharedPtr<MatchJoinPreds>	          MatchJoinPredsPtr;            
typedef QRIntrusiveSharedPtr<MatchOuterJoins>	          MatchOuterJoinsPtr;            
typedef QRIntrusiveSharedPtr<AggregateCollectorVisitor>	  AggregateCollectorVisitorPtr;            
#else
typedef RewriteInstructionsItem*			  RewriteInstructionsItemPtr;
typedef compositeMatchingResults*			  compositeMatchingResultsPtr;
typedef MatchOutput*					  MatchOutputPtr;            
typedef MatchRangePredicates*				  MatchRangePredicatesPtr;            
typedef MatchResidualPredicates*			  MatchResidualPredicatesPtr;            
typedef MatchGroupingColumns*			          MatchGroupingColumnsPtr;            
typedef MatchJoinPreds*			                  MatchJoinPredsPtr;            
typedef MatchOuterJoins*			          MatchOuterJoinsPtr;            
typedef AggregateCollectorVisitor*			  AggregateCollectorVisitorPtr;            
#endif

typedef StringPtrSet                                      IDSet;
typedef NAPtrList<RewriteInstructionsItemPtr>		  RewriteInstructionsItemList;
typedef NAPtrList<QRFunctionPtr>			  FunctionNodesList;
typedef NAPtrList<QRColumnPtr>	                          ColumnNodesList;
typedef NAPtrList<QRJoinPredPtr>			  JoinPredNodesList;
typedef SharedPtrValueHash<const NAString, RewriteInstructionsItem>  OutputsHash;
typedef NAHashDictionary<const NAString, const NAString>  IDHash;

#ifndef _MATCHOUTPUT_H_
#define _MATCHOUTPUT_H_

#include "QRDescriptor.h"
#include "QmsMVCandidate.h"

// Add a few intermediate result codes to QRElement::ExprResult
enum ResultCode {
  RC_OUTSIDE     = QRElement::Outside,
  RC_PROVIDED    = QRElement::Provided,
  RC_NOTPROVIDED = QRElement::NotProvided,
  RC_REJECT,	      // MV was disqualified
  RC_CONTINUE,	      // Not found yet, continue matching.
  RC_EXTRAHUB,	      // Used by output list
  RC_BACKJOIN,	      // Used by output list
  RC_INDIRECT,	      // Used by output list
  RC_INPUTS_PROVIDED, // Secondary: The expression inputs are provided.
  RC_EXPR_REWRITE,    // Secondary: The expression should be rewritten using an alternative expression.
  RC_MATCH_RANGE,     // Match range preds in Pass 2.
  RC_SKIP_ME,         // This element has already been handled so ignore it (but don't reject it).
  RC_ROLLUP,	      // Rollup of an aggregate function.
  RC_ROLLUP_EXPR,     // Rollup of an aggregate expression.
  RC_AGGR_EXPR,       // Complex aggregate expression.
  RC_ROLLUP_ON_GROUPING, // Rollup over a grouping column.
  RC_NO_INPUTS,       // An expression with no input columns
  RC_INVALID_RESULT
};

enum ResultStatus {
  RS_FINAL,
  RS_INTERMEDIATE
};

/**
 * This class is used to store intermediate rewrite instructions, that are 
 * produced by the matching algorithms, for elements such as an output expression,
 * a range or residual predicate. Each object contains the following:
 * - The element from the query descriptor that is rewritten.
 * - The element from the MV descriptor that is used for this rewrite (optional)
 * - The result code (Provided, NotProvided, Outside etc.)
 * - Is the rewrite for this query element final.
 * - Expression rewrites may have sub-elements.
 * For final rewrites, the result code must be one of the QRElement results, 
 * and no further matching is needed.
 * For non-final rewrites, another pass of matching is required to finalize it.
 *****************************************************************************
 */
class RewriteInstructionsItem : public NAIntrusiveSharedPtrObject
{
public:
  /**
   * RewriteInstructionsItem constructor
   * @param queryElement The query element to be rewritten
   * @param mvElement The (optional) MV element used for the rewrite
   * @param resultCode The result code
   * @param status Is the rewrite final or intermediate.
   * @param heap The heap from which to allocate memory.
   */
  RewriteInstructionsItem(const QRElementPtr  queryElement,
			  const QRElementPtr  mvElement,
			  ResultCode	      resultCode,
			  ResultStatus	      status,
			  ADD_MEMCHECK_ARGS_DECL(CollHeap* heap));

  
  /**
   * Copy constructor
   */
  RewriteInstructionsItem(const RewriteInstructionsItem& other,
			  ADD_MEMCHECK_ARGS_DECL(CollHeap* heap));

  virtual ~RewriteInstructionsItem();

  NABoolean operator==(const RewriteInstructionsItem& other)
  {
    // Do a pointer comparison.
    return this == &other;
  }

  /**
   * Get the query element that is rewritten
   * @return 
   */
  const QRElementPtr getQueryElement()	
  { 
    return queryElement_; 
  }

  /**
   * Get the query element that is rewritten
   * If its an Output element, return the output item inside it.
   * @return 
   */
  const QRElementPtr getActualQueryElement()	
  { 
    QRElementPtr queryElem = getQueryElement();
    if (queryElem->getElementType()== ET_Output)
      queryElem = queryElem->downCastToQROutput()->getOutputItem()->getReferencedElement();
    return queryElem; 
  }

  /**
   * Get the MV element that is used to rewrite the query element.
   * @return 
   */
  const QRElementPtr getMvElement()	
  { 
    return mvElement_; 
  }

  /**
   * Get the result code
   * @return 
   */
  ResultCode getResultCode()		
  { 
    return resultCode_; 
  }

  /**
   * Return a string corresponding to the result code.
   * Used for debugging only.
   * @return A pointer to the name of the result code.
   */
  static const char* getResultString(ResultCode rc);

  const char* getResultString();

  /**
   * Is the rewrite final?
   * @return 
   */
  NABoolean isFinal()			
  { 
    return status_ == RS_FINAL; 
  }

  /**
   * Get the result status (Final ot Intermediate)
   * @return 
   */
  ResultStatus getResultStatus()
  {
    return status_;
  }

  /**
   * Set the result status (Final ot Intermediate)
   * @param status 
   */
  void setResultStatus(ResultStatus status)
  {
    status_ = status;
  }

  /**
   * Get the secondary result code
   * @return 
   */
  ResultCode getSecondaryResultCode()
  {
    return secondaryResultCode_;
  }

  /**
   * Get the list of rewrite instructions for sub-elements
   * @return 
   */
  compositeMatchingResultsPtr getSubElements()
  {
    return subElements_;
  }

  /**
   * Does this object include any rewrite instructions for sub-elements?
   * @return 
   */
  NABoolean hasSpecialSubElement()
  {
    return extraHubTables_.entries()>0 || backJoinTables_.entries()>0;
  }

  /**
   * Set the secondary result code
   * @param rc 
   */
  void setSecondaryResultCode(ResultCode rc)
  {
    secondaryResultCode_ = rc;
  }

  /**
   * Add an optional sub-element.
   * @param subElement 
   */
  void addSubElement(RewriteInstructionsItemPtr subElement);

  /**
   * Add a list of rewrite instructions for sub-elements.
   * @param subElements 
   * @param isSpecial Are these of a type that requires special consideration
   *                  such as Back-Join or Extra-Hub?
   */
  void addSubElements(const RewriteInstructionsItemList& subElements, NABoolean isSpecial);
 

  /**
   * Add a list of rewrite instructions for sub-elements.
   * @param subElements 
   */
  void addSubElements(compositeMatchingResultsPtr other);

  /**
   * Set the query element of the rewrite instructions
   * @param queryElement 
   */
  void setQueryElement(const QRElementPtr queryElement)
  {
    queryElement_ = queryElement;
  }

  /**
   * Set the query element of the rewrite instructions
   * @param queryElement 
   */
  void setMvElement(const QRElementPtr mvElement)
  {
    mvElement_ = mvElement;
  }

  /**
   * Set the result code.
   * @param rc 
   */
  void setResultCode(ResultCode rc)
  {
    resultCode_ = rc;
  }

  /**
   * Get the final result code for use in the descriptor.
   * @return 
   */
  QRElement::ExprResult getDescriptorResultCode()
  {
    switch (resultCode_)
    {
      case RC_PROVIDED:
	return QRElement::Provided;

      case RC_NOTPROVIDED:
      case RC_BACKJOIN:
	return QRElement::NotProvided;

      case RC_OUTSIDE:
	return QRElement::Outside;

      default:
	// These are the only result codes allowed for final rewrite instructions.
	assertLogAndThrow(CAT_MATCHTST_MVDETAILS, LL_MVQR_FAIL,
                          FALSE, QRLogicException, 
			  "Not a final result code."); 
	return QRElement::INVALID_EXPR_RESULT;
    }
  }

  /**
   * Add a table ID to the list of extra-hub tables used
   * @param tableID 
   */
  void addExtraHubTable(const NAString* tableID)
  {
    extraHubTables_.insert(tableID);
  }

  /**
   * Add a table ID to the list of back-join tables used
   * @param tableID 
   */
  void addBackJoinTable(const NAString* tableID)
  {
    backJoinTables_.insert(tableID);
  }

  /**
   * Get the list of extra-hub tables used
   * @return 
   */
  IDSet& getExtraHubTables()
  {
    return extraHubTables_;
  }

  /**
   * Get the list of back-join tables used
   * @return 
   */
  IDSet& getBackJoinTables()
  {
    return backJoinTables_;
  }

private:
  QRElementPtr			queryElement_;
  QRElementPtr			mvElement_;
  ResultCode			resultCode_;
  ResultCode			secondaryResultCode_;
  ResultStatus			status_;
  compositeMatchingResultsPtr   subElements_;
  IDSet	                        extraHubTables_;  // Query descriptor IDs of needed extra-hub tables.
  IDSet	                        backJoinTables_;  // Query descriptor IDs of needed back-join tables.
};  // class RewriteInstructionsItem

/**
 * Intermediate results of matching a list of input columns to expressions.
 *****************************************************************************
 */
class compositeMatchingResults : public NAIntrusiveSharedPtrObject
{
public:
  compositeMatchingResults(ADD_MEMCHECK_ARGS_DECL(CollHeap* heap))
    : NAIntrusiveSharedPtrObject(ADD_MEMCHECK_ARGS_PASS(heap))
     ,providedInputs_(heap)
     ,notProvidedInputs_(heap)
     ,outsideInputs_(heap)
     ,extrahubInputs_(heap)
     ,backJoinInputs_(heap)
     ,indirectInputs_(heap)
     ,wasBlockedColumn_(FALSE)
  {}

  virtual ~compositeMatchingResults();

  /**
   * Get the number of rewrite instructions objects contained here.
   * @return 
   */
  CollIndex entries()
  {
    return providedInputs_.entries() +
           notProvidedInputs_.entries() +
	   outsideInputs_.entries()  +
	   extrahubInputs_.entries() +
	   backJoinInputs_.entries() +
	   indirectInputs_.entries();
  }

  void clear()
  {
    providedInputs_.clear();
    notProvidedInputs_.clear();
    outsideInputs_.clear();
    extrahubInputs_.clear();
    backJoinInputs_.clear();
    indirectInputs_.clear();
  }

  /**
   * Insert a single rewrite instructions object.
   * @param item 
   */
  void insert(RewriteInstructionsItemPtr item);

  /**
   * Insert a list of rewrite instructions objects.
   * @param itemList 
   */
  void insert(const RewriteInstructionsItemList& itemList);

  // A blocked column is one that is not provoded by the MV, and cannot be 
  // accesed through a back-join, because its table is not back-joinable.
  NABoolean		      wasBlockedColumn_;

  // Lists of sub-elements by their result code.
  RewriteInstructionsItemList providedInputs_;
  RewriteInstructionsItemList notProvidedInputs_;
  RewriteInstructionsItemList outsideInputs_;
  RewriteInstructionsItemList extrahubInputs_;
  RewriteInstructionsItemList backJoinInputs_;
  RewriteInstructionsItemList indirectInputs_;
};  // class compositeMatchingResults

/**
 * The \c MatchTest class is an abstract superclass to all the specific matching
 * tests, such as range, residual and output list.
 * It provides these main functions:
 * - The definition of a standard interface for calling the Pass 1 and Pass 2
 *   matching tests, as well as for generating the result descriptor.
 * - Implementation of common methods for reusing code.
 * - Storing intermediate and final rewrite instructions.
 *****************************************************************************
 */
class MatchTest : public NAIntrusiveSharedPtrObject
{
public:
  /**
   * RewriteInstructions constructor
   * @param heap The heap from which to allocate memory.
   */
  MatchTest(ADD_MEMCHECK_ARGS_DECL(CollHeap* heap))
    : NAIntrusiveSharedPtrObject(ADD_MEMCHECK_ARGS_PASS(heap))
     ,listOfFinalInstructions_(heap)
     ,listOfPendingWork_(heap)
     ,heap_(heap)
  {
  }

  virtual ~MatchTest();

  /**
   * Run the Pass 1 algorithm.
   * Pure virtual method to be implemented by subclasses.
   * @return TRUE if the MV passed this test, FALSE if it was disqualified.
   */
  virtual NABoolean matchPass1() = 0;

  /**
   * Run the Pass 2 algorithm.
   * This method goes over the remaining intermediate rewrite instructions, 
   * and calls the pure virtual method \c matchPass2OnElement() for each.
   * @return TRUE if the MV passed this test, FALSE if it was disqualified.
   */
  NABoolean matchPass2();

  /**
   * Generate the relevant section of the MV candidate in the result descriptor.
   * @return TRUE if result descriptor was generated, FALSE if MV was rejected.
   */
  NABoolean generateDescriptor(QRCandidatePtr resultDesc);

  /**
   * Set a pointer to the MVCandidate object.
   * @param candidate 
   */
  void setCandidate(MVCandidatePtr candidate)
  {
    candidate_ = candidate;
  }

  /**
   * Add an item of rewrite instructions to the collection.
   * @param rewrite the item to add.
   */
  virtual void addRewriteInstructions(RewriteInstructionsItemPtr rewrite);

protected:

  /**
   * Is the matching work done for this MV candidate, for a particular match test?
   * @return TRUE if done, FALSE if more matching work is needed.
   */
  NABoolean isFinal()
  {
    return (listOfPendingWork_.entries() == 0);
  }

  /**
   * Get the list of final rewrite instructions for the result descriptor.
   * @return 
   */
  RewriteInstructionsItemList& getListOfFinalInstructions()
  {
    return listOfFinalInstructions_;
  }

  /**
   * Get the list of output items that still need some work.
   * @return 
   */
  RewriteInstructionsItemList& getListOfPendingWork()
  {
    return listOfPendingWork_;
  }

  /**
   * Generate the QRMVColumn element for a provided column.
   * @param rewrite 
   * @return 
   */
  QRMVColumnPtr generateProvidedMvColumn(RewriteInstructionsItemPtr rewrite);

  /**
   * Generate the QRColumn element for a NotProvided column.
   * @param rewrite 
   * @return 
   */
  QRColumnPtr generateNotProvidedColumn(RewriteInstructionsItemPtr rewrite);

  /**
   * Run Pass 2 matching test for a particular element.
   * Pure virtual method to be implemented by subclasses.
   * @param pending The rewrite instructions prepared by the Pass 1 code.
   * @return TRUE if the MV passed this test, FALSE if it was disqualified.
   */
  virtual NABoolean matchPass2OnElement(RewriteInstructionsItemPtr pending) = 0;

  /**
   * Generate a Provided result descriptor element.
   * Pure virtual method to be implemented by subclasses.
   * @param resultDesc 
   * @param rewrite 
   * @return TRUE if the MV passed this test, FALSE if it was disqualified.
   */
  virtual NABoolean generateProvidedElement   (QRCandidatePtr resultDesc, RewriteInstructionsItemPtr rewrite) = 0;

  /**
   * Generate a NotProvided result descriptor element.
   * Pure virtual method to be implemented by subclasses.
   * @param resultDesc 
   * @param rewrite 
   * @return TRUE if the MV passed this test, FALSE if it was disqualified.
   */
  virtual NABoolean generateNotProvidedElement(QRCandidatePtr resultDesc, RewriteInstructionsItemPtr rewrite) = 0;

  RewriteInstructionsItemPtr matchColumn(const QRColumnPtr colElem, 
				         NABoolean fromExpr,
                                         NABoolean isAnEqualitySet = FALSE);

  /**
   * Match a single, full (not referencing) output column (Pass 1)
   * @param colElem The query column element
   * @param fromExpr Is this method called from the expression matching algorithm
   * @return The rewrite instructions for matching the query column, 
   *         or NULL if it is not covered by the MV.
   */
  RewriteInstructionsItemPtr matchSingleColumn(const QRColumnPtr colElem, 
					       NABoolean fromExpr);

  /**
   * Match an Equality set (Pass 1).
   * @param joinPred The join predicate element
   * @return The rewrite instructions for matching the query equality set, 
             or NULL if it is not covered by the MV.
   */
  RewriteInstructionsItemPtr matchEqualitySet(const QRJoinPredPtr joinPred);


  /**
   * Check if \c colElem is an Outside column (from a table that is not covered
   * by the MV. There are three possible results: 
   * - FALSE, no extraHubID   - The column is from an MV hub table, need to check if its provided.
   * - FALSE, extraHubID set  - The column is from an MV extra hub table
   * - TRUE		      - the table is not covered by the MV at all.
   * \par
   * @param colElem The column to check
   * @param extraHubID [OUT] When the table is detected as an extra-hub table,
   *                         this returns its descriptor ID.
   */
  NABoolean isOutsideColumn(const QRColumnPtr colElem, const NAString*& extraHubID);

  /**
   * Is this column provided by the MV?
   * @param colElem The column to check
   * @param mvCol [OUT] A pointer to the MV output column that provides it.
   * @return TRUE if the column is Provided, FALSE otherwise.
   */
  NABoolean isProvidedColumn(const QRColumnPtr colElem, QROutputPtr& mvCol);

  /**
   * Can the column be provided by using a back-join?
   * @param colElem The column to check
   * @return TRUE if this column is back-joinable, FALSE otherwise.
   */
  NABoolean isBackJoinableColumn(const QRColumnPtr  colElem);

  /**
   * This method is called only for columns that are inputs of query 
   * output expressions. It checks if this column is an input of an 
   * MV output expression, so there is a chance that the MV provides
   * a sub-expression of the needed query expression.
   * @param colElem The column to check
   * @return The rewrite instructions for matching the query output column,
   *         or NULL if it is not covered by the MV.
   */
  RewriteInstructionsItemPtr isIndirectColumn(const QRColumnPtr colElem);

  /**
   * Match an expression (Pass 1).
   * @param expr The query expression.
   * @return The rewrite instructions for matching the query expression, 
   *         or NULL if it is not covered by the MV.
   */
  RewriteInstructionsItemPtr matchExpression(const QRExprPtr expr);

  /**
   * Check if the MV provides a matching output expression, including its 
   * input columns.
   * @param expr The expression to find
   * @return The rewrite instructions for matching the query expression,
   *         or NULL if it is not provided directly.
   */
  RewriteInstructionsItemPtr findMatchingMvExpression(const QRExplicitExprPtr expr);

  /**
   * Find MV columns that correspond to the input columns of the expression.
   * @param queryColList The list of expression input columns.
   * @return Rewrite instructions or NULL if no matching MV columns found.
   */
  RewriteInstructionsItemPtr findExpressionInputs(const ElementPtrList& queryColList,
						  const QRElementPtr    queryElement);
  /**
   * For a list of columns that can be either the list od exprerssion inputs, 
   * or an equality set list of members, match every member of the list, and 
   * sort it according to the result code in separate lists.
   * @param queryColList The list of columns to sort out.
   * @param matchingResults [OUT] The results are returned here.
   * @param isAnEqualitySet Is this an equality set (TRUE) or an expression 
   *                        input list (FALSE)
   */
  void MatchInputColumnList(const ElementPtrList&          queryColList, 
			    compositeMatchingResultsPtr    matchingResults,
			    NABoolean			   isAnEqualitySet);

 /**
   * Match the input column lists of a query expression from a range or 
   * residual predicate, with a corresponding MV expression that has the same 
   * expression text, in order to make sure its the same predicate.
   * @param queryExpr The query expression.
   * @param mvExpr The MV expression.
   * @return TRUE if the column lists match, FALSE otherwise.
   */
  NABoolean matchExpressionInputs(const ElementPtrList& queryInputColumns,
				  const ElementPtrList& mvInputColumns);

  /**
   * Check if the queryColumn matches the mvColumn, not only by the column 
   * name, but also that its from the same instance of the same table.
   * @param queryColumn Column element from the query descriptor.
   * @param mvColumn Column element from the MV descriptor.
   * @return TRUE if there is a match, FALSE otherwise.
   */
  NABoolean matchColumnsByNameAndID(const QRColumnPtr queryColumn, 
				    const QRColumnPtr mvColumn);


  NABoolean verifyExtraHubAndBackJoinColumns(RewriteInstructionsItemPtr pending);
  NABoolean verifyExtraHubColumn(RewriteInstructionsItemPtr pending);

  void addInputsToOutputList(RewriteInstructionsItemPtr rewrite);

  RewriteInstructionsItemPtr findInternalAggregateFunctions(QRExplicitExprPtr treeExpr);
  RewriteInstructionsItemPtr findAggregateFunction(QRFunctionPtr func);
  QRExprPtr generateAggregateExpressionOutput(RewriteInstructionsItemPtr rewrite);
  QRFunctionPtr generateRollupAggregateFunction(RewriteInstructionsItemPtr rewrite);
  QRFunctionPtr generateRollupOverGroupingAggregateFunction(RewriteInstructionsItemPtr rewrite);
  NABoolean isExtraGroupingColumn(QRColumnPtr col);
  NABoolean areAllInputsGroupingColumns(QRFunctionPtr func, ElementPtrList& inputList);
  void cleanupSubExprHash(subExpressionRewriteHash& subExprHash);

protected:
  CollHeap*			heap_;
  MVCandidatePtr		candidate_;

private:
  // List of final rewrite instructions
  RewriteInstructionsItemList   listOfFinalInstructions_;

  // List of intermediate rewrite instructions.
  RewriteInstructionsItemList   listOfPendingWork_;

};  // class MatchTest

/**
 * Matching test for the output list.
 *****************************************************************************
 */
class MatchOutput : public MatchTest
{
public:
  /**
   * MatchOutput constructor
   * @param heap The heap from which to allocate memory.
   */
  MatchOutput(ADD_MEMCHECK_ARGS_DECL(CollHeap* heap))
    : MatchTest(ADD_MEMCHECK_ARGS_PASS(heap))
     ,OutputRewriteInstructionsByID_(hashKey, INIT_HASH_SIZE_SMALL, TRUE, heap)
     ,OutputRewriteInstructionsByMvColName_(hashKey, INIT_HASH_SIZE_SMALL, TRUE, heap)
     ,OutputsToAvoidByID_(hashKey, INIT_HASH_SIZE_SMALL, TRUE, heap)
  {}

  virtual ~MatchOutput()
  {
    OutputRewriteInstructionsByID_.clear();
    OutputRewriteInstructionsByMvColName_.clear();
    OutputsToAvoidByID_.clear();
  }

  /**
   * Run the Pass 1 algorithm of the output list test.
   * Implementation of superclass pure virtual method.
   * @return TRUE if the MV passed this test, FALSE if it was disqualified.
   */
  virtual NABoolean matchPass1();

  /**
   * Override superclass implementation in order to filter duplicates
   * @param rewrite 
   */
  virtual void addRewriteInstructions(RewriteInstructionsItemPtr rewrite);

  /**
   * Add an IDto the list of outputs to avoid.
   * @param id 
   */
  void addOutputToAvoid(const NAString& id);

protected:

  virtual NABoolean matchPass2OnElement(RewriteInstructionsItemPtr pending);

  NABoolean matchAggregateExpressions(RewriteInstructionsItemPtr pending);  

  // Implementation of virtual methods.
  QROutputPtr createNewResultElement(RewriteInstructionsItemPtr rewrite);

  virtual NABoolean generateProvidedElement   (QRCandidatePtr resultDesc, RewriteInstructionsItemPtr rewrite);
  virtual NABoolean generateNotProvidedElement(QRCandidatePtr resultDesc, RewriteInstructionsItemPtr rewrite);

  QROutputPtr generateRollupAggregateOutput(RewriteInstructionsItemPtr rewrite);

  NABoolean isFromOutside(RewriteInstructionsItemPtr rewrite);
  void switchRewrites(RewriteInstructionsItemPtr newRewrite, 
		      RewriteInstructionsItemPtr firstRewrite,
		      const NAString& MVColName);

private:
  OutputsHash	OutputRewriteInstructionsByID_;
  OutputsHash   OutputRewriteInstructionsByMvColName_;
  IDHash	OutputsToAvoidByID_;
};  // class MatchOutput


/**
 * Matching test for range predicates.
 *****************************************************************************
 */
class MatchRangePredicates : public MatchTest
{
public:
  /**
   * Constructor
   * @param heap The heap from which to allocate memory.
   */
  MatchRangePredicates(ADD_MEMCHECK_ARGS_DECL(CollHeap* heap))
    : MatchTest(ADD_MEMCHECK_ARGS_PASS(heap)),
      mvFullPredList_(heap)
  {}

  virtual ~MatchRangePredicates()
  {}

  /**
   * Run the Pass 1 algorithm of range predicate test.
   * Implementation of superclass pure virtual method.
   * @return TRUE if the MV passed this test, FALSE if it was disqualified.
   */
  virtual NABoolean matchPass1();

  /**
   * Used for Workload Analysis, to match a range predicate between two MVs.
   * @param querySidePred the range predicate from the first MV.
   * @return if the second MV has a matching predicate, return it, or NULL otherwise.
   */
  QRRangePredPtr checkPredicate(QRRangePredPtr querySidePred);

protected:
  /**
   * Check if the query predicate bitmaps are a superset of the MV predicate bitmaps.
   * This method checks the bitmaps for both range and residual predicates, 
   * because the algorithm is the same for both, and it doesn't make sense to 
   * repeat the entire loop for the residual predicates in the MatchResidualPredicates
   * class for the sake of organized code.
   * If the query range predicate bitmap is not a superset of the MV bitmap for
   * all the involved tables, then the MV has predicates that are not covered by
   * the query, and is very quickly disqualified.
   * @return TRUE if the MV passed, and FALSE if it is disqualified.
   */
  NABoolean matchPredicateBitmaps();

  /**
   * Match a range predicate on an expression
   * @param queryRangePred The range predicate element.
   * @return FALSE is the MVCandidate should be disqualified.
   */
  NABoolean matchPredOnExpr(const QRRangePredPtr queryRangePred);

  /**
   * Match a range predicate on a column
   * @param queryRangePred The range predicate element.
   * @return FALSE is the MVCandidate should be disqualified.
   */
  NABoolean matchPredOnColumn(const QRRangePredPtr queryRangePred, const QRColumnPtr rangeColumn);

  /**
   * Match a range predicate on an equality set.
   * @param queryRangePred The range predicate element.
   * @param joinPred The join predicate element.
   * @return FALSE is the MVCandidate should be disqualified.
   */
  NABoolean matchPredOnEqualitySet(const QRRangePredPtr queryRangePred, const QRJoinPredPtr joinPred);

  // Implementation of virtual methods.
  virtual NABoolean matchPass2OnElement(RewriteInstructionsItemPtr pending);

  QRRangePredPtr createNewResultElement(RewriteInstructionsItemPtr rewrite);

  virtual NABoolean generateProvidedElement   (QRCandidatePtr resultDesc, RewriteInstructionsItemPtr rewrite);
  virtual NABoolean generateNotProvidedElement(QRCandidatePtr resultDesc, RewriteInstructionsItemPtr rewrite);

private:
  // Used to track if all the range predicates of the MV have been matched.
  RangePredPtrList  mvFullPredList_;
};  // class MatchRangePredicates

/**
 * Matching test for residual predicates.
 *****************************************************************************
 */
class MatchResidualPredicates : public MatchTest
{
public:
  /**
   * Constructor
   * @param heap The heap from which to allocate memory.
   */
  MatchResidualPredicates(ADD_MEMCHECK_ARGS_DECL(CollHeap* heap))
    : MatchTest(ADD_MEMCHECK_ARGS_PASS(heap)),
      mvFullPredList_(heap)
  {}

  virtual ~MatchResidualPredicates()
  {}

  /**
   * Run the Pass 1 algorithm of residual predicate test.
   * Implementation of superclass pure virtual method.
   * @return TRUE if the MV passed this test, FALSE if it was disqualified.
   */
  virtual NABoolean matchPass1();

  /**
   * Used for Workload Analysis, to match a residual predicate between two MVs.
   * @param querySidePred the residual predicate from the first MV.
   * @return if the second MV has a matching predicate, return it, or NULL otherwise.
   */
  QRExprPtr checkPredicate(QRExprPtr querySidePred);

protected:
    // Implementation of virtual methods.
  virtual NABoolean matchPass2OnElement(RewriteInstructionsItemPtr pending);

  QRExprPtr createNewResultElement(RewriteInstructionsItemPtr rewrite);

  /**
   * Match a query residual predicate
   * @param queryResidualPred The residual predicate element
   * @param usedDupMvPreds List of preds that have been matched, and share the
   *                       same text with 1 or more others.
   * @return The rewrite instructions, or NULL if the MV candidate should be disqualified.
   */
  RewriteInstructionsItemPtr matchResidualPredicate(const QRExprPtr queryResidualPred,
                                                    ResidualPredPtrList& usedDupMvPreds);

  virtual NABoolean generateProvidedElement   (QRCandidatePtr resultDesc, RewriteInstructionsItemPtr rewrite);
  virtual NABoolean generateNotProvidedElement(QRCandidatePtr resultDesc, RewriteInstructionsItemPtr rewrite);

private:
  // Used to track if all the residual predicates of the MV have been matched.
  ResidualPredPtrList mvFullPredList_;
};  // class MatchResidualPredicates

/**
 * Matching test for grouping columns.
 *****************************************************************************
 */
class MatchGroupingColumns : public MatchTest
{
public:
  /**
   * Constructor
   * @param heap The heap from which to allocate memory.
   */
  MatchGroupingColumns(ADD_MEMCHECK_ARGS_DECL(CollHeap* heap))
    : MatchTest(ADD_MEMCHECK_ARGS_PASS(heap))
  {}

  virtual ~MatchGroupingColumns()
  {}

  /**
   * Run the Pass 1 algorithm of residual predicate test.
   * Implementation of superclass pure virtual method.
   * @return TRUE if the MV passed this test, FALSE if it was disqualified.
   */
  virtual NABoolean matchPass1();

  /**
   * Override MatchTest method, because the GroupBy element is different.
   * @param resultDesc The result descriptor being generated.
   * @return FALSE if the MV candidate has been disqualified.
   */
  virtual NABoolean generateDescriptor(QRCandidatePtr resultDesc);

protected:
  // Implementation of virtual methods from parent class.
  ///////////////////////////////////////////////////////////

  // No Pass 2 functionality needed.
  virtual NABoolean matchPass2OnElement(RewriteInstructionsItemPtr pending)
  {
    return TRUE;
  }

  // Not used.
  virtual NABoolean generateProvidedElement(QRCandidatePtr resultDesc, RewriteInstructionsItemPtr rewrite)
  {
    return TRUE;
  }

  // Not used.
  virtual NABoolean generateNotProvidedElement(QRCandidatePtr resultDesc, RewriteInstructionsItemPtr rewrite)
  {
    return TRUE;
  }

  // Internal methods.
  //////////////////////

  NABoolean VerifyGroupingColumns();
  NABoolean verifyGroupingElement(const QRElementPtr groupingElem, NABoolean isRecursive = FALSE);

};  // class MatchGroupingColumns

/**
 * Matching test for Equi-Join predicates.
 * This test checks all the equi-join predicates in the query hub and extrahub,
 * to find any that involve both tables used by the MV, and tables outside
 * the MV. For these predicates, we check if the needed columns from the MV 
 * tables are Provided by the MV. If not - the MV is disqualified. Otherwise
 * the needed columns are added to the result descriptor Output list.
 * The PASS 1 method is the last PASS 1 method being called, so it knows the 
 * needed extrahub tables. Any join predicates that involve needed extrahub 
 * tables are postponed to PASS 2, when the extrahub tables have already been 
 * verified.
 * This test does not create any JoinPred elements in the result descriptor,
 * but rather, just adds MV columns to the Output list. Therefore the 
 * descriptor generation methods are not used.
 *****************************************************************************
 */
class MatchJoinPreds : public MatchTest
{
public:
  /**
   * Constructor
   * @param heap The heap from which to allocate memory.
   */
  MatchJoinPreds(ADD_MEMCHECK_ARGS_DECL(CollHeap* heap))
    : MatchTest(ADD_MEMCHECK_ARGS_PASS(heap))
  {}

  virtual ~MatchJoinPreds()
  {}

  /**
   * Run the Pass 1 algorithm of equi-join predicates test.
   * Implementation of superclass pure virtual method.
   * @return TRUE if the MV passed this test, FALSE if it was disqualified.
   */
  virtual NABoolean matchPass1();

  /**
   * Override MatchTest method, because the JoinPred element is different.
   * @param resultDesc The result descriptor being generated.
   * @return FALSE if the MV candidate has been disqualified.
   */
  virtual NABoolean generateDescriptor(QRCandidatePtr resultDesc);

protected:
  // Implementation of virtual methods from parent class.
  ///////////////////////////////////////////////////////////

  /**
   * Check again any predicate that involves needed extrahub tables.
   * @param pending 
   * @return 
   */
  virtual NABoolean matchPass2OnElement(RewriteInstructionsItemPtr pending);

  NABoolean addBackJoinPredsForTable(const QRTablePtr table, QRCandidatePtr resultDesc);

  // Not used.
  virtual NABoolean generateProvidedElement(QRCandidatePtr resultDesc, RewriteInstructionsItemPtr rewrite)
  {
    return TRUE;
  }

  // Not used
  virtual NABoolean generateNotProvidedElement(QRCandidatePtr resultDesc, RewriteInstructionsItemPtr rewrite)
  {
    return TRUE;
  }

protected:
  // Internal methods.
  //////////////////////

  NABoolean analyzeJoinPredicate(const QRJoinPredPtr joinPred, NABoolean isPass1);

  void analyzeEQMember(QRElementPtr   elem,
                       Int32&         mvTables,
                       Int32&         ehTables,
                       Int32&         outTables,
                       ColumnNodesList& mvColumns,
                       NAPtrList<QRExprPtr>&   mvExprs,
                       NABoolean      isPass1);

  NABoolean analyzeProvidedExpression(const QRExprPtr expr);

};  // class MatchJoinPreds

/**
 * 
 * 
 * 
 * 
 * 
 *****************************************************************************
 */
class MatchOuterJoins : public MatchTest
{
public:
  /**
   * Constructor
   * @param heap The heap from which to allocate memory.
   */
  MatchOuterJoins(ADD_MEMCHECK_ARGS_DECL(CollHeap* heap))
    : MatchTest(ADD_MEMCHECK_ARGS_PASS(heap))
  {}

  virtual ~MatchOuterJoins()
  {}

  /**
   * Run the Pass 1 algorithm of equi-join predicates test.
   * Implementation of superclass pure virtual method.
   * @return TRUE if the MV passed this test, FALSE if it was disqualified.
   */
  virtual NABoolean matchPass1();

  /**
   * Override MatchTest method, because the JoinPred element is different.
   * @param resultDesc The result descriptor being generated.
   * @return FALSE if the MV candidate has been disqualified.
   */
  virtual NABoolean generateDescriptor(QRCandidatePtr resultDesc);

protected:
  // Implementation of virtual methods from parent class.
  ///////////////////////////////////////////////////////////

  /**
   * Check again any predicate that involves needed extrahub tables.
   * @param pending
   * @return
   */
  virtual NABoolean matchPass2OnElement(RewriteInstructionsItemPtr pending);

  // Not used.
  virtual NABoolean generateProvidedElement(QRCandidatePtr resultDesc, RewriteInstructionsItemPtr rewrite)
  {
    return TRUE;
  }

  // Not used
  virtual NABoolean generateNotProvidedElement(QRCandidatePtr resultDesc, RewriteInstructionsItemPtr rewrite)
  {
    return TRUE;
  }

protected:
  // Internal methods.
  //////////////////////

  NABoolean addTheNotNullPred(const QRTablePtr queryTable, const BaseTableDetailsPtr mvTableDetails);

};  // class MatchOuterJoins

/**
 * 
 * 
 * 
 * 
 * 
 *****************************************************************************
 */
class AggregateCollectorVisitor : public Visitor
{
public:
  AggregateCollectorVisitor(ADD_MEMCHECK_ARGS_DECL(CollHeap* heap))
    : Visitor(ADD_MEMCHECK_ARGS_PASS(heap))
     ,aggregateFunctions_(heap)
     ,simpleColumns_(heap)
     ,joinedColumns_(heap)
  {}

  virtual ~AggregateCollectorVisitor()
  {
    aggregateFunctions_.clear();
    simpleColumns_.clear();
    joinedColumns_.clear();
  }

  virtual VisitResult visit(QRElementPtr caller);

  FunctionNodesList& getAggregateFunctionList()
  {
    return aggregateFunctions_;
  }

  ColumnNodesList& getSimpleColumnList()
  {
    return simpleColumns_;
  }

  JoinPredNodesList& getJoinedColumnList()
  {
    return joinedColumns_;
  }

private:
  FunctionNodesList aggregateFunctions_;
  ColumnNodesList   simpleColumns_;
  JoinPredNodesList joinedColumns_;
}; // class AggregateCollectorVisitor


#endif
