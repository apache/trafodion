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
// File:         QRJoinGraph.cpp
// Description:  
//               
//               
//               
//
// Created:      12/11/07
// ***********************************************************************

#include "ComSizeDefs.h"
#include "QmsJoinGraph.h"
#include "QRLogger.h"

// NAString comparison for qsort
//
// Return Value
// -1 elem1 less than    elem2
//  0 elem1 equivalent   elem2
//  1 elem1 greater than elem2
//-----------------------------------------------------------------------------------
static Int32 naStringCompare(const void *elem1, const void *elem2)
{
  const NAString* s1 =  *(NAStringPtr*)elem1;
  const NAString* s2 =  *(NAStringPtr*)elem2;
  return s1->compareTo(*s2);
}

//========================================================================
//  Class JoinGraphTable
//========================================================================

/**
 * This method checks if this table is directly connected to the sub-graph.
 * It checks if there is ANY direct connection to the subgraph.
 *****************************************************************************
 */
NABoolean JoinGraphTable::isConnectedTo(const QRJoinSubGraphPtr subGraph) const
{
  CollIndex maxEntries = predList_.entries();
  for (CollIndex i=0; i<maxEntries; i++)
  {
    JoinGraphEqualitySetPtr eqSet = predList_[i];
    if (eqSet->isConnectedTo(subGraph))
      return TRUE;
  }

  return FALSE;
}

//*****************************************************************************
// Find an equality set from the ones this table is connected to, that match
// otherHalfPred, which is from a different join graph.
//*****************************************************************************
JoinGraphEqualitySetPtr JoinGraphTable::getEqSetUsing(JoinGraphHalfPredicatePtr otherHalfPred) 
{
  // For each equality set
  CollIndex maxEntries = predList_.entries();
  for (CollIndex i=0; i<maxEntries; i++)
  {
    // Find the halfPred back to me
    JoinGraphEqualitySetPtr thisEqSet = predList_[i];
    JoinGraphHalfPredicatePtr thisHalfPred = thisEqSet->findHalfPredTo(this);

    // And match it with the other one.
    if (thisHalfPred->match(otherHalfPred))
      return thisEqSet;
  }

  // No match found.
  return NULL;
}


/**
 * Prepare the input text for the Dotty graphical graph viewer, for this table.
 * @param[out] to Output string.
 *****************************************************************************
 */
void JoinGraphTable::dumpGraph(NAString& to)
{
  char text[256];
  static char blue[] = "lightblue";
  static char red[]  = "coral";
  char* color = isHub_ ? red : blue;

  // Describe this table (using descriptor ID and name)
  const NAString& id = getID();
  sprintf(text, "  %s [shape=box, label=\"%s=%s\",color=%s];\n", 
          id.data(), id.data(), getName().data(), color);
  to += text;
}

//*****************************************************************************
//*****************************************************************************
NABoolean JoinGraphTable::checkPredsOnKey(CollHeap* heap)
{
  QRTablePtr tableElem = getTableElement();

  // If the table has no primary key, skip it.
  const QRKeyPtr theKey = tableElem->getKey();
  if (theKey == NULL)
    return FALSE;

  // We need at least as many half-preds as key columns.
  keyColumns_ = theKey->entries();
  if (keyColumns_ == 0 || (keyColumns_ > predList_.entries()))
    return FALSE;

  EqualitySetList predicatesToCheck(predList_, heap);
  EqualitySetList verifiedPredicates(heap);
  JoinGraphTablePtr sourceTable=NULL;

  // Check all the columns in the key.
  for (CollIndex i=0; i<keyColumns_; i++)
  {
    QRColumnPtr keyCol = theKey->getElement(i)->getReferencedElement()->downCastToQRColumn();
    NABoolean thisColumnFound = FALSE;
  
    // Run the loop on the predicates backwards, because we will be deleting 
    // the current entry every iteration.
    for (Int32 j=predicatesToCheck.entries()-1; j>=0; j--)
    {
      JoinGraphEqualitySetPtr eqSet = predicatesToCheck[j];

      const JoinGraphHalfPredicatePtr halfPred = eqSet->findHalfPredTo(this);
      assertLogAndThrow(CAT_MVMEMO_JOINGRAPH, LL_MVQR_FAIL, 
                        halfPred!=NULL, QRLogicException,
		        "EqualitySet must have a Half-Pred to this table.");

      // Is this half pred on this key column?
      if (keyCol->getID() != halfPred->getID())
      {
        // No, keep looking.
        continue;
      }
      
      // We found a matching equality set - 
      // Remove it from the list, so we don't check it for tother key columns.
      // Add it to the verified list.
      thisColumnFound = TRUE;
      predicatesToCheck.remove(eqSet);
      verifiedPredicates.insert(eqSet);

      // OK, we found a predicate for this key column. 
      // Which other table is it column from?
      JoinGraphTablePtr otherTable = eqSet->getOtherTable(halfPred);
      if (sourceTable == NULL)
        sourceTable = otherTable;
      else
      {
        // All incoming predicates on key columns must be from the same table.
        if (sourceTable != otherTable)
          return FALSE;
      }
    } // for (j) on predicates.

    if (!thisColumnFound)
      return FALSE;
  } // for (i) on key columns.

  // OK, all the key columns have incoming predicates from the same table.
  // Now mark it on all the equality sets.
  for (CollIndex k=0; k<verifiedPredicates.entries(); k++)
  {
    JoinGraphEqualitySetPtr eqSet = verifiedPredicates[k];
    eqSet->setOnKeyTo(this);
  }

  return TRUE;
}

//*****************************************************************************
//*****************************************************************************
NABoolean JoinGraphTable::isOnTheEdge()
{
  return keyColumns_ == predList_.entries() - reducedConnections_;
}

//*****************************************************************************
//*****************************************************************************
void JoinGraphTable::getAndReducePredicateColumnsPointingToMe(ElementPtrList& pointingCols)
{
  CollIndex maxEntries = predList_.entries();
  for (CollIndex i=0; i<maxEntries; i++)
  {
    JoinGraphEqualitySetPtr eqSet = predList_[i];
    JoinGraphHalfPredicatePtr otherHalfPred = eqSet->getForeignKeySide();
    pointingCols.insert(otherHalfPred->getElementPtr());
    otherHalfPred->getTable()->reduceConnection();
  }
}

//*****************************************************************************
//*****************************************************************************
Int32 JoinGraphTable::getDegree()
{
  if (isSelfJoinTable_)
    return 0;

  Int32 degree = 0;
  CollIndex maxEntries = predList_.entries();
  for (CollIndex i=0; i<maxEntries; i++)
  {
    JoinGraphEqualitySetPtr eqSet = predList_[i];
    degree += eqSet->getDegree();
  }

  return degree;
}

//========================================================================
//  Class JoinGraphEqualitySet
//========================================================================

/**
 * Is this equality set part of the current subgraph?
 * @return TRUE if this subgraph has at least two half-predicates
 * that are part of the subgraph, that is, have a tempNumber other
 * than -1.
 *****************************************************************************
 */
NABoolean JoinGraphEqualitySet::isInCurrentSubGraph()
{
  Int32 halfPredsIncluded = 0;

  // Count the number of half-preds in this subgraph for which the temp number
  // is not equal to -1.
  for (CollIndex i=0; i<halfPreds_.entries(); i++)
  {
    if (halfPreds_[i]->getTable()->getTempNumber() != -1)
      halfPredsIncluded++;
  }

  // Are there at least two of them?
  return halfPredsIncluded >= 2;
}

