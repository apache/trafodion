//**********************************************************************
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
//**********************************************************************
// -*-C++-*-
//**********************************************************************
//
// File:         NAMemory.h
// Description:  The new memory management classes for Noah Ark sql.
//               
//               
// Created:      12/13/96
// Language:     C++
//
//
//**********************************************************************
#ifndef NAMEMORY__H
#define NAMEMORY__H

// IMPORTANT NOTE: Any changes to the layout, structures or functionality
//                 of NAMemory or NAHeap must increase the value of
//                 CURRENT_SHARED_OBJECTS_VERSION_ in runtimestats/SqlStats.h.
//                 Because runtime stats uses a shared memory area with
//                 NAMemory, all attached processes must have the same
//                 idea of how that memory is managed.
//
// IMPORTANT NOTE #2: The layout of these structures should be the same
//                    for release and debug builds to prevent corruption
//                    of the shared memory segment used by runtime stats.

#if (defined(NA_LINUX) && !defined(__EID))
#include <stdio.h>
#endif
#ifdef STAND_ALONE
#define NA_EIDPROC
#define TRUE 1
#define NABoolean bool
#endif

#ifndef STAND_ALONE
#include "NABasicObject.h"
#endif

#ifndef STAND_ALONE
#include "Platform.h"
#include "NAStringDefGlobals.h"
#endif
#include <stddef.h>
#include <setjmp.h>

#ifndef STAND_ALONE
#include "NAError.h"
#include "HeapID.h"
#endif

#ifdef _DEBUG
// forward declaration
template <class T> class NAList;
#define LIST(Type) NAList<Type>
#endif // _DEBUG

#if (defined(_DEBUG) || defined(NSK_MEMDEBUG)) && !defined(__EID)
#ifdef NA_STD_NAMESPACE
#include <iosfwd>
using namespace std;
#else
class ostream;
#endif
#endif // (defined(_DEBUG) || defined(NSK_MEMDEBUG)) && !defined(__EID)

#include <unistd.h>
#include <sys/types.h>
typedef uid_t SEG_ID;
#if !defined(__EID) && (defined(NA_LINUX) || defined(NA_WINNT))
#define HAVE_MMAP 1
#endif

// contents of this file:
class NAMemory;
class NASegGlobals;
class NABlock;
class NAHeap;
class NAHeapFragment;
class NATreeFragment;
class MemBinsem;
class SegmentStatus;


// MemoryStats is used for dynamically allocated statistics (when MEMDEBUG=1 or higher)
// and if NAHeap::dump() is called.
#ifndef __EID
class MemoryStats {
public:
  MemoryStats();

  void addEntry(size_t value);
#if (defined(_DEBUG) || defined(NSK_MEMDEBUG)) && !defined(__EID) && !defined(STAND_ALONE)
  void dump(ostream* outstream, const char* name, Lng32 indent);
#endif
private:
  size_t count_;      // number of entries
  double sum_;        // sum of all entries
  double sum2_;       // sum of squares (for variance)
  size_t histBuckets_[18]; // counters for different sizes
                         // 0 - all objects <= 32 bytes
                         // 1 - all objects <= 64 bytes
                         // 2 - all objects <= 128 bytes
                         // ...
                         // 16 - all objects <= 2 MB
                         // 17 - all objects > 2 MB
};
#endif

////////////////////////////////////////////////////////////////////////////
// One NASegGlobals object exists in the executor as a member variable
// of CliGlobals. Information about the first executor flat segment, as well
// as the address of the NASegGlobals object in which the information is
// stored, is passed as arguments to the setFirstSegInfo function, and used
// by NAMEMORY::allocateBlock. addSegId is called by NAMemory::allocateBlock
// to maintain an array of secondary (allocated after the first) flat segment
// ids. getSegId is called on MIPS by switchToPriv and switchToNonPriv to
// obtain the flat segment ids to hide and reveal.  On Yosemite, the
// segments are not hidden and revealed.
////////////////////////////////////////////////////////////////////////////
class NASegGlobals {
public:
  inline short getSegId(Lng32 &index) const
  {
    Lng32 i, addedSegCount;
    for (i = 0, addedSegCount = 0; addedSegCount < addedSegCount_; i++)
    {
      if (addedSegId_[i] != 0)
      {
        addedSegCount += 1;
        if (i >= index)
        {
          index = i;
          return addedSegId_[i];
        }
      }
    }
    return 0;
  }

NA_EIDPROC
  short getSegInfo(Lng32 index, void **startAddr) const
    {
      *startAddr = startAddresses_[index];
      return addedSegId_[index];
    }
  void   setFirstSegInfo(
			 SEG_ID firstSegId,
                         void *firstSegStart,
                         off_t  firstSegOffset,
                         size_t  firstSegLen,
                         size_t  firstSegMaxLen);
  void   setMaxSecSegCount(Lng32 maxSecSegCount)
                                { maxSecSegCount_ = maxSecSegCount; }
  NABoolean reachedMaxSegCnt() const
                                { return addedSegCount_ >= maxSecSegCount_; }
  Lng32   addSegId(short segId, void *start, size_t len);
  void   deleteSegId(short segId);
// LCOV_EXCL_START
  SEG_ID  getFirstSegId() const			 { return firstSegId_; }
  void * getFirstSegStart() const                { return firstSegStart_; }
  off_t   getFirstSegOffset() const              { return firstSegOffset_; }
  size_t   getFirstSegLen() const                { return firstSegLen_; }
  size_t   getFirstSegMaxLen() const             { return firstSegMaxLen_; }
// LCOV_EXCL_STOP
  void   resizeSeg(short segId, void *start, size_t newLen);

  // check whether a specified range of memory overlaps any of the segments
  NABoolean overlaps(void *start, size_t len) const;

