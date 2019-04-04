// ***********************************************************************
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
// ***********************************************************************
// -*-C++-*-
// ***********************************************************************
//
// File:         NAMemory.cpp
// Description:  The implementation of NAMemory classes
//
//
// Created:      12/13/96
// Language:     C++
//
//
// ***********************************************************************
//

// -----------------------------------------------------------------------
//  In July, 2006 replaced the allocation routines in this file 
//  with routines from the public domain malloc from Doug Lea. The Doug
//  Lea allocator is a better performer than the previous allocator and
//  prevents some of the bad memory fragmentation problems that existed
//  in the previous allocator.  For detailed information about the Doug
//  Lea allocator, do an Internet search on "Doug Lea malloc".
//
//  Most of the structures and function names in Doug Lea's allocator
//  are renamed to fit into the existing SQL/MX memory allocation
//  framework.  Most of the algorithms are the same, but are converted
//  from C functions and macros to C++ member functions that are often
//  inlined.
// -----------------------------------------------------------------------

#include <fstream>

#include <errno.h>
#include "seabed/fs.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <iostream>

#include "str.h"
#include "ComSpace.h"

#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>     // For mmap support
#include <seabed/ms.h>

#ifdef TRACE_DP2_MALLOCS  // Only works for NT.
#include <fstream>
#endif

#include "NAError.h"
#include "HeapLog.h"
#include "Platform.h"
#include "NAAssert.h"

#include "ComRtUtils.h"
#include "StmtCompilationMode.h"

#ifdef _DEBUG
//-------------------------------------------------------------------------//
// Using env variable TRACE_ALLOC_SIZE to record the call stacks when a 
// given size of memory block was allocated but not de-allocated later.
// The call stacks can be dumped to a file with name given by env variable
// TRACE_ALLOC_SIZE appended by the process id when the heap is destructed.
//
// The tracing/logging is done via backtrace() and backtrace_symbols()
// calls. The idea is when a specific size (specified by TRACE_ALLOC_SIZE)
// is intercepted at allocation, use backtrace() and backtrace_symbols()
// in saveTrafStack() to get the call stack at the time. The stack info is
// saved to a list in the heap. When such block is freed, the stack info of
// that block will be removed from the list. At the time when the heap is
// to be destroyed, it dumps whatever left in the list via dumpTrafStack()
// to a file or terminal.
//
// This can be combined with a memory debug method that detects memory
// overflow (set the env MEMDEBUG to 2, see checkForOverFlow()). Once
// memory overflow is found, use this tracing method to find where the
// allocation was done and check if the allocation is adequate.
//
// Note that env MEMDEBUG should not be set to 2 when tracing the
// allocation of specified size because the user specified size would
// be different comparing to the allocated size due to extra bytes needed
// for block overflow checking.
//
// As we may delete the entire heap without deallocating each individual
// block we ever allocated for some heaps, the trace file may contains
// blocks that are ok not to be explicitly deleted. One needs to separate
// these from the real issues.
//
// Limitation: we don't track allocations for blocks greater than the
// INT_MAX
//
// All related code are in common/NAMemory.h, common/NAMemory.cpp (this file),
// common/ComRtUtils.h, and common/ComRtUtils.cpp
//
// Todos:
//   1) Re-enabling Setting memory block to 0xfa after allocates and to 0xfc
//      after de-allocates when env MEMDEBUG is set to 2
//
//   2) Ensure existing memory debug methods specified by env MEMDEBUG
//      still work in multi-threaded environment
//-------------------------------------------------------------------------//
// static THREAD_P UInt32 TraceAllocSize = 0; ** defined in common/ComRtUtil.h
#endif // _DEBUG

const  UInt32 deallocTraceEntries = 4096;
struct DeallocTraceEntry
{
  void * begin_;
  void * end_;
};
UInt32 THREAD_P deallocTraceIndex = deallocTraceEntries - 1, deallocCount = 0;
THREAD_P DeallocTraceEntry (*deallocTraceArray)[deallocTraceEntries] = 0;


#include "NAMemory.h"
#ifdef _DEBUG
#include "Collections.h"
#endif // _DEBUG

class NAMutex
{
  bool threadSafe_;
  pthread_mutex_t *mutex_;
public:
  // Do not implement a default constructor. This will generate a link
  // error if anyone tries to call it.
  NAMutex();

  NAMutex(bool threadSafe, pthread_mutex_t *mutex)
    : threadSafe_(threadSafe),
      mutex_(mutex)
  {
    if (threadSafe_)
    {
      int rc = pthread_mutex_lock(mutex_);
      assert(rc == 0);
    }
  }
  ~NAMutex()
  {
    if (threadSafe_)
    {
      int rc = pthread_mutex_unlock(mutex_);
      assert(rc == 0);
    }
  }
};


#include "logmxevent.h"

#define NO_MERGE        0x0
#define BACKWARD_MERGE  0x1
#define FORWARD_MERGE   0x2

#include "logmxevent.h"

extern short getRTSSemaphore();     // Functions implemented in SqlStats.cpp
extern void releaseRTSSemaphore();
extern NABoolean checkIfRTSSemaphoreLocked();
extern void updateMemStats();
extern SB_Phandle_Type *getMySsmpPhandle();

//CollHeap *CTXTHEAP__ = NULL;     // a global, set by CmpContext ctor and used by
	              		 // Collections.cpp allocate method
const size_t DEFAULT_NT_HEAP_INIT_SIZE      = 524288; 
const size_t DEFAULT_NT_HEAP_MAX_SIZE       = (size_t)-1; // by default, heaps have no max size
const size_t DEFAULT_NT_HEAP_INCR_SIZE      = 524288;

// MIN_MMAP_ALLOC_SIZE defines the minimum user size that will be used to
// allocate a NABlock using mmap().
const size_t MIN_MMAP_ALLOC_SIZE = 524288;

// Ignore all HEAPLOG stuff if MUSE
#if defined(MUSE)
#undef HEAPLOG_ON
#undef HEAPLOG_OFF
#undef HEAPLOG_REINITIALIZE
#undef HEAPLOG_ADD_ENTRY
#undef HEAPLOG_DELETE_ENTRY
#define HEAPLOG_ON()
#define HEAPLOG_OFF()
#define HEAPLOG_REINITIALIZE(a)
#define HEAPLOG_ADD_ENTRY(a, b, c, d)
#define HEAPLOG_DELETE_ENTRY(a, b)
#endif 

// ****************************************************************************
// Numeric constants that are local to this file
// ****************************************************************************
const short  NA_FIRST_NONPRIV_SEG_ID        = 1600;
const short  NA_FIRST_IPC_SEG_ID            = 2150;
const size_t MEGABYTE                       = 1024 * 1024;
const size_t MAXFLATSEGSIZE                 = 128 * 1024 * 1024;
const size_t DEFAULT_THRESHOLD_BLOCK_COUNT  = 10;
const size_t DEFAULT_MAX_INCREMENT          = 4194304;

#if defined (NA_YOS) || defined (NA_YOS_SIMULATION)
#define FRAG_BYTE_ALIGN 16
#else
#define FRAG_BYTE_ALIGN 8
#endif
#define FRAG_ALIGN_MASK (FRAG_BYTE_ALIGN - 1)

#define HEAPFRAGMENT_SIZE sizeof(NAHeapFragment)
#define FRAGMENT_OVERHEAD sizeof(size_t)
#define MIN_FRAGMENT_SIZE ((HEAPFRAGMENT_SIZE + FRAG_ALIGN_MASK) & ~FRAG_ALIGN_MASK)

// PAD_REQUEST is used to calculated the needed fragment size based on
// a requested user size.  The "sizeof(size_t)" is added to the requested
// size to account for the "head_" field in the NAHeapSegment, and the
// sum is rounded up to the next byte aligned boundary (of either 8 or 16).
#define PAD_REQUEST(req) (((req) + sizeof(size_t) + FRAG_ALIGN_MASK) \
                          & ~FRAG_ALIGN_MASK)

// ALIGN_OFFSET is used to determine the alignment of the first fragment
// given the memory address A.  The memory address A is passed from the caller
// as the first location that memory could start.  If necessary, ALIGN_OFFSET
// will return a value that indicates how many additional bytes are needed
// to cause the memory to be aligned properly.
#define ALIGN_OFFSET(A) \
   ((((size_t)(A) & FRAG_ALIGN_MASK) == 0)? 0 :\
    ((FRAG_BYTE_ALIGN - ((size_t)(A) & FRAG_ALIGN_MASK)) & FRAG_ALIGN_MASK))

#define BLOCK_OVERHEAD (sizeof(NABlock) + 2 * sizeof(size_t))

#define MAX_REQUEST    (0x7fffffffffffff - FRAG_ALIGN_MASK - FRAGMENT_OVERHEAD)
#define MIN_REQUEST    (MIN_FRAGMENT_SIZE - FRAGMENT_OVERHEAD - 1)
#define SMALLBIN_SHIFT 3
#define TREEBIN_SHIFT  8

#define MIN_LARGE_SIZE (1 << TREEBIN_SHIFT)
#define MAX_SMALL_SIZE (MIN_LARGE_SIZE - 1)
#define MAX_SMALL_REQUEST (MAX_SMALL_SIZE - FRAG_ALIGN_MASK - FRAGMENT_OVERHEAD)
#define MAX_SIZE_T     (~(size_t)0)
#define SIZE_T_BITSIZE (sizeof(size_t) << 3)

#define CMFAIL ((char*)(MAX_SIZE_T))

// ****************************************************************************
// Some helper macros from Doug Lea's malloc.

#define MIN_SMALL_INDEX     (small_index(MIN_FRAGMENT_SIZE))

// addressing by index. See above about smallbin repositioning

#if ( defined(_DEBUG) || defined(NSK_MEMDEBUG) )  
#define RTCHECK(e)                  (e)
#define CORRUPTION_ERROR_ACTION     assert(0)
#define USAGE_ERROR_ACTION          assert(0)
#define allocDebugProcess(P, S)     HEAPLOG_ADD_ENTRY(P->getMemory(), \
                                       (Lng32)userSize, heapID_.heapNum, \
                                       getName()); \
                                    if (debugLevel_) doAllocDebugProcess(P,S);
#define deallocDebugProcess(P)      if (debugLevel_) doDeallocDebugProcess(P)
#define checkFreeFragment(P)        doCheckFreeFragment(P)
#define checkInuseFragment(P)       doCheckInuseFragment(P)
#define checkTopFragment(P)         doCheckTopFragment(P)
#define checkMallocedFragment(P,N)  doCheckMallocedFragment(P,N)
#define checkMallocState()          doCheckMallocState()
#else
// If debugging is not turned on, then don't do any verification
#define RTCHECK(e)                  (1)
#define CORRUPTION_ERROR_ACTION
#define USAGE_ERROR_ACTION
#define allocDebugProcess(P, S)
#define deallocDebugProcess(P)
#define checkFreeFragment(P)
#define checkInuseFragment(P)
#define checkTopFragment(P)
#define checkMallocedFragment(P,N)
#define checkMallocState()
#endif // !( defined(_DEBUG) || defined(NSK_MEMDEBUG) )  
// ****************************************************************************

#ifdef NA_YOS_SIMULATION
// On Windows NT, use malloc16() and free16() to aligned memory like it
// is on NSK.
void * malloc16(Int32 size)
{
  Lng32 startAddr, retAddr, *saveStartAddr;
  startAddr = (Lng32)malloc(size + 20);
  if (startAddr == 0)
    return (void *) NULL;
  retAddr = startAddr + 4;
  retAddr = (((retAddr - 1) / 16 + 1) * 16);
  saveStartAddr = (Lng32 *)(retAddr - 4);
  *saveStartAddr = startAddr;
  return (void *)retAddr;
}

void free16(void *retAddr)
{
  void ** startAddr = (void **)((Lng32)retAddr - 4);
  free(*startAddr);
}

// Macros to use malloc16() and free16()
#define malloc(x) malloc16(x)
#define free(x) free16(x)

#endif // NA_YOS_SIMULATION

#include "muse.cpp"


NABlock::NABlock() :
  segmentId_(0),
  size_(0),
  next_(0),
  sflags_(0)
{}

// Does this NABlock contain the address?
inline NABoolean
NABlock::blockHolds(void *addr)
{
  return ((addr >= (void*)this) && ((char*)addr < (char*)this + size_));
}

// Find the NABlock that contains an address.
inline NABlock*
NABlock::blockHolding(NABlock *firstBlock, void *addr)
{
  while (firstBlock && !firstBlock->blockHolds(addr)) {
    firstBlock = firstBlock->next_;
  }
  return firstBlock;
}

// Return whether this NABlock allocated externally.  This only occurs
// for EXECUTOR_MEMORY when a segment is allocated externally to the
// NAMemory code and passed to the NAMemory within the NAHeap constructor.
// An externally allocated segment will never be freed.
inline NABoolean
NABlock::isExternSegment()
{
  return (sflags_ & EXTERN_BIT) != 0;
}

// Return whether this NABlock was allocated using mmap().
inline NABoolean
NABlock::isMMapped()
{
  return (sflags_ & MMAP_BIT) != 0;
}

// Return the offset of the first fragment in the block
inline size_t
NABlock::firstFragmentOffset()
{
  NAHeapFragment *p = (NAHeapFragment*)((char*)this + sizeof(NABlock));

  // Align the user memory
  return sizeof(NABlock) + ALIGN_OFFSET(p->getMemory());
}

// Return a pointer to the first fragment for this NABlock.
inline NAHeapFragment*
NABlock::alignAsFragment()
{
  return (NAHeapFragment*)((char*)this + firstFragmentOffset());
}

// Return the usable memory used for this NAHeapFragment.
inline void*
NAHeapFragment::getMemory(NABoolean cleanUpPrevNext)
{
  // Add 2 size_t sizes to account for prevFoot_ and head_ at
  // the beginning of the NAHeapFragment.
  Long *memPtr = (Long*)((char*)this + (sizeof(size_t) * 2));
  if (cleanUpPrevNext)
  {
    // clean up prev_ and next_ pointers before handing this 
    // memory to the user.
    memPtr[0] = 0;
    memPtr[1] = 0;
  }
  return (void *)memPtr;
}

// Convert usable memory pointer to an NAHeapFragment pointer.
inline NAHeapFragment*
NAHeapFragment::memToFragment(void *mem)
{
  // Subtract 2 size_t sizes to account for prevFoot_ and head_ at
  // the beginning of the NAHeapFragment.
  return (NAHeapFragment*)((char*)mem - (sizeof(size_t) * 2));
}

// Return whether the CINUSE_BIT (current fragment) is set.
inline NABoolean
NAHeapFragment::cinuse()
{
  return (head_ & CINUSE_BIT) != 0;
}

// Return whether the previous fragment is in use.
inline NABoolean
NAHeapFragment::pinuse()
{
  return (head_ & PINUSE_BIT) != 0;
}

// Return whether both PINUSE_BIT and CINUSE_BIT bits are set.
inline NABoolean
NAHeapFragment::cpinuse()
{
  return (head_ & INUSE_BITS) == INUSE_BITS;
}

// Return the size of the NAHeapFragment
inline size_t
NAHeapFragment::fragmentSize()
{
  return head_ & ~(INUSE_BITS|MMAPPED_BIT|FIRST_FRAGMENT_BIT);
}

// Clear CINUSE_BIT.
inline void 
NAHeapFragment::clearCinuse()
{
  head_ &= ~CINUSE_BIT;
}
  
// Clear PINUSE_BIT.
inline void
NAHeapFragment::clearPinuse()
{
  head_ &= ~PINUSE_BIT;
}

// Set the FIRST_FRAGMENT_BIT and prevFoot_ for the first fragment
// in an NABlock. The FIRST_FRAGMENT_BIT is used to determine quickly
// whether this fragment occupies an entire NABlock during deallocation.
inline void
NAHeapFragment::initializeFirstFragment(size_t s)
{
  head_ |= FIRST_FRAGMENT_BIT;
  prevFoot_ = s;
}

// Return the status of FIRST_FRAGMENT_BIT.
inline size_t
NAHeapFragment::getFirstFragmentBit()
{
  return head_ & FIRST_FRAGMENT_BIT;
}

// Set the head bits of a fragment by ORing them.
inline void
NAHeapFragment::setHeadBits(size_t bits)
{
  head_ |= bits;
}

// Does this fragment occupy the whole NABlock?
inline NABoolean
NAHeapFragment::occupiesCompleteNABlock()
{
  return ((head_ & FIRST_FRAGMENT_BIT) != 0
          && prevFoot_ == fragmentSize());
}

// Return the address of the header.
inline const size_t*
NAHeapFragment::getHeadAddr()
{
  return &head_;
}

