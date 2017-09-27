//*********************************************************************
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
//*********************************************************************

//*********************************************************************
//
// File:         ClusteredBitmap.cpp
// Description:  This implements the ClusteredBitmap class that
//               allows for a sparse bitmap with values clustered
//               around smaller ranges of numbers.  Its primary purpose
//               is to save memory when storing large ValueIds.
//
// Created:      1/20/2008
// Language:     C++
//
//*********************************************************************

#include <stdlib.h>
#include <string.h>
#include "Platform.h"
#include "BaseTypes.h"
#include "NAMemory.h"
#include "ClusteredBitmap.h"


#ifdef WIN32
// The firstSetBit() function produces a warning on Windows.
// Turn it off temporarily.
#pragma warning( disable : 4146 ) // disable "unsigned" warnings
#endif // WIN32

// Isolate the first set bit in the integer value.
inline ClusteredBitmap::cb_int_t 
ClusteredBitmap::firstSetBit(cb_int_t value)
{
  return value & (-value);
}

#ifdef WIN32
#pragma warning( default : 4146 ) // allow "unsigned" warnings
#endif // WIN32

// Return the significant bits for a particular value.  The
// significant bits are bits 9-31 and are used as a lookup
// value in the bitmapMap_ array.
inline UInt32
ClusteredBitmap::significantBits(UInt32 value)
{
  return value & SIGNIFICANT_BITS_MASK;
}

// Determine which word in the bitmap stores this value.
inline UInt32
ClusteredBitmap::word(UInt32 value)
{
  return (value >> WORD_BITS_SHIFT) & WORD_BITS_MASK;
}

// Return the shifted bit value for the passed in value.  The
// returned bit value may be used to perform operations that
// set or clear the bit.
inline ClusteredBitmap::cb_int_t
ClusteredBitmap::shiftedBit(UInt32 value)
{
  return (cb_int_t)1 << (value & BITS_MASK);
}

// Count the number of set bits in an integer
inline UInt32
ClusteredBitmap::popCnt(cb_int_t value)
{
  // This is a well known algorithm for counting the number of set
  // bits in a 32-bit integer.  There are no branches, so this is
  // pretty quick.
  value -= ((value >> 1) & 0x55555555);
  value = (((value >> 2) & 0x33333333) + (value & 0x33333333));
  value = (((value >> 4) + value) & 0x0f0f0f0f);
  value += (value >> 8);
  value += (value >> 16);

  return (value & 0x0000003f);
}

// Set the 64-byte bitmap array to zero.
inline void
ClusteredBitmap::initializeBitmap(cb_int_t *bits)
{
  // Set bits to zero without branches (for slight performance improvement)

  *bits++ = 0; *bits++ = 0; *bits++ = 0; *bits++ = 0;
  *bits++ = 0; *bits++ = 0; *bits++ = 0; *bits = 0;
#if (CB_BITS_PER_WORD == 32)
  *++bits = 0; *++bits = 0; *++bits = 0; *++bits = 0;
  *++bits = 0; *++bits = 0; *++bits = 0; *++bits = 0;
#endif
}

// Count the number of set bits in a 64-byte bitmap.
//  NOTE:  On Yosemite, this is done 64-bits at a time, while on other
//         platforms, it is done 32-bits at a time.
inline Int32 
ClusteredBitmap::countBits(cb_int_t *bits)
{
  Int32 numBits = 0;

  numBits += popCnt(*bits++); numBits += popCnt(*bits++);
  numBits += popCnt(*bits++); numBits += popCnt(*bits++);
  numBits += popCnt(*bits++); numBits += popCnt(*bits++);
  numBits += popCnt(*bits++); numBits += popCnt(*bits);
#if (CB_BITS_PER_WORD == 32)
  numBits += popCnt(*++bits); numBits += popCnt(*++bits);
  numBits += popCnt(*++bits); numBits += popCnt(*++bits);
  numBits += popCnt(*++bits); numBits += popCnt(*++bits);
  numBits += popCnt(*++bits); numBits += popCnt(*++bits);
#endif
  return numBits;
}

// Copy a 64-byte bitmap to another 64-byte bitmap.
inline void
ClusteredBitmap::copyBits(cb_int_t *tobits, cb_int_t *frombits)
{
  *tobits++ = *frombits++; *tobits++ = *frombits++;
  *tobits++ = *frombits++; *tobits++ = *frombits++;
  *tobits++ = *frombits++; *tobits++ = *frombits++;
  *tobits++ = *frombits++; *tobits   = *frombits;
#if (CB_BITS_PER_WORD == 32)
  *++tobits = *++frombits; *++tobits = *++frombits;
  *++tobits = *++frombits; *++tobits = *++frombits;
  *++tobits = *++frombits; *++tobits = *++frombits;
  *++tobits = *++frombits; *++tobits = *++frombits;
#endif
}

