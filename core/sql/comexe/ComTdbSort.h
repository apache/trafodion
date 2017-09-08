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
/* -*-C++-*-
****************************************************************************
*
* File:         ComTdbSort.h
* Description:  
*
* Created:      5/6/98
* Language:     C++
*
*
*
*
****************************************************************************
*/

#ifndef COM_SORT_H
#define COM_SORT_H

#include  "Platform.h"
#include "ComTdb.h"

class SortOptions : public NAVersionedObject
{
public:
  enum SortType
  {
    REPLACEMENT_SELECT, QUICKSORT,ITER_HEAP, ITER_QUICK
  };

  SortOptions() : NAVersionedObject(-1)
  {
    internalSort_ = TRUE;
    sortMaxHeapSizeMB_ = 0;               
    scratchFreeSpaceThreshold_ = 0; 
    sortType_ = ITER_QUICK;
    dontOverflow_ = FALSE;
    sortNRows_ = FALSE;
    flags_ = 0;
    memoryQuotaMB_ = 0;
    bmoMaxMemThresholdMB_ =0;
    pressureThreshold_ = 10;
    scratchIOBlockSize_ = 524288; //512kb initial size.
  };

  // ---------------------------------------------------------------------
  // Redefine virtual functions required for Versioning.
  //----------------------------------------------------------------------
  virtual unsigned char getClassVersionID()
  {
    return 1;
  }

  virtual void populateImageVersionIDArray()
  {
    setImageVersionID(0,getClassVersionID());
  }

  virtual short getClassSize()     { return (short)sizeof(SortOptions); }
  
  Int32 &internalSort(){return internalSort_;};
  Int16 &sortMaxHeapSize() {return sortMaxHeapSizeMB_; };
  UInt16 &scratchFreeSpaceThresholdPct() {return scratchFreeSpaceThreshold_;}
  Int16 &sortType(){return sortType_;};
  Int32 &dontOverflow(){return dontOverflow_;};  
  Int32 &sortNRows(){return sortNRows_;};
  Int16 &memoryQuotaMB(){return memoryQuotaMB_;};
  UInt16 &bmoMaxMemThresholdMB(){return bmoMaxMemThresholdMB_;};
  Int16 &pressureThreshold(){return pressureThreshold_;}
  Int16 &mergeBufferUnit() {return mergeBufferUnit_;};
  Int32 &scratchIOBlockSize() {return scratchIOBlockSize_;}
  Int16 &scratchIOVectorSize(){return scratchIOVectorSize_;}

  NABoolean bufferedWrites() { return (flags_ & BUFFERED_WRITES_) != 0;}
  void setBufferedWrites(NABoolean v)
  {(v ? flags_ |= BUFFERED_WRITES_ : flags_ &= ~BUFFERED_WRITES_);}
  NABoolean logDiagnostics() { return (flags_ & LOG_DIAGNOSTICS_) != 0;}
  void setLogDiagnostics(NABoolean v)
  {(v ? flags_ |= LOG_DIAGNOSTICS_ : flags_ &= ~LOG_DIAGNOSTICS_);}
  NABoolean disableCmpHintsOverflow() 
  { return (flags_ & DISABLE_CMP_HINTS_OVERFLOW_) != 0;}
  void setDisableCmpHintsOverflow(NABoolean v)
  {(v ? flags_ |= DISABLE_CMP_HINTS_OVERFLOW_ : 
    flags_ &= ~DISABLE_CMP_HINTS_OVERFLOW_);}

  NABoolean intermediateScratchCleanup() { return (flags_ & INTERMEDIATE_SCRATCH_CLEANUP_) != 0;}
  void setIntermediateScratchCleanup(NABoolean v)
  {(v ? flags_ |= INTERMEDIATE_SCRATCH_CLEANUP_ : flags_ &= ~INTERMEDIATE_SCRATCH_CLEANUP_);}

  NABoolean resizeCifRecord() { return (flags_ & RESIZE_CIF_RECORD_) != 0;}
  void setResizeCifRecord(NABoolean v)
  {(v ? flags_ |= RESIZE_CIF_RECORD_ : flags_ &= ~RESIZE_CIF_RECORD_);}

  NABoolean considerBufferDefrag() { return (flags_ & CONSIDER_BUFFER_DEFRAG_) != 0;}
  void setConsiderBufferDefrag(NABoolean v)
  {(v ? flags_ |= CONSIDER_BUFFER_DEFRAG_ : flags_ &= ~CONSIDER_BUFFER_DEFRAG_);}


