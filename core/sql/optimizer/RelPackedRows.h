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
#ifndef RELPACKEDROWS_H
#define RELPACKEDROWS_H
/* -*-C++-*-
******************************************************************************
*
* File:         RelPackedRows.h
* Description:  Class definitions for the classes UnPackRows
*               and PhysUnPackRows
* Created:      6/17/97
* Language:     C++
*
*
*
*
******************************************************************************
*/

#include "RelExpr.h"
#include "PackedColDesc.h"
#include "NAString.h"

class BindWA;

// The following two internal-only virtual table name prefixes contain
// a non-Ansi character so is guaranteed not to conflict with any
// possible external user table.
//
// PACKED__ should be used to rename scan-only tables.
// PACKED__IDU__ should be used to rename non-scan-access tables.
//
// This use of distinct prefixes is necessary as otherwise improper
// sharing of renamed table instances with scan-access and non-scan-access
// mode in Compound Statements will lead to binding failure.
//
#define PACKED__	FUNNY_INTERNAL_IDENT("PACKED__")
#define PACKED__IDU__	FUNNY_INTERNAL_IDENT("PACKED__IDU__")
//



// Class UnPackRows ----------------------------------------------------
// The UnPackRows RelExpr is a logical RelExpr node used to implement
// row unpacking.  This node is introduced by Scan::bindNode() when it
// is determined that the table being accessed is packed.  This node is
// implemented by the physical node PhysUnPackRows.  The data members of
// this class are:
//
//  long maxPackingFactor_: This is the packing factor of the packed
//  table.  This value is stored so that the number of rows generated
//  by this node can be estimated.
//
//  ItemExpr *unPackExprTree_: This expression contains a list
//  of expressions to unpack the SYSKEY and all the user columns
//  of the packed table.  The SYSKEY is unpacked by 'calculating'
//  the SYSKEY value based on an initial value of SYSKEY for the
//  packed row and the current value of the index into the packed
//  row.  The user columns are unpacked using the UnPackCol expression
//  based on the value of the index.  The index is provided at runtime
//  thru a hostVar.  The valueId of this HostVar must be captured so
//  that it can be used to map a local variable into the workAtp of
//  the work procedure.
//
//  ValueIdList unPackExpr_: This represents the bound version of
//  unPackExprTree_.  The order of this list is important. It must
//  correspond to the order of the packingFactor_ valueIdList.
//
//  ItemExpr *packingFactorTree_.  This expression contains a list
//  of expressions to extract the NUMROWS field from each packed
//  column.  During normalization, this list will be reduced to
//  only one. (The value of NUMROWS is the same in each packed column).
//
//  ValueIdList packingFactor_: This represents the bound version of
//  packingFactorTree_.  The order of this list is important. It must
//  correspond to the order of the unPackExpr_ valueIdList.
//
//  ValueId indexValue_: The valueId of the HostVar used to provide
//  the index to the packed row.  This valueId will be used in the
//  generator to map a local variable to the WorkATP.
//
class UnPackRows : public RelExpr
{
public:

  // The constructor
  //
  UnPackRows(Lng32 maxPackingFactor = 0,
	     ItemExpr *UnPackExpr = NULL,
	     ItemExpr *packingFactor = NULL,
	     TableDesc *unPackedTable = NULL,
	     RelExpr *child = NULL,
	     ValueId   indexHostVarValueId = NULL_VALUE_ID,
	     CollHeap *oHeap = CmpCommon::statementHeap());

  UnPackRows(Lng32 rwrsMaxSize,
	     ItemExpr *rwrsInputSizeExpr,
	     ItemExpr *rwrsMaxInputRowlen,
	     ItemExpr *rwrsBuffer,
	     RelExpr *child,
	     CollHeap *oHeap = CmpCommon::statementHeap());

  // The destructor
  //
  virtual ~UnPackRows();

  // UnPackRows has one child.
  //
  virtual Int32 getArity() const {return 1;};

  // The potential packing factor of each row.
  //
  inline Lng32 getMaxPackingFactor() { return maxPackingFactor_; };

  // Return a pointer to the UnPackExpr tree.
  //
  inline const ItemExpr *unPackExprTree() const {return unPackExprTree_;};
  inline ItemExpr *&unPackExprTree() {return unPackExprTree_;};

