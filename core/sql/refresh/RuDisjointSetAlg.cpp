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
* File:         RuDisJointSetAlg.h
* Description:  Definition of class CRUDisjointSetAlg
*
* Created:      11/23/2000
* Language:     C++
*
*
******************************************************************************
*/
#include "RuDisjointSetAlg.h"
#include "RuException.h"

//--------------------------------------------------------------------------//
//	Constructor & Destructors
//--------------------------------------------------------------------------//

CRUDisjointSetAlg::CRUDisjointSetAlg()
	:
	V_(HASH_SIZE),numNodes_(0),
	sets_(eItemsArentOwned)
{};

CRUDisjointSetAlg::~CRUDisjointSetAlg() 
{
	CDSMapPosition<CRUDisjointSetAlgVertex*> pos;
	
	CRUDisjointSetAlgVertex *pVertex;
	
	V_.GetStartPosition(pos);
	
	while (TRUE == pos.IsValid())
	{
		TInt64 *pId;
		V_.GetNextAssoc(pos,pId,pVertex);
		delete pVertex;
	}	
};

//--------------------------------------------------------------------------//
//	CRUDisjointSetAlg::AddVertex() 
// 
// Add a vertex to the graph and returns TRUE if the vertex 
// was not already in the tree
//--------------------------------------------------------------------------//

void CRUDisjointSetAlg::AddVertex(TInt64 id)
{
	CRUDisjointSetAlgVertex *pVertex;
	
	RUASSERT(FALSE == V_.Lookup(&id,pVertex));

	pVertex = new CRUDisjointSetAlgVertex(id);

	V_[&(pVertex->id_)] = pVertex;

	// The algorithm starts when each vertex is a tree root
	// Each node becomes a root by pointing to it from the sets_ list
	sets_.AddTail(pVertex);

	numNodes_++;
}

//--------------------------------------------------------------------------//
//	CRUDisjointSetAlg::AddEdge() 
//--------------------------------------------------------------------------//

void CRUDisjointSetAlg::AddEdge(TInt64 first,TInt64 second)
{
        // a copy constructor is activated here
	E_.AddTail(CRUDisjointSetAlgEdge(first,second));
}

//--------------------------------------------------------------------------//
//	CRUDisjointSetAlg::Run() 
//--------------------------------------------------------------------------//

void CRUDisjointSetAlg::Run() 
{
	// Go over the Edges and union the sets that are connected by each edge
	BuildDisjointSetGraph();

	// Naming each set by a unique identifier 
	NameSets();
}

//--------------------------------------------------------------------------//
//	CRUDisjointSetAlg::BuildDisjointSetGraph() 
//
//  While iterating through the edges list we merge the two sets that
//  that are connected by that edge.The sets are represented by one of its 
//  vertices (The set's root node)
//--------------------------------------------------------------------------//

void CRUDisjointSetAlg::BuildDisjointSetGraph() 
{
	// Start finding connected nodes
	DSListPosition pos = E_.GetHeadPosition();
	while (NULL != pos)
	{
		CRUDisjointSetAlgEdge &e = E_.GetNext(pos);

		CRUDisjointSetAlgVertex &left = FindSet(e.leftId_);
		CRUDisjointSetAlgVertex &right = FindSet(e.rightId);

		if ( &left != &right )
		{
			UnionSets(left,right);
		}
	}
}

//--------------------------------------------------------------------------//
//	CRUDisjointSetAlg::NameSets()
//
// Unique identify each set by a number 
//--------------------------------------------------------------------------//

void CRUDisjointSetAlg::NameSets() 
{
	DSListPosition pos = sets_.GetHeadPosition();
	
	Int32 i=0;
	while (NULL != pos)
	{
		CRUDisjointSetAlgVertex *v = sets_.GetNext(pos);
		v->setId_ = i++;
	}
}

//--------------------------------------------------------------------------//
//	CRUDisjointSetAlg::FindSet() 
//
// Find the set which the vertex belongs to.Each set is represented  
// by one of its vertices (The set's root node)
//--------------------------------------------------------------------------//

CRUDisjointSetAlgVertex& CRUDisjointSetAlg::FindSet(TInt64 id) 
{
	
	CRUDisjointSetAlgVertex *pVertex = NULL;
	if (FALSE == V_.Lookup(&id,pVertex))
	{
		RUASSERT(FALSE);
	}

	// Go up the tree until you reach the root 
	for (;;)
	{
		if (pVertex->pParent_ == NULL)
		{
			break;
		}

		pVertex = pVertex->pParent_;
	}

	return *pVertex;
}

//--------------------------------------------------------------------------//
//	CRUDisjointSetAlg::UnionSets() 
//
//	Union the sets while the set with the bigger rank is placed as the son
//  of the other
//--------------------------------------------------------------------------//

void CRUDisjointSetAlg::UnionSets(CRUDisjointSetAlgVertex &first, 
				  CRUDisjointSetAlgVertex &second) 
{
	if (first.rank_ < second.rank_)
	{
		first.pParent_ = &second;
		second.rank_++;
		RemoveFromRootList(first.id_);
	}
	else
	{
		second.pParent_ = &first;
		first.rank_++;
		RemoveFromRootList(second.id_);
	}
}

//--------------------------------------------------------------------------//
//	CRUDisjointSetAlg::RemoveFromRootList() 
//
//--------------------------------------------------------------------------//

void CRUDisjointSetAlg::RemoveFromRootList(TInt64 id)
{
	DSListPosition pos = sets_.GetHeadPosition();

	while (NULL != pos)
	{
		DSListPosition prevpos = pos;

		CRUDisjointSetAlgVertex *v = sets_.GetNext(pos);
		if (id == v->id_)
		{
			sets_.RemoveAt(prevpos);
		}
	}
}

#ifdef _DEBUG
//--------------------------------------------------------------------------//
//	CRUDisjointSetAlg::Test() 
//--------------------------------------------------------------------------//

void CRUDisjointSetAlg::Test()
{
	// BUILD TEST GRAPH
	Int32 i;
	for (i=1;i<=14;i++)
	{
		AddVertex(i);
	}

	AddEdge(1,7);
	AddEdge(7,8);
	AddEdge(10,11);
	AddEdge(2,9);
	AddEdge(3,9);
	AddEdge(13,14);
	AddEdge(4,10);
	AddEdge(5,12);
	AddEdge(12,14);
	AddEdge(6,13);
	AddEdge(2,8);

	// RUN ALG
	Run();

	// CHECK RESULTS

	for (i=1;i<=14;i++)
	{
		cout << "node " << i << "is in set " << GetNodeSetId(i) << endl;
	}
	
	// THE RESULTS SHOULD BE 

	// 1,2,7,8,9,3 are in a first set
	// 4,10,11 are in a second set
	// 5,6,12,13,14 are in a third set

}

#endif
