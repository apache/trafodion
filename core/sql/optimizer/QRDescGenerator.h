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
#ifndef QUERYDESCRIPTOR_H
#define QUERYDESCRIPTOR_H
/* -*-C++-*-
 **************************************************************************
 *
 * File:         QRDescGenerator.h
 * Description:  MV analyzer class and methods
 * Created:      01/24/2008
 * Language:     C++
 *
 **************************************************************************
 */

#include "Analyzer.h"
#include "OptRange.h"
#include "XMLUtil.h"
#include "QRDescriptor.h"
#include "QRLogger.h"
#include "QRMessage.h"

/**
 * \file
 * Contains the class definition for the class used to generate a query
 * descriptor from Analyzer structures.
 */

// Classes defined in this file.
class QRDescGenerator;
class EqualitySet;
class SetRefVisitor;

// Other forward declarations.
class OptRangeSpec;

ULng32 hashString(const NAString& str);
ULng32 hashValueId(const QRValueId& vid);

#ifdef _MEMSHAREDPTR
typedef QRIntrusiveSharedPtr<SetRefVisitor> SetRefVisitorPtr;
#else
typedef SetRefVisitor* SetRefVisitorPtr;
#endif

class SetRefVisitor : public Visitor
{
  public:
    SetRefVisitor(const QRElementHash& idHash,
                  ADD_MEMCHECK_ARGS_DECL(CollHeap* heap))
      : Visitor(ADD_MEMCHECK_ARGS_PASS(heap)),
        idHash_(idHash)
    {}

    virtual Visitor::VisitResult visit(QRElementPtr caller);

  private:
    const QRElementHash& idHash_;
}; // class SetRefVisitor
ULng32 hashQRValueId(const QRValueId& vid);

/**
 * Class that implements the interface specified by the Visitor abstract class
 * and is used to traverse the tree structure of a partial descriptor to see
 * if that partial descriptor contains something that will generate XML that 
 * may not be parseable. For example: string literals containing the '%' and
 * '&' characters.
 */
class XmlValidatorVisitor : public Visitor
{
  public: 
    XmlValidatorVisitor(ADD_MEMCHECK_ARGS_DECL(CollHeap* heap))
      : Visitor(ADD_MEMCHECK_ARGS_PASS(heap)),
        foundProblem_(FALSE)
    {}

    virtual VisitResult visit(QRElementPtr caller);
  
    NABoolean foundProblem() const
    {
      return foundProblem_;
    }

private:
  NABoolean foundProblem_;
}; // class XmlValidatorVisitor

#ifdef _MEMSHAREDPTR
typedef QRIntrusiveSharedPtr<XmlValidatorVisitor> XmlValidatorVisitorPtr;
#else
typedef XmlValidatorVisitor* XmlValidatorVisitorPtr;
#endif

/**
 * Class representing an equality set, a group of columns, expressions, and
 * values that are known to be equal due to equality predicates appearing in
 * the query. This is similar to a VEG, but is more inclusive in that it can
 * contain expressions, and exploits transitivity more fully than VEGs where
 * dynamic parameters are involved.
 * \n\n
 * An EqualitySet can yield a JoinPred element consisting of one column/expression
 * from each of two or more tables. The remaining members of the EqualitySet
 * are linked to the JoinPred through either range or residual predicates.
 * \n\n
 * This class may be treated as a list, since it subclasses a specialization of
 * the NAList template class. The list members are the item expressions that
 * comprise the equality set.
 */
class EqualitySet : public NAList<ItemExpr*>
{
  public:
    /**
     * Instantiates an initially empty equality set.
     * @param heap Heap to use with the underlying list.
     */
    EqualitySet(CollHeap* heap)
      : NAList<ItemExpr*>(heap),
        heap_(heap),
        joinPredId_(NULL_VALUE_ID),
        type_(NULL)
      {
        delete type_;
      }

    virtual ~EqualitySet()
      {}

  /**
   * Returns the id of the \c JoinPred associated with this EqualitySet.
   * @return The \c JoinPred id, or 0 if there is none.
   */
  UInt32 getJoinPredId() const
    {
      return joinPredId_;
    }

  /**
   * Sets the id of the \c JoinPred associated with this EqualitySet. This is used
   * to reference the \c JoinPred in range or residual predicates that are members
   * of the equality set, but outside the JoinPred.
   * @param id The \c JoinPred id.
   */
  void setJoinPredId(UInt32 id)
    {
      joinPredId_ = id;
    }