/**
 * Generate the MVMemo hash key line for this equality set:
 * \par
 * 1. Collect the list of half predicates on participating tables.\par
 * 2. Format them as <table-temp-number>.<column-name/expression> \par
 * 3. Sort them. \par
 * 4. Generate a single line in the format: <tt>{1.CUSTOMER_ID,2.ORDERED_BY}</tt> \par
 * The result is returned in an NAString object allocated off the heap.
 * @return the formatted hash key line.
 *****************************************************************************
 */
NAString* JoinGraphEqualitySet::getHashKeyLine(CollHeap* heap)
{
  char buffer[10];
  NAStringPtr* predsArray = new(heap) NAStringPtr[halfPreds_.entries()]; 
  Int32 pos=0;

  // Collect and format half preds.
  for (CollIndex i=0; i<halfPreds_.entries(); i++)
  {
    JoinGraphHalfPredicatePtr halfPred = halfPreds_[i];
    if (halfPred->getTable()->getTempNumber() != -1)
    {
      sprintf(buffer, "%d.", halfPred->getTable()->getTempNumber());
      NAString* halfPredString = new(heap) NAString(buffer, heap);
      *halfPredString += halfPred->getExpr();

      predsArray[pos++] = halfPredString;
    }
  }

  // Sort the half preds.
  qsort(predsArray,
	pos, 
	sizeof(NAStringPtr), 
	naStringCompare);

  // Construct the result text line.
  NAString* result = new(heap) NAString("{", heap);
  for (CollIndex j=0; j<(CollIndex)pos; j++)
  {
    NAStringPtr* st = &(predsArray[j]);
    *result += **st;
    delete *st;  // This temp string not needed anymore.
    *st = NULL;
    if (j != pos-1)
      *result += ",";
  }
  *result += "}";

  // Delete the string array (Not an NABasicObject)
  NADELETEARRAY(predsArray, halfPreds_.entries(), NAStringPtr, heap);
  return result;
} // JoinGraphEqualitySet::getHashKeyLine()

/**
 * This method checks if this equality set is directly connected to the sub-graph.
 *****************************************************************************
 */
NABoolean JoinGraphEqualitySet::isConnectedTo(const QRJoinSubGraphPtr subGraph) const
{
  CollIndex maxEntries = halfPreds_.entries();
  for (CollIndex i=0; i<maxEntries; i++)
  {
    JoinGraphTablePtr table = halfPreds_[i]->getTable();
    if (subGraph->contains(table))
      return TRUE;
  }

  return FALSE;
}

//*****************************************************************************
// Find the halfPred pointing to sourceTable (must be from the same join graph).
//*****************************************************************************
const JoinGraphHalfPredicatePtr JoinGraphEqualitySet::findHalfPredTo(const JoinGraphTablePtr sourceTable) const
{
  CollIndex maxEntries = halfPreds_.entries();
  for (CollIndex i=0; i<maxEntries; i++)
  {
    const JoinGraphHalfPredicatePtr halfPred = halfPreds_[i];
    
    // Pointer comparison is fine here.
    if (halfPred->getTable() == sourceTable)
      return halfPred;
  }

  return NULL;
}

//*****************************************************************************
// Union this and other equality sets by adding all the half preds
// from other into this, and remove them from other.
//*****************************************************************************
void JoinGraphEqualitySet::addHalfPredicatesFrom(JoinGraphEqualitySetPtr other)
{
  CollIndex maxEntries = other->halfPreds_.entries();
  for (CollIndex i=0; i<maxEntries; i++)
  {
    const JoinGraphHalfPredicatePtr halfPred = other->halfPreds_[i];
    addHalfPredicate(halfPred);
    JoinGraphTablePtr table = halfPred->getTable();
    table->removeEqualitySet(other);
    table->addEqualitySet(this);
  }

  other->halfPreds_.clear();
}

//*****************************************************************************
//*****************************************************************************
void JoinGraphEqualitySet::setOnKeyTo(const JoinGraphTablePtr targetTable)
{
  isOnKey_ = TRUE;
  CollIndex maxEntries = halfPreds_.entries();
  for (CollIndex i=0; i<maxEntries; i++)
  {
    const JoinGraphHalfPredicatePtr halfPred = halfPreds_[i];
    if (halfPred->getTable() == targetTable)
    {
      keyDirection_ = i;
      break;
    }
  }
}

//*****************************************************************************
//*****************************************************************************
NABoolean JoinGraphEqualitySet::isOnKeyTo(const JoinGraphTablePtr targetTable)
{
  if (isOnKey_ == FALSE)
    return FALSE;

  CollIndex maxEntries = halfPreds_.entries();
  for (CollIndex i=0; i<maxEntries; i++)
  {
    const JoinGraphHalfPredicatePtr halfPred = halfPreds_[i];
    if (halfPred->getTable() == targetTable)
    {
      return (keyDirection_ == i);
    }
  }

  return FALSE; // Should not get here!
}

//*****************************************************************************
//*****************************************************************************
JoinGraphTablePtr JoinGraphEqualitySet::getOtherTable(JoinGraphHalfPredicatePtr target)
{
  CollIndex maxEntries = halfPreds_.entries();
  for (CollIndex i=0; i<maxEntries; i++)
  {
    const JoinGraphHalfPredicatePtr halfPred = halfPreds_[i];
    if (halfPred != target)
      return halfPred->getTable();
  }

  assertLogAndThrow(CAT_MVMEMO_JOINGRAPH, LL_MVQR_FAIL,
                    FALSE, QRLogicException,
                    "getOtherTable() called with a bad half pred pointer.");
  return NULL;
}

//*****************************************************************************
//*****************************************************************************
JoinGraphHalfPredicatePtr JoinGraphEqualitySet::getForeignKeySide()
{
  // Assuming only two half preds.
  return halfPreds_[getKeySource()];
}


/**
 * Prepare the input text for the Dotty graphical graph viewer for this
 * equality set.
 * @param[out] to Output string.
 *****************************************************************************
 */
//void JoinGraphEqualitySet::dumpGraph(NAString& to)
//{
// This is the first implementation. Its more accurate, but the result sucks.
//  char text[256];
//
//  // Describe this equality set (using the descriptor ID)
//  sprintf(text, "  %s [shape=ellipse, style=filled, label=\"%s\",color=red];\n", 
//          descriptorID_.data(), 
//	  descriptorID_.data());
//  to += text;
//
//  // List all the connections to the tables.
//  for (CollIndex i=0; i<halfPreds_.entries(); i++)
//  {
//    JoinGraphHalfPredicatePtr halfPred = halfPreds_[i];
//    sprintf(text, "  %s -> %s [label=\"%s\"];\n", 
//            descriptorID_.data(), 
//	    halfPred->getTable()->getID().data(), 
//	    halfPred->getExpr().data());
//    to += text;
//  }
//}  // JoinGraphEqualitySet::dumpGraph()
void JoinGraphEqualitySet::dumpGraph(NAString& to)
{
  char text[256];

  // How many half-preds?
  if (halfPreds_.entries() == 2)
  {
    if (isOnKey_)
    {
      // Two half-preds, on key - have the source point to the target.
      JoinGraphHalfPredicatePtr pred0 = halfPreds_[getKeySource()];
      JoinGraphHalfPredicatePtr pred1 = halfPreds_[keyDirection_];
      sprintf(text, "  %s -> %s [style=bold label=\"%s\"];\n", 
	      pred0->getTable()->getID().data(), 
	      pred1->getTable()->getID().data(),
	      descriptorID_.data() );
      to += text;
    }
    else
    {
      // Two half-preds, not on key - have one point to the other.
      JoinGraphHalfPredicatePtr pred0 = halfPreds_[0];
      JoinGraphHalfPredicatePtr pred1 = halfPreds_[1];
      sprintf(text, "  %s -> %s [label=\"%s\"];\n", 
	      pred0->getTable()->getID().data(), 
	      pred1->getTable()->getID().data(),
	      descriptorID_.data() );
      to += text;
    }
  }
  else
  {
    assertLogAndThrow(CAT_MVMEMO_JOINGRAPH, LL_MVQR_FAIL,
                      halfPreds_.entries() > 0, QRLogicException, 
  		      "Number of halfPreds must be at least 2.");
    // More than two - make a circular reference.
    JoinGraphHalfPredicatePtr prevPred = halfPreds_[halfPreds_.entries()-1];
    for (CollIndex i=0; i<halfPreds_.entries(); i++)
    {
      JoinGraphHalfPredicatePtr thisPred = halfPreds_[i];
      sprintf(text, "  %s -> %s [label=\"%s\"];\n", 
	      prevPred->getTable()->getID().data(), 
	      thisPred->getTable()->getID().data(),
	      descriptorID_.data() );
      prevPred = thisPred;
      to += text;
    }
  }
}  // JoinGraphEqualitySet::dumpGraph()


