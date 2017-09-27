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

#include "QmsMVMemo.h"
#include "QmsGroupLattice.h"
#include "QmsMVCandidate.h"

//
// QRGroupLattice
//

QRGroupLattice::QRGroupLattice(CollHeap* heap,
                               ADD_MEMCHECK_ARGS_DEF(CollIndex maxNumKeys))
  : NAIntrusiveSharedPtrObject(ADD_MEMCHECK_ARGS_PASS(heap)),
    lattice_(new QRLatticeIndex(heap, maxNumKeys)),
    reverseKeyHash_(hashKey, INIT_HASH_SIZE_SMALL, TRUE, heap), // Pass NAString::hashKey
    heap_(heap)
{
}

QRGroupLattice::~QRGroupLattice()
{
  deletePtr(lattice_);
}

void QRGroupLattice::dumpLattice(const char* tag)
{
  QRTRACER("QRGroupLattice::dumpLattice()");
  if (QRLogger::isCategoryInDebug(CAT_GRP_LATTCE_INDX))
  {
    NAString graph(heap_);
    lattice_->dumpLattice(graph, tag);
    QRLogger::log(CAT_GRP_LATTCE_INDX, LL_DEBUG, graph.data());
  }
}

/**
 * This method can be used to dump detailed usage stats to the log file,
 * including MVMemo hash keys and LatticeIndex graphs. 
 * Currently not used.
 *****************************************************************************
 */
void QRGroupLattice::reportStats(NAString& text)
{
  QRTRACER("QRGroupLattice::reportStats()");
  text += "\nLatticeIndex Graph:\n";
  lattice_->dumpLattice(text, "stats");
}

void QRGroupLattice::collectMVGroups(WorkloadAnalysisPtr workload, Int32 minQueriesPerMV, CollHeap* heap)
{
  lattice_->collectMVGroups(workload, minQueriesPerMV, heap);
}

void QRGroupLattice::insert(QRJoinSubGraphMapPtr map, const QRJBBPtr jbb)
{
  QRTRACER("QRGroupLattice::insert()");
  LatticeKeyList keyList;
  NABoolean ok = getGroupingLatticeKeys(keyList, map, map->getMVDetails(), jbb, FALSE, TRUE);
  assertLogAndThrow(CAT_GRP_LATTCE_INDX, LL_ERROR,
                    ok, QRLogicException, 
		    "getGroupingLatticeKeys() failed in Insert mode (Inserting a multi-JBB MV?).");
  lattice_->addMV(keyList, map);

  dumpLattice("1");
}

void QRGroupLattice::remove(QRJoinSubGraphMapPtr map)
{
  QRTRACER("QRGroupLattice::remove()");
  const QRJBBPtr jbb = map->getMVDetails()->getJbbDetails()->getJbbDescriptor();
  LatticeKeyList keyList;
  NABoolean ok = getGroupingLatticeKeys(keyList, map, map->getMVDetails(), jbb, FALSE, TRUE);
  assertLogAndThrow(CAT_GRP_LATTCE_INDX, LL_ERROR,
                    ok, QRLogicException, 
		    "getGroupingLatticeKeys() failed in Insert mode (Inserting a multi-JBB MV?).");
  lattice_->removeMV(keyList, map->getMVDetails());

  dumpLattice("2");
}

NABoolean QRGroupLattice::getGroupingLatticeKeys(LatticeKeyList&         keyList,
                                                 QRJoinSubGraphMapPtr    map,
						 DescriptorDetailsPtr    queryDetails, 
                                                 const QRJBBPtr          jbb,
                                                 NABoolean               primaryOnly,
                                                 NABoolean               insertMode)
{
  QRTRACER("QRGroupLattice::getGroupingLatticeKeys()");
  // Start with the list of primary grouping columns in the Hub.
  const QRGroupByPtr groupBy = jbb->getGroupBy();
  // In the aggregate with no GroupBy case, return an empty list.
  if (groupBy->isEmpty())
    return TRUE;

  const ElementPtrList& gbList = groupBy->getPrimaryList()->getList();
  NABoolean ok = elementListToKeyList(gbList, keyList, map, queryDetails, insertMode);
  if (!ok)
    return FALSE;

  if (primaryOnly)
    return TRUE;

  // Now check for dependent grouping columns in the extra-Hub.
  QRDependentGroupByPtr depGroupBy = groupBy->getDependentList();
  if (depGroupBy != NULL)
  {
    const ElementPtrList& depKeyList = depGroupBy->getList();
    if (!depKeyList.isEmpty())
    {
      // Dependent grouping columns found - add them to the key list.
      ok = elementListToKeyList(depKeyList, keyList, map, queryDetails, insertMode);
      if (!ok)
        return FALSE;
    }
  }

  return TRUE;
}

