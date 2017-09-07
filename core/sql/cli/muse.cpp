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

#include <iomanip>
#include <iostream>
#include <sstream>

NAHeap *ipcHeap;
const size_t numTotalBins = NAHeap::NSMALLBINS + NAHeap::NTREEBINS;

class State {
public:
  NAHeap *executorHeap;
  short summary;
  short error;
  State(NAHeap *executorHeapArg) : executorHeap(executorHeapArg), summary(false)
    {}
};


// The LevelStats are the aggregate stats for each memory level
struct LevelStats
{
  bool   encountered_;
  size_t blockCnt_;
  size_t blockSum_;
  size_t maxAllocSize_;
  size_t allocSize_;
  size_t derivedSize_;
  size_t freeSize_;

  static void resetLevelStats();
};

#define MAX_HEAP_LEVELS 8
static LevelStats heapLevel[MAX_HEAP_LEVELS];
static LevelStats spaceLevel[MAX_HEAP_LEVELS];

void LevelStats::resetLevelStats()
{
  memset(heapLevel, 0, sizeof(heapLevel));
  memset(spaceLevel, 0, sizeof(spaceLevel));
}

// Abstrace base class used to store statistics shared by Spaces and Heaps
class ExMemStats {
public:
  // Constructor and destructor
  ExMemStats(ExMemStats *parent, NAMemory *memory, Int32 level);
  virtual ~ExMemStats();

  // Function for gathering statistics
  virtual void GetStatistics() = 0;

  // Functions for displaying the statistics
  void Display(ostream &os, State *state, size_t size);

  virtual void DisplayLongMemStats(ostream &os, char *ind, size_t minSize) = 0;
  void DisplayShortMemStats(ostream &os);

  Int32    level_;           // Number of levels of derived memory
  size_t blockCnt_;        // Number of blocks in this memory
  size_t blockSum_;        // Sum of the sizes of all blocks
  size_t allocCnt_;        // Number of in use allocations
  size_t allocSum_;        // Sum of all in use allocation sizes
  size_t freeCnt_;         // Number of not used allocations
  size_t freeSum_;         // Sum of all no used allocation sizes
  size_t maxAllocSize_;    // The reported high water mark for this memory
  size_t derivedMemSum_;   // The amount of derived memory in this memory

  ExMemStats *parentStats_;       // Pointer to the parent memory stats
  ExMemStats *derivedStats_;      // Pointer to the first derived stats
  ExMemStats *nextDerivedStats_;  // Pointer to the next derived stats at this level

  NAMemory *thisMemory_;          // Pointer ot NAMemory

  void DisplayLevelStats(ostream &os, char *type, Int32 level, LevelStats &stats);
};

// Heap-specific statistics class
class ExHeapStats : public ExMemStats
{
public:
  ExHeapStats(ExMemStats *parent, NAMemory *heap, Int32 level);

  virtual void GetStatistics();

private:
  virtual void DisplayLongMemStats(ostream &os, char *ind, size_t minSize);

  size_t sizeBinCnt_[numTotalBins]; // The number of allocations for each bin
  size_t sizeBinSum_[numTotalBins]; // Sum of sizes for each bin
};

// Space-specific statistics class
class ExSpaceStats : public ExMemStats
{
public:
  ExSpaceStats(ExMemStats *parent, NAMemory *space, Int32 level);

  virtual void GetStatistics();

private:
  virtual void DisplayLongMemStats(ostream &os, char *ind, size_t minSize);
};

ExMemStats::ExMemStats(ExMemStats *parent, NAMemory *memory, Int32 level) :
  level_(level),
  blockCnt_(0),
  blockSum_(0),
  allocCnt_(0),
  allocSum_(0),
  freeCnt_(0),
  freeSum_(0),
  maxAllocSize_(0),
  derivedMemSum_(0),
  parentStats_(parent),
  derivedStats_(NULL),
  nextDerivedStats_(NULL),
  thisMemory_(memory)
{}

ExMemStats::~ExMemStats()
{
  // Recursivily delete all of the memory statistics
  ExMemStats *p = derivedStats_;
  while (p != NULL) {
    ExMemStats *next = p->nextDerivedStats_;
    delete p;
    p = next;
  }
}