// Set the "next_" pointer in the NAHeapFragment
inline void
NAHeapFragment::setNext(NAHeapFragment *next)
{
  next_ = next;
}

// Set the "prev_" pointer in the NAHeapFragment
inline void
NAHeapFragment::setPrev(NAHeapFragment *prev)
{
  prev_ = prev;
}

// Return the "next_" pointer.
inline NAHeapFragment*
NAHeapFragment::getNext()
{
  return next_;
}

// Return the "prev_" pointer.
inline NAHeapFragment*
NAHeapFragment::getPrev()
{
  return prev_;
}

// Return the size of the previous fragment (only valid if free).
inline size_t
NAHeapFragment::getPrevFoot()
{
  return prevFoot_;
}

// Adjust the size of the previous footer size.  For the first fragment
// in an NABlock, the prevFoot_ is used to indicate the size of the block.
// This function is only used to adjust the size of the prevFoot_ when the
// block is resized.
inline void
NAHeapFragment::adjustBlockSize(Lng32 s)
{
  prevFoot_ += s;
}

// Return pointer offset as a greater NAHeapFragment pointer
inline NAHeapFragment*
NAHeapFragment::fragmentPlusOffset(size_t s)
{
  return (NAHeapFragment*)((char*)this + s);
}

// Return pointer offset as a smaller NAHeapFragment pointer
inline NAHeapFragment*
NAHeapFragment::fragmentMinusOffset(size_t s)
{
  return (NAHeapFragment*)((char*)this - s);
}

// Return a pointer to the next fragment
inline NAHeapFragment*
NAHeapFragment::nextFragment()
{
  return (NAHeapFragment*)((char*)this 
          + (head_ & ~(INUSE_BITS|MMAPPED_BIT|FIRST_FRAGMENT_BIT)));
}

// Return a pointer to the previous fragment.  This code assumes
// that the caller has checked whether the previous fragment is
// not in use.
inline NAHeapFragment*
NAHeapFragment::prevFragment()
{
  return (NAHeapFragment*)((char*)this - prevFoot_);
}

// Return whether the next fragment has PINUSE_BIT set.
inline NABoolean
NAHeapFragment::nextPinuse()
{
  return nextFragment()->pinuse();
}

// Set the size at the footer.
inline void
NAHeapFragment::setFoot(size_t s)
{
  size_t size = s & ~FIRST_FRAGMENT_BIT;
  ((NAHeapFragment*)((char*)this + size))->prevFoot_ = s;
}

// Set the size, pinuse bit, and the size at the footer
inline void
NAHeapFragment::setSizeAndPinuseOfFreeFragment(size_t s)
{
  head_ = (s|PINUSE_BIT);
  setFoot(s);
}

// Set size, pinuse bit, foot, and clear next pinuse
inline void
NAHeapFragment::setFreeWithPinuse(size_t s, NAHeapFragment *next)
{
  next->clearPinuse();
  setSizeAndPinuseOfFreeFragment(s);
}

// Set size, inuse and pinuse of this fragment and pinuse of next
// fragment.
inline void
NAHeapFragment::setInuseAndPinuse(size_t s)
{
  head_ = (s|PINUSE_BIT|CINUSE_BIT);
  size_t size = (s & ~FIRST_FRAGMENT_BIT);
  ((NAHeapFragment*)((char*)this + size))->head_ |= PINUSE_BIT;
}

// Set head to the size passed in.  No inuse bits will be set.
inline void
NAHeapFragment::setSize(size_t s)
{
  head_ = s;
}

// Set the size and pinuse bits
inline void
NAHeapFragment::setSizeAndPinuse(size_t s)
{
  head_ = (s|PINUSE_BIT);
}

// Set size, cinuse and pinuse bit of this fragment.
inline void
NAHeapFragment::setSizeAndPinuseOfInuseFragment(size_t s)
{
  head_ = (s|PINUSE_BIT|CINUSE_BIT);
}

// Return whether the head is a fencepost
inline NABoolean
NAHeapFragment::isFencePost()
{
  return (head_ == FENCEPOST_HEAD ||
          (head_ == (sizeof(size_t) | CINUSE_BIT) &&
           (fragmentPlusOffset(sizeof(size_t)))->head_ == FENCEPOST_HEAD));
}

// Set fenceposts after the top fragment
inline void
NAHeapFragment::setFencePosts()
{
  head_ = sizeof(size_t) | CINUSE_BIT;
  NAHeapFragment *next = fragmentPlusOffset(sizeof(size_t));
  next->head_ = FENCEPOST_HEAD;
}

// Return whether the MMAPPED_BIT is set. This bit is only set when a
// NABlock contains a single large fragment and the NABlock was allocated
// using mmap().
inline NABoolean
NAHeapFragment::isMMapped()
{
  return (head_ & MMAPPED_BIT) != 0;
}

// Set the size in the head with the MMAP bit and initialize the trailing
// fenceposts.
inline void
NAHeapFragment::setSizeOfMMapFragment(size_t size)
{
  head_ = (size|PINUSE_BIT|CINUSE_BIT|MMAPPED_BIT); 
  ((NAHeapFragment*)((char*)this + size))->head_ = FENCEPOST_HEAD;
  ((NAHeapFragment*)((char*)this + size + sizeof(size_t)))->head_ = FENCEPOST_HEAD;
}

// mark free pages in this fragment as "non-dirty". Benefits are:
// 1) pages will become readily stealable if needed by the OS. 
// 2) When stolen, OS will not have to swap the contents to the disk. 
inline void
NAHeapFragment::cleanFreePages(size_t fragSize)
{
}

// Get either the left or right child of an NATreeFragment
inline NATreeFragment*
NATreeFragment::getChild(UInt32 childNo)
{
  return child_[childNo];
}

// Get address of either the left or right child of an NATreeFragment
inline NATreeFragment**
NATreeFragment::getChildAddr(UInt32 childNo)
{
  return &child_[childNo];
}

// Get the leftmost child of an NATreeFragment
inline NATreeFragment*
NATreeFragment::leftmostChild()
{
  return (child_[0] != 0 ? child_[0] : child_[1]);
} 

// Set either the left or right child of an NATreeFragment
inline void
NATreeFragment::setChild(Int32 childNo, NATreeFragment *p)
{
  child_[childNo] = p;
}

// Get the parent of an NATreeFragment
inline NATreeFragment*
NATreeFragment::getParent()
{
  return parent_;
}

// Set the parent of an NATreeFragment
inline void
NATreeFragment::setParent(NATreeFragment *p)
{
  parent_ = p;
}

// Return the index of an NATreeFragment.  This is the index into
// NAHeap.treebins_[].
inline NAHeap::bindex_t
NATreeFragment::getIndex()
{
  return index_;
}

// Set the index of the NATreeFragment.
inline void
NATreeFragment::setIndex(NAHeap::bindex_t idx)
{
  index_ = idx;
}

// Get/Set the freedNSKMemory_ flag

inline short
NATreeFragment::getFreedNSKMemory()
{
  return freedNSKMemory_;
}

inline void 
NATreeFragment::setFreedNSKMemory(short value)
{
  freedNSKMemory_ = value;
}

  Lng32 NAMemory::getVmSize()
  {
    pid_t myPid;
    char fileName[32], buffer[1024], *currPtr;
    size_t bytesRead;
    Int32 success;
    ULng32 memSize; // VMSize in KB
    if (procStatusFile_ == 0)
    {
      myPid = getpid();
      sprintf(fileName, "/proc/%d/status", myPid);
      procStatusFile_ = fopen(fileName, "r");
      static char *envVar = getenv("EXECUTOR_VM_RESERVE_SIZE");
      if (envVar)
        executorVmReserveSize_ = atol(envVar);
      else
        executorVmReserveSize_ = 256;
    }
    else
      success = fseek(procStatusFile_, 0, SEEK_SET);
    bytesRead = fread(buffer, 1, 1024, procStatusFile_);
    currPtr = strstr(buffer, "VmSize");
    sscanf(currPtr, "%*s %u kB", &memSize);
    lastVmSize_ = memSize;
    if (memSize > maxVmSize_)
      maxVmSize_ = memSize;
    return memSize;
  }

  void NAMemory::allocationIncrement(Lng32 increment)
  {
    allocationDelta_ += increment;
    if (allocationDelta_ >= 128 * 1024 * 1024)
    {
      allocationDelta_ = 0ll;
      Lng32 vmSize = getVmSize();
      if (vmSize >= (4 * 1024 * 1024) - ((128 + executorVmReserveSize_) * 1024))
        crowdedTotalSize_ = (Int64)vmSize * 1024;
    }
  }

  void NAMemory::allocationDecrement(Lng32 decrement)
  {
    allocationDelta_ -= decrement;
    if (allocationDelta_ <= -128 * 1024 * 1024)
    {
      allocationDelta_ = 0ll;
      Lng32 vmSize = getVmSize();
      if (vmSize < (4 * 1024 * 1024) - ((128 + executorVmReserveSize_) * 1024))
       crowdedTotalSize_ = 0ll;
    }
  }

NABlock*
NAMemory::allocateMMapBlock(size_t s)
{
  NABlock *blk;
  blk = (NABlock*)mmap(0, s, (PROT_READ|PROT_WRITE),
                       (MAP_PRIVATE|MAP_ANONYMOUS), -1, 0);
  if (blk == (NABlock*)-1)
  {
    mmapErrno_ = errno;
    return NULL;
  }
  else
    allocationIncrement(s);

  // one more block allocated
  blockCnt_++;
  totalSize_+= s;

  blk->segmentId_ = 0;
  blk->size_ = s;
  blk->sflags_ = NABlock::MMAP_BIT;

  return blk;
}

// Free a NABlock back to the operating system using munmap() or the equivalent.
// The caller should have already modified the statistics on NAMemory before
// calling this function.
inline void
NAMemory::deallocateMMapBlock(NABlock *blk)
{
  allocationDecrement(blk->size_);
  Int32 munmapRetVal;
  munmapRetVal = munmap((void*)blk, blk->size_);
  if (munmapRetVal == -1)
    munmapErrno_ = errno;
}

// Either the correct call to free the memory used by a NABlock. This function
// prevents a lot of extra #ifdef code in later sections of code.
inline void
NAMemory::sysFreeBlock(NABlock *blk)
{
  if (blk->isMMapped())
    deallocateMMapBlock(blk);
  else
    free((void*)blk);
}

#ifndef MUSE
NAMemory::NAMemory(const char * name)
     : 
    type_(EXECUTOR_MEMORY),
    maximumSize_((size_t)-1),            // no maximum
    parent_(NULL),
    firstBlk_(NULL),
    allocSize_(0),
    upperLimit_(0),
    highWaterMark_(0),
    intervalWaterMark_(0),
    allocCnt_(0),
    totalSize_(0),
    blockCnt_(0),
    thBlockCnt_(DEFAULT_THRESHOLD_BLOCK_COUNT),
    memoryList_(NULL),
    lastListEntry_(NULL),
    nextEntry_(NULL),
    exhaustedMem_(FALSE),
    errorsMask_(0),
    crowdedTotalSize_(0ll)
    , allocationDelta_(0ll)
    , procStatusFile_(NULL)
    , mmapErrno_(0)
    , munmapErrno_(0)
    , lastVmSize_(0l)
    , maxVmSize_(0l)
    , sharedMemory_(FALSE)
    , heapStartAddr_(NULL)
    , heapStartOffset_(NULL)
{
  setType(type_, 0);
#if ( defined(_DEBUG) || defined(NSK_MEMDEBUG) )  
  char * debugLevel = getenv("MEMDEBUG");
  if (debugLevel)
    debugLevel_ = (Lng32)atoi(debugLevel);
  else
    debugLevel_ = 0;
#else // Release build with no debugging
  debugLevel_ = 0;
#endif
  if (name != NULL)
    setName(name);
  else
    setName("Unnamed memory");

  // need to initialize an NAStringRef object "on top" of the array
  // (don't touch this unless you know what you're doing!)
  NAStringRef * tmp = 
    new ( (void*) (&nullNAStringRep_[3]) ) 
    NAStringRef (NAStringRef::NULL_CTOR, this) ;
}

NAMemory::NAMemory(const char * name, NAHeap * parent, size_t blockSize,
                   size_t upperLimit)
     : 
   type_(DERIVED_MEMORY),
   maximumSize_((size_t)-1),            // no maximum
   parent_(parent),
   firstBlk_(NULL),
   allocSize_(0),
   upperLimit_(upperLimit),
   highWaterMark_(0),
   intervalWaterMark_(0),
   allocCnt_(0),
   totalSize_(0),
   blockCnt_(0),
   thBlockCnt_(DEFAULT_THRESHOLD_BLOCK_COUNT),
   memoryList_(NULL),
   lastListEntry_(NULL),
   nextEntry_(NULL),
   exhaustedMem_(FALSE),
   errorsMask_(0),
    crowdedTotalSize_(0ll)
    , allocationDelta_(0ll)
    , procStatusFile_(NULL)
    , mmapErrno_(0)
    , munmapErrno_(0)
    , lastVmSize_(0l)
    , maxVmSize_(0l)
    , sharedMemory_(FALSE)
    , heapStartAddr_(NULL)
    , heapStartOffset_(NULL)
{
  if (parent_->getSharedMemory())
     setSharedMemory();
  // a derived memory has to have a parent from which it is derived
  assert(parent_);
#if ( defined(_DEBUG) || defined(NSK_MEMDEBUG) )
  char * debugLevel = getenv("MEMDEBUG");
  if (debugLevel)
    debugLevel_ = (Lng32)atoi(debugLevel);
  else
    debugLevel_ = 0;
#else // Release build with no debugging
  debugLevel_ = 0;
#endif
  setName(name);

  if (blockSize <= 0)
    // illegal block size. Use default of 0.5 MB (adjusted).
    blockSize = (Lng32)524288;
  initialSize_ = incrementSize_ = blockSize;

  // need to initialize an NAStringRef object "on top" of the array
  // (don't touch this unless you know what you're doing!)
  NAStringRef * tmp = 
    new ( (void*) (&nullNAStringRep_[3]) ) 
    NAStringRef (NAStringRef::NULL_CTOR, this) ;
}

NAMemory::NAMemory(const char * name, NAMemoryType type, size_t blockSize,
                   size_t upperLimit)
     : 
    type_(type),
    maximumSize_((size_t)-1),            // no maximum
    parent_(NULL),
    firstBlk_(NULL),
    allocSize_(0),
    upperLimit_(upperLimit),
    highWaterMark_(0),
    intervalWaterMark_(0),
    allocCnt_(0),
    totalSize_(0),
    blockCnt_(0),
    thBlockCnt_(DEFAULT_THRESHOLD_BLOCK_COUNT),
    memoryList_(NULL),
    lastListEntry_(NULL),
    nextEntry_(NULL),
    exhaustedMem_(FALSE),
    errorsMask_(0),
    crowdedTotalSize_(0ll)
    , allocationDelta_(0ll)
    , procStatusFile_(NULL)
    , mmapErrno_(0)
    , munmapErrno_(0)
    , lastVmSize_(0l)
    , maxVmSize_(0l)
    , sharedMemory_(FALSE)
    , heapStartAddr_(NULL)
    , heapStartOffset_(NULL)
{
  // call setType to initialize the values of all the sizes
  setType(type_, blockSize);
#if ( defined(_DEBUG) || defined(NSK_MEMDEBUG) ) 
  char * debugLevel = getenv("MEMDEBUG");
  if (debugLevel)
    debugLevel_ = (Lng32)atoi(debugLevel);
  else
    debugLevel_ = 0;
#else // Release build with no debugging
  debugLevel_ = 0;
#endif
  setName(name);

  // need to initialize an NAStringRef object "on top" of the array
  // (don't touch this unless you know what you're doing!)
  NAStringRef * tmp = 
    new ( (void*) (&nullNAStringRep_[3]) ) 
    NAStringRef (NAStringRef::NULL_CTOR, this) ;
}

