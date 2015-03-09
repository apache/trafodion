// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2003-2015 Hewlett-Packard Development Company, L.P.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//
// @@@ END COPYRIGHT @@@

#ifndef TRANSPORT_H
#define TRANSPORT_H

#include "TransportBase.h"

#define _GUARDIAN_SOCKETS
#include "Listener_srvr.h"
#include "Global.h"

class CNSKListenerSrvr;
class CFSystemDrvr;
class CFSystemDrvr_list;
class CFSystemSrvr_list;
class CTimer_list;
class CTCPIPSystemSrvr_list;

class CTransport: public CTransportBase
{
public:
	CTransport();
	~CTransport();

	CNSKListenerSrvr*	    m_listener;
	CFSystemDrvr_list*		m_FSystemDrvr_list;
	CFSystemSrvr_list*		m_FSystemSrvr_list;
	CTCPIPSystemSrvr_list*	m_TCPIPSystemSrvr_list;
	CTimer_list*			m_Timer_list;
#ifdef NSK_PLATFORM
	virtual short AWAITIOX(short* filenum,short* wcount, long* tag, long wtimeout);
#endif
	CFSystemDrvr*			m_asFSystemDrvr;
	void log_error(CError* ierror);
    void initialize(void);
	void log_info(CError* ierror);
	void log_warning(CError* ierror);

	bool m_tcpip_blocked;

	//phandle change for SQ
	TPT_DECL(myHandle);
	TCPU_DECL(myCpu);
	TCPU_DECL(myProcessId);

	int  myNodenumber;
	char  myProcname[128];
	char  myNodename[128];
	char  myProgramFile[128];
	char  myPathname[128];
	short error;
	char  error_message[100];
	SB_Thread::Mutex m_TransportCSObject;

protected:

private:
};

extern void LOG_ERROR(CError* ierror);
extern void LOG_INFO(CError* ierror);
extern void LOG_WARNING(CError* ierror);
extern CTransport GTransport;


#endif