ExHeapStats::ExHeapStats(ExMemStats *parent, NAMemory *heap, Int32 level)
 : ExMemStats(parent, heap, level)
{
  for (size_t i = 0; i < numTotalBins; i++) {
    sizeBinCnt_[i] = 0;
    sizeBinSum_[i] = 0;
  }
}

ExSpaceStats::ExSpaceStats(ExMemStats *parent, NAMemory *space, Int32 level)
 : ExMemStats(parent, space, level)
{
}

void ExHeapStats::GetStatistics()
{
  // Increment NAHeap level stats
  maxAllocSize_ = thisMemory_->highWaterMark_;

  // Get the first NABlock for this NAHeap and return if there are
  // no blocks
  NABlock *block = thisMemory_->firstBlk_;
  if (block == NULL)
    return;

  while (block != NULL) {

    blockCnt_++;
    blockSum_ += block->size_;

    if (parentStats_ != NULL &&
        block->segmentId_ == NABlock::DERIVED_SEGMENT_ID)
      parentStats_->derivedMemSum_ += PAD_REQUEST(block->size_);

    NAHeapFragment *fp = block->alignAsFragment();
    while (!fp->isFencePost()) {
      size_t fragSize = fp->fragmentSize();
      if (fp->cinuse()) {
        allocCnt_++;
        allocSum_ += fragSize;
        NAHeap::bindex_t binNum;
        if (NAHeap::isSmall(fragSize))
          binNum = NAHeap::smallIndex(fragSize);
        else
          binNum = NAHeap::NSMALLBINS + NAHeap::computeTreeIndex(fragSize);

        sizeBinCnt_[binNum]++;
        sizeBinSum_[binNum] += fragSize;

        // SYSTEM_MEMORY and IPC_MEMORY NAMemory classes can be allocated
        // within EXECUTOR_MEMORY, but not registered as derived memory.
        // Try to pick up on that here.  The ipcHeap will be analyzed later
        // after share() maps in the proper segments.
        if (fragSize == PAD_REQUEST(sizeof(NAHeap))) {
           NAHeap *testHeap = (NAHeap*)fp->getMemory();
           if (testHeap->type_ == NAMemory::SYSTEM_MEMORY ||
               testHeap->type_ == NAMemory::IPC_MEMORY) {
             ipcHeap = testHeap;
           }
        }
        // ComSpace memories do not currently register themselves with
        // their parent class.  That could probably be fixed in the future.
        // For now, we can determine whether this fragment contains a
        // ComSpace object.
        else if (fragSize == PAD_REQUEST(sizeof(ComSpace))) {
          ComSpace *testSpace = (ComSpace*)fp->getMemory();
          if (testSpace->ComSpace::parent_ == thisMemory_) {
            ExSpaceStats *newDerivedStats = new ExSpaceStats(
                 this, testSpace, level_ + 1);
                 
            newDerivedStats->nextDerivedStats_ = derivedStats_;
            derivedStats_ = newDerivedStats;

            newDerivedStats->GetStatistics();
          }
        }
      }
      else {
        freeCnt_++;
        freeSum_ += fragSize;
      }
      fp = fp->nextFragment();
    }

    block = block->next_;
  }

  if (blockCnt_ != thisMemory_->blockCnt_)
/*    cout << "Invalid block count (" << blockCnt_ << " != "
         << thisMemory_->blockCnt_ << ")." << endl*/;

  // Loop over the child NAHeaps and read the statistics for them
  NAMemory *derivedMem = thisMemory_->memoryList_;
  while (derivedMem != NULL) {

    ExMemStats *newDerivedStats = new ExHeapStats(
            this, derivedMem, level_ + 1);

    newDerivedStats->nextDerivedStats_ = derivedStats_;
    derivedStats_ = newDerivedStats;

    newDerivedStats->GetStatistics();

    derivedMem = derivedMem->nextEntry_;
  }

  // Update the level stats.  This has to be done after traversing
  // the child memories to calculate the correct derivedSize_.
  heapLevel[level_].encountered_ = true;
  heapLevel[level_].blockCnt_ += blockCnt_;
  heapLevel[level_].blockSum_ += blockSum_;
  heapLevel[level_].maxAllocSize_ += thisMemory_->highWaterMark_;
  heapLevel[level_].allocSize_ += allocSum_;
  heapLevel[level_].derivedSize_ += derivedMemSum_;
  heapLevel[level_].freeSize_ += freeSum_;
}

