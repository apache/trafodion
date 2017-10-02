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

// ***********************************************************************
//
// File:         QmsSelfJoinHandler.cpp
// Description:  
//               
//               
//               
//
// Created:      09/12/2011
// ***********************************************************************

#include "QmsSelfJoinHandler.h"
#include "QRLogger.h"

//========================================================================
//  Class Array2D
//========================================================================

//*****************************************************************************
// Array2D constructor: handle memory allocation.
//*****************************************************************************
Array2D::Array2D(UInt32 rows, UInt32 cols, Int32 initValue,
	         ADD_MEMCHECK_ARGS_DECL(CollHeap* heap))
  : NAIntrusiveSharedPtrObject(ADD_MEMCHECK_ARGS_PASS(heap))
   ,rows_(rows)
   ,cols_(cols)
   ,heap_(heap)
{
  // Allocate the array of rows
  array_ = new(heap) Int32*[rows];

  // Now allocate each row
  for (CollIndex i=0; i<rows; i++)
  {
    Int32* row = new (heap) Int32[cols];
    for (CollIndex j=0; j<cols; j++)
      row[j] = initValue;

    array_[i] = row;
  }
}

//*****************************************************************************
//*****************************************************************************
Array2D::~Array2D()
{
  for (CollIndex i=0; i<rows_; i++)
    NADELETEARRAY(array_[i], cols_, Int32, heap_);

  NADELETEARRAY(array_, rows_, Row, heap_);
}

//*****************************************************************************
//*****************************************************************************
Int32 Array2D::getElement(UInt32 row, UInt32 col)
{
  assertLogAndThrow(CAT_MVMEMO_JOINGRAPH, LL_MVQR_FAIL,
                   (row < rows_ && col < cols_), QRLogicException, 
		    "Out of bounds access to Array2D.");
  return array_[row][col];
}

//*****************************************************************************
//*****************************************************************************
void Array2D::setElement(UInt32 row, UInt32 col, Int32 value)
{
  assertLogAndThrow(CAT_MVMEMO_JOINGRAPH, LL_MVQR_FAIL,
                   (row < rows_ && col < cols_), QRLogicException, 
		    "Out of bounds access to Array2D.");
  array_[row][col] = value;
}

//*****************************************************************************
//*****************************************************************************
void Array2D::dump(NAString& text)
{
  char buffer[100];

  for (CollIndex row=0; row < rows_; row++)
  {
    for (CollIndex col=0; col < cols_; col++)
    {
      sprintf(buffer, "%2d", array_[row][col]);
      text += buffer;
      if (col == cols_-1)
        text += "\n";
      else
        text += ", ";
    }
  }
}

//========================================================================
//  Class ShiftMatrix
//========================================================================

//*****************************************************************************
// Create and initialize a ShiftMatrix of size size.
//*****************************************************************************
ShiftMatrix::ShiftMatrix(Int32  size,
	                 ADD_MEMCHECK_ARGS_DECL(CollHeap* heap))
  : NAIntrusiveSharedPtrObject(ADD_MEMCHECK_ARGS_PASS(heap)),
    theMatrix_(NULL),
    elements_(size),  
    combinations_(factorial(elements_)),  // The number of permutations is size!
    heap_(heap)
{
  // Allocate the array of permutations
  theMatrix_ = new(heap) 
    Array2D(combinations_, elements_, 9999, ADD_MEMCHECK_ARGS(heap));

  // Initialize the matrix.
  init();
}

//*****************************************************************************
//*****************************************************************************
ShiftMatrix::~ShiftMatrix()
{
  deletePtr(theMatrix_);
  theMatrix_ = NULL;
}

//*****************************************************************************
// Get an element of the array.
//*****************************************************************************
Int32 ShiftMatrix::getElement(UInt32 combination, UInt32 element)
{
  return theMatrix_->getElement(combination, element);
}

//*****************************************************************************
// Calculate the factorial of the input parameter.
//*****************************************************************************
Int32 ShiftMatrix::factorial(Int32 num)
{
  Int32 result = 1;
  for (Int32 i=2; i<=num; i++)
    result *= i;

  return result;
}

//*****************************************************************************
// Initialize the ShiftMatrix values.
//*****************************************************************************
void ShiftMatrix::init()
{
  // The usedValues bitmap is used to keep track of which values were already
  // used by the algorithm, and which still need to be used.
  NABitVector usedValues(heap_);
  // Later on, we use the nextUsed() method to find the next value that was not 
  // yet used. Since there is no nextNotUsed() method, we are using the bitmap 
  // negatively - a set bit corresponds to a value that was not yet used.
  for (CollIndex i=0; i<elements_; i++)
    usedValues.insert(i);  // Start with all bits set.

  // Init the entire matrix 
  initNext(usedValues, 0, combinations_-1, elements_);
}