// Copy each of the words in the bitmap and return the number of
// bits that were set in the copy.
inline Int32 
ClusteredBitmap::copyCountBits(cb_int_t *tobits, cb_int_t *frombits)
{
  Int32 numBits = 0;

  // Copy to the other bitmap and call popCnt() to see how many bits
  // are set in the copied word.
  for (UInt32 wordIdx = WORDS_PER_BITMAP; wordIdx != 0; wordIdx--)
  {
    cb_int_t fromWord = *frombits++;
    *tobits++ = fromWord;
    numBits += popCnt(fromWord);
  }

  return numBits;
}

// Logical OR two bit arrays storing the results in the first array.
// The number of bits in the result array is returned.
inline Int32
ClusteredBitmap::orBits(cb_int_t *tobits, cb_int_t *frombits)
{
  Int32 setBits = 0;

  // Perform logical-OR on each bit set (when the bits are not equal)
  // and store the result in the tobits array.
  for (UInt32 wordIdx = WORDS_PER_BITMAP; wordIdx != 0; wordIdx--)
  {
    cb_int_t result = *frombits++ | *tobits;
    *tobits++ = result;
    setBits += popCnt(result);
  }

  return setBits;
}

// Logically-AND two bit arrays storing the results in the first array
// The number of set bits in the result array is returned.
inline Int32
ClusteredBitmap::andBits(cb_int_t *tobits, cb_int_t *otherbits)
{
  Int32 numSetBits = 0;

  for (UInt32 wordIdx = WORDS_PER_BITMAP; wordIdx != 0; wordIdx--)
  {
    cb_int_t andVal = *tobits & *otherbits++;
    *tobits++ = andVal;
    numSetBits += popCnt(andVal);
  }
  
  return numSetBits;
}

// Clear bits in the first array that are set in the second array.
inline Int32
ClusteredBitmap::clearBits(cb_int_t *tobits, cb_int_t *removebits)
{
  Int32 numSetBits = 0;

  for (UInt32 wordIdx = WORDS_PER_BITMAP; wordIdx != 0; wordIdx--)
  {
    cb_int_t result = *tobits & ~(*removebits++);
    *tobits++ = result;
    numSetBits += popCnt(result);
  }

  return numSetBits;
}

// Increase the size of the bitmap array to hold at least the size
// needed.  The maximum number of elements in the array will be
// increased to the next multiple of 8 boundary.
inline void
ClusteredBitmap::increaseBitmapMapSize(UInt32 numNeeded)
{
  if (numNeeded > maxBitmaps_)
  {
    // Increase size to next multiple of 8 boundary.
    maxBitmaps_ = (numNeeded & ~0x7) + 8;

    // Create a new bitsToBitmap to array and copy old values to new
    // array.
    bitsToBitmap *newMap = new (heap_) bitsToBitmap[maxBitmaps_];

    for (UInt32 thisIdx = 0; thisIdx < numBitmaps_; thisIdx++)
    {
      newMap[thisIdx].significantBits_ = bitmapMap_[thisIdx].significantBits_;
      newMap[thisIdx].bitmap_ = bitmapMap_[thisIdx].bitmap_;
    }

    // Delete old array and set current bitmapMap to the new one
    if (bitmapMap_)
      NADELETEBASIC(bitmapMap_, heap_);

    bitmapMap_ = newMap;
  }
}

