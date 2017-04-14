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
#ifndef VALUEDESC_H
#define VALUEDESC_H
/* -*-C++-*-
******************************************************************************
*
* File:         ValueDesc.h
* Description:  Value descriptors, value ids, and collections of value ids
* Created:      4/27/94
* Language:     C++
*
*
*
*
******************************************************************************
*/



#include "ObjectNames.h"
#include "Collections.h"
#include "CascadesBasic.h"
#include "DomainDesc.h"
#include "OperTypeEnum.h"
#include "CmpCommon.h"
#include "Int64.h"
#include "exp_expr.h"
#include "ExprNode.h"
#include "CostScalar.h"
#include "ClusteredBitmap.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class ValueId;
class ValueIdSet;
class ValueIdList;
class ValueIdArray;
class ValueIdMap;
class ValueDesc;
class ValueDescArray;
// class ValueIdListList;

// -----------------------------------------------------------------------
// Forward references
// -----------------------------------------------------------------------
class BindWA;
class CacheWA;
class CharType;
class ExprGroupId;
class GroupAttributes;
class ItemExpr;
class NAColumn;
class BaseColumn;
class NormWA;
class RelExpr;
class SchemaDB;
class DisjunctGlobal;
class VEGRewritePairs;
class TableDesc;
class IndexDesc;
class ConstValue;
class NATable;

////////////////////
class QueryAnalysis;
class ColAnalysis;
class PredAnalysis;
////////////////////
// ***********************************************************************
// A static function (outside of a class) to get the current SchemaDB
//
// ***********************************************************************

extern SchemaDB * ActiveSchemaDB();

extern void DisplayVid(CollIndex id);		// for debugging!

// ***********************************************************************
// ValueId : A Value identifier
// A value id is an index into an array of ValueDesc's, which is stored
// in the SchemaDB.
// ***********************************************************************

class ValueId
{
friend class ValueIdSet; // to access id_

public:

  // ---------------------------------------------------------------------
  // Constructor, default constructor, and copy constructor
  // (specify an index into the global array of ValueDescs)
  // ---------------------------------------------------------------------
  ValueId (CollIndex x = 0) : id_(x) {}

  // ---------------------------------------------------------------------
  // This constructor should usually be used: create a value id by
  // inserting an expression into the global array of value ids
  // and initializing the ValueId with the index in the global array.
  // NOTE: this should probably do duplicate detection??
  // ---------------------------------------------------------------------
  ValueId(ValueDesc *vDPtr);

  // ---------------------------------------------------------------------
  // Destructor
  // ---------------------------------------------------------------------
  ~ValueId() {}

  // ---------------------------------------------------------------------
  // Since a ValueId is an index, it has an automatic conversion operator
  // to type CollIndex.
  // ---------------------------------------------------------------------
  operator CollIndex () const { return id_; }

  // ---------------------------------------------------------------------
  // Comparison of two ValueIds
  // ---------------------------------------------------------------------
  NABoolean operator == (const ValueId & other) { return id_ == other.id_; }
  NABoolean operator != (const ValueId & other) { return id_ != other.id_; }

  // ---------------------------------------------------------------------
  // convert to unsigned long
  // ---------------------------------------------------------------------
  inline const UInt32 toUInt32() const { return id_; } ;

  // ---------------------------------------------------------------------
  // get the corresponding ValueDesc
  // ---------------------------------------------------------------------
  ValueDesc *getValueDesc() const;

  // ---------------------------------------------------------------------
  // get the corresponding Item Expression
  // ---------------------------------------------------------------------
  ItemExpr *getItemExpr() const;

  // ---------------------------------------------------------------------
  // get the itemExpr's NAColumn (if it is a column)
  // ---------------------------------------------------------------------
  NAColumn *getNAColumn(NABoolean okIfNotColumn = FALSE) const;

  // ---------------------------------------------------------------------
  // the converse of getNAColumn() above, return the itemExpr's BaseColumn
  // (if it is a column)
  // ---------------------------------------------------------------------
  BaseColumn *castToBaseColumn(NABoolean *isaConstant=NULL) const ;

  // return my base column value id or set isaConstant to true if a constant
  ValueId getBaseColumn(NABoolean *isaConstant) const ;

  // ---------------------------------------------------------------------
  // get the corresponding NAType
  // ---------------------------------------------------------------------
  const NAType &getType() const;

  // Normalize the valueId replacing valueIds with VEGRefs
  NABoolean normalizeNode(NormWA & normWARef);

  // return TRUE iff i'm a string literal with unknown char set
  NABoolean inferableCharType();

  // return TRUE iff i'm a char type with known char set
  NABoolean hasKnownCharSet(const CharType **ctp);

  // return TRUE iff I am a ValueId associated with an Index Column, or
  // a base column that is a divisioning column. If I am an index column,
  // get the associated base column.
  NABoolean isDivisioningColumn() const;

  // return TRUE iff I am a ValueId associated with an Index Column, or
  // a base column that is a salt (computed) column. If I am an index column,
  // get the associated base column.
  NABoolean isSaltColumn() const;

  // return TRUE if I am a ValueId associated with an Index Column, or
  // a base column , and I am a column with a default value that is not
  // null or not Current.
  NABoolean isColumnWithNonNullNonCurrentDefault() const;

  // ---------------------------------------------------------------------
  // change the ValueId's type to the given type
  // ---------------------------------------------------------------------
  void changeType(const NAType *type);

  // ---------------------------------------------------------------------
  // coerce the ValueId's unknown type to the desired type
  // ---------------------------------------------------------------------
  void coerceType(enum NABuiltInTypeEnum desiredQualifier,
                  enum CharInfo::CharSet charSet=CharInfo::DefaultCharSet);
  void coerceType(const NAType& desiredType,
                  enum NABuiltInTypeEnum defaultQualifier = NA_UNKNOWN_TYPE);

  // ---------------------------------------------------------------------
  // create a ValueId of same datatype as 'this', except also nullable
  // (if 'this' is already nullable, returns 'this', unless forceCast is True)
  // ---------------------------------------------------------------------
  ValueId nullInstantiate(BindWA *bindWAPtr, NABoolean forceCast) const;

  // ---------------------------------------------------------------------
  // get the value id's of all ValueIdUnion nodes which exists as sub
  // expressions of the item expr which has this valueid.
  // used in MergeUnion::preCodeGen() to minimize the set of outputs the
  // operator should produce.
  // ---------------------------------------------------------------------
  void getSubExprRootedByVidUnion(ValueIdSet & vs);

  // ---------------------------------------------------------------------
  // replace any BaseColumn (of the given column name) in of this value
  // expression with the given expression.
  // used in Insert::bindNode() to move constraints from the target table
  // to the source table.
  // ---------------------------------------------------------------------
  void replaceBaseColWithExpr(const NAString& colName, const ValueId & vid);

  // -----------------------------------------------------------------------
  // replace any ColReference (of the given column name) in of this value
  // expression with the given expression.
  // used in ValueId::computeEncodedKey() to assign key values into the
  // salt/DivisionByto expression.
  // ----------------------------------------------------------------------
  void replaceColReferenceWithExpr(const NAString& colName, const ValueId & vid);

  // ---------------------------------------------------------------------
  // Replace the definition of this valueId to be a new itemexpr
  // ---------------------------------------------------------------------
  void replaceItemExpr(ItemExpr * iePtr);

  // ---------------------------------------------------------------------
  // Return the ColAnalysis and PredAnalysis for this valueId
  // ---------------------------------------------------------------------
  ColAnalysis* colAnalysis() const;
  PredAnalysis* predAnalysis() const;

  // return column analysis or set isaConstant to true if a constant
  // set vid to base column's value id
  ColAnalysis* baseColAnalysis(NABoolean *isaConstant, ValueId &vid) const;

  // ----------------------------------------------------------------------
  // returns TRUE if the valueId is a veg_ref referring columns from more
  // than one tables
  // ----------------------------------------------------------------------

