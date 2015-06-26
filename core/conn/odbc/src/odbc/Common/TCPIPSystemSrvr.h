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

#ifndef TCPIP_SYSTEM_SRVR_H
#define TCPIP_SYSTEM_SRVR_H

#include "Transport.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define READ_TIMEOUT 10
#define WRITE_TIMEOUT 10

#include "compression.h"


class CTCPIPSystemSrvr_list; 

class CTCPIPSystemSrvr: public CInterface
{
public:
	CTCPIPSystemSrvr();
	CTCPIPSystemSrvr(short nSocketFnum);
	~CTCPIPSystemSrvr();
	void	cleanup();

	virtual void process_swap( char* buffer );
	virtual char* w_allocate( int size);
	virtual char* r_allocate( int size);
	virtual void w_release();
	virtual void r_release();
	virtual void w_assign(char* buffer, long length);
	virtual void r_assign(char* buffer, long length);
	virtual char* w_buffer();
	virtual char* r_buffer();
	virtual long w_buffer_length();
	virtual long r_buffer_length();
	virtual char swap();
	virtual char transport();
	virtual void send_error(short error, short error_detail, const CEE_handle_def *call_id_);
	virtual CEE_status	send_response(char* buffer, unsigned long message_length, const CEE_handle_def *call_id_);

	bool	do_read(bool bfirstblock, HEADER*& hdr, char*& buffer, int& bufcount, long timeout);
	bool	do_write(char* buffer, int bufcount, short operation_id, long timeout);
	short int getMsgHeader(char* pbuf, int cread, int bufSize, int headerSize, long wtimeout);
    void error_cleanup(void);
	HEADER	m_rhdr;
	HEADER	m_whdr;
	CEE_handle_def m_call_id;

	char*	m_rbuffer;
	long	m_rbuffer_length;
	char*	m_wbuffer;
	long	m_wbuffer_length;
	char*	m_curptr;
	long	m_curlength;
	unsigned long	m_reply_length;
	int		m_rlength;
	short	m_nSocketFnum;
	char*	m_IObuffer;
	CCompression m_compression;
	long	m_trans_begin_tag;
//On SQ, we use select to monitor completion. We need to be 
//able to traverse the list to compare which open sockets have
//been signalled as ready - which is why we will need to make
//'next' public (we could have introduced a get() function too)
//private:
	CTCPIPSystemSrvr*	next;

	friend class CTCPIPSystemSrvr_list;
};

class CTCPIPSystemSrvr_list 
{
public:
	CTCPIPSystemSrvr_list();
	~CTCPIPSystemSrvr_list();
	void	cleanup(); 
	CTCPIPSystemSrvr*	ins_node( short nSocketFnum);
	CTCPIPSystemSrvr*	find_node(short nSocketFnum);
	bool	del_node(CTCPIPSystemSrvr* pnode);
    bool    del_node(short nSocketFnum);
	bool	isListEmpty();
	char* enum_nodes(char* obuffer, FILE* fp);
	inline CTCPIPSystemSrvr* get_list() { return list; }
   CTCPIPSystemSrvr* get_next(){ 
		if(m_node_iterator)
			m_node_iterator=m_node_iterator->next;
		else
			m_node_iterator=list;
		return m_node_iterator; 
	}
	CTCPIPSystemSrvr* move_to_begin(){ m_node_iterator = list; return m_node_iterator; }
	CTCPIPSystemSrvr*	m_current_node;
private:
	CTCPIPSystemSrvr*	list;
	CTCPIPSystemSrvr*	m_node_iterator;
};
bool 
READ_TCPIP_REQUEST(CTCPIPSystemSrvr* pnode);
int 
WRITE_TCPIP_RESPONSE(CTCPIPSystemSrvr* pnode, unsigned long message_length);
void*
PROCESS_TCPIP_REQUEST(CTCPIPSystemSrvr* pnode);
void 
BUILD_TCPIP_REQUEST(CTCPIPSystemSrvr* pnode);

extern void 
DISPATCH_TCPIPRequest( 
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  ,	short operation_id
);

extern void
ReleaseServer();

extern
void DoCompression(CTCPIPSystemSrvr* pnode, HEADER* wheader, unsigned char* wbuffer, unsigned long& write_count);
extern
bool DoExpand(CTCPIPSystemSrvr* pnode, HEADER& rheader, unsigned char* rbuffer, unsigned long& read_count);

#endif
