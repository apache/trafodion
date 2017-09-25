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
#ifndef ITEMEXPR_H
#define ITEMEXPR_H
/* -*-C++-*-
******************************************************************************
*
* File:         ItemExpr.h
* Description:  Item expression base class ItemExpr
* Created:      5/11/94
* Language:     C++
*
*
*
*
******************************************************************************
*/


#include "charinfo.h"		// for CollationAndCoercibility
#include "ComASSERT.h"
#include "ExprNode.h"
#include "ItemExprList.h"
#include "ObjectNames.h"
#include "ValueDesc.h"
#include "QRExprElement.h"
#include "IndexDesc.h"
#include "ComKeyMDAM.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class ExprValueId;
class ItemExpr;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
class Attributes;
class BindWA;
class CacheWA;
class ConstValue;
class ConstantParameter;
class RandomNum;
class SelParameter;
class Generator;
class DisjunctArray;
class ex_clause;
class MdamCodeGenHelper;
class MdamPred;
class VEGReference;
class VEGRewritePairs;
class ColStatDescList;

#define CONST_32K 32768

// -----------------------------------------------------------------------
// An ExprValueId object is a pointer to an ItemExpr object. The pointer
// may be expressed through a true pointer (ItemExpr *) or through a
// ValueId object. This class hides the actual implementation and also
// allows its users to prevent updates to the object. See also class
// ExprGroupId in file RelExpr.h.
// -----------------------------------------------------------------------
class ExprValueId : public NABasicObject
{
public:

  // state of the object
  enum ItemExprMode
  {
    STANDALONE,
    MEMOIZED
  };

  // constructors

  ExprValueId();
  ExprValueId(const ExprValueId &other);
  ExprValueId(ItemExpr * exprPtr);
  ExprValueId(const ValueId & exprId);

  // assignment operators
  ExprValueId & operator = (const ExprValueId & other);
  ExprValueId & operator = (ItemExpr * other);
  ExprValueId & operator = (const ValueId & other);

  // compare an ExprValueId with another ExprValueId or ItemExpr * or ValueId
  NABoolean operator == (const ExprValueId &other) const;
  inline NABoolean operator != (const ExprValueId &other) const
                                           { return NOT operator == (other); }

  NABoolean operator == (const ItemExpr *other) const;
  inline NABoolean operator != (const ItemExpr *other) const
                                           { return NOT operator == (other); }
  NABoolean operator == (const ValueId &other) const;
  inline NABoolean operator != (const ValueId &other) const
                                           { return NOT operator == (other); }

  // dereferencing operator, this class works like a pointer
  inline ItemExpr * operator ->() const;	// defined below class ItemExpr

  // same as a "normal" method
// warning elimination (removed "inline")
  ItemExpr * getPtr() const;		// defined below class ItemExpr

  // cast into an ItemExpr *
  inline operator ItemExpr *() const;		// defined below class ItemExpr

  // cast into a ValueId
  inline operator ValueId() const                     { return getValueId(); }

// warning elimination (removed "inline")
  NAColumn *getNAColumn(NABoolean okIfNotColumn = FALSE) const
			   { return getValueId().getNAColumn(okIfNotColumn); }

  // return the ValueId
  ValueId getValueId() const;

  // return the mode of the pointer
  inline ItemExprMode getMode() const                    { return exprMode_; }

  // change modes
  void convertToMemoized();
  void convertToStandalone();

private:

  enum ItemExprMode exprMode_;
  ItemExpr    * exprPtr_;   // used for the STANDALONE and MEMOIZED mode
  ValueId       exprId_;    // used for the MEMOIZED mode

}; // class ExprValueId

// declarations for ItemExpr::treeWalk below
typedef ItemExpr * (*ItemTreeWalkFunc)(ItemExpr *,
                                       CollHeap *outHeap,
                                       void *context);
enum ItemTreeWalkSeq { ITM_PREFIX_WALK, ITM_POSTFIX_WALK };

// -----------------------------------------------------------------------
// A generic item expression node.
// -----------------------------------------------------------------------



class ItemExpr : public ExprNode
{

public:

  // default constructor
  ItemExpr(OperatorTypeEnum otype,
	   ItemExpr *child0 = NULL,
	   ItemExpr *child1 = NULL);

  // copy constructor
  ItemExpr(const ItemExpr& s);

  // virtual destructor
  virtual ~ItemExpr();

  // Delete this node without deleting its subtree.
  void deleteInstance();

  // operator [] is used to access the children of a tree
  virtual ExprValueId & operator[] (Lng32 index);
  virtual const ExprValueId & operator[] (Lng32 index) const;

  // flip tree without extra resources.
  ItemExpr * reverseTree();

  // for cases where a named method is more convenient than operator []
  ExprValueId & child(Lng32 index)      		{ return operator[](index); }
  const ExprValueId & child(Lng32 index) const	{ return operator[](index); }

  // return a reference (can be used as an lvalue)
  Lng32 &currChildNo()				{ return currChildNo_; }

  virtual NABoolean operator == (const ItemExpr& other) const;

  virtual NABoolean isEquivalentForCodeGeneration(const ItemExpr * other);

  NABoolean hasBaseEquivalenceForCodeGeneration(const ItemExpr * other);

  virtual ConstValue *castToConstValue(NABoolean & negate);

  // a method for initializing the ValueId in the item expression.
  void setValueId(ValueId valId)		{ valId_ = valId; }
  ValueId getValueId() const			{ return valId_; }

  // a method for allocating the ValueId for an ItemExpr
  void allocValueId();

  // perform a safe type cast (return NULL ptr for illegal casts)
  virtual ItemExpr *castToItemExpr();
  const ItemExpr *castToItemExpr() const;

  // Ways of determining if *this represents any "IS NOT NULL" predicate
  // or in particular, a "col IS NOT NULL" pred.
  //
  ItemExpr *getChildIfThisIsISNOTNULL() const
  {
    if (getOperatorType() == ITM_IS_NOT_NULL)
      return child(0);
    else if (getOperatorType() == ITM_NOT &&
    	     child(0)->getOperatorType() == ITM_IS_NULL)
      return child(0)->child(0);
    else
      return NULL;
  }

  NABoolean isISNOTNULL() const
  {
    ItemExpr *ie = getChildIfThisIsISNOTNULL();
    if (ie) return TRUE;
    if (getOperatorType() == ITM_AND)
      return child(0)->isISNOTNULL() && child(1)->isISNOTNULL();
    return FALSE;
  }

  void getColumnsIfThisIsISNOTNULL(ItemExprList &il, NABoolean) const
  {
    ItemExpr *ie = getChildIfThisIsISNOTNULL();
    if (ie)
      {
	if (ie->getOperatorType() == ITM_REFERENCE  ||
	    ie->getOperatorType() == ITM_BASECOLUMN ||
	    ie->getOperatorType() == ITM_INDEXCOLUMN)
	  il.insert(ie);
      }
    else
      {
        child(0)->getColumnsIfThisIsISNOTNULL(il, TRUE);
        child(1)->getColumnsIfThisIsISNOTNULL(il, TRUE);
      }
  }