  NABoolean isInvolvedInJoinAndConst() const;
  NABoolean anExpression();
  NABoolean moreThanOneColFromSameTabOrCharDate(TableDesc * tdesc,
                                                ValueIdSet & colsOfThisTable);

  ULng32 hash() const { return (ULng32) id_;}
private:

  // an index into the ValueDescArray in the Schema DB
  CollIndex id_;

};

// Define a constant for an invalid ValueId
const ValueId NULL_VALUE_ID((CollIndex) 0);

// ***********************************************************************
// ValueIdArray : An ordered collection of Value identifiers
// ***********************************************************************

class ValueIdArray : public ARRAY(ValueId)
{
public:
  ValueIdArray(Lng32 numberOfElements = 0) :
  NAArray<ValueId>(CmpCommon::statementHeap(),numberOfElements)
  {}

  // copy ctor
  ValueIdArray (const ValueIdArray & orig) :
  NAArray<ValueId>(orig, CmpCommon::statementHeap())
  {}
private:
}; // class ValueIdArray

// ***********************************************************************
// ValueIdList : An ordered collection of Value identifiers
// ***********************************************************************

class ValueIdList : public LIST(ValueId)
{
public:
  // --------------------------------------------------------------------
  // Constructor that pre allocates storage for the list.
  // --------------------------------------------------------------------
  ValueIdList(Lng32 numberOfElements = 0, CollHeap *h=CmpCommon::statementHeap()) :
    NAList<ValueId>(h, numberOfElements)
  {(*counter_).incrementCounter();}

  // --------------------------------------------------------------------
  // Constructor that copies from an array (assumes array is densely populated)
  // --------------------------------------------------------------------
  ValueIdList(const ValueIdArray &, CollHeap *h=CmpCommon::statementHeap());

  // --------------------------------------------------------------------
  // copy ctor
  // --------------------------------------------------------------------
  ValueIdList(const ValueIdList &other, CollHeap *h=CmpCommon::statementHeap()) :
    NAList<ValueId>(other, h)
  {(*counter_).incrementCounter();}
  // --------------------------------------------------------------------
  // Constructor that copies from a set
  // --------------------------------------------------------------------
  ValueIdList(const ValueIdSet &, CollHeap *h=CmpCommon::statementHeap());

  // --------------------------------------------------------------------
  // Destructor
  // --------------------------------------------------------------------
  ~ValueIdList() {(*counter_).decrementCounter();};

  // ---------------------------------------------------------------------
  // insert "foreign" data types
  // ---------------------------------------------------------------------
  void insertSet(const ValueIdSet &other);

  void addMember(ItemExpr* x);

  // find the length of the maximal prefix of this such that each element
  // in the prefix is in x.
  //  return 1 when this = [1, 2, 3, 4], and x = {1]
  //  return 2 when this = [1, 2, 3, 4], and x = {1, 2}
  //  return 2 when this = [1, 2, 3, 4], and x = {1, 2, 4}
  //  return 0 when this = [1, 2, 3, 4], and x = {2, 4}
  //  return 1 when this = [1], and x = {1, 2}
  Lng32 findPrefixLength(const ValueIdSet& x) const;

  // --------------------------------------------------------------------
  // transformNode()
  //
  // transformNode() brings predicate trees that are associated with
  // each relational operator into a canonical form. The effect of
  // some predicate transformations is local to the predicate tree.
  // However, the transformation of a subquery predicate requires a
  // semijoin to be performed between the relational operator that
  // contain the predicate tree and the subquery tree. Thus, the effect
  // of such subquery transformations is realized not just in the
  // predicate tree but also in the query tree.
  //
  // This method is invoked on scalar expressions
  //
  // Parameters:
  //
  // NormWA & normWARef
  //    IN : a pointer to the normalizer work area
  //
  // ExprGroupId &introduceSemijoinHere
  //    IN : a pointer to the location in the query tree where a
  //         subquery transformation can introduce a semijoin. It
  //         is usually the location of the pointer to the relational
  //         operator with which the given predicate tree is associated.
  //
  // const ValueIdSet & externalInputs
  //    IN : A set of values that can be supplied as external inputs
  //         to a new SemiJoin or Join node, which is added by a subquery
  //         transformation, if they are required for evaluating the
  //         transformed predicate that is moved to the new node.
  //
  // RETURNS : TRUE  if at least one subquery was transformed
  //           FALSE otherwise
  //
  // --------------------------------------------------------------------
  NABoolean transformNode(NormWA & normWARef,
                          ExprGroupId & introduceSemiJoinHere,
                          const ValueIdSet & externalInputs);

  // --------------------------------------------------------------------
  // ValueIdList::removeCoveredExprs()
  //
  // This method removes from the valueid set that values that
  // are covered by the available inputs.
  // --------------------------------------------------------------------
  void removeCoveredExprs(const ValueIdSet & newExternalInputs);

  // --------------------------------------------------------------------
  // ValueIdList::removeUnCoveredExprs()
  //
  // This method removes from the valueid set that values that
  // are NOT covered by the available inputs.
  // --------------------------------------------------------------------
  void removeUnCoveredExprs(const ValueIdSet & newExternalInputs);

  // --------------------------------------------------------------------
  // normalizeNode()
  //
  // normalizeNode() brings a query tree that consists of relational
  // operators into a canonical form. It also rewrites predicates
  // using the transitive closure of values that is computed by
  // transformNode().
  //
  // Parameters:
  //
  // NormWA & normWA
  //    IN : a reference to the normalizer work area
  //
  // --------------------------------------------------------------------
  NABoolean normalizeNode(NormWA & normWARef);

  // ---------------------------------------------------------------------
  // removeSubqueryOrIsolatedUDFunctionPredicates()
  //
  // This method walks through the ItemExprs that belong to this set,
  // collects the ValueIds of the expressions which contain a subquery
  // or an IsolatedUDFunction and removes them from the orginal set.
  // ---------------------------------------------------------------------
  void removeSubqueryOrIsolatedUDFunctionPredicates(ValueIdSet & subqueryExpr);

  // ---------------------------------------------------------------------
  // Is a prefix of this list covered in the provided set?  If so,
  // return the number of covered elements.  Otherwise, return 0.
  // Warning: before using this function, take a look at the function
  // getLengthOfCoveredPrefix() and determine what functionality you want.
  // ---------------------------------------------------------------------
  Int32 prefixCoveredInSet (const ValueIdSet& vidSet) const;

  // Are all members of this list are covered in the provided set?
  // If exactMatch is set: 
  //    If all the members of this list are covered in the provided set
  //    AND if they are exact match, return TRUE. Otherwise, return FALSE.
  // If exactMatch is not set: 
  //    If all the members of this list are covered in the provided set, 
  //    return TRUE. Otherwise, return FALSE.
  NABoolean allMembersCoveredInSet(const ValueIdSet& vidSet,
                                   NABoolean exactMatch) const;

  // ---------------------------------------------------------------------
  // Check whether a prefix of this list is covered by the provided set.
  // For the remaining suffix, remove those items from this list.
  // ---------------------------------------------------------------------
  void removeUncoveredSuffix (const ValueIdSet& vidSet);

  // ---------------------------------------------------------------------
  // findNJEquiJoinColumns()
  // Determine the maximum prefix of this list that is covered
  // by either a constant, a parameter/host variable, or an equi-join
  // column.  Then return the list that is composed of just the equi-join
  // columns. Also returns in an output parameter the number of columns
  // of this list that were not covered.
  // Input: child0Outputs, child1Inputs
  // Output: numOfUncoveredCols
  // Return value: a list representing the equijoin columns found
  // ---------------------------------------------------------------------
  ValueIdList findNJEquiJoinCols(
                const ValueIdSet & child0Outputs,
                const ValueIdSet & child1Inputs,
                ValueIdList & uncoveredCols) const;

