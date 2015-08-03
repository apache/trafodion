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
 * File:         ex_frag_inst.C
 * Description:  Identifiers for fragments and fragment instances
 *               
 *               
 * Created:      1/24/96
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

// -----------------------------------------------------------------------

#include "ExCollections.h"
#include "Ipc.h"
#include "ex_frag_inst.h"
#include "Ex_esp_msg.h"

ExFragKey::ExFragKey() :
     IpcMessageObj(ESP_FRAGMENT_KEY,CurrFragmentKeyVersion),
     statementHandle_(0),
     fragId_(0),
     spare1_(0),
     spare2_(0)
{
}

ExFragKey::ExFragKey(IpcProcessId          pid,
		     ExEspStatementHandle  statementHandle,
		     ExFragId              fragId) :
     IpcMessageObj(ESP_FRAGMENT_KEY,CurrFragmentKeyVersion),
     pid_(pid),
     statementHandle_(statementHandle),
     fragId_(fragId),
     spare1_(0),
     spare2_(0)
{
}

ExFragKey::ExFragKey(const ExFragKey &other) :
     IpcMessageObj(ESP_FRAGMENT_KEY,CurrFragmentKeyVersion),
     pid_(other.pid_),
     statementHandle_(other.statementHandle_),
     fragId_(other.fragId_),
     spare1_(0),
     spare2_(0)
{
}

NABoolean ExFragKey::operator == (const ExFragKey &other)
{
  return (other.fragId_          == fragId_ AND
	  other.statementHandle_ == statementHandle_ AND
	  other.pid_             == pid_);
}

IpcMessageObjSize ExFragKey::packedLength()
{
  return sizeof(ExFragKey);
}

