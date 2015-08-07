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
 * File:         TriggerEnable.cpp
 * Description:  Classes and methods used by the executor for the trigger 
 *               enable/disable mechanism.
 *
 * Created:      12/30/98
 * Language:     C++
 *
 *
 *
 *****************************************************************************
 */

#include "TriggerEnable.h"

#include "ex_stdh.h"
#include "ComTdb.h"
#include "ex_tcb.h"
#include "ex_root.h"

#include "NAMemory.h"

//-----------------------------------------------------------------------------
//
// Accessor that guarantess tdb to tcb association
//

ComTdbRoot* 
TriggerStatusWA::getRootTdb() const
{ 
	return (ComTdbRoot*) rootTcb_->getTdb(); 
}

//-----------------------------------------------------------------------------
//
// Allocation and deallocation of the per-statement trigger status array.
// The private member heap_ is used.

void
TriggerStatusWA::allocateStatusArray(UInt32 numEntries)

{
	if (numEntries == 0)
		return;

	// The following works with the VC++ compiler, 
	// but probably not on the NSK compiler:
	// triggerStatusArray = new (heap_) TriggerStatus[numEntries];

	// use our own heap_
	triggerStatusArray_ = 
	triggerStatusArray_ = 
		(TriggerStatus *)new (heap_) char[numEntries * sizeof(TriggerStatus)];
	// initialize the status array to all 0's
	memset(triggerStatusArray_, 0, numEntries * sizeof(TriggerStatus));

	currentNumEntries_ = numEntries;

}


void
TriggerStatusWA::deallocateStatusArray()
{
	ex_assert(heap_, "heap of TriggerStatusArray must be initialized");
	if (currentNumEntries_ == 0)
		return;
	currentNumEntries_ = 0;
	NADELETEBASIC(triggerStatusArray_, heap_);
	triggerStatusArray_ = NULL;
}

//-----------------------------------------------------------------------------
//
// Lookup of a trigger ID in the array of trigger IDs of a specific subject 
// table. A lookup of a trigger that is not defined on that table returns 
// TrgStatus::NOT_FOUND.
//

TriggerStatusWA::TrgStatus
TriggerStatusWA::getStatus(ComTimestamp const triggerId) const
{
	ex_assert(triggerStatusArray_ != NULL, "TriggerStatusArray not initialized");

	for (UInt32 i=0; i<currentNumEntries_; i++)
		if (triggerStatusArray_[i].getTriggerId() == triggerId)
			return (triggerStatusArray_[i].getEnableStatus() ?
				ENABLED : DISABLED);
	return NOT_FOUND;
}

//-----------------------------------------------------------------------------
//
// Setting an entry in the array.
//

void 
TriggerStatusWA::setEntry(TriggerStatus &entry, UInt32 index) 
{ 
	ex_assert(index < currentNumEntries_, 
		"Out of bounds access to TriggerStatusArray"); 
	triggerStatusArray_[index] = entry; 
} 

//-----------------------------------------------------------------------------
//
// Debug routine
// 

#ifdef _DEBUG
  
void 
TriggerStatusWA::print(ostream& os, const NAString& tableName)

{
	os	<< endl << "Trigger Status Array for Table " 
		<< tableName << " : " << currentNumEntries_ << endl;
	os	<< "----------------------------------------" << endl;
	
	char int64Str[128];

	for (UInt32 i=0; i<currentNumEntries_; i++)
	{
		convertInt64ToAscii(triggerStatusArray_[i].getTriggerId(), int64Str);
		os	<< int64Str << " : " 
			<< triggerStatusArray_[i].getEnableStatus() << endl; 
	}
}

//-----------------------------------------------------------------------------
//
// Debug Routine
//
// displays bits in b in the following order (ignore loop variable i):
// byte_0, byte_1,...byte_l
// Within each byte:
// left to right
// MSB -> LSB
// 0,1,2,...,7
// 

void bitDisplay(char * b,  Int32 l) 
{
   UInt32 c = 0, displayMask = 1 << 7;

   for (Int32 i = 0; i< l; i++)
   {
	   memcpy(&c, b+i, 1);
	   for (Int32 j = 0; j < 8; j++)
	   {
		   cout << ((c & displayMask) ? '1' : '0');
		   c <<= 1;
	   }
	   cout  << ' ';
   }
   cout << "\n";
}

#endif //_DEBUG

//-----------------------------------------------------------------------------
//
// For each table, this method is called and the TCB buffer holding trigger
// status is updated.
//

void
TriggerStatusWA::updateTriggerStatusPerTable()
{
	Int32 triggersPerStatement =  getRootTdb()->getTriggersCount();

	// robustness: triggers may be dropped and disappear from rfork
	// and still this method can be called from fixup phase, which
	// occurs prior to similarity check.
	if (getCurrentNumEntries() == 0)
		return;

	ex_assert(triggersPerStatement <= MAX_TRIGGERS_PER_STATEMENT,
		"Too many triggers in this statement");

	TrgStatus status;
	unsigned char mask;
	unsigned char mask2;
	
	// first time allocation of the TCB buffer from the executor heap
	if (rootTcb_->getTriggerStatusVector() == NULL) 
	{
		char * p = new (rootTcb_->getGlobals()->getDefaultHeap()) 
			char[TRIGGERS_STATUS_VECTOR_SIZE];
		rootTcb_->setTriggerStatusVector(p);
		// initialize to disabled
		memset(p, 0, TRIGGERS_STATUS_VECTOR_SIZE);
		
	}

	char* tcbBuffer = rootTcb_->getTriggerStatusVector();

	// for all triggers in the statement
	for (Int32 i = 0; i < triggersPerStatement; i++)
	{
		status = getStatus(getRootTdb()->getTriggersList()[i]);
		UInt32 byteOffset = i / 8;
		UInt32 withinByte = i % 8;
		mask  = 0x80; // 128 (single 1 in MSB of byte)
		mask2 = 0xFF; 
		
		switch (status)
		{
			// enabled
			case ENABLED: 
				// set the appropriate bit in the mask
				mask >>= withinByte;
				tcbBuffer[byteOffset] |= mask;
				totalTriggersCount_++;
				break;
			// disabled
			case DISABLED:
				// unset the appropriate bit in the mask
				mask >>= withinByte;
				mask ^= mask2;
				tcbBuffer[byteOffset] &= mask;
				// just count the found trigger
				totalTriggersCount_++;
				break;
			// not found in this array of this table
			case NOT_FOUND:
				break;
			default:
				ex_assert(0, "Illegal status of a trigger");
				break;
		}

	}

#ifdef _DEBUG

	if (getenv("SHOW_ENABLE"))
	{
		cout << "\nTcb Buffer:\n";
		bitDisplay(tcbBuffer, (getRootTdb()->getTriggersCount() / 8 + 1));
		cout << endl;
	}

#endif //_DEBUG
		
}

 
