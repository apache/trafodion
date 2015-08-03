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
#ifndef _RU_JOURNAL_H_
#define _RU_JOURNAL_H_

/* -*-C++-*-
******************************************************************************
*
* File:         RuJournal.h
* Description:  Message file mechanism of the REFRESH utility
*
*
* Created:      03/23/2000
* Language:     C++
* 
*
* 
******************************************************************************
*/

#include "refresh.h"
#include "dsstring.h"
#include "dslog.h"
#include "dserror.h"

//--------------------------------------------------------------------------//
//	CRUJournal
//	
//	This class is an abstraction for the REFRESH utility's output file.
//--------------------------------------------------------------------------//

class REFRESH_LIB_CLASS CRUJournal {

public:
	CRUJournal(const CDSString& fname);
	~CRUJournal();

public:
	void Open();
	void Close();

public:
        // the isError flag is used to indicate whether this message corresponds to an error or not
	void LogMessage(const CDSString& msg, BOOL printRowNum = FALSE, BOOL isError = FALSE);

	void LogError(CDSException &ex);

        // dump error/messages to EMS
        void DumpToEMS (const char* eventMsg, BOOL isAnError);

	// Control the current time printout
	void SetTimePrint(BOOL flag) 
	{ 
		logfile_.SetTimePrint(flag); 
	}

private:
	//-- Prevent copying
	CRUJournal(const CRUJournal &other);
	CRUJournal &operator = (const CRUJournal &other);

private:
	CDSString fname_;
	CDSLogfile logfile_;
	
	// Print numbering counter
	Int32 rowNum_;

        // log to ems only unless the user specifies an OUTFILE option
        // in which case the specified file will also be used to log
        // refresh messages
        BOOL emsOnlyLog_;
};

#endif
