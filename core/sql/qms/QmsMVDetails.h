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
// File:         MVDetails.h
// Description:  
//               
//               
//               
//
// Created:      07/17/08
// ***********************************************************************

// _MEMSHAREDPTR;_MEMCHECK

#ifndef _MVDETAILS_H_
#define _MVDETAILS_H_

#include "QRSharedPtr.h"
#include "QRDescriptor.h"

// Forward declarations
class MapIDsVisitor;
class RefFinderVisitor;
class RangeInformation;
class BaseTableDetails;
class JBBDetails;
class DescriptorDetails;
class MVDetails;

// External class declarations
class MVCandidate;

#ifdef _MEMSHAREDPTR
typedef QRIntrusiveSharedPtr<MapIDsVisitor>		  MapIDsVisitorPtr;
typedef QRIntrusiveSharedPtr<RefFinderVisitor>		  RefFinderVisitorPtr;
typedef QRIntrusiveSharedPtr<BaseTableDetails>		  BaseTableDetailsPtr;
typedef QRIntrusiveSharedPtr<JBBDetails>		  JBBDetailsPtr;
typedef QRIntrusiveSharedPtr<DescriptorDetails>		  DescriptorDetailsPtr;
typedef QRIntrusiveSharedPtr<MVDetails>			  MVDetailsPtr;
#else
typedef MapIDsVisitor*					  MapIDsVisitorPtr;
typedef RefFinderVisitor*				  RefFinderVisitorPtr;
typedef BaseTableDetails*				  BaseTableDetailsPtr;
typedef JBBDetails*					  JBBDetailsPtr;
typedef DescriptorDetails*				  DescriptorDetailsPtr;
typedef MVDetails*					  MVDetailsPtr;
#endif

typedef NAPtrList<JBBDetailsPtr>				JBBDetailsPtrList;
typedef NAPtrList<MVDetailsPtr>				        MVDetailsPtrList;
typedef SharedPtrValueHash<const NAString, BaseTableDetails>	BaseTableDetailsHash;
typedef NAHashDictionary<const NAString, ResidualPredPtrList>	ResidualPredicatesHash;
typedef NAHashDictionary<const NAString, RangePredPtrList>	RangePredicatesHash;

#include "QmsJoinGraph.h"
#include "QmsMVMemo.h"


/**
 * The MapIDsVisitor class implements the mapIDs operation.
 * For every descriptor element from which its called, it will
 * map the element ID to the object pointer, in the idHash
 * hash table. 
 *****************************************************************************
 */
class MapIDsVisitor : public Visitor
{
public:
  MapIDsVisitor(QRElementHash& idHash, 
		ADD_MEMCHECK_ARGS_DECL(CollHeap* heap))
    : Visitor(ADD_MEMCHECK_ARGS_PASS(heap))
     ,idHash_(idHash)
  {}

  /**
   * An implementation of the Visitor design pattern visit() method.
   * @param caller 
   */
  virtual Visitor::VisitResult visit(QRElementPtr caller)
  {
    const NAString* id = &(caller->getID());
    if (id->length() > 0)
      idHash_.insert(id, caller);

    return VR_Continue;
  }

private:
  QRElementHash& idHash_;
}; //  class MapIDsVisitor

/**
 * 
 * 
 * 
 * 
 * 
 *****************************************************************************
 */
class RefFinderVisitor : public Visitor
{
public:
  RefFinderVisitor(DescriptorDetailsPtr descDetails,
		   ADD_MEMCHECK_ARGS_DECL(CollHeap* heap))
    : Visitor(ADD_MEMCHECK_ARGS_PASS(heap))
     ,descDetails_(descDetails)
  {}

  virtual Visitor::VisitResult visit(QRElementPtr caller);

private:
  DescriptorDetailsPtr descDetails_;
}; // class RefFinderVisitor

/**
 * Contains matching data structures for a specific MV base table,
 * such as range predicates and residual predicates.
 *****************************************************************************
 */
