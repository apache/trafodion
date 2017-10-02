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

#include "QmsLatticeIndex.h"

//
// QRLatticeIndexNode
//

QRLatticeIndexNode::QRLatticeIndexNode(const LatticeKeyList& keys,
                                       QRLatticeIndexPtr lattice,
                                       Int32 referenceNumber,
                                       NABoolean noNewEntry,
                                       ADD_MEMCHECK_ARGS_DEF(CollHeap* heap))
  : NAIntrusiveSharedPtrObject(ADD_MEMCHECK_ARGS_PASS(heap)),
    lattice_(lattice)
   ,parents_(heap)
   ,children_(heap)
   ,isTop_(FALSE)
   ,isBottom_(FALSE)
   ,isValid_(TRUE)
   ,keyBitmap_(lattice->getKeyArr(), heap)
   ,visitMarker_(0)
   ,noNewEntry_(noNewEntry)
   ,mapList_(heap)
   ,heap_(heap)    
{
  QRTRACER("QRLatticeIndexNode::QRLatticeIndexNode()");
  if (referenceNumber == SEQNUM_UNASSIGNED)
    *nodeName_ = '\0';
  else
    sprintf(nodeName_, "n%d", referenceNumber);

  isValid_ = lattice->addKeysToBitmap(keys, noNewEntry, keyBitmap_);
}

QRLatticeIndexNode::QRLatticeIndexNode(QRLatticeIndexPtr lattice,
                                       NABoolean noNewEntry,
                                       ADD_MEMCHECK_ARGS_DEF(CollHeap* heap))
  : NAIntrusiveSharedPtrObject(ADD_MEMCHECK_ARGS_PASS(heap))
   ,lattice_(lattice)
   ,parents_(heap)
   ,children_(heap)
   ,isTop_(FALSE)
   ,isBottom_(FALSE)
   ,isValid_(TRUE)
   ,keyBitmap_(lattice->getKeyArr(), heap)
   ,visitMarker_(0)
   ,noNewEntry_(noNewEntry)
   ,mapList_(heap)
   ,heap_(heap)
{
  QRTRACER("QRLatticeIndexNode::QRLatticeIndexNode()");
  *nodeName_ = '\0';
}

QRLatticeIndexNode::QRLatticeIndexNode(const char* const name,
                                       QRLatticeIndexPtr lattice,
                                       LatticeKeyArray* keyArray,
                                       ADD_MEMCHECK_ARGS_DEF(CollHeap* heap))
  : NAIntrusiveSharedPtrObject(ADD_MEMCHECK_ARGS_PASS(heap)),
    lattice_(lattice), parents_(heap), children_(heap),
    isTop_(FALSE), isBottom_(FALSE), isValid_(TRUE),
    keyBitmap_(keyArray, heap),
    visitMarker_(0), heap_(heap)
{
  QRTRACER("QRLatticeIndexNode::QRLatticeIndexNode()");
  strcpy(nodeName_, name);
  if (!strcmp(name, QRLatticeIndex::TOP_NODE_NAME))
    isTop_ = TRUE;
  else if (!strcmp(name, QRLatticeIndex::BOTTOM_NODE_NAME))
    isBottom_ = TRUE;
}

/***
This method is not called from anywhere
void QRLatticeIndexNode::addKeyToSearchNode(LatticeIndexable& key,
                                            QRLatticeIndexPtr lattice)
{
  QRTRACER("QRLatticeIndexNode::addKeyToSearchNode()");
  // Adding a key to a node that has already been inserted into the lattice
  // index could make its placement invalid. This function is only valid for
  // search nodes, which exist outside the lattice and are used to probe it.
  assertLogAndThrow(CAT_GRP_LATTCE_INDX, LL_ERROR,
                    !isLatticeNode(), QRLogicException,
		    "Cannot add key to node already in lattice.");

  // Set the key in the bitmap.
  CollIndex keyIndex = lattice->getKeyIndex(&key, noNewEntry_);
  if (keyIndex == NULL_COLL_INDEX)
    {
      invalidate();
      QRLogger::log(CAT_GRP_LATTCE_INDX, LL_INFO,
        "Node marked invalid due to absence of key '%s' "
                  "in lattice index hash table", key.getSortName().data()); 
    }
  else
    keyBitmap_.addElement(keyIndex);
}
***/