  /**
   * Returns the type of the equality set. For numeric types, this is the most
   * restrictive type common to all members of the equality set. For example,
   * if there are two members of the equality set with types smallint unsigned
   * and numeric(4,2), the type of the equality set as a whole would be
   * numeric(4,2) unsigned.
   *
   * @return The type of the equality set.
   */
  const NAType* getType()
    {
      if (!type_)
        determineType();
      return type_;
    }

  private:
    /**
     * Sets the type for the equality set according to the rules outlined in
     * getType().
     */
    void determineType();

    UInt32 joinPredId_;
    NAType* type_;
    CollHeap* heap_;
};  // EqualitySet

/**
 * Class that performs MV Analysis of a query expression.
 * It uses the QueryAnalysis object (performed on the
 * query expression) for its processing.
 */
class QRDescGenerator : public NABasicObject
{
  friend class OptRangeSpec;
public:

  QRDescGenerator(NABoolean bFormatted, CollHeap* heap)
    : mvqrHeap_(heap),
      bFormatted_(bFormatted),
      bSortJoinPredicateCols_(TRUE),
      bGenColumnRefs_(TRUE),
      generatedJBBid_(GENERATED_JBBID_START),
      putAllJBBsInQD_(CmpCommon::getDefault(MVQR_ALL_JBBS_IN_QD) == DF_ON),
      descriptorType_(ET_INVALID),
      relExpr_(NULL),
      currentJBB_(NULL),
      rangeColHash_(hashValueId, 77, TRUE, heap),
      rangeExprHash_(hashString, 77, TRUE, heap),
      vegsUsedHash_(hashValueId, 77, TRUE, heap),
      exprsUsedHash_(hashString, 77, TRUE, heap),
      colTblIdHash_(hashString, 77, TRUE, heap),
      isDumpMvMode_(FALSE)
    {
      maxExprSize_   = CmpCommon::getDefaultLong(MVQR_MAX_EXPR_SIZE);
      maxExprDepth_  = CmpCommon::getDefaultLong(MVQR_MAX_EXPR_DEPTH);
    }

  virtual ~QRDescGenerator();

  static NABoolean typeSupported(const NAType* type);

  /**
   * Checks to see if at least one of the MVs in the list is enabled for
   * query rewrite.
   *
   * @param tableDesc Table descriptor containing the list of MVs.
   * @return \c TRUE iff at least one MV in the list is rewrite-enabled.
   */
  static NABoolean hasRewriteEnabledMVs(TableDesc* tableDesc);

  /**
   * Determines whether or not the passed JBB should be included in the query
   * descriptor. The criteria are that two or more of the tables in the JBB
   * must have MVs defined on them, or one table with MVs defined on it and a
   * Group By node. When constructing an MV descriptor, all JBBs are included
   * without regard to these criteria.
   *
   * @param jbb The JBB to screen.
   * @return \c TRUE iff the JBB should be included in the descriptor and be
   *         eligible for rewrite.
   */
  NABoolean qrNeededForJBB(JBB* jbb);

  /**
   * Determines whether or not an isolated Scan node should be included in the
   * query descriptor as a pseudo-JBB. The table referenced by the Scan must be
   * used in at least 1 MV, and the Scan must have a Group By node immediately
   * above it. When constructing an MV descriptor, all JBBs are included without
   * regard to these criteria. The need for this function is temporary, until 
   * single-node JBBs are provided by the Analyzer. Currently, a single Scan
   * node is not treated as a JBB.
   *
   * @param parent The parent of the Scan node. This must be a Group By if a
   *               JBB element is to be added to the query descriptor for the
   *               Scan node.
   * @param scanExpr The Scan node to evaluate.
   * @return \c TRUE iff the Scan node is to be included in the descriptor as a
   *         JBB element, and thus eligible for rewrite.
   */
  NABoolean qrNeededForTable(RelExpr* parent, RelExpr* scanExpr);

  /**
   * Used to specify the MV Table Name. Needs to be called if 
   * an MV Descriptor is to be generated.
   *
   * @param MVTableName 
   */
  void setMVTableName(NAString& MVTableName)
  {
    mMVTableName_ = MVTableName;
  }

