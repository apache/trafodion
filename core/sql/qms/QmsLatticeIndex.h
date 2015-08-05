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

#ifndef _QRLATTICEINDEX_H_
#define _QRLATTICEINDEX_H_

#include "NAString.h"
#include "QRSharedPtr.h"
#include "QRLogger.h"
#include "Collections.h"
#include "SharedPtrCollections.h"

/**
 * \file
 * Includes all classes used in the implementation of the lattice index, which
 * is used for fast searching based on subset/superset relationships. The
 * principal classes are QRLatticeIndex, representing the lattice itself, and
 * QRLatticeIndexNode, representing an individual node in the lattice.
 */

typedef NAString    LatticeIndexable;
typedef NAString*   LatticeIndexablePtr;

class QRLatticeIndex;
class QRLatticeIndexNode;
class   QRLatticeIndexSearchNode;
class QRLatticeIndexLock;

typedef NAArray<LatticeIndexablePtr>    LatticeKeyArray;
typedef NASubArray<LatticeIndexablePtr> LatticeKeySubArray;

#ifdef _MEMSHAREDPTR
typedef QRIntrusiveSharedPtr<QRLatticeIndexNode> QRLatticeIndexNodePtr;
typedef QRIntrusiveSharedPtr<QRLatticeIndex>     QRLatticeIndexPtr;
typedef SharedPtrValueHash<LatticeKeySubArray, QRLatticeIndexNode> ExactSetsHash;
#else
typedef QRLatticeIndexNode* QRLatticeIndexNodePtr;
typedef QRLatticeIndex*	    QRLatticeIndexPtr;
typedef NAHashDictionary<LatticeKeySubArray, QRLatticeIndexNode> ExactSetsHash;
#endif

typedef NAList<LatticeIndexablePtr>     LatticeKeyList;

const Int32 SEQNUM_UNASSIGNED = -1;

#include "QmsMVDetails.h"
#include "QmsWorkloadAnalysis.h"

/**
 * A node in a lattice index. Each node represents a set of keys (currently
 * strings), and is positioned within the lattice according to its subset/superset
 * relation to other nodes. The exceptions to this are two artificial nodes
 * that serve only as the "top" and "bottom" of the lattice. The top node is the
 * only node with no parents, and serves as the starting point for recursive
 * downward traversals of the lattice. The bottom node is the only node with no
 * children, and is the starting point for upward traversals. Regular nodes
 * consist of a name (used only to label the node in visual representations of
 * the lattice), a bitmap representing the node's set of keys, and lists of
 * children and parents.
 * \n\n
 * This class is essentially owned by the friend class QRLatticeIndex, which
 * constructs nodes and maintains the parent/child relationships between them.
 * Most of the private member functions are used only by %QRLatticeIndex.
 */
class QRLatticeIndexNode : public NAIntrusiveSharedPtrObject
{
  // Allow lattice to manipulate children and parents of nodes, which defines
  // the structure of the lattice.
  friend class QRLatticeIndex;

  public:
    virtual ~QRLatticeIndexNode()
      {}

    /**
     * Determines if this node is equal to another one.
     *
     * @param other The node to compare to this one.
     * @return 1 if the two nodes are equal, otherwise 0.
     */
    Int32 operator==(const QRLatticeIndexNode& other)
      {
        return keyBitmap_ == other.keyBitmap_;
      }

    /**
     * Determines if this node is unequal to another one.
     *
     * @param other The node to compare to this one.
     * @return 1 if the two nodes are not equal, otherwise 0.
     */
    Int32 operator!=(const QRLatticeIndexNode& other)
      {
        return !(keyBitmap_ == other.keyBitmap_);
      }

    /**
     * Computes the bitwise difference between this node and the other node.
     * The result must be deleted by the caller.
     *
     * @param other The node to compare to this one.
     * @param heap The heap to allocate the result from.
     * @return A newly allocated LatticeKeySubArray with the result bitmap.
     */
    LatticeKeySubArray* computeDiff(const QRLatticeIndexNode& other, CollHeap* heap)
      {
	// Allocate the result bitmap.
	LatticeKeySubArray *result = new(heap) LatticeKeySubArray(keyBitmap_);
	// Compute the difference
	*result -= other.keyBitmap_;
        return result;
      }