  enum { NA_MAX_SECONDARY_SEGS=28 };   // Seg IDs 2111 - 2138
private:
  SEG_ID firstSegId_;
  void *firstSegStart_;    // starting addr of segment
  off_t  firstSegOffset_;  // offset of free space in the segment
  size_t  firstSegLen_;    // length of external segment
  size_t  firstSegMaxLen_; // max. len the segment can be resized to
  Lng32	addedSegCount_;    // number of additional segments
  Lng32  maxSecSegCount_;  // Maximum number of secondary segments
  short addedSegId_     [NA_MAX_SECONDARY_SEGS]; // array of secondary seg ids
  void *startAddresses_ [NA_MAX_SECONDARY_SEGS]; // start addresses of segs
  size_t lengths_[NA_MAX_SECONDARY_SEGS]; // lengths of segments

  // total range of memory spanned by the segments (may have other
  // things or holes between those water marks)
  void *lowWaterMark_;
  void *highWaterMark_;
};



////////////////////////////////////////////////////////////////////////////
// A NABlock is the basic allocation unit, i.e., we always request Blocks
// from the OS.  On NSK, the NABlock contains information about the segment
// that is allocated from the operating system.  On Windows, the NABlock
// represents the memory returned from a call to malloc().  On both Windows
// and NSK derived memories, NABlocks represent the memory that is returned
// from parent memory to the derived memory.
////////////////////////////////////////////////////////////////////////////
class NABlock {
public:
  // If more flags are added, muse.cpp will need to be changed.
  enum { MMAP_BIT        = 0x4,  // This NABlock was allocated with mmap.
         EXTERN_BIT      = 0x8   // Memory for this block was externally
                                 // allocated
       };

  enum { DERIVED_SEGMENT_ID = 0x7EF2 }; // Segment ID in derived blocks.

  SEG_ID segmentId_;     // id of the flat segment on NSK
  size_t size_;         // total size of the block
  NABlock * next_;      // next block in chain
  UInt32 sflags_; // "extern" and future other flags

NA_EIDPROC
  NABoolean blockHolds(void *addr);

NA_EIDPROC
  static NABlock *blockHolding(NABlock *firstBlock, void *addr);

NA_EIDPROC
  NABoolean isExternSegment();

#ifdef HAVE_MMAP
NA_EIDPROC
  NABoolean isMMapped();
#endif // HAVE_MMAP

NA_EIDPROC
  size_t firstFragmentOffset();

NA_EIDPROC
  NAHeapFragment* alignAsFragment();

#if (defined(_DEBUG) || defined(NSK_MEMDEBUG)) && !defined(__EID) && !defined(STAND_ALONE)
// LCOV_EXCL_START
NA_EIDPROC
void dump(ostream* outstream,
	  Lng32 debugLevel,
	  MemoryStats& freeStats,
	  MemoryStats& allocStats,
          NAHeapFragment *top,
	  Lng32 indent);
// LCOV_EXCL_STOP
#endif

NA_EIDPROC
  NABlock();

private:
};


////////////////////////////////////////////////////////////////////////////
// NAMemory is an abstract base class. NAHeap and Space/NASpace are derived
// from it.
////////////////////////////////////////////////////////////////////////////
#ifndef STAND_ALONE
class NAMemory : public NABasicObject {
#else
class NAMemory {
#endif
friend class NAHeap;
friend class ExMemStats;   // Class used in muse
friend class ExHeapStats;  // Class used in muse
friend class ComSpace;
public:
  // we distinguish 6 types of memory. The following table explains the
  // memory types for the different OS environments

  // memory type            NT/UNIX             NSK
  // -----------------------------------------------------------------------
  // DP2_MEMORY             regular memory      memory maintained by DP2
  // EXECUTOR_MEMORY        regular memory      selectable segment used in the
  //                                            master. This is a priv
  //                                            segment.
  // SYSTEM_MEMORY          regular memory      flat segments used in MXCMP
  // IPC_MEMORY             regular memory      flat segments used in ESPs
  // DERIVED_MEMORY         allocated in one of the other memory types.
  // DERIVED_FROM_SYS_HEAP  regular memory      regular memory

  // NO_MEMORY_TYPE is used, if we don't know the memory type at creation
  // time. Before a memory can be used, the type_ has to be set via
  // setType() or setParent()
  //
  // The number of NAMemory objects of NAMemoryType SYSTEM_MEMORY or IPC_MEMORY
  // is currently restricted to one because the assignment of flat segment ids
  // (NSK) for SYSTEM/IPC_MEMORY is not managed globally. If one such object
  // already exists, an assertion failure will occur following an attempt to
  // to allocate a segment with an id that was previously used. This is not
  // a problem because only the compiler's main statement heap resides in
  // SYSTEM_MEMORY, and only ESPs use IPC_Memory. If needed in the future, 
  // multiple SYSTEM/IPC _MEMORY heaps could be supported by keeping track of
  // SYSTEM_MEMORY/IPC segment ids in the NASegGlobals object.
  enum NAMemoryType {
    NO_MEMORY_TYPE = 0,
    DP2_MEMORY = 1,
    EXECUTOR_MEMORY = 2,
    SYSTEM_MEMORY = 3,
    DERIVED_MEMORY = 4,
    DERIVED_FROM_SYS_HEAP = 5,
    IPC_MEMORY = 6
  };

  // DerivedClass is necessary due to a characteristic of runtime stats. Because
  // there are multiple processes that use NAMemory objects in a shared memory
  // segment, virtual functions cannot be used on NAMemory unless the virtual
  // table pointer is modified each time a process needs to allocate or
  // deallocate memory.  This caused problems so it was decided that virtual
  // functions would no longer be used in the scenario.  The DerivedClass enum
  // is part of a mechanism that provides a mechanism similar to virtual functions,
  // but doesn't involve pointers that would be incompatible across different
  // programs.
  enum DerivedClass {
    NAHEAP_CLASS = 0,
    COMSPACE_CLASS = 1,
    DEFAULTIPCHEAP_CLASS = 2
  };

// default constructor. Initializes the memory to NO_MEMORY_TYPE.
NA_EIDPROC
  NAMemory(const char * name = NULL);

