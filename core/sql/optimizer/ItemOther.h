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
#ifndef ITEMOTHER_H
#define ITEMOTHER_H
/* -*-C++-*-
******************************************************************************
*
* File:         ItemOther.h
* Description:  Miscellaneous item expressions
*
* Created:      11/04/94
* Language:     C++
*
*
*
*
******************************************************************************
*/


#include "RelSet.h"		// for Union (needed by ValueIdUnion flags)
#include "NAColumn.h"
#include "QRExprElement.h"
#include "ItemLog.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class Assign;
class Convert;
class ItemList;
class RenameCol;
class ValueIdRef;
class ValueIdProxy;
class ValueIdUnion;
class VEG;
class VEGReference;
class VEGPredicate;

class RangeSpecRef;
class OptRangeSpec;
class OptNormRangeSpec;

// forward
class Generator;
class VEGRewritePairs;
enum  ColumnClass;

// -----------------------------------------------------------------------
// An assignment operator assigns the source value to the target. Used
// for insert and update operations.
// -----------------------------------------------------------------------
class Assign : public ItemExpr
{

public:

  Assign(ItemExpr *target,
	 ItemExpr *source,
	 NABoolean userSpecified = TRUE) :
	 ItemExpr(ITM_ASSIGN, target, source),
	 userSpecified_(userSpecified),
         canBeSkipped_(FALSE),
	 onRollback_(FALSE)
	 {}

  // virtual destructor
  virtual ~Assign() {}

  // get the degree of this node
  virtual Int32 getArity() const;

  NABoolean isUserSpecified() const	{ return userSpecified_; }
  void setUserSpecified(NABoolean userSpecified) { userSpecified_ = userSpecified; }

  NABoolean canBeSkipped() const	{ return canBeSkipped_; }
  void setToBeSkipped(NABoolean canBeSkipped) { canBeSkipped_ = canBeSkipped; }

  ValueId getTarget() const { return child(0)->getValueId(); }
  ValueId getSource() const { return child(1)->getValueId(); }

  // QSTUFF
  NABoolean onRollback() const			{ return onRollback_; }
  void      setOnRollback(NABoolean onRollback)	{ onRollback_ = onRollback; }
  // QSTUFF

  // a virtual function for performing name binding within the query tree
  virtual ItemExpr * bindNode(BindWA *bindWA);

  // these two functions enable relaxation of char type matching rules
  // for assignments
  NABoolean isRelaxCharTypeMatchRulesPossible();
  ItemExpr* tryToRelaxCharTypeMatchRules(BindWA *bindWA);

  // a virtual function for type propagating the node
  virtual const NAType *synthesizeType();

  // another virtual function for type propagating the node. This one
  // is for the benefit of internal stored procedures, which want
  // to report alternative error messages.
  virtual const NAType *synthesizeType(const char * str1, const Lng32 int1);

  virtual NABoolean isCovered(const ValueIdSet& newExternalInputs,
			      const GroupAttributes& newRelExprAnchorGA,
	   	              ValueIdSet& referencedInputs,
			      ValueIdSet& coveredSubExpr,
			      ValueIdSet& unCoveredExpr) const;

  // is any literal in this expr safely coercible to its target type?
  virtual NABoolean isSafelyCoercible(CacheWA& cwa) const;

  // change literals of a cacheable query into constant parameters
  virtual ItemExpr* normalizeForCache(CacheWA& cwa, BindWA& bindWA);

  // Each operator supports a (virtual) method for transforming its
  // query tree to a canonical form.
  virtual ItemExpr * normalizeNode(NormWA & normWARef);

  virtual HashValue topHash();
  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);

  // virtual method to fixup tree for code generation.
  virtual ItemExpr * preCodeGen(Generator*);

  // method to do code generation
  short codeGen(Generator*);

  // get a printable string that identifies the operator
  const NAString getText() const { return "assign"; }

  // Reads column heading and tablename and set these members
  // in Dynamic Parameter (for scalar param) and Hostvar (for array param)
  // ItemExprs. Used only for JDBC/WLI BLOB/CLOB implementation.
  ItemExpr * SetParamHeadingAndTablename(BindWA *bindWA);

  NABoolean CanChild0BeImplicitlyCast() { return FALSE; };

private:

  // This refers to whether the assignment was specified by the user in the
  // DML stmt (INSERT or UPDATE), *not* whether the source or target is a
  // user column!  Binder-generated Assigns (keytags, index maintenance)
  // must set this field to FALSE.
  NABoolean userSpecified_;

  NABoolean onRollback_;	// QSTUFF

  // Do the actual type propagation, to be called by the public synthesizeType methods.
  const NAType *doSynthesizeType(ValueId & targetId, ValueId & sourceId);

  // Assign with omitted default value columns other than COM_CURRENT_DEFAULT class types
  NABoolean canBeSkipped_;

}; // class Assign

// -----------------------------------------------------------------------
// A convert operator converts one data type to another
// -----------------------------------------------------------------------
class Convert : public ItemExpr
{

public:

  Convert(ItemExpr *input,
		  UInt32 lastVOAOffset = 0,
		  Int16 vcIndicatorLength = 0,
		  Int16 nullIndicatorLength = 0,
		  Int16 alignment = 0) :
	  ItemExpr(ITM_CONVERT, input)
  {
	  lastVOAOffset_ = lastVOAOffset;
	  lastVcIndicatorLength_ = vcIndicatorLength;
	  lastNullIndicatorLength_ = nullIndicatorLength;
	  alignment_ = alignment;
  }

  // virtual destructor
  virtual ~Convert() {}

  // get the degree of this node (it is a unary op).
  virtual Int32 getArity() const;

  // get a printable string that identifies the operator
  const NAString getText() const { return "convert"; }

  virtual HashValue topHash();
  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);

  // virtual method to fixup tree for code generation.
  virtual ItemExpr * preCodeGen(Generator*);

  // method to do code generation
  short codeGen(Generator*);