class BaseTableDetails : public NAIntrusiveSharedPtrObject
{
public:
  BaseTableDetails(const QRTablePtr table,
		   const NABoolean  isHub,
		   ADD_MEMCHECK_ARGS_DECL(CollHeap* heap))
    : NAIntrusiveSharedPtrObject(ADD_MEMCHECK_ARGS_PASS(heap))
     ,tableElement_(table)
     ,isHub_(isHub)
     ,id_(table->getID())
     ,name_(table->getTableName())
     ,rangeColumnPredicates_(heap, table->getNumCols())
  {
    for (CollIndex i=0; i<(CollIndex)table->getNumCols(); i++)
      rangeColumnPredicates_.insertAt(i,NULL);
  }

  virtual ~BaseTableDetails()
  { }

  void addRangePredicateOnColumn(const QRRangePredPtr rangePred, 
				 const QRColumnPtr rangeCol);

  const NAString& getID() const
  {
    return id_;
  }

  const NAString& getName() const
  {
    return name_;
  }

  const QRTablePtr getTableElement() const
  {
    return tableElement_;
  }

  NABoolean isHub() const 
  {
    return isHub_;
  }

  NABoolean operator==(const BaseTableDetails& other)
  {
    return name_ == other.name_;
  }

  const XMLBitmap& getRangeBits() const 
  {
    return tableElement_->getRangeBits();
  }

  const XMLBitmap& getResidualBits() const 
  {
    return tableElement_->getResidualBits();
  }

  const QRRangePredPtr getRangeColumnPredicatesFor(CollIndex pos)
  {
    return rangeColumnPredicates_[pos];
  }

private:
  const NAString&  id_;
  const NAString&  name_;
  const NABoolean  isHub_;
  const QRTablePtr tableElement_;

  NAPtrArray<QRRangePredPtr>	    rangeColumnPredicates_;
}; // class BaseTableDetails

/**
 * Contains the matching data structures for a single JBB in a descriptor.
 *****************************************************************************
 */
class JBBDetails : public NAIntrusiveSharedPtrObject
{
public:
  /**
   * JBBDetails constructor
   * @param jbbDesc The JBB descriptor from the MV or query descriptor.
   * @param isAnMV Is this JBB from an MV or from a query?
   * @param heap The heap from which to allocate memory.
   */
  JBBDetails(const QRJBBPtr        jbbDesc,
             DescriptorDetailsPtr  descDetails,
	     NABoolean	           isAnMV, 
	     ADD_MEMCHECK_ARGS_DECL(CollHeap* heap))
    : NAIntrusiveSharedPtrObject(ADD_MEMCHECK_ARGS_PASS(heap))
     ,jbbDesc_(jbbDesc)
     ,hasGroupBy_(FALSE)
     ,hasEmptyGroupBy_(FALSE)
     ,isAnMV_(isAnMV)
     ,baseTablesByID_(hashKey, INIT_HASH_SIZE_SMALL, TRUE, heap)
     ,rangePredicates_(hashKey, INIT_HASH_SIZE_SMALL, TRUE, heap)
     ,residualPredicates_(hashKey, INIT_HASH_SIZE_SMALL, TRUE, heap)
     ,hasNoRangePredicates_(FALSE)
     ,hasNoResidualPredicates_(FALSE)
     ,hasHavingPredicates_(FALSE)
     ,hasLOJs_(FALSE)
     ,constColumns_(heap)
     ,descDetails_(descDetails)
  {}

  /**
   * Initialize internal data structures.
   * @param heap The heap from which to allocate memory.
   */
  void init(CollHeap* heap);
  void initBaseTables(CollHeap* heap);
  void initRangePreds(CollHeap* heap);
  void initResidualPreds(CollHeap* heap);

  const QRJBBPtr getJbbDescriptor() const
  {
    return jbbDesc_;
  }

  /**
   * Does this JBB include a GroupBy?
   * @return TRUE if this JBB includes a GroupBy.
   */
  NABoolean hasGroupBy() const
  {
    return hasGroupBy_;
  }

  /**
   * 
   * @param id 
   * @return 
   */
  BaseTableDetailsPtr getBaseTableByID(const NAString& id) const
  {
    return baseTablesByID_.getFirstValue(&id);
  }

  RangePredPtrList* getRangePredsOnExpression(const NAString& text) const
  {
    return rangePredicates_.getFirstValue(&text);
  }