  // a basic (OS depended) memory. type is one of DP2_MEMORY,
  // or SYSTEM_MEMORY. DERIVED_MEMORY is not allowed as basic memory.
NA_EIDPROC
  NAMemory(const char * name, NAMemoryType type, size_t blockSize,
           size_t upperLimit);

  // an NAMemory of type EXECUTOR_MEMORY that uses a first flat segment
  // that is allocated by the caller (on NSK, uses malloc on NT and ignores
  // the parameters after the first one)
NA_EIDPROC
  NAMemory(const char * name,
           SEG_ID  extFirstSegId,
           void  * extFirstSegStart,
           off_t   extFirstSegOffset,
           size_t  extFirstSegLen,
           size_t  extFirstSegMaxLen,
           NASegGlobals  * segGlobals,
           Lng32   extMaxSecSegCount = NASegGlobals::NA_MAX_SECONDARY_SEGS);

  // DERIVED_MEMORY
NA_EIDPROC
  NAMemory(const char * name, NAHeap * parent, size_t blockSize,
           size_t upperLimit);

NA_EIDPROC
  ~NAMemory();

NA_EIDPROC
  void reInitialize();


NA_EIDPROC
  void setType(NAMemoryType type,
	       Lng32 blockSize = 0);

NA_EIDPROC
  Lng32 getAllocatedSpaceSize();

  // This method takes and returns the same arguments as an "operator new".
  // It is used to allocate arrays(!!) of the collected object(type T).
NA_EIDPROC
  void * allocateMemory(size_t size, NABoolean failureIsFatal = TRUE);

  // This method takes and returns the same arguments as an "operator delete".
  // It is used to deallocate the above arrays.
NA_EIDPROC
  void deallocateMemory(void * addr);

  // this method is used to set the upper limit - currently only used for testing
  // setjmp and longjmp
NA_EIDPROC
  void setUpperLimit ( size_t newUpperLimit ) { upperLimit_ = newUpperLimit; };
  
  // these four methods used to reside in class CollHeap 
NA_EIDPROC
  void setJmpBuf( jmp_buf *newJmpBuf );

NA_EIDPROC
  inline jmp_buf * getJmpBuf()                { return heapJumpBuf_; }

NA_EIDPROC
  inline NABoolean getWasMemoryExhausted()    { return exhaustedMem_; }

NA_EIDPROC
  void logAllocateError(short error, SEG_ID segmentId, Lng32 blockSize, short errorDetail);

NA_EIDPROC
  void handleExhaustedMemory();

#ifndef STAND_ALONE
#if (defined(_DEBUG) || defined(NSK_MEMDEBUG)) && !defined(__EID)
NA_EIDPROC
  void dump(ostream* outstream, Lng32 indent); 
#else
NA_EIDPROC
  inline void dump(void* outstream, Lng32 indent) {}
#endif
#endif // !STAND_ALONE

NA_EIDPROC
  void incrementStats(size_t size);

NA_EIDPROC
  void decrementStats(size_t size);

#ifndef __EID
  NA_EIDPROC
  void showStats(ULng32 level = 0);
#endif

NA_EIDPROC
  inline size_t getTotalSize() {return totalSize_;};

NA_EIDPROC
  inline size_t getIncrementSize() {return incrementSize_;};

NA_EIDPROC
  inline size_t getInitialSize() {return initialSize_;};

NA_EIDPROC
  inline NAHeap* getParent() {return parent_;};

NA_EIDPROC
  inline size_t getAllocSize() {return allocSize_;};

NA_EIDPROC
  inline size_t getAllocCnt() {return allocCnt_;};

NA_EIDPROC
  inline size_t getHighWaterMark() {return highWaterMark_;};

NA_EIDPROC
  inline size_t getIntervalWaterMark() {return intervalWaterMark_;};

NA_EIDPROC
  inline void resetIntervalWaterMark() { intervalWaterMark_ = allocSize_;};

NA_EIDPROC
  inline NASegGlobals * getSegGlobals() { return segGlobals_; }
NA_EIDPROC
  char *getName() {  return name_; }
NA_EIDPROC
  NAMemoryType getType() {  return type_; }
NA_EIDPROC
  NABoolean getUsage(size_t* lastSegSize, size_t* freeSize, size_t* totalSize);
  // for debugging
NA_EIDPROC
  NABoolean containsAddress(void *addr)
        { return NABlock::blockHolding(firstBlk_, addr) != NULL; }

  // #ifdef NA_WINNT
#ifndef __EID
  NABoolean  getSharedMemory() { return sharedMemory_; }
  void setSharedMemory() { sharedMemory_ = TRUE; }
#endif
protected:

  // set the name of this memory
NA_EIDPROC
  void setName(const char * name);

  // put child on memoryList_
NA_EIDPROC
  void registerMemory(NAMemory * child);

  // remove child from memoryList_
NA_EIDPROC
  void unRegisterMemory(NAMemory * child);


NA_EIDPROC
  void sysFreeBlock(NABlock *blk);

#ifdef HAVE_MMAP
NA_EIDPROC
  NABlock* allocateMMapBlock(size_t size);

  void deallocateMMapBlock(NABlock *blk);
#endif // HAVE_MMAP

#if (defined(NA_LINUX) && !defined(__EID))
  Lng32 getVmSize();
  void allocationIncrement(Lng32 size);
  void allocationDecrement(Lng32 size);
#endif // NA_LINUX
private:
  // these ctors make no sense at all -- should never be used
  NAMemory (const NAMemory &) ;            // not written
  NAMemory& operator= (const NAMemory &) ; // not written