void QRLatticeIndexNode::adopt(QRLatticeIndexNodePtr newChild,
                               Int32 newChildPos)
{
  QRTRACER("QRLatticeIndexNode::adopt()");
#ifdef DEBUG_LATTICE
  QRLogger::log(CAT_GRP_LATTCE_INDX, LL_DEBUG,
    "%s adopting %s", nodeName_, newChild->nodeName_);
#endif

  if (newChild == this)
  {
    QRLogger::log(CAT_GRP_LATTCE_INDX, LL_DEBUG,
                  "A node is attempting to adopt itself! Skipping.");
    return;
  }

  if (newChildPos != -1)
    children_.insertAt(newChildPos, newChild);
  else
    children_.insert(newChild);
  newChild->parents_.insert(this);
}

void QRLatticeIndexNode::removeMV(MVDetailsPtr mv)
{
  QRTRACER("QRLatticeIndexNode::removeMV()");
  // removeAll() will leak memory since NAshPtrList only overrides
  // removeAt(), which is not called by the NAList implementation of
  // removeAll() or removeCounted(). Besides, removeAll() goes into
  // an infinite loop if it deletes the only entry in the list.
  //MVs_.removeAll(mv);
  NABoolean found = FALSE;
  for (CollIndex i=0; i<mapList_.entries() && !found; i++)
    {
      if (mapList_[i]->getMVDetails() == mv)
        {
          mapList_.removeAt(i);
          found = TRUE;
        }
    }
}

NABoolean QRLatticeIndexNode::visited() const
{
  // no exception if not in traversal -- may want to see if node was hit
  // in most recent completed traversal.
  return (visitMarker_ == lattice_->visitSeqNum_);
}

void QRLatticeIndexNode::markVisited()
{
  assertLogAndThrow(CAT_GRP_LATTCE_INDX, LL_MVQR_FAIL,
                    lattice_->inTraversal_, QRLogicException,
    		    "Cannot mark node as visited when no traversal is in progress.");

  visitMarker_ = lattice_->visitSeqNum_;
}

void QRLatticeIndexNode::dumpNode(NAString& nodeContents,
                                  const LatticeKeyArray& keyArray) const
{
  QRTRACER("QRLatticeIndexNode::dumpNode()");
  // Append the node name, and then go through the bitmap, adding the key
  // value corresponding to any marked bits.
  NABoolean first = TRUE;
  char mvsText[50];

  sprintf(mvsText, "// %s - %d MVs: ", nodeName_, mapList_.entries());
  nodeContents.append(mvsText);

  for (CollIndex i=0; i<mapList_.entries(); i++)
  {
    if (first)
      first = FALSE;
    else
      nodeContents.append(", ");

    nodeContents.append(mapList_[i]->getMVDetails()->getMVName().data());
  }

  char gbText[50];
  sprintf(gbText, "\n//       GROUP BY %d columns: ", keyBitmap_.entries());
  nodeContents.append(gbText);
  first = TRUE;
  for (CollIndex j = 0; keyBitmap_.nextUsed(j); j++)
    {
      if (first)
          first = FALSE;
      else
        nodeContents.append(", ");

      // Don't call getReferencedElement here, because the ref may be from
      // an MV that has already been dropped.
      const NAString* key = keyArray[j];
      nodeContents.append(*key);
    }

  nodeContents.append("\n");
}

/**
 * This method can be used to dump detailed usage stats to the log file,
 * including MVMemo hash keys and LatticeIndex graphs. 
 * Currently not used.
 *****************************************************************************
 */