  ResidualPredPtrList* getResidualPreds(const NAString& text) const
  {
    return residualPredicates_.getFirstValue(&text);
  }

  NABoolean hasNoRangePredicates()
  {
    return hasNoRangePredicates_;
  }

  NABoolean hasNoResidualPredicates()
  {
    return hasNoResidualPredicates_;
  }

  NABoolean hasHavingPredicates()
  {
    return hasHavingPredicates_;
  }

  NABoolean hasLOJs()
  {
    return hasLOJs_;
  }

  NABoolean isConstColumn(const NAString* id)
  {
    return constColumns_.contains(id);
  }

private:
  const QRJBBPtr	  jbbDesc_;
  NABoolean		  hasGroupBy_;
  NABoolean               hasEmptyGroupBy_;
  NABoolean		  isAnMV_;
  NABoolean		  hasNoRangePredicates_;
  NABoolean		  hasNoResidualPredicates_;
  NABoolean               hasLOJs_;
  DescriptorDetailsPtr    descDetails_;

  // Base table hash - key is the table ID.
  // Used during initialization for inserting predicates.
  BaseTableDetailsHash	  baseTablesByID_;

  // A hash table of range predicates on expressions.
  // The hash key is the expression text.
  RangePredicatesHash	  rangePredicates_;

  // A hash table of residual predicates.
  // The hash key is the expression text.
  ResidualPredicatesHash  residualPredicates_;

  NABoolean               hasHavingPredicates_;

  // Set of IDs of QRColumns that are equal to constant values
  // (Have range preds that are equal to a single constant value)
  StringPtrSet            constColumns_;
};  // class JBBDetails

/**
 * Contains the matching data structures for a descriptor.
 * This class is used directly for query descriptors, and is subclassed by
 * MVDetails for MV descriptors.
 *****************************************************************************
 */
class DescriptorDetails : public NAIntrusiveSharedPtrObject
{
public:
  /**
   * DescriptorDetails constructor
   * @param descriptor The (MV or query) descriptor.
   * @param isAnMV Is this an MV descriptor or a query descriptor?
   * @param heap The heap from which to allocate memory.
   */
  DescriptorDetails(QRDescriptorPtr descriptor, 
		    NABoolean	    isAnMV,
		    ADD_MEMCHECK_ARGS_DECL(CollHeap* heap))
    : NAIntrusiveSharedPtrObject(ADD_MEMCHECK_ARGS_PASS(heap))
     ,descriptor_(descriptor)
     ,isAnMV_(isAnMV)
     ,jbbDetailsList_(heap)
     ,idHash_(hashKey, INIT_HASH_SIZE_LARGE, TRUE, heap) // Pass NAString::hashKey
     ,joinPredHash_(hashKey, INIT_HASH_SIZE_LARGE, TRUE, heap) // Pass NAString::hashKey
  {
  }

  virtual ~DescriptorDetails();

  /**
   * Initialize internal data structures.
   * @param heap The heap from which to allocate memory.
   */
  virtual void init(CollHeap* heap);

  void initJoinPreds();

  /**
   * Gget the descriptor
   * @return a pointer to the descriptor object
   */
  QRDescriptorPtr getDescriptor() { return descriptor_; }

  /**
   * Get the list of JBBDetails objects.
   * @return 
   */
  JBBDetailsPtrList& getJbbDetailsList() { return jbbDetailsList_; }

  /**
   * Get the descriptor element whose ID is \c id
   * @param id The ID of the element to find.
   * @return The pointer to the element object, or NULL if not found.
   */
  const QRElementPtr getElementForID(const NAString& id)
  {
    return idHash_.getFirstValue(&id);
  }

  /**
   * Find the table name for the input column element.
   * @param col The column element 
   * @return 
   */
  const NAString& getTableNameFromColumn(QRColumnPtr col);

  NABoolean isFromJoin(const QRColumnPtr col)
  {
    const NAString& id = col->getID();
    QRElementPtr jp = joinPredHash_.getFirstValue(&id);
    return (jp != NULL);
  }

