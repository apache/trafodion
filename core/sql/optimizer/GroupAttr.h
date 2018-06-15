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
#ifndef GROUPATTR_H
#define GROUPATTR_H
/* -*-C++-*-
*************************************************************************
*
* File:         GroupAttr.h
* Description:  The common attributes for an equivalence class
*               (Cascades Group).
* Created:      11/03/94
* Language:     C++
*
*
*
*
*************************************************************************
*/

// -----------------------------------------------------------------------

#include "BaseTypes.h"
#include "Collections.h"
#include "ValueDesc.h"
#include "EstLogProp.h"
#include "IndexDesc.h"
//QSTUFF
#include "RelScan.h"
#include "ItemOther.h"
//QSTUFF
#include "ReqGen.h"

#include "Stats.h"
#include "SharedPtrCollections.h"

// ----------------------------------------------------------------------
// contents of this file
// ----------------------------------------------------------------------
class GroupAttributes;

// ----------------------------------------------------------------------
// forward declarations
// ----------------------------------------------------------------------
////////////////////
class GroupAnalysis;
///////////////////

/*
*************************************************************************
*
* Cascades forms equivalence classes or Groups of query expressions.
* A Group is defined by the following:
*
* 1) A query expression
*
* 2) A set of values that it receives as inputs from external sources.
*    They are called the characteristic input values of the Group.
*
* 3) A set of values that it produces as its output. They are called
*    the characteristic output values of the Group.
*
* The criteria for adding another query expression to a given Group
* are that:
*
* 1) The candidate query expression must either
*
*    a) be a duplicate of an existing member of the given Group, or,
*
*    b) be derived from an existing member of the given Group
*       by means of a query transformation that is known to be
*       semantics preserving.
*
* 2) The candidate query expression has the same set of
*    characteristic input values as other members of
*    the Group.
*
* 3) The candidate query expression has the same set of
*    characteristic output values as other members of
*    the Group.
*
* If the candidate query expression fails to satisfy any one of
* the above three criteria, then it must belong to another Group.
*
* The requirement is that all members of a given Group MUST be
* provided with the same set of input values and produce the
* same set of output values. This requirement arises from the
* optimization goal of generating the best plan for a given
* query expression, given a set of input and output values.
*
* The input and output values are characteristic attributes of the
* Group. Each Group also has a set of derived attributes called
* its logical properties. The logical properties such as the
* the cardinality of the result, statistics, uniqueness are derived
* by analyzing the query expression. By virtue of belonging to a
* Group all of its members have the same logical properties.
*
* The GroupAttributes class is a repository for all the attributes
* that are common to a Group.
*
*************************************************************************
*/


class GroupAttributes : public ReferenceCounter
{
public:
  // --------------------------------------------------------------------
  // Constructor functions
  // --------------------------------------------------------------------
  GroupAttributes();

  // --------------------------------------------------------------------
  // copy ctor
  // --------------------------------------------------------------------
  GroupAttributes (const GroupAttributes &) ;

  // --------------------------------------------------------------------
  // Destructor functions
  // --------------------------------------------------------------------
  ~GroupAttributes();

  // --------------------------------------------------------------------
  // comparison
  // --------------------------------------------------------------------
  NABoolean operator == (const GroupAttributes &other) const;

  // --------------------------------------------------------------------
  // a hash function
  // --------------------------------------------------------------------
  HashValue hash() const;

  // --------------------------------------------------------------------
  // Methods for Characteristic Inputs
  // --------------------------------------------------------------------
  NABoolean isCharacteristicInput(const ValueId& vid) const
                                { return requiredInputs_.contains(vid); }

  const ValueIdSet& getCharacteristicInputs() const
                                              { return requiredInputs_; }

  void setCharacteristicInputs(const ValueIdSet & vset)
                                              { requiredInputs_ = vset; }
  void addCharacteristicInput(const ValueId& vid)
                                              { requiredInputs_ += vid; }
  void addCharacteristicInputs(const ValueIdSet& vidSet)
                                           { requiredInputs_ += vidSet; }
  void addCharacteristicInputs(const ValueIdList& vidList)
                                 { requiredInputs_.insertList(vidList); }
  void removeCharacteristicInputs(const ValueIdSet &vidSet)
                                           { requiredInputs_ -= vidSet; }