// Allocate and insert a new bitmap based on the significant bits.
// The caller should ensure that this function is not called when
// an entry already exists that matches the significant bits.
ClusteredBitmap::cb_int_t*
ClusteredBitmap::insertMap(UInt32 significantBits)
{
  UInt32 thisIdx;

  // Allocate a new bitmap and initialize it to zero.
  cb_int_t *bitmap = new (heap_) cb_int_t[WORDS_PER_BITMAP];
  initializeBitmap(bitmap);

  // Make sure there are enough bitmap entries
  if (numBitmaps_ == maxBitmaps_)
  {
    // Increase size to next multiple of 8 boundary.
    maxBitmaps_ = (maxBitmaps_ & ~0x7) + 8;

    // Create a new bitsToBitmap array
    bitsToBitmap *newMap = new (heap_) bitsToBitmap[maxBitmaps_];

    // Copy to the new bitmap map while looking for the proper
    // insertion point.
    for (thisIdx = 0; thisIdx < numBitmaps_; thisIdx++)
    {
      if (significantBits < bitmapMap_[thisIdx].significantBits_)
        break;
      newMap[thisIdx].bitmap_ = bitmapMap_[thisIdx].bitmap_;
      newMap[thisIdx].significantBits_ =
           bitmapMap_[thisIdx].significantBits_;
    }

    // Insert the new bitmap into the array of bitmap maps.
    newMap[thisIdx].bitmap_ = bitmap;
    newMap[thisIdx].significantBits_ = significantBits;
    numBitmaps_++;
 
    // Copy the rest of the bitmaps to the bitmap array;
    for (thisIdx++; thisIdx < numBitmaps_; thisIdx++)
    {
      newMap[thisIdx].bitmap_ = bitmapMap_[thisIdx - 1].bitmap_;
      newMap[thisIdx].significantBits_ =
           bitmapMap_[thisIdx - 1].significantBits_;
    }
    // Free the old bitmap map and set the pointer to the new one
    if (bitmapMap_)
      NADELETEBASIC(bitmapMap_, heap_);
    bitmapMap_ = newMap;
  }
  else
  {
    // Start from the last bitmap map and move entries forward
    // until the proper insertion point is found.
    for (thisIdx = numBitmaps_; thisIdx > 0; thisIdx--)
    {
      if (significantBits > bitmapMap_[thisIdx - 1].significantBits_)
        break;

      bitmapMap_[thisIdx].bitmap_ = bitmapMap_[thisIdx - 1].bitmap_;
      bitmapMap_[thisIdx].significantBits_ = 
            bitmapMap_[thisIdx - 1].significantBits_;
    }

    // Insert the new bitmap map;
    bitmapMap_[thisIdx].bitmap_ = bitmap;
    bitmapMap_[thisIdx].significantBits_ = significantBits;
    numBitmaps_++;
  }

  return bitmap;
}

// ClusteredBitmap constructor
ClusteredBitmap::ClusteredBitmap(CollHeap *heap) :
   heap_(heap),
   bitmapMap_(0),
   numBitmaps_(0),
   maxBitmaps_(0),
   entries_(0)
{
}

// ClusteredBitmap constructor
ClusteredBitmap::ClusteredBitmap(const ClusteredBitmap &other,
                                 CollHeap *heap)
 : heap_(heap),
   bitmapMap_(0),
   numBitmaps_(0),  // Reset to new value later
   maxBitmaps_(0),
   entries_(other.entries_)
{
  if (other.numBitmaps_)
  {
    increaseBitmapMapSize(other.numBitmaps_);
    numBitmaps_ = other.numBitmaps_;

    for (UInt32 thisIdx = 0; thisIdx < numBitmaps_; thisIdx++)
    {
      bitmapMap_[thisIdx].significantBits_ = 
         other.bitmapMap_[thisIdx].significantBits_;
      bitmapMap_[thisIdx].bitmap_ = new (heap_) cb_int_t[WORDS_PER_BITMAP];
      copyBits(bitmapMap_[thisIdx].bitmap_, other.bitmapMap_[thisIdx].bitmap_);
    }
  }
}

// ClusteredBitmap constructor
ClusteredBitmap::ClusteredBitmap(const ClusteredBitmap &other)
 : heap_(other.heap_),
   bitmapMap_(0),
   numBitmaps_(0),  // Reset to new value later
   maxBitmaps_(0),
   entries_(other.entries_)
{
  increaseBitmapMapSize(other.numBitmaps_);
  numBitmaps_ = other.numBitmaps_;

  for (UInt32 thisIdx = 0; thisIdx < numBitmaps_; thisIdx++)
  {
    bitmapMap_[thisIdx].significantBits_ =
        other.bitmapMap_[thisIdx].significantBits_;
    bitmapMap_[thisIdx].bitmap_ = new (heap_) cb_int_t[WORDS_PER_BITMAP];
    copyBits(bitmapMap_[thisIdx].bitmap_, other.bitmapMap_[thisIdx].bitmap_);
  }
}

// ClusteredBitmap destructor
ClusteredBitmap::~ClusteredBitmap()
{
  if (bitmapMap_)
  {
    for (UInt32 i = 0; i < numBitmaps_; i++)
      NADELETEBASIC(bitmapMap_[i].bitmap_, heap_);

    NADELETEBASIC(bitmapMap_, heap_);
  }
}

// Clear this ClusteredBitmap object.
void ClusteredBitmap::clear()
{
  if (bitmapMap_)
  {
    for (UInt32 thisIdx = 0; thisIdx < numBitmaps_; thisIdx++)
      NADELETEBASIC(bitmapMap_[thisIdx].bitmap_, heap_);

    NADELETEBASIC(bitmapMap_, heap_);
    bitmapMap_ = NULL;
  }

  numBitmaps_ = 0;
  maxBitmaps_ = 0;
  entries_ = 0;
}