void QRLatticeIndexNode::dumpLattice(NAString& graphText,
                                     NAString& graphLabel)
{
  QRTRACER("QRLatticeIndexNode::dumpLattice()");
  if (visited())
    return;  // node already visited in this traversal

  markVisited();

  // Add the description of the node to the label to be used for the graph.
  if (!isTop_ && !isBottom_)  // omit fake nodes at top and bottom
  {
    dumpNode(graphLabel, lattice_->keyArr_);
    //graphLabel.append("\\l");  // causes left-justification in rendered graph
  }

  // For each child of the current node, write the specification of the link to it,
  // and then process the child recursively.
  NAPtrList<QRLatticeIndexNodePtr>& list = children_;
  for (CollIndex i = 0; i < list.entries(); i++)
  {
    QRLatticeIndexNodePtr child = list[i];
    (((graphText += getNodeName()) += " -> ") += child->getNodeName()) += ";\n";
    child->dumpLattice(graphText, graphLabel);
  }
}

void QRLatticeIndexNode::collectMVGroups(WorkloadAnalysisPtr workload, Int32 minQueriesPerMV, CollHeap* heap)
{
  QRTRACER("QRLatticeIndexNode::collectMVGroups()");
  if (visited())
    return;  // node already visited in this traversal
  markVisited();

  if (!isTop_ && !isBottom_)  // omit fake nodes at top and bottom
  {
    if (getMVs().entries() >= (UInt32)minQueriesPerMV)
    {
      // Propose an MV to cover the queries (MVs) in this node.
      ProposedMVPtr pmv = new (heap) ProposedMV(ADD_MEMCHECK_ARGS(heap));
      pmv->addMVs(getMVs());
      if (workload->addProposedMV(pmv) == FALSE)
      {
	deletePtr(pmv);
      }
    }
  }

  // Now recurse to the children.
  // Recursion ends at the bottom node that has no children.
  NAPtrList<QRLatticeIndexNodePtr>& list = children_;
  for (CollIndex i = 0; i < list.entries(); i++)
  {
    QRLatticeIndexNodePtr child = list[i];
    child->collectMVGroups(workload, minQueriesPerMV, heap);
  }
}

//
// QRLatticeIndex
//

const Int32 QRLatticeIndex::SEQNUM_UNASSIGNED = -1;
const char* const QRLatticeIndex::TOP_NODE_NAME = "Top";
const char* const QRLatticeIndex::BOTTOM_NODE_NAME = "Bottom";

static ULng32 ordinalsHashFn(const NAString& s){ return NAString::hash(s); }
static ULng32 exactSetsHashFn(const LatticeKeySubArray& bitmap) { return bitmap.hash(); }

QRLatticeIndex::QRLatticeIndex(CollHeap *heap,
                               ADD_MEMCHECK_ARGS_DEF(CollIndex maxNumKeys))
  : NAIntrusiveSharedPtrObject(ADD_MEMCHECK_ARGS_PASS(heap)),
    heap_(heap),
    maxNumKeys_(maxNumKeys),
    keyArr_(heap, maxNumKeys),
    ordinalsHash_(ordinalsHashFn, INIT_HASH_SIZE_SMALL, TRUE, heap),
    exactSetsHash_(exactSetsHashFn, INIT_HASH_SIZE_SMALL, TRUE, heap),
    seqNum_(0),
    inTraversal_(FALSE),
    visitSeqNum_(0)
{
  QRTRACER("QRLatticeIndex::QRLatticeIndex()");
  // Lattice initially consists of the fake top and bottom nodes. We don't
  // initialize them above because they require keyArr_ to be initialized first.
  // It would actually be initialized first, since its declaration in the class
  // definition precedes those of topNode_ and bottomNode_, but we choose not to
  // introduce an order dependency.
  topNode_ = new (heap) QRLatticeIndexNode(TOP_NODE_NAME, this, &keyArr_,
                                           ADD_MEMCHECK_ARGS(heap));
  bottomNode_ = new (heap) QRLatticeIndexNode(BOTTOM_NODE_NAME, this, &keyArr_,
                                              ADD_MEMCHECK_ARGS(heap));
  topNode_->adopt(bottomNode_);

  exactSetsHash_.insert(&bottomNode_->keyBitmap_, bottomNode_);
}

QRLatticeIndex::~QRLatticeIndex()
{
  QRTRACER("QRLatticeIndex::~QRLatticeIndex()");
  CollIndex maxEntries = keyArr_.entries();
  for (CollIndex i=0; i<maxEntries; i++)
    delete keyArr_[i];
  keyArr_.clear();
}