  const NAString& getMVTableName() const
  {
    return mMVTableName_;
  }

  /**
   * Whether the columns in a join predicate should be sorted
   * (by column name).
   */
  void setSortJoinPredicateCols(NABoolean bWhat=TRUE)
  {
    bSortJoinPredicateCols_ = bWhat;
  }

  NABoolean getSortJoinPredicateCols() const
  {
    return bSortJoinPredicateCols_;
  }

  /**
   * Whether column refs should be generated (for the same column id)
   */
  void setGenColumnRefs(NABoolean bWhat = FALSE)
  {
    bGenColumnRefs_ = bWhat;
  }
  
  NABoolean getGenColumnRefs() const
  {
    return bGenColumnRefs_;
  }

  /**
   * Generates a unique id value for a "pseudo-JBB". The only such case is
   * currently the sole table in a query or subquery where there is no join.
   * The Analyzer will soon create JBBs for these singletons, and our special
   * treatment will no longer be necessary.
   *
   * @return A generated id for a JBB.
   */
  UInt32 genJBBid()
  {
    return generatedJBBid_--;
  }

  /**
   * Returns the equality set that the passed \c ValueId is part of, or
   * \c NULL if it is not contained in an equality set.
   *
   * @param vidPtr Pointer to the ValueId.
   * @return Pointer to the equality set found, if any.
   */
  EqualitySet* getEqualitySet(QRValueId* vidPtr)
    {
      return vegsUsedHash_.getFirstValue(vidPtr);
    }

  /**
   * Returns the equality set containing the passed expression, or \c NULL if
   * no such equality set exists. The expression is represented by its
   * normalized text.
   *
   * @param strPtr Pointer to the text of the expression.
   * @return Pointer to the equality set found, if any.
   */
  EqualitySet* getEqualitySet(NAString* strPtr)
    {
      return exprsUsedHash_.getFirstValue(strPtr);
    }

  /**
   * Populate the passed descriptor with the JBBs from the underlying query or
   * materialized view.
   *
   * @param descPtr Pointer to the query or MV descriptor.
   * @param qa The QueryAnalysis for the query to get a descriptor for.
   * @return \c TRUE if one or more JBBs were found, \c FALSE otherwise (in
   *         which case the descriptor will not be generated.
   */
  NABoolean processJBBs(QRDescriptorPtr descPtr,
                        QueryAnalysis* qa);

  /**
   * Logs the list of columns for which the corresponding bit is set in the
   * passed bitmap.
   *
   * @param table The table owning the columns covered by the bitmap.
   * @param bitmap The bitmap.
   * @param predType Indicator of whether the bitmap shows the columns having
   *                 a range predicate, or those having a residual predicate.
   *                 This is incorporated into the message displayed in the log.
   */
  void logColumnBitmap(QRTablePtr table,
                       const XMLBitmap& bitmap,
                       ElementType predType);
  /**
   * Derives and returns a query descriptor from a QueryAnalysis object.
   *
   * @param qa The QueryAnalysis for the query to get a descriptor for.
   * @param expr Root of the query tree. This is used to look for singleton
   *             scan nodes that are not part of a join, and create JBBs
   *             for them.
   * @return The query's descriptor, or NULL if the query has no JBBs.
   */
  QRQueryDescriptorPtr createQueryDescriptor(QueryAnalysis* qa, RelExpr* expr);

  /**
   * Creates a materialized view descriptor from a QueryAnalysis object.
   *
   * @param qa The QueryAnalysis for the query used in the MV.
   * @param expr Root of the query tree. This is used to look for singleton
   *             scan nodes that are not part of a join, and create JBBs
   *             for them.
   * @return The MV descriptor.
   */
  QRMVDescriptorPtr createMvDescriptor(QueryAnalysis* qa, RelExpr* expr);

  /**
   * Tells whether this is an MV descriptor or a query descriptor.
   * @return Element type enumeration value corresponding to the descriptor type.
   */
  ElementType getDescriptorType() const
    {
      return descriptorType_;
    }

  /**
   * Serializes a descriptor object to XML form.
   * @param desc The descriptor.
   * @return The XML text for the descriptor.
   */
  XMLString* createXmlText(QRElementPtr desc);