// This returns either the bitmap map that matches the significant bits,
// or returns the first bitmap map that is greater than the significant
// bits.  This uses a binary search to find the matching bitmap.
inline ClusteredBitmap::bitsToBitmap*
ClusteredBitmap::findEqualOrGreaterBitmapMap(UInt32 sigBits) const
{
  if (numBitmaps_ == 0)
    return NULL;

  Int32 low = 0;
  Int32 high = numBitmaps_ - 1;
  Int32 middle;
  bitsToBitmap *map;

  while (low <= high)
  {
    middle = (low + high) >> 1;
    map = &bitmapMap_[middle];

    if (map->significantBits_ > sigBits)
      high = middle - 1;
    else if (map->significantBits_ < sigBits)
      low = middle + 1;
    else
      return map;  // Found an equal match
  }

  if (map->significantBits_ < sigBits)
    map++;

  if (map == &bitmapMap_[numBitmaps_])
    return NULL;

  // This is the map with the next greater significant bits
  return map;
}

// findBitmap() uses a binary search to find a matching bitmap.
inline ClusteredBitmap::cb_int_t*
ClusteredBitmap::findBitmap(UInt32 significantBits) const
{
  Int32 low = 0;
  Int32 high = (Int32)numBitmaps_ - 1;

  while (low <= high)
  {
    Int32 middle = (low + high) >> 1;
    bitsToBitmap *map = &bitmapMap_[middle];

    if (map->significantBits_ > significantBits)
      high = middle - 1;
    else if (map->significantBits_ < significantBits)
      low = middle + 1;
    else
      return map->bitmap_;
  }

  return NULL;
}

// Add a value to this bitmap.
ClusteredBitmap &
ClusteredBitmap::addElement(UInt32 value)
{
  UInt32 sigBits = significantBits(value);
  cb_int_t *bitmap = findBitmap(sigBits);
  if (bitmap == NULL)
    bitmap = insertMap(sigBits);

  cb_int_t *wordPtr = &bitmap[word(value)];
  cb_int_t bit = shiftedBit(value);

  if ((*wordPtr & bit) == 0)
  {
    *wordPtr |= bit;
    entries_++;
  }
  return *this;
}

// Remove a value from this bitmap.
ClusteredBitmap &
ClusteredBitmap::subtractElement(UInt32 value)
{
  UInt32 sigBits = significantBits(value);
  cb_int_t *bitmap = findBitmap(sigBits);
  if (bitmap == NULL)
    return *this;

  cb_int_t *wordPtr = &bitmap[word(value)];
  cb_int_t bit = shiftedBit(value);

  if ((*wordPtr & bit) != 0)
  {
    *wordPtr &= ~bit;
    entries_--;
  }
  return *this;
}

