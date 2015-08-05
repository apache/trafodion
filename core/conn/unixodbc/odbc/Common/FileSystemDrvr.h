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

#ifndef FILE_SYSTEM_DRVR_H
#define FILE_SYSTEM_DRVR_H

//------------------------- classes for File System ---------------------

class CFSystemDrvr_list;

class CFSystemDrvr: public CInterface
{

public:
	CFSystemDrvr();
	~CFSystemDrvr();
	bool		OpenSession(char* process);
	void		CloseSession();
	void		CloseSessionWithCleanup();
	bool		DoWriteRead(HEADER*& hdr, char*& buffer, short& count, long timeout);

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
	virtual	char transport();
	virtual	void send_error(short error, short error_detail, const CEE_handle_def *call_id_);
	virtual	CEE_status send_response(char* buffer, unsigned long message_length, const CEE_handle_def *call_id_);
private:
public:
	short		odbcAPI;
	DIALOGUE_ID_def dialogueId;
	long		dwTimeout;
private:
	char*		m_rbuffer;
	char*		m_wbuffer;
	long		m_rbuffer_length;
	long		m_wbuffer_length;
	char*		m_IObuffer;
	char		m_process_name[MAX_PROCESS_NAME + 1];
	short		m_pHandle[10];
	short		m_filenum;
	CFSystemDrvr*	next;

	friend CFSystemDrvr_list;
};

class CFSystemDrvr_list
{
public:
	CFSystemDrvr_list();
	~CFSystemDrvr_list();
	void cleanup();

	CFSystemDrvr* ins_node();
	bool	del_node(CFSystemDrvr* p );
	bool	find_node(CFSystemDrvr* p );
	CFSystemDrvr* find_node(char* processname );
private:
	CFSystemDrvr* list;
};

extern 
bool DoIO (CFSystemDrvr* pFSystem, char* wbuffer, long write_count, char*& rbuffer, long& read_count);
extern 
bool OpenIO (CFSystemDrvr* pFSystem, char* process);
extern 
void CloseIO (CFSystemDrvr* pFSystem);

#endif
