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
// File:         MVDetails.cpp
// Description:  
//               
//               
//               
//
// Created:      08/21/08
// ***********************************************************************

#include "QmsMVDetails.h"
#include "QmsMVCandidate.h"
#include "QRDescriptor.h"
#include "QmsMatchTest.h"


//========================================================================
//  Class RefFinderVisitor
//========================================================================

Visitor::VisitResult RefFinderVisitor::visit(QRElementPtr caller)
{
  // Elements that don't reference other elements, should reference 
  // themselves (the default value).
  if (caller->getRef() == "")
    return VR_Continue;

  QRElementPtr elem = descDetails_->getElementForID(caller->getRef());
  assertLogAndThrow(CAT_MATCHTST_MVDETAILS, LL_MVQR_FAIL, 
                    elem != NULL, QRLogicException, 
		    "Element is referencing a non-existing element.");
  caller->setReferencedElement(elem);

  return VR_Continue;
}

//========================================================================
//  Class BaseTableDetails
//========================================================================

// ***************************************************************************
// ***************************************************************************
void BaseTableDetails::addRangePredicateOnColumn(const QRRangePredPtr rangePred, 
						 const QRColumnPtr rangeCol)
{
  Int32 index = rangeCol->getColIndex();

  assertLogAndThrow(CAT_MATCHTST_MVDETAILS, LL_MVQR_FAIL,
                    isHub_ == (NABoolean)TRUE, QRLogicException,
		    "No range preds expected on extra-hub tables.");
  assertLogAndThrow(CAT_MATCHTST_MVDETAILS, LL_MVQR_FAIL,
                    index != -1, QRLogicException,
		    "Column index is not initialized.");
  
  rangeColumnPredicates_[index] = rangePred;
}

//========================================================================
//  Class JBBDetails
//========================================================================

// ***************************************************************************
// Initialize internal data structures.
// ***************************************************************************
void JBBDetails::init(CollHeap* heap)
{
  // Check if this JBB has a GroupBy in it.
  const QRGroupByPtr groupBy = jbbDesc_->getGroupBy();
  if (groupBy != NULL)
  {
    hasGroupBy_ = TRUE;
    hasEmptyGroupBy_ = groupBy->isEmpty();
  }

  initBaseTables(heap);
  initRangePreds(heap);  // For query descriptors, partial init only.

  if (isAnMV_)
  {
    // Init residual predicate data stuctures for MVs only.
    initResidualPreds(heap);
  }
  else
  {
    // This is a query descriptor.
    // Check if it explicitly uses MVs, and if so - replace them so that
    // the query will be only on base tables.
    // TBD...
  }
}

// ***************************************************************************
// ***************************************************************************
void JBBDetails::initBaseTables(CollHeap* heap)
{
  // Create BaseTableDetails objects for the JBB hub tables.
  const ElementPtrList& hubTables = jbbDesc_->getHub()->getJbbcList()->getList();
  for (CollIndex i=0; i<hubTables.entries(); i++)
  {
    const QRElementPtr jbbc = hubTables[i];
    if (jbbc->getElementType() != ET_Table)
      continue; // Skip reference to other JBBs.

    const QRTablePtr table = jbbc->downCastToQRTable();
    if (table->hasLOJParent())
      hasLOJs_ = TRUE;

    // Rest of the stuff only needed for MVs.
    if (!isAnMV_)
      continue;

    BaseTableDetailsPtr tableDetails = new(heap) 
      BaseTableDetails(table, TRUE, ADD_MEMCHECK_ARGS(heap));
    baseTablesByID_.insert(&tableDetails->getID(), tableDetails);
  }

  // Now do the same for the extra-hub tables.
  const QRTableListPtr extraHubTables = jbbDesc_->getExtraHub()->getTableList();
  if (extraHubTables != NULL)
  {
    for (CollIndex j=0; j<extraHubTables->entries(); j++)
    {
      const QRTablePtr table = (*extraHubTables)[j];
      if (table->hasLOJParent())
        hasLOJs_ = TRUE;

      // Rest of the stuff only needed for MVs.
      if (!isAnMV_)
        continue;

      BaseTableDetailsPtr tableDetails = new(heap) 
	BaseTableDetails(table, FALSE, ADD_MEMCHECK_ARGS(heap));
      baseTablesByID_.insert(&tableDetails->getID(), tableDetails);
    }
  }
}

