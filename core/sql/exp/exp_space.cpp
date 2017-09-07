
// RETIRED: REFERED TO ComSpace.cpp
#if 0
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         <file>
 * Description:  
 *               
 * Created:      7/10/95
 * Language:     C++
 *
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
 *
 *****************************************************************************
 */

#include <limits.h>

#include <stdlib.h>

#include "NAAssert.h"
#include "exp_space.h"
#include "str.h"
#include "dp2.h"


Space::Space(SpaceType type)
: type_(type)
{
  firstBlock_ = 
    lastBlock_ = (Block *)0;

  totalSize_ = 0;
};


Space::~Space()
{
  // free up all the blocks attached to this space pointer.
  Block * currBlock = firstBlock_;
  switch (type_)
    {
    case EXECUTOR_SPACE:
      {
	// later change it to deallocate from executor segment. TBD.
	while (currBlock)
	  {
	    Block * nextBlock = currBlock->getNext();
	    
	    free(currBlock);
	    currBlock = nextBlock;
	  }
      }
      break;
      
    case SYSTEM_SPACE:
      {
	while (currBlock)
	  {
	    Block * nextBlock = currBlock->getNext();
	    
	    free(currBlock);
	    currBlock = nextBlock;
	  }
      }
      break;
    }
}

Lng32 Space::defaultBlockSize(SpaceType type)
{
  Lng32 block_size;
  
  // use default size
  switch (type)
    {
    case EXECUTOR_SPACE:
      block_size = EXECUTOR_BLOCK_MAX_SIZE;
      break;
      
    case SYSTEM_SPACE:
      block_size = EXECUTOR_BLOCK_MAX_SIZE; //SYSTEM_BLOCK_MAX_SIZE;
      break;
    }
  
  return block_size;
}



Block * Space::allocateBlock(SpaceType type, Lng32 in_block_size)
{
  Block * block;
  
  char * block_ptr;
  Lng32 block_size;
  Lng32 data_size;
  char * data_ptr;
  
  Lng32 needed_size = ((((sizeof(Block) + in_block_size) - 1)/8) + 1) * 8;
  if (needed_size > defaultBlockSize(type))
    block_size = needed_size;
  else
    block_size = defaultBlockSize(type);
  
      
  switch (type)
    {
    case EXECUTOR_SPACE:
      {
	// allocate space from executor extended segment. TBD.
	// For now, get it from system space.
	block_ptr = (char *)malloc(block_size);
	data_ptr = block_ptr + ((((sizeof(Block)-1)/8)+1)*8);
	data_size = block_size - ((((sizeof(Block)-1)/8)+1)*8);
      }
      break;
      
    case SYSTEM_SPACE:
      {
	// allocate space from system heap.
	block_ptr = (char *)malloc(block_size);
	data_ptr = block_ptr + ((((sizeof(Block)-1)/8)+1)*8);
	data_size = block_size - ((((sizeof(Block)-1)/8)+1)*8);
	
      }
      break;
    }
  
  block = (Block *)block_ptr;
  
  if(block) block->init(block_size, data_size, data_ptr);
  
  return block;  
}


char *Space::privateAllocateSpace(ULng32 size)
{
  if (size <= 0)
    return 0;

  if (!firstBlock_)
    {
      firstBlock_ = lastBlock_ = allocateBlock(type_, size);
    }

  Block * curr_block = firstBlock_;
  
  // see if space is available in any of the existing blocks.
  char * sp = 0;
  while ((curr_block) && (! (sp = curr_block->allocateMemory(size))))
    {
      curr_block = curr_block->getNext();
    }
  
  if (!sp)
    {
      // space not found in any existing block.
      // Allocate a new one and append it to the last block.
      // The minimum space allocated in a block is max size for 'type'.
      curr_block = allocateBlock(type_, size);
      
      lastBlock_->setNext(curr_block);
      lastBlock_ = curr_block;
      
      sp = curr_block->allocateMemory(size);
    }
  
  totalSize_ += size;
  
  return sp;
}

void * Space::allocateMemory(size_t size)
{
  return allocateAlignedSpace(size);
}


char *Space::allocateAlignedSpace(ULng32 size)
{
  if (size <= 0)
    return 0;
  
  // return aligned space on an 8 byte boundary
  return privateAllocateSpace((((size-1)/8)+1)*8);
}