  // ---------------------------------------------------------------------
  // Used by the internal SHOWPLAN command to get attributes of a TDB.
  // ---------------------------------------------------------------------
  void displayContents(Space *space);

protected:
  enum
  {
    BUFFERED_WRITES_   = 0x0001,
    LOG_DIAGNOSTICS_   = 0x0002,
    DISABLE_CMP_HINTS_OVERFLOW_ = 0x0004,
    INTERMEDIATE_SCRATCH_CLEANUP_ = 0x0008,
    RESIZE_CIF_RECORD_       = 0x0010,
    CONSIDER_BUFFER_DEFRAG_  = 0x0020
  };

  Int32 internalSort_;              // 00-03
  Int16 sortMaxHeapSizeMB_;          // 04-05
  UInt16 scratchFreeSpaceThreshold_; // 06-07
  Int32 dontOverflow_;              // 08-11
  Int32 sortNRows_;                 // 12-15
  Int16 sortType_;                  // 16-17
  UInt16 flags_;                    // 18-19
  Int16 memoryQuotaMB_;             // 20-21
  Int16 pressureThreshold_;         // 22-23
  Int16 mergeBufferUnit_;          // 24-25
  Int16 scratchIOVectorSize_;      // 26-27
  Int32 scratchIOBlockSize_;       // 28-31
  UInt16 bmoMaxMemThresholdMB_;    // 32-33
  char fillersSortOptions_[22];    // 34-55

};

// ---------------------------------------------------------------------
// Template instantiation to produce a 64-bit pointer emulator class
// for SortOptions
// ---------------------------------------------------------------------
typedef NAVersionedObjectPtrTempl<SortOptions> SortOptionsPtr;


class ComTdbSort : public ComTdb
{
  friend class ExSortTcb;
  friend class ExSortPrivateState;

protected:
  enum
  {
    COLLECT_NF_ERRORS_     = 0x0001,
    PREFIX_ORDERED_INPUT   = 0x0002,
    SORT_FROM_TOP          = 0x0004,
    USER_SIDETREE_INSERT   = 0x0008,
    SORT_TOPN_ENABLE       = 0x0010
  };

  SortOptionsPtr sortOptions_;                          // 00-07

  ExCriDescPtr workCriDesc_;                            // 08-15

  // expression to create the encoded key given to sort
  ExExprPtr sortKeyExpr_;                               // 16-23

  // expression to create the input row for sort
  ExExprPtr sortRecExpr_;                               // 24-31

  ComTdbPtr tdbChild_;                                  // 32-39

  // length of input row to sort.
  Int32 sortRecLen_;                                    // 40-43

  Int32 sortKeyLen_;                                    // 44-47

  // index into atp of new sorted row tupp
  UInt16 tuppIndex_;                                    // 48-49

  UInt16 flags_;                                        // 50-51

  // estimated number of records to be sorted
  Int32 maxNumBuffers_;                                 // 52-55

  // length of partially sorted key.
  Int32 sortPartialKeyLen_;                             // 56-59

  UInt32 minimalSortRecs_;                              // 60-63
  Float32 sortMemEstInKBPerNode_;                        // 64-67
  Float32 bmoCitizenshipFactor_;                        // 68-71
  Int32  pMemoryContingencyMB_;                        // 72-75
  UInt16 sortGrowthPercent_;                            // 76-77
  char filler_1[2];                                     // 78-79
  Int32 topNThreshold_;                                 // 80-83
  Float32 estMemoryUsage_;                             // 84-87
  Float32 bmoQuotaRatio_;                              // 88-92
  char fillersComTdbSort_[4];                          // 93-96

public:

  // Constructor
  ComTdbSort(); // dummy constructor. Used by 'unpack' routines.

  ComTdbSort(ex_expr * sort_key_expr,
	     ex_expr * sort_rec_expr,
	     ULng32 sort_key_len,
	     ULng32 sort_rec_len,
	     ULng32 sort_partial_key_len,
	     const unsigned short tupp_index,
	     ComTdb * child_tdb,
	     ex_cri_desc * given_cri_desc,
	     ex_cri_desc * returned_cri_desc,
	     ex_cri_desc * work_cri_desc,
	     queue_index down,
	     queue_index up,
	     Cardinality estimatedRowCount,
	     Lng32 num_buffers,
	     ULng32 buffer_size,
	     ULng32 num_recs,
	     SortOptions * sort_options,
	     short sortGrowthPercent
	     );

  ~ComTdbSort();

  // ---------------------------------------------------------------------
  // Redefine virtual functions required for Versioning.
  //----------------------------------------------------------------------
  virtual unsigned char getClassVersionID()
  {
    return 1;
  }