  char name_[21];             // name of this memory
  NAMemoryType type_;         // the type of memory.
  size_t initialSize_;        // initial size of a block in bytes. For
                              // EXECUTOR_MEMORY we always allocate just
                              // one NABlock. This NABlock is in fact an
                              // extended segment.
  size_t maximumSize_;        // in Bytes. This is the maximum size of an
                              // extended segment.
  size_t incrementSize_;      // in bytes.

  NAHeap *parent_;            // heap from which this memory allocates
			      // its blocks. This allows for a arbitrary deep
                              // hierarchy of heaps. If parent == 0, the
                              // block is requested from the OS
			      // for now, It is not possible to have a
			      // hierachy of spaces. A space however, can
                              // allocate from a parent heap.
  NABlock *firstBlk_;         // Pointer to first NABlock for this heap
  size_t allocSize_;          // sum of all allocated bytes in this Memory
  size_t upperLimit_;         // ceiling for how much memory we should allocate - currently only used for testing
  size_t highWaterMark_;      // most bytes ever allocated in this memory
  size_t intervalWaterMark_;  // most bytes allocated within some interval (resettable)
  size_t allocCnt_;           // # of allocations
  size_t totalSize_;          // total number of bytes (sum of all blocks)
  size_t blockCnt_;           // number of blocks allocated
  size_t thBlockCnt_;         // threshhold block count. If blockCnt hits
                              // this value we increase the size of a block.
                              // thBlockCnt_ is 10 right now and gets
                              // incremented by 10 whenever we adjust the
                              // blocksize

                              // information on a block in a first segment
                              // that was allocated before this memory
                              // was created (allows to put the memory
                              // itself and other info into the segment)
  NASegGlobals *segGlobals_;  // Executor flat segment globals object

  NAMemory *memoryList_;      // list of memory directly derived from this
  NAMemory *lastListEntry_;   // last entry of this list
  NAMemory *nextEntry_;       // pointer if this memory is on a memoryList_

  Lng32 debugLevel_;           // 0 - no debugging
                              // 1 - re-initialize after alloc/de-alloc
                              // 2 - keep stack trace for each allocation

  // these data members used to be in class CollHeap
protected:
  jmp_buf *heapJumpBuf_;      // Setjmp() buffer for handing memory failures
  NABoolean exhaustedMem_;    // Set to true if cannot satisfy memory request
  unsigned short errorsMask_; // SEGMENT_ALLOCATE_ errors that have occurred
#ifndef STAND_ALONE
  HeapID heapID_;             // For tracking leaks.  (eric)
#endif
  Int64 crowdedTotalSize_; // Total size at which memory pressure seen
#ifndef __EID
  Int64 allocationDelta_; // Change in memory size since last check of VmSize
  FILE* procStatusFile_;      // FILE pointer for "reading" process status
  Lng32 executorVmReserveSize_; // Size in MB of VM safety net
  Int32 mmapErrno_;
  Int32 munmapErrno_;
  Lng32 lastVmSize_;
  Lng32 maxVmSize_;
#endif // __EID
  DerivedClass derivedClass_; // The derived class (removes virtual functions)
public:
#ifndef STAND_ALONE
  // ---------------------------------------------------------------------
  // The following method and data member are needed for minimizing
  // NAString's memory usage -- each heap needs to maintain a
  // (ref-counted) "null" NAString object, for all heap NAString's to
  // share as necessary.
  //
  // See NAStringDef.h/.cpp for a complete explanation of why this
  // dependence on NAString code is necessary.
  // ---------------------------------------------------------------------
NA_EIDPROC
  NAStringRef * nullNAStringRef() 
  // NB: the fudge-factor ("3") below is probably no longer necessary ...
  { return (NAStringRef*) ( (&nullNAStringRep_[3]) ) ; }

protected:
  Lng32 nullNAStringRep_[(sizeof(NAStringRef)+1)/sizeof(Lng32)+1+4] ; // +4 for good luck :-)
#endif
#ifndef __EID
  NABoolean sharedMemory_;
#endif
  // ---------------------------------------------------------------------
private:
#ifndef PRIV_SRL
  // the PRIV SRL will add a vtbl pointer for NAMemory because it
  // will see NABasicObject as an object without virtual functions.
  // Add this filler for the non-priv code so NAMemory will have
  // the same length for both PRIV and non-PRIV code.
  Lng32 fillerForVtblPtr_;
#endif
};


#ifdef _DEBUG
  struct TrafAddrStack {
    void *addr;  // allocated memory address
    size_t size; // size of stack trace entries, default 12
    char **strings; // stack back trace
  };
#endif // _DEBUG

////////////////////////////////////////////////////////////////////////////
// NAHeap is a NAMemory, which shrinks and grows.
////////////////////////////////////////////////////////////////////////////
class NAHeap : public NAMemory {
public:

  // Internal typedefs
  typedef UInt32 bindex_t;     // Index of a bin
  typedef UInt32 binmap_t;     // Bitmap of bins

  enum {
     NSMALLBINS = 32,
     NTREEBINS = 32
  };