  // ---------------------------------------------------------------------
  // Is a prefix of this list covered in the provided set, or in
  // the simplified version of the provided set? If an element is
  // found that is not in the original set but is in the simplified
  // version of the provided set, then replace the valueid in the
  // list with the one in the original provided set - i.e. "complify"
  // it.
  // ---------------------------------------------------------------------
  Int32 complifyAndCheckPrefixCovered (const ValueIdSet& vidSet,
                                       const GroupAttributes *ga);

  // ---------------------------------------------------------------------
  // Check whether a prefix of this list is covered by the provided set.
  // This might involve replacing some elements of the list with
  // their counterparts in the provided set.
  // For the remaining suffix, remove those items from this list.
  // ---------------------------------------------------------------------
  void complifyAndRemoveUncoveredSuffix (const ValueIdSet& vidSet,
                                         const GroupAttributes *ga);

  // ---------------------------------------------------------------------
  // simplifyOrderExpr()
  //
  //  Returns a version of the list that has all expressions involving
  // columns and constants or columns and inverse nodes simplified to be
  // the column only.
  // ---------------------------------------------------------------------
  ValueIdList simplifyOrderExpr() const;

  // ---------------------------------------------------------------------
  // removeInverseOrder()
  //
  //  Returns a version of the list that has all expressions involving
  // columns and inverse nodes simplified to be the column only.
  // ---------------------------------------------------------------------
  ValueIdList removeInverseOrder() const;

  // ---------------------------------------------------------------------
  // removeDuplicateValueIds()
  // Removes duplicate value ids from the ValueIdList object.
  // ---------------------------------------------------------------------
  void removeDuplicateValueIds();


  // ---------------------------------------------------------------------
  // substituteValueIds()
  // Substitutes a one valueId for another in the list 
  // ---------------------------------------------------------------------
  void substituteValueIds(const ValueId vid, const ValueId replacementVid);

  // ---------------------------------------------------------------------
  // If a table is ordered by the expressions described in this list,
  // does the ordering satisfy a required order or arrangement of columns
  // (GroupAttributes and predicates can be provided to allow more matches
  // by applying some optimizations)
  // ---------------------------------------------------------------------
  OrderComparison satisfiesReqdOrder(const ValueIdList &reqdOrder,
				     GroupAttributes *ga = NULL,
                                     const ValueIdSet *preds = NULL) const;
  NABoolean satisfiesReqdArrangement(const ValueIdSet &reqdArrangement,
				     GroupAttributes *ga = NULL,
                                     const ValueIdSet *preds = NULL) const;

  // ---------------------------------------------------------------------
  // Calculate the length of the row containing all the value ids
  // ---------------------------------------------------------------------
  Lng32 getRowLength() const;

  // ---------------------------------------------------------------------
  // Calculate the length of numeric type vids in the row containing all 
  // the value ids
  // ---------------------------------------------------------------------
  Lng32 getRowLengthOfNumericCols() const;
 
  // ---------------------------------------------------------------------
  // Calculate the space needed in an sql_buffer for Mdam values.
  // These values are: high, low, non-null high, non-null low, and current.
  // There are five per key column.  Data in sql_buffer is eight-byte
  // alligned.
  // ---------------------------------------------------------------------
  Lng32 getMdamSqlBufferSpace() const;

  // ---------------------------------------------------------------------
  // rebuildExprTree()
  //
  // This method is used by the code generator for reconstructing a
  // list of expressions that is represented by a ValueIdList.
  // It creates a BoolResult node on top of the ANDed tree, if
  // createFinalizeResultNode flag is set to TRUE. Used by generator
  // to finalize the boolean result.
  // ---------------------------------------------------------------------
  ItemExpr * rebuildExprTree(OperatorTypeEnum op = ITM_ITEM_LIST,
                             NABoolean redriveTypeSynthesisFlag = FALSE,
			     NABoolean createFinalizeResultNode = FALSE) const;

  // ---------------------------------------------------------------------
  // ValueIdList::replaceVEGExpressions()
  //
  // This method is used by the code generator for rewriting
  // expressions that belong to a ValueIdList. A reference to
  // a VEG, i.e., a ValueId Equality Group is replaced with
  // an expression that belongs the VEG as well as to the
  // set of availableValues that is supplied.
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
  // If replicateExpression is set, a copy of the expression is
  // returned.
  //
  // if joinInputAndPotentialOutput is not null, then use it to steer that 
  // join's VEGPredicate::replaceVEGPredicate()'s choice of invariantExprId
  // toward a member of that join's input and potential output.
  // 
  // When thisIsAKeyPredicate is set to TRUE, you need to also pass in
  // an indexDesc in order to guarantee that we will replace a VEGReference
  // with a key column.
  // ---------------------------------------------------------------------
  void replaceVEGExpressions
          (const ValueIdSet & availableValues,
           const ValueIdSet & inputValues,
           NABoolean thisIsAKeyPredicate = FALSE,
           VEGRewritePairs * lookup = NULL,
           NABoolean replicateExpression = FALSE,
           const ValueIdSet * outputExpr = NULL,
           const ValueIdSet * joinInputAndPotentialOutput = NULL,
           const IndexDesc  * iDesc = NULL,
           const GroupAttributes * left_ga = NULL,
           const GroupAttributes * right_ga = NULL);

  // ---------------------------------------------------------------------
  // replaceOperandsOfInstantiateNull()
  // This method is used by the code generator for replacing the
  // operands of an ITM_INSTANTIATE_NULL with a value that belongs
  // to availableValues.
  // ---------------------------------------------------------------------
  void replaceOperandsOfInstantiateNull(const ValueIdSet &,
                                        const ValueIdSet & inputValues);

  // Simplify subexpressions that contain constants only;
  // for example, if the valueIdSet looks like x < 3 + 4
  // then the list returns as x < 7 and the function result will be FALSE.
  // If we see "1=1 and 2=2",
  // then the list returns as "1." and the function result will be TRUE.
  // If error, the list returned will be empty!
  NABoolean constantFolding();

  // Extract the ValueId of the VEG_REFERENCE in the ValueIdList that
  // refers to the same VEG as refered to by x, which must be a VEG predicate,
  // or to an equal predicate.
  ValueId extractVEGRefForEquiPredicate(ValueId x) const;


  // Encode this list of constants into an encoded key and save the result into
  // encodedKeyBuffer, and the key length into keyBufLen. Allocate an buffer
  // if encodedKeyBuffer points at NULL from STMTHEAP. Also return the buffer pointer
  // if everything is OK.  Return NULL otherwise.
  char* computeEncodedKey(const TableDesc* tDesc, NABoolean isMaxKey, char*& encodedKeyBuffer, Int32& keyBufLen) const;


  // ---------------------------------------------------------------------
  // Print
  // ---------------------------------------------------------------------
  void display() const;

  void print( FILE* ofd = stdout,
	      const char* indent = DEFAULT_INDENT,
              const char* title = "ValueIdList",
              CollHeap *c=NULL, char *buf=NULL) const;

  void unparse(NAString &result,
	       PhaseEnum phase = DEFAULT_PHASE,
	       UnparseFormatEnum form = USER_FORMAT,
	       TableDesc * tabId = NULL) const;

  // evaluate this ValueIdList's expr(s) into resultBuffer

  ex_expr::exp_return_type evalAtCompileTime
    (short addConvNodes, //(IN) : 1 to add conv nodes, 0 otherwise
     ExpTupleDesc::TupleDataFormat tf,//(IN): tuple format of resulting expr(s)
     char* resultBuffer, //(INOUT): tuple buffer of resulting expr(s)
     ULng32 resultBufferLength, //(IN): length of the result buffer
     Lng32 *length=NULL,  //(OUT) : length of 1st result expr
     Lng32 *offset=NULL   //(OUT) : offset of 1st result expr
     , ComDiagsArea *diagsArea = NULL
     ) const;


  // is this list cacheable after this phase?
  NABoolean isCacheableExpr(CacheWA& cwa);

  // change literals of a ValueIdList into ConstantParameters
  void normalizeForCache(CacheWA& cwa, BindWA& bindWA);