  // --------------------------------------------------------------------
  // Methods for Characteristic Outputs
  // --------------------------------------------------------------------
  NABoolean isCharacteristicOutput(const ValueId& vid) const
                               { return requiredOutputs_.contains(vid); }
  const ValueIdSet& getCharacteristicOutputs() const
                                             { return requiredOutputs_; }
  void setCharacteristicOutputs(const ValueIdSet & vset)
                                             { requiredOutputs_ = vset; }
  void addCharacteristicOutput(const ValueId& vid)
                                             { requiredOutputs_ += vid; }
  void addCharacteristicOutputs(const ValueIdSet& vidSet)
                                          { requiredOutputs_ += vidSet; }
  void addCharacteristicOutputs(const ValueIdList& vidList)
                                { requiredOutputs_.insertList(vidList); }

  // --------------------------------------------------------------------
  // Methods for Essential/Non-Essential Characteristic Outputs
  // --------------------------------------------------------------------
  NABoolean isEssentialCharacteristicOutput(const ValueId& vid) const
                    { return requiredEssentialOutputs_.contains(vid); }
  const ValueIdSet& getEssentialCharacteristicOutputs() const
                                  { return requiredEssentialOutputs_; }
  void setEssentialCharacteristicOutputs(const ValueIdSet & vset);

  void addEssentialCharacteristicOutputs(const ValueIdSet& vidSet);

  void getNonEssentialCharacteristicOutputs(ValueIdSet & vset) const ;

  // --------------------------------------------------------------------
  // GroupAttr::minimizeOutputs()
  //
  // This method takes in (a) a vidset holding non-essential outputs, (2)
  // a vidset holding essential outputs, and (3) an empty set that will hold 
  // all the outputs. The method reduces each set such that no vid is covered 
  // by all the other vids. In removing any covered essentianl vids, the 
  // values that contribute to that coverage are promoted to essential outputs
  // --------------------------------------------------------------------
  void minimizeOutputs(ValueIdSet& nonEssentialOutputs, 
                       ValueIdSet& essentialOutputs,
                       ValueIdSet& allOutputs);

  // --------------------------------------------------------------------
  // Methods for Constraints
  // --------------------------------------------------------------------
  const ValueIdSet& getConstraints() const       { return constraints_; }
  void getConstraintsOfType(OperatorTypeEnum constraintType, 
			    ValueIdSet& vidSet) const;

  void addConstraint(ItemExpr *c);

  void addConstraint(const ValueId& vid)         { constraints_ += vid; }

  void addConstraints(const ValueIdSet& vidSet)
                                              { constraints_ += vidSet; }
  void addConstraints(const ValueIdList& vidList)
                                    { constraints_.insertList(vidList); }
  void addSuitableRefOptConstraints(const ValueIdSet& vidSet);

  void addSuitableCompRefOptConstraints(const ValueIdSet& vidSet, 
					const ValueIdSet& preds,
					const RelExpr* node);

  void deleteConstraint(const ValueId& vid)      { constraints_ -= vid; }

  void deleteConstraints(const ValueIdSet& vidSet)
                                              { constraints_ -= vidSet; }
  void clearConstraints() { constraints_.clear(); }

  NABoolean isConstraint(const ValueId& vid) const
                                   { return constraints_.contains(vid); }
  NABoolean isUnique(const ValueIdSet &cols) const;
  NABoolean findUniqueCols(ValueIdSet &uniqueCols) const;
  NABoolean hasCardConstraint(Cardinality &minNumOfRows,
                              Cardinality &maxNumOfRows) const;
  NABoolean hasConstraintOfType(OperatorTypeEnum constraintType) const;
  Cardinality getMaxNumOfRows() const
                   { Cardinality x,y; hasCardConstraint(x,y); return y; }

  NABoolean canEliminateOrderColumnBasedOnEqualsPred(ValueId col) const;
  NABoolean tryToEliminateOrderColumnBasedOnEqualsPred(ValueId col,
                                                       const ValueIdSet *preds);

  // ---------------------------------------------------------------------
  // Methods on group persistent estimates
  // ---------------------------------------------------------------------
  inline RowSize getRecordLength() const         { return recordLength_; }
  inline void setRecordLength(RowSize v)            { recordLength_ = v; }
  inline RowSize getInputVarLength() const     { return inputVarLength_; }
  inline void setInputVarLength(RowSize v)        { inputVarLength_ = v; }

  inline Int32 getNumBaseTables() const           { return numBaseTables_; }
  inline void setNumBaseTables(Int32 v)
                             { numBaseTables_ = MAXOF(numBaseTables_,v); }
  inline Int32 getNumTMUDFs() const                 { return numTMUDFs_; }
  inline void setNumTMUDFs(Int32 v)                    { numTMUDFs_ = v; }

  inline CostScalar getMinChildEstRowCount() const       { return minChildEstRowCount_; }
  void setMinChildEstRowCount(CostScalar v)
                             { DCMPASSERT (v >= csZero) ;
				v.roundIfZero() ;
				minChildEstRowCount_ = v; }