//========================================================================
//  Class QRJoinGraph
//========================================================================

/**
 * Destructor
 * @return 
 *****************************************************************************
 */
QRJoinGraph::~QRJoinGraph()
{
  // Clear hash table.
  tableHashByID_.clear();

  // Delete the JoinGraphTablePtr objects themselves from the main array.
  for ( CollIndex i = 0 ; i < (CollIndex)nextTable_ ; i++) 
  {
    JoinGraphTablePtr table = tableArray_[i];
    tableArray_[i] = NULL;
    table->clearEqualitySets();
    deletePtr(table);
  }

  // Delete all the equality sets.
  JoinGraphEqualitySetPtr eqSet;
  while (equalitySets_.entries() > 0)
  {
    CollIndex last = equalitySets_.entries()-1;
    eqSet = equalitySets_[last];
    equalitySets_.removeAt(last);
    deletePtr(eqSet);
    eqSet = NULL;
  }

  if (sortedTables_)
    delete sortedTables_;
}  //  QRJoinGraph::~QRJoinGraph()

/**
 * Create a JoinGraphTable object from the table element from the descriptor 
 * and add it to the join graph.
 * This method is optimized for a sorted table list, and verifies that the 
 * new table name is greater than the previous one. If not, it reorders the 
 * array.
 * @param tableElement The table element from the descriptor.
 *****************************************************************************
 */
void QRJoinGraph::addTable(const QRTablePtr	      tableElement, 
			   NABoolean		      isHub,
			   const BaseTableDetailsPtr  baseTable)
{
  JoinGraphTablePtr newTablePtr = new(heap_) 
    JoinGraphTable(nextTable_++, tableElement, isHub, baseTable, ADD_MEMCHECK_ARGS(heap_));

  // Make sure the new table name is greater than the last one we inserted.
  // Only hub tables must be ordered.
  CollIndex insertPos = newTablePtr->getOrdinalNumber();
  if (nextTable_ >= 2 && newTablePtr->isHub())
  {
    const NAString& thisName = newTablePtr->getName();
    const NAString& lastName = tableArray_[insertPos-1]->getName();
    Int32 compareResult = thisName.compareTo(lastName);
    if (compareResult < 0)
    {
      // If not - it means the table names are not sorted.
      // Looks like we will have to sort them here...
      QRLogger::log(CAT_MVMEMO_JOINGRAPH, LL_INFO,
        "Table list is not sorted!");

      // Create an empty entry in the correct place to insert the new table.
      insertPos = shiftArray(thisName);

      // Correct the ordinal number of the new entry.
      newTablePtr->setOrdinalNumber(insertPos);
    }
    else if (compareResult == 0)
    {
      isSelfJoin_ = TRUE;
      newTablePtr->setSelfJoinTable();
      tableArray_[insertPos-1]->setSelfJoinTable();
    }
  }

  const NAString& name = tableElement->getTableName();
  const NAString& id   = tableElement->getID();

  // Add table to the end of the main array.
  tableArray_.insertAt(insertPos, newTablePtr);
  
  assertLogAndThrow(CAT_MVMEMO_JOINGRAPH, LL_MVQR_FAIL,
                    !tableHashByID_.contains(&id), QRLogicException, 
		    "Table not found by ID.");

  tableHashByID_.insert(&id, newTablePtr);
}  // QRJoinGraph::addTable()

/**
 * Find the spot in the table array where to insert the new table, and shift
 * the tables in the array beyond it to create an open entry for it.
 * @param insertPos 
 * @param name 
 * @return 
 *****************************************************************************
 */
CollIndex QRJoinGraph::shiftArray(const NAString thisName)
{
  CollIndex insertPos = nextTable_-1;
  NABoolean found = FALSE;

  // We have already checked the last entry in the array.
  // Shift it one position, and change its ordinal number accordingly.
  shiftTable(insertPos);
  insertPos--;

  while (insertPos > 0)
  {
    const NAString& prevName = tableArray_[insertPos-1]->getName();
    Int32 compareResult = thisName.compareTo(prevName);
    if (compareResult < 0 )
    {
      // Not found yet. Shift the previous entry one position, 
      // and change its ordinal number accordingly.
      shiftTable(insertPos);
      insertPos--;
    }
    else
    {
      found = TRUE;

      // Did we find another instance of the same table?
      if (compareResult == 0)
	isSelfJoin_ = TRUE;

      break;
    }
  }

  return insertPos;
}  // QRJoinGraph::shiftArray()

/**
 * Shift the table in position insertPos-1 to position insertPos, and 
 * fix its ordinal number.
 * @param insertPos 
 *****************************************************************************
 */
void QRJoinGraph::shiftTable(CollIndex insertPos)
{
  JoinGraphTablePtr shiftedTable = tableArray_[insertPos-1];
  shiftedTable->setOrdinalNumber(insertPos);
  tableArray_.insertAt(insertPos, shiftedTable);
}

/**
 * Create a new JoinGraphHalfPredicate object, and add it to the equality 
 * set. Also add the equality set to the table it point to.
 * @param equalitySet The JoinGraphEqualitySet this predicate is a part of.
 * @param tablePtr The JoinGraphTable the column/expr is on.
 * @param expr The column\expression text
 * @param id The descriptor ID of this column/expression.
 *****************************************************************************
 */
void QRJoinGraph::addHalfPredicate(JoinGraphEqualitySetPtr    equalitySet,
				   JoinGraphTablePtr	      tablePtr, 
                                   const QRElementPtr         elementPtr,
				   const NAString&	      expr)
{
  // Ignore NULL tables. This probably means a join predicate to a table
  // from an outside JBB.
  if (tablePtr == NULL)
    return;

  JoinGraphHalfPredicatePtr newPred = new(heap_) 
    JoinGraphHalfPredicate(tablePtr, elementPtr, expr, ADD_MEMCHECK_ARGS(heap_));

  equalitySet->addHalfPredicate(newPred);
  tablePtr->addEqualitySet(equalitySet);
}

/**
 * Interpret the equality set element of the descriptor, and create a new 
 * JoinGraphEqualitySet object to represent it in the join graph.
 * @param predElement The descriptor element representing the equality set.
 *****************************************************************************
 */
