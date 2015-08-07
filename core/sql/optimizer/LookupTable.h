#ifndef LOOKUPTABLE_H
#define LOOKUPTABLE_H
/* -*-C++-*-
*************************************************************************
*
* File:         LookupTable.h
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
*************************************************************************
*/

// ----------------------------------------------------------------------
// contents of this file
// ----------------------------------------------------------------------
template<class T> class LookupTable;

template<class T>
class LookupTable : public NABasicObject
{
public:
  LookupTable();
  LookupTable(Int32 numRows, 
              Int32 numCols, 
              CollHeap* heap=0); // constructor
  LookupTable(const LookupTable& other, CollHeap* heap=0); // copy constructor
  
  virtual ~LookupTable();  //destructor

  LookupTable& operator=(const LookupTable& other);

  const T& getValue(Int32 rowNum, Int32 colNum) const;
  T& operator[](Int32 elem);
  void setValue(Int32 rowNum, Int32 colNum, const T& value);

private:
  Int32 numRows_;
  Int32 numCols_;
  CollHeap *heap_;
  T *arr_;

}; //LookupTable

// -----------------------------------------------------------------------
// This is done similarly to Tools.h++: if we want to instantiate
// templates at compile time, the compiler needs to know the
// implementation of the template functions. Do this by setting the
// preprocessor define NA_COMPILE_INSTANTIATE.
// -----------------------------------------------------------------------
#ifdef NA_COMPILE_INSTANTIATE
#include "LookupTable.cpp"
#endif

#endif /* LOOKUPTABLE_H */