void QRLatticeIndex::addMV(LatticeKeyList& keys, QRJoinSubGraphMapPtr map)
{
  QRTRACER("QRLatticeIndex::addMV()");
  assertLogAndThrow(CAT_GRP_LATTCE_INDX, LL_MVQR_FAIL,
                    !inTraversal_, QRLogicException, 
		    "Cannot add a node to QRLatticeIndex when a traversal is in progress.");

  // Create the node and call insertNode.
  QRLatticeIndexNodePtr newNode =
       new (heap_) QRLatticeIndexNode(keys, this, ++seqNum_, FALSE,
                                      ADD_MEMCHECK_ARGS(heap_));

  // Look up the new node's bitmap in the exact sets hash table to see if a
  // node with the same set of keys already exists. If so, just add the MV to
  // that existing node rather than creating a duplicate.
  QRLatticeIndexNodePtr oldNode = exactSetsHash_.getFirstValue(&newNode->keyBitmap_);
  if (!oldNode)
    {
      newNode->addMV(map);
      insertNode(newNode, topNode_);
      exactSetsHash_.insert(&newNode->keyBitmap_, newNode);
    }
  else
    {
      oldNode->addMV(map);
      deletePtr(newNode);
    }
}

// Remove a node by removing all its links, then linking all its children to all
// its parents. The node is then removed from the exact sets hash and deleted.
void QRLatticeIndex::removeMV(LatticeKeyList& keys, MVDetailsPtr mvDetails)
{
  QRTRACER("QRLatticeIndex::removeMV()");
  QRLatticeIndexLock lock(this);  // ctor/dtor handles lock for traversal

  // Create the bitmap and initialize it using the passed keys. Pass TRUE to
  // addKeysToBitmap so nonexisting keys won't be added (their presence means
  // no node containing them exists in the lattice anyway).
  LatticeKeySubArray keyBitmap(getKeyArr(), heap_);
  NABoolean found = addKeysToBitmap(keys, TRUE, keyBitmap);

  // If any of the keys were unknown, then a node containing them doesn't exist.
  if (!found)
    return;

  // Search for a node with exactly the indicated keys, return if not found.
  QRLatticeIndexNodePtr nodeToRemove = exactSetsHash_.getFirstValue(&keyBitmap);
  if (!nodeToRemove)
    return;

  nodeToRemove->removeMV(mvDetails);
  if (nodeToRemove->getMVs().entries() > 0)
    return;

  // Don't remove the top and bottom nodes.
  if (nodeToRemove->isBottom_ || nodeToRemove->isTop_)
    return;

  // Get references to the children and parents of the target node.
  NAPtrList<QRLatticeIndexNodePtr>& childList = nodeToRemove->children_;
  NAPtrList<QRLatticeIndexNodePtr>& parentList = nodeToRemove->parents_;

  // Leaf node (parent of "bottom") is treated differently below when linking
  // parents to children.
  NABoolean nodeToRemoveIsLeaf = (childList.entries() == 1 &&
                                  childList[0]->isBottom_);
  NABoolean nodeToRemoveIsRoot = (parentList.entries() == 1 &&
                                  parentList[0]->isTop_);

  // Once the node has been removed from the lattice, each of its parents
  // will represent a minimal superset of each of its children. So link each
  // child to each of its current grandparents.
  CollIndex i, j;
  for (i = 0; i < childList.entries(); i++)
    {
      for (j = 0; j < parentList.entries(); j++)
        {
          // If the node to remove is a leaf (i.e., a parent of the "bottom"
          // node), any parent of the node that has other children should not
          // adopt the bottom node, because it is not a leaf even in the absence
          // of the removed node. Similarly, if the child of a root (child of
          // "top") has another parent, do not give it "top" as a parent.
          if (nodeToRemoveIsLeaf && parentList[j]->children_.entries() > 1)
            continue;
          if (nodeToRemoveIsRoot && childList[i]->parents_.entries() > 1)
            continue;
          parentList[j]->adopt(childList[i]);
        }
    }

  // Drop the links from the node to all its children, and from all its parents
  // to the node. Have to start at the end because removing a list element at the
  // front changes the index of the other items. We could just delete elem 0
  // on each loop iteration, but this would cause a bug if the list became an
  // array some day.
  Lng32 inx;   // Can't use CollIndex here because it is unsigned
  for (inx = childList.entries() - 1; inx >= 0; inx--)
    nodeToRemove->disown(childList[inx]);
  for (inx = parentList.entries() - 1; inx >= 0; inx--)
    parentList[inx]->disown(nodeToRemove);

  // Finally, get the removed node out of the hash table used for exact matching,
  // and delete it.
  exactSetsHash_.remove(&keyBitmap);
  deletePtr(nodeToRemove);
}

