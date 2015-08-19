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
#ifndef EX_FRAG_INST_H
#define EX_FRAG_INST_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ex_frag_inst.h
 * Description:  Identifiers for fragments and fragment instances
 *               
 *               
 * Created:      1/22/96
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

// -----------------------------------------------------------------------

#include "Ipc.h"
#include "FragDir.h"
#include "NAVersionedObject.h"

// -----------------------------------------------------------------------
// Contents of this file
// -----------------------------------------------------------------------

class ExFragKey;
// typedef ExFragInstanceHandle;
// typedef ExEspStatementHandle;

// -----------------------------------------------------------------------
// A fragment instance handle is a unique identifier for a downloaded
// fragment instance within its ESP.
// -----------------------------------------------------------------------
#include "FragInstanceHandle.h"
// typedef CollIndex ExFragInstanceHandle;
const ExFragInstanceHandle NullFragInstanceHandle = NULL_COLL_INDEX;

// -----------------------------------------------------------------------
// A statement handle identifies a statement in a given process
// -----------------------------------------------------------------------
typedef ULng32 ExEspStatementHandle;

// -----------------------------------------------------------------------
// A fragment key is a unique identifier for a fragment of a particular
// statement in a particular process. Given to an ESP, the ESP can find
// the downloaded fragment instance. This object is copied into messages.
// A fragment key consists of
// - the process id of the master executor
// - the statement handle of the master executor
// - the fragment id 
// Together, those three values uniquely identify one fragment of one
// statement in one executing SQL program. Note that multiple instances
// of this fragment may exist in multiple ESPs, but each ESP has at most
// one instance.
// -----------------------------------------------------------------------
class ExFragKey : public IpcMessageObj
{
public:

  ExFragKey();
  ExFragKey(IpcProcessId          pid,
	    ExEspStatementHandle  statementHandle,
	    ExFragId              fragId);
  ExFragKey(const ExFragKey &other);
  NABoolean operator == (const ExFragKey &other);

  inline const IpcProcessId &getProcessId() const         { return pid_; }
  inline ExEspStatementHandle getStatementHandle() const
                                              { return statementHandle_; }
  inline ExFragId getFragId() const                    { return fragId_; }
  inline void setFragId(ExFragId fid)                   { fragId_ = fid; }

  IpcMessageObjSize packedLength();

private:

  IpcProcessId          pid_;
  ExEspStatementHandle  statementHandle_;
  ExFragId              fragId_;
  Int32                 spare1_;
  Int32                 spare2_;
};


#endif /* EX_FRAG_INST_H */

