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

#ifndef _QRGROUPLATTICE_H_
#define _QRGROUPLATTICE_H_

#include "NAString.h"
#include "QmsLatticeIndex.h"
#include "QmsMVCandidate.h"
#include "QRSharedPtr.h"
#include "QmsWorkloadAnalysis.h"

/**
 * \file
 * Contains the class definition for QRGroupLattice, which uses a lattice index
 * to match MVs on grouping columns and expressions.
 */

class QRGroupLattice;

#ifdef _MEMSHAREDPTR
typedef QRIntrusiveSharedPtr<QRGroupLattice>  QRGroupLatticePtr;
#else
typedef QRGroupLattice*  QRGroupLatticePtr;
#endif

/**
 * Class representing a lattice index for grouping columns/expressions of MVs.
 * An instance of this class is used for all MVs with a common join graph that
 * have a Group By clause. The class contains a QRLatticeIndex object, and
 * offers methods to create and insert a node derived from an MV into that
 * lattice, and to search the lattice for MVs having primary grouping
 * columns/expressions matching that are a superset of those of the given query JBB.
 * These MVs are returned in a list of candidates. A candidate with grouping
 * columns/expressions exactly matching those of the MV are marked as preferred.
 */
class QRGroupLattice : public NAIntrusiveSharedPtrObject
{
  public:
    QRGroupLattice(CollHeap* heap, ADD_MEMCHECK_ARGS_DECL(CollIndex maxNumKeys));

    virtual ~QRGroupLattice();

    /**
     * Creates and inserts a node representing an MV's primary and dependent 
     * grouping columns into the lattice index.
     *
     * @param mvDetails Details of the MV for which the node is being created.
     * @param jbb The JBB from which to take the grouping columns.
     */
    void insert(QRJoinSubGraphMapPtr map, const QRJBBPtr jbb);

    /**
     * Removes an MV from the grouping lattice index. The affected node in the
     * lattice index is determined using a key list derived from the MV's primary
     * and dependent grouping columns. The MV is removed from the node, and if
     * there are no other MVs in the node, the node is removed from the lattice.
     *
     * @param map The SubGraphMap of the MV for which is to be removed.
     */
    void remove(QRJoinSubGraphMapPtr map);

    /**
     * Searches the group lattice for MVs having a grouping list that is a
     * superset of that of the passed query JBB. An improper superset (exact
     * match) causes the candidate to be marked as preferred. Only the primary 
     * grouping columns of the JBB are checked.
     *
     * @param queryJbb The query JBB being matched.
     * @param candidates List of MV candidates that match the query based on
     *                   primary grouping columns/expressions.
     * @param map The subGraphMap that defines the JBBSubset of the matching 
     *                 MV candidates.
     * @param minimizedGroupingList The minimized GroupBy list in case of 
     *                 IndirectGroupBy.
     */
    MVCandidatesForJBBSubsetPtr search(QRJBBPtr		      queryJbb, 
				       MVCandidatesForJBBPtr  mvCandidates, 
				       QRJoinSubGraphMapPtr   map,
                                       ElementPtrList*        minimizedGroupingList);

    /**
     * Builds a specification of the group lattice using the DOT language, from
     * which a visual representation can be rendered. This graph will show
     * directed edges in the direction of parent to child.
     *
     * @param tag Text appended to the lines preceding and following the
     *            DOT text.
     */
    void dumpLattice(const char* tag = "");

    void reportStats(NAString& text);

    /**
    * Collect data on query (MV) groups with shared join+GroupBy.
    */
    void collectMVGroups(WorkloadAnalysisPtr workload, Int32 minQueriesPerMV, CollHeap* heap);

  private:
    // Copy construction/assignment not defined.
    QRGroupLattice(const QRGroupLattice&);
    QRGroupLattice& operator=(const QRGroupLattice&);

    /**
     * Gets the list of lattice keys from the grouping columns in the passed JBB.
     *
     * @param latticeKeys The lattice key list to put the keys in. Any prior
     *                    contents are removed.
     * @param map Used only to get MV name if logging a failure.
     * @param jbb JBB containing the grouping columns to use.
     * @param primaryOnly TRUE if only the primary grouping columns should be used.
     * @param insertMode TRUE if this is an Insert operation, FALSE if its a Search.
     * @return TRUE if OK, FALSE if some keys not found (in search mode only)
     */
    NABoolean getGroupingLatticeKeys(LatticeKeyList&         latticeKeys,
                                     QRJoinSubGraphMapPtr    map,
				     DescriptorDetailsPtr    queryDetails, 
                                     const QRJBBPtr          jbb,
                                     NABoolean               primaryOnly,
                                     NABoolean               insertMode);

    LatticeIndexablePtr elementToKey(const QRElementPtr    element, 
                                     QRJoinSubGraphMapPtr  map,
			             DescriptorDetailsPtr  queryDetails, 
                                     NABoolean             insertMode,
                                     NABoolean             isRecursive = FALSE);

    QRElementPtr keyToElement(LatticeIndexablePtr key, QRJoinSubGraphMapPtr map);

    NABoolean elementListToKeyList(const ElementPtrList& elementList, 
                                   LatticeKeyList&       keyList,
                                   QRJoinSubGraphMapPtr  map,
				   DescriptorDetailsPtr  queryDetails, 
                                   NABoolean             insertMode);

private:
    /** Lattice index storing grouping columns for MVs. */
    QRLatticeIndexPtr lattice_;
    QRElementHash     reverseKeyHash_;

    CollHeap* heap_;
}; // QRGroupLattice

#endif  /* _QRGROUPLATTICE_H_ */