void QRJoinGraph::addEqualitySet(const QRJoinPredPtr predElement)
{
  // Create an empty equality set object.
  const NAString& setId = predElement->getID();

  // Do we already have this equality set from a different JBB?
  JoinGraphEqualitySetPtr equalitySet = FindEqualitySetByID(setId);
  if (equalitySet == NULL)
    equalitySet = new(heap_) JoinGraphEqualitySet(setId, ADD_MEMCHECK_ARGS(heap_));

  // This may be an extraHub joi pred referencing a hub join pred.
  JoinGraphEqualitySetPtr hubEqSet = NULL;

  // Get the list of equality set internal elements (columns or expressions).
  const ElementPtrList& equalitySetElement = predElement->getEqualityList();
  for (CollIndex i=0; i<equalitySetElement.entries(); i++)
  {
    const QRElementPtr equalityElement = equalitySetElement[i]->getReferencedElement();

    // What type of element is this?
    switch(equalityElement->getElementType())
    {
      case ET_Column:
      {
	// This is a simple column predicate
	QRColumnPtr col = equalityElement->downCastToQRColumn();
	const NAString& tableID = col->getTableID();

	// create the "half predicate" object and add it to the equality set
	// as well as update the table to point to the equality set.
	addHalfPredicate(equalitySet, getTableByID(tableID), col, col->getColumnName());
	break;
      }

      case ET_Expr:
      {
	// This is an expression
	QRExprPtr expr = equalityElement->downCastToQRExpr();

	// Get the first input column, and find the table from it.
	// All input columns are supposed to be from the same table.
	const ElementPtrList& inputColList = expr->getInputColumns(heap_);
	QRColumnPtr col = inputColList[0]->getReferencedElement()->downCastToQRColumn();
	const NAString& tableID = col->getTableID();

	// create the "half predicate" object and add it to the equality set
	// as well as update the table to point to the equality set.
	addHalfPredicate(equalitySet, getTableByID(tableID), expr, expr->getExprText());
	break;
      }

      case ET_JoinPred:
      {
	// This is an extra-hub join pred referencing a hub join pred.
	// Just keep the pointer for now, and deal with it after preparing all the half preds.
        if (equalityElement->getID() != setId)
	  hubEqSet = FindEqualitySetByID(equalityElement->getID());
	break;
      }

      default:
      {
        assertLogAndThrow1(CAT_MVMEMO_JOINGRAPH, LL_MVQR_FAIL,
                           FALSE, QRLogicException,
			   "Not expecting Equi-join predicate of type: %s", equalityElement->getElementName());
      }
    } // case on element type
  }

  //if (hubEqSet != NULL && 
  //    hubEqSet->getHalfPredicates().entries() < 2)
  //{
  //  // Don't bother with equality sets with less than 2 elements.
  //  deletePtr(hubEqSet);
  //  hubEqSet = NULL;
  //  return;
  //}

  if (hubEqSet == NULL)
  {
    // This is the regular case - an independent hub join pred.
    equalitySets_.insert(equalitySet);
  }
  else
  {
    // Insert the extraHub join pred into the hub join pred.
    // This also clears the half preds from equalitySet.
    hubEqSet->addHalfPredicatesFrom(equalitySet);
    deletePtr(equalitySet);
  }

}  // QRJoinGraph::addEqualitySet()

/**
 * Given a JBB element from the MV or query descriptor, construct
 * the join graph of the JBB hub.
 * Tables in the JBB are assumed to be in ascending order. If not - the
 * table list is sorted.
 * @param jbb JBB element from the descriptor.
 * @param mv The MVDetails object. NULL for search operations.
 *****************************************************************************
 */
void QRJoinGraph::initFromJBB(const QRJBBPtr jbb, MVDetailsPtr mv)
{
  // Insert the hub tables first
  JBBDetailsPtr jbbDetails = (mv==NULL ? NULL : mv->getJbbDetails());
  const QRJBBCListPtr jbbcList = jbb->getHub()->getJbbcList();
  const ElementPtrList& elementList = jbbcList->getList();
  for (CollIndex i=0; i<elementList.entries(); i++)
  {
    QRElementPtr element = elementList[i];
    if (element->getElementType() != ET_Table)
      continue;

    QRTablePtr table = element->downCastToQRTable();

    BaseTableDetailsPtr baseTable = (jbbDetails == NULL ? 
                                     NULL               : 
                                     jbbDetails->getBaseTableByID(table->getID()));
    addTable(table, TRUE, baseTable);
    hubSize_++;
  }

  // Now insert the hub extra-hub tables.
  QRTableListPtr ExtraHubTableList = jbb->getExtraHub()->getTableList();
  if (ExtraHubTableList != NULL && ExtraHubTableList->getList().entries() > 0)
  {
    const NAPtrList<QRTablePtr>& tableList = ExtraHubTableList->getList();
    for (CollIndex j=0; j<tableList.entries(); j++)
    {
      QRTablePtr table = tableList[j];

      BaseTableDetailsPtr baseTable = (jbbDetails==NULL ? 
                                       NULL             : 
                                       jbbDetails->getBaseTableByID(table->getID()));
      addTable(table, FALSE, baseTable);
    }
  }

  // Then insert the hub equality sets.
  const QRJoinPredListPtr hubPredList = jbb->getHub()->getJoinPredList();
  if (hubPredList != NULL)
  {
    for (CollIndex k=0; k<hubPredList->entries(); k++)
    {
      const QRJoinPredPtr predElement = (*hubPredList)[k];
      addEqualitySet(predElement);
    }
  }

  // And the extra-hub equality sets.
  const QRJoinPredListPtr extraHubPredList = jbb->getExtraHub()->getJoinPredList();
  if (extraHubPredList != NULL)
  {
    for (CollIndex k=0; k<extraHubPredList->entries(); k++)
    {
      const QRJoinPredPtr predElement = (*extraHubPredList)[k];
      addEqualitySet(predElement);
    }
  }

  // Check for a GroupBy
  QRGroupByPtr groupBy = jbb->getGroupBy();
  NABoolean isEmptyGroupBy = FALSE;
  if (groupBy!=NULL)
  {
    setGroupBy();
    if (groupBy->isEmpty())
      isEmptyGroupBy = TRUE;
  }

  // Check for joins on key columns.
  // Needed only when there's a GroupBy in a query.
  if (mv==NULL && hasGroupBy() && !isEmptyGroupBy)
  {
    for (CollIndex i=0; i<tableArray_.entries(); i++)
    {
      JoinGraphTablePtr table = tableArray_[i];
      if (table->checkPredsOnKey(heap_))
        tablesToReduce_.insert(table);
    }

    if (tablesToReduce_.entries() > 0)
    {
      minimizedSubGraph_ = new (heap_) QRJoinSubGraph(this, ADD_MEMCHECK_ARGS(heap_));
      minimizedSubGraph_->addAllTables();
      for (CollIndex j=0; j<tablesToReduce_.entries(); j++)
        minimizedSubGraph_->removeTable(tablesToReduce_[j]);
      minimizedSubGraph_->setGroupBy();
    }
  }

  // Now dump the graph if priority of MvMemo category is set DEBUG.
  if (QRLogger::isCategoryInDebug(CAT_MVMEMO_JOINGRAPH))
    dumpGraph(mv);

}  //  QRJoinGraph::initFromJBB()

/**
 * Reset the temp numbers of tables and equality sets to -1.
 * This method is called before begining to generate the hash key for a subgraph.
 *****************************************************************************
 */
