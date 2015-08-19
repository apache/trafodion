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
* File:         RuTblEquivSetBuilder.cpp
* Description:  Implementation of class CRUTblEquivSetBuilder
*
* Created:      11/23/2000
* Language:     C++
*
*
******************************************************************************
*/

#include "RuTblEquivSetBuilder.h"
#include "RuGlobals.h"
#include "RuOptions.h"
#include "RuJournal.h"

//--------------------------------------------------------------------------//
//	Constructor & Destructors
//--------------------------------------------------------------------------//

CRUTblEquivSetBuilder::CRUTblEquivSetBuilder()
: syncTablesList_(eItemsArentOwned),
  onRequestMVsList_(eItemsArentOwned),
  recomputeMVsList_(eItemsArentOwned),
  equivSetsList_(eItemsAreOwned)
{}

CRUTblEquivSetBuilder::~CRUTblEquivSetBuilder()
{}

//--------------------------------------------------------------------------//
//	CRUTblEquivSetBuilder::AddMV() 
//
// Add the given mv and categorize it by adding it to the proper list
//--------------------------------------------------------------------------//

void CRUTblEquivSetBuilder::AddMV(CRUMV *pMV)
{
	RUASSERT(pMV != NULL);

	switch (pMV->GetRefreshType())
	{
		case CDDObject::eON_STATEMENT:
			{
				// on statment does not need syncronization
				break;
			}
		case CDDObject::eON_REQUEST:
			{
				onRequestMVsList_.AddTail(pMV);
				break;
			}
		case CDDObject::eRECOMPUTE:
			{
				recomputeMVsList_.AddTail(pMV);
				break;
			}
		default: RUASSERT(FALSE);
	}
}

//--------------------------------------------------------------------------//
//	CRUTblEquivSetBuilder::GetSet() 
//--------------------------------------------------------------------------//

CRUTblList& CRUTblEquivSetBuilder::GetSet(Int32 num)
{
	RUASSERT (GetNumOfSets() >= num && 0 <= num );
	
	CRUTblList *pTblList = equivSetsList_.GetAt(num);

	RUASSERT (NULL != pTblList);
	
	return *pTblList;
}

//--------------------------------------------------------------------------//
//	CRUTblEquivSetBuilder::Run() 
//
// Main execution function
//--------------------------------------------------------------------------//

void CRUTblEquivSetBuilder::Run() 
{
	// Iterate through all the given mv's and add all the nessesary used
	// objects to the graph.This function also builds the syncronized table
	// list
	BuildDisJointGraph();
	
	// Analyze the graph with the disjoint graph helper class
	GetDisJointAlg().Run();

	// Iterate through all the syncronized tables and build syncronized table
	// sets
	BuildSets();
}


//--------------------------------------------------------------------------//
//	CRUTblEquivSetBuilder::BuildDisJointGraph() 
//
// Iterate through all the given mv's and add all the nessesary used
// objects to the graph.This function also builds the syncronized table
// list
//--------------------------------------------------------------------------//

void CRUTblEquivSetBuilder::BuildDisJointGraph() 
{
	//phase 0 :: Add only on request mv's and their used tables
	AddOnRequestMVs();

	//phase 1 :: Add all recomputed mv's that needs tables syncronization 
	//			 and their used tables
	AddRecomputedMVs();	
}

//--------------------------------------------------------------------------//
//	CRUTblEquivSetBuilder::AddOnRequestMVs() 
//
//--------------------------------------------------------------------------//

void CRUTblEquivSetBuilder::AddOnRequestMVs() 
{
	DSListPosition mvPos = onRequestMVsList_.GetHeadPosition();
	while (NULL != mvPos)
	{
		CRUMV *pMV = onRequestMVsList_.GetNext(mvPos);
		
		AddOnRequestMV(pMV);
	}
}

