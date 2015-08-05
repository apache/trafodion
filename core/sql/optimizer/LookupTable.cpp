/* -*-C++-*-
**************************************************************************
*
* File:         LookupTable.cpp
* Description:  Lookup table template class 
*               i.e. a class that implements a 2-dimensional array
* Created:      09/07/00
* Language:     C++
*
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
*
**************************************************************************
*/

// ***********************************************************************
// LookupTable
// ***********************************************************************

// default constructor
template<class T> LookupTable<T>::LookupTable()
  : numRows_(0),
    numCols_(0),
    heap_(NULL),
    arr_(NULL)
{
}

//constructor
template<class T> LookupTable<T>::LookupTable(Int32 numRows,
                                              Int32 numCols,
                                              CollHeap* heap)
  : numRows_(numRows),
    numCols_(numCols),
    heap_(heap)
{
  DCMPASSERT(numRows > 0);
  DCMPASSERT(numCols > 0);
  arr_ = new (heap_) T[numRows*numCols];
}

//accessor
template<class T> const T& LookupTable<T>::getValue(Int32 rowNum,
                                                   Int32 colNum) const
{
  DCMPASSERT((rowNum >= 0) AND (rowNum < numRows_) AND
             (colNum >= 0) AND (colNum < numCols_));

  Lng32 index = (rowNum * numCols_) + colNum;

  DCMPASSERT(arr_ != NULL);

  return arr_[index];

}


//mutator
template<class T> void LookupTable<T>::setValue(Int32 rowNum,
                                                Int32 colNum,
                                                const T& value)
{
  CMPASSERT((rowNum >= 0) AND (rowNum < numRows_) AND
            (colNum >= 0) AND (colNum < numCols_));

  Lng32 index = (rowNum * numCols_) + colNum;

  DCMPASSERT(arr_ != NULL);

  arr_[index] = value;

}

//copy constructor
template<class T> LookupTable<T>::LookupTable(const LookupTable& other,
                                              CollHeap* heap)
  : numRows_(other.numRows_),
    numCols_(other.numCols_),
    heap_( (heap == NULL) ? other.heap_ : heap)
{
  arr_ = new (heap_) T[numRows_ * numCols_];
  Int32 x,y;
  Lng32 index;
  for (x=0;x < numRows_;x++)
  {
    for (y=0;y < numCols_;y++)
    {
      index = (x * numCols_) + y;
      arr_[index] = other.arr_[index];
    }
  }
}

//equal operator
template<class T> LookupTable<T>& LookupTable<T>::operator=(
                                             const LookupTable& other)
{
  // Test for assignment to itself
  if (this == &other) return *this;

  if (arr_ != NULL)
    delete [] arr_; // free up existing memory

  numRows_ = other.numRows_;
  numCols_ = other.numCols_;

  arr_ = new (heap_) T[numRows_ * numCols_];
  Int32 x,y;
  Lng32 index;
  for (x=0;x < numRows_;x++)
  {
    for (y=0;y < numCols_;y++)
    {
      index = (x * numCols_) + y;
      arr_[index] = other.arr_[index];
    }
  }

  return *this;
}

//destructor
template<class T> LookupTable<T>::~LookupTable()
{
  if (arr_ != NULL)
  {
    delete [] arr_;
    arr_ = NULL;
  }
}




