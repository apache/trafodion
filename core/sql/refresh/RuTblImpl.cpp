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
* File:         RuTblImpl.cpp
* Description:  Implementation of classes CRURegularTbl and CRUMVTbl
*				(derived from CRUTbl)
*
* Created:      03/21/2000
* Language:     C++
*
* 
******************************************************************************
*/

#include "RuTblImpl.h"

//--------------------------------------------------------------------------//
//	Class CRURegularTbl
//--------------------------------------------------------------------------//

CRURegularTbl::CRURegularTbl(CDDTable *pddTable) :
	inherited(pddTable->GetTblUsedByMV()), 
	pddTable_(pddTable)
{}

//--------------------------------------------------------------------------//
//	CRURegularTbl::ExecuteReadProtectedOpen()
//--------------------------------------------------------------------------//

void CRURegularTbl::ExecuteReadProtectedOpen()
{
	

#ifdef NA_LINUX
        pddTable_->
		OpenPartitions(CUOFsFile::eProtected, CUOFsFile::eReadOnly,
                TRUE, FALSE);
#else

        pddTable_->
		OpenPartitions(CUOFsFile::eProtected, CUOFsFile::eReadOnly, TRUE, TRUE, TRUE);
#endif
	inherited::ExecuteReadProtectedOpen();
}

//--------------------------------------------------------------------------//
//	CRURegularTbl::ReleaseReadProtectedOpen()
//--------------------------------------------------------------------------//

void CRURegularTbl::ReleaseReadProtectedOpen()
{
	pddTable_->ClosePartitions();

	inherited::ReleaseReadProtectedOpen();
}

//--------------------------------------------------------------------------//
//	CRURegularTbl::SaveMetadata()
//--------------------------------------------------------------------------//

void CRURegularTbl::SaveMetadata()
{
	inherited::SaveMetadata();

	pddTable_->Save();	// Saving the DDL locks 
}

//--------------------------------------------------------------------------//
//	Class CRUMVTbl
//--------------------------------------------------------------------------//

CRUMVTbl::CRUMVTbl(CDDMV *pddMV) :
	inherited(pddMV->GetTblUsedByMV()), 
	pddMV_(pddMV)
{}

//--------------------------------------------------------------------------//
//	CRUMVTbl::ExecuteReadProtectedOpen()
//--------------------------------------------------------------------------//

void CRUMVTbl::ExecuteReadProtectedOpen()
{
#ifdef NA_LINUX
	pddMV_->
		OpenPartitions(CUOFsFile::eProtected, CUOFsFile::eReadOnly, 
                               TRUE, FALSE);
#else                
	pddMV_->
		OpenPartitions(CUOFsFile::eProtected, CUOFsFile::eReadOnly); 
#endif
	inherited::ExecuteReadProtectedOpen();
}

//--------------------------------------------------------------------------//
//	CRUMVTbl::ReleaseReadProtectedOpen()
//--------------------------------------------------------------------------//

void CRUMVTbl::ReleaseReadProtectedOpen()
{
	pddMV_->ClosePartitions();

	inherited::ReleaseReadProtectedOpen();
}

//--------------------------------------------------------------------------//
//	CRUMVTbl::SaveMetadata()
//--------------------------------------------------------------------------//

void CRUMVTbl::SaveMetadata()
{
	inherited::SaveMetadata();

	pddMV_->Save();	// Saving the DDL locks 
}