NAMemory::NAMemory(const char * name,
           SEG_ID  segmentId,
           void  * baseAddr,
           off_t   heapStartOffset,
           size_t  maxSize)
     : 
    type_(EXECUTOR_MEMORY),
    parent_(NULL),
    firstBlk_(NULL),
    allocSize_(0),
    upperLimit_(0),
    highWaterMark_(0),
    intervalWaterMark_(0),
    allocCnt_(0),
    totalSize_(0),
    blockCnt_(0),
    thBlockCnt_(DEFAULT_THRESHOLD_BLOCK_COUNT),
    memoryList_(NULL),
    lastListEntry_(NULL),
    nextEntry_(NULL),
    exhaustedMem_(FALSE),
    errorsMask_(0),
    crowdedTotalSize_(0ll)
    , allocationDelta_(0ll)
    , procStatusFile_(NULL)
    , mmapErrno_(0)
    , munmapErrno_(0)
    , lastVmSize_(0l)
    , maxVmSize_(0l)
    , sharedMemory_(FALSE)
    , heapStartOffset_(heapStartOffset)
    , heapStartAddr_(baseAddr)
{
  // call setType to initialize the values of all the sizes
  setType(type_, 0);

#if ( defined(_DEBUG) || defined(NSK_MEMDEBUG) ) 
  char * debugLevel = getenv("MEMDEBUG");
  if (debugLevel)
    debugLevel_ = (Lng32)atoi(debugLevel);
  else
    debugLevel_ = 0;
#else // Release build with no debugging
  debugLevel_ = 0;
#endif

  setName(name);

  // If a first segment was passed in and there is anough usable
  // space in the segment, then initialize the firstBlk_ within
  // the passed in memory.  The NAHeap constructor will initialize
  // the top NAHeapFragment.
  if (heapStartAddr_ != NULL) {
    blockCnt_ = 1;
    size_t tsize = maxSize - heapStartOffset_ - BLOCK_OVERHEAD;
    if (tsize > (8 * sizeof(size_t))) {
      firstBlk_ = (NABlock*)((char*)heapStartAddr_ + heapStartOffset_);
      firstBlk_->size_ = maxSize - heapStartOffset_;
      firstBlk_->sflags_ = NABlock::EXTERN_BIT;
      firstBlk_->next_ = NULL;
      firstBlk_->segmentId_ = segmentId;
      totalSize_ = initialSize_ = maximumSize_ = firstBlk_->size_;
    }
  }
  upperLimit_ = maxSize;
  // need to initialize an NAStringRef object "on top" of the array
  // (don't touch this unless you know what you're doing!)
  NAStringRef * tmp = 
    new ( (void*) (&nullNAStringRep_[3]) ) 
    NAStringRef (NAStringRef::NULL_CTOR, this) ;
}


void NAMemory::reInitialize()
{
  // delete all blocks allocated for this heap and re-set the heap
  // to the initial values

  HEAPLOG_REINITIALIZE(heapID_.heapNum)

  NABlock *externSegment = NULL;
  NABlock *p = firstBlk_;

  if (p != NULL) {

    switch (type_) {
    case EXECUTOR_MEMORY:
    case SYSTEM_MEMORY: 
    case IPC_MEMORY:
      {
        while (p) {
          assert(!p->isExternSegment());
          NABlock *q = p->next_;
          sysFreeBlock(p);
          p = q;
        }
      }
      break;
    case DERIVED_FROM_SYS_HEAP:
      {
        while (p) {
          NABlock *q = p->next_;
          sysFreeBlock(p);
          p = q;
        }
      }
      break;

    case DERIVED_MEMORY:
      {
        // make sure that we have a parent
        assert(parent_);
        HEAPLOG_OFF() // no recursive logging. (eric)

        // This code provides mutual exclusion for the runtime stats shared
        // memory segment.
        short semRetcode = 0;
        if (parent_->getType() == EXECUTOR_MEMORY && getSharedMemory()) {
          semRetcode = getRTSSemaphore();
        }
        while (p) {
          NABlock *q = p->next_;
          parent_->deallocateHeapMemory((void*)p);
          p = q;
        }
        if (semRetcode == 1)
          releaseRTSSemaphore();
        if (parent_->type_ == NAMemory::EXECUTOR_MEMORY &&
            parent_->parent_ == NULL)
        {
            updateMemStats();
        }
        HEAPLOG_ON()
      }
      break;
    default:
      assert(0);
    } // switch(_type)
  } // if (p != NULL)

  // reinitialize all data members.
  allocSize_ = highWaterMark_ = intervalWaterMark_ = allocCnt_ = 
    totalSize_ = blockCnt_ = 0;
    crowdedTotalSize_ = 0ll;
  thBlockCnt_ = DEFAULT_THRESHOLD_BLOCK_COUNT;
  incrementSize_ = initialSize_;

  if (externSegment == NULL) {
    firstBlk_ = NULL;
  } else {
    // This code is not executed by the current code, but exists so
    // it would be possible to call reInitialize() on the EXECUTOR_MEMORY
    // or any other type of memory that is allocated externally. The code
    // in NAHeap::reInitializeHeap() will make the necessary calls to
    // reinitialize the top fragment.
    firstBlk_ = externSegment;
    firstBlk_->next_ = NULL;
    blockCnt_ = 1;
    totalSize_ = firstBlk_->size_ - heapStartOffset_;
  }

  // If this is an NAHeap, then call reInitializeHeap() to reinitialize
  // the NAHeap fields.  This must be called after reInitializing the
  // NAMemory fields because code in reInitializeHeap() depends on setting
  // firstBlk_ correctly.
  if (derivedClass_ == NAHEAP_CLASS)
    ((NAHeap*)this)->reInitializeHeap();
}

NAMemory::~NAMemory()
{

}

void NAMemory::setType(NAMemoryType type, Lng32 blockSize)
{
  // upperLimit_ must be zero for EXECUTOR_MEMORY, IPC_MEMORY, SYSTEM_MEMORY,

  type_ = type;

  parent_ = NULL;

  // whenever we make changes to the following initial, increment, and
  // maximum sizes, we should make sure, that all values are divisible
  // by FRAG_BYTE_ALIGN. NAMemory::NAMemory also guarantees this. The
  // only exeption from this rule is EXECUTOR_MEMORY on NSK described
  // below. Also, if we have a maximumSize_, make sure that
  // initialSize_ + n * incrementSize_ = maximumSize_

  switch(type_) {
  case EXECUTOR_MEMORY:
    initialSize_   = DEFAULT_NT_HEAP_INIT_SIZE ; 
    maximumSize_   = DEFAULT_NT_HEAP_MAX_SIZE ;           // no maximum
    incrementSize_ = DEFAULT_NT_HEAP_INCR_SIZE ;
    break;
	
  case SYSTEM_MEMORY:
  case IPC_MEMORY:
    // just regular memory in any other environment
    initialSize_   = DEFAULT_NT_HEAP_INIT_SIZE ; 
    maximumSize_   = DEFAULT_NT_HEAP_MAX_SIZE ;           // no maximum
    incrementSize_ = DEFAULT_NT_HEAP_INCR_SIZE ;
    break;

  case DERIVED_MEMORY: {
    assert(parent_);
    if (blockSize == 0)
      blockSize = (Lng32)32768;
    initialSize_ = incrementSize_ = blockSize;
    maximumSize_ = (size_t)-1;            // no maximum
  }
  break;

  case DERIVED_FROM_SYS_HEAP: {
    // just regular memory in all environments. Even on NSK. Use 32K
    // if no blockSize is given
    if (blockSize == 0)
      blockSize = (Lng32)32768;
    initialSize_ = incrementSize_ = blockSize;
    maximumSize_ = (size_t)-1;            // no maximum
  }
  break;
  default:
    assert(0);
  }
}

// allocateMemory() calls the appropriate function for allocating memory based
// on the derivedClass.  This function provides similar functionality to virtual
// functions, which don't work correctly with runtime stats when the NAMemory
// objects exist in shared memory.
void * NAMemory::allocateMemory(size_t size, NABoolean failureIsFatal)
{
  switch (derivedClass_)
  {
  case NAHEAP_CLASS:
    return ((NAHeap *)this)->allocateHeapMemory(size, failureIsFatal);
  case COMSPACE_CLASS:
    return ((ComSpace *)this)->allocateSpaceMemory(size, failureIsFatal);
  case DEFAULTIPCHEAP_CLASS:
    return ((DefaultIpcHeap *)this)->allocateIpcHeapMemory(size, failureIsFatal);
  default:
    assert(0);
  }
  return NULL;
}

// deallocateMemory() frees memory based on the type of memory class this is.
// Virtual functions are avoided due to problems with runtime stats placing
// NAHeap objects within a shared memory segment.
void NAMemory::deallocateMemory(void* addr)
{
  switch (derivedClass_)
  {
  case NAHEAP_CLASS:
    ((NAHeap *)this)->deallocateHeapMemory(addr);
    break;
  case COMSPACE_CLASS:
    ((ComSpace *)this)->deallocateSpaceMemory(addr);
    break;
  case DEFAULTIPCHEAP_CLASS:
     ((DefaultIpcHeap *)this)->deallocateIpcHeapMemory(addr);
    break;
  default:
    assert(0);
  }
}

// dump() writes memory statistics to an output file that the caller must open.
// It is used as an aid for debugging memory problems and for saving memory
// statistics during certain tests.
#if (defined(_DEBUG) || defined(NSK_MEMDEBUG))
void NAMemory::dump(ostream* outstream, Lng32 indent)
{
  switch (derivedClass_)
  {
  case NAHEAP_CLASS:
    ((NAHeap *)this)->dumpHeapInfo(outstream, indent);
    break;
  case COMSPACE_CLASS:
    // ((ComSpace *)this)->dumpSpaceInfo(outstream, indent);
    break;
  case DEFAULTIPCHEAP_CLASS:
     ((DefaultIpcHeap *)this)->dumpIpcHeapInfo(outstream, indent);
    break;
  default:
    assert(0);
  }
}
#endif

// incrementStats() is called when memory is allocated and a current
// NAHeapFragment becomes in use.
inline void
NAMemory::incrementStats(size_t size)
{
  allocSize_ += size;
  allocCnt_++;
  if (highWaterMark_ < allocSize_)
    highWaterMark_ = allocSize_;   
  if (intervalWaterMark_ < allocSize_)
    intervalWaterMark_ = allocSize_;
}

// decrementStats() is called when memory is deallocated
inline void
NAMemory::decrementStats(size_t size)
{
  allocSize_ -= size;
  allocCnt_--;
}
#endif // !MUSE

// True if address has acceptable alignment
inline NABoolean
NAHeap::isAligned(void *addr)
{
  return ((size_t)addr & (FRAG_ALIGN_MASK)) == 0;
}

// Round a size up to the next byte alignment size.
inline size_t
NAHeap::granularityAlign(size_t size)
{
  return (size + FRAG_BYTE_ALIGN) & ~(FRAG_BYTE_ALIGN - 1);
}

// Check if address is at least the minimum.
inline NABoolean
NAHeap::okAddress(void *addr)
{
  return ((char*)addr >= least_addr_);
}

// Check if address of next fragment n is higher than base fragment p
inline NABoolean
NAHeap::okNext(NAHeapFragment *p, NAHeapFragment *n)
{
  return ((char*)p < (char*)n);
}

// Check if p has its cinuse bit on
inline NABoolean
NAHeap::okCinuse(NAHeapFragment *p)
{
  return p->cinuse();
}

// Check if p has its pinuse bit on
inline NABoolean
NAHeap::okPinuse(NAHeapFragment *p)
{
  return p->pinuse();
}

// bit corresponding to given index
inline NAHeap::binmap_t
NAHeap::idx2bit(bindex_t idx)
{
  return (binmap_t)1 << idx;
}

// Return index corresponding to given bit.  This function assumes
// that the caller has isolated a single bit and that exactly one
// bit is set.
inline NAHeap::bindex_t
NAHeap::bit2idx(binmap_t x)
{
#if defined(i386)
  UInt32 ret;
  __asm__("bsfl %1,%0\n\t" : "=r" (ret) : "rm" (x));
  return (NAHeap::bindex_t)ret;
#else
  // Set all bits right of the set bit.
  x--;

  // Quickly count the number of set bits using a well known
  // population count algorithm.
  x -= ((x >> 1) & 0x55555555);
  x = (((x >> 2) & 0x33333333) + (x & 0x33333333));
  x = (((x >> 4) + x) & 0x0f0f0f0f);
  x += (x >> 8);
  x += (x >> 16);

  return x & 0x3f;
#endif
}

// Return whether this size should be treated as a small fragment
inline NABoolean
NAHeap::isSmall(size_t size)
{
  return (size >> SMALLBIN_SHIFT) < NSMALLBINS;
}

// Return the small bin index for this size
inline NAHeap::bindex_t
NAHeap::smallIndex(size_t size)
{
  return (size >> SMALLBIN_SHIFT);
}

// Return the maximum size for this small bin index
inline size_t
NAHeap::smallIndex2Size(bindex_t idx)
{
  return (idx << SMALLBIN_SHIFT);
}

// Set a bit in the small map to indicate this bin contains free fragments
inline void
NAHeap::markSmallmap(bindex_t idx)
{
   smallmap_ |= idx2bit(idx);
}

// Clear a bit in the small map to indicate this bin does not contain
// free fragments
inline void
NAHeap::clearSmallmap(bindex_t idx)
{
  smallmap_ &= ~idx2bit(idx);
}

// Return whether the bit for this index is set in the small map.
inline NABoolean
NAHeap::smallmapIsMarked(bindex_t idx)
{
  return (smallmap_ & idx2bit(idx)) != 0;
}

// Mark a bit in the tree map to indicate the tree contains free fragments
inline void
NAHeap::markTreemap(bindex_t idx)
{
  treemap_ |= idx2bit(idx);
}

// Clear a bit in the tree map to indicate the tree does not contain
// free fragments
inline void
NAHeap::clearTreemap(bindex_t idx)
{
  treemap_ &= ~idx2bit(idx);
}

// Return whether a bit for this index is set in the tree map
inline NABoolean
NAHeap::treemapIsMarked(bindex_t idx)
{
  return (treemap_ & idx2bit(idx)) != 0;
}

// Compute the tree index for a given size
inline NAHeap::bindex_t
NAHeap::computeTreeIndex(size_t size)
{
  size_t x = size >> TREEBIN_SHIFT;
  if (x == 0)
    return 0;
  else if (x > 0xFFFF)
    return NTREEBINS - 1;
  else {
    UInt32 k;
#if defined(i386)
    __asm__("bsrl %1,%0\n\t" : "=r" (k) : "rm" (x));
#else
    UInt32 y = (UInt32)x;
    UInt32 n = ((y - 0x100) >> 16) & 8;
    k = (((y <<= n) - 0x1000) >> 16) & 4;
    n += k;
    n += k = (((y <<= k) - 0x4000) >> 16) & 2;
    k = 14 - n + ((y <<= k) >> 15);
#endif
    return (k << 1) + ((size >> (k + (TREEBIN_SHIFT-1)) & 1));
  }
}

// Isolate the least set bit of a bitmap
inline NAHeap::binmap_t
NAHeap::leastBit(binmap_t bits)
{
  return bits & -(bits);
}

// Mask with all bits to left of least bit of "bits" on
inline NAHeap::binmap_t
NAHeap::leftBits(binmap_t bits)
{
  return (bits << 1) | -(bits<<1);
}

// Shift placing maximum resolved bit in a treebin at idx as sign bit
// This function is used during traversing the trees.  The right child of
// a node (child[1]) will have the next significant bit set, while the left
// child of a node will have the next signficant bit unset.  This function
// does the proper shifting to allow the first signficant bit to be set for
// the range of sizes used by the tree bin "idx".
inline UInt32
NAHeap::leftshiftForTreeIndex(bindex_t idx)
{
  if (idx == NTREEBINS-1)
    return 0;
  else
    return (SIZE_T_BITSIZE - 1) - ((idx >> 1) + TREEBIN_SHIFT - 2);
}

// The size of the smallest fragment held in bin with index idx
inline size_t
NAHeap::minsizeForTreeIndex(bindex_t idx)
{
  return ((1 << ((idx >> 1) + TREEBIN_SHIFT)) |
   (((size_t)(idx & 1)) << ((idx >> 1) + TREEBIN_SHIFT - 1)));
}

// Return the address of a small bin for a particular index
inline NAHeapFragment*
NAHeap::smallbinAt(bindex_t idx)
{
  return (NAHeapFragment*)&smallbins_[idx<<1];
}

// Return the head of a tree bin for a particular index
inline NATreeFragment**
NAHeap::treebinAt(bindex_t idx)
{
  return &treebins_[idx];
}

#ifndef MUSE
// Initialize bins
void
NAHeap::initBins()
{
  // Establish circular links for smallbins
  bindex_t i;
  for (i = 0; i < NSMALLBINS; ++i) {
    NAHeapFragment *bin = smallbinAt(i);
    bin->setNext(bin);
    bin->setPrev(bin);
  }
  for (i = 0; i < NTREEBINS; ++i)
    *treebinAt(i) = 0;
}