    const LatticeKeySubArray& getKeyBitmap() const
      {
        return keyBitmap_;
      }

    /**
     * Determines if this node is equal to another one (equivalent to
     * operator==).
     *
     * @param other The node to compare to this one.
     * @return 1 if the two nodes are equal, otherwise 0.
     */
    NABoolean isEqual(const QRLatticeIndexNode& other)
      {
        return *this == other;
      }

    /**
     * Determines if the set of keys contained in this node is a superset of
     * those contained by the other node.
     *
     * @param other The possible subset node.
     * @return \c TRUE if this node is a superset of \c other, otherwise \c FALSE.
     */
    NABoolean isSupersetOf(const QRLatticeIndexNode& other) const
      {
        return keyBitmap_.contains(other.keyBitmap_);
      }

    /**
     * Determines if the set of keys contained in this node is a subset of
     * those contained by the other node.
     *
     * @param other The possible superset node.
     * @return \c TRUE if this node is a subset of \c other, otherwise \c FALSE.
     */
    NABoolean isSubsetOf(const QRLatticeIndexNode& other) const
      {
        return other.isSupersetOf(*this);
      }

    /**
     * Returns a reference to the list of MVs associated with this node.
     * @return Reference to list of pointers to MVDetails.
     */
    const SubGraphMapList& getMVs() const
      {
        return mapList_;
      }

    /**
     * Returns the list of nodes that are the parents of this node. Each parent
     * node represents a minimal superset the child, meaning there is no other
     * node in the lattice that is a superset of the child and a subset of the
     * parent.
     *
     * @return List of this node's parents.
     */
    const NAPtrList<QRLatticeIndexNodePtr>& getParents() const
      {
        return parents_;
      }

    /**
     * Returns the list of nodes that are the children of this node. Each child
     * node represents a maximal subset the parent, meaning there is no other
     * node in the lattice that is a superset of the child and a subset of the
     * parent.
     *
     * @return List of this node's children.
     */
    const NAPtrList<QRLatticeIndexNodePtr>& getChildren() const
      {
        return children_;
      }

    /**
     * Indicates whether this node is part of a lattice, as opposed to a node
     * used merely to search the lattice. The QRLatticeIndexSearchNode class
     * overrides this to return \c FALSE.
     *
     * @return \c TRUE unless overridden by a subclass implementation.
     */
    virtual NABoolean isLatticeNode() const
      {
        return TRUE;
      }

    /**
     * Returns the name assigned to this node. The name is used to label the
     * node in the graphviz specification of the lattice, which is used to
     * render a visual representation of the lattice.
     *
     * @return Name associated with the node.
     */
    const char* getNodeName() const
      {
        return nodeName_;
      }

    /**
     * Indicates whether the node is valid. Currently, only a search node can be
     * invalid. This can happen if it is created under the restriction that it
     * adds no new keys to the hash table of the lattice.
     *
     * @return \c TRUE if the node is valid, otherwise \c FALSE.
     */
    NABoolean isValid() const
      {
        return isValid_;
      }

    /**
     * Appends a description of the node to the \c nodeContents string. The
     * description consists of the node's name, and a list of the key values
     * contained in the node. \c nodeContents is appended to; any existing
     * text in the string is preserved.
     *
     * @param nodeContents String to append node description to.
     * @param keyArray Key values contained in the node.
     */
    void dumpNode(NAString& nodeContents, const LatticeKeyArray& keyArray) const;

    /**
     * Indicates whether the node has been visited in the current or most
     * recent traversal. When a node is visited while traversing the lattice,
     * it is marked with the value of the sequence number associated with
     * that traversal. By comparing the node's mark value with the current
     * traversal sequence number of the lattice, we can tell whether the
     * current (or most recent) traversal has visited the node.
     *
     * @param node Node to test for a prior visit.
     * @return \c TRUE if the node has been visited, otherwise \c FALSE.
     * @see #markVisited
     * @see QRLatticeIndexLock
     */
    NABoolean visited() const;

    /**
     * Marks the node as having been visited by the current traversal.

     * @param node The node to mark as visited.
     * @see #visited
     * @see QRLatticeIndexLock
     */
    void markVisited();