  NABoolean getColumnsIfThisIsISNOTNULL(ItemExprList &il) const
  {
    il.clear();
    if (!isISNOTNULL()) return FALSE;
    getColumnsIfThisIsISNOTNULL(il, TRUE);
    return il.entries() > 0;
  }

  // methods required for traversing an ExprNode tree:
  // non-const & const access a child of an ExprNode, replace a particular child
  virtual ExprNode *getChild(Lng32 index)           { return child(index); }
  virtual const ExprNode *getConstChild(Lng32 index) const {return child(index);}
  virtual void setChild(Lng32 index, ExprNode *);

  ItemExpr *containsRightmost(const ItemExpr *ie);

  // The recognition for common subexpressions or a transformation
  // can cause this scalar expression to be replaced with another one.
  // The original expression remembers its replacement expression.
  //
  void setReplacementExpr(ItemExpr * replacementExpr)
  					{ replacementExpr_ = replacementExpr; };
  virtual ItemExpr * getReplacementExpr() const	{ return replacementExpr_; };

  // Take an item expression and remove all nodes from its top that form
  // a "backbone". Then add the value ids of all the remaining nodes to
  // a ValueIdSet.
  Int32 convertToValueIdSet(ValueIdSet &vs,
			  BindWA *bindWA = NULL,
			  OperatorTypeEnum backboneType = ITM_AND,
                          NABoolean transformSubqueries = TRUE,
                          NABoolean flattenLists = FALSE);

  // Take an item expression and remove all nodes from its top that form
  // a "backbone". Then add the value ids of all the remaining nodes to
  // a ValueIdList.
  Int32 convertToValueIdList(ValueIdList &vl,
			    BindWA *bindWA = NULL,
			    OperatorTypeEnum backboneType = ITM_ITEM_LIST,
                            RelExpr *parent = NULL);

  // rewrite this expression by replacing all value ids in it with
  // the equivalent mapped value ids; return a new value id for the result
  // Also add to the map the mapping form the old to the new expression
  virtual ValueId mapAndRewrite(ValueIdMap &map,
				NABoolean mapDownwards = FALSE);

  // The method is similar to the mapAndRewrite method. Except it looks
  // for the bottom value from the given index of the list.
  // This is to take care of duplicates appearing in the
  // bottomValue list. Sol: 10-040416-5166
  virtual ValueId mapAndRewriteWithIndx(ValueIdMap &map,
	  				CollIndex i);
  virtual NABoolean hasEquivalentProperties(ItemExpr * other);
  NABoolean hasBaseEquivalence(ItemExpr * other);
  
  ItemExpr * changeDefaultOrderToDesc();

  ValueId removeInverseFromExprTree( NABoolean & invExists );
  ItemExpr* removeInverseFromExprTree( NABoolean & invExists,
                                     const NABoolean onlyDouble );
  
  static void removeNotCoveredFromExprTree( ItemExpr* &ie, 
                                            const ValueIdSet &seqColumns,
                                            NABoolean rootNode = TRUE);
  
  NABoolean containsSequenceFunctions();



  // This method encapsulates the decision about which scalar expression
  // operators can belong to a VEG.
  virtual NABoolean isAColumnReference( );

  // return true iff maxSelectivity(this) == selectivity(this)
  NABoolean maxSelectivitySameAsSelectivity() const;

  // starts the ItemExpr tree anchored by the this pointer and 
  // checks if childNum[0] th child is of type opType[0] and if so
  // gets that child and proceeds to the same check with childNum[1]
  // and opType[1], and so on. The objective is to walk down a specific
  // tree looking for a certain node. The specific tree is given by the
  // two input arguments. If the required node is not found, NULL is returned.
  // Used currently by LPAD and RPAD.
  ItemExpr* getParticularItemExprFromTree(NAList<Lng32>& childNum, 
                                          NAList<OperatorTypeEnum>& opType) const;

  // This method checks if the ItemExpr represents a OR reprdicate that can be
  // transformed into an IN subquery (i.e. implemented using a semijoin)
  // only logical conditions are checked by this method and a TRUE/FALSE
  // reply is provided. If the return value is TRUE valuesListIE will contain
  // all the constants in the IN List. This datamember is cleared upon entry 
  // into the method. numParams is the number of params/hostvars in the INList,
  // it is guaranteed to be less than equal to number of entries in the first 
  // argument.
  NABoolean canTransformToSemiJoin(ItemExprList& valuesListIE, TableDesc* tdesc,
                           Lng32& numParams, ValueId& colVid, CollHeap* h) const;


  void transformOlapFunctions(CollHeap *wHeap);

 private:
  // This method has the code that is common to
  // mapAndRewrite() and mapAndRewriteWithIndex().
  ValueId mapAndRewriteCommon(ValueIdMap &map,
			      NABoolean mapDownwards);

 public:
  // Recursively walk this item expression tree and try to fold constants
  // (e.g. by transforming the expression "5 + 3" to "8"). Note that
  // at this time there is only limited intelligence built into this
  // method. Errors (such as 1/0) and warnings are reported in the diags area.
  // Unless allowed by setting newTypeSynthesis to TRUE, the folded expression
  // will have the same type as the original expression.
  virtual ItemExpr *foldConstants(ComDiagsArea *diagsArea,
				  NABoolean newTypeSynthesis = FALSE);
  ItemExpr *foldConstants(BindWA *bindWA);

  // Find common subexpressions in an expression, for example
  // (C1 AND C2 AND C3) OR (C1 AND C2 AND C4) OR (C1 AND C5 AND C2 AND C6),
  // and apply the inverse distributivity law to pull them out. This
  // results in this example in a new expression:
  // (C1 AND C2) AND (C3 OR C4 OR (C5 AND C6))
  // The method may someday take operators other than OR and AND as well
  // (e.g. + and *), but that will require some minor code changes.
  ItemExpr *applyInverseDistributivityLaw(
       OperatorTypeEnum backboneType = ITM_OR,
       OperatorTypeEnum innerType = ITM_AND);
  // little helper for the above method
  ItemExpr *connect2(OperatorTypeEnum op, ItemExpr *op1, ItemExpr *op2);

  // a virtual function for performing name binding within the query tree
  // The following method performs name binding, allocates a ValueId and
  // synthesizes the resultant type for the expression.
  virtual ItemExpr *bindNode(BindWA *bindWA);

  // a virtual function for performing name binding within the query tree
  // The following method performs name binding, allocates a ValueId and
  // synthesizes the resultant type for the UDFs or Subqueries in 
  // the expression.
  virtual ItemExpr *bindUDFsOrSubqueries(BindWA *bindWA);