// ***************************************************************************
// ***************************************************************************
void JBBDetails::initRangePreds(CollHeap* heap)
{
  // Initialize the Range predicates.
  const QRRangePredListPtr rangePreds = jbbDesc_->getHub()->getRangePredList();
  if (rangePreds==NULL || rangePreds->entries() == 0)
  {
    hasNoRangePredicates_ = TRUE;
    return;
  }

  for (CollIndex k=0; k<rangePreds->entries(); k++)
  {
    const QRRangePredPtr rangePred = (*rangePreds)[k];
    const QRElementPtr rangeElem = rangePred->getRangeItem()->getReferencedElement();

    if (rangeElem->getElementType() == ET_Column && rangePred->isSingleValue())
    {
      constColumns_.insert(&rangeElem->getID());
    }

    // For query descriptors, only check for const columns.
    if (!isAnMV_)
      continue;

    if (rangeElem->getElementType() == ET_Column)
    {
      const QRColumnPtr rangeColumn = rangeElem->downCastToQRColumn();

      if (!descDetails_->isFromJoin(rangeColumn))
      {
        // This column is NOT part of a join predicate.
        BaseTableDetailsPtr rangeTable = getBaseTableByID(rangeColumn->getTableID());
        rangeTable->addRangePredicateOnColumn(rangePred, rangeColumn);
      }
      else
      {
        // This column IS part of a join predicate.
        // Insert the range pred for all the participating columns.
        const QRJoinPredPtr jp = descDetails_->getJoinPred(rangeColumn);
        const ElementPtrList& equalitySet = jp->getEqualityList();
        for (CollIndex i=0; i<equalitySet.entries(); i++)
        {
	  const QRElementPtr halfPred = equalitySet[i];

	  // Now lets look at the equality set members
	  // Ignore members that are not simple columns.
	  if (halfPred->getElementType() != ET_Column)
	    continue;

	  const QRColumnPtr eqSetColumn = halfPred->getReferencedElement()->downCastToQRColumn();
          BaseTableDetailsPtr rangeTable = getBaseTableByID(eqSetColumn->getTableID());
          rangeTable->addRangePredicateOnColumn(rangePred, eqSetColumn);
        }
      }
    }
    else if (rangeElem->getElementType() == ET_JoinPred)
    {
      assertLogAndThrow(CAT_MATCHTST_MVDETAILS, LL_MVQR_FAIL,
                        (rangeElem->getElementType() == ET_JoinPred), QRLogicException, 
			"Unexpected JoinPred element.");

      // If its an equality set, insert the range pred for all the participating columns.
      const QRJoinPredPtr joinPredElement = rangeElem->downCastToQRJoinPred();
      const ElementPtrList& equalitySet = joinPredElement->getEqualityList();
  
      for (CollIndex i=0; i<equalitySet.entries(); i++)
      {
	const QRElementPtr halfPred = equalitySet[i];

	// Now lets look at the equality set members
	// Ignore members that are not simple columns.
	if (halfPred->getElementType() != ET_Column)
	  continue;

	const QRColumnPtr eqSetColumn = halfPred->getReferencedElement()->downCastToQRColumn();
        BaseTableDetailsPtr rangeTable = getBaseTableByID(eqSetColumn->getTableID());
        rangeTable->addRangePredicateOnColumn(rangePred, eqSetColumn);
      }
    }
    else
    {
      // The range pred is on an expression. Insert it for all the expression's input columns.
      const QRExprPtr rangeExpr = rangeElem->downCastToQRExpr();

      if (rangeExpr->getExprRoot()->containsAnAggregate(heap))
      {
        hasHavingPredicates_ = TRUE;
      }

      // Verify we have a single input column
      // No input columns mean its a COUNT(*), which is OK too.
      const ElementPtrList& inputs = rangeExpr->getInputColumns(heap);
      assertLogAndThrow(CAT_MATCHTST_MVDETAILS, LL_MVQR_FAIL,
                        (inputs.entries() <= 1), QRLogicException, 
			"Range predicate expression must have a single input at most.");

      const NAString& predText = rangeExpr->getExprText();

      // Look it up in the hash table: do we already have a range pred with this text?
      RangePredPtrList* predList = getRangePredsOnExpression(predText);

      if (predList == NULL)
	{
	  // No, its the first one. Create a new list.
	  predList = new(heap) RangePredPtrList(heap);
	  // And insert it into the hash table.
	  rangePredicates_.insert(&predText, predList);
	}

      // Insert the predicate into the pred list.
      predList->insert(rangePred);
    }
  }
}  // JBBDetails::init()