private:
  UInt32 lastVOAOffset_;
  Int16  lastVcIndicatorLength_;
  Int16  lastNullIndicatorLength_;
  Int16  alignment_;
}; // class Convert

typedef LIST(ExprValueId*) ExprValueIdList;
// -----------------------------------------------------------------------
// An element of a list.
// -----------------------------------------------------------------------
class ItemList : public ItemExpr
{

public:

  ItemList(ItemExpr *commaExpr, ItemExpr *otherExpr)
       : ItemExpr(ITM_ITEM_LIST, commaExpr, otherExpr),
	 numOfItems_(0), constChild_(FALSE)
  {}

  // virtual destructor
  virtual ~ItemList() {}

  // does this query's selection predicate list qualify query
  // to be cacheable after this phase?
  NABoolean isListOfCacheableSelPred(CacheWA& cwa, ItemList *other) const;

  // is any literal in this expr safely coercible to its target type?
  virtual NABoolean isSafelyCoercible(CacheWA& cwa) const;

  // change literals of a cacheable query into input parameters
  ItemExpr* normalizeListForCache(CacheWA& cwa, BindWA& bindWA,
                                  ItemList *other);

  virtual NABoolean isCovered(const ValueIdSet& newExternalInputs,
			      const GroupAttributes& newRelExprAnchorGA,
	   	              ValueIdSet& referencedInputs,
			      ValueIdSet& coveredSubExpr,
			      ValueIdSet& unCoveredExpr) const;

  virtual Int32 getArity() const;

  // get a printable string that identifies the operator
  const NAString getText() const { return ","; }

  virtual NABoolean duplicateMatch(const ItemExpr &other) const;
  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);

  // a virtual function for performing name binding within the query tree
  virtual ItemExpr * bindNode(BindWA *bindWA);

  // a virtual function for type propagating the node
  virtual const NAType *synthesizeType();


  // method to do code generation
  short codeGen(Generator*);

  // a helper function collecting leaves into a list
  ExprValueIdList* collectLeaves(CollHeap* heap = 0, ExprValueIdList* = 0);

  // a virtual function to set resolve incomplete type status to each
  // element in the list (for charset inference)
  void setResolveIncompleteTypeStatus(NABoolean x);

  Int64 &numOfItems() {return numOfItems_;}
  NABoolean &constChild() {return constChild_;}

  virtual NABoolean hasEquivalentProperties(ItemExpr * other); 
protected:

private:
  // helper to change literals of a cacheable query into input parameters
  void parameterizeMe(CacheWA& cachewa, BindWA& bindWA, ExprValueId& child,
                      BaseColumn *base, ConstValue *val);

  // total number of items below me.
  // Set when IN list is created during parsing for
  // a <value> IN (list-of-items) pred.
  // Used to do an in-list to (values(..)) transformation.
  // Done if all items are constants and > 100. This could change in future.
  // This value is set to -1 when the first non-const item is seen.
  // All parent ItemLists will have this set to -1.
  Int64 numOfItems_;
  NABoolean constChild_;
}; // class ItemList

// -----------------------------------------------------------------------
// A rename operator gives an expression a new ANSI name
// -----------------------------------------------------------------------
class RenameCol : public ItemExpr
{

public:

  RenameCol(ItemExpr *input,
	    ColRefName *newColRefName)
  : ItemExpr(ITM_RENAME_COL, input),
    targetColumnClass_(USER_COLUMN)
  { newColRefName_ = newColRefName; }

  // virtual destructor
  virtual ~RenameCol() {}

  const ColRefName * getNewColRefName() const { return newColRefName_; }

  // MV --
  inline void setTargetColumnClass(ColumnClass colClass) { targetColumnClass_ = colClass; }
  inline ColumnClass getTargetColumnClass() const { return targetColumnClass_; }

  // get the degree of this node (it is a binary op).
  virtual Int32 getArity() const;

  // get a printable string that identifies the operator
  const NAString getText() const;

  virtual HashValue topHash();
  virtual NABoolean duplicateMatch(const ItemExpr &other) const;
  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);

  // a virtual function for performing name binding within the query tree
  virtual ItemExpr * bindNode(BindWA *bindWA);

private:

  // the parser and binder work with textual column references
  ColRefName*  newColRefName_;

  // MV --
  // Force this column to be added to the RETDesc as a system column.
  ColumnClass targetColumnClass_;

}; // class RenameCol

// -----------------------------------------------------------------------
// A ValueIdRef is a node generated by the SQL binder to denote
// the derivation hierarchy for ValueDescs.
// -----------------------------------------------------------------------

class ValueIdRef : public ItemExpr
{

public:

  ValueIdRef(ValueId valId) : ItemExpr(ITM_VALUEIDREF),
                              derivedFrom_(valId) {}

  virtual ~ValueIdRef() {}

  // get the degree of this node (it is a leaf node)
  virtual Int32 getArity() const;

  ValueId isDerivedFrom() const { return derivedFrom_; }

  virtual NABoolean isCovered(const ValueIdSet& newExternalInputs,
			      const GroupAttributes& newRelExprAnchorGA,
	   	              ValueIdSet& referencedInputs,
			      ValueIdSet& coveredSubExpr,
			      ValueIdSet& unCoveredExpr) const;

  virtual HashValue topHash();
  virtual NABoolean duplicateMatch(const ItemExpr &other) const;
  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);

  virtual OrderComparison sameOrder(ItemExpr *other,
				    NABoolean askOther = TRUE);

  // method to do code generation
  short codeGen(Generator*);

  // get a printable string that identifies the operator
  const NAString getText() const;

private:

  // the ValueId from which this reference is derived
  ValueId  derivedFrom_;

}; // class ValueIdRef