NABoolean QRLatticeIndex::addKeysToBitmap(const LatticeKeyList& keys,
                                          NABoolean noNewEntry,
                                          LatticeKeySubArray& keyBitmap)
{
  QRTRACER("QRLatticeIndex::addKeysToBitmap()");
  // Get current size of key array and associated bitmaps. Each addition of a
  // key could cause a resize of the key array and change this.
  CollIndex latestMaxNumKeys = getMaxNumKeys();

  NABoolean isOK = TRUE;
  for (CollIndex i=0; i<keys.entries(); i++)
    {
      // getKeyIndex() will resize the bitmaps for each node already in the
      // lattice if necessary, but we have to check here for each key added to
      // see if the index is out of bounds for the one we're instantiating now.
      LatticeIndexablePtr keyElement = keys[i];
      CollIndex keyInx = getKeyIndex(keyElement, noNewEntry);
      if (keyInx == NULL_COLL_INDEX)
        {
          // This should happen only if noNewEntry is true, which is the case when
          // a search node is being constructed. Don't return yet, continue so
          // any other absent keys will be noted in the log.
          isOK = FALSE;
          assertLogAndThrow1(CAT_GRP_LATTCE_INDX, LL_MVQR_FAIL,
                             noNewEntry, QRLogicException, 
                             "Node invalid due to absence of key '%s' in "
                             "lattice index hash table",
                             keyElement->data());
          QRLogger::log(CAT_GRP_LATTCE_INDX, LL_DEBUG,
            "Key not found: %s", keyElement->data());
        }
      else
        {
          if (keyInx >= latestMaxNumKeys) // getKeyIndex may have changed max num keys
            {
              latestMaxNumKeys = getMaxNumKeys();
              keyBitmap.resize(latestMaxNumKeys);
            }
          keyBitmap.addElementFast(keyInx);
        }
    }

  return isOK;
}

void QRLatticeIndex::findSubsets(QRLatticeIndexNode& supersetNode,
                                 NAPtrList<QRLatticeIndexNodePtr>& subsets)
{
  QRTRACER("QRLatticeIndex::findSubsets()");
  QRLatticeIndexLock lock(this);  // ctor/dtor handles lock for traversal
  QRLatticeIndexNodePtr exactNode = 
        exactSetsHash_.getFirstValue(&supersetNode.keyBitmap_);
  if (!exactNode)
    findSubsets_(supersetNode, bottomNode_, subsets); // start at bottom, go up
  else
    getSubsets(exactNode, subsets);
}

void QRLatticeIndex::findSubsets_(QRLatticeIndexNode& supersetNode,
                                  QRLatticeIndexNodePtr childNode,
                                  NAPtrList<QRLatticeIndexNodePtr>& subsets)
{
  QRTRACER("QRLatticeIndex::findSubsets_()");
  // Recurse on each parent of the current node that is a subset; if not a
  // subset, none of its ancestors will be either.
  const NAPtrList<QRLatticeIndexNodePtr>& searchList = childNode->getParents();
  QRLatticeIndexNodePtr currNode;
  for (CollIndex i=0; i<searchList.entries(); i++)
    {
      currNode = searchList[i];
      if (currNode->visited())
        continue;
      currNode->markVisited();
      if (currNode->isSubsetOf(supersetNode))
        {
          subsets.insert(currNode);
          findSubsets_(supersetNode, currNode, subsets);
        }
    }
}