  // Return a pointer to the unPackExpr tree after
  // setting it to NULL.
  //
  ItemExpr *removeUnPackExprTree();

  // return a (short-lived) read/write reference to the unPack expression
  //
  inline ValueIdSet & unPackExpr() { return unPackExpr_; }

  // return a read-only reference to the unPack expression
  //
  inline const ValueIdSet & unPackExpr() const { return unPackExpr_; }

  // Return a pointer to the packingFactor tree.
  //
  inline const ItemExpr *packingFactorTree() const
  {
    return packingFactorTree_;
  };
  inline ItemExpr *&packingFactorTree() {return packingFactorTree_;};

  // Return a pointer to the packing Factor ColReference node after setting
  // it to NULL.
  //
  ItemExpr *removePackingFactorTree();

  // Return the packingFactor ValueIdList
  //
  inline ValueIdSet &packingFactor() { return packingFactor_; };

  inline ValueIdSet packingFactor() const { return packingFactor_; };

  inline ValueId &indexValue() { return indexValue_; };

  inline ValueId &sysKeyId() { return sysKeyId_; };

  inline ValueIdList *&originalPreds() { return originalPreds_; };
  inline ValueIdList *&rewrittenPreds() { return rewrittenPreds_; };

  inline TableDesc* & unPackedTable()
  {
    return unPackedTable_;
  }

  inline TableDesc *unPackedTable() const
  {
    return unPackedTable_;
  }

  inline ColStatDescList &colStats()
  {
    return colStats_;
  }

  NABoolean rowwiseRowset() const { return rowwiseRowset_; }
  void setRowwiseRowset(NABoolean v) { rowwiseRowset_ = v; }

  const ItemExpr *rwrsInputSizeExpr() const 
  { return rwrsInputSizeExpr_;}
  const ItemExpr *rwrsMaxInputRowlenExpr() const 
  { return rwrsMaxInputRowlenExpr_;}
  const ItemExpr *rwrsBufferAddrExpr() const 
  { return rwrsBufferAddrExpr_;}

  ItemExpr *rwrsInputSizeExpr()
  { return rwrsInputSizeExpr_;}
  ItemExpr *rwrsMaxInputRowlenExpr()
  { return rwrsMaxInputRowlenExpr_;}
  ItemExpr *rwrsBufferAddrExpr()
  { return rwrsBufferAddrExpr_;}
  ValueIdList &rwrsOutputVids() { return rwrsOutputVids_; }

  void setRwrsInputSizeExpr(ItemExpr * v)
  {rwrsInputSizeExpr_ = v;}
  void setRwrsMaxInputRowlenExpr(ItemExpr * v)
  {rwrsMaxInputRowlenExpr_ = v;}
  void setRwrsBufferAddrExpr(ItemExpr * v)
  {rwrsBufferAddrExpr_ = v;}
  void setRwrsOutputVids(ValueIdList vidl)
  {rwrsOutputVids_ = vidl;}

  // a virtual function for performing name binding within the query tree
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
  // UnPackRows cannot pull up any preds. so redefine this method.
  //
  virtual void pullUpPreds();

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
  // The default implementation is adequate for UnPackRows
  //virtual RelExpr * normalizeNode(NormWA & normWARef);

  // Method to push down predicates from a UnPackRows node into the
  // children
  //
  virtual
  void pushdownCoveredExpr(const ValueIdSet & outputExprOnOperator,
                           const ValueIdSet & newExternalInputs,
                           ValueIdSet& predOnOperator,
			   const ValueIdSet * nonPredExprOnOperator = NULL,
                           Lng32 childId = (-MAX_REL_ARITY) );

  // Return a the set of potential output values of this node.
  // For UnPackRows, the generated syskey column,  and all the value
  // columns generated by UnPackRows
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
  virtual const NAString getText() const {return "UnPack";};

private:

  void constructNewSyskeyPreds(ValueIdSet &predicates);

  // This is the packing factor of the packed table.  This value
  // is stored so that the number of rows generated by this node
  // can be estimated.
  //
  Lng32 maxPackingFactor_;


