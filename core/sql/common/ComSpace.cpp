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
 *****************************************************************************
 *
 * File:         ComSpace.cpp
 * Description:
 *
 * Created:      7/10/95
 * Language:     C++
 *
 *
 *****************************************************************************
 */

#include "Platform.h"
#include <iostream>

#include <limits.h>

// #define TRACE_DP2_MALLOCS        // Does not work for NSK DP2!
#ifdef TRACE_DP2_MALLOCS
#include <fstream>
extern ostream *TraceFile;
#endif

#include <stdlib.h>
#include <string.h>

#include "ComASSERT.h"
#include "ComSpace.h"
#include "str.h"
#include "HeapLog.h"

void * operator new(size_t size, Space *s)
{
  if (s)
    return s->allocateSpaceMemory(size);
  else
    {
      void * result = ::operator new(size);
      ComASSERT(result);
      return result;
    }
}


ComSpace::ComSpace(SpaceType type, NABoolean fillUp, char *name)
  : type_(type),
    parent_(NULL),
    firstBlock_(NULL),
    lastBlock_(NULL),
    searchList_(NULL),
    fillUp_(fillUp),
    initialSize_(0)
{
  derivedClass_ = COMSPACE_CLASS;
  if (name != NULL)
    setName(name);
  outputbuf_ = FALSE;
  totalSize_ = 0;
  allocSize_ = 0;
}

ComSpace::~ComSpace()
{
  destroy();
}

void Space::setType(SpaceType type, Lng32 initialSize)
{
  type_ = type;
  initialSize_ = initialSize;
};

void Space::setParent(CollHeap * parent) {
  // make sure that we haven't allocated anything yet. If something
  // is already allocated, we do not set parent_.
  if (allocSize_)
    return;

  parent_ = parent;
}

void ComSpace::destroy() {
   freeBlocks();
}

void Space::freeBlocks(void) {

  HEAPLOG_REINITIALIZE(heapID_.heapNum)
  // free up all the blocks attached to this space pointer.
  Block * currBlock = firstBlock_;
  switch (type_) {
  case SINGLE_BLOCK_SPACE:
    break;

  case SYSTEM_SPACE:
  case EXECUTOR_SPACE:
  case GENERATOR_SPACE: {
    // later change it to deallocate from executor segment. TBD.
    while (currBlock) {
      Block * nextBlock = currBlock->getNext();

      if (parent_)
	{
	  HEAPLOG_OFF()  // no recursive logging. (eric)
	  parent_->deallocateMemory((void *) currBlock);
	  HEAPLOG_ON()
	}
      else
	{
	  free(currBlock);
	}
      // printf("F\tComSpace\t%8x\t%8x\n", currBlock, this);

      currBlock = nextBlock;
    }
  }
  break;
  }

  firstBlock_ = NULL;
  lastBlock_ = NULL;
  searchList_ = NULL;
  allocSize_ = 0;
  totalSize_ = 0;
}

Lng32 Space::defaultBlockSize(SpaceType type)
{
  Lng32 block_size = 0;

  // use default size
  switch (type)
    {
    case EXECUTOR_SPACE:
    case GENERATOR_SPACE:
    case SYSTEM_SPACE:
      block_size = 32768;
      break;
    case SINGLE_BLOCK_SPACE:
      block_size = -1;
      break;
    }

  return block_size;
}

// Round up input value to nearest multiple of 8.
//
static Lng32 roundUp8(Lng32 val)
{
  ULng32 uval = (ULng32) val;

  // clear the last 3 bits, which effectively rounds
  // down to the nearest multiple of 8
  ULng32 roundedDown = uval LAND 0xFFFFFFF8;

  // if that didn't change anything we're done
  if (uval != roundedDown)
    {
      // else we have to round up and add the filler
      val = (Lng32) (roundedDown + 8);
    }

  return val;
}

