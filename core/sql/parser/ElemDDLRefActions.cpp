/* -*-C++-*-
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
 *****************************************************************************
 *
 * File:         ElemDDLRefActions.C
 * Description:  methods for class ElemDDLRefAct and any classes
 *               derived from class ElemDDLRefAct.
 *
 * Created:      10/16/95
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

#include "ElemDDLRefActions.h"

// -----------------------------------------------------------------------
// methods for class ElemDDLRefAct
// -----------------------------------------------------------------------

// virtual destructor
ElemDDLRefAct::~ElemDDLRefAct()
{
}

// casting
ElemDDLRefAct *
ElemDDLRefAct::castToElemDDLRefAct()
{
  return this;
}

//
// methods for tracing
//

NATraceList
ElemDDLRefAct::getDetailInfo() const
{
  NATraceList detailTextList;

  //
  // Note that the invoked displayLabel1() is a method of
  // class ElemDDLRefActDelete or ElemDDLRefActSelect
  //
  detailTextList.append(displayLabel1());

  return detailTextList;
}

const NAString
ElemDDLRefAct::getText() const
{
  ABORT("internal logic error");
  return "ElemDDLRefAct";
}

// -----------------------------------------------------------------------
// methods for class ElemDDLRefActCascade
// -----------------------------------------------------------------------

// virtual destructor
ElemDDLRefActCascade::~ElemDDLRefActCascade()
{
}

// casting
ElemDDLRefActCascade *
ElemDDLRefActCascade::castToElemDDLRefActCascade()
{
  return this;
}

//
// methods for tracing
//

const NAString
ElemDDLRefActCascade::displayLabel1() const
{
  return NAString("Cascade referential action");
}

const NAString
ElemDDLRefActCascade::getText() const
{
  return "ElemDDLRefActCascade";
}

// -----------------------------------------------------------------------
// methods for class ElemDDLRefActNoAction
// -----------------------------------------------------------------------

// virtual destructor
ElemDDLRefActNoAction::~ElemDDLRefActNoAction()
{
}

// casting
ElemDDLRefActNoAction *
ElemDDLRefActNoAction::castToElemDDLRefActNoAction()
{
  return this;
}

//
// methods for tracing
//

const NAString
ElemDDLRefActNoAction::displayLabel1() const
{
  return NAString("No referential action");
}

const NAString
ElemDDLRefActNoAction::getText() const
{
  return "ElemDDLRefActNoAction";
}

// -----------------------------------------------------------------------
// methods for class ElemDDLRefActRestrict
// -----------------------------------------------------------------------

// virtual destructor
ElemDDLRefActRestrict::~ElemDDLRefActRestrict()
{
}

// casting
ElemDDLRefActRestrict *
ElemDDLRefActRestrict::castToElemDDLRefActRestrict()
{
  return this;
}

//
// methods for tracing
//

const NAString
ElemDDLRefActRestrict::displayLabel1() const
{
  return NAString("Restrict referential action");
}

const NAString
ElemDDLRefActRestrict::getText() const
{
  return "ElemDDLRefActRestrict";
}


// -----------------------------------------------------------------------
// methods for class ElemDDLRefActSetDefault
// -----------------------------------------------------------------------

// virtual destructor
ElemDDLRefActSetDefault::~ElemDDLRefActSetDefault()
{
}

// casting
ElemDDLRefActSetDefault *
ElemDDLRefActSetDefault::castToElemDDLRefActSetDefault()
{
  return this;
}

//
// methods for tracing
//

const NAString
ElemDDLRefActSetDefault::displayLabel1() const
{
  return NAString("Set Default referential action");
}

const NAString
ElemDDLRefActSetDefault::getText() const
{
  return "ElemDDLRefActSetDefault";
}

// -----------------------------------------------------------------------
// methods for class ElemDDLRefActSetNull
// -----------------------------------------------------------------------

// virtual destructor
ElemDDLRefActSetNull::~ElemDDLRefActSetNull()
{
}

// casting
ElemDDLRefActSetNull *
ElemDDLRefActSetNull::castToElemDDLRefActSetNull()
{
  return this;
}

//
// methods for tracing
//

const NAString
ElemDDLRefActSetNull::displayLabel1() const
{
  return NAString("Set Null referential action");
}

const NAString
ElemDDLRefActSetNull::getText() const
{
  return "ElemDDLRefActSetNull";
}

//
// End of File
//