// Add the elements from another bitmap to this one. 
// This function first determines how many bitmaps are needed and allocates
// a new buffer for the map of bitmaps.  Then as the two bitmaps are combined,
// the new buffer is filled in with the new bitmaps.
ClusteredBitmap &
ClusteredBitmap::addSet(const ClusteredBitmap &other)
{
  UInt32 thisIdx = 0;
  UInt32 otherIdx = 0;
  UInt32 neededBitmaps = 0;
  UInt32 numNewBits = 0;

  // First determine how many bitmaps are needed so we only have
  // to allocate memory once for the final bitmap maps.
  while (thisIdx < numBitmaps_ && otherIdx < other.numBitmaps_)
  {
    neededBitmaps++;
    if (bitmapMap_[thisIdx].significantBits_ ==
        other.bitmapMap_[otherIdx].significantBits_)
    {
      thisIdx++;
      otherIdx++;
    }
    else if (bitmapMap_[thisIdx].significantBits_ <
             other.bitmapMap_[otherIdx].significantBits_)
    {
      thisIdx++;
    }
    else
    {
      otherIdx++;
    }
  }

  // Add additional bitmaps that weren't taken into account in previous
  // loop.
  neededBitmaps += numBitmaps_ - thisIdx + other.numBitmaps_ - otherIdx;

  // Increase to next multiple of 8.
  neededBitmaps = (neededBitmaps & ~0x7) + 8;
  maxBitmaps_ = neededBitmaps;

  // Allocate a new buffer to hold the bitmap map.
  bitsToBitmap *newMap = new (heap_) bitsToBitmap[neededBitmaps];

  thisIdx = 0;
  otherIdx = 0;
  UInt32 newMapIdx = 0;

  // Create the new bitmap array that logically OR all of the bitmaps.
  while (thisIdx < numBitmaps_ && otherIdx < other.numBitmaps_)
  {
    if (bitmapMap_[thisIdx].significantBits_ ==
        other.bitmapMap_[otherIdx].significantBits_)
    {
      numNewBits += orBits(bitmapMap_[thisIdx].bitmap_,
                           other.bitmapMap_[otherIdx].bitmap_);
      newMap[newMapIdx].significantBits_ = 
           bitmapMap_[thisIdx].significantBits_;
      newMap[newMapIdx].bitmap_ = bitmapMap_[thisIdx].bitmap_;
      thisIdx++;
      otherIdx++;
    }
    else if (bitmapMap_[thisIdx].significantBits_ <
             other.bitmapMap_[otherIdx].significantBits_)
    {
      newMap[newMapIdx].significantBits_ = 
           bitmapMap_[thisIdx].significantBits_;
      newMap[newMapIdx].bitmap_ = bitmapMap_[thisIdx].bitmap_;
      numNewBits += countBits(bitmapMap_[thisIdx].bitmap_);
      thisIdx++;
    }
    else
    {
      newMap[newMapIdx].significantBits_ =
           other.bitmapMap_[otherIdx].significantBits_;
      newMap[newMapIdx].bitmap_ = new (heap_) cb_int_t[WORDS_PER_BITMAP];
      numNewBits += copyCountBits(newMap[newMapIdx].bitmap_,
                                  other.bitmapMap_[otherIdx].bitmap_);
      otherIdx++;
    }
    newMapIdx++;
  }

  // Set any remaining bitmaps in this object in the new buffer.
  while (thisIdx < numBitmaps_)
  {
    newMap[newMapIdx].significantBits_ =
         bitmapMap_[thisIdx].significantBits_;
    newMap[newMapIdx].bitmap_ = bitmapMap_[thisIdx].bitmap_;
    numNewBits += countBits(bitmapMap_[thisIdx].bitmap_);
    thisIdx++;
    newMapIdx++;
  }

  // Copy any remaining bitmaps in the other object and store pointers
  // to the bitmaps in the new bitmap map.
  while (otherIdx < other.numBitmaps_)
  {
    newMap[newMapIdx].significantBits_ =
         other.bitmapMap_[otherIdx].significantBits_;
    newMap[newMapIdx].bitmap_ = new (heap_) cb_int_t[WORDS_PER_BITMAP];
    numNewBits += copyCountBits(newMap[newMapIdx].bitmap_,
                                other.bitmapMap_[otherIdx].bitmap_);
    otherIdx++;
    newMapIdx++;
  }

  // Destroy the old bitmap map array and assign the new one.
  if (bitmapMap_)
    NADELETEBASIC(bitmapMap_, heap_);
  bitmapMap_ = newMap;
  numBitmaps_ = newMapIdx;
  entries_ = numNewBits;
  return *this;
}

// Remove the bits that are in the other bitmap from this one.
ClusteredBitmap &
ClusteredBitmap::subtractSet(const ClusteredBitmap &other)
{
  UInt32 thisIdx = 0;
  UInt32 otherIdx = 0;
  UInt32 newMapIdx = 0;
  Int32 numSetBits = 0;

  // Examine each bitmap array 
  while (thisIdx < numBitmaps_ && otherIdx < other.numBitmaps_)
  {
    if (bitmapMap_[thisIdx].significantBits_ ==
        other.bitmapMap_[otherIdx].significantBits_)
    {
      Int32 numBits = clearBits(bitmapMap_[thisIdx].bitmap_,
                              other.bitmapMap_[otherIdx].bitmap_);
      if (numBits == 0)
      {
        NADELETEBASIC(bitmapMap_[thisIdx].bitmap_, heap_);
        bitmapMap_[thisIdx].bitmap_ = 0;
        bitmapMap_[thisIdx].significantBits_ = 0;
      }
      else
      {
        numSetBits += numBits;
        bitmapMap_[newMapIdx].significantBits_ =
              bitmapMap_[thisIdx].significantBits_;
        bitmapMap_[newMapIdx].bitmap_ = bitmapMap_[thisIdx].bitmap_;
        newMapIdx++;
      }

      thisIdx++;
      otherIdx++;
    }
    else if (bitmapMap_[thisIdx].significantBits_ <
             other.bitmapMap_[otherIdx].significantBits_)
    {
      numSetBits += countBits(bitmapMap_[thisIdx].bitmap_);
      bitmapMap_[newMapIdx].bitmap_ = bitmapMap_[thisIdx].bitmap_;
      bitmapMap_[newMapIdx].significantBits_ = 
           bitmapMap_[thisIdx].significantBits_;
      thisIdx++;
      newMapIdx++;
    }
    else
    {
      otherIdx++;
    }
  }

  // Move any remaining ones bitmaps forward in the array
  while (thisIdx < numBitmaps_)
  {
    numSetBits += countBits(bitmapMap_[thisIdx].bitmap_);
    bitmapMap_[newMapIdx].bitmap_ = bitmapMap_[thisIdx].bitmap_;
    bitmapMap_[newMapIdx].significantBits_ = 
         bitmapMap_[thisIdx].significantBits_;
    thisIdx++;
    newMapIdx++;
  }

  // Set remaining ones to NULL.  These ones have already been moved
  // forward so they should not be deleted.  Also, this loop isn't really
  // necessary, but may help with debugging.
  for (UInt32 j = newMapIdx; j < numBitmaps_; j++)
  {
    bitmapMap_[j].bitmap_ = 0;
    bitmapMap_[j].significantBits_ = 0;
  }

  numBitmaps_ = newMapIdx;
  entries_ = numSetBits;
  return *this;
}

