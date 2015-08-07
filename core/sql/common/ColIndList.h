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
#ifndef COLINDLIST_H
#define COLINDLIST_H
/* -*-C++-*-
******************************************************************************
*
* File:         ColIndList.h
* Description:  This file is the implementation of ColIndList, a list of longs
*		representing a list of column numbers.
* Created:      01/20/2002
* Language:     C++
*
*
*
******************************************************************************
*/

#include <Collections.h> 

//-----------------------------------------------------------------------------
// class ColIndList
// ----------------
// This is a list of longs representing a list of column numbers. It is used
// to represent clustering indexes.
class ColIndList : public LIST(Lng32)
{
public:
  ColIndList() : LIST(Lng32)() {}
  ColIndList(LIST(Lng32) list) : LIST(Lng32)(list) {}
  virtual ~ColIndList() {}
  NABoolean isPrefixOf(const ColIndList& other) const;
  NABoolean isOrderedPrefixOf(const ColIndList& other) const;
};

#endif
