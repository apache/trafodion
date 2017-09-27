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

#include "QmsQms.h"
#include "QRDescriptor.h"
#include "QRLogger.h"
#include "QmsMVMemo.h"
#include "NAType.h"
#include "ComRtUtils.h"

/**
 * \file
 * A command-line version of the Query Matching Server (QMS). The purpose of the
 * program is to provide a simplified testing tool for Query Rewrite requests,
 * although it could be adapted to start either a simple executable or a server
 * process, based on an additional command-line argument. Presently there are
 * two arguments, an input file and an output file. The input file should contain
 * a number of message specifiers (one per line), each consisting of the header
 * part of the message and a reference to a file containing the associated XML
 * document (if the specific message type requires one). The output file will
 * contain the return codes indicating the outcome of processing each message in
 * the input file, as well as result descriptors for any messages that specify a
 * MATCH request.
 */

// This is needed to avoid a link error.
NABoolean NAType::isComparable(const NAType &other,
			       ItemExpr *parentOp,
			       Int32 emitErr) const
{ return FALSE; }

Qms* Qms::instance_ = NULL;

Qms::~Qms()
{
  MVDetailHashIterator iterator(MVInventoryHash_);
  const NAString* key;
  MVDetailsPtr mvDetails;
  for (CollIndex i = 0; i < iterator.entries(); i++) 
  {
    // Get the next MV
    iterator.getNext(key, mvDetails); 
    // For reference counting.
    mvDetails = NULL;
    // Drop the MV.
    drop(*key);
  }

}

/**
 * Matches the passed query descriptor against known MVs, determining which MVs
 * are candidates for rewriting the query by replacing JBB subsets with the
 * already-computed results of the MV. These candidates, along with the pertinent
 * rewrite instructions, are included in the result descriptor.
 *
 * @param qryDescPtr  Pointer to the document element of the descriptor for the
 *                    query that is to be rewritten.
 * @param requestHeap The heap from which to allocate temporary objects.
 */
QRResultDescriptorPtr Qms::match(QRQueryDescriptorPtr qryDescPtr, NAMemory* requestHeap)
{
  static Int32 queryNumber = 0;

  try
  {
    DescriptorDetailsPtr queryDetails = new(requestHeap)
      DescriptorDetails(qryDescPtr, FALSE, ADD_MEMCHECK_ARGS(requestHeap));
    queryDetails->init(requestHeap);

    MVCandidateCollectionPtr mvCandidates = new(requestHeap)
      MVCandidateCollection(queryDetails, ADD_MEMCHECK_ARGS(requestHeap));

    // Count this query being matched.
    queryNumber++;

    // For each JBB in the query descriptor, search for MV candidates.
    const NAPtrList<QRJBBPtr>& jbbs = qryDescPtr->getJbbList();
    for (CollIndex i = 0; i < jbbs.entries(); i++)
    {
      QRJBBPtr jbb = jbbs[i];

      // Collection of MVCandidates for this JBB.
      MVCandidatesForJBBPtr jbbCandidates = new(requestHeap) 
	MVCandidatesForJBB(jbb, mvCandidates, ADD_MEMCHECK_ARGS(requestHeap));

      // Search MVMemo for candidate MVs for this JBB
      try
        {
          mvMemo_.search(jbb, jbbCandidates, requestHeap);
        }
      catch (...)
        {
          // Exception occurred, skip this JBB and keep going.
          QRLogger::instance().log(CAT_QMS_MAIN, LL_MVQR_FAIL, 
            "JBB %s skipped due to exception", jbb->getID().data());
          deletePtr(jbbCandidates);
          continue;
        }

      mvCandidates->insert(jbbCandidates);
    }

    // Continue matching Pass 1 and Pass 2 algorithms.
    mvCandidates->doMatching();

    // Generate the result descriptor.
    QRResultDescriptorPtr resultDesc = mvCandidates->generateResultDescriptor(requestHeap);
    deletePtr(mvCandidates);

    //dumpInventoryHash();
    return resultDesc;
  }
  catch (QRException e)
  {
    QRLogger::instance().log(CAT_QMS_MAIN, LL_MVQR_FAIL, "MATCH operation aborted.");
    // Return an empty result descriptor.
    QRResultDescriptorPtr resultDesc = MVCandidateCollection::generateEmptyResultDescriptor(requestHeap);
    return resultDesc;
  }
  catch (...)
  {
    QRLogger::instance().log(CAT_QMS_MAIN, LL_MVQR_FAIL,
      "MATCH operation aborted because of unknown exception.");
    // Return an empty result descriptor.
    QRResultDescriptorPtr resultDesc = MVCandidateCollection::generateEmptyResultDescriptor(requestHeap);
    return resultDesc;
  }
}