//! ValueIdProxy  class
// -----------------------------------------------------------------------
// A ValueIdProxy is a node generated by the SQL binder to represent
// an additional outputs of the node (typically a subquery or a Multi Valued 
// function) is derived from.  So if you have a query like this:
// select * from t where (a,b) = (select (select mvf(a) from t3) from t2)
// and the mvf outputs 2 numbers, then the binder will assign one valueId
// to the mvf() function itself, which will be carried forward until transfrom
// time until it gets transformed into a relexpr, and it will create another
// valueId for the other output of the Mvf(). This other valueId is of the
// proxy kind.
// This type of node goes away during the transform phase, thus no need
// for methods used by generator, optimizer etc..
//
// -----------------------------------------------------------------------

class ValueIdProxy : public ItemExpr
{

public:

  //! ValueIdProxy::ValueIdProxy() constructor 
  ValueIdProxy(ValueId valId, ValueId outputId, Int32 outputNum,
               NABoolean transformChild = FALSE) : 
                 ItemExpr(ITM_VALUEID_PROXY),
                    derivedFrom_(valId),
                    outputValueId_(outputId), 
                    transformDerivedFromValueId_(transformChild), 
                    outputOrdinalNumber_(outputNum) {}


  //! ValueIdProxy::~ValueIdProxy() destructor 
  virtual ~ValueIdProxy() {}

  //! getArity() method 
  // get the degree of this node (it is a leaf node)
  virtual Int32 getArity() const  { return 0 ; }

  //! needToTransformChild() method 
  // This method is unique to this class. It is used to mark the "parent"
  // or "source" of other valueIdProxies. For example, say we have a subquery
  // that contains "a" and "b" in its select list. If this subquery gets 
  // expanded and replaces by the values in its select list, then the first
  // valueIdProxy points to the subquery it self, it will have the 
  // transformChild flag set, and while representing the subquery from a 
  // transformation point of view, it only represents the output "a" from 
  // a results point of view. The subsequent ValueIdProxy produced for this
  // subquery will contain the valueId for "b" as its output valueId, the
  // subquery's valueId for the derivedFrom valueId, and will not have the
  // transformChild flag set. 

  // By doing this we guarantee that the subquery will only be transformed
  // once regardless of the order the valueIdProxies created from the subquery
  // are transformed.
  
  inline NABoolean needToTransformChild() const 
     { return transformDerivedFromValueId_; }

  //! isDerivedFrom() method 
  // This method is unique to this class. It returns the valueId of the "source"
  // of the the ValueIdProxy. For now only a subquery or MVF.
  inline ValueId isDerivedFrom() const { return derivedFrom_; }

  //! getOutputId() method 
  // This method is unique to this class. It returns the valueId the 
  // ValueIdProxy actually will represent.
  inline ValueId getOutputId() const   { return outputValueId_; }

  //! getOutputNum() method 
  // This method is unique to this class. It returns the ordinal number 
  // of the ValueIdProxy. This number corresponds to the position in the
  // select list of a subquery or the output parameter number of a MVF. 
  inline Int32 getOutputNum() const      { return outputOrdinalNumber_; }

  //! generateCacheKey() method
  // append an ascii-version of the derivedFrom_ node into cachewa.qryText_
  virtual void generateCacheKey(CacheWA& cwa) const;

  //! hasNoLiterals() method
  // return true if ItemExpr & its descendants have no constants 
  // and no noncacheable nodes
  virtual NABoolean hasNoLiterals(CacheWA& cwa);

  //! isSafelyCoercible() method
  // is any literal in this expr safely coercible to its target type?
  virtual NABoolean isSafelyCoercible(CacheWA& cwa) const;

  //! normalizeForCache() method
  // change literals of a cacheable query into ConstantParameters 
  virtual ItemExpr* normalizeForCache(CacheWA& cwa, 
                                                    BindWA& bindWA);

  //! isCovered() method 
  // See ItemExpr.h
  virtual NABoolean isCovered(const ValueIdSet& newExternalInputs,
			      const GroupAttributes& newRelExprAnchorGA,
	   	              ValueIdSet& referencedInputs,
			      ValueIdSet& coveredSubExpr,
			      ValueIdSet& unCoveredExpr) const;

  //! synthesizeType() method 
  // returns the type of the outputValueId.
  virtual const NAType *synthesizeType();

  //! pushDownType()  method
  // Propogate type information down the node we are Proxy for.  This method
  // is called by coerceType(). It will attempt to coerce (a recursive call)
  // the type of the node we are Proxy for to the desired type.
  //
  const NAType *pushDownType(NAType &desiredType,
                       enum NABuiltInTypeEnum defaultQualifier);

  //! getType() method 
  // returns the type of the outputValueId.
  virtual const NAType &getType() const   { return outputValueId_.getType(); }

  //! topHash() method 
  // See ItemExpr.h
  virtual HashValue topHash();

  //! transformNode method
  // See ItemExpr.h
  virtual void transformNode (NormWA & normWARef,
                              ExprValueId & locationOfPointerToMe,
                              ExprGroupId & introduceSemiJoinHere,
                              const ValueIdSet & externalInputs);

  //! duplicateMatch() method 
  // See ItemExpr.h
  virtual NABoolean duplicateMatch(const ItemExpr &other) const;

  //! copyTopNode() method 
  // See ItemExpr.h
  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);

  //! getText() method 
  // get a printable string that identifies the operator
  virtual const NAString getText() const;


  //! containsUDF() method 
  // return an ItemExpr pointer to the derived node if it contains a UDF
  virtual ItemExpr *containsUDF();

  //! containsIsolatedUDFunction() method 
  // return TRUE if the derived node contains an Isolated UDF
  virtual NABoolean containsIsolatedUDFunction();

  //! containsSubquery() method 
  // return TRUE if the derived node contains an Subquery
  virtual NABoolean containsSubquery();

  //! containsValueIdProxySibling() method 
  // This method is unique to this class.
  // return TRUE if the derivedFrom_ is the same in any of the 
  // ValueIdProxies in the siblings set.
  virtual NABoolean containsValueIdProxySibling( const ValueIdSet &siblings);

  //! setTransformChild() method 
  // This method is unique to this class.
  // sets the transformChild_ class member to value given. Used to indicate
  // that this particular ValueIdProxy should transform the derivedFrom
  // valueId, when set to TRUE.
  void setTransformChild(NABoolean transform) 
    { transformDerivedFromValueId_ = transform; }