//*****************************************************************************
// This is a recursive method used to initialize the matrix a column at a time.
// First divide the array of combinations to a number of segments, one for each 
// possible value left. Fill the next matrix entry for each segment with the 
// next unused value, and call the method recursively for the rest of the values.
//*****************************************************************************
void ShiftMatrix::initNext(NABitVector& usedValuesV, // Which values were already used
                           UInt32       from,        // From which combination to start
                           UInt32       to,          // Till which combination to work
                           UInt32       depth)       // How many values left
{
  // The number of segments is the number of entries left.
  UInt32 segments = depth; 
  // The size of each segment is the number od combinations to do, 
  // divided by the number ofd segments.
  UInt32 segSize = (to - from + 1) / segments; 
  UInt32 nextValue = 0;
  // Create a copy of the bitmap for this recursion level only (Horizontal)
  // that is not affected by the recursive calls (Vertical)
  NABitVector usedValuesH(usedValuesV);

  // For each segment,
  for (CollIndex seg=0; seg<segments; seg++)
  {
    // Find the next unused value.
    usedValuesH.nextUsed(nextValue);

    // Mark it as used in both vertical and horizontal bitmaps.
    usedValuesV.remove(nextValue);
    usedValuesH.remove(nextValue);

    // Calc the first combination of the segment
    UInt32 segStart = from + seg*segSize;
    // For each combination in the segment
    for (UInt32 comb = segStart; comb < segStart+segSize; comb++)
    {
      // Translate the matrix value (nextValue) from the range: [0..elements_]
      // to the shift value in the range: [-(elements_-1)..(elements_-1)]
      Int32 shiftValue = nextValue - elements_ + depth;
      // Update the matrix value, starting with element 0.
      theMatrix_->setElement(comb, elements_ - depth, shiftValue);
    }

    // Make the recursive call
    if (depth > 1)
      initNext(usedValuesV,             // Use the vertical bitmap
               segStart,                // Start of segment
               segStart + segSize - 1,  // End of segment
               depth - 1);              // One less depth level.

    // Clear the value used in the vertical bitmap, but not the horizontal.
    usedValuesV.insert(nextValue);
  }
}

//*****************************************************************************
//*****************************************************************************
void ShiftMatrix::dump(NAString& text)
{
  char buffer[100];
  sprintf(buffer, "ShiftMatrix for size %d:\n", elements_);
  text += buffer;
  theMatrix_->dump(text);
}

//========================================================================
//  Class ShiftMatrixFactory
//========================================================================

ShiftMatrixFactoryPtr ShiftMatrixFactory::instance_ = NULL;

//*****************************************************************************
// Release memory for all the ShiftMatrix objects in the array.
// Since this is a singleton class, this destructor is only called at process exit.
//*****************************************************************************
ShiftMatrixFactory::~ShiftMatrixFactory()
{
  CollIndex i=0; 
  while (matrixSizeArray_.entries() > 0)
  {
    if (matrixSizeArray_.used(i))
    {
      ShiftMatrixPtr matrix = matrixSizeArray_[i];
      matrixSizeArray_.remove(i);
      deletePtr(matrix);
      i++;
    }
  }
}

//*****************************************************************************
// Get the single instance of the singleton.
//*****************************************************************************
ShiftMatrixFactoryPtr ShiftMatrixFactory::getInstance(NAMemory* heap)
{
  if (!instance_)
    instance_ = new(heap) ShiftMatrixFactory(heap);
  return instance_;
}

//*****************************************************************************
// Get a ShiftMatrix for a specific size.
//*****************************************************************************
const ShiftMatrixPtr ShiftMatrixFactory::getMatrixForSize(Int32 size)
{
  // Did we already build a ShiftMatrix for this size?
  if (matrixSizeArray_.used(size))
  {
    // Yes - return it.
    return matrixSizeArray_[size];
  }
  else
  {
    // No - build one now.
    ShiftMatrixPtr newMatrix = new (heap_) ShiftMatrix(size, ADD_MEMCHECK_ARGS(heap_));
    // And keep the pointer in the array.
    matrixSizeArray_.insertAt(size, newMatrix);
    return newMatrix;
  }
}

//========================================================================
//  Class SelfJoinHandler
//========================================================================

const NAString SelfJoinHandler::firstTable_ = NAString("");

//*****************************************************************************
//*****************************************************************************
SelfJoinHandler::~SelfJoinHandler()
{
  CollIndex maxEntries = segmentArray_.entries();
  for (CollIndex i=0; i<maxEntries; i++)
  {
    SelfJoinSegmentPtr seg = segmentArray_[i];
    segmentArray_[i] = NULL;
    deletePtr(seg);
  }
  segmentArray_.clear();

  deletePtr(permMatrix_);
  permMatrix_=NULL;
}