/**
 * Insert a new MV into the QMS matching data structures.
 * @param mvDescPtr Descriptor of MV.
 */
void Qms::insert(QRMVDescriptorPtr mvDescPtr, const QRMVDefinition* mvDefPtr)
{
  static Int32 mvNumber = 0;

  try
  {
    // Count this query being matched.
    mvNumber++;

    MVDetailsPtr mvDetails = new(heap_) 
      MVDetails(mvDescPtr, ADD_MEMCHECK_ARGS(heap_));
    mvDetails->init(heap_);

    if (mvDefPtr)
    {
      mvDetails->setRedefTimestamp(mvDefPtr->redefTimeString_);
      mvDetails->setRefreshTimestamp(mvDefPtr->refreshedTimeString_);
      mvDetails->setIgnoreChanges(mvDefPtr->hasIgnoreChanges_);
    }

    // Iterate on the JBBs
    const NAPtrList<QRJBBPtr>& jbbs = mvDescPtr->getJbbList();
    if (jbbs.entries() > 1)
    {
      QRLogger::instance().log(CAT_QMS_MAIN, LL_MVQR_FAIL,
        "MV Insert operation aborted - Only MVs with a single JBB are supported for now.");
      return;
    }

    for (CollIndex i = 0; i < jbbs.entries(); i++)
    {
      QRJBBPtr jbb = jbbs[i];

      // Insert it into MVMemo
      mvMemo_.insert(jbb, mvDetails);
    }

    MVInventoryHash_.insert(&mvDetails->getMVName(), mvDetails);
    //dumpInventoryHash();
  }
  catch (QRException e)
  {
    // Ignore exceptions for now.
    QRLogger::instance().log(CAT_QMS_MAIN, LL_MVQR_FAIL,
      "PUBLISH operation aborted, An Exception occurred: %s", e.getMessage());
  }
  catch (...)
  {
    // Ignore exceptions for now.
    QRLogger::instance().log(CAT_QMS_MAIN, LL_MVQR_FAIL,
      "PUBLISH operation aborted, An unknown exception occurred.");
  }
}

/**
 * Remove an MV from the matching data structures.
 * @param mvName Name of MV to remove.
 */
void Qms::drop(const NAString& mvName)
{
  try
  {
    // If the MV is contained in QMS
    MVDetailsPtr mv = getMvDetails(mvName);
    if (mv != NULL)
    {
      // Remove it from the inventory.
      MVInventoryHash_.remove(&mvName);

      // Disengage it from MVMemo.
      mv->disengage();

      // Now delete the MVDetails object
      deletePtr(mv);
    }
    //dumpInventoryHash();
  }
  catch (QRException e)
  {
    // Ignore exceptions for now.
    QRLogger::instance().log(CAT_QMS_MAIN, LL_MVQR_FAIL,
      "DROP operation aborted.");
  }
  catch (...)
  {
    // Ignore exceptions for now.
    QRLogger::instance().log(CAT_QMS_MAIN, LL_MVQR_FAIL,
      "DROP operation aborted because of unknown exception.");
  }
}

/**
 * Touch the redefinition timestamp of an MV.
 * @param mvName Name of MV to touch.
 * @param timestamp New redefinition timestamp of MV.
 */
void Qms::touch(const NAString& mvName, const NAString& timestamp)
{
  MVDetailsPtr mvDetails = getMvDetails(mvName);
  if (mvDetails != NULL)
    mvDetails->setRedefTimestamp(timestamp);
  else
    QRLogger::instance().log(CAT_QMS_MAIN, LL_INFO,
      "Unable to TOUCH MV %s - not found in QMS.", mvName.data());
}

/**
 * Alter the IGNORE_CHANGES value of an MV.
 * @param mvName Name of MV to alter.
 * @param hasIgnoreChanges new IGNORE_CHANGES boolean value.
 */
void Qms::alter(const NAString& mvName, NABoolean hasIgnoreChanges)
{
  MVDetailsPtr mvDetails = getMvDetails(mvName);
  if (mvDetails != NULL)
  {
    mvDetails->setIgnoreChanges(hasIgnoreChanges);
  }
  else
    QRLogger::instance().log(CAT_QMS_MAIN, LL_INFO,
     "Unable to ALTER MV %s - not found in QMS.", mvName.data());
}

