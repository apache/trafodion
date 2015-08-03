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
* File:         $File$
* RCS:          $Id$
* Description:  
* Created:      
* Language:     C++
* Status:       $State$
*
*
*
*
******************************************************************************
*/

#include "ComTdbSample.h"
#include "ComTdbCommon.h"

//////////////////////////////////////////////////////////////////////////////////
//
//  TDB procedures
//
//////////////////////////////////////////////////////////////////////////////////

// Constructor
ComTdbSample::ComTdbSample()
: ComTdb(ComTdb::ex_SAMPLE, eye_SAMPLE)
{
}

ComTdbSample::ComTdbSample
(ex_expr *initExpr,
 ex_expr *balanceExpr,
 Int32 returnFactorOffset,
 ex_expr *postPred,
 ComTdb * child_tdb,
 ex_cri_desc * given_cri_desc,
 ex_cri_desc * returned_cri_desc,
 queue_index down,
 queue_index up)

: ComTdb(ComTdb::ex_SAMPLE, eye_SAMPLE,
	 (Cardinality) 0.0,
         given_cri_desc, returned_cri_desc,
	 down, up,
	 0, 0),
  initExpr_(initExpr),
  balanceExpr_(balanceExpr),
  returnFactorOffset_(returnFactorOffset),
  postPred_(postPred),
  tdbChild_(child_tdb)
{
}

ComTdbSample::~ComTdbSample(){
}

void ComTdbSample::display() const {};

Int32 ComTdbSample::orderedQueueProtocol() const 
{
  return -1;
}

Long ComTdbSample::pack(void * space)
{
  tdbChild_.pack(space);
  initExpr_.pack(space);
  balanceExpr_.pack(space);
  postPred_.pack(space);

  return ComTdb::pack(space);
}

Lng32 ComTdbSample::unpack(void * base, void * reallocator)
{
  if (tdbChild_.unpack(base, reallocator)) return -1;
  if (initExpr_.unpack(base, reallocator)) return -1;
  if (balanceExpr_.unpack(base, reallocator)) return -1;
  if (postPred_.unpack(base, reallocator)) return -1;

  return ComTdb::unpack(base, reallocator);
}

  