  // this should be the entry point to bind an item expr tree,
  // as the method checks the incomplete type and fills the
  // unknown. It calls _bindNodeRoot() to do the real work. This
  // method has the choice of returning "this" or the expression
  // returned by _bindNodeRoot().
  virtual ItemExpr *bindNodeRoot(BindWA *bindWA);

  // prepare the expression to be bound again, e.g. after changing
  // a child pointer, something that should be done only if we are
  // sure the expression is not used elsewhere.
  void unBind() { markAsUnBound(); setValueId(NULL_VALUE_ID); }

  virtual NABoolean isTypeComplete(const NAType*) { return TRUE; };
  virtual void reportTypeIsIncomplete() {};

  void bindChildren(BindWA *bindWA); // a method for binding the children
  void bindSelf(BindWA *bindWA); // a method for binding the children and self

  // a method for binding host variables, dynamic parameters and constants.
  ItemExpr *bindUserInput(BindWA *, const NAType *, const NAString &);

  OperatorTypeEnum  origOpType() const		{ return origOpType_; }
  void  setOrigOpType(OperatorTypeEnum o)	{ origOpType_ = o; }

  //OperatorTypeEnum &origOpTypeBeingBound()	{ return origOpTypeBeingBound_; }
  //Int32 &origOpTypeCounter()                      { return origOpTypeCounter_;}

  // the NON-virtual encapsulating function for type-propagating the node
  // (calls the virtual one, below)
  const NAType *synthTypeWithCollateClause(BindWA *bindWA,
					   const NAType *type = NULL);

  // a virtual function to determine whether an itemexpr can relax
  // the charset matching rule for its children WITHOUT examining
  // whether the children have Unicode hostvars
  virtual NABoolean isRelaxCharTypeMatchRulesPossible() {return FALSE;};

  // a virtual function to determine whether an itemexpr's type can
  // be relaxed.
  virtual NABoolean isCharTypeMatchRulesRelaxable() { return FALSE; };

  // relax the charset matching rule for Unicode hostvars
  virtual ItemExpr* tryToRelaxCharTypeMatchRules(BindWA *bindWA);

  ItemExpr* performImplicitCasting(CharInfo::CharSet cs, BindWA *bindWA);
  virtual ItemExpr* tryToDoImplicitCasting(BindWA *bindWA);
  Int32       shouldPushTranslateDown(CharInfo::CharSet chrset) const;

  virtual NABoolean CanChild0BeImplicitlyCast() { return TRUE; };

  // a virtual function for type-propagating the node
  virtual const NAType *synthesizeType();

  // for expressions created after binding, do type synthesis w/o bind
  // If the redriveTypeSynthesisFlag is set, it synthesises iself.
  // If the redriveChildTypeSynthesis is set, it synthesizes the
  // types for the entire subtree, from the leaves upto this operator,
  // once again.
  virtual void synthTypeAndValueId(NABoolean redriveTypeSynthesisFlag = FALSE,
				   NABoolean redriveChildTypeSynthesis = FALSE);
  virtual void synthTypeAndValueId2(NABoolean redriveTypeSynthesisFlag = FALSE,
				    NABoolean redriveChildTypeSynthesis = FALSE);

  // Propagate type information down the ItemExpr tree.
  // Called by coerceType().  The default implementation
  // does nothing.  Currently is only redefined by ValueIdUnion
  // to propagate the desired type to the sources of the ValueIdUnion.
  //
  virtual const NAType *pushDownType(NAType &desiredType,
                               enum NABuiltInTypeEnum defaultQualifier
                               = NA_UNKNOWN_TYPE);

  virtual void coerceChildType(NAType& desiredType,
                enum NABuiltInTypeEnum defaultQualifier);

  // set/get resolving-incomplete-type status. Used by charset inference.
  virtual void setResolveIncompleteTypeStatus(NABoolean x)
    { resolveIncompleteType_ = x; }

  NABoolean getResolveIncompleteTypeStatus()
    { return resolveIncompleteType_ ; }

  void setpreCodeGenNATypeChangeStatus()
    { preCodeGenNATypeChange_ = TRUE;}

  NABoolean isPreCodeGenNATypeChanged()
    { return preCodeGenNATypeChange_;}
  // --------------------------------------------------------------------
  // transformNode()
  //
  // transformNode() is an overloaded name, which is used for a set
  // of methods that implement the transformation phase of query
  // normalization.
  //
  // We use the term query tree for a tree of relational operators,
  // each of which can contain none or more scalar expression trees.
  // The transformations performed by transformNode() brings scalar
  // expressions into a canonical form. The effect of most such
  // transformations is local to the scalar expression tree.
  // However, the transformation of a subquery requires a semijoin
  // to be performed between the relational operator that contains
  // the subquery and the query tree for the subquery. The effect
  // of such a subquery transformation is therefore visible not
  // only in the scalar expression tree but also in the relational
  // expression tree.
  //
  // Originally, transformNode() was a virtual method on ExprNode.
  // The relational as well as scalar expressions are derived from
  // ExprNode and used to provide proprietary implementations for it.
  // However, relational and scalar expressions have different
  // processing requirements during the transformation phase. Hence,
  // each of them now support a virtual transformNode() method that
  // differ in their interfaces.
  //
  // transformNode() uses four parameters, namely,
  // 1) the normalizer work area
  // 2) a location that contains the pointer to the (Rel/Item)Expr that
  //    is currently being processed.
  // 3) a location in the query tree that can be updated by a
  //    subquery tranformation to introduce a new semijoin
  // 4) the set of values that are available as external inputs to
  //    the RelExpr operator whose ItemExprs are being processed
  //    by transformNode().
  //
  //
  // Parameters:
  //
  // NormWA & normWARef
  //    IN : a pointer to the normalizer work area
  //
  // ExprValueId & locationOfPointerToMe
  //    IN : a reference to the location that contains a pointer to
  //         the ItemExpr that is currently being processed.
  //
  // ExprGroupId & introduceSemijoinHere
  //    IN : a reference to the location in the query tree where a
  //         subquery transformation can introduce a semijoin. It
  //         is usually the location of the pointer to the relational
  //         operator with which the given predicate tree is associated.
  //
  // const ValueIdSet &  externalInputs
  //    IN : the set of values that are available as external inputs to
  //         the RelExpr operator whose ItemExprs are being processed
  //         by transformNode().
  //
  // --------------------------------------------------------------------
  virtual void transformNode(NormWA & normWARef,
			     ExprValueId & locationOfPointerToMe,
                             ExprGroupId & introduceSemiJoinHere,
			     const ValueIdSet & externalInputs);