  // see the corresponding NAMemory constructors for an explanation

NA_EIDPROC
  NAHeap();
NA_EIDPROC
  NAHeap(const char * name, 
	 NAHeap * parent, 
	 Lng32 blockSize = 0, 
	 Lng32 upperLimit =0);
NA_EIDPROC
  NAHeap(const char * name, NAMemoryType type = DERIVED_FROM_SYS_HEAP, 
         Lng32 blockSize = 0, Lng32 upperLimit = 0);
NA_EIDPROC
  NAHeap(const char  * name,
         SEG_ID   extFirstSegId,
	 void  * extFirstSegStart,
	 Lng32    extFirstSegOffset,
	 Lng32    extFirstSegLen,
	 Lng32    extFirstSegMaxLen,
	 NASegGlobals *segGlobals,
         Lng32    extMaxSecSegCount = NASegGlobals::NA_MAX_SECONDARY_SEGS);
NA_EIDPROC
  ~NAHeap();
NA_EIDPROC
  void destroy();
NA_EIDPROC
  void reInitializeHeap();
NA_EIDPROC
  void * allocateHeapMemory(size_t userSize, NABoolean failureIsFatal = TRUE);
NA_EIDPROC
  void deallocateHeapMemory(void * addr);
NA_EIDPROC
  void * allocateAlignedHeapMemory(size_t userSize, size_t alignment, NABoolean failureIsFatal = TRUE);

NA_EIDPROC
  void setErrorCallback(void (*errCallback)(NAHeap*,size_t));

#if (defined(_DEBUG) || defined(NSK_MEMDEBUG)) && !defined(__EID) && !defined(STAND_ALONE)
  NA_EIDPROC void dumpHeapInfo(ostream* outstream, Lng32 indent);
#endif

#if defined(NA_LINUX) && !defined(__EID)
// setThreadSafe must be called by the thread that creates the heap
// before other threads use it
  void setThreadSafe();
  const inline bool getThreadSafe() { return threadSafe_; }
#endif

#ifdef _DEBUG
  LIST(TrafAddrStack *) *getSAL() { return la_; }
#endif // _DEBUG

private:
NA_EIDPROC
  NABlock* allocateBlock(size_t size, NABoolean failureIsFatal);

  binmap_t smallmap_;         // Bitmap of small bins
  binmap_t treemap_;          // Bitmap of trees
  size_t   dvsize_;           // Designated victim size
  size_t   topsize_;          // The size of the topmost fragment.
  char*    least_addr_;       // Smallest address
  NAHeapFragment *dv_;        // Designated victim.  This is the preferred
                              // fragment for servicing small requests that
                              // don't have exact fits.  It is normally split
                              // off most recently to service another small
                              // request.  Its size is cached in dvsize.
                              // The link fields of this fragment are not
                              // maintained since it is not kept in a bin.
  NAHeapFragment  *top_;      // The topmost fragment of the currently active
                              // segment.  Its size is cached in topsize.
  NAHeapFragment  *smallbins_[(NSMALLBINS+1)*2];  // Array of bin headers for
                              // free fragments.  These bins hold fragments
                              // with sizes less than MIN_LARGE_SIZE bytes.
                              // Each bin contains fragments all of the same
                              // size spaced 8 bytes apart.  To simplify use
                              // in double-linked lists, each bin header acts
                              // as a NAHeapFragment pointing to the real
                              // first node if it exists (else pointing to
                              // itself).  This avoids special casing for
                              // headers.  But to avoid waste, we allocate
                              // only the next/prev pointers of bins, and
                              // then use repositioning tricks to treat 
                              // these as fields of a fragment.
  NATreeFragment  *treebins_[NTREEBINS]; // Array of pointers to the roots of
                              // the trees holding a range of sizes.  There
                              // are 2 equally spaced treebins for each power
                              // of two from TREE_SHIFT to TREE_SHIFT+16.
                              // The last bin holds anything larger.
#ifndef __EID
  MemoryStats dynamicAllocs;  // Statistics about memory allocations
#endif

  void (*errCallback_)(NAHeap*, size_t); // Pointer to a function that 
                              // will be called when an error occurs. The
                              // function can log the error or handle the
                              // error in an appropriate way.
#if defined(NA_LINUX) && !defined(__EID)
  bool threadSafe_;           // Heao is thread safe
  pthread_mutex_t mutex_;     // Mutex to serialize thread safe access
#endif

#ifdef _DEBUG
  LIST(TrafAddrStack *) *la_; // List of back traces when centain size
                              // of memory blocks got allocated
#endif // _DEBUG

public:
// Operations from Doug Lea's malloc implementation
  NA_EIDPROC static NABoolean isSmall(size_t size);
  NA_EIDPROC static bindex_t  smallIndex(size_t size);
  NA_EIDPROC static size_t    smallIndex2Size(bindex_t idx);

  NA_EIDPROC static bindex_t  computeTreeIndex(size_t size);
  NA_EIDPROC static size_t    minsizeForTreeIndex(bindex_t idx);

  NA_EIDPROC void releaseFreePages(); // release deallocted pages to kernel

#if (defined(_DEBUG) || defined(NSK_MEMDEBUG)) && !defined(__EID) 
  // useful method for debugging buffer overruns; sprinkle your code
  // with calls to this in order to narrow down where a buffer overrun
  // is occurring
  NA_EIDPROC void      doCheckMallocState();
#endif // (defined(_DEBUG) || defined(NSK_MEMDEBUG)) && !defined(__EID)

private:
  NA_EIDPROC static NABoolean isAligned(void *a);
  NA_EIDPROC static size_t    granularityAlign(size_t size);

  NA_EIDPROC NABoolean        okAddress(void *addr);
  NA_EIDPROC static NABoolean okNext(NAHeapFragment *p, NAHeapFragment *n);
  NA_EIDPROC static NABoolean okCinuse(NAHeapFragment *p);
  NA_EIDPROC static NABoolean okPinuse(NAHeapFragment *p);

  NA_EIDPROC static binmap_t idx2bit(bindex_t idx);
  NA_EIDPROC static bindex_t bit2idx(binmap_t x);

  NA_EIDPROC void      markSmallmap(bindex_t idx);
  NA_EIDPROC void      clearSmallmap(bindex_t idx);
  NA_EIDPROC NABoolean smallmapIsMarked(bindex_t idx);

  NA_EIDPROC void      markTreemap(bindex_t idx);
  NA_EIDPROC void      clearTreemap(bindex_t idx);
  NA_EIDPROC NABoolean treemapIsMarked(bindex_t idx);

  NA_EIDPROC static binmap_t leftBits(binmap_t bits);
  NA_EIDPROC static binmap_t leastBit(binmap_t bits);

  NA_EIDPROC static UInt32 leftshiftForTreeIndex(bindex_t idx);

  NA_EIDPROC NAHeapFragment* smallbinAt(bindex_t idx);
  NA_EIDPROC NATreeFragment** treebinAt(bindex_t idx);