Block * Space::allocateBlock(SpaceType type,
			     Lng32 in_block_size,
			     NABoolean firstAllocation,
			     char * in_block_addr,
			     NABoolean failureIsFatal)
{
  Block * block;
  char * block_ptr = 0;
  Lng32 block_size;
  Lng32 data_size;
  char * data_ptr;

  // Don't satisfy request for more blocks if this is a SINGLE_BLOCK_SPACE.
  if (type==SINGLE_BLOCK_SPACE)
    return NULL;

  // size of rounded-up block header
  const Lng32 BlockHdrAllocSz = roundUp8(sizeof(Block));

  // assert that in_block_size is a multiple of 8
  ComASSERT(in_block_size==roundUp8(in_block_size));

  Lng32 defBlkSz;

  if (firstAllocation && (initialSize_ > 0))
    defBlkSz = initialSize_;
  else
    defBlkSz = defaultBlockSize(type);
  Lng32 needed_size = in_block_size + BlockHdrAllocSz;

  // Set block_size, size of block to be allocated, to:
  //
  //    max(needed_size, defBlkSz)
  //
  if (needed_size > defBlkSz)
    block_size = needed_size;
  else
    block_size = defBlkSz;

  // ---------------------------------------------------------------------
  // Special case: if this is the first allocation for this Space, and the
  // needed space is tiny (< 32 bytes), and this is a SYSTEM_SPACE, then
  // we only allocate a tiny block (64 bytes).  This is part of an effort
  // to reduce the total size of tdm_arkcmp.exe.  Before this change,
  // there were 200+ Space objects contained in arkcmp (they came from the
  // compiled system modules), each of which was 32k in size, but each
  // using only 24 bytes!  In other words, arkcmp was using over 6.4MB of
  // virtual memory which was completely unused!  This strange special
  // case logic seems a small price to pay in order to reduce arkcmp's
  // size by so much.
  if ( firstAllocation == TRUE &&
       type == EXECUTOR_SPACE  &&  // see /cli/Statement.cpp, Statement ctor
       in_block_addr == NULL &&
       in_block_size < 32      &&
       needed_size   < 64 ) // this last one's a safety check
    block_size = 64 ;
  // ---------------------------------------------------------------------

  if (in_block_addr != NULL) {
    block_ptr = in_block_addr;
  }
  else {
    switch (type) {
    case EXECUTOR_SPACE:
    case GENERATOR_SPACE:
    case SYSTEM_SPACE: {
      // allocate space from executor extended segment. TBD.
      // For now, get it from system space.
      if (parent_)
	{
	  HEAPLOG_OFF()  // no recursive logging. (eric)
	    block_ptr = (char *) parent_->allocateMemory((size_t) block_size, failureIsFatal);
	  HEAPLOG_ON()
	    }
      else
	{
	  block_ptr = (char *)malloc((UInt32)block_size);
	}
      // printf("M\tComSpace\t%8x\t%8x\t%8x\t%8x\n", block_ptr, block_size,
      //       block_ptr + block_size, this);
    }
    break;
    }
  }

  data_ptr = block_ptr + BlockHdrAllocSz;
  data_size = block_size - BlockHdrAllocSz;

  if (block_ptr == NULL)
    return NULL;

  block = (Block *)block_ptr;

  block->init(block_size, data_size, data_ptr);
  
  totalSize_ += block_size;
  return block;
}

void Space::setFirstBlock(char * blockAddr, Lng32 blockLen, NABoolean failureIsFatal)
{
  if (firstBlock_)
    return;

  initialSize_ = blockLen;

  allocateBlock(type_, 0, TRUE, blockAddr, failureIsFatal);
}

char *Space::privateAllocateSpace(ULng32 size, NABoolean failureIsFatal) {
  if (size <= 0)
    return NULL;

  if (!firstBlock_)
    {
      // for the first allocation from a Space, for memory-usage issues,
      // we do something special in firstAllocation
      const NABoolean firstAllocation = TRUE ;
      firstBlock_ = lastBlock_ = searchList_ =
        allocateBlock(type_, size, firstAllocation, NULL, failureIsFatal);
    }

  if (firstBlock_ == NULL)
    return NULL;

  char * sp = 0;

  Block * currBlock = searchList_;
  Block * prevBlock = 0;
  // try to allocate in the current block. If this is not possible,
  // go thru the list of searchable blocks and try to find a block
  // with enough free space. Do this only if fillUp_ is true. Otherwise
  // we just allocate another block.
  while ((currBlock) &&
	 (!(sp = currBlock->allocateMemory(size))) &&
	 fillUp_) {
    // if the current block does not have any free space, remove it
    // from the list of searchable blocks.
    if (!currBlock->getFreeSpace()) {
      if (!prevBlock)
	// this is the first block in the list, just skip it
	searchList_ = searchList_->getNextSearch();
      else
	prevBlock->setNextSearch(currBlock->getNextSearch());
    }
    else
      // currBlock is not empty. Set prevBlock
      prevBlock = currBlock;

    // we could not allocate in currBlock. Go on with the next block
    // in the searchList.
    currBlock = currBlock->getNextSearch();
    if ((outputbuf_) && (currBlock))
	  currBlock = 0;
  }

  if (!sp) {
    // space not found in any existing block.
    // Allocate a new one and append it to the last block.
    // The minimum space allocated in a block is max size for 'type'.
      currBlock = allocateBlock(type_, size, FALSE, NULL, failureIsFatal);

      // return if we couldn't allocate a block
      if (currBlock == NULL)
	return NULL;

      // add the new block to the list of all blocks
      lastBlock_->setNext(currBlock);
      lastBlock_ = currBlock;

      // add the new block to the list of searchable blocks
      // add it to the beginning of this list
      currBlock->setNextSearch(searchList_);
      searchList_ = currBlock;

      sp = currBlock->allocateMemory(size);
      if (!sp)
	return NULL;
  }
  allocSize_ += size;

  if (type_ == GENERATOR_SPACE)
    {
      // initialize the memory to 0, this sets filler bytes to a
      // defined value and avoids junk data to be included in
      // module files and resource forks
      //str_pad(sp, size, 0);
      memset(sp, 0, size);
    }

  return sp;
}