void QRLatticeIndex::getSubsets(QRLatticeIndexNodePtr node,
                                NAPtrList<QRLatticeIndexNodePtr>& subsets)
{
  QRTRACER("QRLatticeIndex::getSubsets()");
  subsets.insert(node);

  const NAPtrList<QRLatticeIndexNodePtr>& immediateSubsets = node->getChildren();
  QRLatticeIndexNodePtr subsetNode;
  for (CollIndex i=0; i<immediateSubsets.entries(); i++)
    {
      subsetNode = immediateSubsets[i];
      if (subsetNode->isBottom_ || subsetNode->visited())
        continue;
      subsetNode->markVisited();
      getSubsets(subsetNode, subsets);
    }
}

void QRLatticeIndex::findSupersets(QRLatticeIndexNode& subsetNode,
                                   NAPtrList<QRLatticeIndexNodePtr>& supersets)
{
  QRTRACER("QRLatticeIndex::findSupersets()");
  QRLatticeIndexLock lock(this);  // ctor/dtor handles lock for traversal
  QRLatticeIndexNodePtr exactNode =
        exactSetsHash_.getFirstValue(&subsetNode.keyBitmap_);
  if (!exactNode)
    findSupersets_(subsetNode, topNode_, supersets); // start at top, go down
  else
    getSupersets(exactNode, supersets);
}

void QRLatticeIndex::findSupersets_(QRLatticeIndexNode& subsetNode,
                                    QRLatticeIndexNodePtr parentNode,
                                    NAPtrList<QRLatticeIndexNodePtr>& supersets)
{
  QRTRACER("QRLatticeIndex::findSupersets_()");
  // Recurse on each child of the current node that is a superset; if not a
  // superset, none of its descendants will be either.
  const NAPtrList<QRLatticeIndexNodePtr>& searchList = parentNode->getChildren();
  QRLatticeIndexNodePtr currNode;
  for (CollIndex i=0; i<searchList.entries(); i++)
    {
      currNode = searchList[i];
      if (currNode->visited())
        continue;
      currNode->markVisited();
      if (!currNode->isBottom_ && currNode->isSupersetOf(subsetNode))
        {
          supersets.insert(currNode);
          findSupersets_(subsetNode, currNode, supersets);
        }
    }
}

void QRLatticeIndex::getSupersets(QRLatticeIndexNodePtr node,
                                  NAPtrList<QRLatticeIndexNodePtr>& supersets)
{
  QRTRACER("QRLatticeIndex::getSupersets()");
  supersets.insert(node);

  const NAPtrList<QRLatticeIndexNodePtr>& immediateSupersets = node->getParents();
  QRLatticeIndexNodePtr supersetNode;
  for (CollIndex i=0; i<immediateSupersets.entries(); i++)
    {
      supersetNode = immediateSupersets[i];
      if (supersetNode->isTop_ || supersetNode->visited())
        continue;
      supersetNode->markVisited();
      getSupersets(supersetNode, supersets);
    }
}

