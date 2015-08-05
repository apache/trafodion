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
/* -*-C++-*- *****************************************************************************
 *
 * File:         ComSpace.h
 * Description:  The Space object is derived from NAMemory. It is a
 *               specialized object that doesn't handle deallocations
 *               (deallocated memory is just lost) and that has management
 *               functions to generate a contiguous SQL object with pointers
 *               replaced by offset in the code generator.
 *
 * Created:      7/10/95
 * Language:     C++
 *
 *
 *****************************************************************************
 */

#ifndef COM_SPACE_H
#define COM_SPACE_H


#include <stddef.h>
#include "CollHeap.h"


class Block;
#ifdef NA_STD_NAMESPACE
#include <iosfwd>
using namespace std;
#else
class ostream;
#endif

/////////////////////////////////////////////////////
//
// class ComSpace
//
/////////////////////////////////////////////////////
typedef class ComSpace : public CollHeap {
  friend class ExHeapStats;  // Muse uses private data members
  friend class ExSpaceStats; // Other muse class that needs private access
public:
  enum SpaceType
    {
      DP2_SPACE,             // executor in DP2
      EXECUTOR_SPACE,        // master executor and ESPs
      GENERATOR_SPACE,       // arkcmp, used to generate module files and rfork
      SYSTEM_SPACE,
      SINGLE_BLOCK_SPACE
    };

  Int32 operator == (const ComSpace & other) const
    { return this == &other; };

private:
  // recommended/default size for block. Caller can allocate
  // a different size, though, and pass that to
  // the init method.
  enum BlockSize
    { DP2_BLOCK_MAX_SIZE = 100000
      // EXECUTOR_BLOCK_MAX_SIZE = 131072,
      // SYSTEM_BLOCK_MAX_SIZE = 10240
    };

  SpaceType type_;

  // if this space is an EXECUTOR_SPACE or SYSTEM_SPACE, it is
  // allocated from parent_! If parent_ is NULL, it is allocated from
  // the system heap.

  //mar NAHeap * parent_;
  //mar
  //mar NB: parent_ is always a NAHeap*, but we avoid some really nasty
  //mar multiple-inheritance evilness if we store this data member as
  //mar the parent class (it'll do the right thing and call the proper
  //mar virtual method when needed)
  CollHeap * parent_ ;

  Block * firstBlock_;
  Block * lastBlock_;
  Block * searchList_;

  Lng32 initialSize_;

  NABoolean fillUp_;  // flag to indicate if we ever revisit a block to
                      // look for free space. The default is true. If
                      // fillUp_ is false, we can guarantee that objects
                      // in the space are always allocated in cronological
                      // order. ExStatsArea.formatStatistics() relies on
                      // this.
  // don't call this directly, as allocateAlignedSpace() won't work
  // if some of the requests are for non-aligned space
NA_EIDPROC
  char * privateAllocateSpace(ULng32 size, NABoolean failureIsFatal = TRUE);

  // allocate a block of the indicated length (must be a multiple
  // of 8)
NA_EIDPROC
  Block * allocateBlock(SpaceType type,
                        Lng32 in_block_size = 0,
                        NABoolean firstAllocation = FALSE,
			char * in_block_addr = NULL,
			NABoolean failureIsFatal = TRUE);

public:
NA_EIDPROC
  ComSpace(SpaceType type = SYSTEM_SPACE, NABoolean fillUp = TRUE, char *name = NULL);
NA_EIDPROC
  void destroy();

NA_EIDPROC
  ~ComSpace();


NA_EIDPROC
  void freeBlocks(void);

NA_EIDPROC
  void setType(SpaceType type, Lng32 initialSize = 0);

NA_EIDPROC
  void setFirstBlock(char * blockAddr, Lng32 blockLen, NABoolean failureIsFatal = TRUE);

NA_EIDPROC
  void setParent(CollHeap * parent);

NA_EIDPROC
  char * allocateAlignedSpace(ULng32 size, NABoolean failureIsFatal = TRUE);

NA_EIDPROC
  char * allocateAndCopyToAlignedSpace(const char* dp,
				       size_t dlen,
				       size_t countPrefixSize = 0,
				       NABoolean failureIsFatal = TRUE,
				       NABoolean noSizeAdjustment = FALSE);

  // returns total space allocated size (space allocated by the user)
NA_EIDPROC
  inline Lng32 getAllocatedSpaceSize(){
    return allocSize_;
  }

NA_EIDPROC
  void * allocateSpaceMemory(size_t size, NABoolean failureIsFatal = TRUE);

NA_EIDPROC
  void deallocateSpaceMemory(void *)
  { /* no op */ };

NA_EIDPROC
  Long convertToOffset(void *);

NA_EIDPROC
  void* convertToPtr(Long offset) const;

NA_EIDPROC
  Lng32 allocAndCopy(void *, ULng32, NABoolean failureIsFatal = TRUE);

NA_EIDPROC
  short isOffset(void *);

NA_EIDPROC
  NABoolean isOverlappingMyBlocks(char * buf, ULng32 size);

  // moves all the Blocks into the output contiguous buffer.
NA_EIDPROC
  char * makeContiguous(char * out_buf, ULng32 out_buflen);

#if (defined(_DEBUG) || defined(NSK_MEMDEBUG)) && !defined(__EID) && !defined(STAND_ALONE)
NA_EIDPROC
  void dumpSpaceInfo(ostream *outstream, Lng32 indent);
#endif

NA_EIDPROC
  static void outputBuffer(ComSpace * space, char * buf, char * newbuf);

NA_EIDPROC
  static void display(char *buf, size_t buflen,
		      size_t countPrefixSize, ostream &outstream);

NA_EIDPROC
  static Lng32 defaultBlockSize(SpaceType type);

NABoolean outputbuf_; // set to false until we use the buffer for output

} Space;