/**
 * Alter the last refresh timestamp of an MV.
 * @param mvName Name of MV to alter.
 * @param timestamp New last refresh timestamp of MV.
 */
void Qms::refresh(const NAString& mvName, const NAString& timestamp, NABoolean isRecompute)
{
  MVDetailsPtr mvDetails = getMvDetails(mvName);
  if (mvDetails != NULL)
  {
    mvDetails->setRefreshTimestamp(timestamp);
    if (isRecompute)
      mvDetails->setConsistent(TRUE);
    else if (mvDetails->hasIgnoreChanges())
      mvDetails->setConsistent(FALSE);
  }
  else
    QRLogger::instance().log(CAT_QMS_MAIN, LL_INFO,
      "Unable to Refresh MV %s - not found in QMS.", mvName.data());
}

/**
 * Rename an MV from mvName to newName.
 * @param oldName The old name of the MV.
 * @param newName The new name of the MV.
 * @param timestamp New redefinition timestamp of MV.
 * \todo Not sure this is complete. Is the MV name used anywhere else?
 */
void Qms::rename(const NAString& oldName, const NAString& newName)
{
  try
  {
    MVDetailsPtr mvDetails = getMvDetails(oldName);
    if (mvDetails != NULL)
    {
      // Remove it from the inventory.
      MVInventoryHash_.remove(&oldName);

      // Rename the MVDetails object.
      mvDetails->rename(oldName, newName);

      // Re-insert with the new name.
      MVInventoryHash_.insert(&mvDetails->getMVName(), mvDetails);

      //dumpInventoryHash();
    }
    else
      QRLogger::instance().log(CAT_QMS_MAIN, LL_INFO,
        "Unable to rename MV %s - not found in QMS.", oldName.data());
  }
  catch (QRException e)
  {
    // Ignore exceptions for now.
    QRLogger::instance().log(CAT_QMS_MAIN, LL_MVQR_FAIL, "Rename operation aborted.");
  }
  catch (...)
  {
    // Ignore exceptions for now.
    QRLogger::instance().log(CAT_QMS_MAIN, LL_MVQR_FAIL,
      "Rename operation aborted because of unknown exception.");
  }
}

/**
 * Check if QMS includes an MV named mvName.
 * @param mvName The name of the MV to find.
 * @return TRUE if the MV is found, FALSE otherwise.
 */
NABoolean Qms::contains(const NAString& mvName)
{
  MVDetailsPtr mvDetails = getMvDetails(mvName);
  return (mvDetails != NULL);
}
/**
 * Get the redefinition timestamp of an MV.
 * @param mvName The name of the MV to find.
 * @return The redefinition timestamp as an Int64 number, or NULL if the MV is not contained in QMS.
 */
const Int64 *Qms::getMVTimestamp(const NAString& mvName)
{ 
  MVDetailsPtr mvDetails = getMvDetails(mvName);
  if (mvDetails == NULL)
    return NULL;
  else
    return &mvDetails->getRedefTimestamp();
}

void Qms::dumpInventoryHash()
{
  QRLogger::instance().log(CAT_QMS_MAIN, LL_DEBUG, "Dumping MV Inventory:");
  MVDetailHashIterator iterator(MVInventoryHash_);
  const NAString* key;
  MVDetailsPtr mvDetails;
  for (CollIndex i = 0; i < iterator.entries(); i++) 
  {
    iterator.getNext(key, mvDetails); 
    QRLogger::instance().log(CAT_QMS_MAIN, LL_DEBUG, key->data());
  }
}

/**
 * Perform workload analysis.
 * all the workload queries have already been loaded as MV descriptors.
 * Now we need to:
 *   1. Collect groups of queries that share the same join graph and GroupBy list.
 *   2. Perform perdicate analysis on each group.
 *   3. Generate the SQL text for the CREATE MV command for the MV we ar proposing.
 */
void Qms::workloadAnalysis(ofstream& ofs, Int32 minQueriesPerMV, NAMemory* requestHeap)
{
  WorkloadAnalysisPtr workload = new(requestHeap) WorkloadAnalysis(requestHeap);
  // Collect the join graph + GroupBy list information
  mvMemo_.collectMVGroups(workload, minQueriesPerMV, requestHeap);
  // Perform predicate analysis and generate the SQL.
  workload->reportResults(ofs, minQueriesPerMV);
  deletePtr(workload);
}