  // --------------------------------------------------------------------
  // transformToRelExpr()
  //
  // transformToRelExpr() is an overloaded name, it is used during 
  // transformation by those classes that needs to convert an ItemExpr
  // into a RelExpr.
  //
  // At the moment, it is used for subqueries and Isolated Routines.
  //
  // --------------------------------------------------------------------
  virtual void transformToRelExpr(NormWA & normWARef,
			     ExprValueId & locationOfPointerToMe,
                             ExprGroupId & introduceSemiJoinHere,
			     const ValueIdSet & externalInputs);

  // A method for inverting (finding the inverse of) the operators
  // in a subtree that is rooted in a NOT.
  virtual ItemExpr * transformSubtreeOfNot(NormWA & normWARef,
                                           OperatorTypeEnum falseOrNot);

  // A virtual method for transforming a multi-value predicate
  // to ANDs and ORs of simpler subconditions,
  // but only if all (by default) children are ItemList.
  // Returns NULL if no transformation was necessary or possible.
  virtual ItemExpr * transformMultiValuePredicate(
					  NABoolean flattenSubqueries = TRUE,
					  ChildCondition tfmIf = ANY_CHILD);

  NABoolean referencesTheGivenValue(
              const ValueId & exprId,
              NABoolean doNotDigInsideVegRefs = FALSE,
              NABoolean doNotDigInsideInstNulls = FALSE) const;

  // does this item expression reference one of the set of ValueIds?
  NABoolean referencesOneValueFrom(const ValueIdSet & vs) const;

  // Is the item exprId the same value id, or if I'm a VEG is it
  // contained in my set of values?
  NABoolean containsTheGivenValue(const ValueId & exprId) const;


  // Constant fold myself by evaluating all constants in the expression
  // rooted at myself. During the process, 'this' object is not simplified.
  // Return NULL if 'this' contains at least one non-constant or 
  // the evluation is not successful.
  ItemExpr* constFold();

  ItemExpr * createMirrorPred(ItemExpr *compColPtr, 
                              ItemExpr *compColExprPtr,
                              const ValueIdSet &underlyingCols);
  //Does 'this' ItemExpr evaluate to constant e.g.
  //If we go by the strict definition then
  //Sin(Cos(1)+1) will return TRUE.
  //Sin(?p) will return FALSE.
  //If we don't use the strict definition then
  //Sin(?p) would be a constant and therefore
  //we will get a return value of TRUE.
  //The reason for this is that Sin(?p) is constant
  //for a particular execution of a statement as
  //?p remains constant for given execution of a statment
  //And not only dynamic parameters like ?p but host vars
  //like :h also stay constant for a particular execution
  //of a query, therefore Sin(:h) will also return TRUE.
  NABoolean doesExprEvaluateToConstant(NABoolean strict = TRUE,
                                       NABoolean considerVEG = FALSE) const;

  // This method returns TRUE or FALSE depending whether the optimizer
  // should use stats to compute selectivity or use default selectivity
  NABoolean useStatsForPred();

  // get leaf expression if the cardinality for this expression can be
  // estimated using histograms
  ItemExpr * getLeafValueIfUseStats(NABoolean digIntoInstantiateNull = TRUE);

  // get the value ids of all leaf nodes (arity 0) in this expression tree
  void getLeafValueIds(ValueIdSet &lv) const;
  void getLeafValueIds(ValueIdList &lv) const;

  // get the value ids of all leaf nodes (arity 0) in this expression tree
  void getLeafValueIdsForCaseExpr(ValueIdSet &lv);

  // The following two methods are used during the transformation
  // phase of normalization aka transformNode().

  // predicateEliminatesNullAugmentedRows() determines whether the
  // predicate is capable of discarding null augmented rows
  // produced by a left join.
  virtual NABoolean predicateEliminatesNullAugmentedRows(NormWA &, ValueIdSet &);

  // initiateInnerToLeftJoinTransformation() is invoked when
  // a predicateEliminatesNullAugmentedRows().
  virtual ItemExpr * initiateLeftToInnerJoinTransformation(NormWA &);

  // Each operator supports a (virtual) method for transforming its
  // query tree to a canonical form. The parameter setOfPredExpr is
  // supplied only when predicates are normalized.
  virtual ItemExpr * normalizeNode(NormWA & normWARef);
  virtual ItemExpr * normalizeNode2(NormWA & normWARef);

  // A helper function. Used by preCodeGen to convert external types
  // to internal types.
  ItemExpr * convertExternalType(Generator *);

  // virtual method to fixup tree for code generation.
  virtual ItemExpr * preCodeGen(Generator *);

  // virtual method to do code generation
  virtual short codeGen(Generator*);

  // virtual method to generate Mdam predicates
  virtual short mdamPredGen(Generator * generator,
                            MdamPred ** head,
                            MdamPred ** tail,
                            MdamCodeGenHelper & mdamHelper,
                            ItemExpr * parent);

  // virtual method to get details of an mdam predicate, currently used only by
  // BiRelat subclass.
  virtual void getMdamPredDetails(Generator* generator,
                                  MdamCodeGenHelper& mdamHelper, 
                                  enum MdamPred::MdamPredType& predType,
                                  ex_expr** vexpr)
    { CMPASSERT(FALSE); }


  // virtual method to generate MDAM_BETWEEN predicate. Redefined only for
  // BiLogic subclass.
  virtual void mdamPredGenSubrange(Generator* generator,
                                   MdamPred** head,
                                   MdamPred** tail,
                                   MdamCodeGenHelper& mdamHelper)
    { CMPASSERT(FALSE); }

  virtual void codegen_and_set_attributes(Generator *, Attributes **,Lng32);


  inline ex_clause * getClause(){return clause_;};
  inline void setClause(ex_clause * clause){clause_ = clause;};

  // A method that determines if an item expression contains a subquery
  virtual NABoolean containsSubquery();

  // A method that determines if an item expression contains a UDF
  virtual ItemExpr *containsUDF();

  // A method that determines if an item expression contains an Isolated
  // UDFunction
  virtual NABoolean containsIsolatedUDFunction();

  // A method that determines if an item expression contains a valueIdProxy
  // that is derived from given valueId
  virtual NABoolean containsValueIdProxySibling(const ValueIdSet &siblings);

  // A method that determines if an item expression contains an aggregate of
  // type ITM_ONE_ROW
  NABoolean containsOneRowAggregate();

  // A method that determines if an item expression contains an opType of
  // type OperatorTypeEnum
  NABoolean containsOpType(OperatorTypeEnum opType) const;

  // A method that determines whether an item expression contains a child
  // node (or grandchild node, or ...) that is untransformed.  If it does,
  // then the node itself, and the path to that child (or children) is
  // also marked as untransformed.
  NABoolean markPathToUnTransformedNode() ;


  // A method that changes OneRow aggregate expression to a list
  ItemExpr* transformOneRowAggregate( NormWA &);
  ItemExpr* removeOneRowAggregate( ItemExpr *, NormWA &);