  // ---------------------------------------------------------------------
  // Find all common elements between "this" and "other"
  // ---------------------------------------------------------------------
  void findCommonElements(const ValueIdSet &other);
  ValueIdList findCommonElementsFromList(const ValueIdList &other);

  // The following methods are used by constant folding
  // Used by constant folding. Calls the executor evaluator. The parameters are:
  // 1.- The root of the expresion tree to evaluate. It is assumed that it
  // contains constants only.
  // 2.- encodedKeyBuffer, which is a pointer to the result.
  // 3.- The length of the result
  //
  static short evaluateTree( const ItemExpr * root,
			     char * encodedKeyBuffer,
			     ULng32 encodedKeyLength,
			     Lng32 *length,
			     Lng32 *offset,
			     ComDiagsArea *diagsArea = NULL
			     );

  static Lng32 evaluateConstantTree( const ValueId &parent,
				    const ValueId & ch,
				    Int32 childNumber,
				    ItemExpr ** outItemExpr,
				    ComDiagsArea *diagsArea = NULL
				    );

  static Int32 evaluateExpr( const ValueId & parent,
			   const ValueId & ch,
			   Int32 childNumber,
			   NABoolean simplifyExpr = TRUE,
			   NABoolean evalAllConsts = FALSE,
			   ItemExpr ** outAllConstsItemExpr = NULL,
			   ComDiagsArea *diagsArea = NULL
			   );

  NABoolean hasVarChars() const;

  // For each vid in the list represnting a constant,
  //   the literal value of the constant is appended to 'result'
  void convertToTextKey(const ValueIdList& keyList, NAString& result);
  static ConstValue* getConstant(ItemExpr* ie);

  // count the number of prefix constants 
  Int32 countConstantsAsPrefixes();


private:

  static NABoolean canSimplify( ItemExpr *itemExpr,
				const ValueId &parent,
				Int32 i,
				Int32 childNumber,
				Int32 &moved
				);

  static THREAD_P ObjectCounter *counter_;

}; // class ValueIdList

//class ValueIdListList : public LIST(ValueIdList)
//{
//public:
  // --------------------------------------------------------------------
  // Constructor that pre allocates storage for the list.
  // --------------------------------------------------------------------
//  ValueIdListList(long numberOfElements = 0) :
//  NAList<ValueIdList>(CmpCommon::statementHeap(), numberOfElements)
//  {}
//};

// ***********************************************************************
// ValueIdSet : A collection of Value identifiers
// ***********************************************************************

class ValueIdSet : public ClusteredBitmap
{
public:

  // ---------------------------------------------------------------------
  // constructor (the default constructor uses a method to determine
  // the active SchemaDB automatically)
  // ---------------------------------------------------------------------
  ValueIdSet();
  ValueIdSet(const ValueIdSet &other);
  // copy ctor
  ValueIdSet(const ValueIdList &other);
  ValueIdSet(const ValueId& vid);

  ~ValueIdSet()				{ (*counter_).decrementCounter();};

  ValueIdSet(ValueDescArray *arr);

  // operators
  NABoolean operator == (const ValueIdSet &other) const;

  NABoolean operator != (const ValueIdSet &other) const
                                         { return NOT operator==(other); }

  //----------------------------------------------------------------------
  // Method that synthesizes a valueIdset that is comprised of the basecolumn
  // valueids of this valueidset. So the ids in this set are expected to be
  // for Columns.
  //----------------------------------------------------------------------
  ValueIdSet convertToBaseIds() const;
  // ---------------------------------------------------------------------
  // Is this set a (dense) prefix of the ValueIdList "other"?
  // ---------------------------------------------------------------------
  NABoolean isDensePrefix(const ValueIdList &other) const;

  // how many prefix columns of other are found in this predicate?
  Int32 prefixMatchesOf(const ValueIdList &other) const;

  // Does this contain the first N elements of the "other" ValueIdList?
  NABoolean coversFirstN(const ValueIdList &other, Int32 N=INT_MAX) const;

  // ---------------------------------------------------------------------
  // Calculate the estimated length of the row containing all the value ids
  // ---------------------------------------------------------------------
  Lng32 getRowLength() const;

  // ---------------------------------------------------------------------
  // Calculate the length of numeric type vids in the row containing all
  // the value ids
  // ---------------------------------------------------------------------
  Lng32 getRowLengthOfNumericCols() const;
  
  // ---------------------------------------------------------------------
  // Iterator methods for a ValueIdSet
  // use the iterators in a for loop like this (assuming you have a
  // ValueIdSet S over which you want to iterate)
  // for (ValueId x= S.init();  S.next(x); S.advance(x) )
  //    { /* x is the current element */ }
  // ---------------------------------------------------------------------
  ValueId init() const			{ return ValueId((CollIndex) 0); }
  NABoolean next(ValueId &v) const	{ return nextUsed(v.id_); }
  void advance(ValueId & v) const	{ v.id_++; }

  // -----------------------------------------------------------------------
  // This method is a slight modification of intersectSet. Here instead of
  // modifying this operand it returns the modified set
  // -----------------------------------------------------------------------
  ValueIdSet intersect(const ValueIdSet & v) const;

  // like intersectSet but dig into wrappers
  ValueIdSet& intersectSetDeep(const ValueIdSet & v);

  void getFirst(ValueId & v) const; // returns NULL_VALUE_ID if isEmpty()

  // ---------------------------------------------------------------------
  // insert "foreign" data types
  // ---------------------------------------------------------------------
  void insertList(const ValueIdList &other);

  // ---------------------------------------------------------------------
  // insert the valueId of x 
  // ---------------------------------------------------------------------
  void addMember(ItemExpr* x);

  // --------------------------------------------------------------------
  // transformNode()
  //
  // transformNode() brings predicate trees that are associated with
  // each relational operator into a canonical form. The effect of
  // some predicate transformations is local to the predicate tree.
  // However, the transformation of a subquery predicate requires a
  // semijoin to be performed between the relational operator that
  // contain the predicate tree and the subquery tree. Thus, the effect
  // of such subquery transformations is realized not just in the
  // predicate tree but also in the query tree.
  //
  // Parameters:
  //
  // NormWA & normWARef
  //    IN : a pointer to the normalizer work area
  //
  // ExprGroupId &introduceSemijoinHere
  //    IN : a pointer to the location in the query tree where a
  //         subquery transformation can introduce a semijoin. It
  //         is usually the location of the pointer to the relational
  //         operator with which the given predicate tree is associated.
  //
  // const ValueIdSet & externalInputs
  //    IN : A set of values that can be supplied as external inputs
  //         to a new SemiJoin or Join node, which is added by a subquery
  //         transformation, if they are required for evaluating the
  //         transformed predicate that is moved to the new node.
  //
  // const NABoolean movePredicates
  //    IN : If this ValueIdSet represents a predicate tree, then the
  //         caller is expected to set movePredicate to TRUE.
  //         Whenever movePredicate is set and a subquery is transformed,
  //         the transformed predicate is moved to the SemiJoin or Join
  //         node that is introduced by the subquery transformation.
  //
  // const NABoolean postJoinPredicates
  //    IN : If this flag is set, then each predicate is examined to
  //         determine whether it can cause the transformation of a
  //         left join to an inner join.
  //
  // RETURNS : TRUE  if at least one subquery was transformed
  //                             ****************************
  //           FALSE otherwise
  //
  // --------------------------------------------------------------------
  NABoolean transformNode(NormWA & normWARef,
                          ExprGroupId & introduceSemiJoinHere,
                          const ValueIdSet & externalInputs,
                          const NABoolean movePredicates = FALSE,
			  const NABoolean postJoinPredicates = FALSE);

  // Returns TRUE if EACH member of this ValueIdSet isCovered()
  NABoolean isCovered(const ValueIdSet & newExternalInputs,
		      const GroupAttributes & newRelExprAnchorGA,
	   	      ValueIdSet & referencedInputs,
		      ValueIdSet & coveredSubExpr,
		      ValueIdSet & unCoveredExpr) const;