private:
  // Not used, not implemented:
  ValueIdProxy(ValueIdProxy &other);
  ValueIdProxy &operator=(ValueIdProxy &other);

  //! a flag to indicate if we need to transform the derivedFrom valueId.
  NABoolean  transformDerivedFromValueId_;

  //! the ValueId from which this reference is derived
  ValueId  derivedFrom_;

  //! the ValueId of the output if we know it...
  ValueId  outputValueId_;

  //! the ordinal number of the output, ie. first, second ..
  Int32  outputOrdinalNumber_;

}; // class ValueIdProxy

// -----------------------------------------------------------------------
// A ValueIdUnion is used whenever two or more data streams are
// combined to form a single one. It is used for an SQL union and
// (maybe) when data streams from partitioned sources are combined.
// For the SQL union, the input from its two sources are described
// by a list of ValueIdUnion item expressions. One ValueIdUnion
// is allocated per output expression/field.
// -----------------------------------------------------------------------
class ValueIdUnion : public ItemExpr
{

public:

  ValueIdUnion(ValueId lvid,
	       ValueId rvid,
	       ValueId resid,
	       Int32 flags = Union::UNION_NONE)
    : ItemExpr(ITM_VALUEIDUNION),
      result_(resid),
      flags_(flags),
      otherFlags_(0)
  {
    sources_.insert(lvid);
    sources_.insert(rvid);
  }

  ValueIdUnion(ValueIdList vids, ValueId resid, Int32 flags = Union::UNION_NONE)
    : ItemExpr(ITM_VALUEIDUNION),
      result_(resid),
      flags_(flags),
      otherFlags_(0),
      sources_(vids)
  {}

  virtual ~ValueIdUnion() {}

  // get the degree of this node (it is a leaf node)
  virtual Int32 getArity() const;

  // get and set the ValueIds of the underlying sources that are
  // in corresponding positions

  ValueId getLeftSource()
  {
    CMPASSERT(entries() == 2);
    return sources_[0];
  };

  ValueId getRightSource() {
    CMPASSERT(entries() == 2);
    return sources_[1];
  };

  ValueId getSource(Lng32 index) const { return sources_[index]; }

  void changeSource(Lng32 index, ValueId newvid) { sources_[index]= newvid; }

  const ValueIdList getSources() const { return sources_; }

  CollIndex entries() const { return sources_.entries();}

  // The result contains the ValueId for the value that the union
  // produces as its result.
  ValueId getResult()
  {
    if (result_ == NULL_VALUE_ID)
      return getValueId();
    else
      return result_;
  }

  //  void setLeftSource(ValueId v) { leftSource_ = v; }
  //  void setRightSource(ValueId v) { rightSource_ = v; }
  void setSource(Lng32 index, ValueId v);
  void setResult(ValueId v) { result_ = v; }

  // a virtual function for performing name binding within the query tree
  virtual ItemExpr * bindNode(BindWA *bindWA);

  // a virtual function for type propagating the node
  virtual const NAType * synthesizeType();


  // ValueIdUnion::pushDownType() -----------------------------------
  // Propogate type information down the ItemExpr tree.  This method
  // is called by coerceType(). It will attempt to coerce (a recursive call)
  // the type of each member of the ValueIdUnion to the desired type.
  // This only has an affect when none of the members of the ValueIdUnion
  // could be typed bottom up.
  //
  const NAType *pushDownType(NAType &desiredType,
                       enum NABuiltInTypeEnum defaultQualifier);

  virtual ItemExpr * tryToDoImplicitCasting(BindWA *bindWA);

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

  // Vanilla implementation for normalizing a ValueIdUnion
  virtual ItemExpr * normalizeNode(NormWA & normWARef);

  // A special variant for normalizing each child of this node
  // individually.
  ItemExpr * normalizeSpecificChild(NormWA & normWARef, Lng32 childIndex);

  virtual HashValue topHash();
  virtual NABoolean duplicateMatch(const ItemExpr &other) const;
  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);

  // A method for replacing VEGReference and VEGPredicate objects
  // with another expression that belongs to the VEG as well as to the
  // set of availableValues.
  virtual ItemExpr * replaceVEGExpressions
                        (const ValueIdSet& availableValues,
                         const ValueIdSet& inputValues,
                         NABoolean useBridgeValues = TRUE,
                         VEGRewritePairs * lookup = NULL,
                         NABoolean replicateExpression = FALSE,
                         const ValueIdSet * joinInputAndPotentialOutput = NULL,
                         const IndexDesc * iDesc = NULL,
                         const GroupAttributes * left_ga = NULL,
                         const GroupAttributes * right_ga = NULL);

  // method to do code generation
  virtual short codeGen(Generator*);

  // get a printable string that identifies the operator
  virtual const NAString getText() const;
          const NAString getText(UnparseFormatEnum form) const;

  // Only the parent Union expr (if the parent IS a Union, that is)
  // should call setUnionFlags!
  void setUnionFlags(Int32 f)	{ flags_ = f; }
  Int32 getUnionFlags()		{ return flags_; }

  NABoolean isTrueUnion()
  { return (otherFlags_ & IS_TRUE_UNION) != 0; }
  void setIsTrueUnion(NABoolean v)
  { (v ? otherFlags_ |= IS_TRUE_UNION : otherFlags_ &= ~IS_TRUE_UNION); };

  NABoolean isCastTo()
  { return (otherFlags_ & IS_CAST_TO) != 0; }
  void setIsCastTo(NABoolean v)
  { (v ? otherFlags_ |= IS_CAST_TO : otherFlags_ &= ~IS_CAST_TO); };