    /**
     * Writes a specification for the visual rendering of the lattice index,
     * using the DOT language. This specification can be used as input to DOT
     * to create a gif file of the lattice. The DOT text is immediately preceded
     * by a line containing "#STARTLATTICE" with the value of \c tag appended,
     * and immediately followed by a line containing "#ENDLATTICE" with the same
     * tag appended. This facilitates extracting the graph specification using
     * sed or another tool.
     *
     * @param [out] graphText Specification of the lattice in DOT syntax.
     * @param [out] graphLabel Caption for the graph, separately built and then
     *                         appended to graphText.
     * @param tag Text appended to the lines preceding and following the
     *            DOT text.
     */
    void dumpLattice(NAString&  graphText,
                     NAString&  graphLabel);

   /**
    * Collect data on query (MV) groups with shared join+GroupBy.
    */
    void collectMVGroups(WorkloadAnalysisPtr workload, Int32 minQueriesPerMV, CollHeap* heap);

  protected:
    /**
     * Creates a node with the given set of keys to be placed in the given lattice.
     * We require all keys to be present when the node is instantiated, because the
     * key set determines placement within the lattice when the node is added to
     * it. Currently, this ctor is only called from the friend class QRLatticeIndex,
     * which creates the node and adds it, and by the ctor of its subclass,
     * QRLatticeIndexSearchNode.
     *
     * @param keys List of keys to be represented by the new node.
     * @param lattice The lattice this node will belong to.
     * @param referenceNumber Sequential number used to generate the node name.
     * @param noNewKeys If \c TRUE, disallow using a key not already present in
     *                  the lattice, and mark the node invalid if it has such a key.
     * @param heap Heap to use for any memory allocations.
     */
    QRLatticeIndexNode(const LatticeKeyList& keys,
                       QRLatticeIndexPtr lattice,
                       Int32 referenceNumber = SEQNUM_UNASSIGNED,
                       NABoolean noNewEntry = FALSE,
                       ADD_MEMCHECK_ARGS_DECL(CollHeap* heap = NULL));

    /**
     * Creates a node with no initial keys. This is used only by the constructor
     * for the QRLatticeIndexSearchNode subclass, which allows keys to be added
     * after the object is created. This is not possible for the base class,
     * since nodes that are part of the lattice are created at the same time they
     * are inserted into the lattice, and all keys must be present to find the
     * correct location for the node in the lattice.
     *
     * @param lattice The lattice the search node will be used on.
     * @param noNewKeys If \c TRUE, disallow using a key not already present in
     *                  the lattice, and mark the node invalid if it has such a key.
     * @param heap Heap to use for any memory allocations.
     */
    QRLatticeIndexNode(QRLatticeIndexPtr lattice,
                       NABoolean noNewEntry = FALSE,
                       ADD_MEMCHECK_ARGS_DECL(CollHeap* heap = NULL));

    /**
     * Adds a key to the search node. An exception is thrown if this is a
     * lattice node rather than a search node.
     *
     * @param key The key to add to the search node.
     * @param lattice Lattice the node will be used to search.
     */
    //void addKeyToSearchNode(LatticeIndexable& key, QRLatticeIndexPtr lattice);

    NABoolean hasKey(CollIndex i) const
      {
        return keyBitmap_.testBit(i);
      }

    /**
     * Marks the node as invalid. An invalid node may not be part of a lattice
     * index or be used as a search node against one.
     */
    void invalidate()
      {
        isValid_ = FALSE;
      }

  private:
    // Copy construction/assignment not defined.
    QRLatticeIndexNode(const QRLatticeIndexNode&);
    QRLatticeIndexNode& operator=(const QRLatticeIndexNode&);

    /**
     * Creates one of the special, artifical nodes used as the top node or
     * bottom node in the lattice.
     *
     * @param name Name to be assigned to the node.
     * @param keyArray Array of keys used by the lattice.
     * @param heap Heap to use for any memory allocations.
     */
    QRLatticeIndexNode(const char* const name,
                       QRLatticeIndexPtr lattice,
                       LatticeKeyArray* keyArray,
                       ADD_MEMCHECK_ARGS_DECL(CollHeap* heap = NULL));