void * Space::allocateSpaceMemory(size_t size, NABoolean failureIsFatal) {
      void * rc = allocateAlignedSpace(size, failureIsFatal);
      HEAPLOG_ADD_ENTRY(rc, size, heapID_.heapNum, getName())
      if (rc) return rc;
      if (failureIsFatal && size > 0)
        {
          // Might never return...
          handleExhaustedMemory();
          // If we return from this call it means that the caller wanted
          // a memory allocation failure to be fatal yet did not set the
          // the jump buffer.  This is not good.
          assert(0);
          return NULL;
        }
      return rc;
}


char *Space::allocateAlignedSpace(ULng32 size, NABoolean failureIsFatal) {
  if (size <= 0)
    return 0;

  // return aligned space on an 8 byte boundary
  return privateAllocateSpace((ULng32) roundUp8((Lng32)size), failureIsFatal);
}

// If countPrefixSize == 0, make a null-terminated string;
// else,                    make a count-prefixed string (no null terminal).
//
char *Space::allocateAndCopyToAlignedSpace(const char *dp,
					   size_t dlen,
					   size_t countPrefixSize,
					   NABoolean failureIsFatal,
					   NABoolean noSizeAdjustment)
{
  size_t alen;
  if (noSizeAdjustment)
    alen = dlen;
  else
    {
      if (countPrefixSize == 0)
	alen = dlen + 1;
      else
	alen = countPrefixSize + dlen;
    }

  char* rp = allocateAlignedSpace(alen, failureIsFatal);

  switch (countPrefixSize) {
    case 0:
	break;
    case sizeof(char):
	ComASSERT(dlen <= UCHAR_MAX)
	*(char *)rp = dlen;
	break;
    case sizeof(short):
	ComASSERT(dlen <= USHRT_MAX)
	*(short *)rp = dlen;
	break;
    case sizeof(Int32):
	ComASSERT(dlen <= UINT_MAX)
	*(Int32 *)rp = dlen;
	break;
    default:
	ComASSERT(0==1);
  }
  char* rdp = rp + countPrefixSize;
  str_cpy_all(rdp, dp, dlen);
  if ((countPrefixSize == 0) && (NOT noSizeAdjustment))
    rdp[dlen] = '\0';

  return rp;
}	// allocateAndCopyToAlignedSpace()

void Space::outputBuffer(ComSpace * space, char * buf, char * newbuf)
{
  if ((strlen(buf) + strlen(newbuf)) > 75)
  {
    space->allocateAndCopyToAlignedSpace(buf, strlen(buf), sizeof(short));
    strcpy(buf, newbuf);
  }
  else
  {
    strcat(buf, newbuf);
  }
}

// LCOV_EXCL_START
void Space::display(char *buf,
		    size_t buflen,
		    size_t countPrefixSize, ostream &outstream)
{
  size_t dlen = 0;
  char *bend = buf + buflen;
  while (buf < bend) {
    switch (countPrefixSize) {
      case sizeof(char):
	  dlen = *(unsigned char *)buf;
	  break;
      case sizeof(short):
	  dlen = *(unsigned short *)buf;
	  break;
      case sizeof(Lng32):
	  dlen = *(ULng32 *)buf;
	  break;
      default:
	  ComASSERT(0==1);
    }
    buf += countPrefixSize;
    if (dlen) {
      char sav = buf[--dlen];
      buf[dlen] = '\0';
      outstream << buf << sav << '\n';
      buf[dlen] = sav;
      buf += ++dlen;
    }
    else
      outstream << '\n';
  }
  outstream << flush;
  ComASSERT(buf == bend);
}
// LCOV_EXCL_STOP



// NOTE: Sometimes Space::convertToOffset() returns an offset.
//       Other times, it returns an actual address.  
//       Consequently, we declare the return value as "long"
//       so it can hold either one.
Long Space::convertToOffset(void * ptr)
{
  Block * curr_block = firstBlock_;

  // find which block this pointer is allocated from.
  short found = 0;
  Lng32 prev_offset = 0;
  while ((curr_block) && (! found))
    {
      if (((char *)ptr) >= (char *)(curr_block->getDataPtr()) &&
	  ((char *)ptr) < ((char *)(curr_block->getDataPtr()) +
				 curr_block->getAllocatedSize()))
	return -(Long)((char *)ptr - (char *)(curr_block->getDataPtr())
		 + prev_offset);

      prev_offset += curr_block->getAllocatedSize();

      curr_block = curr_block->getNext();
    }

  // pointer not allocated from any of the existing blocks.
  // Return it as is, it is either an offset already or a
  // garbage pointer. The caller should handle error case.

  return (Long)(ptr);
}