void QRLatticeIndex::insertNode(QRLatticeIndexNodePtr nodeToInsert,
                                QRLatticeIndexNodePtr nodeToCheck)
{
  QRTRACER("QRLatticeIndex::insertNode()");
  NABoolean added = FALSE;
  NAPtrList<QRLatticeIndexNodePtr>& list = nodeToCheck->children_;
  QRLatticeIndexNodePtr currentChild;
  NABoolean nodeToInsertHasBeenAdopted = FALSE;
  CollIndex skip = NULL_COLL_INDEX;
  for (CollIndex i = 0; i < list.entries(); i++)
    {
      currentChild = list[i];

      // Not catching this bug here may cause an infinite recursion.
      assertLogAndThrow(CAT_GRP_LATTCE_INDX, LL_MVQR_FAIL,
                        currentChild != nodeToCheck, QRLogicException, 
                        "LatticeIndex node pointing to itself! Aborting.");

      // If already a child of this node, nothing else to do in this branch.
      // However, we can only exit the current level of recursion, because the
      // node may have been added in this same root call, and a node on another
      // branch may need to adopt it. An example of this scenario is creating a
      // lattice with nodes in this order: {1,2,3,4}, {1,2,3,5}, {1,2,6},
      // {1,2,3}, {1,2}.
      if (nodeToInsert->isEqual(*currentChild))
        return;

      if (currentChild->isBottom_ || nodeToInsert->isSupersetOf(*currentChild))
        {
          // Insert nodeToInsert between nodeToCheck and currentChild. Insert the
          // node at the same position in the child list as the disowned node, to
          // prevent visiting it again and concluding that we are done (see exit
          // check above). The inserted node may represent a superset of more
          // than one of nodeToCheck's children. If so, make sure it is only
          // adopted once, or it will appear more than once in its child list.
          nodeToCheck->disown(currentChild);
          if (!nodeToInsertHasBeenAdopted)
            {
              nodeToCheck->adopt(nodeToInsert, i);
              nodeToInsertHasBeenAdopted = TRUE;
              skip = i;  // skip the node in the search for its subclasses
            };

          // If currentChild_ is the (artificial) bottom node, make sure the node
          // being inserted is a leaf node, because only leaves adopt the bottom
          // node. Otherwise, just make sure currentChild_ is not already a child
          // of the node being inserted.
          if (currentChild->isBottom_)
            {
              if (nodeToInsert->children_.isEmpty())
                nodeToInsert->adopt(currentChild);
            }
          else if (!nodeToInsert->children_.contains(currentChild))
            nodeToInsert->adopt(currentChild);
              
          added = TRUE;
        }
      else if (nodeToInsert->isSubsetOf(*currentChild))
        {
          insertNode(nodeToInsert, currentChild);
          added = TRUE;
        }
    }

  // If not a subset or superset of any node at this level, add it to the
  // list of the current node's children.
  if (!added)
    {
      nodeToCheck->adopt(nodeToInsert);
      nodeToInsertHasBeenAdopted = TRUE;
      skip = list.entries() - 1;
    }

  // The invariant by which the lattice structure is maintained guarantees that
  // nodeToInsert will not be both a superset of one of nodeToCheck's children
  // and a subset of another one of its children. Therefore, we will find no
  // additional supersets of nodeToInsert in this sublattice.

  // If the inserted node was adopted by nodeToCheck, then any additional maximal
  // subsets will be found within the sublattice rooted by nodeToCheck -- any
  // subset of a node are also subsets of that node's parent.
  if (nodeToInsertHasBeenAdopted)
    {
      assertLogAndThrow(CAT_GRP_LATTCE_INDX, LL_MVQR_FAIL,
                        skip != NULL_COLL_INDEX, QRLogicException, 
			"No branch skipped for subset lookup");
      { // Enter new block to restrict the scope of the lock.
        QRLatticeIndexLock lock(this);  // ctor/dtor handles lock for traversal
        for (CollIndex i = 0; i < list.entries(); i++)
          {
            if (i == skip)
              continue;
            currentChild = list[i];
            findAndAttachSubset(nodeToInsert, currentChild);
          }
      }

      // If the inserted node has no subsets, make its only child the bottom
      // node.
      if (nodeToInsert->children_.isEmpty())
        nodeToInsert->adopt(bottomNode_);
    }
}

void QRLatticeIndex::findAndAttachSubset(QRLatticeIndexNodePtr insertedNode,
                                         QRLatticeIndexNodePtr searchRoot)
{
  QRTRACER("QRLatticeIndex::findAndAttachSubset()");
  if (searchRoot->visited())
    return;
  searchRoot->markVisited();

  NAPtrList<QRLatticeIndexNodePtr>& list = searchRoot->children_;
  QRLatticeIndexNodePtr currentChild;
  for (CollIndex i = 0; i < list.entries(); i++)
    {
      currentChild = list[i];
      if (currentChild->visited())
        continue;
      currentChild->markVisited();

      // Have to see if the subset node is already in the child list; even if
      // unmarked, it may have been visited in a search initiated from a
      // different node.
      if (currentChild->isSubsetOf(*insertedNode) &&
          !insertedNode->children_.contains(currentChild))
        insertedNode->adopt(currentChild);
      else
        findAndAttachSubset(insertedNode, currentChild);
    }
}