  void setOutputCardinalityForEmptyLogProp (CostScalar v)
                             { DCMPASSERT (v >= csZero) ;
				v.roundIfZero() ;
				outputCardinalityForEmptyLogProp_ = v; }

  inline void resetNumBaseTables(Int32 v)            { numBaseTables_ = v; }
  inline Int32 getNumJoinedTables() const
  {
    // QSTUFF
    // this prevent heuristics from
    // treating a generic update tree like
    // any other join tree causing TSJ
    // rules to be disabled.
    if (genericUpdateRoot_)
      return 1;
    else
      return numJoinedTables_;
    // QSTUFF
  }
  inline void setNumJoinedTables(Int32 v)       // can be set only once
                      { if (numJoinedTables_ == 1) numJoinedTables_ = v; }

  inline void resetNumJoinedTables(Int32 v)            { numJoinedTables_ = v; }

  // QSTUFF
  // ---------------------------------------------------------------------
  // Methods to set and query stream logical properties
  // ---------------------------------------------------------------------
  inline void setStream(NABoolean b) { stream_ = b; };
  inline NABoolean isStream() const          { return stream_; }

  inline void setSkipInitialScan(NABoolean b) { skipInitialScan_ = b; };
  inline NABoolean isSkipInitialScan() const          { return skipInitialScan_; }

  // indicates the embedded IUD operation contained within the group
  inline void setEmbeddedIUD(OperatorTypeEnum o)
    { embeddedIUD_ = o; };
  inline OperatorTypeEnum getEmbeddedIUD()
    { return embeddedIUD_; };
  inline NABoolean isEmbeddedUpdateOrDelete() const
    { return (isEmbeddedUpdate() || isEmbeddedDelete());}
  inline NABoolean isEmbeddedUpdate() const
    { return (embeddedIUD_ == REL_UNARY_UPDATE);}
  inline NABoolean isEmbeddedDelete() const
    { return (embeddedIUD_ == REL_UNARY_DELETE);}
 inline NABoolean isEmbeddedInsert() const
    { return (embeddedIUD_ == REL_UNARY_INSERT);}

  // indicates whether a node/group is at the root of a  generic update subtree
  inline void setGenericUpdateRoot(NABoolean b) { genericUpdateRoot_ = b; };
  inline NABoolean isGenericUpdateRoot() const  { return genericUpdateRoot_;}

  // indicates whether query tree needs to be reordered to satisfy the
  // constraints for embedded update and delete or stream operators
  // all of them have to be the leftmost child of the query tree
  inline void setReorderNeeded(NABoolean reorderNeeded) { reorderNeeded_ = reorderNeeded; };
  inline NABoolean reorderNeeded() const        { return reorderNeeded_;}

  inline const ValueIdSet& getGenericUpdateRootOutputs() const
    { return genericUpdateRootOutputs_; }
  inline void setGenericUpdateRootOutputs(const ValueIdSet & vset)
    { genericUpdateRootOutputs_ = vset; }

  // This method returns the operation as a string to be used
  // in reporting errors. 
  // This method is primarily used for reporting errors
  // on embedded IUD. Most error messages are common to all IUD,
  // just differing in the operation name.

  const NAString getOperationWithinGroup() const;

  inline void setHasRefOptConstraint(NABoolean b) { hasRefOptConstraint_ = b; };
  inline NABoolean hasRefOptConstraint() const  { return hasRefOptConstraint_; }

  inline void setHasCompRefOptConstraint(NABoolean b) 
    { hasCompRefOptConstraint_ = b; };
  inline NABoolean hasCompRefOptConstraint() const  
    { return hasCompRefOptConstraint_; }

  // QSTUFF

  // --------------------------------------------------------------------
  // Methods for Input and Output Estimated Logical Properties
  // --------------------------------------------------------------------
  SHPTR_LIST(EstLogPropSharedPtr) & inputLogPropList() { return inputEstLogProp_; }
  const SHPTR_LIST(EstLogPropSharedPtr) & getInputLogPropList() const
                                             { return inputEstLogProp_; }

  SHPTR_LIST(EstLogPropSharedPtr) & outputLogPropList() { return outputEstLogProp_; }
  const SHPTR_LIST(EstLogPropSharedPtr) & getOutputLogPropList() const
                                            { return outputEstLogProp_; }

  SHPTR_LIST(EstLogPropSharedPtr) & intermedOutputLogPropList()
                                           { return intermedOutputLogProp_; }
  const SHPTR_LIST(EstLogPropSharedPtr) & getIntermedOutputLogPropList() const
                                           { return intermedOutputLogProp_; }

