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
#ifndef _RU_DISJOINT_SETS_H_
#define _RU_DISJOINT_SETS_H_

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

#include "refresh.h"
#include "dsplatform.h"
#include "dsptrlist.h"
#include "dsmap.h"

//--------------------------------------------------------------------------//
// CRUDisjointSetAlg
// 
// This class recieves a graph definition (V,E) and resolove it 
// to a disjoint sets,it means that a path from a node in one set to
// any other node in a different set does not exists.
// 
//
// A short description of the algorithm : 
//		phase 0: We begin in a forest where each node is tree. 
//      phase 1: We make a single pass over the edges in E and for each edge
//				 if the left and right nodes belongs to two different trees
//				 we merge the two trees by finding the trees roots and making
//				 one root point to the other
//		phase 2: We go over all the roots and number them by an ever 
//				 increasing number
// 
// 
//	There is a small test function that represent the use of this algorithm. 
//--------------------------------------------------------------------------//

//--------------------------------------------------------------------------//
//	CRUDisjointSetAlg Helper Classes (used to be local classes but
//                                        that caused compiler problems)
//--------------------------------------------------------------------------//

//--------------------------------------------------------------------------//
//	CRUDisjointSetAlgVertex 
//--------------------------------------------------------------------------//

struct CRUDisjointSetAlgVertex {

	CRUDisjointSetAlgVertex(TInt64 id)
		:id_(id),rank_(0),pParent_(NULL),setId_(-1)
		
	{};

	// A unique Identifier 
	TInt64 id_;
	// pointer to the next node in the same set
	CRUDisjointSetAlgVertex *pParent_;
	// maximal depth of nodes that this node is their ancestor
	Int32 rank_;
	// A unique Identifier that represent the set to which this node belongs
	Int32 setId_;

};

//--------------------------------------------------------------------------//
//	CRUDisjointSetAlgEdge 
//--------------------------------------------------------------------------//

struct CRUDisjointSetAlgEdge {

	// A default constructor is needed for the Edges list
	CRUDisjointSetAlgEdge() {};

	CRUDisjointSetAlgEdge(TInt64 fromId,TInt64 toId) :
	  leftId_(fromId),rightId(toId) {};

	TInt64 leftId_;
	TInt64 rightId;

};

class REFRESH_LIB_CLASS CRUDisjointSetAlg {
public:

	CRUDisjointSetAlg();

	virtual ~CRUDisjointSetAlg();

public:
	
	// returns FALSE if the id was already been added
	void AddVertex(TInt64 id);

	BOOL HasVertex(TInt64 id)
	{
		CRUDisjointSetAlgVertex *pVertex;	
		return V_.Lookup(&id,pVertex);
	}

	void AddEdge(TInt64 first,TInt64 second);

public:
	
	void Run();
	Int32 GetNumOfSets()
	{
		return sets_.GetCount();
	}

	inline Int32 GetNodeSetId(TInt64 id);
	
public:
#ifdef _DEBUG
	// This is a test function for this class 
	void Test();
#endif

private:
	void BuildDisjointSetGraph();
	void NameSets();

private:

	CRUDisjointSetAlgVertex& FindSet(TInt64 id);
	
	void UnionSets(CRUDisjointSetAlgVertex &first,
		       CRUDisjointSetAlgVertex &second);

private:	
	void RemoveFromRootList(TInt64 id);

private:
	typedef CDSTInt64Map<CRUDisjointSetAlgVertex*> VertexMap;	
	

	enum { HASH_SIZE = 101 };

private:
	
	// A hash table of links to all the tasks 
	VertexMap			    V_;
	CDSList<CRUDisjointSetAlgEdge>	    E_;

	Int32				    numNodes_;
	
	CDSPtrList<CRUDisjointSetAlgVertex> sets_;

};

//--------------------------------------------------------------------------//
//	CRUDisjointSetAlg Inlines
//--------------------------------------------------------------------------//


//--------------------------------------------------------------------------//
//	CRUDisjointSetAlg::GetNodeSetId() 
//--------------------------------------------------------------------------//

Int32 CRUDisjointSetAlg::GetNodeSetId(TInt64 id)
{
	return FindSet(id).setId_;
}

#endif