  // --------------------------------------------------------------------
  // ValueIdSet::removeCoveredExprs()
  //
  // This method removes from the this valueid set those values that
  // are covered by the available inputs.
  // It returns the number of elements removed.
  // --------------------------------------------------------------------
  Int32 removeCoveredExprs(const ValueIdSet & newExternalInputs, 
                           ValueIdSet* usedInputs = NULL);

  //-------------------------------------------------------
  //removeCoveredVidSet()
  //Input: other set
  //output: reduced *this set by other set
  //Constraints: will remove the ids as long as
  //other set contains a reference or the id itself.
  //---------------------------------------------------------
  void removeCoveredVIdSet(const ValueIdSet & otherSet);
  // --------------------------------------------------------------------
  // ValueIdSet::removeUnCoveredExprs()
  //
  // This method removes from the valueid set that values that
  // are NOT covered by the available inputs.
  // It returns the number of elements removed.
  // --------------------------------------------------------------------
  Int32 removeUnCoveredExprs(const ValueIdSet & newExternalInputs);

  // ---------------------------------------------------------------------
  // simplifyOrderExpr()
  //
  //  Returns a version of the set that has all expressions involving
  // columns and constants or columns and inverse nodes simplified to be
  // the column only.
  // ---------------------------------------------------------------------

  ValueIdSet simplifyOrderExpr() const;

  // ---------------------------------------------------------------------
  // removeInverseOrder()
  //
  //  Returns a version of the set that has all expressions involving
  // columns and inverse nodes simplified to be the column only.
  // ---------------------------------------------------------------------

  ValueIdSet removeInverseOrder() const;


  // ---------------------------------------------------------------------
  // Remove all expressions that are not common subexpressions with the
  // other set from this set.
  // ---------------------------------------------------------------------
  void findCommonSubexpressions(ValueIdSet &other,
				NABoolean removeCommonExprFromOther = FALSE);

  // ---------------------------------------------------------------------
  // Build predicate like the originals in the set, that substitues the
  // given column reference with a computed column and the corresponding 
  // computed column expression.
  //
  // For example: 
  // predicate is : A = 2
  // computed Column is: SQR
  // computed Column expr is: POW(A, 2)
  // Then this method will return a ValueIdSet that contains
  // a predicate that looks like this:
  //    SQR = POW(VEG(A,2),2)
  // ---------------------------------------------------------------------
  ValueIdSet createMirrorPreds(ValueId &computedCol,
                               ValueIdSet underlyingCols);

  // --------------------------------------------------------------------
  // lookForVEGReferences()
  // Accumulate ValueIds of VEGPredicates, if any, in vs.
  // --------------------------------------------------------------------
  void lookForVEGReferences(ValueIdSet & VEGRefSet) const;

  // *this is a set of (join) equality predicates.
  // grcols is a set of group by columns.
  // if c is in grcols and c is an opd of an equality predicate then
  // add the equality's other opd to grcols.
  void introduceMissingVEGRefs(ValueIdSet & grcols) const;

  // --------------------------------------------------------------------
  // lookForVEGPredicates()
  // Accumulate ValueIds of VEGPredicates, if any, in vs.
  // --------------------------------------------------------------------
  void lookForVEGPredicates(ValueIdSet & VEGPredSet) const;

  // return TRUE iff this has no equality predicates at all
  // put in joinCols the simple base columns being equijoined
  // given "a-1=b and c=d+1", joinCols should get "b,c"
  NABoolean hasNoEquiPredicates(ValueIdSet& joinCols) const;


  // return TRUE iff x appears in this in a vegRef, a vegPredicate or
  // an equal predicate. A constant must also appear in the vegRef,
  // vegPredicate or the equal predicate.
  NABoolean containsAsEquiLocalPred(ValueId x) const;

  // return TRUE iff x appears in this in a vegRef and is part of a 
  // range predicate (<.<=,>,>=) whee the other side is a constant
  NABoolean containsAsRangeLocalPred(ValueId x) const;

  // --------------------------------------------------------------------
  // ValueIdSet::getVEGesWithMultipleConsts()
  // This method accumulates those VEGes that have more than one constant
  // references in them
  // --------------------------------------------------------------------

  void getVEGesWithMultipleConsts(ValueIdSet & keyPredsToBeEvaluated);
  // --------------------------------------------------------------------
  // ValueIdSet::accumulateReferencedValues()
  //
  // This method accumulates those values that are members of the
  // referencedSet and are referenced either individually or in a
  // VEGReference in the referencingSet (if the optional parameters
  // allow it). The original contents of this ValueIdSet are augmented.
  // --------------------------------------------------------------------
  void accumulateReferencedValues(const ValueIdSet & referencedSet,
				  const ValueIdSet & referencingSet,
                                  NABoolean doNotDigInsideVegRefs = FALSE,
                                  NABoolean doNotDigInsideInstNulls = FALSE);

  // --------------------------------------------------------------------
  // ValueIdSet::getLeafValuesForCoverTest()
  //
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
  // ValueIdSet::referencesOneValueFromTheSet()
  //
  // Check whether an expression contained in this ValueIdSet
  // references an expression that is a member of the otherSet.
  // --------------------------------------------------------------------
  NABoolean referencesOneValueFromTheSet(const ValueIdSet & otherSet) const;

  // --------------------------------------------------------------------
  // ValueIdSet::referencesAllValuesFromMe()
  //
  // Check whether every expression contained in this ValueIdSet
  // references an expression that is a member of the otherSet.
  // --------------------------------------------------------------------
  NABoolean referencesAllValuesFromMe(const ValueIdSet & otherSet) const;

  // --------------------------------------------------------------------
  // referencesTheGivenValue()
  //
  // Check whether any expression contained in this ValueIdSet
  // references the given value.
  // --------------------------------------------------------------------
  NABoolean referencesTheGivenValue(
              const ValueId & theValue,
              ValueId & exprId,
              NABoolean doNotDigInsideVegRefs = FALSE,
              NABoolean doNotDigInsideInstNulls = FALSE) const;

  // --------------------------------------------------------------------
  // containsTheGivenValue()
  //
  // Check whether any expression contained in this ValueIdSet
  // is the given valueId or is a VEGref that contains the given
  // valueId.
  // --------------------------------------------------------------------
  NABoolean containsTheGivenValue( const ValueId & theValue ) const;

  // return true iff set has RandomNum expr
  NABoolean hasRandom() const;

  // --------------------------------------------------------------------
  // membersCoveredInSet()
  //
  // Check whether any of the members of this ValueIdSet are contained
  // in the provided ValueIdSet.  Return the number of members found.
  // --------------------------------------------------------------------
  Int32 membersCoveredInSet (const ValueIdSet & vidSet, NABoolean lookBelowInstantiateNull) const;

  // --------------------------------------------------------------------
  // return true iff ValueIdSet has predicates that guarantee
  // that opd is not nullable
  // --------------------------------------------------------------------
  NABoolean isNotNullable(const ValueId& opd);

  // --------------------------------------------------------------------
  // weedOutUnreferenced()
  //
  // Check which values belonging to other are referenced in this set.
  // Delete all values in other that are not referenced by the value
  // expressions that belong to this set.
  // --------------------------------------------------------------------
  void weedOutUnreferenced(ValueIdSet & other) const;

  // --------------------------------------------------------------------
  // weedOutNonEquiPreds()
  //
  // Remove everything except equi-predicates (VEGPredicate and ITM_EQUAL)
  // --------------------------------------------------------------------
  void weedOutNonEquiPreds();

  // --------------------------------------------------------------------
  // normalizeNode()
  //
  // normalizeNode() brings a query tree that consists of relational
  // operators into a canonical form. It also rewrites predicates
  // using the transitive closure of values that is computed by
  // transformNode().
  //
  // Parameters:
  //
  // NormWA & normWA
  //    IN : a reference to the normalizer work area
  //
  // --------------------------------------------------------------------
  NABoolean normalizeNode(NormWA & normWARef);