  // Get the respective (intermediate or final) output estimated logical
  // property (OLP), given the provided input estimated logical
  // property(ILP).

  // If the requested OLP has not been synthesized, then synthesize "on
  // demand".  Since the estLogProp may not be synthesized by the time
  // they are requested, this method must side-effect the EstLogProp
  // self. Therefore, no const version for it can be constructed.
  EstLogPropSharedPtr outputLogProp (const EstLogPropSharedPtr& inputLogProp);
  EstLogPropSharedPtr intermedOutputLogProp (const EstLogPropSharedPtr& inputLogProp);

  // Answers an often-asked question : what is the result cardinality of
  // the output log prop when given an "empty" input logprop?
  CostScalar getResultCardinalityForEmptyInput ();

  CostScalar getResultMaxCardinalityForEmptyInput ();
  CostScalar getResultMaxCardinalityForInput(EstLogPropSharedPtr & inLP);

  // synthesize, if not already synthesized, the input or output
  // estLogProp for the child of a Materialize Operator.  NOTE: the
  // following two routines should Only be used in synthesizing the output
  // estLogProp for the child of a Materialize Operator.
  EstLogPropSharedPtr
  materializeInputLogProp(const EstLogPropSharedPtr& inLP, Int32 *multipleReads);
  EstLogPropSharedPtr
  materializeOutputLogProp (const EstLogPropSharedPtr& inLP, Int32 *numOfCalls);

  // See if the provided est. input logical property exists?  If so, return
  // the index in the list; otherwise, return -1.
  Int32 existsInputLogProp (const EstLogPropSharedPtr& inputLP) const;

  // Checks whether the OLP for the provided ILP has been synthesized.
  NABoolean isPropSynthesized (const EstLogPropSharedPtr& inputLogProp) const;

  // Add a reference to the provided ILP, and allocate a corresponding OLP.
  NABoolean addInputOutputLogProp (const EstLogPropSharedPtr& inputLogProp,
                                   const EstLogPropSharedPtr& outputLogProp,
                                   const EstLogPropSharedPtr& intermedOutputLogProp = NULL);

  void setLogExprForSynthesis (RelExpr * expr) { logExprForSynthesis_ = expr; }
  RelExpr * getLogExprForSynthesis () const    { return logExprForSynthesis_; }

  NABoolean existsLogExprForSynthesis() const {
    return (logExprForSynthesis_ == NULL ? FALSE : TRUE); }

  // ---------------------------------------------------------------------
  // Methods regarding the clearing/setting of logical properties
  // ---------------------------------------------------------------------
  void clearLogProperties();

  void clearAllEstLogProp();
  // --------------------------------------------------------------------
  // reconcile() merges two compatible group attribute, i.e.,
  // those that have the same characteristic inputs and outputs.
  // ('this' is the merged one, 'other' remains unchanged)
  // --------------------------------------------------------------------
  void reconcile (GroupAttributes &other) { lomerge(other, FALSE); }

  // --------------------------------------------------------------------
  // merge() performs an unconditional merge of two Group Attributes.
  // --------------------------------------------------------------------
  void merge(GroupAttributes &other) { lomerge(other, TRUE); }

  // --------------------------------------------------------------------
  // Method for normalizing the Characteristic Inputs and Outputs.
  // --------------------------------------------------------------------
  void normalizeInputsAndOutputs(NormWA & normWARef);

  // --------------------------------------------------------------------
  // Methods for availableBtreeIndexes
  // --------------------------------------------------------------------
  SET(IndexDesc *) & availableBtreeIndexes(){ return availableBtreeIndexes_;}
  const SET(IndexDesc *) & getAvailableBtreeIndexes() const
                                            { return availableBtreeIndexes_;}
  void addToAvailableBtreeIndexes(const SET(IndexDesc *) & newIndexes)
  {  availableBtreeIndexes_.insert(newIndexes);}

  // --------------------------------------------------------------------
  // Methods for predicate pushdown
  // --------------------------------------------------------------------