  // A method to determine whether this is an equijoin predicate
  NABoolean isAnEquiJoinPredicate(
       const GroupAttributes* const leftGroupAttr,
       const GroupAttributes* const rightGroupAttr,
       const GroupAttributes* const joinGroupAttr,
       ValueId & leftChildValueId,
       ValueId & rightChildValueId,
       NABoolean & isOrderPreserving) const;

  // determines whether this is a join predicate for
  // an operator under a nested join
  NABoolean isANestedJoinPredicate (
       const ValueIdSet& inputValues,
       const ValueIdSet& operatorValues) const;

  // determines whether a predicate references a constant
  // in any of its operands (it may be a complicated
  // predicate expression). Also, all the predicates that have
  // constants are collected in constExprs.
  void accumulateConstExprs (ValueIdSet & constExprs);
  // is the predicate an equality predicate equating to a constant:
  NABoolean equatesToAConstant () const;
  // is the predicate an equality predicate equating to a constant, a
  // host variable, or a parameter:
  NABoolean equatesToAConstExpr () const;


  // A method that determines whether evaluation of this predicate will preserve order
  virtual NABoolean isOrderPreserving () const;

  // A method that determines if an item expression is a subquery expression
  virtual NABoolean isASubquery() const;

  // An indicator whether this item expression is an aggregate function
  // (aggregate functions can only be evaluated in groupby nodes).
  virtual NABoolean isAnAggregate() const;
  virtual NABoolean containsAnAggregate() const;

  // An indicator whether this item expression is a predicate.
  virtual NABoolean isAPredicate() const;

  // An indicator whether this item expression is an expansion of a LIKE predicate.
  // This is redefined only for class BiLogic.
  virtual NABoolean isLike() const { return FALSE; }

  // An indicator whether this item expression is a sequence function.
  virtual NABoolean isASequenceFunction () const;

  virtual NABoolean isOlapFunction () const;

  // An indicator whether this item expression contains a THIS function.
  virtual NABoolean containsTHISFunction();

  // A transformation method that places a "NotTHIS" wrapper around an expression
  virtual void transformNotTHISFunction();

  // A transformation method for protecting sequence functions from not
  // being evaluated due to short-circuit evaluation.
  //
  virtual void protectiveSequenceFunctionTransformation(Generator *generator);

  // A method that returns for "user-given" input values.
  // These are values that are either constants, host variables, parameters,
  // or even values that are sensed from the environment such as
  // current time, the user name, etcetera.
  virtual NABoolean isAUserSuppliedInput() const;

  // Is this a subclass of GenericUpdateOutputFunction BuildinFunction?
  virtual NABoolean isAGenericUpdateOutputFunction() const { return FALSE; }

  // a method that finds all occurences of a certain operator in an
  // item expression tree
  void findAll(OperatorTypeEnum wantedType,
	       ValueIdSet & result,
	       NABoolean visitVEGMembers = FALSE,
	       NABoolean visitIndexColDefs = FALSE);

  template <class Result>
  void findAllT(OperatorTypeEnum wantedType,
	       Result& result,
	       NABoolean visitVEGMembers = FALSE,
	       NABoolean visitIndexColDefs = FALSE);

  // A variation of the above method that saves the matching ItemExprs in a
  // ItemExprList. The order of occurences of these ItemExprs is maintained.
  void findAll(OperatorTypeEnum wantedType,
	       ItemExprList& result,
	       NABoolean visitVEGMembers = FALSE,
	       NABoolean visitIndexColDefs = FALSE);

  // Find the number of elements in the expression tree, and the maximum tree depth.
  Lng32 getTreeSize(Lng32& maxDepth, NABoolean giveUpThreshold);

  // Find all eqaulity columns in an item expression tree.
  void findEqualityCols(ValueIdSet & result);

  // execute transformation function f on each node of the tree and
  // return the transformed tree
  ItemExpr *treeWalk(ItemTreeWalkFunc f,
                     CollHeap *outHeap = NULL,
                     enum ItemTreeWalkSeq sequence = ITM_POSTFIX_WALK,
                     void *context = NULL);

  // --------------------------------------------------------------------
  // isCovered() : (for a ValueId)
  //
  // A method to determine whether a given ValueId is covered, i.e,
  // it can be satisfied, by
  // 1) the ValueIds that appear in the GroupAttributes of a relational
  //    operator and
  // 2) a set of external dataflow inputs that are provided.
  //
  // An Item Expression isCovered() if each of its children isCovered().
  //
  // Parameters:
  //
  // ValueIdSet newExternalInputs
  //    IN : a read-only reference to a set of new external inputs
  //         (ValueIds) that will be provided to the new RelExpr anchor
  //         for evaluating the given predicate factor.
  //
  // GroupAttributes  newRelExprAnchorGA
  //    IN : the Group Attributes of the new RelExpr anchor for
  //         the predicate factor.
  //
  // ValueIdSet referencedInputs
  //    OUT: a subset of newExternalInputs. It contains the ValueIds
  //         ValueIds of those inputs that are referenced in this
  //         ItemExpr.
  //
  // ValueIdSet coveredSubExpr
  //    OUT: It contains the ValueIds of all those sub-expressions
  //         of this item expression that are covered, while the
  //         containing item expression may not be covered.
  //
  // ValueIdSet unCoveredExpr
  //    OUT: If non-null, unCoveredExpr contains the value ids of all
  //         those expressions and/or subexpressions that could not
  //         be covered by the group attributes and the new inputs. In
  //         other words, unCoveredExpr contains the minimum set of
  //         additional value ids needed to cover the item expression.
  //
  // Returns TRUE : If ValueId is covered by newRelExprGA and the
  //                newExternalInputs
  //         FALSE: Otherwise.
  //
  // --------------------------------------------------------------------
  virtual NABoolean isCovered(const ValueIdSet& newExternalInputs,
			      const GroupAttributes& newRelExprAnchorGA,
	   	              ValueIdSet& referencedInputs,
			      ValueIdSet& coveredSubExpr,
			      ValueIdSet& unCoveredExpr) const;

  // --------------------------------------------------------------------
  // Walk through an ItemExpr tree and gather the ValueIds of those
  // expressions that behave as if they are "leaves" for the sake of
  // the coverage test, e.g., expressions that have no children, or
  // aggregate functions, or instantiate null. These are usually values
  // that are produced in one "scope" and referenced above that "scope"
  // in the dataflow tree for the query.
  // --------------------------------------------------------------------
  virtual void getLeafValuesForCoverTest(ValueIdSet & leafValues, 
                                         const GroupAttributes& coveringGA,
                                         const ValueIdSet & newExternalInputs) const;