  const QRJoinPredPtr getJoinPred(const QRColumnPtr col)
  {
    const NAString& id = col->getID();
    QRElementPtr jp = joinPredHash_.getFirstValue(&id);
    if (jp == NULL)
      return NULL;
    else
      return jp->downCastToQRJoinPred();
  }

  /**
   * Is this column from a table that has an LOJ parent?
   * @param col The column in question.
   * @return 
   */
  NABoolean isColumnFromLojTable(const QRColumnPtr col);

protected:

  void addJBBDetails(JBBDetailsPtr jbb)
  {
    jbbDetailsList_.insert(jbb);
  }

  void initJoinPredList(QRJoinPredListPtr jpList);

protected:
  QRDescriptorPtr     descriptor_;
  NABoolean	      isAnMV_;
  JBBDetailsPtrList   jbbDetailsList_;       // When we add support for multi-JBB MVs...
  QRElementHash	      idHash_;               // Maps element IDs to QRElement objects
  QRElementHash	      joinPredHash_;         // Maps columns to JoinPreds they are part of.
};  // class DescriptorDetails

/**
 * Contains all the details about an MV that are needed after the MV has 
 * already been inserted into the matching data structures. These include:
 * - The MV name.
 * - the MV query join graph and other details for rewriting incoming 
 *   queries that use it explicitly.
 * Internal data members of MVDetails must be local copies and not 
 * pointers into data in the MV descriptor, because those should be 
 * allocated from a different heap.
 * \todo Add support for join graph rewrite data.
 *****************************************************************************
 */
class MVDetails : public DescriptorDetails
{
public:
  /**
   * MVDetails constructor.
   * @param descriptor The MV descriptor
   * @param heap The heap from which to allocate memory.
   */
  MVDetails(QRMVDescriptorPtr descriptor, 
            ADD_MEMCHECK_ARGS_DECL(CollHeap* heap))
    : DescriptorDetails(descriptor, TRUE, ADD_MEMCHECK_ARGS_PASS(heap))
     ,name_("", heap)
     ,redefTimestamp_(0)
     ,redefTimestampString_("", heap)
     ,refreshTimestamp_(0)
     ,refreshTimestampString_("", heap)
     ,hasIgnoreChanges_(FALSE)
     ,isConsistent_(FALSE)
     ,isUMV_(FALSE)
     ,jbbDetails_(NULL)
     ,joinGraph_(NULL)
     ,tableByNameHash_(hashKey, INIT_HASH_SIZE_SMALL, FALSE, heap)	// Key is not unique with self joins.
     ,outputByIDHash_(hashKey, INIT_HASH_SIZE_SMALL, TRUE, heap)	// Unique IDs
     ,outputByColumnName_(hashKey, INIT_HASH_SIZE_SMALL, FALSE, heap)	// Key is not unique
     ,outputByExprText_(hashKey, INIT_HASH_SIZE_SMALL, FALSE, heap)	// Key is not unique
     ,outputByInputColumns_(hashKey, INIT_HASH_SIZE_SMALL, FALSE, heap)	// Key is not unique
     ,mvMemoGroups_(heap)
     ,countStar_(NULL)
  {
    QRTablePtr mvName = descriptor->getTable();
    name_ = mvName->getTableName();
    setRedefTimestamp(mvName->getTimestamp());
    isIncremental_ = descriptor->getMisc()->isIncremental();
    isImmediate_ = descriptor->getMisc()->isImmediate();
    isUMV_ = descriptor->getMisc()->isUMV();
  }

  virtual ~MVDetails();

  // Accessors
  // ===========

  /**
   * Get the MV descriptor
   * @return The MV descriptor
   */
  const QRMVDescriptorPtr getMvDescriptor()
  {
    return static_cast<const QRMVDescriptorPtr>(getDescriptor());
  }

  /**
  * Get the name of the MV.
  * @return the name of the MV.
  */
  const NAString& getMVName() { return name_; }

  /**
   * Get the MV redefinition timestamp as an Int64
   * @return 
   */
  const Int64& getRedefTimestamp() { return redefTimestamp_; }

  /**
   * Get the MV redefinition timestamp as a string
   * @return 
   */
  const NAString& getRedefTimestampAsString() { return redefTimestampString_; }

