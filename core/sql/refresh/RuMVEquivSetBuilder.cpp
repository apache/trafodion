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
/* -*-C++-*-
******************************************************************************
*
* File:         RuMVEquivSetBuilder.cpp
* Description:  Implementation of class CRUMVEquivSetBuilder
*
* Created:      11/23/2000
* Language:     C++
*
*
******************************************************************************
*/

#include "RuMVEquivSetBuilder.h"
#include "RuGlobals.h"
#include "RuOptions.h"
#include "RuJournal.h"
#include "RuTbl.h"

//--------------------------------------------------------------------------//
//	Constructor & Destructors
//--------------------------------------------------------------------------//

CRUMVEquivSetBuilder::CRUMVEquivSetBuilder()
: rootsMvsList_(eItemsArentOwned),
  onRequestMVsList_(eItemsArentOwned),
  equivSetsRootsList_(eItemsAreOwned)
{}

CRUMVEquivSetBuilder::~CRUMVEquivSetBuilder()
{}

//--------------------------------------------------------------------------//
//	CRUMVEquivSetBuilder::AddMV() 
//
//	Add the given MV to the data structure
//--------------------------------------------------------------------------//

void CRUMVEquivSetBuilder::AddMV(CRUMV *pMV)
{
	RUASSERT (
		pMV != NULL 
		&&
		CDDObject::eON_REQUEST == pMV->GetRefreshType()
	);

	onRequestMVsList_.AddTail(pMV);
}

//--------------------------------------------------------------------------//
//	CRUMVEquivSetBuilder::GetSet() 
//--------------------------------------------------------------------------//

CRUMVList& CRUMVEquivSetBuilder::GetSet(Int32 num)
{
	RUASSERT (GetNumOfSets() >= num && 0 <= num );
	
	CRUMVList *pMVList = equivSetsRootsList_.GetAt(num);

	RUASSERT (NULL != pMVList);
	
	return *pMVList;
}

//--------------------------------------------------------------------------//
//	CRUMVEquivSetBuilder::Run() 
//
//	Main execution function
//--------------------------------------------------------------------------//

void CRUMVEquivSetBuilder::Run() 
{
	// Iterate through all the given mv's and add all the nessesary used
	// objects to the graph.This function also builds the root mv's list
	BuildDisJointGraph();
	
	// Analyze the graph with the disjoint graph helper class
	GetDisJointAlg().Run();

	// Iterate through all the root mv's list and build the root mvs' sets
	BuildSets();
}

//--------------------------------------------------------------------------//
//	CRUMVEquivSetBuilder::BuildDisJointGraph() 
//
//	Iterate through all the given mv's and add all the nessesary used
//	objects to the graph.
//--------------------------------------------------------------------------//

void CRUMVEquivSetBuilder::BuildDisJointGraph() 
{
	AddOnRequestMVs();
}

//--------------------------------------------------------------------------//
//	CRUMVEquivSetBuilder::AddOnRequestMVs() 
//--------------------------------------------------------------------------//

void CRUMVEquivSetBuilder::AddOnRequestMVs() 
{
	DSListPosition mvPos = onRequestMVsList_.GetHeadPosition();
	while (NULL != mvPos)
	{
		CRUMV *pMV = onRequestMVsList_.GetNext(mvPos);
		
		AddOnRequestMV(pMV);
	}
}

//--------------------------------------------------------------------------//
//	CRUMVEquivSetBuilder::AddOnRequestMV() 
//
//	For each ON REQUEST mv add it as a vertex to the disjoint sets alg' 
//	and then iterate through all of its tables and add edges between the MVs.
//
//	We do not need to deal with recomputed mv's because there are no 
//	recomputed MVs under an on request MV.
//--------------------------------------------------------------------------//

void CRUMVEquivSetBuilder::AddOnRequestMV(CRUMV *pMV)
{
	RUASSERT(CDDObject::eON_REQUEST == pMV->GetRefreshType());

	GetDisJointAlg().AddVertex(pMV->GetUID());

	CRUTblList &tblList = pMV->GetTablesUsedByMe();
		
	DSListPosition tblPos = tblList.GetHeadPosition();

	BOOL isRoot = TRUE;

	while (NULL != tblPos)
	{
		CRUTbl *pTbl = tblList.GetNext(tblPos);
		
		if (TRUE  == pTbl->IsFullySynchronized() ||
			FALSE == pTbl->IsInvolvedMV())
		{
			// regular tables and uninvolved mv's are not a part 
			// of the equivalent mv's sets
			continue;
		}

		isRoot = FALSE;

		GetDisJointAlg().AddEdge(pTbl->GetUID(), pMV->GetUID());
	}

	if (TRUE == isRoot)
	{
		rootsMvsList_.AddTail(pMV);
	}
}

//--------------------------------------------------------------------------//
//	CRUMVEquivSetBuilder::BuildSets() 
//
//	Collect the output from the disjoint set algorithm and prepare the sets
//--------------------------------------------------------------------------//

void CRUMVEquivSetBuilder::BuildSets()
{
	Int32 equivSetsSize = GetDisJointAlg().GetNumOfSets();
	
	// Initialize the list of root mv's sets 
	for (Int32 i=0;i<equivSetsSize;i++)
	{
		equivSetsRootsList_.AddTail(
			new CRUMVList(eItemsArentOwned));
	}

	DSListPosition mvPos = rootsMvsList_.GetHeadPosition();

	while (NULL != mvPos)
	{
		CRUMV *pMV = rootsMvsList_.GetNext(mvPos);

		Int32 setId = GetDisJointAlg().GetNodeSetId(pMV->GetUID());

		equivSetsRootsList_.GetAt(setId)->AddTail(pMV);
	}
}

//--------------------------------------------------------------------------//
//	CRUMVEquivSetBuilder::GetEquivSetsRootsByMVUID() 
//
//	Returns the root mv's for which the given MV belongs to
//--------------------------------------------------------------------------//

CRUMVList& CRUMVEquivSetBuilder::GetEquivSetsRootsByMVUID(TInt64 uid)
{
	Int32 setId = GetDisJointAlg().GetNodeSetId(uid);

	RUASSERT(0 <= setId && setId < equivSetsRootsList_.GetCount())

	return *equivSetsRootsList_.GetAt(setId);
}

#ifdef _DEBUG
//--------------------------------------------------------------------------//
//	CRUMVEquivSetBuilder::DumpSets() 
//--------------------------------------------------------------------------//

void CRUMVEquivSetBuilder::DumpSets()
{
	CDSString msg;
	char tmpstr[10];

	msg = "\nMV Equivalence sets: \n" ;

	CRUGlobals::GetInstance()->GetJournal().LogMessage(msg);
	for (Int32 i=0;i<GetNumOfSets();i++)
	{
		sprintf(tmpstr, "%d", i+1);
		msg = "\nRoots of Set #" + CDSString(tmpstr) + ":\n" ;
		
		CRUGlobals::GetInstance()->GetJournal().LogMessage(msg);

		DSListPosition tblPos = GetSet(i).GetHeadPosition();

		while (NULL != tblPos)
		{
			CRUMV *pMV = GetSet(i).GetNext(tblPos);
			
			CRUGlobals::GetInstance()->GetJournal().
		                          LogMessage(pMV->GetFullName());
		}
	}
}
#endif