//--------------------------------------------------------------------------//
//	CRUTblEquivSetBuilder::AddOnRequestMV() 
//
// We go over all the involved on request mv's and for each mv ,
// we first add it as a vertex to the disjointsets alg' and than 
// go over all of his tables and add to the disjointsets alg' those who  
// are not an involved mv's but are an involved tables 
//
// We make exception for an involved table that is a not involved mv 
// This will be explain in more detail in the function body
//--------------------------------------------------------------------------//

void CRUTblEquivSetBuilder::AddOnRequestMV(CRUMV *pMV)
{
	RUASSERT(CDDObject::eON_REQUEST == pMV->GetRefreshType());

	GetDisJointAlg().AddVertex(pMV->GetUID());

	// All tables in this list are involved tables by definition
	CRUTblList &tblList = pMV->GetTablesUsedByMe();
		
	DSListPosition tblPos = tblList.GetHeadPosition();

	while (NULL != tblPos)
	{
		CRUTbl *pTbl = tblList.GetNext(tblPos);
		
		if (TRUE == pTbl->IsFullySynchronized())
		{
			// This is as syncronized table ,we must place it in the 
			// syncronized table list and in the disjoint graph
			AddSyncTable(pTbl);

			GetDisJointAlg().AddEdge(pTbl->GetUID(),pMV->GetUID());
		}

		if (FALSE == pTbl->IsInvolvedMV())
		{
			// This is not an involved mv do not add it to the set.
			// A not involved mv does not require syncronization 
			// because it may only be changed by the refresh utility
			// and this is prevented by the ddl locks that are place on this
			// object
			continue;
		}

		GetDisJointAlg().AddEdge(pTbl->GetUID(),pMV->GetUID());
	}
}

//--------------------------------------------------------------------------//
//	CRUTblEquivSetBuilder::AddRecomputedMVs() 
//
// We only add a recompute mv that obeys the rule : 
//	   the mv is based on more than two objects and at least on is an mv.
//--------------------------------------------------------------------------//

void CRUTblEquivSetBuilder::AddRecomputedMVs() 
{
	DSListPosition mvPos = recomputeMVsList_.GetHeadPosition();
	while (NULL != mvPos)
	{
		CRUMV *pMV = recomputeMVsList_.GetNext(mvPos);
		
		if (0 == pMV->GetNumDirectlyUsedMVs() ||
			1 == pMV->GetTablesUsedByMe().GetCount())
		{
			continue;
		}

		AddRecomputeTree(pMV);
	}
}

//--------------------------------------------------------------------------//
//	CRUTblEquivSetBuilder::AddRecomputeTree() 
//
// If the mv is already in the graph we have nothing more to do,otherwise
// We should add recursively the tree beneath the given mv. We should also
// force long lock on all syncronized tables that are in the tree.
// It is important to remember that every on-request mv we encounter is 
// already in the graph (and his sub-tree) so we just need to add the edge 
// to it
//--------------------------------------------------------------------------//

void CRUTblEquivSetBuilder::AddRecomputeTree(CRUMV *pMV)
{
	RUASSERT(CDDObject::eRECOMPUTE == pMV->GetRefreshType());

	if (TRUE == GetDisJointAlg().HasVertex(pMV->GetUID()))
	{
		// The mv is already in the graph and his sub tree must also be there  
		// by induction 
		return;
	}
	
	GetDisJointAlg().AddVertex(pMV->GetUID());
	
	// Go over all the childs
	CRUTblList &tblList = pMV->GetTablesUsedByMe();
	DSListPosition tblPos = tblList.GetHeadPosition();
	while (NULL != tblPos)
	{
		CRUTbl *pTbl = tblList.GetNext(tblPos);
		
		if (TRUE == pTbl->IsFullySynchronized())
		{
			GetDisJointAlg().AddEdge(pTbl->GetUID(),pMV->GetUID());

			AddSyncTable(pTbl);
		
			pTbl->SetLongLockIsNeeded();
			
			continue;
		}

		if (FALSE == pTbl->IsInvolvedMV())
		{
			// This is not an involved mv do not add it to the set.
			// A not involved mv does not require syncronization 
			// because it may only be changed by the refresh utility
			// and this is prevented by the ddl locks that are place on this
			// object
			continue;
		}

		// The object is an mv - if its on request we should do nothing
		// forcing lock may be skipped and all involved on request 
		// are already in the graph
		
		GetDisJointAlg().AddEdge(pTbl->GetUID(),pMV->GetUID());

		CRUMV *pNextMV = pTbl->GetMVInterface();

		RUASSERT(NULL != pNextMV);

		if (CDDObject::eON_REQUEST == pNextMV->GetRefreshType() )
		{
			// All on requests mv's are already in the graph 
			continue;
		}
		
		RUASSERT(CDDObject::eRECOMPUTE == 
			     pNextMV->GetRefreshType());

		// call this function recursively for adding all sub tree 
		AddRecomputeTree(pNextMV);
	}
}