  // --------------------------------------------------------------------
  // replaceVEGExpressions()
  //
  // A method for replacing VEGReference and VEGPredicate objects
  // with another expression that belongs to the VEG as well as to the
  // set of availableValues.
  //
  // The lookup parameter provides a way to make replaceVEGExpressions
  // idempotent when this is desired.  This is useful in contexts where
  // a VEGPredicate must be evaluated twice but using different physical
  // implementations (e.g. when it is used to compute a begin or end key, but
  // also must be part of an Executor predicate, all within the same
  // scan node).  If lookup is NULL, then replaceVEGExpressions is not
  // idempotent, and will raise an assertion error when asked to process
  // a VEGPredicate twice.
  //
  // If replicateExpression is set, then a copy of the expression tree
  // is returned in which VEGReferences have been replaced.
  //
  // thisIsAKeyPredicate=TRUE indicates that the rewrite logic must
  // make sure that a key predicate is generated from the VEG.
  // 
  // In order to guarantee a keyColumn is returned when the 
  // thisIsAKeyPredicate flag is set, we also need the indexDesc parameter
  //
  // The two optional left_ga and right_ga 
  // are used when we resolve expressions in hash joins as we need to 
  // guarantee that an expression on the left side of an equipred only 
  // contains values avaiable on the left and vice versa. 
  //
  // --------------------------------------------------------------------
  virtual ItemExpr * replaceVEGExpressions
                        (const ValueIdSet& availableValues,
                         const ValueIdSet& inputValues,
                         NABoolean thisIsAKeyPredicate = FALSE,
                         VEGRewritePairs * lookup = NULL,
                         NABoolean replicateExpression = FALSE,
                         const ValueIdSet *joinInputAndPotentialOutput = NULL,
                         const IndexDesc *iDesc = NULL,
                         const GroupAttributes *left_ga = NULL,
                         const GroupAttributes *right_ga = NULL);

  ItemExpr * replaceVEGExpressions1( VEGRewritePairs* lookup );
  void       replaceVEGExpressions2( Int32 index
                                   , const ValueIdSet& availableValues
                                   , const ValueIdSet& inputValues
                                   ,       ValueIdSet& currAvailableValues
                                   , const GroupAttributes * left_ga
                                   , const GroupAttributes * right_ga
                                   );


  // ---------------------------------------------------------------------
  // ItemExpr::replaceOperandsOfInstantiateNull()
  // This method is used by the code generator for replacing the
  // operands of an ITM_INSTANTIATE_NULL with a value that belongs
  // to availableValues.
  // ---------------------------------------------------------------------
   void replaceOperandsOfInstantiateNull(const ValueIdSet &,
                                         const ValueIdSet & inputValues );

  // check whether this item expression delivers its values in the
  // same order as another item expression, assuming this is an output
  // value in the specified group
  virtual OrderComparison sameOrder(ItemExpr *other,
				    NABoolean askOther = TRUE);
  virtual OrderComparison sameOrder(VEGReference *other,
				    NABoolean askOther = TRUE);
  virtual ItemExpr * simplifyOrderExpr(OrderComparison *newOrder = NULL);

  // Replace VEG Reference by the constant contained in the VEG.
  // This is done for all the VEG References rooted directly or
  // indirectly at this. If no constant is found, this expression
  // is not simpfified and a NULL is returned.
  virtual ItemExpr * simplifyBeforeConstFolding();

  // Return the only constant itemexpr contained in a VEG pointed by 'this'.
  // If 'this' is not a VEG or has more than one constant, return NULL.
  virtual ItemExpr* getConstantInVEG();


  virtual ItemExpr * removeInverseOrder();