// Insert a small fragment into the small bins
inline void
NAHeap::insertSmallFragment(NAHeapFragment *p, size_t size)
{
  bindex_t idx = smallIndex(size);
  NAHeapFragment *binptr = smallbinAt(idx);
  NAHeapFragment *nextptr = binptr;
  assert(size >= MIN_FRAGMENT_SIZE);

  // If the smallmap is not marked, then mark it.  If it is not marked,
  // then set the next pointer to the first fragment on the list.
  // NOTE: For release mode builds, RTCHECK is defined as "1" so the
  // okAddress() check is not made, and nextptr is always set to the
  // first fragment in the list when the smallmap wasn't marked.
  if (!smallmapIsMarked(idx))
    markSmallmap(idx);
  else if (RTCHECK(okAddress(binptr->getNext())))
    nextptr = binptr->getNext();
  else {
    CORRUPTION_ERROR_ACTION;
  }
  binptr->setNext(p);
  nextptr->setPrev(p);
  p->setNext(nextptr);
  p->setPrev(binptr);
}

// Unlink a small fragment from the small bins
inline void
NAHeap::unlinkSmallFragment(NAHeapFragment *p, size_t size)
{
  NAHeapFragment *nextFrag = p->getNext();
  NAHeapFragment *prevFrag  = p->getPrev();
  bindex_t idx = smallIndex(size);

  // Verify a few things that may catch programming errors or data corruption
  assert(p != prevFrag);
  assert(p != nextFrag);
  assert(p->fragmentSize() == smallIndex2Size(idx));

  // If the next fragment pointer is equal to the previous fragment pointer,
  // then this indicates the fragment being unlinked is the only fragment in
  // the list.  The smallmap can be cleared without dealing with additional
  // pointers because it is initialized properly in insertSmallFragment().
  // If this is not the last fragment in the list, then the previous fragment
  // and next fragments pointers are modified to point to each other.
  // NOTE: RTCHECK is defined as "1" in release mode, so the checks within
  // the macro are not made.
  if (nextFrag == prevFrag) {
    clearSmallmap(idx);
  } else if (RTCHECK((nextFrag == smallbinAt(idx) || okAddress(nextFrag)) &&
                     (prevFrag  == smallbinAt(idx) || okAddress(prevFrag)))) {
    nextFrag->setPrev(prevFrag);
    prevFrag->setNext(nextFrag);
  } else {
    CORRUPTION_ERROR_ACTION;
  }
}

// Unlink the first small fragment from a small bin
inline void
NAHeap::unlinkFirstSmallFragment(NAHeapFragment *binPtr,
                                 NAHeapFragment *fragPtr,
                                 bindex_t  idx)
{
  NAHeapFragment *next = fragPtr->getNext();
  assert(fragPtr != binPtr);
  assert(fragPtr != next);
  assert(fragPtr->fragmentSize() == smallIndex2Size(idx));
  if (binPtr == next) {
    clearSmallmap(idx);
  } else if (RTCHECK(okAddress(next))) {
    binPtr->setNext(next);
    next->setPrev(binPtr);
  } else {
    CORRUPTION_ERROR_ACTION;
  }
}

// Insert fragment into tree
inline void
NAHeap::insertLargeFragment(NATreeFragment *treeFrag, size_t size)
{
  bindex_t idx = computeTreeIndex(size);        // Index for this tree
  NATreeFragment **treebinPtr = treebinAt(idx); // Treebin pointer

  // Initialize part of the tree fragment that is being inserted into
  // the tree.
  treeFrag->setIndex(idx);
  treeFrag->setChild(0, NULL);
  treeFrag->setChild(1, NULL);
  treeFrag->setFreedNSKMemory(0); // memory not yet released to NSK

  // If this is the first fragment inserted into the tree, then mark
  // the treemap, set the treebin pointer to point to this fragment, and
  // finish initializing the tree fragment.
  if (!treemapIsMarked(idx)) {
    markTreemap(idx);
    *treebinPtr = treeFrag;
    treeFrag->setParent((NATreeFragment*)treebinPtr);
    treeFrag->setNext(treeFrag);
    treeFrag->setPrev(treeFrag);
  }
  else {
    // If the code reaches here, then this is not the first fragment
    // inserted into this tree bin.  The code below looks at the bits
    // of the size for a tree fragment range.  For instance, there
    // could be a tree bin that deals with sizes of 769 bytes to 1024
    // bytes.  In this case, the most signficant bit would be the bit
    // set by 1024, or (1 << 10).  The leftshiftForTreeindex determines
    // the number of bits needed to shift the size so the most signficant
    // bit of the range is in the 32nd bit. Then the code below looks
    // at the most signficant bit in "sizebits" during each time through
    // the loop.  Each time through the "sizebits" is shifted left by one
    // so that the next significant bit can be examined.  This technique
    // allows for a theoretically balanced tree, and allows for quick
    // traversing of tree tree.
    NATreeFragment *treePtr = *treebinPtr;  // Pointer for tree traversal
    size_t sizebits = size << leftshiftForTreeIndex(idx);
    for (;;) {
      if (treePtr->fragmentSize() != size) {

        NATreeFragment **childPtr = treePtr->getChildAddr(
                          (sizebits >> (SIZE_T_BITSIZE - 1)) & 1);
        sizebits <<= 1;
        if (*childPtr != 0)
          treePtr = *childPtr;
        else if (RTCHECK(okAddress(childPtr))) {
          *childPtr = treeFrag;
          treeFrag->setParent(treePtr);
          treeFrag->setNext(treeFrag);
          treeFrag->setPrev(treeFrag);
          break;  // Break from for loop
        } else {
          CORRUPTION_ERROR_ACTION;
          break;  // Break from for loop
        }
      } else {
        NATreeFragment *next = (NATreeFragment*)treePtr->getNext();
        if (RTCHECK(okAddress(treePtr) && okAddress(next))) {
          treePtr->setNext(treeFrag);
          next->setPrev(treeFrag);
          treeFrag->setNext(next);
          treeFrag->setPrev(treePtr);
          treeFrag->setParent(NULL);
          break;  // Break from for loop
        } else {
          CORRUPTION_ERROR_ACTION;
          break;  // Break from for loop
        }
      }
    }
  }
}

// Unlink a large fragment from the tree
//  Steps:
//  1. If "treeFrag" is a chained node, unlink it from its same-sized
//     next/prev links and choose its prev node as its replacement.
//  2. If treeFrag was the last node of its size, but not a leaf node,
//     it must be replaced with a leaf node (not merely one with an
//     open left or right), to make sure that lefts and rights of
//     descendents correspond properly to bit masks.  We use the
//     rightmost descendent of treeFrag.  We could use any other leaf,
//     but this is easy to locate and tends to counteract removal of
//     leftmosts elsewhere, and so keeps paths shorter than minimally
//     guaranteed.  This doesn't loop much because on average a node
//     in a tree is near the bottom.
//  3. If x is the base of a chain (i.e., has parent links) relink
//     x's parent and children to x's replacement (or null if none).
inline void
NAHeap::unlinkLargeFragment(NATreeFragment *treeFrag)
{
  NATreeFragment *parent = treeFrag->getParent();
  NATreeFragment *prev;
  if (treeFrag->getPrev() != treeFrag) {
    NATreeFragment *next = (NATreeFragment*)treeFrag->getNext();
    prev = (NATreeFragment*)treeFrag->getPrev();
    if (RTCHECK(okAddress(next))) {
      next->setPrev(prev);
      prev->setNext(next);
    } else {
      CORRUPTION_ERROR_ACTION;
    }
  } else {
    // If the current node has any children, then the next section of
    // code searches for the rightmost descendent and sets "prev" to
    // the parent of this descendent, and "rightmostPtr" to the
    // descendent.  
    NATreeFragment **rightmostPtr;
    if (((prev = *(rightmostPtr = treeFrag->getChildAddr(1))) != 0) ||
        ((prev = *(rightmostPtr = treeFrag->getChildAddr(0))) != 0)) {
      NATreeFragment **childPtr;
      while ((*(childPtr = prev->getChildAddr(1)) != 0) ||
             (*(childPtr = prev->getChildAddr(0)) != 0)) {
        prev = *(rightmostPtr = childPtr);
      }
      if (RTCHECK(okAddress(rightmostPtr))) {
        *rightmostPtr = 0;
      } else {
        CORRUPTION_ERROR_ACTION;
      }
    }
  }

  if (parent != 0) {
    NATreeFragment **treebinPtr = treebinAt(treeFrag->getIndex());
    if (treeFrag == *treebinPtr) {
      if ((*treebinPtr = prev) == 0) 
        clearTreemap(treeFrag->getIndex());
    } else if (RTCHECK(okAddress(parent))) {
      if (parent->getChild(0) == treeFrag) 
        parent->setChild(0, prev);
      else 
        parent->setChild(1, prev);
    } else {
      CORRUPTION_ERROR_ACTION;
    }

    if (prev != 0) {
      if (RTCHECK(okAddress(prev))) {
        NATreeFragment *leftChild, *rightChild;
        prev->setParent(parent);
        if ((leftChild = treeFrag->getChild(0)) != 0) {
          if (RTCHECK(okAddress(leftChild))) {
            prev->setChild(0, leftChild);
            leftChild->setParent(prev);
          } else {
            CORRUPTION_ERROR_ACTION;
          }
        }
        if ((rightChild = treeFrag->getChild(1)) != 0) {
          if (RTCHECK(okAddress(rightChild))) {
            prev->setChild(1, rightChild);
            rightChild->setParent(prev);
          } else {
            CORRUPTION_ERROR_ACTION;
          }
        }
      } else {
        CORRUPTION_ERROR_ACTION;
      }
    }
  }
}

// Insert this fragment into either a large or small bin
inline void
NAHeap::insertFragment(NAHeapFragment *p, size_t size)
{
  if (isSmall(size)) {
    insertSmallFragment(p, size);
  } else {
    NATreeFragment *tp = (NATreeFragment*)p;
    insertLargeFragment(tp, size);
  }
}

// Unlink either a large or small fragment from the small bins or
// large trees
inline void
NAHeap::unlinkFragment(NAHeapFragment *p, size_t size)
{
  if (isSmall(size)) {
    unlinkSmallFragment(p, size);
  } else {
    NATreeFragment *tp = (NATreeFragment*)p;
    unlinkLargeFragment(tp);
  }
}

// Replace the designated victim with a different fragment
inline void
NAHeap::replaceDV(NAHeapFragment *p, size_t size)
{
  if (dvsize_ != 0)
    insertSmallFragment(dv_, dvsize_);

  dvsize_ = size;
  dv_ = p;
}

// Resize the top fragment when a segment changes size.
inline void
NAHeap::resizeTop(size_t newsize)
{
  topsize_ = newsize;

  size_t firstFragBit = top_->getFirstFragmentBit();
  top_->setSizeAndPinuse(newsize | firstFragBit);

  NAHeapFragment *next = top_->nextFragment();
  next->setFencePosts();
}

// Initialize the top fragment
inline void
NAHeap::initTop(NABlock *block)
{
  size_t offset = block->firstFragmentOffset();

  // Calculate topsize_ with rounding down to the FRAG_ALIGN boundary.
  topsize_ = (block->size_ - offset - MIN_FRAGMENT_SIZE) & ~FRAG_ALIGN_MASK;
  top_ = (NAHeapFragment*)((char*)block + offset);

  top_->setSizeAndPinuse(topsize_);

  // Set trailing fenceposts.  The assert insures that there is enough
  // space for the trailing fenceposts at the end of the block.
  NAHeapFragment *r = top_->nextFragment();
  assert((char*)r + (3 * sizeof(size_t)) <= (char*)block + block->size_);
  r->setFencePosts();
}

// initialize the fields in the NAHeapFragment structures when a new
// noncontiguous NABlock is allocated.
void
NAHeap::addBlock(NABlock *newBlock)
{
  // Insert the old top into a bin as an ordinary free fragment.
  if (topsize_ != 0) {
    size_t firstFragBit = top_->getFirstFragmentBit();
    NAHeapFragment *next = top_->fragmentPlusOffset(topsize_);

    top_->setFreeWithPinuse(topsize_, next);
    insertFragment(top_, topsize_);
    top_->setHeadBits(firstFragBit); // Set first fragment bit if it was set
  }

  // reset top to new space
  initTop(newBlock);
  top_->initializeFirstFragment(topsize_);
  checkTopFragment(top_);
}

// Free an NABlock if all of the memory becomes free.  This releases
// it to the operating system or frees it in the parent memory.
NABoolean
NAHeap::deallocateFreeBlock(NAHeapFragment *p)
{
  NABlock *prev = NULL;
  NABlock *curr = firstBlk_;

  // Find the NABlock containing this fragment
  while (curr != NULL && !curr->blockHolds(p)) {
    prev = curr;
    curr = curr->next_;
  }

  assert(curr != NULL); // A block had better contain this fragment

  // If this block was allocated externally, then it should not be
  // deallocated here.  This shouldn't really happen.  For a future
  // project it might make sense to resize this segment to something
  // smaller to save on memory.
  if (curr->isExternSegment())
    return FALSE;

  // Decrement statistics
  totalSize_ -= curr->size_;
  blockCnt_--;

  // Reset top_ if it is in this NABlock
  if (curr->blockHolds(top_)) {
    top_ = NULL;
    topsize_ = 0;
  }

  // Reset designated victim if it is within this NABlock
  if (curr->blockHolds(dv_)) {
    dv_ = NULL;
    dvsize_ = 0;
  }

  // Remove this NABlock from the NABlock list
  if (prev == NULL)
    firstBlk_ = curr->next_;
  else
    prev->next_ = curr->next_;

  // Now release the memory to the operating system.
  switch (type_)
  {
  case EXECUTOR_MEMORY:
  case SYSTEM_MEMORY:
  case IPC_MEMORY:
    sysFreeBlock(curr);
    break;
  case DERIVED_FROM_SYS_HEAP:
    sysFreeBlock(curr);
    break;
  case DERIVED_MEMORY:
    assert(parent_);
    HEAPLOG_OFF() // no recursive logging.
    // This code provides mutual exclusion for the runtime stats shared
    // memory segment.
    if (parent_->getType() == EXECUTOR_MEMORY && getSharedMemory()) {
       short retcode = getRTSSemaphore();
       parent_->deallocateHeapMemory((void*)curr);
       if (retcode == 1)
         releaseRTSSemaphore(); 
    } else {
      parent_->deallocateHeapMemory((void*)curr);
    }
    HEAPLOG_ON()
    break;
  default:
    assert(0);  // Shouldn't happen
    break;
  }

  return TRUE;
}

// Allocate a large request from the best fitting fragment in a treebin
// and return a pointer to the NAHeapFragment that best fits the request.
// If the allocation cannot be satisfied or if the designated victim is
// a better fit than the best fitting fragment in any of the trees, then
// a NULL pointer is returned.
NAHeapFragment*
NAHeap::tmallocLarge(size_t nb)
{
  NATreeFragment *v = NULL;
  size_t rsize = -nb; // Unsigned negation.
  NATreeFragment *t;
  bindex_t idx = computeTreeIndex(nb);

  if ((t = *treebinAt(idx)) != 0) {
    // Traverse tree for this bin looking for node with size == nb
    size_t sizebits = nb << leftshiftForTreeIndex(idx);
    NATreeFragment *rst = NULL;  // The deepest untaken right subtree
    for (;;) {
      size_t trem = t->fragmentSize() - nb;  // Can cause underflow
      if (trem < rsize) {
        v = t;
        if ((rsize = trem) == 0)
          break;
      }
      NATreeFragment *rt = t->getChild(1);
      t = t->getChild((sizebits >> (SIZE_T_BITSIZE - 1)) & 1);
      if (rt != 0 && rt != t)
        rst = rt;
      if (t == 0) {
        t = rst; // set t to least subtree holding sizes > nb
        break;
      }
      sizebits <<= 1;
    }
  }

  if (t == 0 && v == 0) { // set t to root of next non-empty treebin
    binmap_t leftbits = leftBits(idx2bit(idx)) & treemap_;
    if (leftbits != 0) {
      binmap_t leastbit = leastBit(leftbits);
      bindex_t i = bit2idx(leastbit);
      t = *treebinAt(i);
    }
  }

  while (t != 0) { // find smallest of tree or subtree
    size_t trem = t->fragmentSize() - nb;
    if (trem < rsize) {
      rsize = trem;
      v = t;
    }
    t = t->leftmostChild();
  }

  // If dv is a not a better fit, then use the tree fragment that was
  // found.  If necessary, split the tree fragment into two fragments,
  // and reinsert the remainder into a small bin or another tree.
  if (v != 0 && rsize < (dvsize_ - nb)) {
    if (RTCHECK(okAddress(v))) { // split
      NAHeapFragment *r = v->fragmentPlusOffset(nb);
      assert(v->fragmentSize() == rsize + nb);
      size_t firstFragBit = v->getFirstFragmentBit();
      if (RTCHECK(okNext(v, r))) {
        unlinkLargeFragment(v);
        if (rsize < MIN_FRAGMENT_SIZE) {
          v->setInuseAndPinuse((rsize + nb) | firstFragBit);
        } else {
          v->setSizeAndPinuseOfInuseFragment(nb | firstFragBit);
          r->setSizeAndPinuseOfFreeFragment(rsize);
          insertFragment(r, rsize);
        }
        return v;
      }
    }
    CORRUPTION_ERROR_ACTION;
  }
  // Return NULL because the designated victim is larger than the treebin
  // or there wasn't a tree fragment that satisfied the request.
  return NULL;
}