  // ---------------------------------------------------------------------
  // rebuildExprTree()
  //
  // This method is used by the code generator for reconstructing a
  // tree of predicate factors that is represented by a ValueIdSet
  // ---------------------------------------------------------------------
  ItemExpr * rebuildExprTree(OperatorTypeEnum op = ITM_AND,
                             NABoolean redriveTypeSynthesisFlag = FALSE,
			     NABoolean createFinalizeResultNode = FALSE) const;

  // -----------------------------------------------------------------------
  // ValueIdSet::referencesOneValueFromTheSet()
  //
  // Check whether a VEGPred contained in this ValueIdSet references
  // references a value that is a member of the otherSet and remove it
  // othewise.
  // -----------------------------------------------------------------------

  void removeUnReferencedVEGPreds
                           (const ValueIdSet & otherSet);

  //--------------------------------------------------------------------------
  // Check whether the Valudidset contains any CAST expressions && and replace
  // it with the original expression
  //--------------------------------------------------------------------------
  void replaceCastExprWithOriginal(const ValueIdSet &, const RelExpr *);

  //--------------------------------------------------------------------------
  // Check whether the ValueIdSet contains expressions of the form
  // InstNull(Cast(aggregate))) and modify them to aggregate.
  //--------------------------------------------------------------------------
  void replaceInstnullCastAggregateWithAggregateInLeftJoins(RelExpr *);

  // ---------------------------------------------------------------------
  // removeSubqueryOrIsolatedUDFunctionPredicates()
  //
  // This method walks through the ItemExprs that belong to this set,
  // collects the ValueIds of the expressions which contain a subquery
  // or an Isolated UDFunction and removes them from the orginal set.
  // ---------------------------------------------------------------------
  void removeSubqueryOrIsolatedUDFunctionPredicates(ValueIdSet & subqueryExpr);

  // -----------------------------------------------------------------------
  // ValueIdSet::replaceVEGExpressionsAndCopy()
  //
  // Copy the Values contained in setContainingWildCards into this set
  // after expanding all wild card expressions.
  // -----------------------------------------------------------------------
  void replaceVEGExpressionsAndCopy(const ValueIdSet & setContainingWildCards);

  // -----------------------------------------------------------------------
  // ValueIdSet::getUnReferencedVEGmembers()
  //
  //   Called only from preCodeGen. Gets all the VEG members that
  //   have not been bound yet.
  // -----------------------------------------------------------------------
  void getUnReferencedVEGmembers(const ValueIdSet & setContainingWildCards);

  // ---------------------------------------------------------------------
  // ValueIdSet::replaceVEGExpressions()
  //
  // This method is used by the code generator for rewriting
  // expressions that belong to a ValueIdSet. A reference to
  // a VEG, i.e., a ValueId Equality Group is replaced with
  // an expression that belongs the VEG as well as to the
  // set of availableValues that is supplied.
  //
  // if joinInputAndPotentialOutput is not null, then use it to steer that 
  // join's VEGPredicate::replaceVEGPredicate()'s choice of invariantExprId
  // toward a member of that join's input and potential output.
  // 
  // When thisIsAKeyPredicate is set to TRUE, you need to also pass in
  // an indexDesc in order to guarantee that we will replace a VEGReference
  // with a key column.
  //
  // The last two optional parameters, avaliableValues_left and 
  // availableValues_right are used by the hash join to make sure we 
  // keep the resulution of VEGReferences to values apropriate for each 
  // child of the join.
  // ---------------------------------------------------------------------
  void replaceVEGExpressions(const ValueIdSet & availableValues,
                             const ValueIdSet & inputValues,
                             NABoolean thisIsAKeyPredicate = FALSE,
                             VEGRewritePairs * lookup = NULL,
                             NABoolean replicateExpression = FALSE,
                             const ValueIdSet * outputExpr = NULL,
                             const ValueIdSet * joinInputAndPotentialOutput = NULL,
                             const IndexDesc * iDesc = NULL,
                             const GroupAttributes * left_ga = NULL,
                             const GroupAttributes * right_ga = NULL);

  // ---------------------------------------------------------------------
  // replaceOperandsOfInstantiateNull()
  // This method is used by the code generator for replacing the
  // operands of an ITM_INSTANTIATE_NULL with a value that belongs
  // to availableValues.
  // ---------------------------------------------------------------------
  void replaceOperandsOfInstantiateNull(const ValueIdSet & availableValues,
                                        const ValueIdSet & inputValues);

  // ---------------------------------------------------------------------
  // References a  Constant Value
  // Does this valueidset reference a constant?  If so, return it.
  // ---------------------------------------------------------------------
  NABoolean referencesAConstValue (ItemExpr ** constant) const;

  // ---------------------------------------------------------------------
  // References a Constant Expression
  // Does this valueidset reference a constant, hostvariable, or param?
  // Optionally return the constant expression if constExprptr argument is
  // not NULL.
  // ---------------------------------------------------------------------
  NABoolean referencesAConstExpr(ItemExpr** constExprPtr = NULL) const;

  // ---------------------------------------------------------------------
  // Removes constant expression references if any in this valueidset.
  // ---------------------------------------------------------------------
  void removeConstExprReferences(NABoolean considerExpressions=FALSE);

  // ---------------------------------------------------------------------
  // Checks if the valueidset has a veg predicate and contains a
  // hostvariable or param
  // ---------------------------------------------------------------------
  NABoolean referencesAHostvariableorParam() const;

  // ---------------------------------------------------------------------
  // Replace any predicate consisting of a RangeSpecRef with the predicates
  // that it was derived from.
  // ---------------------------------------------------------------------
  ValueIdSet replaceRangeSpecRefs() const;

  // ---------------------------------------------------------------------
  // Remove any predicate with host variable or dunamic parameter from the
  // set of predicates
  // ---------------------------------------------------------------------
  ValueIdSet removePredsWithHostVars() const;

  // ---------------------------------------------------------------------
  // Remove any predicate with rolling columns from the
  // set of predicates
  // ---------------------------------------------------------------------
  ValueIdSet removePredsWithRollingCols(TableDesc *tdesc) const;

  // -----------------------------------------------------------------------
  //  referenceConstExprCount
  //
  //  This method is invoked to determine how many elements of the
  //  valueidset contains a "constant expression", i.e. a constant,
  //  host variable or a parameter.
  // -----------------------------------------------------------------------
  Int32 referencesConstExprCount() const;

  // -----------------------------------------------------------------------
  //  referencesBignumNumericKeyColumns
  //
  //  This method is invoked to determine if at least one of ItemExprs
  //  contained in the set is of big number numeric type.
  // -----------------------------------------------------------------------
  NABoolean referencesBignumNumericDataType() const;

  // -----------------------------------------------------------------------
  // getColumnsForHistogram
  //
  // returns columns which are referred in the expressions containing constants,
  // constant expressions or host variables
  // --------------------------------------------------------------------------

  ValueIdSet getColumnsForHistogram() const;

  // Out of all columns in the set, return the one with minimum UEC
  CostScalar getMinOrigUecOfJoiningCols();

  // ---------------------------------------------------------------------
  // Get the constant values that are in the set
  // ---------------------------------------------------------------------
  void getConstants(ValueIdSet & addConstantsToThisSet, NABoolean includeCacheParams = FALSE) const;

  // ---------------------------------------------------------------------
  // If isStrict is false, get the constant expressions 
  // (constants, hostvars and dynamic para) that are in the set. 
  // If isStrict is true look only for constants
  // in both cases look inside a veg.
  // ---------------------------------------------------------------------
  void getConstantExprs(ValueIdSet & addConstantExprsToThisSet, 
			NABoolean isStrict = FALSE) const;

