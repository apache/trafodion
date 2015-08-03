/* -*-C++-*-
****************************************************************************
*
* File:         NAExecTrans.h
* Description:  
*
* Created:      5/6/98
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
****************************************************************************
*/

#ifndef NAEXECTRANS_H
#define NAEXECTRANS_H

#include "BaseTypes.h"
#include "Int64.h"

// -----------------------------------------------------------------------
// some transaction related routines, called by both executor and arkcmp
// -----------------------------------------------------------------------

// if command is 0, it is to get the status of current transaction
//   return TRUE, if there is a transaction, transId will hold transaction Id
//   return FALSE, if there is no transaction
// Other values for command are not supported at this time.
NABoolean NAExecTrans(Lng32 command,   // 0 to get transId
                      Int64& transId); // IN/OUT
#endif