// allocate a small request from the best fitting fragment in a treebin
NAHeapFragment*
NAHeap::tmallocSmall(size_t nb)
{
  binmap_t leastbit = leastBit(treemap_);
  bindex_t idx = bit2idx(leastbit);

  NATreeFragment *t, *v;
  v = t = *treebinAt(idx);
  size_t rsize = t->fragmentSize() - nb;

  while ((t = t->leftmostChild()) != 0) {
    size_t trem = t->fragmentSize() - nb;
    if (trem < rsize) {
      rsize = trem;
      v = t;
    }
  }

  if (RTCHECK(okAddress(v))) {
    NAHeapFragment *r = v->fragmentPlusOffset(nb);
    assert(v->fragmentSize() == rsize + nb);
    size_t firstFragBit = v->getFirstFragmentBit();

    if (RTCHECK(okNext(v, r))) {
      unlinkLargeFragment(v);
      if (rsize < MIN_FRAGMENT_SIZE) {
        v->setInuseAndPinuse((rsize + nb) | firstFragBit);
      } else {
        v->setSizeAndPinuseOfInuseFragment(nb | firstFragBit);
        r->setSizeAndPinuseOfFreeFragment(rsize);
        replaceDV(r, rsize);
      }
      return v;
    }
  }

  CORRUPTION_ERROR_ACTION;
  return NULL;
}

Lng32 NAMemory::getAllocatedSpaceSize()
{
  assert(type_ != NO_MEMORY_TYPE);
  return allocSize_;
}

void NAMemory::setName(const char * name)
{
  Lng32 copyLen = str_len(name);
  if (copyLen > 20)
    copyLen = 20;
  memcpy(name_, name, copyLen);
  name_[copyLen] = 0;
}

void NAMemory::registerMemory(NAMemory * child)
{
  // put child at the end of the memoryList_
  if (lastListEntry_)
    lastListEntry_->nextEntry_ = child;
  else
    memoryList_ = child;

  lastListEntry_ = child;
}

void NAMemory::unRegisterMemory(NAMemory * child)
{
  NAMemory * p = memoryList_;
  NAMemory * q = NULL;  
  while (p && p != child) {
    q = p;
    p = p->nextEntry_;
  }
  assert(p);

  if (q == NULL)
    // we remove the first entry
    memoryList_ = p->nextEntry_;
  else
    q->nextEntry_ = p->nextEntry_;

  // adjust lastListEntry_
  if (child == lastListEntry_)
    lastListEntry_ = q;
}

// Return the resize segment size including the first seg offset
// (totalSize_, maximumSize_, etc does not include the first seg offset)


NABlock*
NAHeap::allocateBlock(size_t size, NABoolean failureIsFatal)
{
  assert(type_ != NO_MEMORY_TYPE);

  // if a limit is specified for this heap, make sure we don't pass it
  if ((IdentifyMyself::GetMyName() == I_AM_EMBEDDED_SQL_COMPILER) &&
      (upperLimit_ != 0) && 
      ((allocSize_ + size) > upperLimit_))
  {
     // reset the upper limit for now so testing can continue
     upperLimit_ = 0;
     handleExhaustedMemory();
  }


  void * addr = 0;
  short segmentId = (firstBlk_ == NULL ? 0 : firstBlk_->segmentId_);

  // for derived memories, double the incrementSize_ if we have
  // allocated 10 (20, 30, ...) blocks
  if ((type_ == DERIVED_MEMORY ||
      type_ == DERIVED_FROM_SYS_HEAP) &&
      thBlockCnt_ == (blockCnt_ + 1) &&
      incrementSize_ < DEFAULT_MAX_INCREMENT) {

    // next adjustment when we get 10 more blocks.
    incrementSize_ *= 2;
    if (incrementSize_ > DEFAULT_MAX_INCREMENT)
      incrementSize_ = DEFAULT_MAX_INCREMENT;
    thBlockCnt_ += DEFAULT_THRESHOLD_BLOCK_COUNT;
  }

  // allocate always at least blockSize bytes
  size_t blockSize = (firstBlk_ == NULL) ? initialSize_ : incrementSize_;



  // Ensure that the blockSize is at least large enough to hold the
  // requested size.
  if (blockSize < size) // Compare as unsigned integer
    blockSize = size;

  // make sure that the block size is a multiple of FRAG_BYTE_ALIGN
  blockSize = (blockSize + FRAG_ALIGN_MASK) & ~FRAG_ALIGN_MASK;

  NABlock *p = NULL;              // Pointer to the returned block.

  switch (type_) {
  case EXECUTOR_MEMORY: {

    // This could be either Global Executor Memory or Stats Globals
    // Don't add a block if Stats Globals!
    if (getSharedMemory())
      return NULL;

    // Try to allocate the NABlock using mmap(). If it succeeds return the
    // NABlock.  Otherwise, fall through and try to allocate the normal way.
    if (blockSize >= MIN_MMAP_ALLOC_SIZE &&
        (p = allocateMMapBlock(blockSize)) != NULL)
      return p;

    // allocate a block from the OS memory
    p = (NABlock*)malloc(blockSize);
    if (p)
      allocationIncrement(blockSize);
  }
  break;

  case SYSTEM_MEMORY: 
  case IPC_MEMORY:
  {
    // Try to allocate the NABlock using mmap(). If it succeeds return the
    // NABlock.  Otherwise, fall through and try to allocate the normal way.
    if (blockSize >= MIN_MMAP_ALLOC_SIZE &&
        (p = allocateMMapBlock(blockSize)) != NULL)
      return p;

    // allocate a block from the OS memory
    p = (NABlock*)malloc(blockSize);
    if (p)
      allocationIncrement(blockSize);
  }
  break;

  case DERIVED_FROM_SYS_HEAP: {
    // allocate a block from the OS memory
    // Try to allocate the NABlock using mmap(). If it succeeds return the
    // NABlock.  Otherwise, fall through and try to allocate the normal way.
    if (blockSize >= MIN_MMAP_ALLOC_SIZE &&
        (p = allocateMMapBlock(blockSize)) != NULL)
      return p;

    p = (NABlock*)malloc(blockSize);
    if (p)
      allocationIncrement(blockSize);
  }
  break;

  case DERIVED_MEMORY: {
    // make sure that we have a parent
    assert(parent_);
    // allocate a block from the parent
    HEAPLOG_OFF()  // no recursive logging. (eric)

    // This code provides mutual exclusion for the runtime stats shared
    // memory segment.
    // The IF condition below is not needed since we are checking if
    // semaphore is obtained in allocateHeapMemory or deallocateHeapMemory
    // for both global and process stats heap. But leaving it now
    // since it won't hurt other than extra cpu cycles
    if (getSharedMemory()) {
      short retcode = getRTSSemaphore();
      p = (NABlock*)parent_->allocateHeapMemory(blockSize, FALSE);

      if (p == NULL)
// Unit tested this code with the test case in QC 1387
// - 3/22/2012.
      {
        // Exhausted shared segment. Prevent cascade of core-files by
        // making one core-file of myself while holding semaphore.
        // Then bring down this node.
        genLinuxCorefile("Shared Segment might be full");
        Int32 nid = 0;
        Int32 pid = 0;
        if (XZFIL_ERR_OK == msg_mon_get_my_info(&nid, &pid, NULL,
                        0, NULL, NULL, NULL, NULL))
        {
          // The SSMP is responsible for preventing leaks. So get a 
          // corefile of it.
          char ssmpName[MS_MON_MAX_PROCESS_NAME];
          memset(ssmpName, 0, MS_MON_MAX_PROCESS_NAME);
          if (XZFIL_ERR_OK == XPROCESSHANDLE_DECOMPOSE_(
			      getMySsmpPhandle()
			    , NULL // cpu
			    , NULL // pin
			    , NULL // nodenumber
			    , NULL // nodename
			    , 0    // nodename_maxlen
			    , NULL // nodename_length
			    , ssmpName 
			    , sizeof(ssmpName)))
          {
            char coreFile[1024];
            msg_mon_dump_process_name(NULL, ssmpName, coreFile);
          }
          Int32 ndRetcode = msg_mon_node_down2(nid,
                              "RMS shared segment is exhausted.");
          sleep(30);
          NAExit(0);    // already made a core.
        }
        else
          assert(p != NULL);
      }

      if (retcode == 1)
        releaseRTSSemaphore();
    } else {
      p = (NABlock*)parent_->allocateHeapMemory(blockSize, failureIsFatal);
     if (parent_->type_ == NAMemory::EXECUTOR_MEMORY &&
            parent_->parent_ == NULL)
     {
        updateMemStats();
     }
    }
    segmentId = NABlock::DERIVED_SEGMENT_ID;  // Add another check for muse
    HEAPLOG_ON()
  }
  break;
  }  // switch(type_)

  // if the allocation was not sucessfull, we return a NULL
  if (p == NULL)
    return NULL;

  // one more block allocated
  blockCnt_++;
  totalSize_+= blockSize;

  // Initialize the returned NABlock information
  p->segmentId_ = segmentId;
  p->size_ = blockSize;
  p->next_ = NULL; // The caller is responsible for linking this
  p->sflags_ = 0;

  return p;
} // NAHeap::allocateBlock(size_t size, NABoolean failureIsFatal)
#endif // !MUSE

void NAMemory::showStats(ULng32 level)
{
  char indent[100];
  Int32 i = 0;
  for (; i < (2 + 2 * (Int32) level); i++)
    indent[i] = ' ';
  indent[i] = '\0';

  cerr << indent << "NAMemory: " << this << " is a ";
  switch(type_) {
  case NO_MEMORY_TYPE:
    cerr << "NO_MEMORY_TYPE";
    break;
  case EXECUTOR_MEMORY:
    cerr << "EXECUTOR_MEMORY";
    break;
  case SYSTEM_MEMORY:
    cerr << "SYSTEM_MEMORY";
    break;
  case IPC_MEMORY:
    cerr << "IPC_MEMORY";
    break;
  case DERIVED_MEMORY:
    cerr << "DERIVED_MEMORY";
    break;
  case DERIVED_FROM_SYS_HEAP:
    cerr << "DERIVED_FROM_SYS_HEAP";
    break;
  }
  cerr << endl
    << indent << "name of memory: " << name_ << endl
       << indent << "parent memory: " << parent_ << endl
       << indent << "initial size: " << initialSize_ << endl
       << indent << "maximum size: " << maximumSize_ << endl
       << indent << "increment size: " << incrementSize_ << endl
       << indent << "total size: " << totalSize_ << endl
       << indent << "#of blocks: " << blockCnt_ << endl
       << indent << "allocated size: " << allocSize_ << endl
       << indent << "high water mark: " << highWaterMark_ << endl
       << indent << "interval water mark: " << intervalWaterMark_ << endl
       << indent << "allocated fragments: " << allocCnt_ << endl
       << indent << "-------------------------------------------------" << endl;

  for (NAMemory * p = memoryList_; p != NULL; p = p->nextEntry_)
    p->showStats(level + 1);
}

// this method used to belong to CollHeap
#ifndef MUSE
void 
NAMemory::handleExhaustedMemory()
{
  exhaustedMem_ = TRUE;
}
#endif // MUSE


NABoolean NAMemory::getUsage(size_t * lastBlockSize, size_t * freeSize, size_t * totalSize)
{
  *freeSize = 0;
  NAMemory * memory = this;
  NABoolean crowded = FALSE;
  
  do
  {
    *freeSize += memory->totalSize_ - memory->allocSize_;
    *totalSize = memory->totalSize_;
   if (memory->type_ == EXECUTOR_MEMORY)
    {
      //if (memory->firstBlk_->next_ == NULL)
      *lastBlockSize = 0; 
        //*lastBlockSize = memory->firstBlk_->size_;
      //else
        //*lastBlockSize = memory->firstBlk_->next_->size_;
      if (memory->crowdedTotalSize_ > 0ll &&
	*freeSize < *totalSize / 2) // Free size is < half total alloc'd
        {
          // Simulate size on NSK when unable to allocate 128 MB flat segment
          *lastBlockSize = (Lng32)((4ll * 1024ll * 1024ll * 1024ll - memory->crowdedTotalSize_) / 2);
	  crowded = TRUE;
        }
    }
    memory = memory->parent_;
  } while (memory != NULL);
  return crowded;
}

NABoolean NAMemory::checkSize(size_t size, NABoolean failureIsFatal)
{
  if (size > MAX_MEMORY_SIZE_IN_AN_ALLOC) {
     if (failureIsFatal) 
        abort();
     else
        return FALSE;  
  }
  return TRUE;
}

// ---------------------------------------------------------------------------
// NAHeap methods
// ---------------------------------------------------------------------------
#ifndef MUSE
NAHeap::NAHeap()
  : NAMemory("Unknown Memory Type"),
    smallmap_(0),
    treemap_(0),
    dvsize_(0),
    topsize_(0),
    least_addr_(0),
    dv_(NULL),
    top_(NULL),
    errCallback_(NULL)
{
  initBins();
  derivedClass_ = NAHEAP_CLASS;

  threadSafe_ = false;
  memset(&mutex_, '\0', sizeof(mutex_));

#ifdef _DEBUG
  setAllocTrace();
#endif // _DEBUG
}

NAHeap::NAHeap(const char * name, 
	       NAHeap * parent, 
	       Lng32 blockSize, 
	       size_t upperLimit)
  : NAMemory(name, parent, blockSize, upperLimit),
    smallmap_(0),
    treemap_(0),
    dvsize_(0),
    topsize_(0),
    least_addr_(0),
    dv_(NULL),
    top_(NULL),
    errCallback_(NULL)
{
  initBins();
  derivedClass_ = NAHEAP_CLASS;

  threadSafe_ = false;
  memset(&mutex_, '\0', sizeof(mutex_));
  if (parent != NULL)
  {
     NAMutex mutex(parent->threadSafe_, &parent->mutex_);
     parent_->registerMemory(this);
  }

#ifdef _DEBUG
  setAllocTrace();
#endif // _DEBUG
}

NAHeap::NAHeap(const char * name, 
	       NAMemoryType type, 
	       Lng32 blockSize, 
	       size_t upperLimit) 
  : NAMemory(name, type, blockSize, upperLimit),
    smallmap_(0),
    treemap_(0),
    dvsize_(0),
    topsize_(0),
    least_addr_(0),
    dv_(NULL),
    top_(NULL),
    errCallback_(NULL)
{
  initBins();
  derivedClass_ = NAHEAP_CLASS;

  threadSafe_ = false;
  memset(&mutex_, '\0', sizeof(mutex_));

#ifdef _DEBUG
  setAllocTrace();
#endif // _DEBUG
}

NAHeap::NAHeap(const char  * name)
  : NAMemory(name),
    smallmap_(0),
    treemap_(0),
    dvsize_(0),
    topsize_(0),
    least_addr_(0),
    dv_(NULL),
    top_(NULL),
    errCallback_(NULL)
{
  initBins();
  derivedClass_ = NAHEAP_CLASS;

  if (firstBlk_) {
    initTop(firstBlk_);
    least_addr_ = (char*)firstBlk_;
  }

  if (deallocTraceArray == 0)
  {
    static char *deallocTraceEnvvar = getenv("EXE_DEALLOC_MEM_TRACE");
    if (deallocTraceEnvvar != NULL)
    {
           deallocTraceArray =
        (DeallocTraceEntry (*) [deallocTraceEntries])malloc(sizeof(DeallocTraceEntry) * deallocTraceEntries);
      memset((void *)deallocTraceArray, '\0', sizeof(DeallocTraceEntry) * deallocTraceEntries);
    }
  }

  threadSafe_ = false;
  memset(&mutex_, '\0', sizeof(mutex_));

#ifdef _DEBUG
  setAllocTrace();
#endif // _DEBUG
}