// All bitmaps in a lattice index have to be the same size, or &, |, etc.
// will throw an exception.
// @ZX -- above comment was for RW bitmap -- may not apply with NASubArray.
void QRLatticeIndex::resizeKeyBitmaps()
{
  QRTRACER("QRLatticeIndex::resizeKeyBitmaps()");
  QRLatticeIndexLock lock(this);     // ctor/dtor handles lock for traversal
  resizeKeyBitmaps(topNode_);
}

void QRLatticeIndex::resizeKeyBitmaps(QRLatticeIndexNodePtr node)
{
  QRTRACER("QRLatticeIndex::resizeKeyBitmaps()");
  if (node->visited())
    return;
  node->markVisited();

  node->keyBitmap_.resize(maxNumKeys_);
  NAPtrList<QRLatticeIndexNodePtr>& children = node->children_;
  for (CollIndex i = 0; i < children.entries(); i++)
    resizeKeyBitmaps(children[i]);
}


CollIndex QRLatticeIndex::getKeyIndex(LatticeIndexablePtr key, NABoolean noNewEntry)
{
  CollIndex *ordinal = ordinalsHash_.getFirstValue(key);
  if (!ordinal)
  {
    if (noNewEntry)
    {
#ifdef DEBUG_LATTICE
      QRLogger::log(CAT_GRP_LATTCE_INDX, LL_DEBUG,
                    "Key '%s' not added to lattice index hash "
                    "table due to noNewEntry==TRUE", key->data());
#endif
      return NULL_COLL_INDEX;
    }
    ordinal = new (heap_) CollIndex;
    *ordinal = keyArr_.getUsedLength();
    // Check size before inserting, to avoid resizing by 1 every time.
    if (*ordinal >= maxNumKeys_)
    {
      // Got to limit, resize array.
      assertLogAndThrow2(CAT_GRP_LATTCE_INDX, LL_MVQR_FAIL,
                maxNumKeys_ == keyArr_.getUsedLength(), QRLogicException, 
                "Actual length of lattice key array of %d exceeds the maximum length of %s", 
                  keyArr_.getUsedLength(), maxNumKeys_);

      maxNumKeys_ = (CollIndex)((maxNumKeys_ > 20 ? maxNumKeys_ : 20) * 1.5);
      keyArr_.resize(maxNumKeys_);
      resizeKeyBitmaps();
    }

    keyArr_.insertAt(*ordinal, key);
    ordinalsHash_.insert(key, ordinal);
  }

  return *ordinal;
}

NABoolean QRLatticeIndex::contains(LatticeIndexablePtr key)
{
  return ordinalsHash_.contains(key);
}

/**
 * This method can be used to dump detailed usage stats to the log file,
 * including MVMemo hash keys and LatticeIndex graphs. 
 * Currently not used.
 *****************************************************************************
 */
void QRLatticeIndex::dumpLattice(NAString& graphText, const char* tag)
{
  QRTRACER("QRLatticeIndex::dumpLattice()");
  QRLatticeIndexLock lock(this);  // ctor/dtor handles lock for traversal
  NAString graphLabel(heap_);

  // The textual lattice specification is delimited by #STARTLATTICE and
  // #ENDLATTICE lines so sed can be used to extract it from the log.
  (graphText += "#STARTLATTICE ") += tag;
  graphText += "\ndigraph G { Top[shape=invtriangle]; Bottom[shape=triangle];\n";

  // Get the output for each node, and the label (which includes info from each
  // node).
  topNode_->dumpLattice(graphText, graphLabel);

  // Now write the closing part, including the label.
  //((graphText += "graph[labeljust=l, label=\"") += graphLabel.data()) += "\"];\n";
  graphText += graphLabel.data();
  (graphText += "}\n#ENDLATTICE ") += tag;
  graphText += '\n';
}

void QRLatticeIndex::collectMVGroups(WorkloadAnalysisPtr workload, Int32 minQueriesPerMV, CollHeap* heap)
{
  QRTRACER("QRLatticeIndex::collectMVGroups()");
  QRLatticeIndexLock lock(this);  // ctor/dtor handles lock for traversal

  // Start from the top.
  topNode_->collectMVGroups(workload, minQueriesPerMV, heap);
}
