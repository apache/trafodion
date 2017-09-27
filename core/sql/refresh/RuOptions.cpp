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
* File:         RuOptions.cpp
* Description:  API to the command-line parameters of the REFRESH utility.
*
*
* Created:      01/09/2000
* Language:     C++
* 
*
* 
******************************************************************************
*/

#include "RuOptions.h"
#include "uofsIpcMessageTranslator.h"
#include "RuException.h"

// the refresh messages output filename will be initialialized to empty string
CDSString CRUOptions::defOutFilename = "REFRESH.LOG";

//--------------------------------------------------------------------------//
//	Constructors
//--------------------------------------------------------------------------//

CRUOptions::CRUOptions() :
	invocType_(SINGLE_MV),
	isRecompute_(FALSE),
	isCancel_(FALSE),
	lcType_(DONTCARE_LC), 
	catalogName_(""),
	schemaName_(""),
	objectName_(""),
	fullName_(""),
	defaultSchema_(""),
#if defined(NA_WINNT)
        outFilename_(defOutFilename),
#else
        outFilename_(""),
#endif
	forceFilename_("")
{}

CRUOptions::CRUOptions(const CRUOptions& other) :
	invocType_(other.invocType_),
	isRecompute_(other.isRecompute_),
	isCancel_(other.isCancel_),
	lcType_(other.lcType_),
	catalogName_(other.catalogName_),
	schemaName_(other.schemaName_),
	objectName_(other.objectName_),
	fullName_(other.fullName_),
	outFilename_(other.outFilename_),
	forceFilename_(other.forceFilename_)
{}

//--------------------------------------------------------------------------//
//	CRUOptions::FindDebugOption()
//
//	Search for the option using the <testpoint, objectName> pair.
//	If the object name in the options list is empty, 
//	the search does not use it as a criterion.
//
//--------------------------------------------------------------------------//

CRUOptions::DebugOption *CRUOptions::
FindDebugOption(Int32 testpoint, const CDSString &objName)
{
	DSListPosition pos = debugOptionList_.GetHeadPosition();
	while (NULL != pos)
	{
		DebugOption &opt = debugOptionList_.GetNext(pos);

		if (testpoint != opt.testpoint_)
		{
			continue;
		}

		if (0 == opt.objName_.GetLength()
			||
			objName == opt.objName_
			)
		{
			return &opt;
		}
	}

	return NULL;
}

//--------------------------------------------------------------------------//
//	CRUOptions::AddDebugOption()
//--------------------------------------------------------------------------//

void CRUOptions::AddDebugOption(Int32 testpoint, const CDSString &objName)
{
	CRUOptions::DebugOption opt;

	opt.testpoint_ = testpoint;
	opt.objName_ = objName;

	debugOptionList_.AddTail(opt);

        // when debuuging is set outfile is always set to default out filename
        if (outFilename_.IsEmpty())
           outFilename_ = defOutFilename;
}

#ifdef _DEBUG
//--------------------------------------------------------------------------//
//	CRUOptions::Dump()
//--------------------------------------------------------------------------//
void CRUOptions::Dump(CDSString &to) 
{
	to += "\n\t\tCOMMAND OPTIONS DUMP\n\n";

	to += "Catalog name = " + GetCatalogName() + "\n";
	to += "Schema name = " + GetSchemaName() + "\n";
	to += "Object name = " + GetObjectName() + " ";
	
	switch (GetInvocType()) {

	case SINGLE_MV:	to += "(MV)\n";
					break;

	case CASCADE  : to += "(CASCADE)\n";
					break;

	case MV_GROUP : to += "(MV GROUP)\n";
					break;

	default		  :	RUASSERT(FALSE);
	}

	to += "Recompute mode = ";
	to += (IsRecompute() ? "YES" : "NO");
	to += "\n";

	to += "DDL locks cancel mode = ";
	to += (IsCancel() ? "YES" : "NO");
	to += "\n";

	switch (GetLogCleanupType()) {

	case DONTCARE_LC:	to += "LOG CLEANUP NOT SPECIFIED";
						break;

	case WITH_LC	:	to += "INCLUDING LOG CLEANUP";
						break;

	case WITHOUT_LC	:	to += "NOT INCLUDING LOG CLEANUP";
						break;

	case DO_ONLY_LC :	to += "LOG CLEANUP ONLY";
						break;
	
	default		  :		ASSERT(FALSE);
	}

	to += "\nOutput file = \""; 
	to += GetOutputFilename();
	to += "\"\n";

	char buf[200];
	DSListPosition pos = debugOptionList_.GetHeadPosition();
	while (NULL != pos)
	{
		DebugOption &opt = debugOptionList_.GetNext(pos);
		sprintf(buf, "TESTPOINT %6d\t%s\n", opt.testpoint_, opt.objName_.c_string());
		to += CDSString(buf);
	}
}
#endif