// Constructor that imposes the NAHeap struture on already allocated memory 
NAHeap::NAHeap(const char * name,
       SEG_ID  segmentId,
       void  * baseAddr,
       off_t   heapStartOffset,
       size_t  maxSize)
  : NAMemory(name, segmentId, baseAddr, heapStartOffset, maxSize),
    smallmap_(0),
    treemap_(0),
    dvsize_(0),
    topsize_(0),
    least_addr_(0),
    dv_(NULL),
    top_(NULL),
    errCallback_(NULL)
{
  initBins();
  derivedClass_ = NAHEAP_CLASS;

  if (firstBlk_) {
    initTop(firstBlk_);
    least_addr_ = (char*)firstBlk_;
  }

  if (deallocTraceArray == 0)
  {
    char *deallocTraceEnvvar = getenv("EXE_DEALLOC_MEM_TRACE");
    if (deallocTraceEnvvar != NULL)
    {
      deallocTraceArray =
        (DeallocTraceEntry (*) [deallocTraceEntries])malloc(sizeof(DeallocTraceEntry) * deallocTraceEntries);
      memset((void *)deallocTraceArray, '\0', sizeof(DeallocTraceEntry) * deallocTraceEntries);
    }
  }

  threadSafe_ = false;
  memset(&mutex_, '\0', sizeof(mutex_));

#ifdef _DEBUG
  setAllocTrace();
#endif // _DEBUG
}

NAHeap::~NAHeap()
{
   destroy();
  if (threadSafe_)
     pthread_mutex_destroy(&mutex_);
}

void NAHeap::setThreadSafe()
{
  int rc;
  pthread_mutexattr_t attr;
  rc = pthread_mutexattr_init(&attr);
  assert(rc == 0);
  rc = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
  assert(rc == 0);
  NAHeap *heap = this;
  // set this heap and heaps it is derived from
  while (heap)
  {
    if (heap->threadSafe_ == false)
    {
      rc = pthread_mutex_init(&(heap->mutex_), &attr);
      assert(rc == 0);
      heap->threadSafe_ = true;
    }
    heap = heap->parent_;
  }
}

// destroy() is called by the NAMemory destructor to perform the
// derived class destructor.  It is part of the mechanism that
// prevents virtual functions, which cause problems when used with
// runtime statistics.
void NAHeap::destroy() 
{
  if (parent_ != NULL)
     NAMutex mutex(parent_->threadSafe_, &parent_->mutex_);

#ifdef _DEBUG
  if (la_)
    {
      char header[120];
      sprintf(header, "Stacks when block of %d bytes was asked from heap (%s)",
              TraceAllocSize, getName());
      dumpTrafStack(getSAL(), header, true);
      delete la_;
    }
#endif // _DEBUG
  reInitialize();

  // remove this memory from the parents memoryList_
  if (parent_)
    parent_->unRegisterMemory(this);
}

// reInitializeHeap reinitializes the NAHeap structures.  It is called from the
// NAMemory reInitialize(), which may either be called directly or from a destructor.
void NAHeap::reInitializeHeap()
{
  NAMutex mutex(threadSafe_, &mutex_);

#if (defined(_DEBUG) || defined(NSK_MEMDEBUG))
  // Before freeing the NABlocks in reInitialize(), check to see whether any
  // of the memory fragments in the blocks have overflowed
  if (debugLevel_ == 2)
    checkForOverflow();
#endif

  // Reinitialize the fields that are specific to the NAHeap class
  smallmap_ = treemap_ = dvsize_ = topsize_ = 0;
  dv_ = NULL;
  top_ = NULL;
  least_addr_ = NULL;

  initBins();

  // If a segment is still active, then reinitialize the top fragment
  // so that it can be used. This code is not currently called, but
  // is here to allow the EXECUTOR_MEMORY or any other memory that
  // will be allocated by an external means to be reinitialized.
  // The first part of reinitialization occurs in NAMemory::reInitialize().
  // That code frees the NABlocks and will reinitialize the firstBlk_
  // if it was allocated externally.
  if (firstBlk_ != NULL) {
     assert((char*)firstBlk_ == ((char*)heapStartAddr_ - heapStartOffset_));
     least_addr_ = (char*)firstBlk_;
     initTop(firstBlk_);
  }
}

// NAHeap::setErrorCallback() sets a pointer to a function that
// will be called if the heap cannot allocate an NABlock.  The
// callback function will be called with a NAHeap pointer argument
// and an argument that indicates the amount of memory that was
// being allocated.
void NAHeap::setErrorCallback(void (*errCallback)(NAHeap*,size_t))
{
  errCallback_ = errCallback;
}

void * NAHeap::allocateAlignedHeapMemory(size_t userSize, size_t alignment, NABoolean failureIsFatal)
{
  if (alignment & (sizeof(void *) - 1)) // Is alignment a multiple of sizeof(void *)?
    return NULL;
  if (alignment & (alignment - 1)) // Is alignment a power of 2?
    return NULL;
  size_t retAddr, alignedAddr;
  retAddr = (size_t)allocateHeapMemory(userSize + alignment, failureIsFatal);
  if (retAddr == (size_t) NULL)
    return NULL;
  alignedAddr = retAddr & ~(alignment - 1);
  if (alignedAddr != retAddr)
  {
    alignedAddr += alignment;
    if ((alignedAddr - retAddr) < (sizeof(size_t) * 2))
    { // need to double the alignment to avoid override header info
      deallocateHeapMemory((void *)retAddr);
      retAddr = (size_t)allocateHeapMemory(userSize + 2 * alignment, failureIsFatal);
      if (retAddr == (size_t) NULL)
        return NULL;
      alignedAddr = retAddr & ~(alignment - 1);
      if (alignedAddr == retAddr)
        return (void *)alignedAddr;
      alignedAddr += alignment;  // move to aligned addr inside the fragment
      if ((alignedAddr - retAddr) < (sizeof(size_t) * 2))
        alignedAddr += alignment;  // move to next alignment
    }
    NAHeapFragment *p = NAHeapFragment::memToFragment((void *)alignedAddr);
    p->initializeFirstFragment(alignedAddr - retAddr);
    p->setSizeAndPinuseOfInuseFragment(0); // Set inuse bits on and size = 0
   }
  return (void *)alignedAddr;
}

// NAHeap::allocateHeapMemory()
//   This algorithm is taken from Doug Lea's public domain malloc.
//     Basic algorithm:
//     If a small request (< 256 bytes minus per-fragment overhead):
//       1. If one exists, use a remainderless fragment in associated smallbin.
//          (Remainderless means that there are too few excess bytes to
//          represent as a fragment.)
//       2. If it is big enough, use the designated victim fragment.  The
//          designated fragment is used for quickly servicing small requests.
//       3. If one exists, split the smallest available fragment in a bin,
//          saving remainder in dv.
//       4. If the top fragment is not large enough to handle the request, then
//          call NAHeap::allocateBlock() to allocate another segment from the
//          OS or to increase the size of the last segment.  Set the size of or
//          resize the top fragment.
//       5. Allocate memory from the top fragment.
//     Otherwise, for a large request:
//       1. Find the smallest available binned fragment that fits, and use it
//          if it is better fitting than dv fragment, splitting if necessary.
//       2. If better fitting than any binned fragment, use the dv fragment.
//       3. If the top fragment is not large enough to handle the request, then
//          call NAHeap::allocateBlock() to allocate another segment from the
//          OS or to increase the size of the last segment.  Set the size of or
//          resize the top fragment.
//       4. Allocate memory from the top fragment.
//
void * NAHeap::allocateHeapMemory(size_t userSize, NABoolean failureIsFatal)
{

  NAMutex mutex(threadSafe_, &mutex_);

  assert(type_ != NO_MEMORY_TYPE);

  // allocation of size 0. We return a NULL. Alternative would be to
  // allocate 0 bytes. But this would waste memory to maintain a
  // heap fragment of size 0.
  if (userSize == 0)
     return NULL;

  if (! checkSize(userSize, failureIsFatal))
     return NULL;

  // getSharedMemory() check alone is enough since it will return for both
  // global and process stats heap. Leaving the rest of the condition here
  //
  if (getSharedMemory())
  {
    // Check if you are within semaphore
    if (! checkIfRTSSemaphoreLocked())
       NAExit(1);
  }

  // Some additional bytes are added in debug mode, but the original userSize
  // must be retained.
  size_t additionalUserSize = userSize;

#if (defined(_DEBUG) || defined(NSK_MEMDEBUG))  
  if (debugLevel_)
    dynamicAllocs.addEntry(userSize);

  // Allocate some additional memory to help with memory debugging. At
  // debug level 2 on Windows, there are 40 bytes allocated to store
  // 10 program counters of 4 bytes each.  There are 4 bytes to store
  // the original user size.  Finally, there are between 8 and 11
  // additional bytes that are set to a pattern to allow buffer overflow
  // detection. If the user asked for a number of bytes that is not
  // a multiple of 4, then the rest of the word will be used for
  // buffer flow detection (along with the next 8 bytes).  NSK is similar
  // to Windows, but doesn't store 40 extra bytes for program counters.
  // A lookup table is the quickest way to determine how many extra bytes
  // should be used.
  if (debugLevel_ == 2) {
    const size_t additionalBytes[4] = { 12, 15, 14, 13 };
    additionalUserSize += additionalBytes[userSize & 0x3];
  }
#endif // (defined(_DEBUG) || defined(NSK_MEMDEBUG)) 

  size_t nb;
  NAHeapFragment *p;
  // Clean up prev_ and next_ pointers only when debugLevel_ is 0.
  // When debugLevel_ is either 1 or 2, user memory gets reset to some
  // eye catching pattern and it must not be overwritten for overflow
  // checking logic to work.
  NABoolean cleanUpPrevNext = debugLevel_== 0 ? TRUE : FALSE;

  // If the user is allocating a large piece of memory from a non-derived
  // heap that is also not the runtime statistics shared memory segment,
  // then allocate a NABlock using mmap(). This prevents any changes
  // to the "top_" fragment and allows the memory used by the request to
  // be returned to the operating system when the user frees it.
  if (additionalUserSize >= MIN_MMAP_ALLOC_SIZE && parent_ == NULL && (! getSharedMemory()))
  {

    nb = PAD_REQUEST(additionalUserSize);
    size_t reqSize = NAHeap::granularityAlign(nb+(2*sizeof(size_t))+BLOCK_OVERHEAD+1);
    NABlock *mmappedBlock = allocateMMapBlock(reqSize);
    if (mmappedBlock != NULL) {
      // Insert this NABlock after firstBlk_ so it doesn't interfere with
      // logic that assumes firstBlk_ contains the top fragment
      if (firstBlk_ != NULL) {
        mmappedBlock->next_ = firstBlk_->next_;
        firstBlk_->next_ = mmappedBlock;
      } else {
        mmappedBlock->next_ = NULL;
        firstBlk_ = mmappedBlock;
      }

      // Adjust the least addr
      if (least_addr_ == NULL || (char*)mmappedBlock < least_addr_)
        least_addr_ = (char*)mmappedBlock;

      // Set up the NAHeapFragment, adjust more statistics and return the memory.
      p = mmappedBlock->alignAsFragment();
      p->setSizeOfMMapFragment(nb);
      incrementStats(nb);
      allocDebugProcess(p, userSize);
      return p->getMemory();
    }
    // Fall through and try to allocate the regular way.
  }

  if (additionalUserSize <= MAX_SMALL_REQUEST) {

    nb = (additionalUserSize < MIN_REQUEST) ?
           MIN_FRAGMENT_SIZE : PAD_REQUEST(additionalUserSize);
    bindex_t idx = smallIndex(nb);
    binmap_t smallbits = smallmap_ >> idx;

    if ((smallbits & 0x3) != 0) { // Remainderless fit to a smallbin.
      idx += ~smallbits & 1;       // Uses next bin if idx empty
      NAHeapFragment *b = smallbinAt(idx);
      p = b->getNext();
      size_t firstFragBit = p->getFirstFragmentBit();
      size_t fragSize = smallIndex2Size(idx);
      if (!(p->fragmentSize() == fragSize))
      assert(p->fragmentSize() == fragSize);
      unlinkFirstSmallFragment(b, p, idx);
      p->setInuseAndPinuse(fragSize | firstFragBit);
#ifdef _DEBUG
          if (p->fragmentSize() == TraceAllocSize)
            saveTrafStack(getSAL(), p);
#endif // _DEBUG
      incrementStats(fragSize);
      allocDebugProcess(p, userSize);
      checkMallocedFragment(p, nb);
      return p->getMemory(cleanUpPrevNext);
    } else if (nb > dvsize_) {

      if (smallbits != 0) { // Use fragment in next nonempty smallbin
        binmap_t leftbits = (smallbits << idx) & leftBits(idx2bit(idx));
        binmap_t leastbit = leastBit(leftbits);
        bindex_t i = bit2idx(leastbit);
        NAHeapFragment *b = smallbinAt(i);
        NAHeapFragment *p = b->getNext();
        size_t firstFragBit = p->getFirstFragmentBit();
        size_t fragSize = smallIndex2Size(i);

        assert(p->fragmentSize() == fragSize);
        unlinkFirstSmallFragment(b, p, i);
        size_t rsize = fragSize - nb;
        // Fit here cannot be remainderless if 4byte sizes
        if (sizeof(size_t) != 4 && rsize < MIN_FRAGMENT_SIZE) {
          p->setInuseAndPinuse(fragSize | firstFragBit);
          incrementStats(fragSize);
        } else {
          p->setSizeAndPinuseOfInuseFragment(nb | firstFragBit);
          NAHeapFragment *r = p->fragmentPlusOffset(nb);
          r->setSizeAndPinuseOfFreeFragment(rsize);
          replaceDV(r, rsize);
          incrementStats(nb);
        }
        allocDebugProcess(p, userSize);
        checkMallocedFragment(p, nb);
#ifdef _DEBUG
          if (p->fragmentSize() == TraceAllocSize)
            saveTrafStack(getSAL(), p);
#endif // _DEBUG
        return p->getMemory(cleanUpPrevNext);
      }

      // Try to allocate from the smallest tree fragment
      if (treemap_ != 0 && (p = tmallocSmall(nb)) != 0) {
        incrementStats(p->fragmentSize());
        allocDebugProcess(p, userSize);
        checkMallocedFragment(p, nb);
#ifdef _DEBUG
          if (p->fragmentSize() == TraceAllocSize)
            saveTrafStack(getSAL(), p);
#endif // _DEBUG
        return p->getMemory(cleanUpPrevNext);
      }
    }
  } else if (additionalUserSize >= MAX_REQUEST) {
    nb = MAX_REQUEST + 100L; // Too big to allocate. Force failure in allocateBlock.
  } else {
    nb = PAD_REQUEST(additionalUserSize);

    if (treemap_ != 0 && (p = tmallocLarge(nb)) != 0) {
      allocDebugProcess(p, userSize);
      checkMallocedFragment(p, nb);
      incrementStats(p->fragmentSize());
#ifdef _DEBUG
          if (p->fragmentSize() == TraceAllocSize)
            saveTrafStack(getSAL(), p);
#endif // _DEBUG
      return p->getMemory(cleanUpPrevNext);
    }
  }

  // The following code allocates memory from the designated victim if
  // possible.
  if (nb <= dvsize_) {
    size_t rsize = dvsize_ - nb;
    p = dv_;
    size_t firstFragBit = p->getFirstFragmentBit();

    if (rsize >= MIN_FRAGMENT_SIZE) { // split dv
      NAHeapFragment *r = dv_ = p->fragmentPlusOffset(nb);
      dvsize_ = rsize;
      r->setSizeAndPinuseOfFreeFragment(rsize);
      p->setSizeAndPinuseOfInuseFragment(nb | firstFragBit);
      incrementStats(nb);
    } else { // exhaust dv
      size_t dvs = dvsize_;
      dvsize_ = 0;
      dv_ = 0;
      p->setInuseAndPinuse(dvs | firstFragBit);
      incrementStats(dvs);
    }
    allocDebugProcess(p, userSize);
    checkMallocedFragment(p, nb);
#ifdef _DEBUG
          if (p->fragmentSize() == TraceAllocSize)
            saveTrafStack(getSAL(), p);
#endif // _DEBUG
    return p->getMemory(cleanUpPrevNext);
  }

  // If the top fragment is not large enough to hold the allocation,
  // then call allocateBlock to allocate memory from either the
  // operating system or the parent memory.
  if (nb > topsize_) {

    // Save the current size of the first block because it will be lost
    // if the first block is resized.  The size is needed later in this
    // function.
    size_t prevFirstBlockSize = firstBlk_ ? firstBlk_->size_ : 0;

    // reqSize is the maximum size that may be needed to satisfy
    // this request. NOTE: "+1" may not be nececessary but was part of
    // the Doug Lea malloc().
    size_t reqSize = NAHeap::granularityAlign(nb+(2*sizeof(size_t))+BLOCK_OVERHEAD+1);

    // allocateBlock allocates memory of at least reqSize.
    NABlock *newBlock = allocateBlock(reqSize, failureIsFatal);
    if (newBlock == NULL) {

      // If the caller set up an error callback, then call it here.
      if (errCallback_ != NULL)
        (*errCallback_)(this, userSize);

      if (failureIsFatal) {
        handleExhaustedMemory();
        abort();
      }

      // Caller will handle the error so just return null.
      return NULL;
    }

    if (least_addr_ == NULL || (char*)newBlock < least_addr_)
      least_addr_ = (char*)newBlock;

    // If the current block was resized, then adjust the top size and
    // adjust the size of prevFoot_ in the first fragment.
    if (newBlock == firstBlk_) {
      assert(newBlock->size_ > prevFirstBlockSize);
      Lng32 increasedSize = (Lng32)(newBlock->size_ - prevFirstBlockSize);
      resizeTop(topsize_ + increasedSize);

      // Adjust prevFoot_ in first fragment.
      NAHeapFragment *firstFrag = newBlock->alignAsFragment();
      firstFrag->adjustBlockSize(increasedSize);
    } else {
      // Chain the new block on the linked-list of NABlocks.
      newBlock->next_ = firstBlk_;
      firstBlk_ = newBlock;

      // If the top fragment has not been initialized or has been
      // exhausted in a previous allocation, then call initTop() to
      // initialized a new top fragment. In other cases, the top
      // fragment will contain useful memory that can be used
      // by later allocations. The code calls addBlock() to add
      // the top fragment to the freelist and to create a new top
      // fragment in the newly allocated NABlock.
      if (topsize_ == 0) {
        initTop(newBlock);
        top_->initializeFirstFragment(topsize_);
      } else {
        addBlock(newBlock);
      }
    }
  }

  assert(topsize_ >= nb);

  // If execution reaches here, then the request can be satisfied from
  // the top fragment.  If the top fragment is large enough to have useful
  // memory after satisfying the request, the top fragment will be split
  // into two fragments.  The first fragment will be returned to the caller,
  // and a pointer to the second fragment will be assigned to the top
  // fragment pointer.
  size_t rsize = topsize_ - nb;
  p = top_;

  size_t firstFragmentBit = p->getFirstFragmentBit();

  NAHeapFragment *r;

  if (rsize >= MIN_FRAGMENT_SIZE) {
    topsize_ = rsize;
    r = top_ = p->fragmentPlusOffset(nb);
    r->setSizeAndPinuse(rsize);
    p->setSizeAndPinuseOfInuseFragment(nb|firstFragmentBit);
  } else {
    // The top fragment has been exhausted. Set top_ to point to
    // the next fragment in case a later allocation resizes the
    // current NABlock.
    nb = topsize_;
    topsize_ = 0;
    top_ = p->fragmentPlusOffset(nb);
    p->setInuseAndPinuse(nb|firstFragmentBit);
  }
  checkTopFragment(top_);
  incrementStats(nb);
  allocDebugProcess(p, userSize);
  checkMallocedFragment(p, nb);
#ifdef _DEBUG
          if (p->fragmentSize() == TraceAllocSize)
            saveTrafStack(getSAL(), p);
#endif // _DEBUG
  return p->getMemory(cleanUpPrevNext);
}