  virtual void populateImageVersionIDArray()
  {
    setImageVersionID(1,getClassVersionID());
    ComTdb::populateImageVersionIDArray();
  }

  virtual short getClassSize()      { return (short)sizeof(ComTdbSort); }

  Long pack(void *);
  Lng32 unpack(void *, void * reallocator);

  void display() const;

  // inline methods
  inline ComTdb * getChildTdb(){return tdbChild_;};
 
  Int32 orderedQueueProtocol() const ; 

  virtual const ComTdb* getChild(Int32 pos) const
    {
      if (pos == 0)
	return tdbChild_.getPointer();
      else
	return NULL;
    }

  virtual Int32 numChildren() const { return 1; }
  virtual const char *getNodeName() const { return "EX_SORT"; };
  virtual Int32 numExpressions() const { return 2; }
  virtual ex_expr* getExpressionNode(Int32 pos) {
     if (pos == 0)
	return sortKeyExpr_;
     else
       if (pos == 1)
	 return sortRecExpr_;
       else
	 return NULL;
  }

  virtual const char * getExpressionName(Int32 pos) const {
     if (pos == 0)
	return "sortKeyExpr_";
     else if (pos == 1)
       return "sortRecExpr_";
     else
       return NULL;
  }

  void setMinimalSortRecs(UInt32 sortRecs = 0)
  {
    minimalSortRecs_ = sortRecs;
  }

  NABoolean collectNFErrors() { return (flags_ & COLLECT_NF_ERRORS_) != 0;}
  void setCollectNFErrors(NABoolean v)
  {(v ? flags_ |= COLLECT_NF_ERRORS_ : flags_ &= ~COLLECT_NF_ERRORS_);}

  NABoolean partialSort() { return (flags_ & PREFIX_ORDERED_INPUT) != 0;}
  void setPartialSort(NABoolean v)
  {(v ? flags_ |= PREFIX_ORDERED_INPUT : flags_ &= ~PREFIX_ORDERED_INPUT);}

  NABoolean sortFromTop() { return (flags_ & SORT_FROM_TOP) != 0;}
  void setSortFromTop(NABoolean v)
  {(v ? flags_ |= SORT_FROM_TOP : flags_ &= ~SORT_FROM_TOP);}
  
  NABoolean topNSortEnabled() { return (flags_ & SORT_TOPN_ENABLE) != 0;}
  void setTopNSortEnabled(NABoolean v)
  {(v ? flags_ |= SORT_TOPN_ENABLE : flags_ &= ~SORT_TOPN_ENABLE);}

  NABoolean userSidetreeInsert()
  {
    return ((flags_ & USER_SIDETREE_INSERT) != 0);
  }
  void setUserSidetreeInsert(NABoolean v)
  {
    (v ? flags_ |= USER_SIDETREE_INSERT : flags_ &= ~USER_SIDETREE_INSERT);
  }

  SortOptionsPtr getSortOptions() {return sortOptions_ ;}
  
  void setBmoCitizenshipFactor(Float32 bmoCf)
    { bmoCitizenshipFactor_ = bmoCf; }
  Float32 getBmoCitizenshipFactor(void)
    { return bmoCitizenshipFactor_;}
  void setMemoryContingencyMB(Int32 mCMB)
    {  pMemoryContingencyMB_ = mCMB;} 
  Int32 getMemoryContingencyMB(void)
    { return pMemoryContingencyMB_; }
  
  void setSortMemEstInKBPerNode(Float32 s) {sortMemEstInKBPerNode_=s;}
  Float32 getSortMemEstInKBPerNode() {return sortMemEstInKBPerNode_;}
  Float32 sortGrowthPercent() {return Float32(sortGrowthPercent_/100.0);}

  void setTopNThreshold(Int32 limit)
    {  topNThreshold_ = limit;} 
  Int32 getTopNThreshold(void)
    { return topNThreshold_; }

  void setEstimatedMemoryUsage(Float32 estMemory)
    { estMemoryUsage_ = estMemory; }
  virtual Float32 getEstimatedMemoryUsage(void)
    { return estMemoryUsage_;}

  void setBmoQuotaRatio(Float32 bmoQuotaRatio)
    { bmoQuotaRatio_ = bmoQuotaRatio; }
  virtual Float32 getBmoQuotaRatio(void)
    { return bmoQuotaRatio_;}

  // ---------------------------------------------------------------------
  // Used by the internal SHOWPLAN command to get attributes of a TDB.
  // ---------------------------------------------------------------------
  virtual void displayContents(Space *space,ULng32 flag);
};


#endif