  // --------------------------------------------------------------------
  // coverTest()
  //
  // A method to determine whether a given set of expressions are
  // covered, i.e., they can be satisfied, by the ValueIds that appear
  // a) in the GroupAttributes of a relational operator or
  // b) a set of new input values that the caller is willing to provide.
  //
  // This method is called by an operator for a specific child, i.e.,
  //         J -> predicates     the join operator can invoke
  //        / \                  GroupAttr::coverTest() for either
  // scan T1   scan T2           one or both of the scans that are its
  //                             children
  // Parameters:
  //
  // ValueIdSet setOfExprOnParent
  //    IN:  a read-only reference to a set of expressions that
  //         are associated with the parent.
  //
  // ValueIdSet newInputsSuppliedByParent
  //    IN : a read-only reference to a set of new external inputs
  //         (ValueIds) that are provided for evaluating the above
  //         expressions.
  //
  // ValueIdSet coveredExpr
  //    OUT: a subset of setOfExprOnParent. It contains the ValueIds
  //         of only those expressions that are covered.
  //
  // ValueIdSet referencedInputs
  //    OUT: a subset of newInputsSuppliedByParent. It contains the
  //         ValueIds of only those inputs that are referenced in
  //         the expressions that belong to coveredExpr.
  //
  // ValueIdSet *coveredSubExpr
  //    OUT: It contains the ValueIds of all those sub-expressions
  //         that are covered in the set (setOfExprOnParent-coveredExpr)
  //         However, the expression that contain them are not covered.
  //
  // ValueIdSet *unCoveredExpr
  //    OUT: If non-null, unCoveredExpr contains the value ids of all
  //         those expressions and/or subexpressions that could not
  //         be covered by the group attributes and the new inputs. In
  //         other words, unCoveredExpr contains the minimum set of
  //         additional value ids needed to cover all of the expressions
  //         in "setOfExprOnParent".
  // --------------------------------------------------------------------
  void coverTest(const ValueIdSet& setOfExprOnParent,
                 const ValueIdSet& newInputsSuppliedByParent,
                 ValueIdSet& coveredExpr,
                 ValueIdSet& referencedInputs,
                 ValueIdSet* coveredSubExpr = NULL,
                 ValueIdSet* unCoveredExpr = NULL) const;

  // --------------------------------------------------------------------
  // covers()
  //
  // A method to determine whether a given expression is covered,
  // i.e., its can be satisfied, by the ValueIds that appear
  // a) in the GroupAttributes of a relational operator or
  // b) a set of new input values that the caller is willing to provide.
  //
  // Parameters:
  //
  // ValueId  exprId
  //    IN : a read-only reference to the ValueId of an expression
  //         that is to be checked for coverage
  //
  // ValueIdSet newInputsSuppliedByParent
  //    IN : a read-only reference to a set of new external inputs
  //         (ValueIds) that are provided for evaluating the above
  //         expressions.
  //
  // ValueIdSet referencedInputs
  //    OUT: a subset of newInputsSuppliedByParent. It contains the
  //         ValueIds of only those inputs that are referenced in
  //         the expression exprId.
  //
  // ValueIdSet *coveredSubExpr
  //    OUT: It contains the ValueIds of all those sub-expressions
  //         of exprId that are covered, while exprId itself may
  //         not be covered.
  //
  // ValueIdSet *unCoveredExpr
  //    OUT: If non-null, unCoveredExpr contains the value ids of all
  //         those expressions and/or subexpressions that could not
  //         be covered by the group attributes and the new inputs. In
  //         other words, unCoveredExpr contains the minimum set of
  //         additional value ids needed to cover all of the expressions
  //         in "setOfExprOnParent".
  //
  // Returns TRUE : If ValueId is covered
  //         FALSE: Otherwise.
  //
  // --------------------------------------------------------------------
  NABoolean covers(const ValueId& exprId,
                   const ValueIdSet& newInputsSuppliedByParent,
                   ValueIdSet& referencedInputs,
                   ValueIdSet* coveredSubExpr = NULL,
                   ValueIdSet* unCoveredExpr = NULL) const;