  /**
   * Get the MV refresh timestamp as an Int64
   * @return 
   */
  const Int64& getRefreshTimestamp() { return refreshTimestamp_; }

  /**
   * Get the MV refresh timestamp as a string
   * @return 
   */
  const NAString& getRefreshTimestampAsString() { return refreshTimestampString_; }

  /**
   * Does the MV have an IGNORE CHANGES clause?
   * @return 
   */
  NABoolean hasIgnoreChanges() { return hasIgnoreChanges_; }

  /**
   * Is the MV incremental?
   * @return 
   */
  NABoolean isIncremental() { return isIncremental_; }

  /**
   * Is the MV an ON STATEMENT MV?
   * @return 
   */
  NABoolean isImmediate() { return isImmediate_; }

  /**
   * Returns FALSE is the MV has an IGNORE CHANGES clause, and has not been
   * recently recomputed.
   * @return 
   */
  NABoolean isConsistent() { return isConsistent_; }

  /**
   * Returns TRUE if this is a user maintaind MV
   * @return 
   */
  NABoolean isUMV() { return isUMV_; }

  /**
   * Has the MV been initialized?
   * @return NABoolean TRUE or FALSE
   */
  NABoolean isInitialized()
  { 
    if (refreshTimestamp_ > 0)
      return TRUE;
    else
      return FALSE;
  }

  /**
   * Get the JBBDetails of the top (and only) JBB.
   * @return 
   */
  JBBDetailsPtr getJbbDetails() const { return jbbDetails_; }


  // Mutators
  // ==========

  /**
   * Rename the MV
   * @param newName The new name of the MV.
   */
  void rename(const NAString& oldName, const NAString& newName);

  /**
   * Set the redefinition timestamp of the MV
   * @param timestamp 
   */
  void setRedefTimestamp(const NAString& timestamp)
  {
    if (timestamp == "")
    {
      redefTimestampString_ = "0";
      redefTimestamp_ = 0;
    }
    else
    {
      redefTimestampString_ = timestamp;
      redefTimestamp_ = atoInt64(timestamp);
    }
  }

  /**
   * Set the refresh timestamp of the MV
   * @param timestamp 
   */
  void setRefreshTimestamp(const NAString& timestamp)
  {
    if (timestamp == "")
    {
      refreshTimestampString_ = "0";
      refreshTimestamp_ = 0;
    }
    else
    {
      refreshTimestampString_ = timestamp;
      refreshTimestamp_ = atoInt64(timestamp.data());
    }
  }

  /**
   * Set the IGNORE CHANGES state.
   * @param ic 
   */
  void setIgnoreChanges(NABoolean ic)
  {
    hasIgnoreChanges_ = ic;
  }

  /**
   * Set the IGNORE CHANGES state.
   * @param ic 
   */
  void setConsistent(NABoolean ic)
  {
    isConsistent_ = ic;
  }

  /**
   * Set this MV is user maintained.
   * @param ic 
   */
  void setUMV(NABoolean umv)
  {
    isUMV_ = umv;
  }

  /**
   * Equality operator. Uses only the MV name.
   * @param other 
   * @return 
   */
  NABoolean operator==(const MVDetails& other)
  {
    return !name_.compareTo(other.name_); 
  }

  /**
   * Does this MV have a groupBy in it (or rather, in its top JBB)?
   * @return 
   */
  NABoolean hasGroupBy() 
  {
    return jbbDetails_->hasGroupBy();
  }

  /**
   * Get the MV output column that is COUNT(*).
   * @return 
   */
  QROutputPtr getCountStar()
  {
    return countStar_;
  }

  /**
   * Set the join graph of this MV
   * @param joinGraph 
   */
  void setJoinGraph(QRJoinGraphPtr joinGraph)
  {
     joinGraph_ = joinGraph;
  }

  /**
   * Initialize internal data structures.
   * @param heap 
   */
  virtual void init(CollHeap* heap);

  /**
   * Get the output list element, that its output item (column or expression)
   * has the ID \c id (or NULL if not found).
   * @param id The ID of the column or expression, 
   * @return The pointer of the output element object.
   */
  const QROutputPtr getOutputForID(const NAString& id);