// ***************************************************************************
// ***************************************************************************
void JBBDetails::initResidualPreds(CollHeap* heap)
{
  // Initialize the Residual predicates.
  const QRResidualPredListPtr residualPreds = jbbDesc_->getHub()->getResidualPredList();
  if (residualPreds==NULL || residualPreds->entries() == 0)
  {
    hasNoResidualPredicates_ = TRUE;
    return;
  }

  // For each residual predicate in the MV descriptor
  for (CollIndex k=0; k<residualPreds->entries(); k++)
  {
    // Get The predicate text
    const QRExprPtr residualPred = (*residualPreds)[k];

    if (residualPred->getExprRoot()->containsAnAggregate(heap))
    {
      hasHavingPredicates_ = TRUE;
    }

    const NAString& predText = residualPred->getExprText();

    // The goal of the next call is to init the input column list
    // using the MV heap rather than the query heap.
    residualPred->getInputColumns(heap);

    // Look it up in the hash table: do we already have a residual pred with this text?
    ResidualPredPtrList* predList = getResidualPreds(predText);

    if (predList == NULL)
    {
      // No, its the first one. Create a new list.
      predList = new(heap) ResidualPredPtrList(heap);
      // And insert it into the hash table.
      residualPredicates_.insert(&predText, predList);
    }

    // Insert the predicate into the pred list.
    predList->insert(residualPred);
  }
}  // JBBDetails::initResidualPreds()

//========================================================================
//  Class DescriptorDetails
//========================================================================

DescriptorDetails::~DescriptorDetails()
{
  for (CollIndex i=0; i<jbbDetailsList_.entries(); i++)
    deletePtr(jbbDetailsList_[i]);

  deletePtr(descriptor_);
}

// ***************************************************************************
// Initialize internal data structures.
// ***************************************************************************
void DescriptorDetails::init(CollHeap* heap)
{
  // Map all the IDs in the MV descriptor for easy access.
  // This call does a tree walk to initialize the idHash_ map of IDs to elements.
  MapIDsVisitorPtr visitor1 = new(heap) MapIDsVisitor(idHash_, ADD_MEMCHECK_ARGS(heap));
  descriptor_->treeWalk(visitor1);
  deletePtr(visitor1);
  
  RefFinderVisitorPtr visitor2 = new(heap) RefFinderVisitor(this, ADD_MEMCHECK_ARGS(heap));
  descriptor_->treeWalk(visitor2);
  deletePtr(visitor2);

  // Map all the JoinPreds to their participating columns.
  // This must be done before initializing outputs and other preds.
  initJoinPreds();

  // Init the list of JBBDetails
  const NAPtrList<QRJBBPtr>& jbbs = descriptor_->getJbbList();
  for (CollIndex i = 0; i < jbbs.entries(); i++)
  {
    QRJBBPtr jbb = jbbs[i];
    JBBDetailsPtr jbbDetails = 
      new(heap) JBBDetails(jbb, this, isAnMV_, ADD_MEMCHECK_ARGS(heap));
    jbbDetails->init(heap);
    addJBBDetails(jbbDetails);
  }
}

// ***************************************************************************
// ***************************************************************************
const NAString& DescriptorDetails::getTableNameFromColumn(QRColumnPtr colElem)
{
  static const NAString empty("");

  const NAString& tableID = colElem->getReferencedElement()->downCastToQRColumn()->getTableID();
  const QRElementPtr tableElem = getElementForID(tableID);
  if (tableElem==NULL)
  {
    // This table is in a JBB that was ommitted from the query descriptor.
    // Return an empty table name.
    return empty;
  }

  const QRTablePtr table = tableElem->downCastToQRTable();
  return table->getTableName();
}