void QRJoinGraph::resetTempNumbers()
{
  for (CollIndex i=0; i<tableArray_.entries(); i++)
    tableArray_[i]->setTempNumber(-1);

//  for (CollIndex j=0; j<equalitySets_.entries(); j++)
//    equalitySets_[j]->setTempNumber(-1);
}

/**
 * Find an equality set using its descriptor ID.
 *****************************************************************************
 */
JoinGraphEqualitySetPtr QRJoinGraph::FindEqualitySetByID(const NAString& id)
{
  // Search list for the equality set with the correct ID.
  // If we get here a lot, we may change the implementation to a hash table.
  CollIndex maxEntries = equalitySets_.entries();
  for (CollIndex j=0; j<maxEntries; j++)
  {
    JoinGraphEqualitySetPtr eqSet = equalitySets_[j];
    if (eqSet->getID() == id)
      return eqSet;
  }

  return NULL;
}

/**
 * Find the table whose joinOrder is the smallest.
 *****************************************************************************
 */
JoinGraphTablePtr QRJoinGraph::getFirstTable()
{
  // Sort the tables by the joinOrder attribute
  UInt32 joinSize = entries();
  sortedTables_ = new(heap_) JoinGraphTableList(joinSize, heap_);

  // Insert the first table first.
  sortedTables_->insert(getTableByOrdinal(0));
  for (CollIndex i=1; i<joinSize; i++)
  {
    JoinGraphTablePtr table = getTableByOrdinal(i);
    QRTablePtr tableElem = table->getTableElement();
    Int32 joinOrder = tableElem->getJoinOrder();
    Int32 degree = table->getDegree();
    UInt32 pos=0;

    // First sort by ascending joinOrder group.
    while (pos < i && joinOrder > (*sortedTables_)[pos]->getTableElement()->getJoinOrder())
      pos++;

    // Within the joinOrder group, sort by descending degree.
    while (pos < i && joinOrder == (*sortedTables_)[pos]->getTableElement()->getJoinOrder() &&
           degree < (*sortedTables_)[pos]->getDegree() )
      pos++;

    sortedTables_->insertAt(pos, table);
  }

  if (QRLogger::isCategoryInDebug(CAT_MVMEMO_JOINGRAPH))
  {
    CollIndex maxEntries = sortedTables_->entries();
    for (CollIndex i=0; i<maxEntries; i++)
    {
      JoinGraphTablePtr table = (*sortedTables_)[i];
      QRLogger::log(CAT_MVMEMO_JOINGRAPH, LL_DEBUG, " #%d, Join order: %3d, Degree: %3d, %s",
	                                            i, 
						    table->getTableElement()->getJoinOrder(), 
						    table->getDegree(), 
						    table->getName().data());
    }

  }

  pos_ = 0;  // Initialize the iteration on the sortedTables.
  return (*sortedTables_)[0];
}

/**
 * Find a table that is connected to the subgraph, but not yet part of it.
 * @param subGraph 
 * @return 
 *****************************************************************************
 */
JoinGraphTablePtr QRJoinGraph::getNextTable(QRJoinSubGraphPtr subGraph)
{
  JoinGraphTablePtr table = (*sortedTables_)[++pos_];

  assertLogAndThrow(CAT_MVMEMO_JOINGRAPH, LL_MVQR_FAIL,
                    !subGraph->contains(table), QRLogicException, 
  		    "Duplicate table in join order.");

  if (!table->isConnectedTo(subGraph))
  {
    // Find the next connected table.
    CollIndex nextPos = pos_;
    while (nextPos < sortedTables_->entries() && !(*sortedTables_)[nextPos]->isConnectedTo(subGraph))
      nextPos++;

    JoinGraphTablePtr nextTable = (*sortedTables_)[nextPos];
    assertLogAndThrow(CAT_MVMEMO_JOINGRAPH, LL_MVQR_FAIL,
                      nextTable->isConnectedTo(subGraph), QRLogicException, 
                      "Can't find the next connected table.");

    // Now move the connected table up to the current position.
    QRLogger::log(CAT_MVMEMO_JOINGRAPH, LL_DEBUG, "Moving table %s from #%d to #%d", nextTable->getName().data(), nextPos, pos_);
    sortedTables_->removeAt(nextPos);
    sortedTables_->insertAt(pos_, nextTable);
    table = nextTable;
  }

  return table;
}

/**
 * Prepare the input text for the Dotty graphical graph viewer.
 * @param[out] to Output string.
 *****************************************************************************
 */
void QRJoinGraph::dumpGraph(MVDetailsPtr mv)
{
  // Now dump the graph.
  NAString dotGraph(heap_);
  NAString graphName("Query", heap_);
  if (mv != NULL)
    graphName = mv->getMVName();

  // Beginning parameters.
  dotGraph += "\nJoin Graph Drawing for " + graphName + ": \n";
  dotGraph += "digraph G { \n";
  dotGraph += "  graph [labelloc=\"t\",fontname = \"Times New Roman\",fontsize=20];\n";
  dotGraph += "  node [style=filled,fontsize=18,fontname = \"Courier New\"];\n";
  dotGraph += "  edge [fontsize=16,fontname = \"Courier New\"];\n";
  dotGraph += "  label=\"Join Graph for " + name_ + "\";\n";

  // Iterate over the tables in the main array.
  for ( CollIndex i = 0 ; i < (CollIndex)nextTable_ ; i++) 
  {
    tableArray_[i]->dumpGraph(dotGraph);
  }

  // Iterate over the equality sets.
  for (CollIndex j=0; j<equalitySets_.entries(); j++)
    equalitySets_[j]->dumpGraph(dotGraph);

  // End the graph description.
  dotGraph += "}\n";

  QRLogger::log(CAT_MVMEMO_JOINGRAPH, LL_DEBUG, dotGraph);
}  // QRJoinGraph::dumpGraph()


//========================================================================
//  Class QRJoinSubGraph
//========================================================================

/**
 * Add a new table to the subgraph. Check if the addition makes this
 * subgraph a self-join.
 * @param table 
 *****************************************************************************
 */
void QRJoinSubGraph::addTable(JoinGraphTablePtr table)
{
  // We need to check if this is a self-join subgraph first.
  // When the parent join graph is a self join, and we still
  // didn't flag this subgraph as a self-join.
  if (parentGraph_->isSelfJoin() && !isSelfJoin_)
  {
    for( reset(); hasNext(); advance() )
    {
      const NAString& name = getCurrent()->getName();
      if (table->getName().compareTo(name) == 0)
      {
	isSelfJoin_ = TRUE;
	break;
      }
    }
  }

  // Now, add the table to the subarray.
  subArray_.addElement(table->getOrdinalNumber());
  graphHashKey_ = ""; // Invalidate hash key.
  fastKey_ = "";      // Invalidate hash key.
}  //  QRJoinSubGraph::addTable()

/**
 * Remove a table from this join subgraph. 
 * @return 
 *****************************************************************************
 */
void QRJoinSubGraph::removeTable(JoinGraphTablePtr table)
{
  subArray_.subtractElement(table->getOrdinalNumber());
  graphHashKey_ = ""; // Invalidate hash key.
  fastKey_ = "";      // Invalidate hash key.
}

/**
 * Calculate the join graph map and hash key for this subgraph.
 * @return 
 *****************************************************************************
 */