  NA_EIDPROC void initBins();

  NA_EIDPROC void insertSmallFragment(NAHeapFragment *p, size_t size);
  NA_EIDPROC void unlinkSmallFragment(NAHeapFragment *p, size_t size);
  NA_EIDPROC void unlinkFirstSmallFragment(NAHeapFragment *b,
                     NAHeapFragment *p, bindex_t idx);

  NA_EIDPROC void insertLargeFragment(NATreeFragment *p, size_t size);
  NA_EIDPROC void unlinkLargeFragment(NATreeFragment *p);

  NA_EIDPROC void insertFragment(NAHeapFragment *p, size_t size);
  NA_EIDPROC void unlinkFragment(NAHeapFragment *p, size_t size);

  NA_EIDPROC void replaceDV(NAHeapFragment *p, size_t size);

  NA_EIDPROC void resizeTop(size_t newsize);
  NA_EIDPROC void initTop(NABlock *block);
  NA_EIDPROC void addBlock(NABlock* newBlock);

  NA_EIDPROC NABoolean deallocateFreeBlock(NAHeapFragment *p);

  NA_EIDPROC NAHeapFragment *tmallocLarge(size_t nb);
  NA_EIDPROC NAHeapFragment *tmallocSmall(size_t nb);

#if (defined(_DEBUG) || defined(NSK_MEMDEBUG)) && !defined(__EID) 
  NA_EIDPROC void      doAllocDebugProcess(NAHeapFragment *p, size_t userSize);
  NA_EIDPROC void      doDeallocDebugProcess(NAHeapFragment *p);
  NA_EIDPROC void      doCheckAnyFragment(NAHeapFragment *p);
  NA_EIDPROC void      doCheckTopFragment(NAHeapFragment *p);
  NA_EIDPROC void      doCheckInuseFragment(NAHeapFragment *p);
  NA_EIDPROC void      doCheckFreeFragment(NAHeapFragment *p);
  NA_EIDPROC void      doCheckMallocedFragment(NAHeapFragment *p, size_t s);
  NA_EIDPROC void      doCheckTree(NATreeFragment *t);
  NA_EIDPROC void      doCheckTreebin(bindex_t i);
  NA_EIDPROC void      doCheckSmallbin(bindex_t i);
  NA_EIDPROC NABoolean binFind(NAHeapFragment *x);
  NA_EIDPROC size_t    traverseAndCheck();
#ifndef STAND_ALONE
  NA_EIDPROC void checkForOverflow();
#endif // STAND_ALONE
#endif // (defined(_DEBUG) || defined(NSK_MEMDEBUG)) && !defined(__EID) 

#ifdef _DEBUG
  NA_EIDPROC void      setAllocTrace();
#endif // _DEBUG
};


//
// The NAHeapFragment is the same as the malloc_chunk in Doug Lea's malloc
// implementation.  The wording within this explanation uses the word
// "fragment" in place of the word "chunk" used within the explanation in
// Doug Lea's malloc.
//
// The NAHeapFragment declares a "view" into memory allowing access to
// necessary fields at known offsets from a given base.
//
// Fragments of memory are maintained using a `boundary tag' method as
// originally described by Knuth  Sizes of free fragments are stored both in the front of
// each fragment and at the end.  This makes consolidating fragmented 
// fragments into bigger fragments fast.  The head fields also hold bits
// representing whether fragments are free or in use.
//  
// Here are some pictures to make it clearer.  They are "exploded" to
// show that the state of a fragment can be thought of as extending from
// the high 31 bits of the head field of its header through the
// prevFoot and PINUSE_BIT bit of the following fragment header.
//  
// A fragment that's in use looks like:
//
//   fragment-> +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//              | Size of previous fragment (if P = 0)                          |
//              +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//            +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ |P|
//            | Size of this fragment                                      1| +-+
//      mem-> +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//            |                                                               |
//            +-                                                             -+
//            |                                                               |
//            +-                                                             -+
//            |                                                               |
//            +-      size - sizeof(size_t) available payload bytes          -+
//            |                                                               |
// fragment-> +-                                                             -+
//            |                                                               |
//            +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//          +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ |1|
//          | Size of next fragment (may or may not be in use)            | +-+
//    mem-> +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
//    And if it's free, it looks like this:
//
//   fragment-> +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//              | User payload (must be in use, or we would have merged!)       |
//              +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//            +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ |P|
//            | Size of this fragment                                      0| +-+
//      mem-> +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//            | Next pointer                                                  |
//            +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//            | Prev pointer                                                  |
//            +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//            |                                                               |
//            +-      size - sizeof(struct NAHeapFragment) unused bytes      -+
//            |                                                               |
// fragment-> +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//            | Size of this fragment                                         |
//          +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ |0|
//          | Size of next frag (must be in use, or we would have merged) | +-+
//    mem-> +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//          |                                                               |
//          +- User payload                                                -+
//          |                                                               |
//          +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//                                                                        |0|
//                                                                        +-+
// Note that since we always merge adjacent free fragments, the fragments
// adjacent to a free fragment must be in use.
//
// Given a pointer to a fragment (which can be derived trivially from the
// payload pointer) we can, in O(1) time, find out whether the adjacent
// fragments are free, and if so, unlink them from the lists that they
// are on and merge them with the current fragment.
//
// Fragments always begin on even word boundaries, so the mem portion
// (which is returned to the user) is also on an even word boundary, and
// thus at least double-word aligned.
//
// The P (PINUSE_BIT) bit, stored in the unused low-order bit of the
// fragment size (which is always a multiple of two words), is an in-use
// bit for the *previous* fragment.  If that bit is *clear*, then the
// word before the current fragment size contains the previous fragment
// size, and can be used to find the front of the previous fragment.
//
// Normally, if the pinuse bit is set for any given fragment, you CANNOT
// determine the size of the previous fragment.  However, the first
// NAHeapFragment in an NABlock will have both PINUSE_BIT and 
// FIRST_FRAGMENT_BIT set. For this special case, prevFoot_ will contain
// the usable size of the NABlock.  This helps to quickly determine
// whether the NABlock can be freed.
//
// The C (CINUSE_BIT) bit, stored in the unused second-lowest bit of
// the fragment size redundantly records whether the current fragment is
// inuse. This redundancy enables usage checks within free() and reduces
// indirection when freeing and consolidating fragments.
//
// Each freshly allocated fragment must have both cinuse and pinuse set.
// That is, each allocated fragment borders either a previously allocated
// and still in-use fragment, or the base of its memory arena. This is
// ensured by making all allocations from the the `lowest' part of any
// found fragment.  Further, no free fragment physically borders another
// one, so each free fragment is known to be preceded and followed by
// either inuse fragments or the ends of memory.
//
// The MMAPPED_BIT will be set in a NAHeapFragment if a single allocation
// was made that spans the entire NABlock and the NABlock was allocated
// using mmap() on POSIX systems or VirtualAlloc() on Windows. When
// the allocation is freed, the entire NABlock is freed back to the 
// operating system. No heap fragments are placed on the free lists when
// the MMAPPED_BIT is set in a NAHeapFragment. There is also a MMAP_BIT
// that may be set on a NABlock. NABlocks that have the MMAP_BIT behave
// identically to other NABlocks, but the memory will be freed back to
// the operating system when all memory is freed within the NABlock.
//
// Note that the `foot' of the current fragment is actually represented
// as the prevFoot of the NEXT fragment. This makes it easier to
// deal with alignments etc but can be very confusing when trying
// to extend or adapt this code.
//
// The exceptions to all this are
//   1. The special fragment `top' is the top-most available fragment
//      (i.e., the one bordering the end of available memory). It is
//      treated specially.  Top is never included in any bin, is used
//      only if no other fragment is available. In effect, the top
//      fragment is treated as larger (and thus less well fitting) than
//      any other available fragment.  The top fragment space is still
//      allocated for it (TOP_FOOT_SIZE) to enable separation or merging
//      when space is extended.
//
// There should be no virtual functions on the NAHeapFragment class.

class NAHeapFragment {
public:
  NA_EIDPROC void *getMemory(NABoolean cleanUpPrevNext=FALSE);
  NA_EIDPROC static NAHeapFragment *memToFragment(void *mem);