private:
  enum
  {
    IS_TRUE_UNION = 0x0001,
    IS_CAST_TO    = 0x0002
  };

  // ValueIds of sources that are in corresponding positions
  ValueIdList sources_;

  ValueId  result_;

  Int32 flags_;		// enum Union::UnionFlags value copied from parent Union

  Int32 otherFlags_;

}; // class ValueIdUnion

// -----------------------------------------------------------------------
// VEG : A ValueId Equality Group
//
// An equality group is a set of ValueIds that have an equality
// relationship imposed on them. The expressions that are members
// of this set therefore produce identical values.
// The latter property permits a given ValueId belonging
// to an equality group to be replaced by any other ValueId from
// the same equality group.
// -----------------------------------------------------------------------
class VEG : public ItemExpr
{
public:
  VEG();
  VEG(const ValueIdSet & vegSet);

  virtual ~VEG();

  // get the degree of this node (it is a leaf node)
  virtual Int32 getArity() const;

  void insert(const ValueId & newValue);
  void insert(const ValueIdSet & newValues);
  void merge(const VEG& other);

  // A record of the normalized state. Set by the normalizer.
  void  setNormalized()                                 { done_ = TRUE; }
  NABoolean  isNormalized()                             { return done_; }

  // Store and retrieve the ValueId of the common VEGReference
  // that should be used for referencing any member of this
  // VEG.
  void setVEGReference(VEGReference * vegrefPtr) { vegRef_ = vegrefPtr; }
  VEGReference * getVEGReference() const { return vegRef_; }

  // Store and retrieve the ValueId of the comman VEGPredicate
  // that replaces each "=" predicate subsumed by this VEG.
  void setVEGPredicate(VEGPredicate * vegpredPtr) { vegPred_ = vegpredPtr; }
  VEGPredicate * getVEGPredicate() const                { return vegPred_; }

  Lng32 getCountOfUserSuppliedInputs() const          { return userInputs_; }

  // return a constant, hostvar or parameter that is part of this VEG
  ValueId getAConstantHostVarOrParameter() const;

  // return a constant that is part of this VEG
  ValueId getAConstant(NABoolean includeCacheParam = FALSE) const;

  virtual HashValue topHash();
  virtual NABoolean duplicateMatch(const ItemExpr &other) const;
  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);

  const ValueIdSet & getAllValues() const { return eqGroup_; }
  ValueIdSet & getAllValuesToUpdate()     { return eqGroup_; }

  // getAllValues may not expand VegRefs which are inside
  // a VEG, this does:
  void getAndExpandAllValues(ValueIdSet& expandedValues) const;


  // Method used for keeping track of referenced values.
  const ValueIdSet & getReferencedValues() const
                                               { return referencedValues_; }
  void markAsReferenced(const ValueId & vid);

  // Method for saving and accessing the last value for which a predicate
  // was generated.
  void setBridgeValue(const ValueId & bridgeValueId);
  const ValueIdSet & getBridgeValues() const { return bridgeValues_; }
  void removeBridgeValues(const ValueIdSet & vidSet) { bridgeValues_ -= vidSet; }

  // get a printable string that identifies the operator
  const NAString getText() const;

  // Used to store whether the VEG has already been seen before while walking
  // an expression tree. Set when VEG is first seen on the way down the tree.
  // Reset on the way up. This is used to prevent walking into an infinite
  // recursive with nested VEG containing self-references. These functions
  // need to be declare "const", since they are called from other functions
  // which are declare "const". However, we are actually cheating here since
  // we change a member in the class, although the member being changed is not
  // a very important element for the class.
  //
  NABoolean seenBefore() const                       { return seenBefore_; }
  void markAsSeenBefore() const       { ((VEG *)this)->seenBefore_ = TRUE; }
  void markAsNotSeenBefore() const   { ((VEG *)this)->seenBefore_ = FALSE; }

  void setSpecialNulls(NABoolean flag) {specialNulls_ = flag; }
  NABoolean getSpecialNulls()	    const	{ return specialNulls_; }

private:
  // Set to TRUE after the transitive closure is computed.
  NABoolean  done_;

  // The set of all transitively related values.
  ValueIdSet eqGroup_;

  // The VEGReference that is allocated for this VEG.
  VEGReference*  vegRef_;

  // The VEGPredicate that is allocated for this VEG.
  VEGPredicate* vegPred_;

  // -- The following data is used by the predicate pushdown logic

  // A count of user supplied inputs that are members of this VEG.
  NABoolean userInputs_;

  // Used to store whether the VEG has already been seen before while walking
  // an expression tree. Set when VEG is first seen on the way down the tree.
  // Reset on the way up. This is used to prevent walking into an infinite
  // recursive with nested VEG containing self-references.
  //
  NABoolean seenBefore_;

  // -- The following data is used by the generator.

  // The set of all values that have been referenced in some
  // "=" predicate that was built from this VEG at least once.
  ValueIdSet referencedValues_;

  // The set of values that must be "bridged" because they link two
  // islands of "=" predicates. When a VEGPredicate is replaced by
  // rewriting it in terms of the values that are available at a given
  // relational operator, the code generator tries to use at least one
  // value that belongs to the following set.
  ValueIdSet bridgeValues_;

  // The flag if set, indicates that 'nulls are values'.
  // That means that nulls are equal to other nulls
  NABoolean specialNulls_;

}; // class VEG

// -----------------------------------------------------------------------
// VEGPredicate : An equality predicate
//
// A VEG predicate is an expression that is of the boolean
// datatype. It is used for replacing a predicate factor that is
// rooted in a binary relational operator, whose children belong
// to the same ValueId Equality Group (VEG).
// -----------------------------------------------------------------------
class VEGPredicate : public ItemExpr
{
public:
  VEGPredicate(const ValueId & ofVEG);
  VEGPredicate(const VEGPredicate&);

  virtual ~VEGPredicate();

  // get the degree of this node (it is a leaf node)
  virtual Int32 getArity() const;

  // An indicator whether this item expression is a predicate.
  virtual NABoolean isAPredicate() const;

  // a virtual function for type propagating the node
  virtual const NAType * synthesizeType();