// NAHeap::deallocateHeapMemory()
//   Memory is deallocated in this function.  The memory fragments
//   that may exist before and after this freed fragment are coalesced
//   with the freed memory if possible.  If the previous memory or
//   next memory is the designated victim, then the designated victim
//   will be adjusted to include this memory.  If the next memory is
//   the top fragment, then the top fragment will now include the
//   coalesced memory.  If all memory in an NABlock is freed, then
//   NAMemory::deallocateFreeBlock() will be called to either free
//   the block to the OS or to the parent memory.  If the memory
//   is not all freed and is not the designated victim or top fragment,
//   the fragment will be linked into a small bin or tree.
void NAHeap::deallocateHeapMemory(void* addr)
{
  // If addr is NULL, we are done.
  // This is apparently standard behavior. I don't like it, but some of
  // our code relies on it.
  if (addr == NULL)
    return;


  assert(type_ != NO_MEMORY_TYPE);

  NAMutex mutex(threadSafe_, &mutex_);
 
  if (getSharedMemory())
  {
    // Check if you are within semaphore
    if (! checkIfRTSSemaphoreLocked())
       NAExit(1);
  }

  HEAPLOG_DELETE_ENTRY(addr, heapID_.heapNum)

  // Get a pointer to the NAHeapFragement for this memory address
  NAHeapFragment *p  = NAHeapFragment::memToFragment(addr); 

  if (p->fragmentSize() == 0 && okCinuse(p) && okPinuse(p)) // allocateAlignedHeapMemory fragment?
  {
    p = (NAHeapFragment *)((size_t)(p) - p->getPrevFoot());
  }

#ifdef _DEBUG
  if (TraceAllocSize > 0 && p->fragmentSize() == TraceAllocSize)
    delTrafStack(getSAL(), p);
#endif // _DEBUG

  checkInuseFragment(p);
  if (!(RTCHECK(okAddress(p) && okCinuse(p)))) {
    USAGE_ERROR_ACTION;
    return;
  }

  // deallocDebugProcess() is only called in debug builds.  If
  // MEMDEBUG=1, then the freed memory is set to a 0xfdfdfdfd
  // pattern.  If MEMDEBUG=2, then a check is made to see if
  // there was a buffer overflow.
  deallocDebugProcess(p);

  // get the size of the current fragment
  size_t psize = p->fragmentSize();

  // update allocation statistics
  decrementStats(psize);

  if (type_ == EXECUTOR_MEMORY &&
      deallocTraceArray != NULL &&
      (memcmp(name_, "Global Executor Memo", 20) == 0))
  {
    deallocCount += 1;
    unsigned short i = deallocTraceIndex == deallocTraceEntries - 1 ? 0 : deallocTraceIndex + 1;
    (*deallocTraceArray)[i].begin_ = (void *)p;
    (*deallocTraceArray)[i].end_ = (void *)((char *)p + psize);
    deallocTraceIndex = i;
  }

  if (p->isMMapped()) {
    deallocateFreeBlock(p);
    return;
  }

  // Get a pointer to the next fragment
  NAHeapFragment *next = p->fragmentPlusOffset(psize);
  // Pointer to the previous fragment
  NAHeapFragment *prev = NULL;

  size_t firstFragmentBit;  // Is this the first fragment of a NABlock?

  // Save the pointer to this fragment and the size
  NAHeapFragment *q = p;
  size_t qsize = psize;

  Int32 mergeFlags = NO_MERGE;

  if (p->pinuse()) {
    firstFragmentBit = p->getFirstFragmentBit();
  } else {
    mergeFlags |= BACKWARD_MERGE;
    size_t prevsize = p->getPrevFoot();
    prev = p->fragmentMinusOffset(prevsize);
    firstFragmentBit = prev->getFirstFragmentBit();
    psize += prevsize;
    p = prev;
    if (RTCHECK(okAddress(prev))) { // consolidate backward
      if (p != dv_) {
        unlinkFragment(p, prevsize);
      } else if (next->cpinuse()) { // CINUSE_BIT and PINUSE_BIT set.
        // No further consolidation can take place.  The previous
        // fragment was the designated victim so update the dvsize.
        dvsize_ = psize;
        p->setFreeWithPinuse(psize, next);
        p->setHeadBits(firstFragmentBit);
        if (p->occupiesCompleteNABlock())
          deallocateFreeBlock(p);
        else
          q->releaseFreePages(prev, next, mergeFlags);
        return;
      }
    } else {
      USAGE_ERROR_ACTION;
      return;
    }
  }

  if (!(RTCHECK(okNext(p, next) && okPinuse(next)))) {
    USAGE_ERROR_ACTION;
    return;
  }

  if ((!next->cinuse()) || 
      ((next == top_) && (next->isFencePost()) && (topsize_ == 0))) 
  {  // consolidate forward
    mergeFlags |= FORWARD_MERGE;
    if (next == top_) {
      if ((top_->isFencePost()) && (topsize_ == 0))
         top_->clearPinuse(); // If top was set to fencepost because top was
                              // exhausted, reset the pinuse bit of the 
                              // fencepost, because the prev of the fencepost 
                              // is now top_, which should not be in use.
      size_t tsize = topsize_ += psize;
      top_ = p;
      p->setSizeAndPinuse(tsize);
      p->setHeadBits(firstFragmentBit);
      if (p == dv_) {
        dv_ = NULL;
        dvsize_ = 0;
      }
      if (p->occupiesCompleteNABlock())
        deallocateFreeBlock(p);
      else
        q->releaseFreePages(prev, next, mergeFlags);
      return;
    } else if (next == dv_) {
      size_t dsize = dvsize_ += psize;
      dv_ = p;
      p->setSizeAndPinuseOfFreeFragment(dsize);
      p->setHeadBits(firstFragmentBit);
      if (p->occupiesCompleteNABlock())
        deallocateFreeBlock(p);
      else
        q->releaseFreePages(prev, next, mergeFlags);
      return;
    } else {
      size_t nsize = next->fragmentSize();
      psize += nsize;
      unlinkFragment(next, nsize);
      p->setSizeAndPinuseOfFreeFragment(psize);
      p->setHeadBits(firstFragmentBit);
      if (p == dv_) {
        dvsize_ = psize;
        if (p->occupiesCompleteNABlock())
          deallocateFreeBlock(p);
        else
          q->releaseFreePages(prev, next, mergeFlags);
        return;
      }
    }
  } else {  // This fragment could not be consolidated forward
    p->setFreeWithPinuse(psize, next);
    p->setHeadBits(firstFragmentBit);
  }

  // If this fragment uses the whole NABlock, then deallocate it.
  if (p->occupiesCompleteNABlock()) {
    // Only return if the deallocation really occurred.
    if (deallocateFreeBlock(p))
      return;
  }
   
  q->releaseFreePages(prev, next, mergeFlags);

  // Insert the free fragment into either a small bin or tree
  insertFragment(p, psize);
  checkFreeFragment(p);

  return;
}

#endif // !MUSE

#if (defined(_DEBUG) || defined(NSK_MEMDEBUG))
void NAHeap::dumpHeapInfo(ostream* outstream, Lng32 indent)
{
  NAMutex mutex(threadSafe_, &mutex_);

  char ind[100];

  Lng32 indIdx = 0;
  for (; indIdx < indent; indIdx++)
    ind[indIdx] = ' ';
  ind[indIdx] = '\0';

  // Verify the malloc state.  This can be fairly heavyweight depending
  // on the number of allocations.
  checkMallocState();

  if (!outstream)
    outstream = &cerr;
  *outstream << ind << "Dump of Heap             " << this << ':' << endl
	     << ind << "Name:                    " << name_ << endl
	     << ind << "Parent:                  "
	     << ind << (parent_ ? parent_->name_ : " ") << endl
	     << ind << "Initial Size (Bytes):    " << initialSize_ << endl
	     << ind << "Maximum Size (Bytes):    " << maximumSize_ << endl
	     << ind << "Increment Size (Bytes):  " << incrementSize_ << endl
	     << ind << "Allocated Size (Bytes):  " << allocSize_ << endl
	     << ind << "High Water Mark (Bytes): " << highWaterMark_ << endl
             << ind << "Interval Water Mark (Bytes): " << intervalWaterMark_ << endl
	     << ind << "Allocated Units:         " << allocCnt_ << endl
	     << ind << "Total Size (Bytes):      " << totalSize_ << endl
	     << ind << "Block Count:             " << blockCnt_ << endl;
  
  MemoryStats freeStats;
  MemoryStats allocStats;
  NABlock * p = firstBlk_;
  while (p) {
    p->dump(outstream, debugLevel_, freeStats, allocStats, top_, indent);
    p = p->next_;
  }

  if (debugLevel_ == 0) {
    *outstream << "dynamicAllocs not kept because debugLevel_ == 0" << endl;
  } else {
    dynamicAllocs.dump(outstream, "all allocated", indent);
  }
  allocStats.dump(outstream, "leftover allocated", indent);
  freeStats.dump(outstream, "free", indent);
  *outstream << ind << "---------------------------------------------" << endl;

  // we are done with this heap. Go thru all the derived heaps
  // and dump their info too
  NAMemory *derivedHeap = memoryList_;
  while (derivedHeap) {
    derivedHeap->dump(outstream, indent + 2);
    derivedHeap = derivedHeap->nextEntry_;
  }
}
#endif // (defined(_DEBUG) || defined(NSK_MEMDEBUG))  ...

//////////////////////////////////////////////////////////////////////////////

// ---------------------------------------------------------------------------
// NABlock methods
// ---------------------------------------------------------------------------

#if (defined(_DEBUG) || defined(NSK_MEMDEBUG))
void NABlock::dump(ostream* outstream,
                   Lng32 debugLevel,
                   MemoryStats& freeStats,
                   MemoryStats& allocStats,
                   NAHeapFragment *top,
                   Lng32 indent)
{
  char ind[100];
  Lng32 indIdx = 0;
  for (; indIdx < indent; indIdx++)
    ind[indIdx] = ' ';
  ind[indIdx] = '\0';
  if (!outstream)
    outstream = &cerr;

  // go thru all allocated and free fragments and for Windows dump the
  // stack trace for allocated fragments
  NAHeapFragment *q = alignAsFragment();

  while (blockHolds(q) && q != top && !q->isFencePost()) {

    if (q->cinuse()) {
      // this is an allocated fragment.
      allocStats.addEntry(q->fragmentSize());


    } else {
      // this is a free fragment
      freeStats.addEntry(q->fragmentSize());
    }

    // Go to next fragment
    q = q->nextFragment();
  }
}
#endif // (defined(_DEBUG) || defined(NSK_MEMDEBUG))  ...

//////////////////////////////////////////////////////////////////////////////

MemoryStats::MemoryStats()
  : count_(0),
    sum_(0.0),
    sum2_(0.0)
{
  for (Lng32 i = 0; i < 18; i++)
    histBuckets_[i] = 0;
}

void MemoryStats::addEntry(size_t value)
{
  count_++;  // one more entry
  double fvalue = (double)value;
  sum_ += fvalue;
  sum2_ += (fvalue * fvalue);
  // determine the histogram bucket
  value = value >> 5;  // everything <= 32 ends up in the first bucket
  Lng32 i = 0;
  while(value) {
    value = value >> 1;
    i++;
  }
  // everything bigger than 2 MB ends up in the last bucket
  i = (i < 18) ? i : 17;
  histBuckets_[i]++;
}

#if (defined(_DEBUG) || defined(NSK_MEMDEBUG))
void MemoryStats::dump(ostream* outstream, const char * name, Lng32 indent)
{
  char ind[100];
  Lng32 indIdx = 0;
  for (; indIdx < indent; indIdx++)
    ind[indIdx] = ' ';
  ind[indIdx] = '\0';
  if (!outstream)
    outstream = &cerr;
  *outstream << ind << name << " units:" << endl
	     << ind << "# of entries: " << count_ << endl
	     << ind << "average size: " << (count_ ? (sum_/count_) : 0.0)
	     << endl
	     << ind << "variance:     " << ((count_ > 1) ?
    ((1.0/(double)(count_ - 1)) *
     (sum2_ - (1.0/(double)count_) * sum_ * sum_)) : 0.0) << endl;
  
  Lng32 size = 16;
  char unit = 'B';
  for (Lng32 i = 0; i < 17; i++) {
    size *= 2;
    if (i == 5) {
      unit = 'K';
      size /= 1024;
    }
    else if (i == 15) {
      size = size/1024;
      unit = 'M';
    }
    *outstream << ind << "# entries <= " << size << unit
	       << " : " << histBuckets_[i] << endl;
  }
  *outstream << ind << "# entries >  " << size << unit
	     << " : " << histBuckets_[17] << endl;
  
}
#endif // (defined(_DEBUG) || defined(NSK_MEMDEBUG)) 


// -------------------- Debugging Support ---------------------------
#if ( defined(_DEBUG) || defined(NSK_MEMDEBUG) )  

// The below values are used for buffer overflow detection. Lookup
// tables are used so that the logic for checking the remaining
// bytes in a word can be very quick.
const UInt32 buf_overflow_val = 0xF2F2F2F2; 
#ifdef NA_LITTLE_ENDIAN
const UInt32 overflow_mask[4] = { 0, 0xFFFFFF00, 0xFFFF0000, 0xFF000000 };
const UInt32 overflow_val[4] = { 0, 0xF2F2F200, 0xF2F20000, 0xF2000000 };
#else // NA_BIG_ENDIAN
const UInt32 overflow_mask[4] = { 0, 0xFFFFFF, 0xFFFF, 0xFF };
const UInt32 overflow_val[4] = { 0, 0xF2F2F2, 0xF2F2, 0xF2 };
#endif 

