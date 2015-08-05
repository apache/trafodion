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

#ifndef _QMS_H_
#define _QMS_H_

#ifdef NA_NSK
#include "ComRtUtils.h"
#endif

#include <fstream>
#include "QRLogger.h"
#include "QRDescriptor.h"
#include "QmsMVMemo.h"
#include "QmsJoinGraph.h"
#include "QmsLatticeIndex.h"
#include "QRMVDefinition.h"
#include "NAString.h"

class Qms;

typedef SharedPtrValueHash<const NAString, MVDetails>		MVDetailHash;
typedef SharedPtrValueHashIterator<const NAString, MVDetails>	MVDetailHashIterator;

/**
 * Class Qms encapsulates everything that has to do with the matching data structures.
 * There is a single static instance of this class so it should not be a SharedPtr class.
 */
class Qms : public NABasicObject
{
public:
  static Qms* getInstance(NAMemory* heap = NULL)
  {
    if (!instance_)
      instance_ = new(heap) Qms(heap);
    return instance_;
  }

  /**
   * Deletes the single instance of this class. This happens in response to a
   * CLEANUP request, and forces the next invocation of getInstance() to create
   * a new instance. This new instance will have no knowledge of any MVs in the
   * system catalog until an INITIALIZE request is made on it.
   */
  static void deleteInstance()
  {
    delete instance_;
    instance_ = NULL;
  }
 
  virtual ~Qms();

  /**
   * Tells whether the object has been initialized. Initialization includes
   * populating the search structures witn information on the MVs stored in the
   * system catalog.
   *
   * @return \c TRUE iff the object has been initialized.
   */
  NABoolean isInitialized() const
  {
    return isInitialized_;
  }

  /**
   * Sets the initialization status of the object. A Qms object is created in
   * the uninitalized state, and is initialized upon request.
   *
   * @param newVal Boolean value indicating new initialization status.
   */
  void setInitialized(NABoolean newVal)
  {
    isInitialized_ = newVal;
  }
     
  /**
   * Insert a new MV into the QMS matching data structures.
   * @param mvDescPtr Descriptor of MV.
   * @param mvDefPtr QRMVDefinition descriptor pointer.
   */
  void insert(QRMVDescriptorPtr mvDescPtr, const QRMVDefinition* mvDefPtr = NULL);

  /**
   * Search for MVs matching the input query.
   * @param qryDescPtr Descriptor of query to match.
   * @param requestHeap Heap on which to allocate all temporary data.
   * @return 
   */
  QRResultDescriptorPtr match(QRQueryDescriptorPtr qryDescPtr, NAMemory* requestHeap);

  /**
   * Remove an MV from the matching data structures.
   * @param mvName Name of MV to remove.
   */
  void drop(const NAString& mvName);

  /**
   * Alter the ignore changes value of an MV.
   * @param mvName Name of MV to alter.
   * @param hasIgnoreChanges New IGNORE_CHANGES status of the MV.
   */
  void alter(const NAString& mvName, NABoolean hasIgnoreChanges);

  /**
   * Update the redefinition timestamp of an MV.
   * @param mvName Name of MV to alter.
   * @param timestamp New redefinition timestamp of MV.
   */
  void touch(const NAString& mvName, const NAString& timestamp);

  /**
   * Alter the last refresh timestamp of an MV.
   * @param mvName Name of MV to alter.
   * @param timestamp New last refresh timestamp of MV.
   */
  void refresh(const NAString& mvName, const NAString& timestamp, NABoolean isRecompute = FALSE);

  /**
   * Rename an MV from mvName to newName.
   * @param oldName The old name of the MV.
   * @param newName The new name of the MV.
   * @param timestamp New redefinition timestamp of MV.
   */
  void rename(const NAString& oldName, const NAString& newName);

  /**
   * Check if QMS includes an MV named mvName.
   * @param mvName The name of the MV to find.
   * @return TRUE if the MV is found, FALSE otherwise.
   */
  NABoolean contains(const NAString& mvName);

  /**
   * Get the redefinition timestamp of an MV.
   * @param mvName The name of the MV to find.
   * @return The redefinition timestamp as an Int64 number, or NULL if the MV is not contained in QMS.
   */
  const Int64 *getMVTimestamp(const NAString& mvName);

  /**
   * Perform workload analysis.
   */
  void workloadAnalysis(ofstream& ofs, Int32 minQueriesPerMV, NAMemory* requestHeap);

  /**
   * Used to collect memory usage by QMS.
   * 
   * Get pointer to the heap used by QMS.
   */
   inline NAMemory* getHeap() { return (NAMemory *) heap_; };

  void dumpInventoryHash();

protected:
  Qms(NAMemory* heap)
    : mvMemo_(ADD_MEMCHECK_ARGS(heap))
     ,MVInventoryHash_(hashKey, INIT_HASH_SIZE_LARGE, TRUE, heap) // Pass NAString::hashKey
     ,isInitialized_(FALSE)
     ,heap_(heap)
  {}
 
private:
  QRJoinGraphPtr createJoinGraphForJBB(const QRJBBPtr jbb, const NAString& title, CollHeap* heap);

  inline MVDetailsPtr getMvDetails(const NAString& mvName)
  {
    return MVInventoryHash_.getFirstValue(&mvName);
  }

private:
  static Qms*  instance_;
  NAMemory*    heap_;
  MVMemo       mvMemo_;
  MVDetailHash MVInventoryHash_;
  NABoolean    isInitialized_;
};

#endif  /* _QMS_H_ */