NABoolean QRGroupLattice::elementListToKeyList(const ElementPtrList&   elementList, 
                                               LatticeKeyList&         keyList,
                                               QRJoinSubGraphMapPtr    map,
					       DescriptorDetailsPtr    queryDetails, 
                                               NABoolean               insertMode)
{
  CollIndex maxEntries = elementList.entries();
  for (CollIndex i=0; i<maxEntries; i++)
  {
    QRElementPtr gbElement = elementList[i];
    LatticeIndexablePtr key = elementToKey(gbElement, map, queryDetails, insertMode);
    if (key == NULL)
    {
      assertLogAndThrow(CAT_GRP_LATTCE_INDX, LL_ERROR,
                        !insertMode, QRLogicException, 
		        "elementToKey() failed in Insert mode (Inserting a multi-JBB MV?).");
      // Cleanup
      for (CollIndex j=0; j<i; j++)
        delete keyList[j];
      return FALSE;
    }
    keyList.insert(key);
  }

  return TRUE;
}

MVCandidatesForJBBSubsetPtr QRGroupLattice::search(QRJBBPtr	           queryJbb, 
						   MVCandidatesForJBBPtr   mvCandidates, 
						   QRJoinSubGraphMapPtr    map,
                                                   ElementPtrList*         minimizedGroupingList)
{
  QRTRACER("QRGroupLattice::search()");
  DescriptorDetailsPtr queryDetails = mvCandidates->getAllCandidates()->getQueryDetails();
  LatticeKeyList keyList(heap_);

  if (minimizedGroupingList)
  {
    if (!elementListToKeyList(*minimizedGroupingList, keyList, map, queryDetails, FALSE))
      return NULL;
  }
  else
  {
    // Get the grouping columns of the passed query JBB, and create a key list
    // from them. Return now if there are no grouping items -- a query without a
    // Group By can't match an MV that has one.
    QRGroupByPtr groupBy = queryJbb->getGroupBy();
    if (!groupBy)
      return NULL;

    // An aggregate query with no GroupBy has an empty key list.
    if (!getGroupingLatticeKeys(keyList, map, queryDetails, queryJbb, TRUE, FALSE))
      return NULL;
  }

  // Create the search node, disallowing the addition of any new keys to the hash
  // table used by the lattice; if the key isn't already used, the node can have
  // no supersets.
  QRLatticeIndexSearchNode searchNode(keyList, lattice_, TRUE, heap_);
  if (!searchNode.isValid())   // because a key wasn't added
    return NULL;

  NAString keysText;
  searchNode.dumpNode(keysText, *lattice_->getKeyArr());
  QRLogger::log(CAT_GRP_LATTCE_INDX, LL_DEBUG, 
    "Searching LatticeIndex for: %s.", keysText.data());

  // Find the supersets of the query's group-by list in the MV group lattice
  // index. For each lattice index node returned, create an MVCandidate
  // corresponding to each MV represented by that node, and add it to the
  // candidate list passed by the caller.
  NAPtrList<QRLatticeIndexNodePtr> nodes;
  lattice_->findSupersets(searchNode, nodes);

  if (nodes.entries() == 0)
  {
    QRLogger::log(CAT_GRP_LATTCE_INDX, LL_DEBUG, "No match found.");
    return NULL;
  }

  MVCandidatesForJBBSubsetPtr jbbSubset = new(heap_) 
    MVCandidatesForJBBSubset(mvCandidates, ADD_MEMCHECK_ARGS(heap_));
  jbbSubset->setSubGraphMap(map);
  jbbSubset->setGroupBy(TRUE);

  if (minimizedGroupingList != NULL)
    jbbSubset->setMinimizedGroupingList(minimizedGroupingList);

  for (CollIndex i=0; i<nodes.entries(); i++)
    {
      QRLatticeIndexNodePtr thisNode = nodes[i];
      const SubGraphMapList& matchingMVs = thisNode->getMVs();
      NABoolean isPreferred = (*thisNode == searchNode); // preferred if exact match

      GroupingList* extraGroupingColumns = new(heap_) GroupingList(heap_);
      if (!isPreferred)
      {
	// For preferred match modes, there are no extraGroupingColumns.
	LatticeKeySubArray* diff = thisNode->computeDiff(searchNode, heap_);
	for(CollIndex i = 0; diff->nextUsed(i); i++)
	{
          LatticeIndexablePtr key = diff->element(i);
          QRElementPtr elem = keyToElement(key, map);
	  extraGroupingColumns->insert(elem);
	}
	delete diff;
      }

      for (CollIndex j=0; j<matchingMVs.entries(); j++)
        {
	  MVDetailsPtr mv = matchingMVs[j]->getMVDetails();
          QRLogger::log(CAT_GRP_LATTCE_INDX, LL_DEBUG, 
            "Found MV: %s!", mv->getMVName().data() );
	  jbbSubset->insert(mv, queryJbb, isPreferred, extraGroupingColumns, NULL, heap_);
        }
    }

  return jbbSubset;

}  // QRGroupLattice::search()