// Find the next set bit in the bitmap starting with the passed in value.
// True is returned if a value was successfully found, and false is returned
// if there is not another bit set.
NABoolean
ClusteredBitmap::nextUsed(CollIndex &start) const
{
  cb_int_t *bitmap;
  UInt32 sigBits = significantBits(start);
  bitsToBitmap * map = findEqualOrGreaterBitmapMap(sigBits);

  if (!map)
    return false;

  // If an exact bitmap match was found, search the rest of it for another
  // set bit.
  if (map->significantBits_ == sigBits)
  {
    bitmap = map->bitmap_;
    UInt32 wordIdx = word(start);
    cb_int_t bit = shiftedBit(start);

    // Set all bits to the left of the bit including the shifted bit.
    cb_int_t bitMask = ~(bit - 1);

    // Determine if any bits are set above this one in this word.
    cb_int_t setBits = bitmap[wordIdx] & bitMask;

    // If any bits are set, then determine the next set bit and return
    // it in the "start" parameter.
    if (setBits)
    {
      // Isolate the least significant bit set in this word.
      cb_int_t leastBit = firstSetBit(setBits);

      // Determine the index of this bit using population count.
      UInt32 bitIdx = popCnt(--leastBit);

      start = map->significantBits_ + (wordIdx << WORD_BITS_SHIFT) +
              bitIdx;
      return true;
    }

    // Check the rest of the words in this bitmap.
    for (wordIdx++; wordIdx < WORDS_PER_BITMAP; wordIdx++)
    {
      // Store the word in a local variable
      cb_int_t wordVal = bitmap[wordIdx];

      // If any bits are set, then determine the first set bit and
      // return it in the "start" parameter.
      if (wordVal != 0)
      {
        // Isolate the least significant bit set in this word.
        cb_int_t leastBit = firstSetBit(wordVal);

        // Determine the index of this bit using population count.
        UInt32 bitIdx = popCnt(--leastBit);

        start = map->significantBits_ + (wordIdx << WORD_BITS_SHIFT) +
                bitIdx;
        return true;
      }
    }
    // Increment map pointer to point to next bitmap map.
    map++;
  }

  // Get a pointer to one past the final bitmap.
  bitsToBitmap *endPtr = &bitmapMap_[numBitmaps_];

  // Search the rest of the bitmaps for a set bit.
  while (map < endPtr)
  {
    cb_int_t *wordPtr = map->bitmap_;
    for (UInt32 wordIdx = 0; wordIdx != WORDS_PER_BITMAP; wordIdx++)
    {
      cb_int_t wordVal = *wordPtr++;
      if (wordVal != 0)
      {
        // Isolate the least significant bit set in this word.
        cb_int_t leastBit = firstSetBit(wordVal);

        // Determine the index of this bit using population count.
        UInt32 bitIdx = popCnt(--leastBit);
        start = map->significantBits_ + (wordIdx << WORD_BITS_SHIFT) +
                bitIdx;
        return true;
      }
    }
    // Increment map pointer to point to next bitmap map.
    map++;
  }

  // There were no more set bits.  Return false.
  return false;
}

