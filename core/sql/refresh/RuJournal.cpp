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
* File:         RuJournal.cpp
* Description:  Implementation of class CRUJournal
*				
*
* Created:      03/23/2000
* Language:     C++
* 
*
* 
******************************************************************************
*/

#include "dsstring.h"

#include "RuException.h"
#include "RuJournal.h"
#include "RuGlobals.h"
#include "logmxevent.h"

//--------------------------------------------------------------------------//
//	Constructor and destructor
//--------------------------------------------------------------------------//

CRUJournal::CRUJournal(const CDSString& fname) :
	fname_(fname), 
	logfile_(),
        // if the user does not specify an OUTFILE option REFRESH messages are only sent to EMS
	emsOnlyLog_(fname.IsEmpty()?TRUE:FALSE),
	rowNum_(0)
{}

CRUJournal::~CRUJournal()
{
	Close();
}

//--------------------------------------------------------------------------//
//	CRUJournal::Open()
//
//	Open the file and print the opening message
//--------------------------------------------------------------------------//

void CRUJournal::Open()
{
	RUASSERT(FALSE == logfile_.IsOpen());

        // if the user does not specify an OUTFILE option REFRESH messages are only sent to EMS
        if (!emsOnlyLog_)
        {
	   logfile_.OpenWOHeader(fname_.c_string(), CDSLogfile::eWrite);
	   if (FALSE == logfile_.IsOpen())
	   {
		   // Failed to open the file
		   CDSException ex;
		   ex.SetError(IDS_RU_OUTFILE_FAILED);
		   ex.AddArgument(fname_);
   
		   throw ex;
	   }
        }
	LogMessage(CDSString(RefreshDiags[0] + CDSString("\n")));
	SetTimePrint(FALSE);

	rowNum_ = 0;
}

//--------------------------------------------------------------------------//
//	CRUJournal::Close()
//--------------------------------------------------------------------------//

void CRUJournal::Close()
{
	SetTimePrint(TRUE);
	LogMessage(RefreshDiags[1] + CDSString("\n"));

        // if the user specified an OUTFILE option and the file is open
	if ((!emsOnlyLog_) && (logfile_.IsOpen()))
	  logfile_.Close();
}

//--------------------------------------------------------------------------//
//      CRUJournal::LogMessage()
// Logs messages that will go to the refresh log file to EMS
//--------------------------------------------------------------------------//

void CRUJournal::DumpToEMS (const char* eventMsg, BOOL isAnError)
{
   switch (isAnError)
   {
      case FALSE:
          SQLMXLoggingArea::logMVRefreshInfoEvent(eventMsg);
       break;
      case TRUE:
          SQLMXLoggingArea::logMVRefreshErrorEvent(eventMsg);
       break;
      default: // do nothing 
       break;
   }

}

//--------------------------------------------------------------------------//
//	CRUJournal::LogMessage()
//--------------------------------------------------------------------------//

void CRUJournal::LogMessage(const CDSString& msg,
                            BOOL printRowNum,
                            BOOL isError /* is this an error msg or just an informational msg */)
{
	if (TRUE == printRowNum)
	{
		char row[20];
		sprintf(row, "%d", ++rowNum_);
		CDSString tempStr(row);
		tempStr += ": " + msg;
		if (tempStr[(Lng32)tempStr.GetLength()] != '\n')
			tempStr += "\n";
		const char *buf = tempStr.c_string();

                // if the user does not specify an OUTFILE option REFRESH messages are only sent to EMS
                if ((!emsOnlyLog_) && (logfile_.IsOpen()))
		   logfile_.WriteWOHeader(buf, (Lng32)strlen(buf));

                // avoid logging to EMS messages that only contain the new line character
                if ((strlen(buf)) > 1)
                   DumpToEMS (buf, isError);
	}
	else
	{
		// The CDSLogfile class has changed to support a generic
		// logging operation, in order for refresh to have the 
		// same logging behavior, refresh needs to add a timestamp
		// to the log message (if requested).
		// In addition the lower level calls are used - 
		// CDSLogfile::WriteWOHeader instead of CDSLogfile::Write. This 
		// was done to avoid log header issues.
		// However, refresh needs to add a new line (if needed)
		// and call flush before returning.
		CDSString newString;
		if (logfile_.GetTimePrint())
			newString =+ logfile_.GetTimeToLog().c_string();
		newString += msg.c_string();

		// Add a new line to prevent lines from running together
		if (newString[(Lng32)newString.GetLength()] != '\n')
			newString += "\n";
		const char *buf = newString.c_string();

                // if the user does not specify an OUTFILE option REFRESH messages are only sent to EMS
                if ((!emsOnlyLog_) && (logfile_.IsOpen()))
		   logfile_.WriteWOHeader(buf, (Lng32)strlen(buf));

                // avoid logging to EMS messages that only contain the new line character
                if ((strlen(buf)) > 1)
                   DumpToEMS (buf, isError);
	}

        // if the user specified an OUTFILE option and the file is open
        if ((!emsOnlyLog_) && (logfile_.IsOpen()))
	   logfile_.Flush();
}

//--------------------------------------------------------------------------//
//	CRUJournal::LogError(CDSException &ex)
//
//	Log all the messages from the exception object,	top-down (stacktrace).
//--------------------------------------------------------------------------//

void CRUJournal::LogError(CDSException &ex)
{
	enum { BUFSIZE = 4096 };

	Int32 nerr = ex.GetNumErrors();
	char buffer[BUFSIZE];
	CDSString msg;

	for (Int32 i=0; i<nerr; i++) 
	{
		ex.GetErrorMsg(i, buffer, BUFSIZE);
		
		if (buffer[0] != 0)
		{
			// Clear the trailing whitespace
			char *p = buffer + strlen(buffer) - 1;
			for (;buffer != p && isspace((unsigned char)*p); p--, *p=0); // For VS2003
		}

		msg += buffer + CDSString("\n");
	}

        LogMessage(msg, FALSE /* don't print row number */, TRUE /* it is an error msg */);
}