//--------------------------------------------------------------------------//
//	CRUOptions::DebugOption &operator = ()
//--------------------------------------------------------------------------//

CRUOptions::DebugOption &CRUOptions::DebugOption::
operator = (const CRUOptions::DebugOption &other)
{
	testpoint_ = other.testpoint_;
	objName_ = other.objName_;

	return *this;
}

//--------------------------------------------------------------------------//
//	CRUOptions::StoreData()
//
//	The LoadData/StoreData methods move only the output 
//	file's name and the debug options between the processes.
//--------------------------------------------------------------------------//
void CRUOptions::StoreData(CUOFsIpcMessageTranslator &translator)
{
 Int32 stringSize;
	
	// Output filename
	stringSize = outFilename_.GetLength() + 1;
	translator.WriteBlock(&stringSize, sizeof(Int32));
#pragma nowarn(1506)   // warning elimination 
	translator.WriteBlock(outFilename_.c_string(), stringSize);
#pragma warn(1506)  // warning elimination 
	
	// Force filename
	stringSize = forceFilename_.GetLength() + 1;
	translator.WriteBlock(&stringSize, sizeof(Int32));
#pragma nowarn(1506)   // warning elimination 
	translator.WriteBlock(forceFilename_.c_string(), stringSize);
#pragma warn(1506)  // warning elimination 

	// Debug options 
 Int32 size = debugOptionList_.GetCount();
	translator.WriteBlock(&size,sizeof(Int32));

	DSListPosition pos = debugOptionList_.GetHeadPosition();
	while (NULL != pos)
	{
		DebugOption &opt = debugOptionList_.GetNext(pos);
		
		translator.WriteBlock(&(opt.testpoint_),sizeof(Int32));
		
		stringSize = opt.objName_.GetLength() + 1;
		translator.WriteBlock(&stringSize, sizeof(Int32));
#pragma nowarn(1506)   // warning elimination 
		translator.WriteBlock(opt.objName_.c_string(), stringSize);
#pragma warn(1506)  // warning elimination 
	}
}

//--------------------------------------------------------------------------//
//	CRUOptions::LoadData()
//--------------------------------------------------------------------------//
void CRUOptions::LoadData(CUOFsIpcMessageTranslator &translator)
{
	char buf[PACK_BUFFER_SIZE];
 Int32 stringSize;
	
	// Output filename
	translator.ReadBlock(&stringSize, sizeof(Int32));
#pragma nowarn(1506)   // warning elimination 
	translator.ReadBlock(buf, stringSize);
#pragma warn(1506)  // warning elimination 
	
	CDSString outFileName(buf);
	SetOutputFilename(outFileName);

	// Force filename
	translator.ReadBlock(&stringSize, sizeof(Int32));
#pragma nowarn(1506)   // warning elimination 
	translator.ReadBlock(buf, stringSize);
#pragma warn(1506)  // warning elimination 
	
	CDSString forceFileName(buf);
	SetForceFilename(forceFileName);

	// Debug options 
 Int32 size;
	translator.ReadBlock(&size,sizeof(Int32));

	for (Int32 i=0;i<size;i++)
	{
	 Int32 testpoint;
				
		translator.ReadBlock(&testpoint,sizeof(Int32));
		
		translator.ReadBlock(&stringSize, sizeof(Int32));
		
		RUASSERT(PACK_BUFFER_SIZE > stringSize);

#pragma nowarn(1506)   // warning elimination 
		translator.ReadBlock(buf, stringSize);
#pragma warn(1506)  // warning elimination 
		
		CDSString objName(buf);
		AddDebugOption(testpoint, objName);
	}
}