  virtual SimpleHashValue hash(); // from ExprNode
  virtual HashValue topHash();    // for this and all subclasses
  HashValue treeHash();
  virtual NABoolean duplicateMatch(const ItemExpr & other) const;
  NABoolean genericDuplicateMatch(const ItemExpr &other) const;
  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);

  // "virtual copy constructor", provides a generic method to duplicate
  // the top node of an expression tree or an entire tree
  ItemExpr * copyTree(CollHeap* outHeap=0);

  // Is this operator supported by the synthesis functions?
  virtual NABoolean synthSupportedOp() const { return FALSE; }

  // Return the default selectivity for this predicate
  virtual double defaultSel();
  
  // This method will apply the selectivity of default predicate on histograms
  virtual NABoolean applyDefaultPred(ColStatDescList & histograms,
                                                 OperatorTypeEnum exprOpCode,
                                                 ValueId predValueId,
                                     NABoolean & globalPredicate,
                                     CostScalar *maxSelectivity=NULL);

  // This method will apply the selectivity of unsupported default predicate on histograms
  NABoolean applyUnSuppDefaultPred(ColStatDescList & histograms,
                                   ValueId predValueId,
                                   NABoolean & globalPredicate);

  // Helper method for applyDefaultPred methods
  NABoolean checkForStats(ColStatDescList & histograms, 
				  CollIndex & columnIndex, 
				  ValueIdSet & leafValues);

  NABoolean calculateUecs(ColStatDescList & histograms,
                                 CostScalar & minUec,
                                 CostScalar & maxUec);

  void resetRealBigNumFlag(ItemExpr *node);

  virtual NABoolean calculateMinMaxUecs(ColStatDescList & histograms,
					       CostScalar & minUec,
					       CostScalar & maxUec);
  

  //++MV
  // Return the selectivityFactor for this predicate.
  // The selectivity factor is initialy set to -1, which means
  // that the default selectivity is used. If the selectivity
  // factor is set to a number between 0 and 1 , this number
  // serves as the selectivity factor for this ItemExpr.
  double getSelectivityFactor() const { return selectivityFactor_; }
  void setSelectivityFactor(double selectivityFactor)
  { selectivityFactor_ = selectivityFactor; }
  //--MV

  // Return the inverse of the operator type if any.  Otherwise, return its operatorType.
  OperatorTypeEnum getInverseOpType () const;

  // ---------------------------------------------------------------------
  //  MDAM Related methods
  // ---------------------------------------------------------------------
  // This is a recursive
  // function that returns the leaf predicates of the tree
  // whose root is the ItemExpr into the ValueIdSet&.
  void getLeafPredicates(ValueIdSet& leafPredicates);

  NABoolean referencesAHostVar() const;

  // ---------------------------------------------------------------------
  //  Utility methods
  // ---------------------------------------------------------------------
  
  // Evaluate the exprssion at compile time. Assume all operands are constants.
  // Return NULL if the computation fails and CmpCommon::diags() may be side-affected.
  ConstValue* evaluate(CollHeap* heap);

  // produce an ascii-version of the object (for display or saving into a file)
  virtual void unparse(NAString &result,
		       PhaseEnum phase = DEFAULT_PHASE,
		       UnparseFormatEnum form = USER_FORMAT,
		       TableDesc * tabId = NULL) const;

  void      computeKwdAndFlags( NAString  &kwd,
                                NABoolean &prefixFns,
                                NABoolean &specialPrefixFns,
                                PhaseEnum phase,
                                UnparseFormatEnum form,
                                TableDesc * tabId) const;

  void    computeKwdAndPostfix( NAString &kwd, NAString &postfix,
                                UnparseFormatEnum form = USER_FORMAT ) const ;

  virtual void print(FILE * f = stdout,
		     const char * prefix = "",
		     const char * suffix = "") const;

  void display();

  // MDAM related methods
  // ---------------------------------------------------------------------
  // mdamTreeWalk is a virtual method.  It is relevant only for the
  // following ItemExprs at the current time:
  // - VEGPredicate
  // - BiLogic
  // - UnLogic
  // - BiRelat
  //
  // This tree walk will build a DisjunctArray for the expression sub-tree
  // that it is invoked for.  It is a recursive method which recurses down
  // the tree and then back up.  On its way up is when it returns the
  // DisjunctArray built up to that point in the recursion.
  //
  // VEGPredicate, UnLogic, BiRelat Node Precessing
  // ----------------------------------------------
  // Predicates are encountered at the VEGPredicate, UnLogic, and BiRelat
  // nodes.  At these nodes:
  // - A DisjunctArray is created.
  // - A ValueIdSet is created with the value id of the predicate.
  // - The address of the ValueIdSet is inserted into the DisjunctArray.
  //
  // BiLogic Node Processing
  // -----------------------
  // The BiLogic node is where all the main processing for MDAM gets done.
  // This is where the left and right child of an AND or OR return
  // DisjunctArrays.  These DisjunctArrays are ANDed or ORed together.
  // The resulting DisjunctArray is returned to the previous ItemExpr node
  // in the recursion.
  // ---------------------------------------------------------------------
  virtual DisjunctArray * mdamTreeWalk();

  NABoolean containsColumn();

    // See coments for previousHostVar_ in private section
  NABoolean &previousHostVar()
  {
    return previousHostVar_;
  }

  NAString &previousName()
  {
    return previousName_;
  }

  CollationAndCoercibility *& collateClause()	{ return collateClause_; }
  static void cleanupPerStatement();

  virtual ConstantParameter *castToConstantParameter() { return NULL; }

  // perform a safe type cast to RandomNum (return NULL ptr for illegal casts)
  virtual RandomNum *castToRandomNum() { return NULL; }

  virtual const SelParameter *castToSelParameter() const { return NULL; }

  // does this entire ItemExpr qualify query to be cacheable after this phase?
  virtual NABoolean isCacheableExpr(CacheWA& cwa);

  // is this single ExprNode cacheable after this phase?
  NABoolean isCacheableNode(CmpPhase phase) const;

  // mark this ExprNode as cacheable for this and later phases
  void setCacheableNode(CmpPhase phase);

  // is any literal in this expr safely coercible to its target type?
  virtual NABoolean isSafelyCoercible(CacheWA& cwa) const;

  // append an ascii-version of ItemExpr into cachewa.qryText_
  virtual void generateCacheKey(CacheWA& cachewa) const;

  // A helper routine for ItemExpr::generateCacheKey()
  void addSelectivityFactor( CacheWA& cwa ) const ;

  // return a string that identifies the operator. same as getText() except
  // we prepend char set to string literals. This is to fix genesis case
  // 10-040616-0347 "NF: query cache does not work properly on || for certain
  // character set". If we change ConstValue::getText() for this fix, we risk
  // breaking QA tests due to expected output diffs.
  virtual const NAString getText4CacheKey() const { return getText(); }

  // change literals of a cacheable query into constant parameters
  virtual ItemExpr* normalizeForCache(CacheWA& cwa, BindWA& bindWA);

  // return true if ItemExpr & its descendants have no constants
  // and no noncacheable nodes
  virtual NABoolean hasNoLiterals(CacheWA& cwa);

  // return TRUE if this node can be used in a group by or order by expr.
  // returns FALSE otherwise. Optionally sets error in diags area.
  virtual NABoolean canBeUsedInGBorOB(NABoolean setErr);

  // Defined only for HV's and Dynamic parameters
  virtual ComColumnDirection getParamMode () const;
  virtual Int32 getOrdinalPosition () const;
  virtual Int32 getHVorDPIndex () const;
// warning elimination (removed "inline")
  virtual void setPMOrdPosAndIndex( ComColumnDirection paramMode,
					   Int32 ordinalPosition,
                                    Int32 index) { CMPASSERT (0);}

  NABoolean isARangePredicate() const;

  // get and set for flags_. See enum Flags.
  NABoolean constFoldingDisabled()   { return (flags_ & CONST_FOLDING_DISABLED) != 0; }
  void setConstFoldingDisabled(NABoolean v)
  { (v ? flags_ |= CONST_FOLDING_DISABLED : flags_ &= ~CONST_FOLDING_DISABLED); }

  // get and set for flags_. See enum Flags.
  NABoolean inGroupByOrdinal()   { return (flags_ & IN_GROUPBY_ORDINAL) != 0; }
  void setInGroupByOrdinal(NABoolean v)
  { (v ? flags_ |= IN_GROUPBY_ORDINAL : flags_ &= ~IN_GROUPBY_ORDINAL); }

  NABoolean inOrderByOrdinal()   { return (flags_ & IN_ORDERBY_ORDINAL) != 0; }
  void setInOrderByOrdinal(NABoolean v)
  { (v ? flags_ |= IN_ORDERBY_ORDINAL : flags_ &= ~IN_ORDERBY_ORDINAL); }

  NABoolean isSelectivitySetUsingHint()   { return (flags_ & SELECTIVITY_SET_USING_HINT) != 0; }
  void setSelectivitySetUsingHint(NABoolean v = TRUE)
  { (v ? flags_ |= SELECTIVITY_SET_USING_HINT : flags_ &= ~SELECTIVITY_SET_USING_HINT); }

  // get and set for flags_. See enum Flags.
  NABoolean isGroupByExpr()   const { return (flags_ & IS_GROUPBY_EXPR) != 0; }
  void setIsGroupByExpr(NABoolean v)
  { (v ? flags_ |= IS_GROUPBY_EXPR : flags_ &= ~IS_GROUPBY_EXPR); }

  // get and set for IS_RANGESPEC_ITEM_EXPR flag.
  NABoolean isRangespecItemExpr() const { return (flags_ & IS_RANGESPEC_ITEM_EXPR) != 0; }
  void setRangespecItemExpr(NABoolean v = TRUE)
  { (v ? flags_ |= IS_RANGESPEC_ITEM_EXPR : flags_ &= ~IS_RANGESPEC_ITEM_EXPR); }

  NABoolean wasDefaultClause()   const { return (flags_ & WAS_DEFAULT_CLAUSE) != 0; }
  void setWasDefaultClause(NABoolean v)
  { (v ? flags_ |= WAS_DEFAULT_CLAUSE : flags_ &= ~WAS_DEFAULT_CLAUSE); }

  NABoolean isGroupByRollup()   const { return (flags_ & IS_GROUPBY_ROLLUP) != 0; }
  void setIsGroupByRollup(NABoolean v)
  { (v ? flags_ |= IS_GROUPBY_ROLLUP : flags_ &= ~IS_GROUPBY_ROLLUP); }

  virtual QR::ExprElement getQRExprElem() const;

  virtual ItemExpr* removeRangeSpecItems(NormWA* normWA = NULL);

  // output degree functions for ItemExprs that can have 
  // multiple outputs (such as scalar UDFs and subqueries).
  virtual Int32       getOutputDegree()     { return 1; }
  virtual ItemExpr *getOutputItem(UInt32 i) { return this; }