// doAllocDebugProcess() is called with debugLevel_ is set to at
// least 1.  It resets newly allocated memory to a 0xfafafafa. On
// Windows when debugLevel is set to 2, this function also adds
// information to indicate whether the end of the buffer has been
// overwritten and saves the last 10 levels of the stack frame.

void
NAHeap::doAllocDebugProcess(NAHeapFragment *p, size_t userSize)
{
  // Reset the user requested memory to a 0xfafafafa pattern.
  // Disabled for now, see Todo 1)
  // memset(p->getMemory(), 0xfa, userSize);

  if (debugLevel_ == 2 && p->fragmentSize() < INT_MAX) {

    // Determine location that the user requested size is stored
    // in the memory.
    size_t userSizeOffset = p->fragmentSize() - (2 * sizeof(size_t));
    UInt32 * userSizePtr = (UInt32 *)((char*)p->getMemory() + userSizeOffset);

    // Set the integer pointer to the last word in the user
    // requested memory
    UInt32 *lastWord = (UInt32*)((char*)p->getMemory() +
          ((userSize - 1) & ~(0x3))); 

    // Set the bits in the first word.  If the user memory uses the
    // whole word, then the user memory will not be modified.
    UInt32 remainder = userSize & 0x3;
    lastWord[0] = (lastWord[0] & ~overflow_mask[remainder])
                  | (buf_overflow_val & overflow_mask[remainder]); 
    // Set the bits in the next 2 words.
    lastWord[1] = buf_overflow_val; 
    lastWord[2] = buf_overflow_val; 
    // Store the userSize in the buffer.
    // Note that the userSizePtr may point to the same location of lastWord[2]
    *userSizePtr = (UInt32) userSize;

  }
}

// doDeallocDebugProcess() resets the deallocated memory to
// a 0xfdfdfdfd.  This function is only called when debugLevel_ is
// at set to at least 1.

void
NAHeap::doDeallocDebugProcess(NAHeapFragment *p)
{
  if (debugLevel_ == 2 && p->fragmentSize() < INT_MAX) {
    // Check for buffer overflows
    p->checkBufferOverflow();
  }

  // Reset the user memory to a 0xfdfdfdfd pattern.
  // size_t userSize = p->fragmentSize() - sizeof(size_t);
  // Disabled for now, see Todo 1)
  // memset(p->getMemory(), 0xfd, userSize);
}

// Check properties of any fragment, whether free, inuse, etc.
void
NAHeap::doCheckAnyFragment(NAHeapFragment *p)
{
  assert((isAligned(p->getMemory())) || (p->isFencePost()));
  assert(okAddress(p));
}

// Check properties of top fragment
void
NAHeap::doCheckTopFragment(NAHeapFragment *p)
{
  NABlock* sp = NABlock::blockHolding(firstBlk_,p);
  size_t  sz = p->fragmentSize();
  assert(sp != 0);
  assert(okAddress(p));
  assert(p->pinuse());
  if (topsize_ == 0) {
    assert(p->isFencePost());
  } else {
    assert(!p->isFencePost());
    assert(isAligned(p->getMemory()));
    assert(sz == topsize_);
    assert(sz > 0);
    assert(sz == (size_t)sp + sp->size_
               - (size_t)p - sp->firstFragmentOffset());
    assert(!p->nextPinuse());
  }
}

// Check properties of inuse fragments
void
NAHeap::doCheckInuseFragment(NAHeapFragment *p)
{
  doCheckAnyFragment(p);
  assert(p->cinuse());
  assert(p->nextPinuse());
  // If not pinuse, previous fragment has OK offset
  assert(p->pinuse() || p->prevFragment()->nextFragment() == p);
}

// Check properties of free fragments
void
NAHeap::doCheckFreeFragment(NAHeapFragment *p)
{
  size_t sz = p->fragmentSize();
  NAHeapFragment *next = p->fragmentPlusOffset(sz);
  doCheckAnyFragment(p);
  assert(!p->cinuse());
  assert(!p->nextPinuse());
  if (p != dv_ && p != top_) {
    if (sz >= MIN_FRAGMENT_SIZE) {
      assert((sz & FRAG_ALIGN_MASK) == 0);
      assert(isAligned(p->getMemory()));
      assert(next->getPrevFoot() == sz);
      assert(p->pinuse());
      assert (next == top_ || next->cinuse());
      assert(p->getNext()->getPrev() == p);
      assert(p->getPrev()->getNext() == p);
    }
    else  // markers are always of size size_t
      assert(sz == sizeof(size_t));
  }
}

// Check properties of malloced fragments at the point they are malloced
void
NAHeap::doCheckMallocedFragment(NAHeapFragment *p, size_t s)
{
  if (p != 0) {
    size_t sz = p->fragmentSize();
    doCheckInuseFragment(p);
    assert((sz & FRAG_ALIGN_MASK) == 0);
    assert(sz >= MIN_FRAGMENT_SIZE);
    assert(sz >= s);

    // size is less than MIN_FRAGMENT_SIZE more than request
    assert(sz < (s + MIN_FRAGMENT_SIZE));
  }
}

// Check a tree and its subtrees
void
NAHeap::doCheckTree(NATreeFragment *t)
{
  NATreeFragment *head = NULL;
  NATreeFragment *u = t; 
  bindex_t tindex = t->getIndex();
  size_t tsize = t->fragmentSize();
  bindex_t idx = computeTreeIndex(tsize);

  assert(tindex == idx);
  assert(tsize >= MIN_LARGE_SIZE);
  assert(tsize >= minsizeForTreeIndex(idx));
  assert((idx == NTREEBINS-1) || (tsize < minsizeForTreeIndex((idx+1))));
  
  do { // traverse through chain of same-sized nodes
    doCheckAnyFragment((NAHeapFragment*)u);
    assert(u->getIndex() == tindex);
    assert(u->fragmentSize() == tsize);
    assert(!u->cinuse());
    assert(!u->nextPinuse());
    assert(u->getNext()->getPrev() == u);
    assert(u->getPrev()->getNext() == u);
    if (u->getParent() == NULL) {
      assert(u->getChild(0) == NULL);
      assert(u->getChild(1) == NULL);
    }
    else {
      assert(head == 0); // only one node on chain has parent
      head = u;
      assert(u->getParent() != u);
      assert (u->getParent()->getChild(0) == u ||
              u->getParent()->getChild(1) == u ||
              *((NATreeFragment**)u->getParent()) == u);
      if (u->getChild(0) != NULL) {
        assert(u->getChild(0)->getParent() == u);
        assert(u->getChild(0) != u);
        doCheckTree(u->getChild(0));
      }
      if (u->getChild(1) != NULL) {
        assert(u->getChild(1)->getParent() == u);
        assert(u->getChild(1) != u);
        doCheckTree(u->getChild(1));
      }
      if (u->getChild(0) != NULL && u->getChild(1) != NULL) {
        assert(u->getChild(0)->fragmentSize() <
               u->getChild(1)->fragmentSize());
      }
    }
    u = (NATreeFragment*)u->getNext();
  } while (u != t);
  assert(head != NULL);
}

// Check all the fragments in a treebin
void
NAHeap::doCheckTreebin(bindex_t i)
{
  NATreeFragment **tb = treebinAt(i);
  NATreeFragment *t = *tb;
  NABoolean empty = (treemap_ & (1U << i)) == 0;
  if (t == 0)
    assert(empty);
  if (!empty)
    doCheckTree(t);
}

// Check all the fragments in a smallbin
void
NAHeap::doCheckSmallbin(bindex_t i)
{
  NAHeapFragment *b = smallbinAt(i);
  NAHeapFragment *p = b->getPrev();
  NABoolean empty = (smallmap_ & (1U << i)) == 0;
  if (p == b)
    assert(empty);
  if (!empty) {
    for (; p != b; p = p->getPrev()) {
      size_t size = p->fragmentSize();
      NAHeapFragment *q;
      // each fragment claims to be free
      doCheckFreeFragment(p);
      // fragment belongs in bin
      assert(smallIndex(size) == i);
      assert(p->getPrev() == b ||
             p->getPrev()->fragmentSize() == p->fragmentSize());
      // fragment is followed by an inuse fragment
      q = p->nextFragment();
      if (!q->isFencePost())
        doCheckInuseFragment(q);
    }
  }
}

// Check all of the properties in NAMemory related to Doug Lea's malloc()
void
NAHeap::doCheckMallocState()
{
  bindex_t i;
  size_t total;

  // check bins
  for (i = 0; i < NSMALLBINS; ++i)
    doCheckSmallbin(i);
  for (i = 0; i < NTREEBINS; ++i)
    doCheckTreebin(i);

  if (dvsize_ != 0)  { // check dv fragment
    doCheckAnyFragment(dv_);
    assert(dvsize_ == dv_->fragmentSize());
    assert(dvsize_ >= MIN_FRAGMENT_SIZE);
    assert(binFind(dv_) == 0);
  }

  if (top_ != NULL) {   // check top fragment
    doCheckTopFragment(top_);
    if (topsize_ == 0) {
      assert(top_->isFencePost());
    } else {
      assert(topsize_ == top_->fragmentSize());
    }
    assert(binFind(top_) == 0);
  }

  total = traverseAndCheck();
  assert(total <= totalSize_);
  assert(allocSize_ <= highWaterMark_);
  assert(allocSize_ <= intervalWaterMark_);

  // also check for overflow
  if (debugLevel_ == 2)
    checkForOverflow();

}

// Find x in a bin. Used in other check functions.
NABoolean
NAHeap::binFind(NAHeapFragment *x)
{
  size_t size = x->fragmentSize();
  if (isSmall(size)) {
    bindex_t sidx = smallIndex(size);
    NAHeapFragment *b = smallbinAt(sidx);
    if (smallmapIsMarked(sidx)) {
      NAHeapFragment *p = b;
      do {
        if (p == x)
          return TRUE;
      } while ((p = p->getNext()) != b);
    }
  } else {
    bindex_t tidx = computeTreeIndex(size);
    if (treemapIsMarked(tidx)) {
      NATreeFragment *t = *treebinAt(tidx);
      size_t sizebits = size << leftshiftForTreeIndex(tidx);
      while (t != 0 && t->fragmentSize() != size) {
        t = t->getChild((sizebits >> (SIZE_T_BITSIZE - 1)) & 1);
        sizebits <<= 1;
      }
      if (t != 0) {
        NATreeFragment *u = t;
        do {
          if (u == (NATreeFragment*)x)
            return TRUE;
        } while ((u = (NATreeFragment*)u->getNext()) != t);
      }
    }
  }
  return FALSE;
}

// Traverse each fragment and check it; return total
size_t
NAHeap::traverseAndCheck()
{
  size_t sum = 0;
  size_t blockCnt = 0;
  NABlock *s = firstBlk_;

  if (s != NULL) {
    sum += topsize_ + BLOCK_OVERHEAD;

    while (s != NULL) {
      ++blockCnt;
      NAHeapFragment *q = s->alignAsFragment();

      // The first fragment of an externally allocated segment should
      // not have the first fragment bit set. If the NABlock was allocated
      // using mmap, then either the first fragment should have the first
      // fragment bit set or should have the MMAPPED_BIT set. If mmap was
      // not used to allocate the NABlock, then the first fragment should
      // have the first fragment bit set. All other fragments should not
      // have either bit set.
      if (s->isExternSegment()) {
        assert(!q->getFirstFragmentBit());
      } else {
        if (s->isMMapped()) {
          assert(q->getFirstFragmentBit() || q->isMMapped());
        } else {
          assert(q->getFirstFragmentBit());
        }
      }

      NAHeapFragment *lastq = NULL;
      assert(q->pinuse());
      while (s->blockHolds(q) && q != top_) {
        sum += q->fragmentSize();
        if (q->cinuse()) {
          assert(!binFind(q));
          doCheckInuseFragment(q);
        } else {
          assert(q == dv_ || binFind(q));
          assert(lastq == NULL || lastq->cinuse()); // Not 2 consecutive free
          doCheckFreeFragment(q);
        }
        lastq = q;
        q = q->nextFragment();

        // Quit if this is a fencepost. This is put here because a fencepost
        // may have the same bit set as a mmap'ed fragment.
        if (q->isFencePost())
          break;
        assert(!q->getFirstFragmentBit());
        assert(!q->isMMapped());
      }
      s = (NABlock*)s->next_;
    }
  }
  assert (blockCnt == blockCnt_);
  return sum;
}

void
NAHeapFragment::checkBufferOverflow()
{
  size_t userSizeOffset = fragmentSize() - (2 * sizeof(size_t));
  UInt32 *userSizePtr = (UInt32*)((char*)getMemory() + userSizeOffset);
  UInt32 userSize = *userSizePtr;

  // Set pointer to the last word of the user allocated memory.
  UInt32 *lastWord = (UInt32*)((char*)getMemory() 
        + ((userSize - 1) & ~(0x3))); 

  // Check for buffer overflow;
  UInt32 remainder = userSize & 0x3;
  if ((lastWord[0] & overflow_mask[remainder]) != overflow_val[remainder] || 
      lastWord[1] != buf_overflow_val) {
//    char buf[128];
//    sprintf(buf, "Buffer overflow of memory allocated at %p (Size=" PFSZ ")\n",
//            getMemory(), userSize);
//    cerr << buf;
    assert(0);  // overflow detected
  }
  if (userSizePtr != &lastWord[2] && lastWord[2] != buf_overflow_val)
    assert(0);  // overflow detected
}

// Traverse over all of the blocks for this NAHeap to see if any of them contain
// buffers that have overflowed.
void
NAHeap::checkForOverflow()
{
  NABlock *bp = firstBlk_;
  while (bp != NULL) {
    NAHeapFragment *fp = bp->alignAsFragment();
    while (!fp->isFencePost()) {
      if (fp->cinuse()) {
        fp->checkBufferOverflow();
      }
      fp = fp->nextFragment();
    }
    bp = bp->next_;
  }
}

#endif // ( defined(_DEBUG) || defined(NSK_MEMDEBUG) )  

#ifdef _DEBUG
void
NAHeap::setAllocTrace()
{
  static THREAD_P bool traceEnvChecked = 0;

  if (!traceEnvChecked)
    {
      char *traceEnv = getenv("TRACE_ALLOC_SIZE");
      if (traceEnv != NULL && atol(traceEnv) > 0)
        TraceAllocSize = atol(traceEnv);
      else
        TraceAllocSize = 0;
      traceEnvChecked = true;
    }
  if (TraceAllocSize != 0 && memcmp(name_, "Process Stats Heap", 18))
    la_ = new LIST(TrafAddrStack *)(NULL); // on C++ heap
  else
    la_ = NULL;
}
#endif // _DEBUG


// -----------------------------------------------------------------------
// methods for class DefaultIpcHeap
// -----------------------------------------------------------------------

// This object is allocated on either global or a passed-in heap. 
// Its dstr (defined here) is never explicitly called. 
DefaultIpcHeap::~DefaultIpcHeap()
{
   destroy();
}
void DefaultIpcHeap::destroy()
{
  // This class can't delete anything because it doesn't keep track of
  // any of its allocations.
}

void * DefaultIpcHeap::allocateIpcHeapMemory(size_t size, NABoolean failureIsFatal)
{
  if (! checkSize(size, failureIsFatal))
     return NULL;

  void * rc = ::operator new(size);
  HEAPLOG_ADD_ENTRY(rc, size, heapID_.heapNum, getName())
  if (rc) return rc;
  if (failureIsFatal) 
    {
      handleExhaustedMemory();
      abort();
    }
  return rc; 
}

void DefaultIpcHeap::deallocateIpcHeapMemory(void * buffer)
{
  HEAPLOG_DELETE_ENTRY(buffer, heapID_.heapNum)
  ::operator delete(buffer);
}

#if (defined(_DEBUG) || defined(NSK_MEMDEBUG))
void DefaultIpcHeap::dumpIpcHeapInfo(ostream* outstream, Lng32 indent)
{
  // This class doesn't keep track of anything so it can't dump anything
  // useful.
}
#endif

// Release unused memory in the heap back to kernel. This will help
// reduce the memory pressure and increase the free page count.
void NAHeapFragment::releaseFreePages(NAHeapFragment *prev, 
                                      NAHeapFragment *next, 
                                      Int32 mergeFlags)
{
}

SEG_ID gStatsSegmentId_ = -1;
#define RMS_SEGMENT_ID_OFFSET 0x10000000

SEG_ID getStatsSegmentId()

{
  Int32 segid;
  Int32 error;
  if (gStatsSegmentId_ == -1)
  {
   error = msg_mon_get_my_segid(&segid);
   assert(error == 0);//XZFIL_ERR_OK);
   gStatsSegmentId_ = segid + RMS_SEGMENT_ID_OFFSET;
  }
  return gStatsSegmentId_; 
}

