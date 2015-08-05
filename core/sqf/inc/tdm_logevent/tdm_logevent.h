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

#pragma once
#include "seaquest/sqtypes.h"

#ifndef TDM_LOGEVENT_API_GENERATOR
#define TDM_LOGEVENT_API _declspec(dllimport)
#else
#define TDM_LOGEVENT_API _declspec(dllexport)
#endif

class TDM_LOGEVENT_API LogEvent
  {
  private:
	enum  { max_tokens = 32 };          // Max number of tokens per message
    enum  { max_token_len = 4096 };      // Maximum token size. SQL requested 4KB size

    BOOL    log_suspended;              // TRUE if logging disabled

    short   nTokens;                    // Number of tokens accumulated

    HANDLE  hLog;                       // Handle for application log

    char    source_name[260];           // Source identification for messages

    WCHAR  *Tokens[max_tokens];         // Tokens (insertion strings) accumulated for message

    DWORD   log_error;                  // Last error from ReportEvent failure

    DWORD   dwLogResume_EventID;        // Message EventID  for "logging has resumed" event

    // Called by the constructor to initialize the various data members.
	void    Initialize();
	// Releases all memory that may be in use to store tokens.
	void	EmptyAllTokens();
	// Present a message box for debugging in the Debug version of the library.
    void    Event_MessageBox(const char * format, ... );

  public:
	  // These Categories values must correspond to the MessageId defined in
	  // msg.mc for this product.
	enum Categories
	{
		NonStopGeneral=1,
		SoftwareFailure=2,
		TransientFault=3,
		ServiceUnavailable=4,
		ServiceAvailable=5,
		OtherStateChange=6,
		OperatorActionNeeded=7,
		OperatorActionCompleted=8,
		UsageThreshold=9,
		TraceData=10
	};


     LogEvent();
    ~LogEvent();

    BOOL    Registration(const char * MessageFileName, const char * EventSource);
    void    DeRegister(void);
    BOOL    AddToken(const char * Token);    // Add ASCII string token.
	BOOL	AddTokenW(const WCHAR * Token ); // Add UNICODE string token.
    BOOL    AddInteger(DWORD Value);         // Add integer token.
    BOOL    AddFileName(char * FileName);    // Add Guardian-style name token.
    BOOL    Send(DWORD EventId, short Type);
    BOOL    Send(DWORD EventId, short Type, short wCategory,
		         long dwDataSize, void *lpRawData );
    BOOL    Active(void)
              { return (hLog != NULL); };
    void    SetResumeEvent(DWORD dwLogResume_EventID);

  };
