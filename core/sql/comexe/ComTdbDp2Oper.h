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
* File:         ComTdbDp2Oper.h
* Description:  
*
* Created:      5/6/98
* Language:     C++
*
*
*
*
****************************************************************************
*/

#ifndef COM_DP2_OPER_H
#define COM_DP2_OPER_H

#include "Platform.h"
#include "ComTdb.h"

///////////////////////////////////////////////////////
// class ComTdbDp2Oper
///////////////////////////////////////////////////////
#pragma nowarn(1506)   // warning elimination 
#pragma nowarn(1103)   // warning elimination 
#pragma nowarn(161)  // warning elimination 
class ComTdbDp2Oper : public ComTdb
{

public:
  enum SqlTableType 
  { KEY_SEQ_, KEY_SEQ_WITH_SYSKEY_, ENTRY_SEQ_, RELATIVE_, NOOP_ };
  
};


#pragma warn(161)  // warning elimination 
#pragma warn(1506)  // warning elimination 
#pragma warn(1103)  // warning elimination 

#endif

