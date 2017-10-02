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
//
**********************************************************************/
#ifndef _RU_MV_EQUIVSET_BUILDER_H_
#define _RU_MV_EQUIVSET_BUILDER_H_

/* -*-C++-*-
******************************************************************************
*
* File:         RuMVEquivSetBuilder.h
* Description:  Definition of class CRUMVEquivSetBuilder
*
* Created:      11/23/2000
* Language:     C++
*
*
******************************************************************************
*/

#include "refresh.h"

#include "RuMV.h"
#include "RuEquivSetBuilder.h"

//--------------------------------------------------------------------------//
// CRUMVEquivSetBuilder
// 
//	The input for this class is a set of the involved MVs, and the output 
//	is a list of root MVs for each equivalence set.
//
//	An MV equivalence set contains involved ON REQUEST MVs using each other, 
//	either directly or indirectly. The *root MV* in the set is the MV that 
//	uses no other MV.
//	
//--------------------------------------------------------------------------//


class REFRESH_LIB_CLASS CRUMVEquivSetBuilder : public CRUEquivSetBuilder
{
	//----------------------------------//
	//	Public Members
	//----------------------------------//
public:

	CRUMVEquivSetBuilder();

	virtual ~CRUMVEquivSetBuilder();

public:

	// This function is used for entering the input 
	// for the builder
	// The whole graph is build from the given mv's
	virtual void AddMV(CRUMV *pMV);


	CRUMVList& GetSet(Int32 num);

	// The main execution function for this class
	virtual void Run();

	// These functions are used for retrieving the results
	virtual Int32 GetNumOfSets() 
	{
		return equivSetsRootsList_.GetCount();
	}

	CRUMVList& GetEquivSetsRootsByMVUID(TInt64 uid);

public:

#ifdef _DEBUG
	// For debug purpose , dumps all sets to the output file
	virtual void DumpSets();
#endif

	//----------------------------------//
	//	Private Members
	//----------------------------------//

	// Run() callee
private:
	
	void BuildDisJointGraph();
	void BuildSets();
	void AddOnRequestMVs();
	void AddOnRequestMV(CRUMV *pMV);

private:

	CRUMVList				onRequestMVsList_;

	CDSPtrList<CRUMVList>	equivSetsRootsList_;

	CDSPtrList<CRUMV>		rootsMvsList_;
};

#endif