  /**
   * Sets the bit corresponding to the passed column value id in either the
   * table's range or residual predicate bit map.
   *
   * @param colVid Value id of the column to set the bit for. 
   * @param elemType Indicates whether the range or residual bitmap should
   *                 be used.
   */
  void setPredBitmap(QRValueId colVid, ElementType elemType);

  /**
   * Saves information on a range specification for later use. A RangeInfo
   * object, which associates a range specification with its owning range
   * predicate list, is looked for in one of the range hash tables. If found,
   * the range specification is merged with the one in that %RangeInfo. If not,
   * a new %RangeInfo is created with the passed range spec and added to the
   * appropriate hash table.
   *
   * @param range The range specification to save. If its content is merged into
   *              an existing one for the same column or expression, it is deleted.
   * @param jbbElem Ptr to the JBB element owning the range predicate list the
   *                range belongs to.
   */
  void storeRangeInfo(OptRangeSpec* range, QRJBBPtr jbbElem);

  const QRElementHash& getColTblIdHash() const
    {
      return colTblIdHash_;
    }

  QRElementPtr getElementForValueID(ValueId& id) const;
  
  const NAList<EqualitySet*>& getAllEqualitySets() const
    {
      return allEqualitySets_;
    }

  /**
   * Adds information from data structures of another QRDescGenerator into this
   * one. Rangespec processing in the Normalizer uses a separate generator before
   * we get to query rewrite, and some columns have already been assigned ids,
   * entered in the hash table, etc. Those must be incorporated into the descriptor
   * generator we use in mvqr processing. Only certain data structures are affected:
   * the used columns lists for regular and extra-hub tables, and the hash table of
   * column and table ids. I tried creating the generator where it could be passed
   * to both places so a single one could be used, but there were cases where this
   * could not be done.
   * 
   * @param other The descriptor generator providing the data to merge into this
   *              one, typically from Normalizer.
   */
  void mergeDescGenerator(const QRDescGenerator* other);

  /**
   * Creates the equality sets implied by any vegpreds or equality predicates
   * in \c preds. Hash tables are also created that map value ids of columns
   * or expressions to the equality set they participate in. These structures
   * allow the Normalizer to use the correct vegref ValueId instead of a
   * base column vid when creating rangespec operators (Bug 2707). This function
   * is only called by the Normalizer; for MVQR, the private function
   * formEqualitySets() is called directly during its processing of predicates.
   *
   * @param preds The set of predicate ValueIds to create equality sets from.
   *              This is passed by value so the copy ctor will be invoked and
   *              protect the argument from the destructive effects of
   *              formEqualitySets(), which removes any vegpreds or equality
   *              preds from the list.
   */
  void createEqualitySets(ValueIdSet preds)
    {
      NAList<EqualitySet*> equalitySets;
      formEqualitySets(preds, equalitySets);
    }

  NABoolean isDumpMvMode()
  {
    return isDumpMvMode_;
  }    
  
  void setDumpMvMode()
  {
    isDumpMvMode_ = TRUE;
  }

  /**
   * Tells whether the passed value id is that of a joinpred.
   * @param vid The value id to look up.
   * @return True iff the vid is a joinpred id.
   */
  NABoolean isJoinPredId(ValueId vid)
  {
    return colToJoinPredMap_.getTopValues().contains(vid);
  }

private:
  /**
   * Adds a JoinPred element to a JBB element of a descriptor. The elements
   * passed in the array are examined to see if they belong to the hub or
   * extrahub, and placed in the appropriate JoinPred.
   *
   * @param jbbElem The JBB element to add the JoinPred to.
   * @param qrElemArray Sorted array of elements comprising the JoinPred.
   * @param idArray Array of ids corresponding to the elements.
   * @param eqCount Number of elements in the JoinPred.
   * @param [out] hubJoinPredId The id of the JoinPred element for the hub.
   */
  void addJoinPred(QRJBBPtr jbbElem, QRElementPtr* qrElemArray,
                   Int32* idArray, Int32 eqCount, UInt32& hubJoinPredId);

  /**
   * Returns the node id of the passed item expression, or \c NULL_CA_ID if no
   * single node contains the entire expression.
   *
   * @param itemExpr Item expression to check.
   * @return The CA node id for the expression, or \c NULL_CA_ID if it is a
   *         multi-node expression.
   */
  CANodeId getExprNode(ItemExpr* itemExpr);

