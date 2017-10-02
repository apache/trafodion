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
#ifndef TRIG_ENABLE_H
#define TRIG_ENABLE_H

/* -*-C++-*-
 *****************************************************************************
 *
 * File:         TriggerEnable.h
 * Description:  Classes and methods used by the executor for the trigger 
 *               enable/disable mechanism.
 *               
 * Created:      12/30/98
 * Language:     C++
 *
 *****************************************************************************
 */

#include "NAVersionedObject.h"
#include "ComSmallDefs.h"

// needed for the TDB member
typedef NAVersionedObjectPtrTempl<Int64> TriggersListPtr;

//-----------------------------------------------------------------------------
// classes defined in this file

class TriggerStatus;
class TriggerStatusWA;

//-----------------------------------------------------------------------------
// forward declarations

class NAHeap;
class ComTdbRoot;
class ex_root_tcb;

//-----------------------------------------------------------------------------

// each trigger is represented by a bit in the vector
const Int32 TRIGGERS_STATUS_VECTOR_SIZE	= 32;
// limitation, enforced by binder
const Int32 MAX_TRIGGERS_PER_STATEMENT	= 
									TRIGGERS_STATUS_VECTOR_SIZE * 8;

//-----------------------------------------------------------------------------
//
// -- class TriggerStatus
//
// Holds a pair (triggerId, enable/disable status).
// I rely on ComTimeStamp (aka Int64, aka __int64 on winnt) to behave 
// properly...
//

class TriggerStatus : public NABasicObject
{

public:

	// default ctor
	TriggerStatus():
			triggerId_((ComTimestamp)0),
			enableStatus_(FALSE)
		{}

	// ctor
	TriggerStatus(
		ComTimestamp triggerId, 
		NABoolean enableStatus):
			triggerId_(triggerId),
			enableStatus_(enableStatus)
		{}
		
	// copy ctor
	TriggerStatus(
		const TriggerStatus &other):
			enableStatus_(other.enableStatus_),
			triggerId_(other.triggerId_)

		{}

	inline NABoolean getEnableStatus() { return enableStatus_; }

	inline ComTimestamp getTriggerId() { return triggerId_; }

private:
	ComTimestamp triggerId_; // Int64
	NABoolean enableStatus_;
};

//-----------------------------------------------------------------------------
//
// -- class TriggerStatusWA
//
// Trigger status is stored in rfork on a per-table basis. For each statement, 
// trigger status is accumulated for all subject tables in the statement.
// This Working Area serves for this purpose. It assumes that the statement is 
// already fixed-up.
// Holds an array of trigger status entries per table. 
// Allocated from a given heap in a fixed size.
// 


class TriggerStatusWA : public NABasicObject
{

public:

	enum TrgStatus
	{
		ENABLED, 
		DISABLED,
		NOT_FOUND // trigger not found in this table's rfork
	};

	// ctor. Called once per Statement after fixup
	TriggerStatusWA(NAHeap* heap, 
		ex_root_tcb* rootTcb) :
		currentNumEntries_(0), 
		totalTriggersCount_(0),
		triggerStatusArray_(NULL),
		heap_(heap),
		rootTcb_(rootTcb)
	{}

	~TriggerStatusWA()
		{}

	// called once per table
	void allocateStatusArray(UInt32 numEntries);

	// called once per table
	void deallocateStatusArray();

	// given a triggerId, get its enable status, if trigger exists.
	TrgStatus getStatus(ComTimestamp const triggerId) const;

	// number of triggers on current table, as read from rfork.
	inline Int32 getCurrentNumEntries()	const { return currentNumEntries_; }

	// total number of triggers in the statement
	inline Int32 getTotalTriggersCount()	const { return totalTriggersCount_; }

	ComTdbRoot* getRootTdb() const;

	void setEntry(TriggerStatus &entry, UInt32 index);

	// Given trigger status per table, update the (per statement) TCB trigger 
	// status vector
    void updateTriggerStatusPerTable();

#ifdef _DEBUG
	// for debug
	void print(ostream& os, const NAString& tableName);
#endif //_DEBUG

private:

 	// number of triggers on current table, as read from rfork.
	UInt32 currentNumEntries_;

	// Holds the status of triggers per a subject table as read from rfork
	TriggerStatus *triggerStatusArray_;

	// The heap used for the allocations of the triggerStatusArray
	NAHeap* heap_;

	// The total number of triggers handled by this WA, in this statement
	UInt32 totalTriggersCount_;

	// The TCB of the statement. Used for getting root TDB as well
	ex_root_tcb* rootTcb_;

};

#endif