  NA_EIDPROC NABoolean cinuse();
  NA_EIDPROC NABoolean pinuse();
  NA_EIDPROC NABoolean cpinuse();
  NA_EIDPROC size_t fragmentSize();
  NA_EIDPROC void clearCinuse();
  NA_EIDPROC void clearPinuse();

  NA_EIDPROC void initializeFirstFragment(size_t s);
  NA_EIDPROC size_t getFirstFragmentBit();
  NA_EIDPROC void setHeadBits(size_t bits);
  NA_EIDPROC NABoolean occupiesCompleteNABlock();

  NA_EIDPROC const size_t* getHeadAddr();
  NA_EIDPROC void setNext(NAHeapFragment *next);
  NA_EIDPROC void setPrev(NAHeapFragment *prev);
  NA_EIDPROC NAHeapFragment* getNext();
  NA_EIDPROC NAHeapFragment* getPrev();
  NA_EIDPROC size_t getPrevFoot();
  NA_EIDPROC void adjustBlockSize(Lng32 s);
  NA_EIDPROC NAHeapFragment *fragmentPlusOffset(size_t s);
  NA_EIDPROC NAHeapFragment *fragmentMinusOffset(size_t s);
  NA_EIDPROC NAHeapFragment *nextFragment();
  NA_EIDPROC NAHeapFragment *prevFragment();
  NA_EIDPROC NABoolean nextPinuse();
  NA_EIDPROC void setFoot(size_t s);
  NA_EIDPROC void setSizeAndPinuseOfFreeFragment(size_t s);
  NA_EIDPROC void setFreeWithPinuse(size_t s, NAHeapFragment *next);
  NA_EIDPROC void setInuseAndPinuse(size_t s);
  NA_EIDPROC void setSize(size_t s);
  NA_EIDPROC void setSizeAndPinuse(size_t s);
  NA_EIDPROC void setSizeAndPinuseOfInuseFragment(size_t s);

  NA_EIDPROC NABoolean isFencePost();
  NA_EIDPROC void setFencePosts();

#ifdef HAVE_MMAP
  NA_EIDPROC NABoolean isMMapped();
  NA_EIDPROC void setSizeOfMMapFragment(size_t s);
#endif

#if (defined(_DEBUG) || defined(NSK_MEMDEBUG)) && !defined(__EID) && !defined(STAND_ALONE)
  NA_EIDPROC void checkBufferOverflow();
#endif // (defined(_DEBUG) || defined(NSK_MEMDEBUG)) && !defined(__EID) && !defined(STAND_ALONE)

  NA_EIDPROC void cleanFreePages(size_t fragSize); // mark free pages as "clean". 
  // Release unused/free memory in the heap back to the kernel
  NA_EIDPROC void releaseFreePages(NAHeapFragment *prev, 
                                   NAHeapFragment *next, 
                                   Int32 mergeFlags);

private:
  enum inUseBits {
    PINUSE_BIT = 1,     // Previous fragment is in use
    CINUSE_BIT = 2,     // Current fragment is in use
    MMAPPED_BIT = 4,    // This NABlock was allocated with MMAP and contains a single fragment.
#ifdef NA_64BIT
    FIRST_FRAGMENT_BIT = 0x8000000000000000,  // First fragment in NABlock
#else
    FIRST_FRAGMENT_BIT = 0x80000000,  // First fragment in NABlock
#endif
    INUSE_BITS = (PINUSE_BIT|CINUSE_BIT),
    FENCEPOST_HEAD = (INUSE_BITS|sizeof(size_t))
  };