  /**
   * Goes through the members of an equality set, identifying the JoinPred
   * items, and creating range or residual predicates for the remaining members
   * that reference the JoinPred.
   *
   * @param jbbElem JBB element the equality set is part of.
   * @param eqSet The equality set.
   */
  void processEqualitySet(QRJBBPtr jbbElem, EqualitySet& eqSet);

  VEGPredicate* getVegPredicate(UInt32 hubJoinPredId);

  /**
   * Adds a range predicate expressing equality of #rangeItemExpr to the
   * constant #constItem. This is used when there is a constant included in an
   * equality set, as well as another member that is not a join pred operand.
   *
   * @param rangeItemExpr The non-joinpred member of the equality set that is
   *                      to be equated to the constant.
   * @param type Type associated with the equality set.
   * @param jbbElem JBB element the equality set is part of.
   * @param constItem The constant the range item is equated to.
   * @see #processEqualitySet
   */
   void addEqualityRangePred(ItemExpr* rangeItemExpr,
                             const NAType* type,
                             QRJBBPtr jbbElem,
                             ConstValue* constItem);

  /**
   * Adds a range predicate expressing equality of a JoinPred to a constant.
   * This is used when there is a constant included in an equality set, and two
   * or more of the other equality set members form an equijoin predicate.
   * 
   * @param hubJoinPredId Id of the hub JoinPred.
   * @param type Type associated with the equality set.
   * @param jbbElem JBB element the equality set is part of.
   * @param constItem The constant the range item is equated to.
   * @see #processEqualitySet
   */
   void addEqualityRangePred(UInt32 hubJoinPredId,
                             const NAType* type,
                             QRJBBPtr jbbElem,
                             ConstValue* constItem);

  /**
   * Adds a residual predicate to a JBB element for an equality condition.
   * This function is called to create a predicate for an equality relationship
   * between between JoinPred and a non-JoinPred member of the same equality
   * set, or one of a chain of equality conditions linking all members of an
   * equality set if the set does not contain a JoinPred (which happens when
   * only a single table is involved).
   *
   * @param jbbElem JBB element the residual predicate will belong to.
   * @param op1 Left-hand side of the equality predicate.
   * @param op2 Right-hand side of the equality predicate, or NULL if a join
   *            pred is referenced.
   * @param hubJoinPredId Id of the join pred that op1 shares equality with,
   *                      if such a join pred exists.
   * @see #processEqualitySet
   */
  void addEqualityResidPred(QRJBBPtr jbbElem,
                            ItemExpr* op1,
                            ItemExpr* op2,
                            ValueId hubJoinPredId = NULL_VALUE_ID);

  /**
   * Adds the source equality set to the destination, and removes the source.
   * This is necessary when two equality sets exist and an equality condition
   * is encountered between a member of one and a member of the other. This also
   * requires changing the hash tables that have equality sets as the values,
   * for any entry with the source as the value.
   *
   * @param source Equality set to add to the other one.
   * @param destination Equality set to add to.
   * @param equalitySets The list of all equality sets, needed so the source
   *                     equality set can be removed.
   */
  void combineEqSets(EqualitySet* source,
                     EqualitySet* destination,
                     NAList<EqualitySet*>& equalitySets);

  /**
   * Adds the members of #vegVals into the equality set #eqSet, creating that
   * equality set if it is \c NULL. The appropriate hash tables are also
   * updated.
   *
   * @param vegPred VEG predicate members are derived from. \c NULL if called
   *                for an equality pred instead of a vegpred.
   * @param vegVid ValueId of a VEG being added.
   * @param vegVals Set of ValueIds to add. These are often the base columns of
   *                the Veg referred to by #vegVid, but are passed as a
   *                \ValueIdSet, because some may have been removed for various
   *                reasons.
   * @param eqSet Equality set to add to. This may be null, in which case a new
   *              one is allocated.
   * @param equalitySets The list of all equality sets. If a new equality set
   *                     must be allocated, add it to this list.
   */
  void putVegMembersInEqualitySet(
              ItemExpr* vegPred,
              const ValueId& vegVid,
              const ValueIdSet& vegVals,
              EqualitySet*& eqSet,
              NAList<EqualitySet*>& equalitySets);