void* Space::convertToPtr(Long offset) const
{
  Block * curr_block = firstBlock_;

  // find which block this offset falls into
  Long remainder = offset;
  while (curr_block && remainder <= 0) {
    if (curr_block->getAllocatedSize() + remainder > 0) {
      return curr_block->getDataPtr() - remainder;
    }
    remainder += curr_block->getAllocatedSize();
    curr_block = curr_block->getNext();
  }

  // offset does not fall into any of the existing blocks.
  // Return a NULL. Let caller check for & handle error case.

  return NULL;
}

// LCOV_EXCL_START
// The method is not called elsewhere
Lng32 Space::allocAndCopy(void * from, ULng32 size, NABoolean failureIsFatal)
{
  char * to = allocateAlignedSpace(size, failureIsFatal);
  str_cpy_all(to, (char *)from, size);
  return (convertToOffset(to));
}
// LCOV_EXCL_STOP


#if 0 /* NOT CURRENTLY USED and does not compile for 64 bits */
short Space::isOffset(void * ptr)
{
  if ((Lng32)ptr < 0)
    return -1;
  else
    return 0;
}
#endif

NABoolean Space::isOverlappingMyBlocks(char *buf, ULng32 size)
{
  Block *curr_block =  firstBlock_;
  while (curr_block)
    {
      if (curr_block->isOverlapping(buf, size))
        return TRUE;
      curr_block = curr_block->getNext();
    }
  return FALSE;
}


// moves all the Blocks into the output contiguous buffer.

char * Space::makeContiguous(char * out_buf, ULng32 out_buflen)
{
  Block * curr_block = firstBlock_;
  Lng32 curr_offset = 0;
  while (curr_block)
    {
      if ((Lng32) out_buflen - curr_offset < curr_block->getAllocatedSize())
	return 0;

      str_cpy_all(&out_buf[curr_offset],
		  curr_block->getDataPtr(),
		  curr_block->getAllocatedSize());

      curr_offset += curr_block->getAllocatedSize();
      curr_block = curr_block->getNext();
    }

  return out_buf;
}

#if (defined(_DEBUG) || defined(NSK_MEMDEBUG))

void Space::dumpSpaceInfo(ostream* outstream, Lng32 indent) {
  char ind[100];
  Lng32 indIdx = 0;
  for (; indIdx < indent; indIdx++)
    ind[indIdx] = ' ';
  ind[indIdx] = '\0';
  if (!outstream)
    outstream = &cerr;
  *outstream << ind << "Dump of Space: " << this << " (";
  switch (type_) {
  case EXECUTOR_SPACE:
    *outstream << "EXECUTOR_SPACE";
      break;
  case GENERATOR_SPACE:
    *outstream << "GENERATOR_SPACE";
      break;
  case SYSTEM_SPACE:
    *outstream << "SYSTEM_SPACE";
    break;
  }
  *outstream << "):" << endl
	     << ind << "Parent:             " << parent_ << endl
             << ind << "Total Size(Bytes)   " << totalSize_ << endl
	     << ind << "Total Allocated Size (Bytes): " << allocSize_ << endl;

}
#endif

/////////////////////////////////////////////
//
// class Block
//
/////////////////////////////////////////////

void Block::init(Lng32 block_size, Lng32 data_size, char * data_ptr)
{
  dataPtr_ = data_ptr;
  blockSize_ = block_size;
  freeSpaceSize_ = maxSize_ = data_size;
  allocatedSize_ = 0;
  freeSpaceOffset_ = 0;
  nextBlock_ = NULL;
  nextSearchBlock_ = NULL;
}


char *Block::allocateMemory(ULng32 size)
{
  if (freeSpaceSize_ < (Lng32) size)
    return 0;

  freeSpaceSize_ -= size;
  allocatedSize_ += size;

  char *s = &dataPtr_[freeSpaceOffset_];

  freeSpaceOffset_ += size;

  return s;
}

NABoolean Block::isOverlapping(char * buf, ULng32 size)
{
  if ((buf >= dataPtr_) &&
      (buf < (dataPtr_ + blockSize_)))
    // buf starts within block.
    return TRUE;
  if ((buf < dataPtr_) &&
      ((buf + size) > dataPtr_))
    // buf starts before block, but is too long and thus overlaps block.
    return TRUE;
  return FALSE;
}
