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
#ifndef ELEMDDLWITHCHECKOPTION_H
#define ELEMDDLWITHCHECKOPTION_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ElemDDLWithCheckOption.h
 * Description:  class respresenting the With Check Option clause
 *               in Create View statement
 *
 *               
 * Created:      1/12/96
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */


#include "ComSmallDefs.h"
#include "ElemDDLNode.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class ElemDDLWithCheckOption;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
// None

// -----------------------------------------------------------------------
// definition of base class ElemDDLWithCheckOption
// -----------------------------------------------------------------------
class ElemDDLWithCheckOption : public ElemDDLNode
{

public:

  // constructor
  ElemDDLWithCheckOption(ComLevels level)
  : ElemDDLNode(ELM_WITH_CHECK_OPTION_ELEM),
  level_(level)
  { }


  // virtual destructor
  virtual ~ElemDDLWithCheckOption();

  // cast
  virtual ElemDDLWithCheckOption * castToElemDDLWithCheckOption();

  // accessors
  inline ComLevels getLevel() const;
  NAString getLevelAsNAString() const;

  // methods for tracing
  virtual const NAString displayLabel1() const;  // display level info
  virtual const NAString getText() const;

private:

  ComLevels level_;

}; // class ElemDDLWithCheckOption

// -----------------------------------------------------------------------
// definitions of inline methods for class ElemDDLWithCheckOption
// -----------------------------------------------------------------------

//
// accessor
//

inline ComLevels
ElemDDLWithCheckOption::getLevel() const
{
  return level_;
}

#endif // ELEMDDLWITHCHECKOPTION_H