protected:
  // ---------------------------------------------------------------------
  // This function does the real work of bind nodes for an expression.
  // It calls bindNode on the tree and then fills any incomplete
  // type information. Right now, it takes care of missing charset.
  // ---------------------------------------------------------------------
  virtual ItemExpr *_bindNodeRoot(BindWA *bindWA);

  // A virtual function performing the actual charset matching rule relaxation
  // on "this" ItemExpr by adding a translate node on top of "this". Return "this"
  // if relaxation is done successfully or not necessary; return NULL if the relaxation
  // failed.
  virtual ItemExpr* performRelaxation(CharInfo::CharSet csTranslatedTo, BindWA *bindWA);

  // A special version of the virtual function performing the actual charset
  // matching rule relaxation for two ExprValueIds.
  // Returns TRUE if the relaxation is successfully done and one of the ExprValueIds
  // is changed, or relaxation is not necessary; return FALSE if the relaxation failed.
  virtual NABoolean performRelaxation(ExprValueId* ie1, ExprValueId* ie2, BindWA *bindWA);


private:

  enum Flags
  {
    // if set, then constant folding is disabled for this node.
    // The child nodes are folded, if allowed.
    CONST_FOLDING_DISABLED = 0x0001,

    // if set, then subtree rooted below this ItemExpr has been referenced
    // as a group by ordinal.
    // Do not validate that all columns that are part of this expr
    // need to be specified in the group by list.
    IN_GROUPBY_ORDINAL = 0x0002,

    // This flag indicates whether selectivity hint was specified
    SELECTIVITY_SET_USING_HINT = 0x0004,

    // if set, this ItemExpr (and the subtree rooted below)is a grouping
    // column. This flag is used to ensure that translate expressions are
    // not pushed below a grouping column. A grouping column is an expression
    // specified in the GROUP BY clause
    IS_GROUPBY_EXPR = 0x0008,
    
    // If set, the subtree rooted by this ItemExpr was derived from a rangespec
    // (see OptRangeSpec::getRangeItemExpr()). This flag is consulted to avoid
    // repeating the rangespec analysis process when the expression is in more
    // than one node.
    IS_RANGESPEC_ITEM_EXPR = 0x0010,

    // if set, then subtree rooted below this ItemExpr has been referenced
    // as an order by ordinal.
    IN_ORDERBY_ORDINAL = 0x0020,

    // If set, this subtree was created while processing the DEFAULT
    // clause in DefaultSpecification::bindNode. 
    WAS_DEFAULT_CLAUSE = 0x0040,

    // if set, the subtree rooted below was part of "groupby rollup" clause.
	// Currently used during parsing phase. See parser/sqlparser.y.
    IS_GROUPBY_ROLLUP  = 0x0080
  };

  // ---------------------------------------------------------------------
  // The value identifier for this expression.
  // This value is filled in by the binder for every expression.
  // ---------------------------------------------------------------------
  ValueId valId_;

  // ---------------------------------------------------------------------
  // The children of the expression tree (for expressions with more than
  // two children, the indexing operator is redefined and uses other
  // data members).
  // ---------------------------------------------------------------------
  ExprValueId inputs_[MAX_ITM_ARITY];

  // ---------------------------------------------------------------------
  // The index (0-based position) of the child currently being bound;
  // after binding, it's one greater than the maximum index,
  // i.e. the degree of the item -- see ItemExpr::bindSelf for more notes.
  //
  // Before transform/normalize, could be reset to 0 and again loop-incremented.
  // Should this be made more general and moved to ExprNode?
  // ---------------------------------------------------------------------
  Lng32 currChildNo_;

  // ---------------------------------------------------------------------
  // SqlParser lets every value expression be followed by a COLLATE clause.
  // When we're binding and have determined the type of the expr,
  // we disallow it for non-CHARACTER-type exprs, and apply it to the CharTypes.
  // ---------------------------------------------------------------------
  CollationAndCoercibility * collateClause_;

  // ---------------------------------------------------------------------
  // A pointer to the scalar expression that replaces this expression
  // during the transformation/normalization phase.
  // ---------------------------------------------------------------------
  ItemExpr * replacementExpr_;

  // ---------------------------------------------------------------------
  // A pointer to the generated clause for this ItemExpr.
  // Set and used at code generation time to fixup stuff in
  // the generated clause after code generation.
  // For example, to backpatch branch addresses for boolean clauses.
  // ---------------------------------------------------------------------
  ex_clause * clause_;

  // ---------------------------------------------------------------------
  // The optype of the item that the user actually specified;
  // it will differ from the item's getOperatorType() only if
  // the Binder rewrites the item (usually a function) in terms of other
  // item primitives; e.g. for ZZZBinderFunction or Aggregate functions.
  // ---------------------------------------------------------------------
  OperatorTypeEnum origOpType_;

  //static THREAD_P OperatorTypeEnum origOpTypeBeingBound_;
  //static THREAD_P Int32 origOpTypeCounter_;

  // ------------------
  // Object Counter
  // ------------------
  static THREAD_P ObjectCounter *counter_;

  // Indicates if this is an expression that originally was a hostvar but
  // got changed to something else after binding (for instance, a column).
  // This can happen in a compound statement when we do an INTO :hostvar and
  // use that hostvar later on in the same compound statement
  NABoolean previousHostVar_;
  NAString  previousName_;

  // an indicator on whether to resolve incomplete types
  // in the method BindNodeRoot().
  NABoolean resolveIncompleteType_ ;

  NABoolean preCodeGenNATypeChange_;

  //++MV
  // If selectivityFactor_ is not negative, then it contains the assumed selectivity
  // for this item expression (known by the optimizer or by inline information).
  // The item expression that has a non negative selectivityFactor, will be categorize
  // as a Default Predicate and the defaultSel method will return this attribute.
  double selectivityFactor_;
  //--MV

  UInt32 flags_;
}; // class ItemExpr

// -----------------------------------------------------------------------
// Inlines that couldn't be defined till now due to dependencies
// -----------------------------------------------------------------------

inline ItemExpr * ExprValueId::operator ->() const	{ return getPtr(); }
inline ExprValueId::operator ItemExpr *() const		{ return getPtr(); }


#endif /* ITEMEXPR_H */