const QRJoinSubGraphMapPtr QRJoinSubGraph::getSubGraphMap(OperationType op)
{
  CollHeap* heap = parentGraph_->getHeap();
  parentGraph_->resetTempNumbers();

  if (isSelfJoin_)
  {
    if (selfJoinHandler_)
    {
      deletePtr(selfJoinHandler_);
      selfJoinHandler_ = NULL;
    }

    selfJoinHandler_ = new (heap) SelfJoinHandler(ADD_MEMCHECK_ARGS(heap));
  }

  QRJoinSubGraphMapPtr map = new (heap) 
    QRJoinSubGraphMap(this, (op != OP_SEARCH), getParentGraph()->getNumberOfTables(), ADD_MEMCHECK_ARGS(heap));

  if (graphHashKey_ == "")
  {
    // Generate the MVMemo hash key. Example (from the doc):
    //   [1=TRAFODION.SCH.CUSTOMER]
    //   [2=TRAFODION.SCH.SALES]
    //   [3=TRAFODION.SCH.STATES]
    //   [4=TRAFODION.SCH.STORES]
    //   E1={1.CUSTOMER_ID,2.ORDERED_BY}
    //   E2={2.STORE_ID,4.ID}
    //   E3={3.ID,4.STATE}
    //   GB

    // First add and enumerate the table names
    doTablesPart(graphHashKey_, map);
    doEqualitySetsPart(graphHashKey_);
  }

  map->setHashKey(graphHashKey_);
  return map;
}

/**
 * First part of hash key generation: Add and enumerate the table names.
 * @param hashKey 
 *****************************************************************************
 */
void QRJoinSubGraph::doTablesPart(NAString& hashKey, QRJoinSubGraphMapPtr map)
{
  char buffer[10];

  //if (entries() == 10)
  //{
  //  // For debugging.
  //  QRLogger::log(CAT_MVMEMO_JOINGRAPH, LL_DEBUG, "Got to size 10!");
  //}

  // First add and enumerate the table names
  // Iterate over the internal subarray of tables.
  reset();
  for( CollIndex i=0; 
       hasNext(); 
       i++, advance() )
  {
    JoinGraphTablePtr table = getCurrent();

    // Have each table remember its number in this subgraph.
    table->setTempNumber(i); 

    if (isSelfJoin_)
      selfJoinHandler_->addTable(table);

    // Prepare the hash key line for this table.
    sprintf(buffer, "[%d=", i);
    NAString st(buffer, parentGraph_->getHeap());
    st += table->getName().data();
    st += "]\n";

    // Add line to the hash key
    hashKey += st;

    map->addTable(i, table);
  }

  if (isSelfJoin_)
  {
    selfJoinHandler_->doneAddingTables();
  }

}  // QRJoinSubGraph::doTablesPart()

/**
 * Second part of hash key generation: Sort, add and enumerate equality sets.
 * @param hashKey 
 *****************************************************************************
 */
void QRJoinSubGraph::doEqualitySetsPart(NAString& hashKey) const
{
  CollHeap* heap = parentGraph_->getHeap();

  // Collect the equality sets that have at least two of their 
  // half-predicates on the subgraph's tables.
  EqualitySetList& eqSets = parentGraph_->getEqualitySets();
  // Use system heap for array allocations
  NAStringPtr* eqArray = new(heap) NAStringPtr[eqSets.entries()];
  Int32 eqPos=0;
  UInt32 maxLineSize = 0;

  for (CollIndex j=0; j<eqSets.entries(); j++)
  {
    JoinGraphEqualitySetPtr eqSet = eqSets[j];
    if (eqSet->isInCurrentSubGraph())
    {
      NAString* line = eqSet->getHashKeyLine(heap);
      eqArray[eqPos++] = line;

      // Maintain the size of the biggest line.
      if (line->length() > maxLineSize)
	maxLineSize = line->length();
    }
  }

  // Sort the equality sets.
  qsort(eqArray,
	eqPos, 
	sizeof(NAStringPtr), 
	naStringCompare);

  // Add the text of the equality sets lines.
  char buffer[10];
  for (CollIndex k=0; k<(CollIndex)eqPos; k++)
  {
    NAStringPtr st = eqArray[k];

    sprintf(buffer, "E%d=", k);
    hashKey += (char*)buffer;
    hashKey += *st;
    hashKey += "\n";

    delete st;  // This temp string not needed anymore.
  }

  // Add the GroupBy flag if needed.
  if (hasGroupBy_)
    hashKey += "GB\n";

  NADELETEARRAY(eqArray, eqSets.entries(), NAStringPtr, heap);
}  // QRJoinSubGraph::doEqualitySetsPart()

/**
 * Set this subgraph to use all the join graph's hub tables.
 *****************************************************************************
 */
void QRJoinSubGraph::addAllTables()
{
  CollIndex numTables = parentGraph_->entries();
  for (CollIndex i=0; i<numTables; i++)
  {
    if (parentGraph_->getTableByOrdinal(i)->isHub())
      subArray_.addElement(i);
  }
}

/*****************************************************************************/
void QRJoinSubGraph::calcFastKey()
{
  fastKey_ = "[";
  reset();
  for( CollIndex i=0; 
       hasNext(); 
       i++, advance() )
  {
    JoinGraphTablePtr table = getCurrent();
    fastKey_ += table->getID();
    fastKey_ += "; ";
  }

  if (hasGroupBy())
    fastKey_ += "GB";
  fastKey_ += "]"; 
}

//========================================================================
//  Class QRJoinSubGraphMap
//========================================================================

QRJoinSubGraphMap::QRJoinSubGraphMap(const QRJoinSubGraphMap& other, ADD_MEMCHECK_ARGS_DEF(CollHeap* heap))
  : NAIntrusiveSharedPtrObject(ADD_MEMCHECK_ARGS_PASS(heap))
    ,subGraph_(new (heap) QRJoinSubGraph(*other.subGraph_, ADD_MEMCHECK_ARGS_PASS(heap)))
    ,hashKey_(other.hashKey_, heap)
    ,tableMapping_(other.tableMapping_)
    ,indexHashByID_(hashKey, INIT_HASH_SIZE_SMALL, TRUE, heap) // Pass NAString::hashKey
    ,isMV_(other.isMV_)
    ,isACopy_(TRUE)
    ,shiftVector_(other.shiftVector_)
{
  IndexHashIterator iter(other.indexHashByID_);
  const NAString* key;
  UnsignedInteger* value;
  CollIndex maxEntries = iter.entries();
  for (CollIndex j=0; j<maxEntries; j++)
  {
    iter.getNext(key, value); 
    UnsignedInteger* newValue = new(heap) UnsignedInteger(*value);
    indexHashByID_.insert(key, newValue);
  }
}

//*****************************************************************************
//*****************************************************************************
QRJoinSubGraphMap::~QRJoinSubGraphMap()
{
  tableMapping_.clear();

  IndexHashIterator iter(indexHashByID_);
  const NAString* key;
  UnsignedInteger* value;
  CollIndex maxEntries = iter.entries();
  for (CollIndex j=0; j<maxEntries; j++)
  {
    iter.getNext(key, value); 
    delete value;
    indexHashByID_.remove(key);
  }
  indexHashByID_.clear();

  subGraph_->setStillUsed(FALSE);

  // If this object was created using the copy Ctor, then the subgraph is a 
  // private copy, and should now be deleted.
  if (isACopy_)
    deletePtr(subGraph_);
}

//*****************************************************************************
//*****************************************************************************
void QRJoinSubGraphMap::addTable(CollIndex ix, const JoinGraphTablePtr table)
{
  CollHeap* heap = subGraph_->getParentGraph()->getHeap();
  tableMapping_.insertAt(ix, table);
  const NAString& id = table->getID();
  UnsignedInteger *ixInt = new (heap) UnsignedInteger(ix);
  indexHashByID_.insert(&id, ixInt);
}

