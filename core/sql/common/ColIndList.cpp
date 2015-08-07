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
******************************************************************************
*
* File:         ColIndList.cpp
* Description:  This file is the implementation of ColIndList, a list of longs
*		representing a list of column numbers.
* Created:      01/20/2002
* Language:     C++
*
*
*
*
******************************************************************************
*/

#include "ColIndList.h"

//*
//*-------	isPrefixOf
//*---------------------------------------------------
// Checks is this is a prefix of other, in any order.
NABoolean 
ColIndList::isPrefixOf(const ColIndList& other) const
{
  size_t mySize = entries();
  size_t otherSize = other.entries();
  
  if (mySize > otherSize) {
    return FALSE;
  }
  
  for (size_t i = 0; i < mySize; i++) {
    if (!contains(other[i] )) {
      return FALSE;
    }
  }
  
  return TRUE;
}

//*
//*-------	isOrderedPrefixOf
//*---------------------------------------------------
// Checks is this is a prefix of other, in the same order.
NABoolean
ColIndList::isOrderedPrefixOf(const ColIndList& other) const
{
  size_t mySize = entries();
  size_t otherSize = other.entries();
  
  if (mySize > otherSize) {
    return FALSE;
  }
  
  for (size_t i = 0; i < mySize; i++) {
    if (this->at(i) != other[i] ) {
      return FALSE;
    }
  }
  
  return TRUE;
}