  /**
   * Get the table element (from the JBBC list) with this name (or NULL if not found).
   * @param name The name of the table element to find.
   * @return The QRTable element pointer.
   */
  const QRTablePtr  getTableFromName(const NAString& name);

  /**
   * Get the output list element, that its internal column name is \c name (or 
   * NULL if not found). See calcIDQualifiedColumnName().
   * @param id The ID of the table from the MV descriptor.
   * @param name The column name to find.
   * @return The pointer of the output element object.
   */
  const QROutputPtr getOutputByColumnName(const NAString& id, const NAString& name, CollHeap* heap);

  /**
   * Get the output list element, that represents the output expression that 
   * matches \c text (or NULL if not found).
   * @param text the normalized text of the expression to find.
   * @return The pointer of the output element object.
   */
  const QROutputPtr getOutputByExprText(const NAString& text);

  /**
   * Get access to the actual hash table in order to create an iterator on it.
   * @return The hash table of output elements by the expression text.
   */
  const QRElementHash& getOutputByExprTextHash()
  {
    return outputByExprText_;
  }

  /**
   * Get the output list element, that represents an expression, that has 
   * this column name as one of its input columns (or NULL if not found).
   * See calcIDQualifiedColumnName().
   * @param id The ID of the table from the MV descriptor.
   * @param colName The column name of the input to find.
   * @return The pointer of the output element object.
   */
  const QROutputPtr getOutputByInputColumns(const NAString& id, const NAString& name, CollHeap* heap);

  /**
   * Compute an "ID Qualified" column name.
   * The result looks like this: <tableID>.<column name>
   * This gets around the problem that with self-joins, different instances
   * of the same column from different instances of the self-join table, have
   * identical fully qualified column names.
   * @param id The ID of the table from the MV descriptor.
   * @param name The (not fully qualified) column name.
   * @param heap 
   * @return 
   */
  const NAString* calcIDQualifiedColumnName(const NAString& id, const NAString& name, CollHeap* heap)
  {
    NAString* result = new(heap) NAString(id, heap);
    *result += ".";
    *result += name;
    return result;
  }

  /**
   * Add a new group to the list of MVMemoGroups pointing to this object.
   * Used during cleanup to remove pointers to this object before deleting it.
   * @param group 
   * @return 
   */
  void addMVMemoGroup(MVMemoGroupPtr group)
  {
    mvMemoGroups_.insert(group);
  }

  /**
   * Remove pointers to this object from all MVMemo nodes pointing to it.
   * This method must be called before deleting this object.
   */
  void disengage();

protected:
  // Methods called by init().
  void initOutputs(const QRJBBPtr jbb, CollHeap* heap);
  void initTables(const QRJBBPtr jbb, CollHeap* heap);
  void addEqualitySet(QRJoinPredPtr joinPredElement, NABoolean isFromExpr, QROutputPtr output, CollHeap* heap);

private:
  // Copy construction/assignment not defined.
  MVDetails(const MVDetails&);
  MVDetails& operator=(const MVDetails&);

private:
  NAString	      name_;
  Int64		      redefTimestamp_;
  NAString	      redefTimestampString_;
  Int64		      refreshTimestamp_;
  NAString	      refreshTimestampString_;
  NABoolean	      hasIgnoreChanges_;
  NABoolean	      isIncremental_;
  NABoolean	      isImmediate_;
  NABoolean	      isConsistent_;
  NABoolean           isUMV_;

  JBBDetailsPtr       jbbDetails_;
  QRJoinGraphPtr      joinGraph_;  // This pointer is held here so it can be deleted in the Dtor.
  QRElementHash	      tableByNameHash_;
  QRElementHash	      outputByIDHash_;      // Maps IDs of outputs to QROutput objects.
  QRElementHash	      outputByColumnName_;  // Uses the ID qualified column names.
  QRElementHash	      outputByExprText_;
  QRElementHash	      outputByInputColumns_;
  MVMemoGroupList     mvMemoGroups_;        // List of MVMemoGroups pointing to me.
  QROutputPtr         countStar_;
};  // class MVDetails



#endif 