  // ---------------------------------------------------------------------
  // Loops through the vidset and returns true iff all vids evaluate to a constant
  // Essentially calld doesExprEvaluateToAConstant() in a loop.
  // ---------------------------------------------------------------------
  NABoolean doAllExprsEvaluateToConstant(NABoolean isStrict, 
                                         NABoolean considerVEG) const ;

  // ---------------------------------------------------------------------
  // Get Outer References
  // Given a set of input values, return all outer column references.
  // ---------------------------------------------------------------------
  void getOuterReferences (ValueIdSet & OuterRefs) const;

  //----------------------------------------------------------------------
  // a utility routine for analyzing predicates, and categorizing them as
  // (1) Local Predicates - those that do not contain outer references
  //     a) equality predicates
  //     b) other non-equality predicates supported by synthesis (for now,
  //        these include inequality predicates only)
  // (2) Non-local predicates - those that do contain outer references
  //     a) equality predicates
  //     b) other non-equality predicates
  // (3) BiLogic: AND, or OR
  //----------------------------------------------------------------------
  void categorizePredicates (const ValueIdSet & outerReferences,
			     ValueIdSet & EqLocalPreds,
			     ValueIdSet & OtherLocalPreds,
			     ValueIdSet & EqNonLocalPreds,
			     ValueIdSet & OtherNonLocalPreds,
			     ValueIdSet & BiLogicPreds,
			     ValueIdSet & DefaultPreds ) const;


  // This method returns TRUE if there are any predicates for which the 
  // cardinality cannot be accurately estimated by the optimizer
  NABoolean predsReqFetchCnt(TableDesc * tdes);

  // -----------------------------------------------------------------------
  // getReferencedPredicates:
  // a utility routine that is a simple version of categorizePredicates. It
  // simply picks out all predicates that are referenced by any valueid in
  // first argument and puts them in the second set.
  // This method goes through the ValueIdSet pointed to by the this pointer.
  // All those valueids that are referenced by any member
  // of the valueIdSet that is the first argument to this method are inserted
  // into the ValueIdSet that is the second argument. This method can be used
  // get all the predicates in the selection predicate of a node that contain
  // outer references. Method returns TRUE if any predicate is found that 
  // references a valueid from the first set.
  //
  // The default behavior of the routine is to skip anything that evaluates
  // to a constant. If you set the searchConstExpr boolean parameter to
  // TRUE, the routine (see ItemExpr::referencesOneValueFrom()) will search 
  // those expressions as well.
  // -----------------------------------------------------------------------
  NABoolean getReferencedPredicates (const ValueIdSet & outerReferences,
				      ValueIdSet & nonLocalPreds ) const;

  // ---------------------------------------------------------------------
  // Print
  // ---------------------------------------------------------------------
  void display() const;

  void print( FILE* ofd = stdout,
	      const char* indent = DEFAULT_INDENT,
              const char* title = "ValueIdSet",
              CollHeap *c=NULL, char *buf=NULL) const;

  void unparse(NAString &result,
	       PhaseEnum phase = DEFAULT_PHASE,
	       UnparseFormatEnum form = USER_FORMAT,
	       TableDesc * tabId = NULL) const;

  short showQueryStats(CollHeap *c, char *buf);

  void replaceOneRowbyList( NormWA&);

  // change literals of a ValueIdSet into ConstantParameters
  void normalizeForCache(CacheWA& cwa, BindWA& bindWA);

/////////////////////////////////////////////////////////////
  // -----------------------------------------------------------------------
  // ValueIdSet::accumulateReferencingValues()
  //
  // This method accumulates those expressions that are members of the
  // referencingSet and are referencing any value in ReferencedSet.
  // -----------------------------------------------------------------------
  void accumulateReferencingExpressions(const ValueIdSet & referencingSet,
                                  const ValueIdSet & referencedSet,
                                  NABoolean doNotDigInsideVegRefs = FALSE,
                                  NABoolean doNotDigInsideInstNulls = FALSE);

  // -----------------------------------------------------------------------
  // ValueIdSet::removeNonReferencingExpressions()
  //
  // This method remove those expressions that are members of the
  // referencingSet and not referencing any value in ReferencedSet.
  // -----------------------------------------------------------------------
  void removeNonReferencingExpressions(const ValueIdSet & otherSet,
                                  NABoolean doNotDigInsideVegRefs = FALSE,
                                  NABoolean doNotDigInsideInstNulls = FALSE);

  // -----------------------------------------------------------------------
  // ValueIdSet::findAllReferencedBaseCols()
  //
  // This method find all base columns referenced directly or indirectly
  // via this ValueIdSet. This includes degging into VEGs and recursively
  // walking ItemExpr trees until the leaf nodes.
  // -----------------------------------------------------------------------
  void findAllReferencedBaseCols(ValueIdSet & result) const;

  void findAllReferencedIndexCols(ValueIdSet & result) const;

  // -----------------------------------------------------------------------
  // ValueIdSet::findAllEqualityCols()
  //
  // This method finds all eqaulity columns referenced directly or indirectly 
  // via this ValueIdSet.
  // -----------------------------------------------------------------------
  void findAllEqualityCols(ValueIdSet & result) const;

  // -----------------------------------------------------------------------
  // This method finds all itemExpr with the type referenced directly 
  // -----------------------------------------------------------------------
  void findAllOpType(OperatorTypeEnum type, ValueIdSet & result) const;

  // -----------------------------------------------------------------------
  // This method collects all child expressions, for each member of the 
  // set. This method does not recursively go below the current set.
  // -----------------------------------------------------------------------
  void findAllChildren(ValueIdSet & result) const;

  // ---------------------------------------------------------------------
  // ValueIdSet::getAllTables()
  // This method will get all tables whose columns are included in the set
  // ----------------------------------------------------------------------

  SET(TableDesc *) * getAllTables();

  // ---------------------------------------------------------------------
  // ValueIdSet::doColumnsConstituteUniqueIndex(const NATable * table)
  // Returns a boolean to indicate if these columns constitute a primary
  // key or unique index.
  // ---------------------------------------------------------------------
  NABoolean doColumnsConstituteUniqueIndex(TableDesc * tableDesc, NABoolean considerStats = TRUE);

  // -------------------------------------
  // xxx
  // ------------------
  void columnAnalysis(QueryAnalysis* qa, NABoolean predOnSemiOrLeftJoin = FALSE) const;

  // ---------------------------------------------------------------------
  // Returns a boolean to indicate if this vid set has a vid that
  // corresponds to a subquery
  // ---------------------------------------------------------------------
  NABoolean containsSubquery() const;

  // ---------------------------------------------------------------------
  // Returns a pointer to the ItemExpr node of UDFunction if this vid set 
  // has a vid that corresponds to a UDF.  0 otherwise.
  // ---------------------------------------------------------------------
  ItemExpr *containsUDF() const;

  // ---------------------------------------------------------------------
  // Returns a boolean to indicate if this vid set has a vid that
  // corresponds to an Isolated UDFunction 
  // ---------------------------------------------------------------------
  NABoolean containsIsolatedUDFunction() const;

  // ---------------------------------------------------------------------
  // ---------------------------------------------------------------------
  // Returns a boolean to indicate if this vid set has a vid that 
  // corresponds to a count function 
  // ---------------------------------------------------------------------
  NABoolean containsCount() const;

  // ---------------------------------------------------------------------
  // Returns a boolean to indicate if this vid set has a vid that 
  // corresponds to a oneTrue function. If a reference to a ValueId
  // is given, we initialize it to the ValueID of the oneTrue expression.
  // ---------------------------------------------------------------------
  NABoolean containsOneTrue(ValueId &refOneTrue ) const;

  // ---------------------------------------------------------------------
  // Returns a boolean to indicate if this vid set has a vid that 
  // corresponds to a anyTrue function. If a reference to a ValueId
  // is given, we initialize it to the ValueID of the oneTrue expression.
  // ---------------------------------------------------------------------
  NABoolean containsAnyTrue(ValueId &refAnyTrue ) const;