  void replaceVEG(const ValueId & ofVEG); // used when VEGs are merged
  VEG * getVEG() const { return ((VEG *)(veg_.getItemExpr())); }
  VEG * getVEGToUpdate() { return ((VEG *)(veg_.getItemExpr())); }

  // ++MV - Irena
  NABoolean getSpecialNulls()	    const	{ return specialNulls_; }
  void	    setSpecialNulls(NABoolean flag)	{ specialNulls_ = flag; }
  // --MV - Irena

  ValueIdSet & getPredsWithSelectivities() { return predsWithSelectivities_; };

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

  // Each operator supports a (virtual) method for transforming its
  // query tree to a canonical form. The parameter setOfPredExpr is
  // supplied only when predicates are normalized.
  virtual ItemExpr * normalizeNode(NormWA & normWARef);

  // default selectivity for VEGPredicates
  virtual double defaultSel() ;

  virtual NABoolean applyDefaultPred(ColStatDescList & histograms,
                                     OperatorTypeEnum exprOpCode,
                                     ValueId predValueId,
                                     NABoolean & globalPredicate,
                                     CostScalar *maxSelectivity=NULL);

  // the VEG predicates are supported by the statistics synthesis functions
  virtual NABoolean synthSupportedOp() const;

  virtual HashValue topHash();
  virtual NABoolean duplicateMatch(const ItemExpr &other) const;
  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);

  // A method for replacing VEGReference and VEGPredicate objects
  // with another expression that belongs to the VEG as well as to the
  // set of availableValues.
  virtual ItemExpr * replaceVEGExpressions
                        (const ValueIdSet& availableValues,
                         const ValueIdSet& inputValues,
                         NABoolean useBridgeValues = TRUE,
                         VEGRewritePairs * lookup = NULL,
                         NABoolean replicateExpression = FALSE,
                         const ValueIdSet * joinInputAndPotentialOutput = NULL,
                         const IndexDesc *iDesc = NULL,
                         const GroupAttributes * left_ga = NULL,
                         const GroupAttributes * right_ga = NULL);

  // Methods used by the code generator for replacing a reference
  // to a VEGPredicate with an equality predicate that relates
  // any two members of the VEG, which also belong to availableValues.
  ItemExpr *replaceVEGPredicate(const ValueIdSet& availableValues,
                                const ValueIdSet& inputValues,
				VEGRewritePairs* lookup,
                                const ValueIdSet * joinInputAndPotentialOutput = NULL);

  // get a printable string that identifies the operator
  const NAString getText() const;
  virtual void unparse(NAString &result,
		       PhaseEnum phase = OPTIMIZER_PHASE,
		       UnparseFormatEnum form = USER_FORMAT,
		       TableDesc * tabId = NULL) const;

  // MDAM related methods
  // Performs the MDAM tree walk.  See ItemExpr.h for a detailed description.
  DisjunctArray * mdamTreeWalk();

private:

  // Method used by the code generator for replacing a reference
  // to a VEGPredicate with an equality predicate that relates
  // any two members of the VEG, which also belong to availableValues.
  ItemExpr* replaceVEGPredicate(const ValueIdSet& availableValues,
                                const ValueIdSet& inputValues);

  // Method used by the code generator for replacing a reference
  // to a VEGPredicate with a tree of equality predicate that
  // span all the members of the VEG.
  ItemExpr* replaceVEGPredicateInAnOrSubtree
               (const ValueIdSet& availableValues,
                const ValueIdSet& inputValues);

  // A ValueId Equality Group (VEG) that is represented by this predicate
  ValueId  veg_;

  // ++MV - Irena
  // The flag is set according to the same flag of BiRelat, which
  // the VEGPredicate is generated from.
  //
  // The flag if set, indicates that 'nulls are values'.
  // That means that nulls are equal to other nulls, and they
  // sort higher than other values.
  // For the rest of details, see <ItemLog.h>
  NABoolean specialNulls_;
  // --MV - Irena

  // The following data member is needed by Selectivity Hints
  // feature to assign correct user-specified selectivity after 
  // predicates have been transformed into VEG predicates.
  ValueIdSet predsWithSelectivities_;
}; // class VEGPredicate

// -----------------------------------------------------------------------
// A VEGReference is a special wild card generated by the SQL normalizer.
// It is a reference to a ValueId Equality Group (VEG). Any member
// of the VEG that it references can be used for replacing the
// VEGReference, as long as the value is available at a given point of
// reference.
// Its datatype is computed as the minimum of the values that belong
// to the VEG it references.
// -----------------------------------------------------------------------
class VEGReference : public ItemExpr
{

public:

  VEGReference(const ValueId & vegId);

  virtual ~VEGReference();

  // get the degree of this node (it is a leaf).
  virtual Int32 getArity() const;

  NABoolean referencesVegRefValue(ValueId& ofVegRef);
  void replaceVEG(const ValueId& ofVEG); // used when VEGs are merged
  VEG * getVEG() const { return ((VEG *)(veg_.getItemExpr())); }
  VEG * getVEGToUpdate() { return ((VEG *)(veg_.getItemExpr())); }

  // a virtual function for type propagating the node
  virtual const NAType * synthesizeType();

  virtual NABoolean isCovered(const ValueIdSet& newExternalInputs,
			      const GroupAttributes& newRelExprAnchorGA,
	   	              ValueIdSet& referencedInputs,
			      ValueIdSet& coveredSubExpr,
			      ValueIdSet& unCoveredExpr) const;

  // Each operator supports a (virtual) method for transforming its
  // query tree to a canonical form. The parameter setOfPredExpr is
  // supplied only when predicates are normalized.
  virtual ItemExpr * normalizeNode(NormWA & normWARef);

  virtual OrderComparison sameOrder(ItemExpr *other,
				    NABoolean askOther = TRUE);