  size_t    prevFoot_;     // Size of previous fragment (if free)
  size_t    head_;         // Size and inuse bits
  NAHeapFragment * next_;  // points to next fragment - only when free
  NAHeapFragment * prev_;  // points to previous fragment - only when free

  NAHeapFragment() {}      // Make constructor private because we never
                           // construct objects of this type.
};

// NATreeFragments are bitwise digital trees keyed on fragment sizes.
// They are the same as Doug Lea's malloc_tree_chunk structures. Because
// NATreeFragments are only for free fragments greater than 256 bytes,
// their size doesn't impose any constraints on user fragment sizes. Each
// node looks like the following:
//
//    fragment-> +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//               |        Size of previous fragment                          |
//               +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//       `head:' |        Size of fragment, in bytes                       |P|
//         mem-> +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//               |        Forward pointer to next fragment of same size      |
//               +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//               |        Back pointer to previous fragment of same size     |
//               +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//               |        Pointer to left child (child[0])                   |
//               +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//               |        Pointer to right child (child[1])                  |
//               +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//               |        Pointer to parent                                  |
//               +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//               |        bin index of this fragment                         |
//               +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//               |        Unused space                                       .
//               .                                                           |
//               +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//               +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//nextfragment-> +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//    `foot:'    |        Size of fragment, in bytes                         |
//               +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// Each tree holding treenodes is a tree of unique fragment sizes.  Fragments
// of the same size are arranged in a circularly-linked list, with only the
// oldest fragment (the next to be used, in our FIFO ordering) actually in the
// tree.  Tree members are distinguished by a non-null parent pointer.  If a
// fragment with the same size as an existing node is inserted, it is linked 
// off the existing node using pointers that work in the same way as the next
// and prev pointers of small fragments.
//
// Each tree contains a power of 2 sized range of fragment sizes.  The smallest
// is 0x100 <= x < 0x180.  Each tree is divided in half at each tree level,
// with the fragments in the smaller half of the range (0x100 < x < 0x140)
// for the top nose) in the left subtree and the larger half (0x140 <= x <0x180)
// in the right subtree.  This is, of course, done by inspecting individual
// bits.
// 
// Using these rules, each node's left subtree contains all smaller
// sizes than its right subtree.  However, the node at the root of each
// subtree has no particular ordering relationship to either.  (The
// dividing line between the subtree sizes is based on tree relation.)
// If we remove the last fragment of a given size from the interior of the
// tree, we need to replace it with a leaf node.  The tree ordering
// rules permit a node to be replaced by any leaf below it.
//
// The smallest fragment in a tree (a common operation in a best-fit
// allocator) can be found by walking a path to the leftmost leaf in
// the tree.  Unlike a usual binary tree, where we follow left child
// pointers until we reach a null, here we follow the right child
// pointer any time the left one is null, until we reach a leaf with
// both child pointers null. The smallest fragment in the tree will be
// somewhere along that path.
//
// The worst case number of steps to add, find, or remove a node is
// bounded by the number of bits differentiating fragments within
// bins. Under current bin calculations, this ranges from 6 up to 21
// (for 32 bit sizes) or up to 53 (for 64 bit sizes). The typical case
// is of course much better.

class NATreeFragment : public NAHeapFragment {
public:
  NA_EIDPROC NATreeFragment* getChild(UInt32 childNo);
  NA_EIDPROC NATreeFragment** getChildAddr(UInt32 childNo);
  NA_EIDPROC NATreeFragment* leftmostChild();
  NA_EIDPROC void setChild(Int32 childNo, NATreeFragment *p);
  NA_EIDPROC NATreeFragment* getParent();
  NA_EIDPROC void setParent(NATreeFragment *p);
  NA_EIDPROC NAHeap::bindex_t getIndex();
  NA_EIDPROC void setIndex(NAHeap::bindex_t idx);
  NA_EIDPROC short getFreedNSKMemory();
  NA_EIDPROC void setFreedNSKMemory(short value);

  NA_EIDPROC void recurseFreeMemory();
  NA_EIDPROC void releaseFreePages();

private:
  // The first four fields are in NAHeapFragment:
  //   size_t    prevFoot_;      // Size of previous fragment (if free)
  //   size_t    head_;          // Size and inuse bits
  //   NAHeapFragment *next_;    // points to next fragment - only when free
  //   NAHeapFragment *prev_;    // points to previous fragment - only when free

  NATreeFragment *child_[2];     // Left and right children
  NATreeFragment *parent_;       // Pointer to parent NATreeFragment
  short  index_;                 // Index into NAHeap.treebins_
  short  freedNSKMemory_;        // this memory has already been freed
};

// The DefaultIpcHeap is used for IPC only.  Destroying this heap will not
// cause all memory that is used by this heap to be destroyed.  This heap
// uses malloc() and free() to allocate memory and does not keep track of
// the allocations.
class DefaultIpcHeap : public CollHeap
{
public:
NA_EIDPROC
  DefaultIpcHeap() { derivedClass_= DEFAULTIPCHEAP_CLASS; }
NA_EIDPROC
  void * allocateIpcHeapMemory(size_t size, NABoolean failureIsFatal = TRUE);
NA_EIDPROC
  void deallocateIpcHeapMemory(void * buffer);
NA_EIDPROC
  ~DefaultIpcHeap();
NA_EIDPROC
  void destroy();
  // bogus fn needed simply because of inheritance
#if (defined(_DEBUG) || defined(NSK_MEMDEBUG)) && !defined(__EID) && !defined(STAND_ALONE)
NA_EIDPROC
  void dumpIpcHeapInfo(ostream* outstream, Lng32 indent);
#endif
};

#if ((defined(NA_NSK) || defined (NA_LINUX)) && (!defined(__EID)))
SEG_ID getStatsSegmentId();
#endif

extern SEG_ID gStatsSegmentId_;

#endif // NAMEMORY__H