  // --------------------------------------------------------------------
  // computeCharacteristicIO()
  //
  // A method to recompute the Characteristic Inputs and Outputs.
  //
  // This method is called by an operator for a specific child, i.e.,
  //         J -> predicates     the join operator can invoke
  //        / \                  GroupAttr::computeCharacteristicIO()
  // scan T1   scan T2           for eitherone or both of the scans
  //                             that are its children
  //
  // Parameters:
  //
  // ValueIdSet newInputs
  //    IN : a read-only reference to a set of input values that
  //         the parent is willing to provide
  //
  // ValueIdSet exprOnParent
  //    IN : a read-only reference to a set of expressions (ValueIds)
  //         that will be evaluated on the parent.
  //
  // ValueIdSet outputExprOnParent
  //    IN : a read-only reference to a set of expressions (ValueIds)
  //         that will be produced by the parent. Any output that 
  //         the parent produces that it is simply passing through
  //         from its children without using them is classified as 
  //         an non-essential output.
  //
  // ValueIdSet essentialChildOutputs
  //    IN : a read-only reference to a set of expressions (ValueIds)
  //         that are marked essential for the calling node's grandchildren.
  //         These grandchild nodes are the direct children of the node
  //         whose IO is being computed. This input parameter is used to
  //         enforce the rule, 
  //         If an output is essential in any of my children, then if the same
  //         valueid is present in my output, it is essential for me too.
  //
  // ValueIdSet* selPred
  //    IN : a read-only pointer to the selection preds of the calling node
  //         this input param is used only when the following input param
  //         childofaleftjoin is TRUE
  //
  // NABoolean childOfALeftJoin
  //    IN : TRUE => if calling node is a left join.
  //
  // NABoolean optimizeOutputs
  //    IN : FALSE => do not optimize characteristics outputs
  // These are especially needed for nested joins, as the parent cannot
  // produce anything by itself
  //
  // Effect :
  // The Group Attributes for the child can get new characteristic
  // inputs and outputs.
  // --------------------------------------------------------------------
  void computeCharacteristicIO(const ValueIdSet& newInputs,
                               const ValueIdSet& exprOnParent,
			       const ValueIdSet& outputExprOnParent,
			       const ValueIdSet& essentialChildOutputs,
                               const ValueIdSet * selPred = NULL,
			       const NABoolean childOfALeftJoin = FALSE,
			       const NABoolean optimizeOutputs = TRUE,
                               const ValueIdSet *extraHubEssOutputs = NULL);


  // --------------------------------------------------------------------
  // resolveCharacteristicInputs()
  //
  // A method for replacing each VEGReference that is a member of
  // the Characteristic Inputs with one or more values that belong
  // to the VEG and are also available in the external inputs.
  //
  // This method is used by the code generator.
  //
  // Effect :
  // The Characteristic Inputs can change.
  // --------------------------------------------------------------------
  void resolveCharacteristicInputs(const ValueIdSet& externalInputs);

  // --------------------------------------------------------------------
  // resolveCharacteristicOutputs()
  //
  // A method for replacing each VEGReference that is a member of
  // the Characteristic Outputs with one value that belongs to the
  // VEG and is also available in the the output values produced
  // by the children.
  //
  // This method is used by the code generator.
  //
  // Effect :
  // The Characteristic Outputs can change.
  // --------------------------------------------------------------------
  void resolveCharacteristicOutputs(const ValueIdSet & childOutputs,
                                    const ValueIdSet & externalInputs);

  // --------------------------------------------------------------------
  // replaceOperandsOfInstantiateNull()
  //
  // This method is used by the code generator for rewriting all
  // those expressions in the Characteristic Outputs that contain
  // an InstantiateNull. The operand of such an instantiateNull
  // MUST be a value produced by the inner/right/second table of
  // a LeftJoin. This method is provided in order to prevent the
  // substitution of such values by other members of its VEG
  // that are external input values.
  //
  // Effect :
  // The Characteristic Outputs can change.
  // --------------------------------------------------------------------
  void replaceOperandsOfInstantiateNull(const ValueIdSet & childOutputs,
                                        const ValueIdSet & inputValues)
  {
    requiredOutputs_.replaceOperandsOfInstantiateNull(
         childOutputs,inputValues);
  }

// -----------------------------------------------------------------------
// Recommended order for NJ probing.
// Find a good forward probing order for this group. A preferred probing
// order is chosen as the key prefix of any available Btree index
// that is covered by either constants or params/host vars
// and at least one equijoin column, has the minimum number of
// uncovered columns, and satisfys all partitioning and ordering
// requirements.
// Inputs: child0 Group Attributes
//         Number of forced ESPs (-1 if no forcing)
//         Requirement Generator object containing left child requirements
//         Any required order or arrangement for the right child
// Outputs: chosenIndexDesc
//          Indicator if all part key cols are equijoin cols or
//          covered by a partitioning requirement that has no part key cols
// Returns: A list that is a prefix of the chosen index sort key.
// -----------------------------------------------------------------------
 
ValueIdList recommendedOrderForNJProbing( 
                               GroupAttributes* child0GA, // IN
                               Lng32 numForcedParts, //IN
                               RequirementGenerator& rg, // IN
                               ValueIdList& reqdOrder1, // IN
                               ValueIdSet& reqdArr1, // IN
                               IndexDesc* &chosenIndexDesc, // OUT
                               NABoolean partKeyColsAreMappable // OUT
                              );

  inline GroupAnalysis * getGroupAnalysis()
  {
    return groupAnalysis_;
  }

  void clearGroupAnalysis();