//*****************************************************************************
// Add a table to the Self-Join analysis.
// This method, together with doneAddingTables(), implement a state machine
// with the following states:
//   ST_START:    The start state. No open segments.
//   ST_SINGLE:   Got the first table of a new segment, don't know segment type yet.
//   ST_UNIQUE:   Last two tables were different.
//   ST_SELFJOIN: Last two tables were the same.
//   ST_END:      We are done.
// The full state machine diagram is in the MVQR IS document.
//*****************************************************************************
void SelfJoinHandler::addTable(JoinGraphTablePtr table)
{
  const NAString& newTable = table->getName(); 
  // This is the current table index.
  UInt32 currentIndex = table->getTempNumber();
  if (currentIndex == -1)
  {
    // This subgraph generation was not yet started, 
    // This must be the initial self-join check.
    currentIndex = table->getOrdinalNumber();
  }

  // Is this table the same as the last one?
  // Skip the string comparison if the table is unique in the full join graph.
  NABoolean isDifferent = !table->isSelfJoinTable() || newTable != *lastTable_;

  // Which state in the state machine are we in?
  switch (state_)
  {
    case ST_START:
      segmentStart_ = currentIndex;
      state_ = ST_SINGLE;
      break;

    case ST_SINGLE:
      if (isDifferent)
        state_ = ST_UNIQUE;
      else
        state_ = ST_SELFJOIN;
      break;

    case ST_SELFJOIN:
      if (isDifferent)
      {
        // We have an open selfjoin segment and got a different table.
        // The selfjoin segment ends with the previous table, and the next
        // segment (which can be either type) starts with this table.
        addSegment(SelfJoinSegment::SELF_JOIN_SEGMENT);
        segmentStart_ = currentIndex;
        state_ = ST_SINGLE;
      }
      else
      {
        // do nothing.
      }
      break;

    case ST_UNIQUE:
      if (isDifferent)
      {
        // do nothing.
      }
      else
      {
        // OK, we have an open unique segment, and the new table is the same 
        // as the last one. So actually, the open unique segment ended with the
        // table before the prevous one, and the new self-join segment started 
        // with the previous table.
        segmentEnd_--;
        addSegment(SelfJoinSegment::UNIQUE_TABLE_SEGMENT);
        segmentStart_ = currentIndex-1;
        state_ = ST_SELFJOIN;
      }
      break;

    case ST_END:
      assertLogAndThrow(CAT_MVMEMO_JOINGRAPH, LL_MVQR_FAIL,
                        FALSE, QRLogicException, 
		        "Adding a table to SelfJoinHandler in ST_END state.");
      break;
  }

  lastTable_ = &table->getName();
  segmentEnd_ = currentIndex;
}

//*****************************************************************************
// Implement the end of the state machine algorithm, closing the last segment.
//*****************************************************************************
void SelfJoinHandler::doneAddingTables()
{
  // Which state in the state machine are we in?
  switch (state_)
  {
    case ST_SINGLE:
    case ST_UNIQUE:
      addSegment(SelfJoinSegment::UNIQUE_TABLE_SEGMENT);
      break;

    case ST_SELFJOIN:
      addSegment(SelfJoinSegment::SELF_JOIN_SEGMENT);
      break;

    case ST_START:
    case ST_END:
      assertLogAndThrow(CAT_MVMEMO_JOINGRAPH, LL_MVQR_FAIL,
                        FALSE, QRLogicException, 
                        "calling SelfJoinHandler::end() in ST_START or ST_END state.");
      break;
  }

  state_ = ST_END;
}

//*****************************************************************************
// Add a segment to the segment array.
//*****************************************************************************
void SelfJoinHandler::addSegment(SelfJoinSegment::SegmentType type)
{
  SelfJoinSegmentPtr newSegment = new (heap_) 
    SelfJoinSegment(type, segmentStart_, segmentEnd_, ADD_MEMCHECK_ARGS(heap_));
  segmentArray_.insertAt(segmentArray_.entries(), newSegment);
}

