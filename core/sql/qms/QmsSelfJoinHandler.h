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
// File:         QmsSelfJoinHandler.h
// Description:  Generation of equivalent MVMemo hash keys for self-join MVs.
//               The theoretical background for this code is described in 
//               section 3.8.3.4.2.5 (Disambiguating tables involved in a self-join) 
//               of the Internal Spec of MVQR.
//               
// Created:      09/12/2011
// ***********************************************************************

#include "NABasicObject.h"
#include "NAString.h"
#include "Collections.h"
#include "Int64.h"
#include "QRSharedPtr.h"
#include "QRDescriptor.h"

class Array2D;
class PermutationMatrix;
class ShiftMatrix;
class ShiftMatrixFactory;
class SelfJoinSegment;
class SelfJoinHandler;

#ifdef _MEMSHAREDPTR
typedef QRIntrusiveSharedPtr<Array2D>   	        Array2DPtr;
typedef QRIntrusiveSharedPtr<PermutationMatrix>	        PermutationMatrixPtr;
typedef QRIntrusiveSharedPtr<ShiftMatrix>	        ShiftMatrixPtr;
typedef QRIntrusiveSharedPtr<ShiftMatrixFactory>	ShiftMatrixFactoryPtr;
typedef QRIntrusiveSharedPtr<SelfJoinSegment>	        SelfJoinSegmentPtr;
typedef QRIntrusiveSharedPtr<SelfJoinHandler>	        SelfJoinHandlerPtr;
#else
typedef Array2D*				        Array2DPtr;
typedef PermutationMatrix*				PermutationMatrixPtr;
typedef ShiftMatrix*				        ShiftMatrixPtr;
typedef ShiftMatrixFactory*		                ShiftMatrixFactoryPtr;
typedef SelfJoinSegment*		                SelfJoinSegmentPtr;
typedef SelfJoinHandler*		                SelfJoinHandlerPtr;
#endif

typedef NAPtrArray<SelfJoinSegmentPtr>                  SelfJoinSegmentArray;

#ifndef _SELFJOIN_H_
#define _SELFJOIN_H_

#include "QmsJoinGraph.h"

/**
 * Array2D is an encapsulation of a fixed two-dimentional array of Int32 elements.
 *****************************************************************************
 */
class Array2D : public NAIntrusiveSharedPtrObject
{
public:
  /**
   * The constructor takes the array size and optional initial value.
   * @param rows Number of rows
   * @param cols Number of Columns
   * @param initValue Initial value (default is 0).
   * @param heap Heap pointer.
   */
  Array2D(UInt32 rows, UInt32 cols, Int32 initValue=0,
	  ADD_MEMCHECK_ARGS_DECL(CollHeap* heap=NULL));

  virtual ~Array2D();

  /**
   * Get the array hight.
   * @return 
   */
  UInt32 getRows()
  {
    return rows_;
  }

  /**
   * Get the array width.
   * @return 
   */
  UInt32 getCols()
  {
    return cols_;
  }

  typedef Int32* Row;

  /**
   * Get an array element.
   * @param row The row.
   * @param col The column.
   * @return The value.
   */
  Int32 getElement(UInt32 row, UInt32 col);

  /**
   * Set an array element.
   * @param row The row.
   * @param col The column.
   * @param value The value.
   */
  void setElement(UInt32 row, UInt32 col, Int32 value);

  /**
   * Return a textual representation of the Array2D.
   * @param text 
   */
  void dump(NAString& text);

private:
  UInt32    rows_;
  UInt32    cols_;
  Int32**   array_;
  CollHeap* heap_;
};

/**
 * A ShiftMatrix provides all the possible permutations for arranging some values.\n
 * Example: \par <tt> 
 * \verbatim
   ShiftMatrix for size 2:
     0,  0
     1, -1
  
   ShiftMatrix for size 3:
     0,  0,  0
     0,  1, -1
     1, -1,  0
     1,  1, -2
     2, -1, -1
     2,  0, -2
   \endverbatim 
 * </tt> \par
 * 
 * Each row is one permutation, so if we take 2 values, we can either
 * leave them where they are (0, 0) or switch them by moving the first 1 
 * step forward, and the other 1 step back (1, -1).\par
 * 
 * For each size there is one ShiftMatrix that cannot be changed.\n
 * For every ShiftMatrix of size n, there are n! combinations.
 *****************************************************************************
 */
class ShiftMatrix  : public NAIntrusiveSharedPtrObject
{
public:
  /**
   * Create and initialize a ShiftMatrix of size size.
   * @param size The number of elements to rearrange.
   * @param heap 
   */
  ShiftMatrix(Int32  size,
	      ADD_MEMCHECK_ARGS_DEF(CollHeap* heap));