  // This method is used by the debugger for displaying estLogProps
  // It gets the list of inputEstLogProps that have been used to compute
  // outputEstLogProps for this group. If these are cacheable, it will
  // get the stats from the ASM cache, form a list and return to the debugger

  SHPTR_LIST (EstLogPropSharedPtr) * getCachedStatsList();

  // This method will provide information about skewness of data for a column.
  // The skewness will indicate the highest frequency among all the intervals
  // and will be returned as a fraction of the entire row count of histogram.

  CostScalar getSkewnessFactor(const ValueId vId,
	       EncodedValue & mostFreqVal,
	       const EstLogPropSharedPtr& inLP = (*GLOBAL_EMPTY_INPUT_LOGPROP));

  // This method will return a list of skew values associated with the
  // left child of the equal predicate such that formula
  // rowcount/totalRowCount > threshold holds for eaceh such value.
  // vId is the value Id of the equal predicate.
  SkewedValueList* getSkewedValues(const ValueId& vId,
               double threshold,
               NABoolean& statsExist,
               const EstLogPropSharedPtr& inLP = (*GLOBAL_EMPTY_INPUT_LOGPROP),
               NABoolean includeNullIfSkewed = FALSE
                                  );
  
 double getAverageVarcharSize(const ValueId vId,
                             const EstLogPropSharedPtr& inLP = (*GLOBAL_EMPTY_INPUT_LOGPROP));

 double getAverageVarcharSize(const ValueIdList &  vidList, UInt32 & maxRowSize);

  // methods to set, get and update group potential
  void setPotential(Int32 potential) { potential_ = potential; };
  Int32 getPotential() const { return potential_;};
  Int32 updatePotential(Int32 potential)
  {
    if ((potential != -1) &&
        (potential_ == -1))
    {
      potential_ = potential;
    }

    return potential_;
  };

  // Does the group contain non-deterministic UDRs?
  void setHasNonDeterministicUDRs(NABoolean b)
  { hasNonDeterministicUDRs_ = b; }
  NABoolean getHasNonDeterministicUDRs() const
  { return hasNonDeterministicUDRs_; }

  ColStatsSharedPtr getColStatsForSkewDetection(
                                     const ValueId vId,
                                     const EstLogPropSharedPtr& inLP);

  void setIsProbeCacheable(NABoolean b=TRUE)
  { probeCacheable_ = b; }
  NABoolean getIsProbeCacheable() const
  { return probeCacheable_; }

  void setSeabaseMDTable(NABoolean b = FALSE)
                { isSeabaseMD_ = b; }
  NABoolean isSeabaseMDTable() const  { return isSeabaseMD_; }

private:

  // --------------------------------------------------------------------
  // lomerge() is the low-level merge utility that is used by merge()
  // an reconcile(). If the flag mergeCIO() is set, then it also merges
  // the Characteristic Inputs and Outputs.
  // --------------------------------------------------------------------
  void lomerge(GroupAttributes &other, NABoolean mergeCIO);

  // --------------------------------------------------------------------
  // Characteristic Inputs
  // --------------------------------------------------------------------
  ValueIdSet  requiredInputs_;

  // --------------------------------------------------------------------
  // Characteristic Outputs
  // --------------------------------------------------------------------
  ValueIdSet  requiredOutputs_;

  // --------------------------------------------------------------------
  // Characteristic Outputs that 
  // a) are needed by immediate parent to evaluate expressions
  // and are simply passed up to the granparent nodes OR
  // b) have been used to evaluate any expressions in children
  // nodes all the way down to leaf nodes.
  // --------------------------------------------------------------------
  ValueIdSet  requiredEssentialOutputs_;


  // --------------------------------------------------------------------
  // Constraints
  // --------------------------------------------------------------------
  ValueIdSet  constraints_;
  // The next two members indicate whether the constraints_ object contains
  // constraints of type RefOptConstraint and ComplementaryRefOptConstraint
  // respectively. These flags have been added for improved performance.
  NABoolean hasRefOptConstraint_;
  NABoolean hasCompRefOptConstraint_;

  // ---------------------------------------------------------------------
  // Estimates that persists for a group, independent of different
  // input estimated logical properties.
  // ---------------------------------------------------------------------
  RowSize          recordLength_;      // estimated size of a record
  RowSize          inputVarLength_;    // estimated size of input parameters

  // values used to avoid bushy join trees
  Int32          numBaseTables_;    // # of base tables involved in this subtree
  Int32          numJoinedTables_;  // # of tables in join backbone (see note below)
  Int32          numTMUDFs_;        // # of table-mapping UDFs in this subtree