//*****************************************************************************
// Calculate the needed size of the permutationMatrix, allocate it, 
// and initialize it.
// The permutationMatrix has a column for each SelfJoin segment, and a row
// for each permutation.
//*****************************************************************************
void SelfJoinHandler::preparePermutationMatrix()
{
  totalPermutations_ = 1;
  CollIndex segments = segmentArray_.entries();
  UInt32 matrixWidth = 0;
  // The permVector holds for each SelfJoin segment its number of permutations.
  UInt32* permVector = new (heap_) UInt32[segments];
  ShiftMatrixFactoryPtr factory = ShiftMatrixFactory::getInstance(heap_);

  // Go over each segment in the array.
  for (CollIndex i=0; i<segments; i++)
  {
    SelfJoinSegmentPtr segment = segmentArray_[i];
    if (segment->isSelfJoinSegment())
    {
      totalPermutations_ *= segment->getPermutations();
      segment->setMatrixIndex(matrixWidth);
      permVector[matrixWidth] = segment->getPermutations();
      matrixWidth++;
      segment->setShiftMatrix(factory->getMatrixForSize(segment->getSize()));
    }
  }
  
  // Allocate the permutationMatrix.
  permMatrix_ = new(heap_) 
    Array2D(matrixWidth, totalPermutations_, 9999, ADD_MEMCHECK_ARGS(heap_));

  // And initialize it.
  initPermutationMatrix(permVector);
}

//*****************************************************************************
// Initialize the permutationMatrix by using th permVector, which holds the 
// Number of permutations for each segment.
//*****************************************************************************
void SelfJoinHandler::initPermutationMatrix(UInt32* permVector)
{
  // The number of times to duplicate each value.
  // This starts with 1 for the last row, and is multiplied by the number
  // of permutations of each row.
  UInt32 dups=1;
  // Starting with the last row and going backwards.
  for (Int32 row=permMatrix_->getRows()-1; row>=0; row--)
  {
    // Each row is divided into sets, where each set is made of the numbers
    // 0...perms-1, and each number is duplicated dups times.
    // There are setReps identical sets per row.
    UInt32 perms = permVector[row];
    UInt32 setSize = perms*dups;
    UInt32 setReps = permMatrix_->getCols()/setSize;
    // For each set repetition
    for (UInt32 rep=0; rep<setReps; rep++)
    {
      UInt32 setStart = rep*setSize;
      // Inside each set we go over the numbers from 0 to perms-1
      for (UInt32 num=0; num<perms; num++)
      {
        UInt32 dupStart = setStart+num*dups;
        // And duplicate each number as needed.
        for (UInt32 col=0; col<dups; col++)
          permMatrix_->setElement(row, dupStart+col, num);
      }
    }

    dups *= perms;
  }
}

//*****************************************************************************
// Generate and fetch the next ShiftVector.
//*****************************************************************************
void SelfJoinHandler::getNextShiftVector(Int32* shiftVector)
{
  // Increment to the next row of the permutationMatrix.
  // We are skipping row 0 because its all zeroes which means its our base hash key.
  currentPermutation_++;
  // For each segment
  for (UInt32 segmentNo=0; segmentNo<segmentArray_.entries(); segmentNo++)
  {
    SelfJoinSegmentPtr segment = segmentArray_[segmentNo];
    if (segment->isSelfJoinSegment())
    {
      // This is a SelfJoin segment, so get the values from the corresponding ShiftMatrix
      // For each table ih the segment
      for (CollIndex tableNo=segment->getStart(); tableNo<=segment->getEnd(); tableNo++)
      {
        // Get the ShiftMatrix row number from the permutationMatrix.
        UInt32 row = permMatrix_->getElement(segment->getMatrixIndex(), currentPermutation_);
        // The ShiftMatrix column number is the offset into the segment.
        UInt32 col = tableNo - segment->getStart();
        // Fetch the ShiftMatrix value and use it in the ShiftVector.
        shiftVector[tableNo] = segment->getShiftMatrix()->getElement(row, col);
      }
    }
    else
    {
      // This is a Unique table segment, so just fill it up with zeroes.
      for (CollIndex i=segment->getStart(); i<=segment->getEnd(); i++)
        shiftVector[i] = 0;
    }
  }
}

//*****************************************************************************
// How many permutation do we need to generate for this self-join top graph?
// Its the multipication product of the size of all the segment's number
// of permutations.
//*****************************************************************************
Int32 SelfJoinHandler::howmanyPermutations()
{
  Int32 perms = 1;
  CollIndex maxEntries = segmentArray_.entries();
  for (CollIndex i=0; i<maxEntries; i++)
  {
    SelfJoinSegmentPtr seg = segmentArray_[i];
    perms *= seg->getPermutations();
  }

  return perms;
}

//*****************************************************************************
//*****************************************************************************
void SelfJoinHandler::dump(NAString& text)
{
  char buffer[100];

  text += "Self-Join segments: ";
  CollIndex maxEntries = segmentArray_.entries();
  for (CollIndex i=0; i<maxEntries; i++)
  {
    SelfJoinSegmentPtr seg = segmentArray_[i];
    sprintf(buffer, 
            "[%c, %d - %d] ", 
            seg->isSelfJoinSegment() ? 'S' : 'U',
            seg->getStart(),
            seg->getEnd());
    text += buffer;
  }

  text += "\nPermutationMatrix:\n";
  permMatrix_->dump(text);
}