  virtual ~ShiftMatrix();

  /**
   * Get a specific shift value.
   * @param combination The combination number.
   * @param element The element number within the combination.
   * @return theh shift value.
   */
  Int32 getElement(UInt32 combination, UInt32 element);

  /**
   * Return a textual representation of the ShiftMatrix.
   * @param text 
   */
  void dump(NAString& text);

  /**
   * Compute the factorial of a number.
   * @param num The input value.
   * @return The factorial of the input value.
   */
  static Int32 factorial(Int32 num);

private:

  /**
   * Initialize the ShiftMatrix
   */
  void  init();

  /**
   * Recursive method to initialize a subsection of the matrix.
   * @param usedValues Bitmap of values already used.
   * @param from Starting column
   * @param to Ending column
   * @param depth Current depth.
   */
  void  initNext(NABitVector& usedValues,
                 UInt32 from,
                 UInt32 to,
                 UInt32 depth);

private:
  const UInt32      elements_;      // Number of matrix columns.
  const UInt32      combinations_;  // Number of matrix rows.
  Array2DPtr        theMatrix_;     // The matrix itself.
  CollHeap*         heap_;          // The heap pointer.
};  // class ShiftMatrix

/**
 * This is a singleton class, building ShiftMatrix objects.
 * Since ShiftMatrix objects are constant per size, we keep
 * a pointer to each one we build, and reuse it in future calls.
 *****************************************************************************
 */
class ShiftMatrixFactory : public NAIntrusiveSharedPtrObject
{
public:
  /**
   * Get the pointer to the singleton instance.
   * @param heap Heap pointer for allocation if this is the first call.
   * @return 
   */
  static ShiftMatrixFactoryPtr getInstance(NAMemory* heap);
 
  /**
   * Free resources used by all the ShiftMatrix objects constructed so far.
   */
  virtual ~ShiftMatrixFactory();

  /**
   * Get a pointer to a ShiftMatrix object of size \c size.
   * The caller must not delete the returned object.
   * @param size Required size of ShiftMatrix
   * @return 
   */
  const ShiftMatrixPtr getMatrixForSize(Int32 size);

private:
  /**
   * A private constructor used only by the getInstance() method.
   * @param heap 
   */
  ShiftMatrixFactory(ADD_MEMCHECK_ARGS_DECL(CollHeap* heap))
    : NAIntrusiveSharedPtrObject(ADD_MEMCHECK_ARGS_PASS(heap)),
      matrixSizeArray_(heap),
      heap_(heap)
  {
  }

private:
  static ShiftMatrixFactoryPtr instance_;
  NAPtrArray<ShiftMatrixPtr>   matrixSizeArray_;
  CollHeap*                    heap_;
};  // class ShiftMatrixFactory

/**
 * A structure for holding information of segments
 *****************************************************************************
 */
class SelfJoinSegment : public NAIntrusiveSharedPtrObject
{
public:
  enum SegmentType { SELF_JOIN_SEGMENT, UNIQUE_TABLE_SEGMENT };

  /**
   * Constructor
   * @param type SELF_JOIN_SEGMENT or UNIQUE_TABLE_SEGMENT.
   * @param start First table of segment
   * @param end Last table of segment
   * @param heap Heap pointer.
   */
  SelfJoinSegment(SegmentType type, UInt32 start, UInt32 end, ADD_MEMCHECK_ARGS_DECL(CollHeap* heap))
    : NAIntrusiveSharedPtrObject(ADD_MEMCHECK_ARGS_PASS(heap))
     ,type_(type)
     ,start_(start)
     ,end_(end)
     ,size_(end - start + 1)
     ,permutations_(type == UNIQUE_TABLE_SEGMENT ? 1 : ShiftMatrix::factorial(size_))
     ,matrixIndex_(0)
     ,shiftMatrix_(NULL)
  {
  }

  /**
   * Is this a SelfJoin segment?
   * @return 
   */
  NABoolean isSelfJoinSegment()
  {
    return type_ == SELF_JOIN_SEGMENT;
  }

  /**
   * Get the first table of the segment.
   * @return 
   */
  UInt32 getStart()
  {
    return start_;
  }

  /**
   * Get the last table of the segment.
   * @return 
   */
  UInt32 getEnd()
  {
    return end_;
  }

  /**
   * Get the number of tables in the segment.
   * @return 
   */
  UInt32 getSize()
  {
    return size_;
  }

  /**
   * Get the number of permutations for this segment
   * @return 
   */
  UInt32 getPermutations()
  {
    return permutations_;
  }

