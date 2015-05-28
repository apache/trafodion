// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2003-2014 Hewlett-Packard Development Company, L.P.
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

#ifndef TCPIP_SYSTEM_DRVR_H
#define TCPIP_SYSTEM_DRVR_H

#include "Compression.h"

class CConnect;
class CStmt;
//------------------------- classes for TCPIP File System ---------------------

class CTCPIPSystemDrvr_list;

class CTCPIPSystemDrvr: public CInterface 
{

public:
	CTCPIPSystemDrvr();
	~CTCPIPSystemDrvr();
	void		CloseSessionWithCleanup();
	void		cleanup(void);

	virtual	void process_swap( char* buffer );
	virtual	char* w_allocate(int s);
	virtual char* r_allocate(int s);
	virtual void w_release();
	virtual void r_release();
	virtual void w_assign(char* buffer,long length);
	virtual void r_assign(char* buffer,long length);
	virtual char* w_buffer();
	virtual char* r_buffer();
	virtual long w_buffer_length();
	virtual long r_buffer_length();
	virtual	char swap();
	virtual void setSwap(char swap);
    virtual void resetSwap();
	virtual	char transport();
	virtual	void send_error(short error, short error_detail, const CEE_handle_def *call_id_);
	virtual	CEE_status send_response(char* buffer, unsigned long message_length, const CEE_handle_def *call_id_);

	short		odbcAPI;
	DIALOGUE_ID_def dialogueId;
	long		dwTimeout;

	SOCKET		m_hSocket;
	HANDLE		m_hEvents[2];

	char*		m_rbuffer;
	long		m_rbuffer_length;
	char*		m_wbuffer;
	long		m_wbuffer_length;
	char*		m_IObuffer;
	HEADER		m_rheader;
	char		m_object_ref[MAX_OBJECT_REF + 1];
	int          m_IOCompression;
	CCompression m_compression;
private:
	char		m_swap;

	CTCPIPSystemDrvr*	next;

	friend CTCPIPSystemDrvr_list;
};

class CTCPIPSystemDrvr_list
{
public:
	CTCPIPSystemDrvr_list();
	~CTCPIPSystemDrvr_list();
	void cleanup();

	CTCPIPSystemDrvr* ins_node();
	bool	del_node(CTCPIPSystemDrvr* p );
	bool	find_node(CTCPIPSystemDrvr* p );
	CTCPIPSystemDrvr* find_node(char* object_ref );

	CRITICAL_SECTION CSTCPIP;
private:
	CTCPIPSystemDrvr* list;
};

extern 
bool DoIO (CTCPIPSystemDrvr* pTCPIPSystem, char* wbuffer, long write_count, char*& rbuffer, long& read_count,CConnect *pConnection,CStmt *pStatement = NULL);
extern 
bool OpenIO (CTCPIPSystemDrvr* pTCPIPSystem, char* object_ref);
extern 
void CloseIO (CTCPIPSystemDrvr* pTCPIPSystem);
extern
void DoCompression(CTCPIPSystemDrvr* pTCPIPSystem, HEADER& wheader, unsigned char* wbuffer, unsigned long& write_count);
extern
bool DoExpand(CTCPIPSystemDrvr* pTCPIPSystem, HEADER& rheader, unsigned char* rbuffer, unsigned long& read_count);
extern "C" { 
void WINAPI TCPIP_SET_ERROR(long signature, char platform, char transport, int api, ERROR_TYPE error_type, char* process, OPERATION operation, FUNCTION function, int error, int errordetail);
}

#endif