  virtual HashValue topHash();
  virtual NABoolean duplicateMatch(const ItemExpr & other) const;
  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);

  // A method for replacing VEGReference and VEGPredicate objects
  // with another expression that belongs to the VEG as well as to the
  // set of availableValues.
  virtual ItemExpr * replaceVEGExpressions
                        (const ValueIdSet& availableValues,
                         const ValueIdSet& inputValues,
                         NABoolean useBridgeValues = TRUE,
                         VEGRewritePairs * lookup = NULL,
                         NABoolean replicateExpression = FALSE,
                         const ValueIdSet * joinInputAndPotentialOutput = NULL,
                         const IndexDesc *iDesc = NULL,
                         const GroupAttributes * left_ga = NULL,
                         const GroupAttributes * right_ga = NULL);

  // get a printable string that identifies the operator
  const NAString getText() const;
  virtual void unparse(NAString &result,
		       PhaseEnum phase = OPTIMIZER_PHASE,
		       UnparseFormatEnum form = USER_FORMAT,
		       TableDesc * tabId = NULL) const;

  virtual QR::ExprElement getQRExprElem() const;

  virtual NABoolean calculateMinMaxUecs(ColStatDescList & histograms,
					       CostScalar & minUec,
					       CostScalar & maxUec);

private:

  // Methods used by the code generator for replacing a reference
  // to a VEG with a reference to one of its members, which also
  // belongs to availableValues.
  ItemExpr * replaceVEGReference(const ValueIdSet& availableValues,
                                 const ValueIdSet& inputValues,
                                 NABoolean useBridgeValues = TRUE, 
                                 const IndexDesc *iDesc = NULL);

  // A ValueId Equality Group (VEG) that is represented by this VEGReference
  ValueId  veg_;

}; // class VEGReference


class BiConnectBy :public ItemExpr {
public:
  BiConnectBy( BiRelat * start,
          BiRelat * conn)
   :
    ItemExpr(ITM_CONNECT_BY),
    noCycle_(FALSE)
  {
    startWith_ = start;
    connectBy_ = (BiConnectByRelat*)conn;
  }
  virtual ~BiConnectBy() {}

  BiRelat * getStartWith() {return startWith_; }
  BiConnectByRelat * getConnectBy() {return connectBy_; }
  NAString getStartWithString() {return startWithString_; }
  void setStartWithString(char *v ) {startWithString_ = v; }
  void setNoCycle(NABoolean v) { noCycle_ = v; }
  NABoolean getNoCycle() {return noCycle_; }
  NAString startWithString_;

private:
  BiRelat * startWith_;
  BiConnectByRelat * connectBy_;
  NABoolean noCycle_;
};


//This RangeSpecRef class is an wrapper for RangeSpec object and also holds a <key> <op> <valueset> relationship
// which needs to be used for traversal and selectivity estimation/generating MDAM Pred for different BiRelational as well.

class RangeSpecRef : public ItemExpr
{
public:

  RangeSpecRef(OperatorTypeEnum otype,
	       OptNormRangeSpec* range,
	       ItemExpr *colValueId /* column Value Id  OR an ItemExpression e.g. a+1 */,
	       ItemExpr *reConsIExpr /* Reconstructed or Rearranged Item Expression 
					after combination of disjoint BiRel at different depth of the tree */
	       );

  virtual ~RangeSpecRef();
  /* Kind of BiRelational: column <in> { Value Id Set } */
  /* That's why it is 2 */
  virtual Int32 getArity() const;
  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);

  virtual void unparse(NAString &result,
                       PhaseEnum phase = OPTIMIZER_PHASE,
                       UnparseFormatEnum form = USER_FORMAT,
                       TableDesc * tabId = NULL) const;

  virtual ItemExpr* removeRangeSpecItems(NormWA* normWA = NULL);
  // method to do code generation
  // short codeGen(Generator*);

  // method to generate Mdam predicates for executor.
  // we need to implement this 
  short mdamPredGen(Generator * generator,
                    MdamPred ** head,
                    MdamPred ** tail,
                    MdamCodeGenHelper & mdamHelper,
                    ItemExpr * parent);
  inline OptNormRangeSpec* getRangeObject() const
  {
    return range_;
  }
  /* This method in turn calls convertToValueIdSet() */
  void getValueIdSetForReconsItemExpr(ValueIdSet &outvs);
  /* This method depends upon the right child pointer */
  const NAString getText() const;
  /* Tree walk is need'ed since it will be called from MaterializeDisjunct()
  and traverse through the tree, however there will be fewer disjunct(s) for MDAM,
  so it is a shallow tree after the conversion. 

                and
                / \
               /   \
              /    and
  RangeSpecRef(a)  /   \
       )          /     \
                 /       \ 
   RangeSpecRef(b)       Or // Bushy here
                         / \
                        /   \
            RangeSpecRef(c) RangeSpecRef(a)
    This is the tree after the conversion, and we need to walk thru this. */
  
  DisjunctArray * mdamTreeWalk();
  
  // method to do code generation
  short codeGen(Generator*);
 private:
  OptNormRangeSpec* range_;

};

ItemExpr* revertBackToOldTree(  CollHeap *heap, ItemExpr* newTree);
void revertBackToOldTreeUsingValueIdSet(  ValueIdSet& inputSet /* IN */, ValueIdSet& outputSet /* OUT */) ;
/*
static void doNotReplaceAnItemExpression( ItemExpr* inputTree, ValueIdSet& inputSet,ItemExpr* parent=0)
{
  ValueIdSet pvalId,storeSet;


  ValueIdSet predId;
  NABoolean predDerivOfLike = FALSE;

  OperatorTypeEnum op = inputTree->getOperatorType();
  if ( (op == ITM_GREATER_EQ) OR
	 (op == ITM_GREATER) OR
	 (op == ITM_LESS) OR
	 (op == ITM_LESS_EQ))
    {
      BiRelat *br = (BiRelat *) inputTree;
      predDerivOfLike = br->derivativeOfLike();
      if (predDerivOfLike)
      {
	predId += inputTree->getValueId();
	pvalId += parent->getValueId();
	inputSet -= predId;
	storeSet += inputSet;
	storeSet.intersectSet(pvalId);
	if (!storeSet.entries())
	{
          inputSet += pvalId; 
	}
      } // if(2)
    } // if(3)
  else if(op == ITM_AND ) 
  {
    for (long i=0; i < (long)(inputTree->getArity()); i++)
    {
	doNotReplaceAnItemExpression(inputTree->child(i),inputSet,inputTree);
    }
  }
}
*/