void ExSpaceStats::GetStatistics()
{
  Block * block = ((Space*)thisMemory_)->firstBlock_;
  while (block != NULL) {
    blockCnt_++;
    blockSum_ += block->getBlockSize();
    parentStats_->derivedMemSum_ += PAD_REQUEST(block->getBlockSize());
    allocSum_ += block->getAllocatedSize();
    freeSum_  += block->getFreeSpace();
    block = block->getNext();
  }
  maxAllocSize_ = allocSum_;

  // Update the level stats
  spaceLevel[level_].encountered_ = true;
  spaceLevel[level_].blockCnt_ += blockCnt_;
  spaceLevel[level_].blockSum_ += blockSum_;
  spaceLevel[level_].maxAllocSize_ += allocSum_;
  spaceLevel[level_].allocSize_ += allocSum_;
  spaceLevel[level_].freeSize_ += freeSum_;
}

void ExHeapStats::DisplayLongMemStats(ostream &os, char *ind, size_t minSize)
{

  if (blockSum_ < minSize)
    return;

  os << ind << "Type: ";

  switch (thisMemory_->type_)
  {
    case NAMemory::NO_MEMORY_TYPE:
            os << "NO_MEMORY_TYPE" << endl;
            break;
    case NAMemory::EXECUTOR_MEMORY:
            os << "EXECUTOR_MEMORY" << endl;
             break;
    case NAMemory::SYSTEM_MEMORY:
            os << "SYSTEM_MEMORY" << endl;
            break;
    case NAMemory::DERIVED_MEMORY:
            os << "DERIVED_MEMORY" << endl;
            break;
    case NAMemory::DERIVED_FROM_SYS_HEAP:
            os << "DERIVED_FROM_SYS_HEAP" << endl;
            break;
    case NAMemory::IPC_MEMORY:
            os << "IPC_MEMORY" << endl;
            break;
  }

  os << ind << "NAMemory address: " << thisMemory_ << endl;
  if (thisMemory_->parent_)
    os << ind << "Parent address: " << thisMemory_->parent_ << endl;
  os << ind << "Number of blocks: " << blockCnt_ << endl;
  switch (thisMemory_->type_)
  {
    case NAMemory::EXECUTOR_MEMORY:
    case NAMemory::SYSTEM_MEMORY:
    case NAMemory::IPC_MEMORY:
      {
        NABlock *block = thisMemory_->firstBlk_;
        while (block != NULL) {
          os << ind << /*"  Seg Id " << block->segmentId_
             << */"  NABlock@" << block 
             << "  (" << (block->size_ >> 10) << " Kbytes)" << endl;
          block = block->next_;
        }
      }
      break;
    default:
      break;
  }

  os << ind << "Total size of blocks: " << blockSum_ << endl;
  os << ind << "Number of free fragments: " << freeCnt_ << endl;
  os << ind << "Total size of free fragments: " << freeSum_ << endl;
  os << ind << "Number of allocated fragments: " << allocCnt_ << endl;
  os << ind << "Total size of allocated fragments: " << allocSum_ << endl;
  os << ind << "Maximum size of allocated fragments: " << maxAllocSize_ 
     << endl;
  os << ind << "Derived memory level: " << level_ << endl;
  os << ind << "Total size of derived memory blocks: " << derivedMemSum_ 
     << endl;

  os << ind << " Frag Min - Frag Max  Num Allocs  Total Frag Size" << endl;
  size_t prevSize = 1;
  for (size_t i = 0; i < numTotalBins; i++) {
    size_t binSize;
    if (i < NAHeap::NSMALLBINS) {
      binSize = NAHeap::smallIndex2Size(i);
      prevSize = binSize;
    } else {
      binSize = NAHeap::minsizeForTreeIndex(i - NAHeap::NSMALLBINS + 1);
    }

    if (sizeBinCnt_[i] != 0) {
      os << ind << setw(9) << prevSize << " - " << setw(9) 
         << setiosflags(ios::left) << binSize << resetiosflags(ios::left)
         << setw(11) << sizeBinCnt_[i] 
         << setw(17) << sizeBinSum_[i] << endl;
    }
    prevSize = binSize + 1;
  }
}

