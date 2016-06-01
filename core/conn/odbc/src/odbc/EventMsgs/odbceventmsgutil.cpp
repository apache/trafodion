/*************************************************************************
*
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
**************************************************************************/
//
// FILE: odbcEventMsgUtil.cpp
//

#include "tdm_logevent.h"
#include "tdm_odbcSrvrMsg.h"
#include "odbcEventMsgUtil.h"


#define EVENTMSG_ERROR_MSGBOX_TITLE	"ODBC/MX Event Message Handling Error"

LogEvent  *EventLogPtr = NULL;

//////////////////////////////////////////////////////////////////////////
//
// Name:    DllMain
//
// Purpose: Initialize the logging class 
//      
// Input:   None
//
// Output:  TRUE for success
//          FALSE for failure
//
//////////////////////////////////////////////////////////////////////////
BOOL   WINAPI   DllMain (HANDLE hInst, 
                        ULONG ul_reason_for_call,
                        LPVOID lpReserved)
{

	HMODULE    msgHandle  = (HMODULE)(INVALID_HANDLE_VALUE);
	const char evSource[] = { "ODBC/MX" };	
	char       msgFile[MAX_PATH+1];
	BOOL       result;
	DWORD retLength;
	
	msgFile[0] = '\0';
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		// Create an instance of the LogEvent class.
		EventLogPtr = new LogEvent();

		// Ensure that the new operation is successful.
		if (!EventLogPtr)
			return TRUE;

		// Get a handle to our message DLL.	
		
		msgHandle = LoadLibrary("tdm_odbcSrvrMsg_intl.dll");
		if (!msgHandle)
		{
			PVOID lpMsgBuf;
			FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
				NULL,
				GetLastError(),
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
				(LPTSTR) &lpMsgBuf,
				0,
				NULL);

			/////////////////////////////////////////////////////////////////////////
			// Note: MessageBox call is a temporary solution for now.
			// We need to change this call later when ODBC is starting as a service.
			/////////////////////////////////////////////////////////////////////////
#ifdef _DEBUG			
			MessageBox(NULL, (const char*) lpMsgBuf, EVENTMSG_ERROR_MSGBOX_TITLE, 
						MB_TASKMODAL  | MB_SERVICE_NOTIFICATION);		
			MessageBox(NULL, "Load Library of tdm_odbcSevrMsg_init.dll failed",
						EVENTMSG_ERROR_MSGBOX_TITLE, MB_TASKMODAL  | MB_SERVICE_NOTIFICATION);
#endif
			LocalFree(lpMsgBuf);
		}
		else
		{
			
			if ((retLength = GetModuleFileName(msgHandle, msgFile, sizeof(msgFile))) == 0)
			{
				
				PVOID lpMsgBuf;
				FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
					NULL,
					GetLastError(),
					MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
					(LPTSTR) &lpMsgBuf,
					0,
					NULL);
			
				/////////////////////////////////////////////////////////////////////////
				// Note: MessageBox call is a temporary solution for now.
				// We need to change this call later when ODBC is starting as a service.
				/////////////////////////////////////////////////////////////////////////
#ifdef _DEBUG
				MessageBox(NULL, (const char*) lpMsgBuf, EVENTMSG_ERROR_MSGBOX_TITLE, 
						MB_TASKMODAL   | MB_SERVICE_NOTIFICATION);
				MessageBox(NULL, "GetModuleFileName failed",
						EVENTMSG_ERROR_MSGBOX_TITLE, 
						MB_TASKMODAL   | MB_SERVICE_NOTIFICATION);
#endif
				LocalFree(lpMsgBuf);
			}
			else
			{
				// Don't be tempted to close the DLL handle -- bad things happen.	
				// Now register the event source with this DLL as the message store
				// If Registration fails in the debug mode, a message box will be displayed.
				if (!(result = EventLogPtr->Registration( msgFile , evSource )))
				{
#ifdef _DEBUG			
					MessageBox(NULL, "Registration of tdm_odbcSevrMsg_init.dll failed",
							EVENTMSG_ERROR_MSGBOX_TITLE, MB_TASKMODAL  | MB_SERVICE_NOTIFICATION);
#endif
					delete EventLogPtr;
					EventLogPtr = NULL;
				}
			}
		}
		return TRUE;
	case DLL_PROCESS_DETACH:
		if (EventLogPtr)
		{
			// Finally, before the program terminates, it's nice to clean up.
			EventLogPtr->DeRegister();
			delete EventLogPtr;
			EventLogPtr = NULL;
		}
		return TRUE;
	default:
		return TRUE;
	}
}