  /**
   * Get the column index into the permutationMatrix
   * @return 
   */
  UInt32 getMatrixIndex()
  {
    return matrixIndex_;
  }

  /**
   * Set the column index into the permutationMatrix
   * @param inx 
   */
  void setMatrixIndex(UInt32 inx)
  {
    matrixIndex_ = inx;
  }

  /**
   * Get the corresponding ShiftMatrix.
   * @return 
   */
  ShiftMatrixPtr getShiftMatrix()
  {
    return shiftMatrix_;
  }

  /**
   * Set the corresponding ShiftMatrix.
   * @param matrix 
   */
  void setShiftMatrix(ShiftMatrixPtr matrix)
  {
    shiftMatrix_ = matrix;
  }

private:
  const SegmentType  type_;         // Segment type
  const UInt32       start_;        // Index of first segment table 
  const UInt32       end_;          // Index of last segment tagble.
  const UInt32       size_;         // Number of tables in segment.
  const UInt32       permutations_; // Number of permutations for this segment
  UInt32             matrixIndex_;  // The column index into the permutationMatrix
  ShiftMatrixPtr     shiftMatrix_;  // The corresponding ShiftMatrix.
};  // class SelfJoinSegment 

/**
 * This class handles all the algorithms involved in generating equivalent 
 * MVMemo hash keys for selfJoin queries. This is done in 3 major steps:
 * 1. Detection of SelfJoin segments.
 * 2. Construction of the PermutationMatrix (including ShiftMatrix objects)
 * 3. Construction of the ShiftVector for every PermutationMatrix row.
 *****************************************************************************
 */
class SelfJoinHandler : public NAIntrusiveSharedPtrObject
{
public:
  /**
   * Default constructor
   * @param heap 
   * @return 
   */
  SelfJoinHandler(ADD_MEMCHECK_ARGS_DECL(CollHeap* heap))
    : NAIntrusiveSharedPtrObject(ADD_MEMCHECK_ARGS_PASS(heap))
     ,segmentArray_(heap)
     ,permMatrix_(NULL)
     ,state_(ST_START)
     ,lastTable_(&firstTable_)
     ,segmentStart_(-1)
     ,segmentEnd_(-1)
     ,totalPermutations_(1)
     ,currentPermutation_(0)
     ,halfHashKey_(heap)
     ,heap_(heap)
  {
  }

  virtual ~SelfJoinHandler();

  /**
   * Step 1. Add a table to the Self-Join analysis.
   * This is the main body of the segment detection state machine.
   * @param table 
   */
  void addTable(JoinGraphTablePtr table);

  /**
   * Step 1. Mark that all the tables have been added.
   * This is the final step of the segment detection state machine.
   */
  void doneAddingTables();

  /**
   * Step 2. Prepare the PermutationMatrix which is the main data structure.
   */
  void preparePermutationMatrix();

  /**
   * Step 2. Set the first (constant) half of the hAsh key.
   * @param key 
   */
  void setHalfHashKey(const NAString& key)
  {
    halfHashKey_ = key;
  }

   /**
   * Step 3. Did we get the ShiftVectors for all the PermutationMatrix rows?
   * @return 
   */
  NABoolean isDone()
  {
    return currentPermutation_ >= totalPermutations_-1;
  }

  /**
   * Step 3. Get the first (constant) half of the hAsh key.
   * @return 
   */
  const NAString& getHalfHashKey()
  {
    return halfHashKey_;
  }

  /**
   * Step 3. Get the next ShiftVector.
   * @param shiftVector 
   */
  void getNextShiftVector(Int32* shiftVector);

  /**
   * How many self-join permutations for this join graph?
   * @return 
   */
  Int32 howmanyPermutations();

  void dump(NAString& text);

protected:
  void addSegment(SelfJoinSegment::SegmentType type);
  void initPermutationMatrix(UInt32* permVector);

private:
  // Segment detection state machine states
  enum SegmentState { 
    ST_START=0, 
    ST_SINGLE,
    ST_UNIQUE,
    ST_SELFJOIN,
    ST_END 
  };

  SelfJoinSegmentArray      segmentArray_;  
  Array2DPtr                permMatrix_;
  SegmentState              state_;
  const NAString*           lastTable_;
  static const NAString     firstTable_;
  Int32                     segmentStart_;
  Int32                     segmentEnd_;

  UInt32                    totalPermutations_;
  UInt32                    currentPermutation_;
  NAString                  halfHashKey_;
  CollHeap*                 heap_;
};  // class SelfJoinHandler

#endif   // _SELFJOIN_H_