  /**
   * Adds the members of a VEGPredicate to an equality set. Some of the members
   * may already belong to different equality sets. For instance, if the preds
   * a=b+1 and c=d+1 have been processed, the sets {a, b+1} and {c, d+1} will
   * be present (expressions are not part of VEGs). When the VEGPredicate
   * consisting of a and b is processed, the two existing equality sets will
   * be merged. If only a=b+1 has been seen, then the VEGPredicate members
   * (other than a) will be added to {a, b+1}. If no member of the veg pred is
   * already a member of an existing equality set, a new one is created.
   *
   * @param vegPred The VegPredicate whose members will be added to an equality
   *                set.
   * @param equalitySets List of equality sets, in case a new one is created and
   *                     must be added to it.
   */
  void addVegPredToEqualitySets(
              VEGPredicate* vegPred,
              NAList<EqualitySet*>& equalitySets);

  /**
   * Adds the operands of an equality predicate to an equality set. If the
   * operands are members of two different equality sets, the sets are combined.
   * If only one is a member, the other is added. If neither is a member, a new
   * equality set is created and they are added to it.
   *
   * @param pred The equality predicate (item expression tree rooted by an
   *             equals operator.
   * @param [out] equalitySets The list of equality sets, in case a new one needs
   *                           to be created and added to it.
   */
  void addEqPredToEqualitySets(
              ItemExpr* pred,
              NAList<EqualitySet*>& equalitySets);

  /**
   * Create equality sets from the VEGPredicates and other equality conditions
   * in the query. Although a VEG is conceptually the same thing as the equality
   * set we use, they do not correspond exactly, and our processing here may
   * combine two or more VEGs into a single equality set, as well as adding
   * expressions that are equivalent to the members of a VEG.
   *
   * @param [out] preds List of predicates, from which the VEGPredicates and
   *                    equality conditions will be removed and analyzed to
   *                    produce the equality sets.
   * @param [out] equalitySets The list of equality sets produced by the function.
   */
  void formEqualitySets(ValueIdSet& preds,
                        NAList<EqualitySet*>& equalitySets);

  /**
   * Obtains the set of predicates from the query that reference children of
   * #jbbElem, and creates join, range, and residual predicates from them for
   * the descriptor.
   *
   * @param nodeSet Set of CA nodes corresponding the the JBBCs of \c jbbElem.
   * @param gbNode Group By node for the JBB (null if there isn't one), needed to
   *               find predicates on GB node that aren't pushed down (count(*)).
   * @param jbbElem The JBB being processed.
   */
  void processReferencingPreds(CANodeIdSet* nodeSet,
                               RelExpr* gbNode,
                               QRJBBPtr jbbElem);


  /**
   * Deletes the equality sets in the passed list, and clears the hash tables
   * that reference EqualitySet as a value. The dynamically allocated keys and
   * values are also deleted.
   *
   * @param equalitySets List of equality sets that have served their purpose.
   */
  void discardEqualitySets(NAList<EqualitySet*>& equalitySets);

  /**
   * Traverses the query tree looking for table scan nodes that are not part of
   * a join, so that a JBB element can be created for them. The Analyzer does
   * not currently treat these isolated nodes as JBBs, so we have to find them.
   *
   * @param desc The query descriptor we are creating.
   * @param expr RelExpr node that is the root of the query tree.
   */
  void getSingleTableJBBs(QRDescriptorPtr desc, RelExpr* expr);

  /**
   * Gets information about a column, including the node ID of the table that
   * owns it.
   *
   * @param vid ValueId of a veg reference or base column.
   * @param [out] nodeID Id of the owning node, or \c NULL_CA_ID if #vid is not
   *                     the id of a veg ref or base column.
   * @param [out] cvid ValueId of the column. This will be the same as #vid if
   *                   vid is for a base column.
   * @param [out] vegrefVid ValueId of the vegref the column belongs to, or
   *                        \c NULL_VALUE_ID if none. This will be the same as
   *                        #vid if vid is for a vegref.
   * @param [out] baseColName Name of the base column.
   * @param [out] isExtraHub \c TRUE if the node is an extra-hub node.
   * @param [out] colIndex The ordinal number of the column in the base table.
   * @return \c TRUE if a node id was determined, \c FALSE otherwise.
   */
  NABoolean getTableId(ValueId    vid,
                       CANodeId&  nodeID,
                       ValueId&   cvid,
                       ValueId&   vegrefVid,
                       NAString&  baseColName,
                       NABoolean& isExtraHub,
                       Int32&       colIndex);
  