// ***************************************************************************
// ***************************************************************************
void DescriptorDetails::initJoinPreds()
{
  // Initialize the Join predicates.
  const NAPtrList<QRJBBPtr>& jbbs = descriptor_->getJbbList();
  for (CollIndex i = 0; i < jbbs.entries(); i++)
  {
    QRJBBPtr jbb = jbbs[i];
    const QRJoinPredListPtr hubJoinPreds = jbb->getHub()->getJoinPredList();
    initJoinPredList(hubJoinPreds);

    const QRJoinPredListPtr extraHubJoinPreds = jbb->getExtraHub()->getJoinPredList();
    initJoinPredList(extraHubJoinPreds);
  }
}

// ***************************************************************************
// ***************************************************************************
void DescriptorDetails::initJoinPredList(const QRJoinPredListPtr jpList)
{
  if (jpList == NULL)
    return;

  CollIndex maxEntries = jpList->entries();
  //Iterate over JoinPred list
  for (CollIndex i=0; i<maxEntries; i++)
  {
    const QRJoinPredPtr joinPred = (*jpList)[i];
    const ElementPtrList& equalitySet = joinPred->getEqualityList();

    // Iterate over equality set in JoinPred.
    for (CollIndex i=0; i<equalitySet.entries(); i++)
    {
      const QRElementPtr halfPred = equalitySet[i]->getReferencedElement();

      // Now lets look at the equality set members
      // Ignore members that are not simple columns.
      if (halfPred->getElementType() != ET_Column)
        continue;

      const QRColumnPtr eqSetColumn = halfPred->getReferencedElement()->downCastToQRColumn();

      // Skip columns from other JBBs.
      if (getElementForID(eqSetColumn->getTableID()) == NULL)
        continue;

      // Skip columns from LOJ tables.
      if (isColumnFromLojTable(eqSetColumn))
        continue;

      const NAString& columnID = eqSetColumn->getID();
      joinPredHash_.insert(&columnID, joinPred);
    }
  }
}

// ***************************************************************************
// Is this column from a table that has an LOJ parent?
// ***************************************************************************
NABoolean DescriptorDetails::isColumnFromLojTable(const QRColumnPtr col)
{
  const NAString& tableID = col->getTableID();
  const QRElementPtr tableElem = getElementForID(tableID);
  assertLogAndThrow(CAT_MATCHTST_MVDETAILS, LL_MVQR_FAIL,
                    tableElem, QRLogicException, 
		    "Table not found in JBB.");
  const QRTablePtr table = tableElem->downCastToQRTable();
  return table->hasLOJParent();
}


//========================================================================
//  Class MVDetails
//========================================================================

// ***************************************************************************
// MVDetails desctructor.
// Delete the join graph of the MV.
// ***************************************************************************
MVDetails::~MVDetails()
{
  if (joinGraph_)
    deletePtr(joinGraph_);

  // In outputByColumnName_ we use the ID qualified column name as the key.
  // For each entry we dynamically allocate this string and initialize it,
  // so now we need to delete it.
  // The value of this hash table is a descriptor element, that will be deleted
  // with the rest of the descriptor.
  QRElementHashIterator iter(outputByColumnName_);
  CollIndex maxEntries = iter.entries();
  const NAString*    key;
  QRElementPtr value;
  for (CollIndex i=0; i<maxEntries; i++)
  {
    iter.getNext(key, value);
    delete key;
  }
  outputByColumnName_.clear();

  if (mvMemoGroups_.entries() > 0)
    disengage();

  outputByExprText_.clear();  
}

// ***************************************************************************
// Remove this MV from all the MVMemoGroups that keep a pointer to this object.
// ***************************************************************************
void MVDetails::disengage()
{
  CollIndex maxEntries = mvMemoGroups_.entries();
  for (CollIndex j=0; j<maxEntries; j++)
  {
    mvMemoGroups_[j]->dropMV(getMVName());
  }
  mvMemoGroups_.clear();
}