//--------------------------------------------------------------------------//
//	CRUTblEquivSetBuilder::AddSyncTable() 
//--------------------------------------------------------------------------//

void CRUTblEquivSetBuilder::AddSyncTable(CRUTbl *pTbl)
{
	RUASSERT(pTbl != NULL);

	if (FALSE == GetDisJointAlg().HasVertex(pTbl->GetUID()))
	{
		// We should add all base tables to the disjoint graph
		// because the same table can be used by several mv's
		// and there for be a connection point 
		GetDisJointAlg().AddVertex(pTbl->GetUID());
		// This is the first encounter with this table 
		// lets add it to the table list
		syncTablesList_.AddTail(pTbl);
	}
}

//--------------------------------------------------------------------------//
//	CRUTblEquivSetBuilder::BuildSets() 
//
// Collect the output from the disjoint set algorithm and prepare the sets
//--------------------------------------------------------------------------//

void CRUTblEquivSetBuilder::BuildSets()
{
	// Initialize the array of sets of CRUTbl
	Int32 equivSetsSize = GetDisJointAlg().GetNumOfSets();
	
	// Initialize the sets of CRUTbl
	for (Int32 i=0;i<equivSetsSize;i++)
	{
		equivSetsList_.AddTail(
			new CRUTblList(eItemsArentOwned));
	}

	// Pass all regular tables and add them to the appropriate list
	DSListPosition tblPos = syncTablesList_.GetHeadPosition();

	while (NULL != tblPos)
	{
		CRUTbl *pTbl = syncTablesList_.GetNext(tblPos);
		
		Int32 setId = GetDisJointAlg().GetNodeSetId(pTbl->GetUID());
		
		equivSetsList_.GetAt(setId)->AddTail(pTbl);
	}

	// Remove set with zero size
	DSListPosition setsPos = equivSetsList_.GetHeadPosition();

	while (NULL != setsPos)
	{
		DSListPosition setsPrevpos = setsPos;

		CRUTblList *pTblList = equivSetsList_.GetNext(setsPos);
		if (0 == pTblList->GetCount())
		{
			equivSetsList_.RemoveAt(setsPrevpos);
		}
	}	
}

#ifdef _DEBUG
//--------------------------------------------------------------------------//
//	CRUTblEquivSetBuilder::DumpSets() 
//--------------------------------------------------------------------------//

void CRUTblEquivSetBuilder::DumpSets()
{
	CDSString msg;

	msg = "\n Equivalent sets: \n" ;

	CRUGlobals::GetInstance()->GetJournal().LogMessage(msg);
	for (Int32 i=0;i<GetNumOfSets();i++)
	{
		msg = "\n Table Set: \n" ;
		
		CRUGlobals::GetInstance()->GetJournal().LogMessage(msg);

		DSListPosition tblPos = GetSet(i).GetHeadPosition();

		while (NULL != tblPos)
		{
			CRUTbl *pTbl = GetSet(i).GetNext(tblPos);
			
			CRUGlobals::GetInstance()->GetJournal().
				                          LogMessage(pTbl->GetFullName());
		}
	}
}
#endif