  /**
   * Returns the node id for the table that owns the referenced column.
   *
   * @param vid ValueId of a veg reference or base column.
   * @return The node id of the owning table, or \c NULL_CA_ID if #vid is not
   *         the id of a veg ref or base column.
   */
  CANodeId getNodeId(ValueId vid);

  /**
   * Creates an oject representing a column element in the descriptor.
   *
   * @param vid ValueId of the veg ref or base column.
   * @param joinPredId If nonzero, the column element should be a reference to
   *                   the existing JoinPred element with this id.
   * @param markAsUsed If TRUE, add the column's value id to the list of those
   *                   used in the descriptor, so subsequent uses will cause a
   *                   reference to the original to be used.
   * @return Pointer to the create column element.
   */
  QRColumnPtr genQRColumn(ValueId vid,
                          UInt32 joinPredId = 0, 
                          NABoolean markAsUsed = TRUE);

  /**
   * Creates a hierarchy of descriptor objects corresponding to the passed
   * item expression.
   *
   * @param itemExpr Root of the item expression tree to create descriptor
   *                 objects for.
   * @return Root of the hierarchy of created descriptor objects.
   */
  QRExplicitExprPtr getExprTree(ItemExpr* itemExpr);
  
  /**
   * Creates a descriptor object corresponding to the passed item expression.
   * This method is used for both output expressions and residual predicates.
   *
   * @param pExpr The item expression from which to derive the descriptor object.
   * @param isResidual Is this a residual predicate or an output expression?
   * @param joinPredId If nonzero, the expression element will be a reference to
   *                   the existing JoinPred element with this id.
   * @return The created expression descriptor object.
   */
  QRExprPtr genQRExpr(ItemExpr* pExpr,
                      NABoolean isResidual,
                      UInt32 joinPredId = 0);

  /**
   * Is this expression an InstantiateNull function?
   *
   * @param itemExpr The InstantiateNull expression
   * @return TRUE is the passed itemExpr is an InstantiateNull function.
   */ 
  NABoolean isInstNull(ItemExpr* itemExpr);
  	
  /**
   * Skip the VEGReference and/or InstantiateNull function to get to the column itself.
   *
   * @param itemExpr The InstantiateNull expression
   * @return A QRColumn element for the column pointed to by the InstantiateNull
   *         function.
   */
  QRColumnPtr skipInstNull(ItemExpr* itemExpr);
  	
  /**
   * Replaces all occurrences of a given column name in a string with a numbered
   * placeholder.
   *
   * @param pItemText The expression text to do the replacement in.
   * @param colvid ValueId of the column to replace.
   * @param dColIndex Numeric suffix to append to the base marker text for each
   *                  occurrence of the column in the text.
   * @return \c TRUE if at least one instance of the column was replaced,
   *         otherwise \c FALSE.
   */
  NABoolean normalizeColumnInExpression(NAString& pItemText,
					ValueId   colvid,
					short     dColIndex);

  /**
   * Creates a JBB descriptor object from an Analyzer JBB object, and fills in
   * the descriptor object with information derived from the Analyzer object.
   *
   * @param jbb The Analyzer JBB.
   * @return The descriptor version of the JBB.
   */
  QRJBBPtr createJbb(JBB* jbb);

  /**
   * Sets bits in the residual predicate bitmap for any column that is part of
   * a VEG in the list of veg refs.
   *
   * @param vegrefsInExpr List of veg refs to go through.
   */
  void markColumnsAsResidual(ValueIdSet& vegrefsInExpr);

  /**
   * Creates a residual predicate descriptor object for the predicate with the
   * given ValueId.
   *
   * @param predVid ValueId of the predicate.
   * @param jbbElement JBB element to which the residual predicate belongs.
   */
  void processResidualPredicate(ValueId predVid, QRJBBPtr jbbElement);

  /**
   * Creates a group by descriptor object corresponding to the passed
   * \c GBAnalysis object.
   *
   * @param gbAnalysis The Analyzer's analysis object for the group by.
   * @param jbbElement The JBB descriptor object the group by is for.
   */
  void processGroupBy(RelExpr*    groupByNode,
                      CANodeId    gbID,
		      QRJBBPtr    jbbElement);