    /**
     * Adds \c newChild to the list of children of this node, and adds this
     * node to the parent list for \c newChild. The existing children and
     * parents of \c newChild are not disturbed. This function does not check
     * whether the new child node is already a child of this node.
     *
     * @param newChild The node to make a child of this one.
     * @param newChildPos Position of new child in the list (0-based). If -1,
     *                    just place the new child at the end of the list.
     */
    void adopt(QRLatticeIndexNodePtr newChild, Int32 newChildPos = -1);

    /**
     * Removes \c disownedChild from the list of this node's children, and
     * removes this node from the parent list of \c disownedChild. The other
     * parents and existing children of the disowned child are not disturbed.
     *
     * @param disownedChild The node to be removed from the child list of this
     *                      node.
     */
    void disown(QRLatticeIndexNodePtr disownedChild)
      {
        children_.remove(disownedChild);
        disownedChild->parents_.remove(this);
      }

    /**
     * Adds \c mv to the list of MVs associated with this node.
     * @param mv The MV to be added to the list.
     */
    void addMV(QRJoinSubGraphMapPtr map)
      {
        mapList_.insert(map);
      }

    /**
     * Removes \c mv from the list of MVs associated with this node.
     * @param mv The MV to be removed from the list.
     */
    void removeMV(MVDetailsPtr mv);


private:
    /**
     * The lattice this node will be used to search. It is needed for calls to
     * #addSearchKey.
     */
    QRLatticeIndexPtr lattice_;

    /** List of parent nodes of this node. */
    NAPtrList<QRLatticeIndexNodePtr> parents_;

    /** List of child nodes of this node. */
    NAPtrList<QRLatticeIndexNodePtr> children_;

    /** List of MVs associated with by this node. */
    SubGraphMapList mapList_;

    /**
     * TRUE if this is the special node that is the child of all leaf regular
     * nodes.
     */
    NABoolean isTop_;

    /**
     * TRUE if this is the special node that is the parent of all root regular
     * nodes.
     */
    NABoolean isBottom_;

    /** TRUE if the node is valid. */
    NABoolean isValid_;

    /** Name of the node, used for labeling it in visual representations. */
    char nodeName_[11];

    /** Bitmap with bits set corresponding to the keys contained by this node. */
    LatticeKeySubArray keyBitmap_;

    /** 
     * Used to indicate that the node has been visited in a particular traversal.
     * Each new traversal gets an identifying number that it uses to mark nodes
     * it has seen.
     */
    UInt32 visitMarker_;

    /** Indicator of whether the node may contain a key not already in the hash table. */
    NABoolean noNewEntry_;

    NAMemory* heap_;
}; // QRLatticeIndexNode


/**
 * A node used for searching the lattice index. This class differs from its
 * parent class (QRLatticeIndexNode) only in that it has a public constructor,
 * allowing instances to be created without being part of a lattice index, and
 * keys may be added to the node after creation.
 */
class QRLatticeIndexSearchNode : public QRLatticeIndexNode
{
  public:
    /**
     * Creates a node used for searching a particular lattice. The node is tied
     * to a single lattice, because the bitmap is dependent on the set of keys
     * used in the lattice, and their order.
     *
     * @param keys The keys that form the search set for this node. The
     *             appropriate bits in the node's bitmap will be set to
     *             represent these.
     * @param lattice The lattice this node will be used to search.
     * @param noNewKeys If \c TRUE, disallow using a key not already present in
     *                  the lattice, and mark the node invalid if it has such a key.
     * @param heap Heap to use for any memory allocations.
     */
    QRLatticeIndexSearchNode(const LatticeKeyList& keys,
                             QRLatticeIndexPtr lattice,
                             NABoolean noNewKeys = FALSE,
                             ADD_MEMCHECK_ARGS_DECL(CollHeap* heap = NULL))
      : QRLatticeIndexNode(keys, lattice, SEQNUM_UNASSIGNED, noNewKeys,
                           ADD_MEMCHECK_ARGS_PASS(heap))
      {}