// ***************************************************************************
// The MV name inside the MVDetails object is the key of the hash table of 
// physical expressions inside each MVMemoGroup.
// Therefore we must start the rename operation when this object holds the
// old name, and complete it when it holds the new name.
// ***************************************************************************
void MVDetails::rename(const NAString& oldName, const NAString& newName)
{
  CollIndex maxEntries = mvMemoGroups_.entries();
  for (CollIndex j=0; j<maxEntries; j++)
  {
    mvMemoGroups_[j]->startRenameMV(oldName);
  }

  name_ = newName;

  for (CollIndex j=0; j<maxEntries; j++)
  {
    mvMemoGroups_[j]->FinishRenameMV();
  }
}

// ***************************************************************************
// Initialize internal data structures.
// ***************************************************************************
void MVDetails::init(CollHeap* heap)
{
  // Init the superclass
  DescriptorDetails::init(heap);

  const NAPtrList<QRJBBPtr>& jbbs = descriptor_->getJbbList();
  assertLogAndThrow(CAT_MATCHTST_MVDETAILS, LL_MVQR_FAIL,
                    jbbs.entries() == 1, QRLogicException, 
		    "Support single JBB MVs only for now.");
  // Init the shortcut to the first (and only) JBB in MV descriptors.
  jbbDetails_ = jbbDetailsList_[0];
  // TBD - When we support multi-JBB MVs, we need to loop over the jbb list.
  const QRJBBPtr jbb = jbbs[0];

  // Initialize the several hash tables that map output list elements.
  initOutputs(jbb, heap);

  // Initialize the hash of tables in the descriptor.
  initTables(jbb, heap);
}

