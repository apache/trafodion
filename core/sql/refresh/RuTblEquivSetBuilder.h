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
#ifndef _RU_TBL_EQUIVSET_BUILDER_H_
#define _RU_TBL_EQUIVSET_BUILDER_H_

/* -*-C++-*-
******************************************************************************
*
* File:         RuTblEquivSetBuilder.h
* Description:  Definition of class CRUTblEquivSetBuilder
*
* Created:      11/23/2000
* Language:     C++
*
*
******************************************************************************
*/

#include "refresh.h"

#include "RuMV.h"
#include "RuTbl.h"
#include "RuEquivSetBuilder.h"

//--------------------------------------------------------------------------//
// CRUTblEquivSetBuilder
// 
//
// This class take as input the involved mv's and outputs the involved table
// equivalent sets that needs syncronization. 
// The table equivalent builder generates a digraph by the following rules:
//   1. For on request mvs , we add all mvs and used objects as nodes in the 
//      graph and the depedencies between them as edges.
//		
//	 2. For recomputed mv's , we add only the recomputed mv's that requires 
//		syncronization.A recomputed mv that requires syncronization is a 
//		recomputed mv that obey one of the following rules:
//			1. The mv is under another mv that requires syncronization in 
//			   the expanded tree.
//			2. The mv is based on more than two objects and at least on is 
//			   an mv.
//      For each fully syncronized table (i.e.,A regular table or on statment 
//      mv.) that we add to the graph we also force 
//      a long lock (a lock that will stay until the last mv that requires 
//		that table will be refreshed)
//	
//--------------------------------------------------------------------------//

class REFRESH_LIB_CLASS CRUTblEquivSetBuilder : public CRUEquivSetBuilder
{
	//----------------------------------//
	//	Public Members
	//----------------------------------//
public:

	CRUTblEquivSetBuilder();

	virtual ~CRUTblEquivSetBuilder();

public:

	// This function is used for entering the input 
	// for the builder
	// The whole graph is build from the given mv's
	virtual void AddMV(CRUMV *pMV);

	// The main execution function for this class
	virtual void Run();

	// These functions are used for retrieving the results
	virtual Int32 GetNumOfSets() 
	{
		return equivSetsList_.GetCount();
	}

	CRUTblList& GetSet(Int32 num);

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

// BuildDisJointGraph() callee
private:

	void AddOnRequestMVs();
	void AddRecomputedMVs();

private:
	void AddOnRequestMV(CRUMV *pMV);
	void AddRecomputeTree(CRUMV *pMV);

private:	
	
	void AddSyncTable(CRUTbl *pTbl);

private:

	// These lists hold the input for this class
	CRUMVList	onRequestMVsList_;

	CRUMVList	recomputeMVsList_;

	// This list holds the output of this class
	CDSPtrList<CRUTblList>	equivSetsList_;

private:
	// All syncronized tables (A regular table and an on statement mv)
	// that are used by the given mv's directly or indirectly 
	CDSPtrList<CRUTbl>			syncTablesList_;
};

#endif