//////////////////////////////////////////////////////////////////////////
//
// Name:    SendODBCEventMsg
//
// Purpose: Add a specified type of parameter to an event.
//          This logs a message with (any) token parameters previously 
//          specified by the Add* methods.
//
// Note:    Parameters in event messages are specified in the message 
//          template by using the form "%n", where "n" indicates the 
//          relative call to AddToken which supplies the value for the 
//          parameter.  Thus, the first AddToken call will provide the 
//          value for token %1, the second for token %2, etc.
//          The maximum number of tokens is defined in teventlog.h.
//          Token is a null-terminated string that becomes the parameter 
//          value.
//
// Input:   EventId - numeric value for the event.  This corresponds
//                    to the value defined by the message compiler for 
//                    the event.
//          EventLogType - The type of the event being logged
//                         (e.g., EVENTLOG_INFORMATION_TYPE, 
//                         EVENTLOG_WARNING_TYPE, and
//                         EVENTLOG_ERROR_TYPE)
//          Pid - Process ID
//          ComponentName - ODBC/MX Component (e.g., Association Server)
//          ObjectRef - Object Reference
//          nToken - Number of additional tokens
//
// Output:  None
//
//////////////////////////////////////////////////////////////////////////
extern "C"
void SendODBCEventMsg(DWORD EventId, 
					  short EventLogType, 
					  DWORD Pid, 
					  char *ComponentName, 
					  char *ObjectRef, 
					  char *ClusterName, 
					  int  NodeId, 
					  short nToken,
					  va_list vl)
{
	BOOL result = FALSE;
	char *TokenPtr;

	if (!EventLogPtr)
		return; 

	// Provide an EventID number to be used with the "logging resumed"
    // event.  This event is posted if logging becomes enabled after it had been
    // previously disabled due to a resource constraint, such as the log being full.
    // If not invoked, then no event is generated under these conditions.

	EventLogPtr->SetResumeEvent(LogEventLoggingResumed);	

	//////////////////////////////////////////////////////////
	// Note: 
	// AddInteger and AddToken will display a message box 
	// for the debug version if an error occurred.
	// Therefore, we do not need to show the reason here;
	// we can just simply "return."
	//////////////////////////////////////////////////////////

	// Pid is %1.
	if (Pid != 0)
	{
		if (!(result = EventLogPtr->AddInteger(Pid)))
			return;
	}

	// ComponentName is %2.
	if (ComponentName != NULL)
	{
		if (!(result = EventLogPtr->AddToken(ComponentName)))
			return;
	}

	// ObjectRef is %3.
	if (ObjectRef != NULL)
	{
		if (!(result = EventLogPtr->AddToken(ObjectRef)))
			return;
	}

	// ClusterName is %4.
	if (ClusterName!= NULL)
	{
		if (!(result = EventLogPtr->AddToken(ClusterName)))
			return;
	}

	// NodeId is %5
	if (NodeId != -1)
	{
		if (!(result = EventLogPtr->AddInteger(NodeId)))
			return;
	}
	else
	{
		char szNodeId[20];
		strcpy(szNodeId, "Unknown");
		if (!(result = EventLogPtr->AddToken(szNodeId)))
			return;
	}

	
	for (int i = 0; i<nToken; i++)
	{
		// Process additional tokens.
		// Start with %6, ...
		TokenPtr = va_arg(vl, char *);
		if (!(result = EventLogPtr->AddToken( TokenPtr)))
		{
			// An error has occurred.
			va_end(vl);
			return;
		}
	}

	// Now log the event.
	EventLogPtr->Send( EventId, EventLogType );

	va_end(vl);
}