// ***************************************************************************
// Initialize the several hash tables that map output list elements:
//    outputByIDHash_	      - Using the ID of the output item as the key.
//    outputByColumnName_     - Using the "ID Qualified" column name as the key.
//    outputByExprText_	      - Using the expression text as the key.
//    outputByInputColumns_   - Using the name of expression input column as the key.
// ***************************************************************************
void MVDetails::initOutputs(const QRJBBPtr jbb, CollHeap* heap)
{
  // Match the output elements with their names.
  const QROutputListPtr outputList = jbb->getOutputList();

  assertLogAndThrow(CAT_MATCHTST_MVDETAILS, LL_MVQR_FAIL,
                    outputList != NULL, QRLogicException,
                    "Output list is null.");

  // For each output list element
  for (CollIndex i=0; i<outputList->entries(); i++)
  {
    QROutputPtr output = (*outputList)[i];
    // Set the ordinal number of the output element.
    output->setColPos(i+1);

    // Get the output item (column or expression).
    QRElementPtr outputItem = output->getOutputItem()->getReferencedElement();
    const NAString& id = outputItem->getID();
    // Insert the ID of the output item (whatever it is) into the ID hash.
    outputByIDHash_.insert(&id, output);

    // OK, now lets check what type of output item it is.
    switch (outputItem->getElementType())
    {
      case ET_Column:
      {
	// If its an output column, add it to the output column by name hash.
	const QRColumnPtr col = outputItem->downCastToQRColumn();

        const QRElementPtr tableElem = getElementForID(col->getTableID());
        NABoolean hasLOJParent = tableElem->downCastToQRTable()->hasLOJParent();

        if (!isFromJoin(col) || hasLOJParent)
        {
	  // Compute the "ID Qualified" column name as the key.
	  const NAString* idQualifiedColName = 
	    calcIDQualifiedColumnName(col->getTableID(),  col->getColumnName(), heap);

	  // Insert into the hash table.
	  outputByColumnName_.insert(idQualifiedColName, output);
        }
        else
        {
	  // If its an equality set, insert all the participating columns
	  // as pointing to the same MV output column.
          const QRJoinPredPtr jp = getJoinPred(col);
	  const QRJoinPredPtr joinPredElement = jp->downCastToQRJoinPred();
          addEqualitySet(joinPredElement, FALSE, output, heap);
        }
	break;
      }

      case ET_JoinPred:
      {
        assertLogAndThrow(CAT_MATCHTST_MVDETAILS, LL_MVQR_FAIL,
                          (outputItem->getElementType() == ET_JoinPred), QRLogicException, 
			  "Unexpected JoinPred element.");

	// If its an equality set, insert all the participating columns
	// as pointing to the same MV output column.
	const QRJoinPredPtr joinPredElement = outputItem->downCastToQRJoinPred();
        addEqualitySet(joinPredElement, FALSE, output, heap);
	break;
      }

      case ET_Expr:
      {
	// This is an expression. Insert the expression text into the expression hash.
	const QRExprPtr expr = outputItem->downCastToQRExpr();
        const NAString& exprText = expr->getExprText(); 
	outputByExprText_.insert(&exprText, output);
        QRLogger::log(CAT_MATCHTST_MVDETAILS, LL_DEBUG,
          "Adding output expression: %s.", exprText.data());

	// Also map the expressions's input columns to it.
	// This call to getInitColumns() also inits the input list using the 
	// right heap.
	const ElementPtrList& inputs = expr->getInputColumns(heap);
	if (inputs.entries() > 0)
	{
	  for (CollIndex j=0; j<inputs.entries(); j++)
	  {
            QRElementPtr inputElem = inputs[j]->getReferencedElement();
            if (inputElem->getElementType() == ET_Column)
            {
	      QRColumnPtr inputCol = inputElem->downCastToQRColumn();

	      // Compute the "ID Qualified" column name as the key.
	      const NAString* idQualifiedColName = 
	        calcIDQualifiedColumnName(inputCol->getTableID(),  inputCol->getColumnName(), heap);

	      outputByInputColumns_.insert(idQualifiedColName, output);
	    }
            else
            {
              QRJoinPredPtr inputJoinPred = inputElem->downCastToQRJoinPred();
              addEqualitySet(inputJoinPred, TRUE, output, heap);
            }
          }
	}

        // Check if this is the COUNT(*) function.
        QRExplicitExprPtr rootExpr = expr->getExprRoot();
        if (rootExpr->getElementType() == ET_Function)
        {
          QRFunctionPtr func = rootExpr->downCastToQRFunction();
          if (func->getAggregateFunc() == QRFunction::AFT_COUNTSTAR)
          {
            // Found it, now remember it for later use.
            // If a COUNT(*) equivalent function has already been found 
            // (see below) - overwrite it.
            countStar_ = output;
          }
          else if (countStar_ == NULL &&
                   func->isCountStarEquivalent(heap) )
          {
            // Well, we didn't find a COUNT(*) yet, but we found a COUNT(a)
            // and the input column is NOT NOLL, we can use it as a COUNT(*) column.
            countStar_ = output;
          }
        }
      } // end of case ET_Expr
      break;
    }  // switch on element type
  }  // for on output list elements
}  //  MVDetails::initOutputs()

// ***************************************************************************
// ***************************************************************************
void MVDetails::addEqualitySet(QRJoinPredPtr joinPredElement, 
                               NABoolean     isFromExpr, 
                               QROutputPtr   output, 
                               CollHeap*     heap)
{
  const ElementPtrList& equalitySet = joinPredElement->getEqualityList();

  for (CollIndex i=0; i<equalitySet.entries(); i++)
  {
    const QRElementPtr halfPred = equalitySet[i];

    // Now lets look at the equality set members
    // Ignore members that are not simple columns.
    if (halfPred->getElementType() != ET_Column)
      continue;

    const QRColumnPtr eqSetColumn = halfPred->getReferencedElement()->downCastToQRColumn();

    // If this JoinPred element is a column of an LOJ table,
    // skip it because it is not really equal to the others.
    const QRElementPtr tableElem = getElementForID(eqSetColumn->getTableID());
    if (tableElem->downCastToQRTable()->hasLOJParent())
      continue;

    // Compute the "ID Qualified" column name as the key.
    const NAString* idQualifiedColName = 
      calcIDQualifiedColumnName(eqSetColumn->getTableID(),  eqSetColumn->getColumnName(), heap);

    // Insert into the hash table.
    if (isFromExpr)
      outputByInputColumns_.insert(idQualifiedColName, output);
    else
      outputByColumnName_.insert(idQualifiedColName, output);
  }
} // MVDetails::addEqualitySet