NA_EIDPROC
void * operator new(size_t size, ComSpace *s);

#ifndef ARKFS_OPEN
#endif // !ARKFS_OPEN


/////////////////////////////////////////////
//
// class Block
//
// A block is a contiguous space of bytes.
// The first sizeof(Block) bytes are the
// Block itself. The space following it is
// the dataspace.
// There is no constructor or destructor for
// this class since the caller allocates
// space from dp2 segment, executor segment
// or system memory. See class Space.
/////////////////////////////////////////////
class Block {
  Lng32 blockSize_;

  Lng32 maxSize_; // max data space size
  Lng32 allocatedSize_;
  Lng32 freeSpaceSize_;
  Lng32 freeSpaceOffset_;

  char * dataPtr_;

  Block * nextBlock_;
  Block * nextSearchBlock_;   //list of searchable blocks

public:

NA_EIDPROC
  void init(Lng32 block_size, Lng32 data_size, char * data_ptr);

  // allocate 'size' amount of space in this block
NA_EIDPROC
  char *allocateMemory(ULng32 size);

NA_EIDPROC
  NABoolean isOverlapping(char * buf, ULng32 size);

NA_EIDPROC
  inline Lng32 getAllocatedSize(){return allocatedSize_;};

NA_EIDPROC
  inline Block *getNext(){return nextBlock_;};

NA_EIDPROC
  inline Lng32 getFreeSpace(){return freeSpaceSize_;};

NA_EIDPROC
  inline Lng32 getBlockSize(){return blockSize_;};

NA_EIDPROC
  inline Block *getNextSearch(){return nextSearchBlock_;};

NA_EIDPROC
  inline void setNext(Block *b){nextBlock_ = b;};

NA_EIDPROC
  inline void setNextSearch(Block* b){nextSearchBlock_ = b;};

NA_EIDPROC
  inline char * getDataPtr(){return dataPtr_;};

NA_EIDPROC
  inline Lng32 getMaxSize(){return maxSize_;};
};


#endif

