#ifndef _DISJUNCT_H
#define _DISJUNCT_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         disjunct.h
 * Description:  Handling of a single disjunct. A Disjunct object represents
 *               the conjunction of a set of predicates
 *
 * Code location: mdam.C
 *               
 * Created:      //96
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
 *****************************************************************************
 */

// -----------------------------------------------------------------------


// -----------------------------------------------------------------------
// Class Disjunct
// -----------------------------------------------------------------------
#include "Collections.h"
#include "NABasicObject.h"

class Disjunct : public NABasicObject
{
public:
  virtual ~Disjunct()
  {}

  // Mutators:
  virtual void clear()
  { disjunct_.clear(); }

  virtual void remove(const ValueId& valueId)
  { disjunct_.remove(valueId); }
       
  virtual void insertSet(const ValueIdSet& idSet)
  { disjunct_.insert(idSet); }

  virtual void intersectSet(const ValueIdSet& idSet)
  { disjunct_.intersectSet(idSet); }

  // const functions:
  NABoolean isEmpty() const {return disjunct_.isEmpty(); }

  virtual void print( FILE* ofd = stdout,
		    const char* indent = DEFAULT_INDENT,
		    const char* title = "disjunct") const
  { disjunct_.print(ofd,indent,title); }

  const ValueIdSet& getAsValueIdSet() const
  { return disjunct_; }

private:
  ValueIdSet disjunct_;


};
#endif
// eof 