  // Note: For numJoinedTables_, we calculate this when we create the
  // GroupAttributes for an expression for the first time. For most nodes
  // this will be 1; for a Join node it will be more than 1 in general.
  // Later, when we form Groups for a given expression, this initial value
  // applies to the Group. Optimization will in general add more expressions
  // to a given group. For example, a Group originally associated with a
  // GroupByAgg node might get a Join expression if the GroupByOnJoinRule
  // chooses to push a Group By below a Join. When this happens, though,
  // we leave numJoinedTables_ as 1 in the GroupAttributes (see
  // Join::synthLogProp), which will inhibit the LeftShiftJoinRule from
  // firing on that Join. This is important, because we want the large
  // scope rules to control what join orders are enumerated.

  // QSTUFF VV
  // --------------------------------------------------------------------
  // attribute to determine whether RelExpr is a select with an embedded
  // insert, update or delete statement
  // ---------------------------------------------------------------------
  OperatorTypeEnum embeddedIUD_;

  // --------------------------------------------------------------------
  // attribute to determine whether RelExpr is a continous data stream
  // not producing end-of-data (EOD)
  // ---------------------------------------------------------------------
  NABoolean stream_;

  // --------------------------------------------------------------------
  // attribute to determine whether RelExpr stream has the property that
  // causes it to skip the initial scan.
  // ---------------------------------------------------------------------
  NABoolean skipInitialScan_;

  // --------------------------------------------------------------------
  // attribute to determine whether a node is at the root of a generic
  // update tree. If so we prevent predicates from being pushed into the
  // subtree by forcing them not to be covered by the coverTest method.
  // ---------------------------------------------------------------------
  NABoolean genericUpdateRoot_;

  // stream, embedded delete or update require query tree to be reordered
  NABoolean reorderNeeded_;

  // attribute to determine RelExpr is seabase MD Table
  NABoolean isSeabaseMD_;

  // we need this to check whether an index qualifies, i.e. covers all
  // the outputs of the genericupdate subtree - we can't just simply look
  // at the update outputs as those may inlcude values needed for index
  // maintenance which are unrelated to the user query.
  ValueIdSet  genericUpdateRootOutputs_;
  // QSTUFF ^^

  // ---------------------------------------------------------------------
  // Referenced Logical Expression for which synthesis is being performed.
  //   This reference is set upon the first invocation of synthLogProp()
  //   for this set of Group Attributes.
  // ---------------------------------------------------------------------
  RelExpr * logExprForSynthesis_;

  // ---------------------------------------------------------------------
  // Input Estimated Logical Properties
  // ---------------------------------------------------------------------
  SHPTR_LIST(EstLogPropSharedPtr) inputEstLogProp_;

  // ---------------------------------------------------------------------
  // Intermediate Output Estimated Logical Properties
  // ---------------------------------------------------------------------
  SHPTR_LIST(EstLogPropSharedPtr) intermedOutputLogProp_;

  // ---------------------------------------------------------------------
  // Final Output Estimated Logical Properties
  // ---------------------------------------------------------------------
  SHPTR_LIST(EstLogPropSharedPtr) outputEstLogProp_;

  // ---------------------------------------------------------------------
  // Output Estimated Cardinality -- cached so we don't have to call the
  // (relatively) expensive routine existsInputLogProp() as often, when
  // the caller only wants to know the group's rowcount.
  // ---------------------------------------------------------------------
  CostScalar outputCardinalityForEmptyLogProp_;

  CostScalar outputMaxCardinalityForEmptyLogProp_;

  // ---------------------------------------------------------------------
  // Set of all available indexes that can be used by a parent Nested Join
  // ---------------------------------------------------------------------
  SET(IndexDesc *) availableBtreeIndexes_;

  // ---------------------------------------------------------------------
  // This object contains analysis result for this group
  // ---------------------------------------------------------------------
  GroupAnalysis* groupAnalysis_;

  // ---------------------------------------------------------------------
  // This contains the minimum row count of all the tables involved in this
  // group. It is used to better estimate the cardinality of join, if the
  // tables are being joined on more than one column and there is no multi-
  // column uec available for that set of columns
  // ----------------------------------------------------------------------
  CostScalar minChildEstRowCount_;

  // cached skew values in relationship to EqualPredicates
  NAHashDictionary<ValueId,SkewedValueList>* cachedSkewValuesPtr_;

  //group's potential
  Int32 potential_;

  // Does the group contain non-deterministic UDRs?
  NABoolean hasNonDeterministicUDRs_;

  // is cacheable per probe
  NABoolean probeCacheable_;

}; // class GroupAttributes

#endif /* GROUPATTR_H */