// If countPrefixSize == 0, make a null-terminated string;
// else,                    make a count-prefixed string (no null terminal).
//
char *Space::allocateAndCopyToAlignedSpace(const char* dp,
					   size_t dlen,
					   size_t countPrefixSize)
{
  size_t alen;
  if (countPrefixSize == 0)
    alen = dlen + 1;
  else
    alen = countPrefixSize + dlen;
  char* rp = allocateAlignedSpace(alen);

  switch (countPrefixSize) {
    case 0:
	break;
    case sizeof(char):
	assert(dlen <= UCHAR_MAX);	// NT_PORT ( bd 10/30/96 ) added missing semicolon
	*(char *)rp = dlen;
	break;
    case sizeof(short):
	assert(dlen <= USHRT_MAX);	// NT_PORT ( bd 10/30/96 ) added missing semicolon
	*(short *)rp = dlen;
	break;
    case sizeof(Int32):
	assert(dlen <= UINT_MAX);	// NT_PORT ( bd 10/30/96 ) added missing semicolon
	*(Int32 *)rp = dlen;
	break;
    default:	
	assert(0==1);
  }
  char* rdp = rp + countPrefixSize;
  str_cpy_all(rdp, dp, dlen);
  if (countPrefixSize == 0) rdp[dlen] = '\0';

  return rp;
} 	// allocateAndCopyToAlignedSpace()


Lng32 Space::convertToOffset(void * ptr)
{
  Block * curr_block = firstBlock_;
  
  // find which block this pointer is allocated from.
  short found = 0;
  Lng32 prev_offset = 0;
  while ((curr_block) && (! found))
    {
      if ((Lng32)((char *)ptr) >= (Lng32)(curr_block->getDataPtr()) &&
	  (Lng32)((char *)ptr) < ((Lng32)(curr_block->getDataPtr()) + 
				 curr_block->getAllocatedSize()))
	return -((Lng32)((char *)ptr) - (Lng32)(curr_block->getDataPtr())
		 + prev_offset);
      
      prev_offset += curr_block->getAllocatedSize();
      
      curr_block = curr_block->getNext();
    }

  // pointer not allocated from any of the existing blocks.
  // Return it as is, it is either an offset already or a
  // garbage pointer. The caller should handle error case.
  return (Lng32)((char *)ptr);
}


Lng32 Space::allocAndCopy(void * from, ULng32 size)
{
  char * to = allocateAlignedSpace(size);
  str_cpy_all(to, (char *)from, size);
  return (convertToOffset(to));
}


#if 0 /* NOT CURRENTLY USED and does not compile for 64 bits */
short Space::isOffset(void * ptr)
{
  if ((Lng32)ptr < 0)
    return -1;
  else
    return 0;
}
#endif

// moves all the Blocks into the output contiguous buffer.

char * Space::makeContiguous(char * out_buf, ULng32 out_buflen)
{
  Block * curr_block = firstBlock_;
  Lng32 curr_offset = 0;
  while (curr_block)
    {
      // NT_PORT (BD 7/11/96) cast to unsigned long
     if (out_buflen - curr_offset < (ULng32)curr_block->getAllocatedSize())
		return 0;
      
      str_cpy_all(&out_buf[curr_offset],
		  curr_block->getDataPtr(),
		  curr_block->getAllocatedSize());

      curr_offset += curr_block->getAllocatedSize();
      curr_block = curr_block->getNext();
    }
  
  return out_buf;
};

/////////////////////////////////////////////
//
// class Block
//
/////////////////////////////////////////////

void Block::init(Lng32 block_size, Lng32 data_size, char * data_ptr)
{  
  dataPtr_ = data_ptr;

  blockSize_ = block_size;
  
  freeSpaceSize_ =
    maxSize_ = data_size;
  
  allocatedSize_ = 0;
  
  freeSpaceOffset_ = 0;
  
  nextBlock_ = (Block *)0;
}
  

char *Block::allocateMemory(ULng32 size)
{
  // NT_PORT (BD 7/11/96) cast to long
  if (freeSpaceSize_ < (Lng32) size)
    return 0;
  
  freeSpaceSize_ -= size;
  allocatedSize_ += size;
  
  char *s = &dataPtr_[freeSpaceOffset_];
  
  freeSpaceOffset_ += size;
  
  return s;
}

#endif