  // This expression contains a list of expressions to unpack the
  // SYSKEY and all the user columns of the packed table.  The SYSKEY
  // is unpacked by 'calculating' the SYSKEY value based on an initial
  // value of SYSKEY for the packed row and the current value of the
  // index into the packed row.  The user columns are unpacked using
  // the UnPackCol expression based on the value of the index.  The
  // index is provided at runtime thru a hostVar.  The valueId of this
  // HostVar must be captured so that it can be used to map a local
  // variable into the workAtp of the work procedure.
  //
  ItemExpr   *unPackExprTree_;

  // This represents the bound version of unPackExprTree_.  The order
  // of this list is important. It must correspond to the order of the
  // packingFactor_ valueIdList.
  //
  ValueIdSet unPackExpr_;

  // This expression contains a list of expressions to extract the
  // NUMROWS field from each packed column.  During normalization,
  // this list will be reduced to only one. (The value of NUMROWS is
  // the same in each packed column).
  //
  ItemExpr *packingFactorTree_;

  // This represents the bound version of packingFactorTree_.  The
  // order of this list is important. It must correspond to the order
  // of the unPackExpr_ valueIdList.
  //
  ValueIdSet packingFactor_;

  // The valueId of the HostVar used to provide the index to the packed
  // row.  This valueId will be used in the generator to map a local
  // variable to the WorkATP.
  //
  ValueId indexValue_;

  ValueId sysKeyId_;

  ValueIdList *originalPreds_;
  ValueIdList *rewrittenPreds_;

  // A TableDesc to the logical table produced by this UnPackRows node.
  // This is created only to hold statistics for the logical unpacked
  // table. The vid's stored in this TableDesc are not referenced above
  // the tree at all.
  //
  TableDesc *unPackedTable_;

  ColStatDescList colStats_;

  // if the rows are packed rowwise
  NABoolean rowwiseRowset_;
  ItemExpr *rwrsInputSizeExpr_;
  ItemExpr *rwrsMaxInputRowlenExpr_;
  ItemExpr *rwrsBufferAddrExpr_;

  // value ids of values which are returned by this operator.
  ValueIdList rwrsOutputVids_;

}; // class UnPackRows

// Class PhysUnPackRows ----------------------------------------------------
// The PhysUnPackRows node replaces the logical UnPackRows node through the
// application of the PhysUnPackRowsRule. This transformation is
// designed to present a purely physical verison of this operator
// that is both a logical and physical node. The PhysUnPackRows node
// does not add any data members. It adds a few virtual methods.
// -----------------------------------------------------------------------
class PhysUnPackRows : public UnPackRows
{
public:

  // The constructor
  //
  PhysUnPackRows(RelExpr *child = NULL,
                        CollHeap *oHeap = CmpCommon::statementHeap())
    : UnPackRows(0, NULL,NULL,NULL,child, NULL_VALUE_ID, oHeap)
  {
  };

  // The destructor.
  //
  virtual ~PhysUnPackRows();

  // methods to do code generation of the physical node.
  //
  virtual RelExpr * preCodeGen(Generator * generator,
			       const ValueIdSet & externalInputs,
			       ValueIdSet &pulledNewInputs);
  virtual short codeGen(Generator*);

  // generate CONTROL QUERY SHAPE fragment for this node.
  //
  virtual short generateShape(CollHeap * space, char * buf, NAString * shapeStr = NULL);

  // Copy a chain of derived nodes (Calls UnPackRows::copyTopNode).
  // Needs to copy all relevant fields (in this case no fields
  // need to be copied)
  // Used by the Cascades engine.
  //
  virtual RelExpr * copyTopNode(RelExpr *derivedNode = NULL,
				CollHeap *outHeap = NULL);

  // cost functions
  //
  virtual PhysicalProperty *synthPhysicalProperty(const Context *context,
                                                  const Lng32    planNumber,
                                                  PlanWorkSpace  *pws);
  virtual CostMethod* costMethod() const;

  // Redefine these virtual methods to declare this node as a
  // physical node.
  //
  virtual NABoolean isLogical() const {return FALSE;};
  virtual NABoolean isPhysical() const {return TRUE;};

}; // class PhysUnPackRows

#endif
