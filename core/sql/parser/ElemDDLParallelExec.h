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
#ifndef ELEMDDLPARALLELEXEC_H
#define ELEMDDLPARALLELEXEC_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ElemDDLParallelExec.h
 * Description:  class for the parallel execution clause specified in
 *               DDL statements associating with INDEX.
 *
 *               
 * Created:      9/18/95
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */


#include "ComASSERT.h"
#include "ComOperators.h"
#include "ElemDDLNode.h"
#include "NABoolean.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class ElemDDLParallelExec;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
// None.

// -----------------------------------------------------------------------
// definition of class ElemDDLParallelExec
// -----------------------------------------------------------------------
class ElemDDLParallelExec : public ElemDDLNode
{

public:

  // constructors
  ElemDDLParallelExec(NABoolean parallelExecSpec)
  : ElemDDLNode(ELM_PARALLEL_EXEC_ELEM),
  parallelExecSpec_(parallelExecSpec),
  configFileName_(PARSERHEAP())
  { }

  ElemDDLParallelExec(NABoolean parallelExecSpec,
                             const NAString & configFileName)
  : ElemDDLNode(ELM_PARALLEL_EXEC_ELEM),
  parallelExecSpec_(parallelExecSpec),
  configFileName_(configFileName, PARSERHEAP())
  {
    ComASSERT(parallelExecSpec_ EQU TRUE);
  }


  // virtual destructor
  virtual ~ElemDDLParallelExec();

  // cast
  virtual ElemDDLParallelExec * castToElemDDLParallelExec();

  // accessors
  inline NAString getConfigFileName() const;

  // is parallel execution enabled?
  inline NABoolean isParallelExecEnabled() const;

  // methods for tracing
  virtual const NAString displayLabel1() const;
  virtual const NAString displayLabel2() const;
  virtual const NAString getText() const;


private:

  NABoolean parallelExecSpec_;
  NAString configFileName_;

}; // class ElemDDLParallelExec

// -----------------------------------------------------------------------
// definitions of inline methods for class ElemDDLParallelExec
// -----------------------------------------------------------------------


inline NAString
ElemDDLParallelExec::getConfigFileName() const
{
  return configFileName_;
}

inline NABoolean
ElemDDLParallelExec::isParallelExecEnabled() const
{
  return parallelExecSpec_;
}

#endif // ELEMDDLPARALLELEXEC_H