    /**
     * Creates a search node without an initial set of keys. The keys can be
     * added later using #addSearchKey.
     *
     * @param lattice The lattice this node will be used to search.
     * @param noNewKeys If \c TRUE, disallow using a key not already present in
     *                  the lattice, and mark the node invalid if it has such a key.
     * @param heap Heap to use for any memory allocations.
     */
    QRLatticeIndexSearchNode(QRLatticeIndexPtr lattice,
                             NABoolean noNewKeys = FALSE,
                             ADD_MEMCHECK_ARGS_DECL(CollHeap* heap = NULL))
      : QRLatticeIndexNode(lattice, noNewKeys, ADD_MEMCHECK_ARGS_PASS(heap))
      {}

    virtual ~QRLatticeIndexSearchNode()
      {}

    /**
     * Adds a search key to the node. Search keys can be supplied in the ctor,
     * after construction, or both.
     *
     * @param key The key to add.
     */
    /***
    This method is not called from anywhere
    void addSearchKey(LatticeIndexable& key)
      {
        // Call the protected member function of QRLatticeIndexNode.
        addKeyToSearchNode(key, lattice_);
      }
      ***/

    /**
     * Returns FALSE to indicate that this is an external node used for
     * searching, rather than a node in the lattice.
     *
     * @return FALSE
     */
    virtual NABoolean isLatticeNode() const
      {
        return FALSE;
      }

}; // QRLatticeIndexSearchNode


/**
 * Class representing a lattice index, a directed, acyclic graph that allows
 * fast searching for subsets or supersets of a set of keys. Each node in the
 * lattice contains a set of keys (strings). The organizing property of the
 * lattice is that for a given node \e n, each adjacent node attached by a
 * parent/child link contains a set of keys that is a <i>minimal superset/
 * maximal subset</i> \e n (a set \e s1 is a minimal superset of set \e s2 if
 * there is no other set in the lattice that is both a superset of \e s2 and
 * a subset of \e s1).
 * \n\n
 * The set of keys contained by a node are encoded in a bitmap, allowing subset
 * and superset determinations to be made using logical operators rather than
 * string matching. All possible keys for a lattice are stored in a dense
 * array, and the index of a given key entry in this array serves as the
 * corresponding bit in the bitmap. If bit \e i is on, it indicates that the
 * key at index \e i in the array is part of the set of keys represented by the
 * node containing the bitmap.
 * \n\n
 * A lattice index will contain one or more nodes with no superset nodes, and
 * one or more nodes with no subset nodes (usually >1 of each). To facilitate
 * node-based recursive traversals of the lattice in either direction, two
 * artificial (i.e., not representing a set of keys) nodes are part of each
 * lattice. The "top" node is the parent of all nodes with no supersets, and
 * the "bottom" node is the child of all nodes with no subsets.
 * \n\n
 * Since it is a graph, nodes in the lattice may be reached multiple times in
 * the same traversal. To avoid duplicate results while searching, visited nodes
 * are marked, and hitting a marked node cuts the traversal at that point.
 * Since visited nodes are marked rather than remembered (e.g., stored in a
 * separate list), only one traversal can be in progress at once. This is
 * ensured by obtaining a sort of lock on the lattice when initiating a traversal.
 * See the QRLatticeIndexLock for the details of how this is done. Each time
 * the lattice is traversed, a sequentially generated value is used as the value
 * of the mark indicating a prior visit to a node. This avoids the need to reset
 * the marks after each traversal.
 */
class QRLatticeIndex : public NAIntrusiveSharedPtrObject
{
  friend class QRLatticeIndexNode;

  public:
    static const Int32 SEQNUM_UNASSIGNED;
    static const char* const TOP_NODE_NAME;
    static const char* const BOTTOM_NODE_NAME;

    /**
     * Creates a lattice with the given heap and maximum number of keys.
     *
     * @param *heap Heap to use for any memory allocations.
     * @param maxNumKeys The maximum number of keys the lattice will use. This
     *                   will be resized on demand.
     */
    QRLatticeIndex(CollHeap *heap = NULL,
                   ADD_MEMCHECK_ARGS_DECL(CollIndex maxNumKeys = 128));

    virtual ~QRLatticeIndex();

