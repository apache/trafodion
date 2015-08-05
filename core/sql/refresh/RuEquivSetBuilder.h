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
#ifndef _RU_EQUIVSET_BUILDER_H_
#define _RU_EQUIVSET_BUILDER_H_

/* -*-C++-*-
******************************************************************************
*
* File:         RuEquivSetBuilder.h
* Description:  Definition of class CRUEquivSetBuilder
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
#include "RuMV.h"
#include "RuDisjointSetAlg.h"

//--------------------------------------------------------------------------//
// CRUEquivSetBuilder
// 
//	An abstract class for the equivalence sets builders.
//
//	An *equivalence set* is a connected component of objects involved into
//	REFRESH. The connection is implied by the "using" property between MVs
//	and tables, or MVs and MVs. 
//
//	There are two kinds of equivalence sets: a table equiv set and an MV 
//	equiv set. Each *table equiv set* is a union of non-disjoint table sets
//	used by the involved MVs' expanded trees. Each *MV equiv set* contains 
//	involved MVs using each other, either directly or indirectly.
//
//--------------------------------------------------------------------------//


class REFRESH_LIB_CLASS CRUEquivSetBuilder {
	//----------------------------------//
	//	Public Members
	//----------------------------------//
public:
	virtual ~CRUEquivSetBuilder() {}

public:
	// This function is used for entering the input into the builder.
	// The whole graph is built from the given mv's
	virtual void AddMV(CRUMV *pMV) = 0;

	// The main execution function
	virtual void Run() = 0;

	// These functions are used for retrieving the results
	virtual Int32 GetNumOfSets() = 0;

public:

#ifdef _DEBUG
	// For debug purpose , dumps all sets to the output file
	virtual void DumpSets() = 0;
#endif

protected:
	CRUDisjointSetAlg&	GetDisJointAlg() { return disJointAlg_; }

private:

	// A helper class used for analyzing the graph into disjoint sets
	CRUDisjointSetAlg		disJointAlg_;
};

#endif