//*****************************************************************************
// The query side translation: find the hash key index for a particular table.
// Return -1 if the table ID is not found (This can happen in multi-JBB queries)
//*****************************************************************************
Int32 QRJoinSubGraphMap::getIndexForTable(const NAString& id) const
{
  UnsignedInteger* ix = indexHashByID_.getFirstValue(&id);
  if (ix == NULL)
    return -1;
  else
    return ix->getInt();
}

//*****************************************************************************
//  The MV side translation: find the table information for its index.
//*****************************************************************************
const JoinGraphTablePtr QRJoinSubGraphMap::getTableForIndex(CollIndex ix) const
{
  assertLogAndThrow(CAT_MVMEMO_JOINGRAPH, LL_MVQR_FAIL,
                    isMV_, QRLogicException, 
  		    "This method should be called for MV subGraphs only.");
  return tableMapping_[ix];
}

//*****************************************************************************
// Restore the temp numbers to correspond to this map.
//*****************************************************************************
void QRJoinSubGraphMap::restoreTempNumbers() const
{
  CollIndex maxEntries = tableMapping_.entries();
  for (CollIndex i=0; i<maxEntries; i++)
    tableMapping_[i]->setTempNumber(i);
}

//*****************************************************************************
//*****************************************************************************
void QRJoinSubGraphMap::prepareForSelfJoinWork()
{
  SelfJoinHandlerPtr  selfJoinHandler = subGraph_->getSelfJoinHandler();
  selfJoinHandler->preparePermutationMatrix();

  NAString text;
  selfJoinHandler->dump(text);
  QRLogger::log(CAT_MVMEMO_JOINGRAPH, LL_DEBUG, text);

  // Restore the temp numbers to correspond to this map.
  restoreTempNumbers();

  const NAString& hashKey = getHashKey();
  Int32 firstESet = hashKey.subString("\nE").start();
  NAString halfHashKey = hashKey(0, firstESet);
  halfHashKey += "\n";
  selfJoinHandler->setHalfHashKey(halfHashKey);
}

//*****************************************************************************
//*****************************************************************************
NABoolean QRJoinSubGraphMap::hasNextEquivalentMap() const
{ 
  return (subGraph_->getSelfJoinHandler()->isDone() == FALSE); 
}

//*****************************************************************************
//*****************************************************************************
QRJoinSubGraphMapPtr QRJoinSubGraphMap::nextEquivalentMap() const
{
  restoreTempNumbers();
  SelfJoinHandlerPtr  selfJoinHandler = subGraph_->getSelfJoinHandler();

  UInt32 numberOfTables = tableMapping_.entries();
  CollHeap* heap = subGraph_->getParentGraph()->getHeap();
  QRJoinSubGraphMapPtr newMap = new(heap) 
    QRJoinSubGraphMap(*this, ADD_MEMCHECK_ARGS(heap));
  newMap->isACopy_ = FALSE;

  Int32* shiftRow = new(heap) Int32[numberOfTables];
  selfJoinHandler->getNextShiftVector(shiftRow);

  if (QRLogger::isCategoryInDebug(CAT_MVMEMO_JOINGRAPH))
  {
    NAString text("shiftRow is: ");
    for (CollIndex i=0; i<numberOfTables; i++)
    {
      char buffer[10];
      sprintf(buffer, "%2d, ", shiftRow[i]);
      text += buffer;
    }
    QRLogger::log(CAT_MVMEMO_JOINGRAPH, LL_DEBUG, text);
    
    newMap->shiftVector_ = text;  // For debugging.
  }

  for (CollIndex table=0; table<numberOfTables; table++)
  {
    if (shiftRow[table])
    {
      // Determine which entry to use over this one. 
      UInt32 source = table + shiftRow[table];
      UInt32 dest   = table;

      // Handle the tableMapping_ array.
      JoinGraphTablePtr tablePtr = tableMapping_[source];
      const NAString& id = tablePtr->getID();
      tablePtr->setTempNumber(dest);

      newMap->tableMapping_[dest] = tablePtr;

      // Handle the indexHashByID_ hash table.
      UnsignedInteger* prevIndex = newMap->indexHashByID_.getFirstValue(&id);
      newMap->indexHashByID_.remove(&id);
      if (prevIndex != NULL)
        delete prevIndex;

      UnsignedInteger *destInt = new (heap) UnsignedInteger(dest);
      newMap->indexHashByID_.insert(&id, destInt);

      // Now create the new MVMemo hash key.
      NAString newHashKey(selfJoinHandler->getHalfHashKey());
      subGraph_->doEqualitySetsPart(newHashKey);
      newMap->setHashKey(newHashKey);
    }
  }
  return newMap;
}

//========================================================================
//  Class HubIterator
//========================================================================

//*****************************************************************************
// 
//*****************************************************************************
HubIterator::~HubIterator()
{
  // Cannot use subGraphsVisited_.clear(TRUE); because that will attempt
  // to delete both key and value for each pair. Iterate manually.
  SubGraphHashIterator hashIterator(subGraphsVisited_);
  const NAString* key;
  QRJoinSubGraphPtr subGraph;

  for ( CollIndex i = 0 ; i < hashIterator.entries() ; i++) 
  {
    hashIterator.getNext(key, subGraph); 
    if (!subGraph->stillUsed())
      deletePtr(subGraph);
  }
  subGraphsVisited_.clear(); 

  // The other hash tables/lists hold pointers to the same subgraph objects.
  subGraphsReady_.clear();
  subGraphsToAvoid_.clear();

}  // HubIterator::~HubIterator()

//*****************************************************************************
// Initialize the iterator: populate the ready list with all the single-table
// subgraphs of the join graph.
//*****************************************************************************
void HubIterator::init()
{
  CollHeap* heap = joinGraph_->getHeap();

  if (opType_ == OP_INSERT_TOP || joinGraph_->hasGroupBy())
  {
    // Start with the full join graph.
    QRJoinSubGraphPtr subGraph = getFullSubGraph();
    subGraphsReady_.insert(subGraph);
    if (subGraph->hasGroupBy())
      usedGroupBy_ = TRUE;
    if (opType_ == OP_INSERT_TOP)
    {
      // This is a query descriptor during workload analysis.
      // We only need the top expression for the full join graph.
      return;
    }
  }

  // For each table in the join graph hub
  for (CollIndex i=0; i<joinGraph_->entries(); i++)
  {
    // Get the table object
    JoinGraphTablePtr table = joinGraph_->getTableByOrdinal(i);
    if (table->isHub() == FALSE)
      continue;

    // Create a new subgraph for it.
    QRJoinSubGraphPtr subGraph = new(heap) 
      QRJoinSubGraph(joinGraph_, ADD_MEMCHECK_ARGS(heap));
    // Add the table to the subgraph.
    subGraph->addTable(table);

    // Insert the new subgraph into the ready list.
    subGraphsReady_.insert(subGraph);
  }
}  // HubIterator::init()

