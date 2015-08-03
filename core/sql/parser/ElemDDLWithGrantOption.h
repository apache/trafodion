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
#ifndef ELEMDDLWITHGRANTOPTION_H
#define ELEMDDLWITHGRANTOPTION_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ElemDDLWithGrantOption.h
 * Description:  class respresenting the With Grant Option
 *               clause/phrase in Grant DDL statement
 *
 *               
 * Created:      10/6/95
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */


#include "ElemDDLNode.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class ElemDDLWithGrantOption;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
// None

// -----------------------------------------------------------------------
// definition of base class ElemDDLWithGrantOption
// -----------------------------------------------------------------------
class ElemDDLWithGrantOption : public ElemDDLNode
{

public:

  // constructor
  ElemDDLWithGrantOption()
  : ElemDDLNode(ELM_WITH_GRANT_OPTION_ELEM)
  { }

  // virtual destructor
  virtual ~ElemDDLWithGrantOption();

  // cast
  virtual ElemDDLWithGrantOption * castToElemDDLWithGrantOption();

  // method for tracing
  virtual const NAString getText() const;

private:

}; // class ElemDDLWithGrantOption


#endif // ELEMDDLWITHGRANTOPTION_H