void ExSpaceStats::DisplayLongMemStats(ostream &os, char *ind, size_t minSize)
{

  if (blockSum_ < minSize)
    return;

  Space * space = (Space*)thisMemory_;
  os << ind << "Type: ";

  switch (space->ComSpace::type_)
  {
    case ComSpace::EXECUTOR_SPACE:
            os << "EXECUTOR_SPACE" << endl;
            break;
    case ComSpace::GENERATOR_SPACE:
            os << "GENERATOR_SPACE" << endl;
            break;
    case ComSpace::SYSTEM_SPACE:
            os << "SYSTEM_SPACE" << endl;
            break;
    case ComSpace::SINGLE_BLOCK_SPACE:
            os << "SINGLE_BLOCK_SPACE" << endl;
            break;
    default:
            os << "Unknown space type: " 
               << space->ComSpace::type_ << endl;
            break;
  }
  os << ind << "NAMemory address: " << thisMemory_ << endl;
  os << ind << "Derived memory level: " << level_ << endl;
  os << ind << "Number of blocks: " << blockCnt_ << endl;
  os << ind << "Total size of blocks: " << blockSum_ << endl;
  os << ind << "Used space memory: " << allocSum_ << endl;
  os << ind << "Unused space memory: " << freeSum_ << endl;
}

void ExMemStats::DisplayLevelStats(ostream &os, char *type, Int32 level,
                                   LevelStats &stats)
{
  os << setw(5)  << level << "  "
     << setw(5)  << type  << " "
     << setw(6)  << stats.blockCnt_  << "  "
     << setw(10) << stats.blockSum_  << " "
     << setw(10) << stats.maxAllocSize_ << " "
     << setw(10) << stats.allocSize_ << "   "
     << setw(10) << stats.derivedSize_ << " "
     << setw(10) << stats.freeSize_
     << endl;
}

void ExMemStats::DisplayShortMemStats(ostream &os)
{
  // Display short memory stats header
  os << "Level  Type  Blocks        Size   MaxAlloc  AllocSize  "
        "DerivedSize   FreeSize" << endl;

  // Display the statistics for each level
  for (Int32 i = 0; i < MAX_HEAP_LEVELS; i++)
  {
    if (heapLevel[i].encountered_)
      DisplayLevelStats(os, (char *)"Heap", i, heapLevel[i]);
    if (spaceLevel[i].encountered_)
      DisplayLevelStats(os, (char *)"Space", i, spaceLevel[i]);
  }
}

void ExMemStats::Display(ostream &os, State *state, size_t minSize)
{
  // For short memory stats, just print a short summary of the top-most
  // memory and return
  if (level_ == 0) {
    DisplayShortMemStats(os);
    if (state->summary)
      return;
    os << endl;
  }

  if (blockSum_ < minSize)
    return;

  // Set the amount of indent
  char ind[20];
  size_t numSpaces = level_ << 1;  // 2 spaces for each level
  memset(ind, ' ', numSpaces);
  ind[numSpaces] = '\0';

  os << ind << "Name: " << thisMemory_->name_ << endl;

  // Call virtual function to display information specific to NAHeap
  // or ComSpace
  DisplayLongMemStats(os, ind, minSize);

  os << endl;

  ExMemStats *derivedMemStats = derivedStats_;
  while (derivedMemStats != NULL) {
    derivedMemStats->Display(os, state, minSize);
    derivedMemStats = derivedMemStats->nextDerivedStats_;
  }
}

Int32 muse(NAHeap *heap, size_t minTotalSize, char *rspBuffer, size_t rspMaxSize, size_t *rspSize, bool *bufferFull) {
  stringstream *ss = new stringstream(stringstream::in | stringstream::out);
  streamsize ssSize;
  char testBuf[1];
  char truncatedMsg[] = "\n***Muse output has been truncated***";

  State state(heap); //arg 1
  memset(heapLevel, 0, sizeof(heapLevel));
  memset(spaceLevel, 0, sizeof(spaceLevel));
  ExHeapStats exHeapInfo(NULL, state.executorHeap, 0);
  exHeapInfo.GetStatistics();
  exHeapInfo.Display(*ss, &state, minTotalSize);
  ss->read(rspBuffer, rspMaxSize);
  ssSize = ss->gcount();
  *rspSize = (size_t)ssSize;
  ss->read(&testBuf[0], 1);
  ssSize = ss->gcount();
  if (ssSize > 0)
  {
    strcpy(rspBuffer + rspMaxSize - sizeof(truncatedMsg), truncatedMsg);
    *bufferFull = true;
  }
  else
    *bufferFull = false;
  delete ss;
  return 0;
}