    /**
     * Adds an MV to a node in the lattice, creating a new node if necessary.
     * A node containing each key in \c keys is searched for in the lattice,
     * and if found, the MV is added to it. If no such node is found, a node
     * is created from the key list, initialized to hold the MV, and inserted
     * into the lattice according to the organizing principle of the lattice
     * index (based on subset/superset relationships).
     *
     * @param keys List of keys to be contained by the node to which the MV is
     *             added.
     * @param mvDetails Details object for MV associated to be added.
     */
    void addMV(LatticeKeyList& keys, QRJoinSubGraphMapPtr map);

    /**
     * Removes an MV from a node in the lattice. If the removed MV was the last
     * one contained by the node, the node is removed from the lattice. If the
     * node is removed, each child of the removed node becomes a child of each
     * parent of the removed node.
     *
     * @param keys List of keys contained in the node that would contain the MV
     *             to be removed.
     * @param mvDetails Details object for MV to be removed from the node.
     */
    void removeMV(LatticeKeyList& keys, MVDetailsPtr mvDetails);

    /**
     * Sets the entries in \c keyBitmap corresponding to the keys in \c keys.
     *
     * @param keys List of keys.
     * @param noNewEntry If \c TRUE, do not create a new entry for the key in
     *                   the lattice's key array. This is used when called by
     *                   the constructor for a search node.
     * @param keyBitmap The bitmap to set bits in for the keys.
     * @return 
     */
    NABoolean addKeysToBitmap(const LatticeKeyList& keys,
                              NABoolean noNewEntry,
                              LatticeKeySubArray& keyBitmap);

    /**
     * Initiates a traversal of the lattice. There must not be a traversal
     * already in progress. A sequence number is generated which is used to
     * mark visited nodes and prevent multiple visits.
     *
     * @see QRLatticeIndexLock
     */
    void startTraversal()
      {
	assertLogAndThrow(CAT_GRP_LATTCE_INDX, LL_MVQR_FAIL, 
                          !inTraversal_, QRLogicException,
			  "Only one traversal of QRLatticeIndex may be in progress at a given time.");
        visitSeqNum_++;
        inTraversal_ = TRUE;
      }

    /**
     * Effectively ends a traversal by resetting a flag indicating that one is
     * in progress.
     *
     * @see QRLatticeIndexLock
     */
    void endTraversal()
      {
        inTraversal_ = FALSE;
      }

    /**
     * Finds all nodes within the lattice representing proper subsets of a
     * search node. The subset nodes are added to the \c subsets list.
     * Previously existing nodes in this list are not removed.
     *
     * @param supersetNode The node to search for subsets of.
     * @param subsets The list to which any found subset nodes are added.
     */
    void findSubsets(QRLatticeIndexNode& supersetNode,
                     NAPtrList<QRLatticeIndexNodePtr>& subsets);

    /**
     * Finds all nodes within the lattice representing proper supersets of a
     * search node. The superset nodes are added to the \c supersets list.
     * Previously existing nodes in this list are not removed.
     *
     * @param subsetNode The node to search for supersets of.
     * @param supersets The list to which any found superset nodes are added.
     */
    void findSupersets(QRLatticeIndexNode& subsetNode,
                       NAPtrList<QRLatticeIndexNodePtr>& supersets);

    /**
     * Returns the maximum number of keys the lattice can currently accommodate.
     * This number is automatically increased on demand.
     *
     * @return Number of keys the lattice can currently handle.
     */
    CollIndex getMaxNumKeys() const
      {
        return maxNumKeys_;
      }

    /**
     * Hashes the key to get its index in the key array. If the key is not
     * found, it is added to the end of the key array (and the hash table).
     * The key array is resized if necessary.
     *
     * @param key The key value to find the index of.
     * @param noNewEntry Boolean indicating if the key may be added to the hash
     *                   table. Search nodes looking for supersets might not
     *                   allow this, marking the node invalid instead and relying
     *                   on the absence of the key in the hash table to demonstrate
     *                   that the lattice holds no superset of it.
     * @return The index of the key in the key array, or \c NULL_COLL_INDEX if
     *         it was not added because of \c noNewEntry.
     */
    CollIndex getKeyIndex(LatticeIndexablePtr element, NABoolean noNewEntry = FALSE);