// Perform a logical AND of this bitmap with another.
// This operation always results in either the same number of
// bitmaps or less bitmaps than this object started with.
// The only bitmaps that remain must be in common between
// the two objects.  This function removes any bitmaps that
// do not have any bits set.
ClusteredBitmap &
ClusteredBitmap::intersectSet(const ClusteredBitmap &other)
{
  UInt32 thisIdx = 0;
  UInt32 otherIdx = 0;
  UInt32 numBitmaps = 0;
  UInt32 newNumBits = 0;
  UInt32 numSetBits;

  // Examine each bitmap array 
  while (thisIdx < numBitmaps_ && otherIdx < other.numBitmaps_)
  {
    if (bitmapMap_[thisIdx].significantBits_ ==
        other.bitmapMap_[otherIdx].significantBits_)
    {
      // Perform a logical AND of the two bitmaps.
      numSetBits = andBits(bitmapMap_[thisIdx].bitmap_,
                           other.bitmapMap_[otherIdx].bitmap_);

      // If there were no bits in common between the two bitmaps
      // then delete this bitmap.  Otherwise adjust the significant
      // bits and bitmap pointer in the array to account for any
      // other bitmaps that may have been deleted during other times
      // through the "while" loop.
      if (numSetBits == 0)
      {
        NADELETEBASIC(bitmapMap_[thisIdx].bitmap_, heap_);
        bitmapMap_[thisIdx].bitmap_ = 0;
        bitmapMap_[thisIdx].significantBits_ = 0;
      }
      else
      {
        newNumBits += numSetBits;
        bitmapMap_[numBitmaps].significantBits_ =
              bitmapMap_[thisIdx].significantBits_;
        bitmapMap_[numBitmaps].bitmap_ = bitmapMap_[thisIdx].bitmap_;
        numBitmaps++;
      }

      thisIdx++;
      otherIdx++;
    }
    else if (bitmapMap_[thisIdx].significantBits_ <
             other.bitmapMap_[otherIdx].significantBits_)
    {
      // This bitmap does not have a match in the other.  It must be
      // deleted.
      NADELETEBASIC(bitmapMap_[thisIdx].bitmap_, heap_);
      bitmapMap_[thisIdx].bitmap_ = 0;
      bitmapMap_[thisIdx].significantBits_ = 0;
      thisIdx++;
    }
    else
    {
      // This other bitmap does not have a match in this one.  Simply
      // increment the other index.
      otherIdx++;
    }
  }

  // Remove any that weren't already handled in previous loop.
  while (thisIdx < numBitmaps_)
  {
    NADELETEBASIC(bitmapMap_[thisIdx].bitmap_, heap_);

    // It isn't necessary to set these to 0, but it may help with debugging.
    bitmapMap_[thisIdx].bitmap_ = 0;
    bitmapMap_[thisIdx].significantBits_ = 0;
    thisIdx++;
  }
  // Set the number of bitmaps to the number that were handled in the first
  // while loop.
  entries_ = newNumBits;
  numBitmaps_ = numBitmaps;
  return *this;

} // ClusteredBitmap::intersectSet(const ClusteredBitmap &other)

// Return whether all bits in the other set are set within this set.
NABoolean
ClusteredBitmap::contains(const ClusteredBitmap & other) const
{
  UInt32 thisIdx = 0;
  UInt32 otherIdx = 0;
  UInt32 wordIdx;
  cb_int_t *otherWordPtr;
  cb_int_t *thisWordPtr;

  // Search through the the bitmaps of both objects for bits that
  // are set only in the other ClusteredBitmap
  while (thisIdx < numBitmaps_ && otherIdx < other.numBitmaps_)
  {
    if (bitmapMap_[thisIdx].significantBits_ ==
        other.bitmapMap_[otherIdx].significantBits_)
    {
      // Set the bitmap pointers and also increment each map index
      otherWordPtr = other.bitmapMap_[otherIdx++].bitmap_;
      thisWordPtr = bitmapMap_[thisIdx++].bitmap_;
      for (wordIdx = WORDS_PER_BITMAP; wordIdx != 0; wordIdx--)
      {
        // for a word present in both, "other" mustn't have bits on that
        // are off in this word (in other words, when ORing the two words,
        // other must not cause any changes by having additional bits set)
        cb_int_t word = *thisWordPtr++;
        if (word != (word | *otherWordPtr++))
          return false;
      }
    }
    else if (bitmapMap_[thisIdx].significantBits_ <
             other.bitmapMap_[otherIdx].significantBits_)
    {
      // If the other contains bits and this is missing a bitmap
      // for this range, then return false;
      // If there isn't a bitmap in the other, then just skip
      // the bitmap in this by incrementing the map index.
      thisIdx++;
    }
    else
    {
      // Set the bitmap pointer and increment the other map index
      otherWordPtr = other.bitmapMap_[otherIdx++].bitmap_;

      // If the other contains bits and this is missing a bitmap
      // for this range, then return false;
      for (wordIdx = WORDS_PER_BITMAP; wordIdx != 0; wordIdx--)
        if (*otherWordPtr++ != 0)
          return false;
    }
  }

  // Search any remaining in other for set bits.  If there are
  // any, then return false.
  while (otherIdx < other.numBitmaps_)
  {
    otherWordPtr = other.bitmapMap_[otherIdx++].bitmap_;
    for (wordIdx = WORDS_PER_BITMAP; wordIdx != 0; wordIdx--)
      if (*otherWordPtr++ != 0)
        return false;
  }

  // No bits were found that are only set in "other", so return true.
  return true;

} // ClusteredBitmap::contains(const ClusteredBitmap & other) const

// Return whether the passed in value is set within this bitmap.
NABoolean
ClusteredBitmap::contains(const UInt32 value) const
{
  cb_int_t *bitmap = findBitmap(significantBits(value));
  return (bitmap && (bitmap[word(value)] & shiftedBit(value)) != 0);
}