// Represents the equality predicates that were derived from the transformation
// of a single LIKE predicate.
class LikePredDetails
{
public:
  // Instantiate with the value id of the original like predicate. This vid
  // will be used to identify references to the same LIKE.
  LikePredDetails(ValueId likeVid)
    : likeVid_(likeVid),
      exprs_()   // () inits to all NULLs
    {}

  ~LikePredDetails()
    {}

  ValueId getVid() const
    {
      return likeVid_;
    }

  // Add an ItemExpr for an equality predicate derived from this LIKE. There
  // should be only 2, for instance >= and < for a LIKE pred with a regexp
  // like 'abc%'. I'm not sure how <= and > occur, but they are included in
  // code for LIKE predicates elsewhere.
  // If this instance already has an expression of the passed type, it is
  // overwritten with the new one. This is a common case, as duplicates are
  // often generated. They will be removed by this action.
  void addExpr(ItemExpr* expr)
    {
      switch (expr->getOperatorType())
        {
          case ITM_GREATER_EQ:
            exprs_[0] = expr;
            break;
          case ITM_GREATER:
            exprs_[1] = expr;
            break;
          case ITM_LESS:
            exprs_[2] = expr;
            break;
          case ITM_LESS_EQ:
            exprs_[3] = expr;
            break;
          default:
            CMPASSERT(FALSE);
            break;
        }
    }

  // Build and return a left-linear AND backbone of the LIKE's inequality
  // expressions.
  ItemExpr* reconstructLikeExpr() const
    {
      ItemExpr* left;
      ItemExpr* retVal = NULL;
      for (Int16 i=0; i<4; i++)
        {
          if (exprs_[i])
            {
              if (!retVal)
                retVal = exprs_[i];
              else
                {
                  left = retVal;
                  retVal = new(STMTHEAP) BiLogic(ITM_AND, left, exprs_[i]);
                }
            }
        }

      return retVal;
    }

private:
  ValueId likeVid_;
  ItemExpr* exprs_[4];
};

// Restores like predicate (range predicate) handling for cardinality estimate fix.
static void doNotReplaceAnItemExpressionForLikePredicates(ItemExpr* inputTree,
                                                          ValueIdSet& inputSet,
                                                          ItemExpr* parent=NULL)
{
  NAList<LikePredDetails*> likeList(STMTHEAP);
  ValueIdSet vidsToRemove;

  // For each inequality expression in inputSet, see if it is derived from the
  // transformation of a LIKE predicate; if so, create a LikePredDetails object
  // for that LIKE pred instance (unless we already have one) and add the
  // inequality to it. This will effectively remove any duplicates, as only one
  // of each inequality type is maintained in LikePredDetails. The value id of
  // the inequality predicate is added to a list that will be removed from inputSet
  // after the loop, and a new predicate for each LikePredDetails will be added.
  for (ValueId vid = inputSet.init(); inputSet.next(vid); inputSet.advance(vid))
    {
      ItemExpr* ie = vid.getItemExpr();
      OperatorTypeEnum op = ie->getOperatorType();
      if (op == ITM_GREATER_EQ ||
          op == ITM_GREATER    ||
          op == ITM_LESS       ||
          op == ITM_LESS_EQ)
        {
          ValueId likeVid = (static_cast<BiRelat*>(ie))->originalLikeExprId();
          if (likeVid != NULL_VALUE_ID)
            {
              LikePredDetails* lpd = NULL;
              for (CollIndex i=0; i<likeList.entries() && !lpd; i++)
                {
                  if (likeList[i]->getVid() == likeVid)
                    lpd = likeList[i];
                }
              if (!lpd)
                {
                  lpd = new(STMTHEAP) LikePredDetails(likeVid);
                  likeList.insert(lpd);
                }
              lpd->addExpr(ie);

              // Save the vids to remove and do it after loop so as not to disrupt iteration.
              vidsToRemove += vid;
            }
        }
    }

  // Remove the vids for the inequality preds that were derived from a LIKE pred,
  // and generate/add an AND bilogic pred for each LIKE.
  inputSet -= vidsToRemove;
  for (CollIndex i=0; i<likeList.entries(); i++)
    {
      ItemExpr* likeExpr = likeList[i]->reconstructLikeExpr();
      likeExpr->synthTypeAndValueId(FALSE);
      inputSet += likeExpr->getValueId();
    }
}

// removes non key columns from selectionpredicates to form disjuncts.
static void usePartofSelectionPredicatesFromTheItemExpressionTree(ValueIdSet& inputSet,
								  ValueIdSet KeyColumnSet
								  ) 
{
// This method is protected by comp_bool_144 for separate testing
  if (CmpCommon::getDefault(COMP_BOOL_144) == DF_ON ){
  ValueIdSet result;
  for (ValueId predId = inputSet.init();
       inputSet.next(predId);
       inputSet.advance(predId) )
  {
    OperatorTypeEnum op = predId.getItemExpr()->getOperatorType();
    if (op == ITM_RANGE_SPEC_FUNC )
    {
      predId.getItemExpr()->child(1)->findAll(ITM_INDEXCOLUMN, result, TRUE, TRUE);
    }
    else
      predId.getItemExpr()->findAll(ITM_INDEXCOLUMN, result, TRUE, TRUE);
    if(!result.convertToBaseIds().intersectSet(KeyColumnSet).entries())
    {
      inputSet -= predId;
    }
    result.clear();
  }
 }
}

#endif /* ITEMOTHER_H */