  // ---------------------------------------------------------------------
  // Returns a boolean to indicate if this vid set has a vid that 
  // corresponds to a FalseConstant. Such a constant is generated 
  // during constant folding. For example a user given predicate such as 
  // 1 = 0 will cause constant folding to generate such a Constant. 
  // The reference to a ValueId given is initialized to the ValueID 
  // of the FalseConstant.
  // ---------------------------------------------------------------------
  NABoolean containsFalseConstant(ValueId &falseConstant ) const;

  // for each OLAP LEAD function cotnained in this, add the equivalent
  // OLAP LEAD function for each element (as the child of LEAD) in input, 
  // and save the new function in result
  void addOlapLeadFuncs(const ValueIdSet& input, ValueIdSet& result);

/////////////////////////////////////////////////////////////

private:
  static THREAD_P ObjectCounter *counter_;
}; // class ValueIdSet

// ***********************************************************************
// ValueIdMap : A structure that bidirectionally maps value ids to new
//              value ids (arbitrarily called top and lower value ids)
// ***********************************************************************

class ValueIdMap : public NABasicObject
{
public:

  // constructors
  ValueIdMap() {}
  ValueIdMap(const ValueIdMap &other)
    : topValues_(other.topValues_), bottomValues_(other.bottomValues_) {}
  // copy ctor
  ValueIdMap(const ValueIdSet &identity);
  ValueIdMap(const ValueIdList &topList,const ValueIdList &bottomList)
    : topValues_(topList), bottomValues_(bottomList) {}

  // operators
  ValueIdMap & operator = (const ValueIdMap &other)
                      { topValues_ = other.topValues_;
			bottomValues_ = other.bottomValues_; return *this; }

  NABoolean operator == (const ValueIdMap &other) const;

  NABoolean operator != (const ValueIdMap &other) const
                                         { return NOT operator==(other); }

  // accessor functions
  const ValueIdList & getTopValues() const    { return topValues_; }
  const ValueIdList & getBottomValues() const { return bottomValues_; }

  void remapTopValue(const ValueId & newTopValue, const ValueId &bottomValue);

  void remapBottomValue(const ValueId & topValue, const ValueId &newBottomValue);

  void addMapEntry(const ValueId & newTopValue, const ValueId &newBottomValue);

  void clear()      { topValues_.clear(); bottomValues_.clear(); }

  // map value ids from the topValue side to the bottomValue side and vice versa
  // assume that expressions are already contained in the map, otherwise
  // use the "rewrite" methods below
  void mapValueIdUp(ValueId &newTopValue, const ValueId &bottomValue) const;
  void mapValueIdDown(const ValueId &topValues, ValueId &newBottomValues) const;
  void mapValueIdUpWithIndex(ValueId &topValue, const ValueId &bottomValue, CollIndex i) const;
  void mapValueIdListUp(ValueIdList &newTopValues, const ValueIdList &bottomValues) const;
  void mapValueIdListDown(const ValueIdList &topValues, ValueIdList &newBottomValues) const;
  void mapValueIdSetUp(ValueIdSet &newTopValues, const ValueIdSet &bottomValues) const;
  void mapValueIdSetDown(const ValueIdSet &topValues, ValueIdSet &newBottomValues) const;

  // rewrite expressions from the topValue side to the bottomValue side and vice versa
  void rewriteValueIdUp(ValueId &newTopValue, const ValueId &bottomValue);
  void rewriteValueIdUpWithIndex(ValueId &topValue, const ValueId &bottomValue, CollIndex i);
  void rewriteValueIdDown(const ValueId &topValues, ValueId &newBottomValues);
  void rewriteValueIdListUp(ValueIdList &newTopValues, const ValueIdList &bottomValues);
  void rewriteValueIdListUpWithIndex(ValueIdList &topValues, const ValueIdList &bottomValues);
  void rewriteValueIdListDown(const ValueIdList &topValues, ValueIdList &newBottomValues);
  void rewriteValueIdSetUp(ValueIdSet &newTopValues, const ValueIdSet &bottomValues);
  void rewriteValueIdSetDown(const ValueIdSet &topValues, ValueIdSet &newBottomValues);

  // flip the top and bottom maps
  void flipSides();

  // add VEGPreds for VEGRefs contained in the map
  void augmentForVEG(NABoolean addVEGPreds,
                     NABoolean addVEGRefs,
                     NABoolean compareConstants,
                     const ValueIdSet *topInputsToCheck,
                     const ValueIdSet *bottomInputsToCheck,
                     ValueIdSet *vegRefsWithDifferentConstants = NULL,
                     ValueIdSet *vegRefsWithDifferentInputs = NULL);

  // Normalize the map replacing valueIds with VEGRefs
  NABoolean normalizeNode(NormWA & normWARef);

  // Removed items from the map that are no longer required.
  void removeUnusedEntries(const ValueIdSet &requiredValues, NABoolean matchWithTopValues);

private:

  // value ids from corresponding list positions are mapped
  // (topValues_.entries() == bottomValues_.entries())
  ValueIdList topValues_;
  ValueIdList bottomValues_;
};

// ***********************************************************************
//
// ValueDesc : A Value descriptor
//
// ***********************************************************************

class ValueDesc  : public NABasicObject
{
friend class ValueDescArray; // to set valId_

public:

  // ---------------------------------------------------------------------
  // Constructor functions
  // ---------------------------------------------------------------------
  ValueDesc(ItemExpr *expr);

  static ValueId create(ItemExpr *expr,
  			const NAType *type,
  			CollHeap *h = CmpCommon::statementHeap());

  // ---------------------------------------------------------------------
  // Accessor functions
  // ---------------------------------------------------------------------
  ValueId  getValueId() const { return valId_; }

  DomainDesc * getDomainDesc() const { return domId_; }

  ItemExpr *getItemExpr() const { return exprPtr_; }

  // ---------------------------------------------------------------------
  // Mutator functions
  // ---------------------------------------------------------------------
  void setDomainDesc(DomainDesc * domId) { domId_ = domId; }
  void replaceItemExpr(ItemExpr * iePtr) { exprPtr_ = iePtr; }

private:

  // ---------------------------------------------------------------------
  // The value identifier
  // ---------------------------------------------------------------------
  ValueId  valId_;

  // ---------------------------------------------------------------------
  // The value-producing expression (an item expression)
  // ---------------------------------------------------------------------
  ItemExpr *exprPtr_;

  // ---------------------------------------------------------------------
  // The domain (type) information for the above expression.
  // ---------------------------------------------------------------------
  DomainDesc *  domId_;

}; // class ValueDesc

// ***********************************************************************
// ValueDescArray : An ordered collection of Value descriptors
// ***********************************************************************

class ValueDescArray : public ARRAY(ValueDesc *)
{
public:

  // ---------------------------------------------------------------------
  // Constructor
  // ---------------------------------------------------------------------
  ValueDescArray() :
    ARRAY(ValueDesc *) (CmpCommon::statementHeap())
    { insertAt(0,NULL); /* create a NULL entry (ValueId = 0) */ }

  // ---------------------------------------------------------------------
  // copy ctor
  // ---------------------------------------------------------------------
  ValueDescArray (const ValueDescArray & orig) :
    ARRAY(ValueDesc *) (orig, CmpCommon::statementHeap())
    {}

  // ---------------------------------------------------------------------
  // Insert a new ValueDesc and assign a ValueId to it
  // ---------------------------------------------------------------------
  void insert(ValueDesc * newElem)
    { newElem->valId_ = entries(); insertAt(entries(),newElem); }

  // ---------------------------------------------------------------------
  // Print
  // ---------------------------------------------------------------------
  static void Display(NABoolean dontDisplayErrors = FALSE);

  void display(NABoolean dontDisplayErrors = FALSE) const;

  virtual void print( FILE* ofd = stdout,
		      const char* indent = DEFAULT_INDENT,
                      const char* title = "ValueDescArray",
		      NABoolean dontDisplayErrors = FALSE) const;

}; // class ValueDescArray

#endif /* VALUEDESC_H */