LatticeIndexablePtr QRGroupLattice::elementToKey(const QRElementPtr      element, 
                                                 QRJoinSubGraphMapPtr    map,
						 DescriptorDetailsPtr    queryDetails, 
                                                 NABoolean               insertMode,
                                                 NABoolean               isRecursive)
{
  QRElementPtr elem = element->getReferencedElement();
  QRColumnPtr col = NULL;
  if (elem->getElementType() == ET_Column)
  {
    col = elem->downCastToQRColumn();
    if (queryDetails->isFromJoin(col) && !isRecursive)
    {
    // This groupby element is a join pred.
    // During insert, this loop inserts the first column.
    // During search, try each equality set element until we find one that 
    // appears in our array.
      const QRJoinPredPtr eqSet = queryDetails->getJoinPred(col);
      const ElementPtrList& equalityList = eqSet->getEqualityList();

      if (insertMode)
      {
        // Insert mode - pick the first column of the equality set.
        col = equalityList[0]->downCastToQRColumn();
      }
      else
      {
        // Search mode - pick the first equality set entry that is found in this LatticeIndex.
        for (CollIndex i=0; i<equalityList.entries(); i++)
        {
          QRElementPtr entry = equalityList[i]->getReferencedElement();
          // Call recursively for each join pred column.
          LatticeIndexablePtr entryKey = elementToKey(entry, map, queryDetails, insertMode, TRUE);
          if (entryKey == NULL)
            continue;

          if (lattice_->contains(entryKey))
          {
            // Found it in the lattice index.
            col = entry->downCastToQRColumn();
            break;
          }
        } // for ( )

        // If none of the entries was found - give up now.
        if (col == NULL)
          return NULL;

      }  // if insert mode
    }  // if JoinPred
  } // if Column

  NAString* key = NULL;
  if (col != NULL)
  {
    col = col->getReferencedElement()->downCastToQRColumn();
    const NAString& tableID = col->getTableID();
    Int32 Inx = map->getIndexForTable(tableID);
    if (Inx == -1)
    {
      assertLogAndThrow(CAT_GRP_LATTCE_INDX, LL_ERROR,
                        !insertMode, QRLogicException, 
		        "Table index not found in Insert mode (Inserting a multi-JBB MV?).");
      return NULL;
    }

    char buffer[4096+5];
    sprintf(buffer, "%d.%s", Inx, col->getColumnName().data() );

    key = new(heap_) NAString(buffer, heap_);
  }
  else
  {
    // This is an expression.
    // TBD We still do not handle expressions using columns from self-join tables.
    key = new(heap_) NAString(element->getSortName(), heap_);
  }

  // The reverseKeyHash should always use the most recent MV inserted.
  if (insertMode)
  {
    reverseKeyHash_.remove(key);
    reverseKeyHash_.insert(key, element);
  }
  return key;
}

QRElementPtr QRGroupLattice::keyToElement(LatticeIndexablePtr key, QRJoinSubGraphMapPtr map)
{
  const QRElementPtr elem = reverseKeyHash_.getFirstValue(key);
  return elem;
}
