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
 *****************************************************************************
 *
 * File:         ElemDDLQualName.cpp
 * Description:  an element representing a qualified name
 *					used as a node in a list of qualified names
 *               
 * Created:      06/20/99
 * Language:     C++
 * Project:		 MV refresh groups ( OZ ) & general use
 *
 *
 *
 *****************************************************************************
 */

#include "ElemDDLQualName.h"

#ifndef   SQLPARSERGLOBALS_CONTEXT_AND_DIAGS
#define   SQLPARSERGLOBALS_CONTEXT_AND_DIAGS
#endif
#include "SqlParserGlobals.h"
  
ElemDDLQualName::ElemDDLQualName(const QualifiedName & mvGroupName)
: mvQualName_(mvGroupName, PARSERHEAP())
{

}

ElemDDLQualName::~ElemDDLQualName()
{



}

ElemDDLQualName * 
ElemDDLQualName::castToElemDDLQualName()
{
	return this;
}