//*****************************************************************************
// Return the next subgraph to search/insert.
//*****************************************************************************
const QRJoinSubGraphPtr HubIterator::nextSubGraph() 
{
  // First, get the supersets of the current subgraph, and insert 
  // them into the ready list. The current subgraph pointer is NULL 
  // when it should be skipped.
  if (currentSubgraph_ != NULL)
  {
    // Insert the connected supersets of currentSubgraph_ into the ready list.
    getNextLevelSupersets(currentSubgraph_);
    currentSubgraph_ = NULL;
  }

  QRJoinSubGraphPtr sg;
  while (subGraphsReady_.entries() > 0 )
  {
    // Return the hash key of the first subgraph in the ready list.
    // It is also removed from the ready list.
    subGraphsReady_.getFirst(sg);

    // If we have already tried this one, skip it.
    if (subGraphsVisited_.contains(sg->getFastKey()) ||
        subGraphsToAvoid_.contains(sg->getFastKey())    )
    {
      deletePtr(sg);
      continue;
    }

    subGraphsVisited_.insert(sg->getFastKey(), sg);
    currentSubgraph_ = sg;

    return currentSubgraph_;
  }

  // The ready list is empty. No more subgraphs.
  // If the join graph includes a GROUP BY, and we have not returned the 
  // corresponding subgraph yet - do it now.
  if (joinGraph_->hasGroupBy() && !usedGroupBy_)
  {
    assertLogAndThrow(CAT_MVMEMO_JOINGRAPH, LL_MVQR_FAIL,
                      currentSubgraph_ == NULL, QRLogicException, 
		      "Should not get here when currentSubgraph != NULL.");
    usedGroupBy_ = TRUE;

    currentSubgraph_ = getFullSubGraph();
    subGraphsVisited_.insert(currentSubgraph_->getFastKey(), currentSubgraph_);
    return currentSubgraph_;
  }
  return NULL;
}  // HubIterator::nextSubGraph()

//*****************************************************************************
//*****************************************************************************
const QRJoinSubGraphPtr HubIterator::getFullSubGraph()
{
  QRJoinSubGraphPtr subGraph = 
    new(joinGraph_->getHeap()) QRJoinSubGraph(joinGraph_, ADD_MEMCHECK_ARGS(joinGraph_->getHeap()));
  subGraph->addAllTables();
  if (joinGraph_->hasGroupBy())
    subGraph->setGroupBy();
  if (joinGraph_->isSelfJoin())
    subGraph->setSelfJoin();

  return subGraph;
}

//*****************************************************************************
// Computes the supersets of subGraph that include a single additional table
// that is conneted to it, and add them to the ready list. \par
// 1. For each table included in the subgraph, find all the connected tables
//    that are not yet part of the subgraph, and add them to a set of 
//    unique entries. \par
// 2. For each of the tables in the resulting set, create a new subgraph from 
//    the current subgraph plus that table, and add it to the ready list. \par
// @param subGraph The current subgraph.
//*****************************************************************************
void HubIterator::getNextLevelSupersets(QRJoinSubGraphPtr subGraph)
{
  if (subGraph->isFull())
    return;

  // This should be a set whenever we have an NASet that supports shared pointers.
  JoinGraphTableList tablesToAdd(joinGraph_->getHeap(), 10);

  // For each table included in the subgraph
  for(subGraph->reset(); subGraph->hasNext(); subGraph->advance())
  {
    JoinGraphTablePtr table = subGraph->getCurrent();

    // For each equality set connected to that table
    const EqualitySetList& eqSets = table->getEqualitySets();
    for (CollIndex i=0; i<eqSets.entries(); i++)
    {
      JoinGraphEqualitySetPtr eqSet = eqSets[i];

      // For each half-pred in the equality set, find the connected table.
      HalfPredicateList& halfPreds = eqSet->getHalfPredicates();
      for (CollIndex j=0; j<halfPreds.entries(); j++)
      {
	JoinGraphHalfPredicatePtr pred = halfPreds[j];
	JoinGraphTablePtr table = pred->getTable();
	if (table->isHub() == FALSE)
	  continue;

	// Check if the table is already included in the subgraph
	if ( (subGraph->contains(table) == FALSE) &&
	     (tablesToAdd.contains(table) == FALSE) )
	{
	  // We found a table we can add to the subgraph.
	  tablesToAdd.insert(table);
	}
      } // for (j) on halfPreds
    } // for (i) on equality sets
  } // for () on subgraph tables.

  // Now iterate on all the tables we found that we can add to the subgraph.
  // For each of them:
  for(CollIndex k=0; k<tablesToAdd.entries(); k++)
  {
    // Create a new subgraph based on the current subgraph.
    QRJoinSubGraphPtr newSubGraph = 
      new(joinGraph_->getHeap()) QRJoinSubGraph(*subGraph, ADD_MEMCHECK_ARGS(NULL));

    // Add the table to it.
    newSubGraph->addTable(tablesToAdd[k]);

    // If this subgraph should be skipped - skip it.
    if (subGraphsToAvoid_.contains(newSubGraph->getFastKey()) ||
        subGraphsVisited_.contains(newSubGraph->getFastKey())    )
    {
      deletePtr(newSubGraph);
      continue;
    }

    // And insert it into the ready list.
    subGraphsReady_.insert(newSubGraph);
  }
}  // HubIterator::getNextLevelSupersets()

//*****************************************************************************
// During a search operation, report that the current subgraph was not found
// in MVMemo, and so it and its supersets should be avoided.
//*****************************************************************************
void HubIterator::reportAsMissing()
{
  // This method should only be called during MVMemo search, not during insert.
  assertLogAndThrow(CAT_MVMEMO_JOINGRAPH, LL_MVQR_FAIL,
                    opType_ == OP_SEARCH, QRLogicException, 
		    "This method should only be called for search operations.");

  // Insert the current subgraph into the set of subgraphs to avoid.
  subGraphsToAvoid_.insert(currentSubgraph_->getFastKey(), currentSubgraph_);

  // Don't insert supersets of the current subgraph into the ready list.
  currentSubgraph_ = NULL;
}  // HubIterator::reportAsMissing()

//*****************************************************************************
//*****************************************************************************
const QRJoinSubGraphPtr HubIterator::getMinimizedSubGraph()
{
  QRJoinSubGraphPtr mini = joinGraph_->getMinimizedSubGraph();
  if (mini == NULL)
    return NULL;

  currentSubgraph_ = mini;
  subGraphsVisited_.insert(currentSubgraph_->getFastKey(), currentSubgraph_);
  return currentSubgraph_;
}

//*****************************************************************************
// Does this self-join have too many permutatations?
//*****************************************************************************
NABoolean HubIterator::isSelfJoinTooBig()
{
  if (!joinGraph_->isSelfJoin())
    return FALSE;

  // Allocate a SelfJoinHandler object for the calculations.
  CollHeap* heap = joinGraph_->getHeap();
  SelfJoinHandlerPtr selfJoinHandler = new (heap) 
    SelfJoinHandler(ADD_MEMCHECK_ARGS(heap));

  // Initialize it with all the tables in the join graph.
  CollIndex maxEntries = joinGraph_->entries();
  for (CollIndex i=0; i<maxEntries; i++)
  {
    JoinGraphTablePtr table = joinGraph_->getTableByOrdinal(i);
    selfJoinHandler->addTable(table);
  }
  selfJoinHandler->doneAddingTables();

  // Now calculate the number of permutations in the top join graph
  Int32 selfJoinPermutations = selfJoinHandler->howmanyPermutations();
  deletePtr(selfJoinHandler);
  if (selfJoinPermutations > MAX_SELFJOIN_PERMUTATIONS)
  {
    QRLogger::log(CAT_MVMEMO_JOINGRAPH, LL_WARN, 
                  "JoinGraph has %d self-join permutations - Skipping self-join overhead.", 
                  selfJoinPermutations);
    return TRUE;
  }
  else
  {
    QRLogger::log(CAT_MVMEMO_JOINGRAPH, LL_INFO, 
                  "JoinGraph has %d self-join permutations - Enumerating...", 
                  selfJoinPermutations);
    return FALSE;
  }
}
