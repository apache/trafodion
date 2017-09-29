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
****************************************************************************
*
* File:         ComTdbFirstN.cpp
* Description:  
*
* Created:      5/2/2003
* Language:     C++
*
*
*
*
****************************************************************************
*/

#include "ComTdbFirstN.h"
#include "ComTdbCommon.h"
#include "ComQueue.h"
#include "str.h"

///////////////////////////////////////////////////////////////////////////////
//
//  TDB procedures
////////////////////////////////////////////////////////////////////////

ComTdbFirstN::ComTdbFirstN()
:ComTdb(ComTdb::ex_FIRST_N, eye_FIRST_N), 
 tdbChild_(NULL), 
 firstNRows_(0)
{
}

 ComTdbFirstN::ComTdbFirstN(
      ComTdb * child_tdb,
      Int64 firstNRows,
      ex_expr * firstNRowsExpr,
      ex_cri_desc * workCriDesc,
      ex_cri_desc * givenCriDesc,
      ex_cri_desc * returnedCriDesc,
      queue_index down,
      queue_index up,
      Lng32 numBuffers,
      ULng32 bufferSize)
      : ComTdb(ComTdb::ex_FIRST_N,
	       eye_FIRST_N,
	       0,
	       givenCriDesc,
	       returnedCriDesc,
	       down,
	       up,
	       numBuffers,
	       bufferSize),
	tdbChild_(child_tdb),
	firstNRows_(firstNRows),
        firstNRowsExpr_(firstNRowsExpr),
        workCriDesc_(workCriDesc)
{
}

ComTdbFirstN::~ComTdbFirstN()
{
}

void ComTdbFirstN::display() const {};

Long ComTdbFirstN::pack(void * space)
{
  tdbChild_.pack(space);
  firstNRowsExpr_.pack(space);
  workCriDesc_.pack(space);
 
  return ComTdb::pack(space);
}

Lng32 ComTdbFirstN::unpack(void * base, void * reallocator)
{
  if(tdbChild_.unpack(base, reallocator)) return -1;
  if(firstNRowsExpr_.unpack(base, reallocator)) return -1;
  if(workCriDesc_.unpack(base, reallocator)) return -1;

  return ComTdb::unpack(base, reallocator);
}

void ComTdbFirstN::displayContents(Space * space,ULng32 flag)
{
  ComTdb::displayContents(space,flag & 0xFFFFFFFE);
  
  if (flag & 0x00000008)
    {
      char buf[100];
      str_sprintf(buf, "\nFor ComTdbFirstN :");
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
      
      str_sprintf(buf,"firstNRows = %ld", firstNRows_);
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
    }
  
  if (flag & 0x00000001)
    {
      displayExpression(space,flag);
      displayChildren(space,flag);
    }
}

