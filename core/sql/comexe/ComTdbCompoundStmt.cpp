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
* File:         ComTdbCompoundStmt.cpp 
* Description:  3GL compound statement (CS) operator.
*	
* Created:      4/1/98 
* Language:     C++
*
*
*
******************************************************************************
*/

#include "ComTdb.h"
#include "ComTdbCompoundStmt.h"
#include "ComTdbCommon.h"

//////////////////////////////////////////////////////////////////////////////
//
// CompoundStmt TDB methods.
//
//////////////////////////////////////////////////////////////////////////////

ComTdbCompoundStmt::ComTdbCompoundStmt() : 
  ComTdb(ComTdb::ex_COMPOUND_STMT, eye_CS)
{}


ComTdbCompoundStmt::ComTdbCompoundStmt(ComTdb *left,
                                       ComTdb *right,
                                       ex_cri_desc *given,
                                       ex_cri_desc *returned,
                                       queue_index down,
                                       queue_index up,
              			       Lng32 numBuffers,
	            		       ULng32 bufferSize,
                                       NABoolean rowsFromLeft,
                                       NABoolean rowsFromRight,
                                       NABoolean afterUpdate) :
 ComTdb(ComTdb::ex_COMPOUND_STMT, eye_CS,
	(Cardinality) 0.0,
         given, returned,
         down, up,
	 numBuffers, bufferSize),
  tdbLeft_(left),
  tdbRight_(right),
  flags_(0)
{
  if (rowsFromLeft)
    flags_ |= ComTdbCompoundStmt::ROWS_FROM_LEFT;
  if (rowsFromRight)
    flags_ |= ComTdbCompoundStmt::ROWS_FROM_RIGHT;
  if (afterUpdate)
    flags_ |= ComTdbCompoundStmt::AFTER_UPDATE;
}


Long ComTdbCompoundStmt::pack(void * space)
{
  tdbLeft_.pack(space);
  tdbRight_.pack(space);

  return ComTdb::pack(space);

} // ComTdbCompoundStmt::pack


Lng32 ComTdbCompoundStmt::unpack(void * base, void * reallocator)
{
  if(tdbLeft_.unpack(base, reallocator)) return -1;
  if(tdbRight_.unpack(base, reallocator)) return -1;

  return ComTdb::unpack(base, reallocator);

} // ComTdbCompoundStmt::unpack


// exclude from code coverage analysis sind it is used only by GUI
inline const ComTdb* ComTdbCompoundStmt::getChild(Int32 pos) const
{
  if (pos == 0)
    return tdbLeft_;
  else if (pos == 1)
    return tdbRight_;
  else
    return NULL;

} // ComTdbCompoundStmt::getChild