// Assign this ClusteredBitmap to another one.
ClusteredBitmap &
ClusteredBitmap::operator= (const ClusteredBitmap &other)
{
  if (this != &other)
  {
    // Delete all of the previous bitmaps
    for (UInt32 thisIdx = 0; thisIdx < numBitmaps_; thisIdx++)
      NADELETEBASIC(bitmapMap_[thisIdx].bitmap_, heap_);

    // Set numBitmaps_ to zero and call increaseBitmapMapSize() to
    // allocation additional memory if necessary.  NOTE: numBitmaps_
    // is set to zero before calling increaseBitmapMapSize() because
    // there is no need to copy the old bitmap information.
    numBitmaps_ = 0;
    if (other.numBitmaps_ > maxBitmaps_)
      increaseBitmapMapSize(other.numBitmaps_);

    numBitmaps_ = other.numBitmaps_;
    entries_ = other.entries_;

    // Copy each of the other's bitmaps
    for (UInt32 thisIdx = 0; thisIdx < numBitmaps_; thisIdx++)
    {
      bitmapMap_[thisIdx].significantBits_ = 
           other.bitmapMap_[thisIdx].significantBits_;
      bitmapMap_[thisIdx].bitmap_ = new (heap_) cb_int_t[WORDS_PER_BITMAP];
      copyBits(bitmapMap_[thisIdx].bitmap_, other.bitmapMap_[thisIdx].bitmap_);
    }
  }
  return *this;
}

// Compare two ClusteredBitmap objects to see whether they
// contain the same set bits.
NABoolean
ClusteredBitmap::operator== (const ClusteredBitmap &other) const
{
  if (entries_ != other.entries_)
    return false;

  if (entries_ == 0 && other.entries_ == 0)
    return true;

  UInt32 thisIdx = 0;
  UInt32 otherIdx = 0;
  UInt32 wordIdx;
  cb_int_t *thisWordPtr;
  cb_int_t *otherWordPtr;

  // Compare each bitmap to the other.  This code must take into
  // account the possibility of an empty bitmap in either of the
  // ClusteredBitmap objects.
  while (thisIdx < numBitmaps_ && otherIdx < other.numBitmaps_)
  {
    if (bitmapMap_[thisIdx].significantBits_ ==
        other.bitmapMap_[otherIdx].significantBits_)
    {
      thisWordPtr = bitmapMap_[thisIdx++].bitmap_;
      otherWordPtr = other.bitmapMap_[otherIdx++].bitmap_;
      for (wordIdx = WORDS_PER_BITMAP; wordIdx != 0; wordIdx--)
        if (*thisWordPtr++ != *otherWordPtr++)
          return false;
    }
    else if (bitmapMap_[thisIdx].significantBits_ <
             other.bitmapMap_[otherIdx].significantBits_)
    {
      // if this contains any set bits, then the two are not
      // equal.
      thisWordPtr = bitmapMap_[thisIdx++].bitmap_;
      for (wordIdx = WORDS_PER_BITMAP; wordIdx != 0; wordIdx--)
        if (*thisWordPtr++ != 0)
          return false;
    }
    else
    {
      // if the other contains any set bits, then the two are not
      // equal.
      otherWordPtr = other.bitmapMap_[otherIdx++].bitmap_;
      for (wordIdx = WORDS_PER_BITMAP; wordIdx != 0; wordIdx--)
        if (*otherWordPtr++ != 0)
          return false;
    }
  }

  // Check any remaining bitmaps in this object
  while (thisIdx < numBitmaps_)
  {
    thisWordPtr = bitmapMap_[thisIdx++].bitmap_;
    for (wordIdx = WORDS_PER_BITMAP; wordIdx != 0; wordIdx--)
      if (*thisWordPtr++ != 0)
        return false;
  }

  // Check any remaining bitmaps in the other object
  while (otherIdx < other.numBitmaps_)
  {
    otherWordPtr = other.bitmapMap_[otherIdx++].bitmap_;
    for (wordIdx = WORDS_PER_BITMAP; wordIdx != 0; wordIdx--)
      if (*otherWordPtr++ != 0)
        return false;
  }
 
  return true;
}

// Provide a hash value based on bits that are set in the bitmaps
ULng32
ClusteredBitmap::hash() const
{
  cb_int_t hashVal = 0;

  for (UInt32 thisIdx = 0; thisIdx < numBitmaps_; thisIdx++)
  {
    cb_int_t *wordPtr = bitmapMap_[thisIdx].bitmap_;
    for (UInt32 wordIdx = WORDS_PER_BITMAP; wordIdx != 0; wordIdx--)
      hashVal ^= *wordPtr++;
  }

#if (CB_BITS_PER_WORD == 32)
  return hashVal;
#else
  return (ULng32)(hashVal & 0xFFFFFFFF) ^ (ULng32)(hashVal >> 32);
#endif
}