    /**
     * Adds \c key to the key array, and returns its index.
     *
     * @param key The key to add.
     * @param noNewEntry Boolean indicating if the key may be added to the hash
     *                   table. Search nodes looking for supersets might not
     *                   allow this, marking the node invalid instead and relying
     *                   on the absence of the key in the hash table demonstrates
     *                   that the lattice holds no superset of it.
     * @return The index of the added key in the key array.
     */
    CollIndex addKey(LatticeIndexable *key, NABoolean noNewEntry = FALSE)
      {
        return getKeyIndex(key, noNewEntry);  // adds key if not found
      }

    /**
     * Returns a reference to the artificial root node of the lattice.
     *
     * @return Reference to the top node of the lattice.
     */
    const QRLatticeIndexNode& getTopNode() const
      {
        return *topNode_;
      }

    /**
     * Returns a reference to the artificial bottom-most node of the lattice.
     *
     * @return Reference to the bottom node of the lattice.
     */
    const QRLatticeIndexNode& getBottomNode() const
      {
        return *bottomNode_;
      }

    LatticeKeyArray* getKeyArr()
      {
        return &keyArr_;
      }

    /**
     * Initiates the traversal of the lattice to build a specification of it
     * using the DOT language, from which a visual representation can be rendered.
     * This graph will show directed edges in the direction of parent to child.
     *
     * @param graphText Specification of the lattice in DOT syntax.
     * @param tag Text appended to the lines preceding and following the
     *            DOT text.
     * @see dumpLattice(NAString&, NAString&, QRLatticeIndexNodePtr)
     */
    void dumpLattice(NAString& graphText, const char* tag = "");

   /**
    * Collect data on query (MV) groups with shared join+GroupBy.
    */
    void collectMVGroups(WorkloadAnalysisPtr workload, Int32 minQueriesPerMV, CollHeap* heap);

    NABoolean contains(LatticeIndexablePtr key);

  private:
    // Copy construction/assignment not defined.
    QRLatticeIndex(const QRLatticeIndex&);
    QRLatticeIndex& operator=(const QRLatticeIndex&);

    /**
     * Inserts the node at the appropriate location under \c nodeToCheck.
     * On the initial (nonrecursive) call to this function, #topNode_ is
     * passed as \c nodeToCheck.
     *
     * @param nodeToInsert The node to be inserted into the lattice.
     * @param nodeToCheck The node at which to start the search for the
     *                    insertion point.
     */
    void insertNode(QRLatticeIndexNodePtr nodeToInsert,
                    QRLatticeIndexNodePtr nodeToCheck);

    /**
     * Searches for maximal subsets of \c insertedNode among the children of
     * \c searchRoot. For each child that is not a subset, this function is
     * called recursively. No recursive call is made on a node that is a
     * subset, because we are only looking for maximal subsets.
     *
     * @param nodeToInsert Looking for maximal subsets of this node.
     * @param searchRoot   Node at which to begin the search.
     */
    void findAndAttachSubset(QRLatticeIndexNodePtr insertedNode,
                             QRLatticeIndexNodePtr searchRoot);

    /**
     * Find the supersets of a given node. This private member function is
     * called by the public function to find supersets, which initiates the
     * search at the top (artificial) node.
     *
     * @param subsetNode The node to search for supersets of.
     * @param parentNode The node to search from. The search moves recursively
     *                   from this node to its children.
     * @param supersets The list to which any found superset nodes are added.
     */
    void findSupersets_(QRLatticeIndexNode& subsetNode,
                        QRLatticeIndexNodePtr parentNode,
                        NAPtrList<QRLatticeIndexNodePtr>& supersets);

    void getSupersets(QRLatticeIndexNodePtr node,
                      NAPtrList<QRLatticeIndexNodePtr>& supersets);

    /**
     * Find the subsets of a given node. This private member function is
     * called by the public function to find subsets, which initiates the
     * search at the bottom (artificial) node.
     * 
     * @param supersetNode The node to search for subsets of.
     * @param childNode The node to search from. The search moves recursively
     *                  from this node to its parents.
     * @param subsets The list to which any found subset nodes are added.
     */
    void findSubsets_(QRLatticeIndexNode& supersetNode,
                      QRLatticeIndexNodePtr childNode,
                      NAPtrList<QRLatticeIndexNodePtr>& subsets);