  /**
   * Creates the output list for a descriptor from the characteristic outputs
   * of the top node of an Analyzer JBB.
   *
   * @param normOutput Characteristic outputs of the JBB
   * @param jbbElement Descriptor object for the JBB this is the output of.
   */
  void processOutputList(const ValueIdSet& normOutput,
			 QRJBBPtr jbbElement);

  /**
   * Creates the table descriptor objects and possibly group by descriptor
   * object that are the contents of a JBB in the descriptor.
   *
   * @param jbbcNodeIds Node ids for the JBBCs of the JBB.
   * @param jbbElement The JBB descriptor object.
   * @param jbbOutputs The ValueIdSet of the JBB outputs.
   */
  void processJBBCList(CANodeIdSet* jbbcNodeIds,
                       QRJBBPtr jbbElement,
                       ValueIdSet &jbbOutputs,
                       CANodeId& groupJbbcNodeId);

  /**
   * Process the primary key of the table, and add it into the table element.
   * @param tableAnalysis 
   * @param tableElement 
   * @param jbbOutputs
   */
  void processKeys(TableAnalysis* tableAnalysis, 
                   QRTablePtr tableElement,
                   ValueIdSet &jbbOutputs);

  /**
   * Creates a version descriptor object. Each of the top-level descriptor
   * objects (query, MV, result) has a version associated with it.
   *
   * @return The version object.
   */
  QRVersionPtr createVersionElement();

  /**
   * Adds stored range predicates to their correct lists. To this point they
   * have been stored in a hash table so they can be found and updated if
   * another predicate causes the range to be modified, as in <code>a between
   * 10 and 20 and a <> 15</code>, which is represented by two separate
   * predicates.
   */
  void addRangePredicates();

  QRElementPtr getElementForValueID(char firstChar, UInt32 id) const;
	
	NABoolean isMvMode()
	{
		return descriptorType_ == ET_MVDescriptor;
	}
	
	NABoolean isQueryMode()
	{
		return descriptorType_ == ET_QueryDescriptor;
	}
	
	NABoolean isRangeSupported(QRRangePredPtr rangePred);

private:
  /** Starting value for generated IDs for pseudo-JBBs. */
  static const UInt32 GENERATED_JBBID_START;

  // To keep track of column IDs - the set gets populated 
  // and utilized when bGenColumnRefs_ is TRUE
  ValueIdSet	mColumnsUsed_;
  ValueIdSet	mExtraHubColumnsUsed_;

  CollHeap*	mvqrHeap_;

  NAString	mMVTableName_;

  NABoolean     bSortJoinPredicateCols_;
  NABoolean     bGenColumnRefs_;
  NABoolean	bFormatted_;
  UInt32  generatedJBBid_;
  NABoolean     putAllJBBsInQD_;    // read from CQD MVQR_ALL_JBBS_IN_QD
  ElementType   descriptorType_;    // Query or MV descriptor being generated?
  RelExpr*      relExpr_;           // root node of query
  JBB*          currentJBB_;        // currently generating desc for this JBB

  /** Hash table mapping ValueIds of columns to their associated range info. */
  NAHashDictionary<QRValueId, RangeInfo> rangeColHash_;

  /** Hash table mapping text of expressions to their associated range info. */
  NAHashDictionary<const NAString, RangeInfo> rangeExprHash_;

  /** Hash table mapping ValueIds of columns to their associated equality set. */
  NAHashDictionary<QRValueId, EqualitySet> vegsUsedHash_;

  /** Hash table mapping text of expressions to their associated equality set. */
  NAHashDictionary<const NAString, EqualitySet> exprsUsedHash_;

  /** Hash table mapping ids to their QRColumn or QRTable object. */
  QRElementHash colTblIdHash_;

  /** Map column value ids to the join pred they are in. In MV and query
   *  descriptors, a ref to a column uses a 'J' element id with the join pred's
   *  value id number. Col ids are the bottom values in the map.
   */
  ValueIdMap colToJoinPredMap_;

  NAList<EqualitySet*> allEqualitySets_;
  
  NABoolean isDumpMvMode_;
  
  Lng32 maxExprSize_;
  Lng32 maxExprDepth_;
}; // QRDescGenerator

#endif /* QUERYDESCRIPTOR_H */