// ***************************************************************************
// Initialize the hash of tables in the descriptor.
// For every table in the JBBC list of the Hub as well as the extraHub, 
// insert an entry into the hash table, using the table's name as the hash key.
// ***************************************************************************
void MVDetails::initTables(const QRJBBPtr jbb, CollHeap* heap)
{
  // Map all the table names
  // Start with the Hub tables.
  const ElementPtrList& hubTables = jbb->getHub()->getJbbcList()->getList();
  for (CollIndex i=0; i<hubTables.entries(); i++)
  {
    QRElementPtr element = hubTables[i];

    // Ignore JBBCs that are not tables - they must be child-JBB references,
    // so we will get to them when looping over the JBB list.
    if (element->getElementType() == ET_Table)
    {
      const QRTablePtr table = element->downCastToQRTable();
      const NAString& name = table->getTableName();
      tableByNameHash_.insert(&name, table);
    }
  }

  // Continue with the extra-hub tables.
  const QRExtraHubPtr extraHub = jbb->getExtraHub();
  if (extraHub != NULL)
  {
    QRTableListPtr extraHubTables = extraHub->getTableList();
    if (extraHubTables != NULL)
    {
      for (CollIndex j=0; j<extraHubTables->entries(); j++)
      {
	QRTablePtr table = (*extraHubTables)[j];
	const NAString& name = table->getTableName();
	tableByNameHash_.insert(&name, table);    
      }
    }
  }
}

// ***************************************************************************
// Get the output list element, that its output item (column or expression)
// has the ID id (or NULL if not found).
// ***************************************************************************
const QROutputPtr MVDetails::getOutputForID(const NAString& id)
{
  QRElementPtr output = outputByIDHash_.getFirstValue(&id);
  if (output == NULL)
    return NULL;
  
  return output->downCastToQROutput();
}

// ***************************************************************************
// Get the table element (from the JBBC list) with this name (or NULL if not found).
// This method must NOT be used to find a specific table instance, because
// with self-joins, multiple tables have the same name. 
// Use MVCandidate::getMvTableForQueryID() instead.
// ***************************************************************************
const QRTablePtr MVDetails::getTableFromName(const NAString& name)
{
  QRElementPtr table = tableByNameHash_.getFirstValue(&name);
  if (table == NULL)
    return NULL;
  
  return table->downCastToQRTable();
}

// ***************************************************************************
// Get the output list element, that its internal column name is the name 
// parameter (or NULL if not found).
// ***************************************************************************
const QROutputPtr MVDetails::getOutputByColumnName(const NAString& id, 
						   const NAString& name, 
						   CollHeap* heap)
{
  const NAString* idName = calcIDQualifiedColumnName(id, name, heap);

  QRElementPtr output = outputByColumnName_.getFirstValue(idName);
  delete idName;

  if (output == NULL)
    return NULL;
  
  return output->downCastToQROutput();
}

// ***************************************************************************
// Get the output list element, that represents the output expression that 
// matches text (or NULL if not found).
// ***************************************************************************
const QROutputPtr MVDetails::getOutputByExprText(const NAString& text)
{
  QRElementPtr output = outputByExprText_.getFirstValue(&text);
  QRLogger::log(CAT_MATCHTST_MVDETAILS, LL_DEBUG,
    "Checking for output expression: %s, %s.", 
    text.data(),
    (output == NULL) ? "Not found it" : "Found it");

  if (output == NULL)
    return NULL;
  else
    return output->downCastToQROutput();
}

// ***************************************************************************
// Get the output list element, that represents an expression, that has 
// this column name as one of its input columns (or NULL if not found).
// ***************************************************************************
const QROutputPtr MVDetails::getOutputByInputColumns(const NAString& id, 
						     const NAString& name, 
						     CollHeap* heap)
{
  const NAString* idName = calcIDQualifiedColumnName(id, name, heap);

  QRElementPtr output = outputByInputColumns_.getFirstValue(idName);
  delete idName;
  if (output == NULL)
    return NULL;
  
  return output->downCastToQROutput();
}