    void getSubsets(QRLatticeIndexNodePtr node,
                      NAPtrList<QRLatticeIndexNodePtr>& subsets);

    /**
     * Initiates the traversal of the lattice to resize the bitmap for each node.
     * @see #resizeKeyBitmaps(QRLatticeIndexNodePtr)
     */
    void resizeKeyBitmaps();

    /**
     * Resizes the bitmap that encodes the contained keys, for each node in the
     * lattice. The set of keys used in the lattice as new nodes with different
     * keys are introduced. At some point it is possible that more keys than
     * were originally anticipated will be used, and the original bitmaps will be
     * too small. This function goes through the lattice and reallocates the
     * bitmaps so that all keys can be represented. The new size is the updated
     * value of #maxNumKeys_.
     *
     * @param node The current node in the traversal of the lattice.
     * @see #resizeKeyBitmaps()
     */
    void resizeKeyBitmaps(QRLatticeIndexNodePtr node);

    /** Heap to use for any memory allocations. */
    CollHeap* heap_;
    
    /** 
     * Maximum number of keys used in the lattice. This is used to size the
     * key array and the bitmaps in the nodes. It is increased on demand.
     */
    CollIndex maxNumKeys_;

    /** Array of all the keys used in the lattice. */
    LatticeKeyArray keyArr_;

    /** Hash table mapping key values to their index in the key array. */
    NAHashDictionary<const NAString, CollIndex> ordinalsHash_;

    /** Hash table mapping sets of key values to lattice index nodes with the
        exact same set of keys. */
    ExactSetsHash exactSetsHash_;

    /** Artificial node introduced to give the lattice a single root. */
    QRLatticeIndexNodePtr topNode_;

    /**
     * Artificial node introduced to provide a starting point for upward
     * traversals. All leaf regular nodes have this as their only child.
     */
    QRLatticeIndexNodePtr bottomNode_;

    /**
     * Sequence numbers are assigned to nodes as they are added to the lattice.
     * They are used as node labels in the visual representation of the lattice
     * that is output to the log, and also indicate the order of addition to
     * the lattice.
     */
    UInt32 seqNum_;

    /** Indicates whether a traversal of the lattice is in progress. */
    NABoolean inTraversal_;

    /** 
     * Incremented for each new traversal, this is used to mark nodes as having
     * been visited during a given traversal.
     */
    UInt32 visitSeqNum_;
}; // QRLatticeIndex

/**
 * Resource management class to manage access to a lattice during a
 * traversal. The class uses the RIAA (resource acquisition is initialization)
 * design pattern, using the constructor to request access to the lattice and
 * the destructor to release it for subsequent use. If a %QRLatticeIndexLock
 * is created on the stack in a function, it ensures that the lattice will be
 * released when the function exits, even if an exception is thrown.
 * \n\n
 * Traversals of the lattice must be single-threaded, because prior visitation
 * of a node is indicated by a member variable in the node itself. This does
 * not limit anything that needs to be done in the current implementation.
 * This class is used to catch any attempt to do simultaneous traversals, or to
 * update the lattice while a traversal is in progress.
 */
class QRLatticeIndexLock
{
  public:
    /**
     * Creates the lock and initiates the traversal. If a traversal is already
     * in progress, an exception is thrown. No updates to the lattice are
     * permitted while the traversal is in progress.
     *
     * @param lattice The lattice to be traversed.
     */
    QRLatticeIndexLock(QRLatticeIndexPtr lattice)
      : lattice_(lattice)
      {
        lattice_->startTraversal();
      }

    /**
     * Ends the traversal and releases the lock.
     */
    virtual ~QRLatticeIndexLock()
      {
        lattice_->endTraversal();
      }

  private:
    // Copy construction/assignment not defined.
    QRLatticeIndexLock(const QRLatticeIndexLock&);
    QRLatticeIndexLock& operator=(const QRLatticeIndexLock&);

    /** Lattice being traversed. */
    QRLatticeIndexPtr lattice_;
}; // QRLatticeIndexLock

#endif  /* _QRLATTICEINDEX_H_ */

