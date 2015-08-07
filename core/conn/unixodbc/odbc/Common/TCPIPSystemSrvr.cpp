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
#include <fs\feerrors.h>
#include "zspic.h"
#include "zemsc.h"
#include "zinic.h"
#include "zmxoc.h"
#include "zcomc.h"
#include "ztcic.h"
#include "errno.h"

#include "ceercv.h"
#include "zsysc.h"

#include <windows.h>
#include "tal.h"
#include "global.h"
#include "odbcCommon.h"
#include "Transport.h"

#include "listener.h"
#include "swap.h"
#include "TCPIPSystemSrvr.h"


CTCPIPSystemSrvr::CTCPIPSystemSrvr()
{
}
CTCPIPSystemSrvr::CTCPIPSystemSrvr(const CEE_handle_def* call_id)
{
	memcpy(&m_call_id,call_id,sizeof(CEE_handle_def));
	m_wbuffer = NULL;
	m_rbuffer = NULL;
	m_rbuffer_length = 0;
	m_wbuffer_length = 0;
	m_curptr = NULL;
	m_curlength = 0;
	m_reply_length = 0;

	m_nSocketFnum = -2;
	m_IObuffer = new char[MAX_BUFFER_LENGTH];
	if (m_IObuffer == NULL)
		exit(0);
	next = NULL;
	m_trans_begin_tag = 0;
}

CTCPIPSystemSrvr::~CTCPIPSystemSrvr()
{
	if (m_IObuffer != NULL)
		delete m_IObuffer;
	if (m_nSocketFnum > 0)
	{
		shutdown(m_nSocketFnum, 2);
		FILE_CLOSE_(m_nSocketFnum);
//		ReleaseServer();
	}
	RESET_ERRORS((long)this);
	cleanup();
}

void CTCPIPSystemSrvr::cleanup()
{
	w_release();
	r_release();
}

char* CTCPIPSystemSrvr::w_allocate( int size)
{
	if (m_wbuffer != NULL)
		delete m_wbuffer;
	m_wbuffer = new char[size];
	if (m_wbuffer != NULL)
		m_wbuffer_length = size;
	else
		m_wbuffer_length = 0;
	return m_wbuffer;
}

char* CTCPIPSystemSrvr::r_allocate( int size)
{
	if (m_rbuffer != NULL)
		delete m_rbuffer;
	m_rbuffer = new char[size];
	if (m_rbuffer != NULL)
		m_rbuffer_length = size;
	else
		m_rbuffer_length = 0;
	return m_rbuffer;
}

void CTCPIPSystemSrvr::r_assign(char* buffer, long length)
{
	if (m_rbuffer != NULL)
		delete m_rbuffer;
	m_rbuffer = buffer;
	m_rbuffer_length = length;
}

void CTCPIPSystemSrvr::w_assign(char* buffer, long length)
{
	if (m_wbuffer != NULL)
		delete m_wbuffer;
	m_wbuffer = buffer;
	m_wbuffer_length = length;
}

void CTCPIPSystemSrvr::w_release()
{
	if (m_wbuffer != NULL)
		delete m_wbuffer;
	m_wbuffer = NULL;
	m_wbuffer_length = 0;
}

void CTCPIPSystemSrvr::r_release()
{
	if (m_rbuffer != NULL)
		delete m_rbuffer;
	m_rbuffer = NULL;
	m_wbuffer_length = 0;
}

char* CTCPIPSystemSrvr::w_buffer()
{
	return m_wbuffer;
}

char* CTCPIPSystemSrvr::r_buffer()
{
	return m_rbuffer;
}

long CTCPIPSystemSrvr::w_buffer_length()
{
	return m_wbuffer_length;
}

long CTCPIPSystemSrvr::r_buffer_length()
{
	return m_rbuffer_length;
}

char CTCPIPSystemSrvr::swap()
{
	return m_rhdr.swap;
}

char CTCPIPSystemSrvr::transport()
{
	return m_rhdr.transport;
}

void CTCPIPSystemSrvr::process_swap( char* buffer )
{
	PROCESS_res_swap(buffer, m_rhdr.operation_id); 
}

CTCPIPSystemSrvr_list::CTCPIPSystemSrvr_list()
{
	list=NULL;
}
CTCPIPSystemSrvr_list::~CTCPIPSystemSrvr_list()
{
	cleanup();
}
void CTCPIPSystemSrvr_list::cleanup() 
{
	CTCPIPSystemSrvr* cnode = list;
	CTCPIPSystemSrvr* nnode;
	while( cnode != NULL ){
		nnode = cnode->next;
		delete cnode;
		cnode = nnode;
	}
	list=NULL;
}

bool CTCPIPSystemSrvr_list::isListEmpty()
{
	return list==NULL;
}

CTCPIPSystemSrvr* CTCPIPSystemSrvr_list::ins_node( const CEE_handle_def* call_id )
{
	CTCPIPSystemSrvr* cnode = list;
	CTCPIPSystemSrvr* pnode = list;
	CTCPIPSystemSrvr* nnode;

	del_node(call_id);

	while(cnode!=NULL )
	{
		pnode=cnode;
		cnode=cnode->next;
	}
	if((nnode = (CTCPIPSystemSrvr*) new CTCPIPSystemSrvr(call_id))!=NULL)
	{
		nnode->next = cnode;
		if(pnode!=NULL) 
			pnode->next = nnode;
		else
			list = nnode;
	}
	return nnode;
}

bool CTCPIPSystemSrvr_list::del_node(CTCPIPSystemSrvr* ipnode)
{
	CTCPIPSystemSrvr* cnode = list;
	CTCPIPSystemSrvr* pnode = list;
	while( cnode!= NULL )
	{
		if ( cnode == ipnode )
			break;

		pnode = cnode;
		cnode = cnode->next;
	}
	if( cnode==NULL )
		return false;
	if (pnode == list && cnode == list)
		list = cnode->next;
	else
		pnode->next = cnode->next;
	delete cnode;
	return true;
}

bool CTCPIPSystemSrvr_list::del_node(const CEE_handle_def* call_id)
{
	CTCPIPSystemSrvr* cnode = list;
	CTCPIPSystemSrvr* pnode = list;
	while( cnode!= NULL )
	{
		if ( memcmp(&cnode->m_call_id, call_id, sizeof(CEE_handle_def)) == 0 )
			break;

		pnode = cnode;
		cnode = cnode->next;
	}
	if( cnode==NULL )
		return false;
	if (pnode == list && cnode == list)
		list = cnode->next;
	else
		pnode->next = cnode->next;
	delete cnode;
	return true;
}

CTCPIPSystemSrvr* CTCPIPSystemSrvr_list::find_node(const CEE_handle_def* call_id)
{
	CTCPIPSystemSrvr* cnode = list;

	while( cnode != NULL )
	{
		if ( memcmp(&cnode->m_call_id, call_id, sizeof(CEE_handle_def)) == 0 )
			break;
		cnode = cnode->next;
	}
	return cnode;
}

CTCPIPSystemSrvr* CTCPIPSystemSrvr_list::find_node(short nSocketFnum)
{
	CTCPIPSystemSrvr* cnode = list;

	while( cnode != NULL )
	{
		if ( cnode->m_nSocketFnum == nSocketFnum )
			break;
		cnode = cnode->next;
	}
	return cnode;
}

bool CTCPIPSystemSrvr_list::cleanup_node(const CEE_handle_def* call_id)
{
	CTCPIPSystemSrvr* pnode;

	if ((pnode = find_node(call_id)) != NULL)
		pnode->cleanup();
	return pnode != NULL;
}

char* CTCPIPSystemSrvr_list::enum_nodes(char* obuffer, FILE* fp)
{
	CTCPIPSystemSrvr* cnode = list;
	char* pbuffer = obuffer;
	int	ip;

	ip=sprintf(pbuffer,"\t%s\n","<TCPIP NODES>");
	while( cnode != NULL )
	{
		pbuffer +=ip;ip=sprintf(pbuffer,"\t\t%15.15s\t\t=\t\t%d\n","SocketFnum",cnode->m_nSocketFnum);

		cnode = cnode->next;
	}
	return pbuffer+ip;
}

void CTCPIPSystemSrvr::send_error(short error, short error_detail, const CEE_handle_def *call_id_)
{
	long tag;
	short fnum;
	_cc_status cc;
	short wcount;
	long wtimeout = 10 * 100;

	if (m_nSocketFnum < 0)
		goto bailout;
	m_rhdr.error = error;
	m_rhdr.error_detail = error_detail;
	m_rhdr.hdr_type = SRVR_TRANSPORT_ERROR;
	memcpy(m_IObuffer,&m_rhdr,sizeof(m_rhdr));
	if (m_rhdr.swap == SWAP_YES)
		HEADER_swap((HEADER*)m_IObuffer);
	if (send_nw2(m_nSocketFnum, m_IObuffer, sizeof(HEADER), 0, 1) <0)
		goto bailout;
	fnum = m_nSocketFnum;
	cc = AWAITIOX(&fnum,,&wcount, &tag, wtimeout);
	if (!_status_eq(cc))
		goto bailout;
bailout:
	DEALLOCATE_TEMP_MEMORY(&m_call_id);
}

CEE_status CTCPIPSystemSrvr::send_response(char* buffer, unsigned long message_length, const CEE_handle_def *call_id_)
{
	CEE_status retcode = WRITE_TCPIP_RESPONSE(this, message_length);
	DEALLOCATE_TEMP_MEMORY(&m_call_id);
	return retcode;
}

short int CTCPIPSystemSrvr::getMsgHeader(char* buf, short int cread, short int bufSize, short int headerSize, long wtimeout)
{
	long tag;
	_cc_status cc;
	short int error = 0;
	short int fnum;
	short int wcount;
	char* pbuf;
	short int inp_size;

	short int already_read = cread;

	while (already_read < headerSize )
	{
		pbuf = buf + already_read;
		inp_size = bufSize - already_read;

		if (recv_nw(m_nSocketFnum, pbuf, inp_size, 0, 0) < 0)
		{
			SET_ERROR((long)0, NSK, TCPIP, UNKNOWN_API, E_TCPIPROCESS, "getMsgHeader", O_DO_WRITE_READ, F_RECV, errno, 0);
			return -1;
		}
		fnum = m_nSocketFnum;
		cc = AWAITIOX(&fnum,,&wcount, &tag, wtimeout);
		if (!_status_eq(cc))
		{
			FILE_GETINFO_(fnum,&error);
			SET_ERROR((long)0, NSK, TCPIP, UNKNOWN_API, E_TCPIPROCESS, "getMsgHeader", O_DO_WRITE_READ, F_AWAITIOX, error, cc);
			return -1;
		}
		already_read += wcount;
	}
	return already_read;
}

bool CTCPIPSystemSrvr::do_read(bool bfirstblock, HEADER*& hdr, char*& buffer, short& bufcount, long timeout)
{
	long tag;
	_cc_status cc;
	short error = 0;
	short fnum;
	short wcount;
	long wtimeout = (timeout == 0)? -1: timeout * 100;
	wtimeout = -1;
	short sizeof_header = sizeof(HEADER);

	if (m_nSocketFnum < 0)
	{
		SET_ERROR((long)0, NSK, TCPIP, UNKNOWN_API, E_TCPIPROCESS, "do_read", O_DO_WRITE_READ, F_CHECKSOCKET, m_nSocketFnum, 0);
		return false;
	}

	if (bfirstblock)
	{
		if (bufcount < sizeof_header)
		{
			bufcount = getMsgHeader(m_IObuffer, bufcount, TCPI_SRVR_RECV_BUFFER, sizeof_header, wtimeout);
			if (bufcount == -1)
				return false;
		}
		wcount = bufcount;
		bufcount = wcount - sizeof(HEADER);
		buffer = m_IObuffer + sizeof(HEADER);
		hdr = (HEADER*)m_IObuffer;
	}
	else
	{
		if (recv_nw(m_nSocketFnum, m_IObuffer, TCPI_SRVR_RECV_BUFFER, 0, 0) < 0)
		{
			SET_ERROR((long)0, NSK, TCPIP, UNKNOWN_API, E_TCPIPROCESS, "do_read", O_DO_WRITE_READ, F_RECV, errno, 0);
			return false;
		}
		fnum = m_nSocketFnum;
		cc = AWAITIOX(&fnum,,&wcount, &tag, wtimeout);
		if (!_status_eq(cc))
		{
			FILE_GETINFO_(fnum,&error);
			SET_ERROR((long)0, NSK, TCPIP, UNKNOWN_API, E_TCPIPROCESS, "do_read", O_DO_WRITE_READ, F_AWAITIOX, error, cc);
			return false;
		}
		bufcount = wcount;
		buffer = m_IObuffer; 
	}
	return true;
}
bool CTCPIPSystemSrvr::do_write(char* buffer, short bufcount, long timeout)
{
	int count = bufcount;
	char* bp = 0;
	long tag;
	_cc_status cc;
	short error = 0;
	short fnum;
	short wcount;
	long wtimeout = (timeout == 0)? -1: timeout * 100;
	wtimeout = -1;

	if (m_nSocketFnum < 0)
	{
		SET_ERROR((long)0, NSK, TCPIP, UNKNOWN_API, E_TCPIPROCESS, "do_write", O_DO_WRITE_READ, F_CHECKSOCKET, m_nSocketFnum, 0);
		return false;
	}

	for (bp = &buffer[0]; count > 0; count -= cc)
	{
		if (send_nw2(m_nSocketFnum, bp, count, 0, 0) < 0)
		{
			SET_ERROR((long)0, NSK, TCPIP, UNKNOWN_API, E_TCPIPROCESS, "do_write", O_DO_WRITE_READ, F_SEND, errno, 0);
			return false;
		}
		fnum = m_nSocketFnum;
		cc = AWAITIOX(&fnum,,&wcount, &tag, wtimeout);
		if (!_status_eq(cc))
		{
			FILE_GETINFO_(fnum,&error);
			SET_ERROR((long)0, NSK, TCPIP, UNKNOWN_API, E_TCPIPROCESS, "do_write", O_DO_WRITE_READ, F_AWAITIOX, error, cc);
			return false;
		}
		cc = socket_get_len(m_nSocketFnum);
		if (cc < 0)
		{
			SET_ERROR((long)0, NSK, TCPIP, UNKNOWN_API, E_TCPIPROCESS, "do_write", O_DO_WRITE_READ, F_SOCKET_GET_LEN, error, cc);
			return false;
		}
		bp += cc;
	}
	return true;
}

//====================================================================================

bool READ_TCPIP_REQUEST(CTCPIPSystemSrvr* pnode)
{
	HEADER* hdr;
	char* buffer;
	short length;
	unsigned long total_length;

	RESET_ERRORS((long)pnode);

	length = pnode->m_rlength;
	if (pnode->do_read(true, hdr, buffer, length, READ_TIMEOUT)== false)
	{
		pnode->send_error(SRVR_ERR_READ_OPERATION,0, NULL);
		return false;
	}
	if (hdr->signature != SIGNATURE)
		return false;

	memcpy(&pnode->m_rhdr,hdr,sizeof(HEADER));
	if (pnode->m_rhdr.compress_ind == COMP_YES && pnode->m_rhdr.compress_type != 0)
		total_length = hdr->cmp_length;
	else
		total_length = hdr->total_length;
	if(pnode->r_allocate(total_length) == NULL)
	{
		pnode->send_error(SRVR_ERR_MEMORY_ALLOCATE,0, NULL);
		return false;
	}
	memcpy(pnode->m_rbuffer, buffer, length);
	pnode->m_curptr = pnode->m_rbuffer + length;
	total_length -= length;
	while(total_length > 0)
	{
		if (pnode->do_read(false, hdr, buffer, length, READ_TIMEOUT)== false)
		{
			pnode->send_error(SRVR_ERR_READ_OPERATION,0, NULL);
			return false;
		}
		memcpy(pnode->m_curptr, buffer, length);
		pnode->m_curptr += length;
		total_length -= length;
	}
	if (pnode->m_rhdr.compress_ind == COMP_YES && pnode->m_rhdr.compress_type != 0)
		return DoExpand(pnode, pnode->m_rhdr, pnode->m_rbuffer, total_length);
	return true;
}

int WRITE_TCPIP_RESPONSE(CTCPIPSystemSrvr* pnode, unsigned long message_length)
{
	HEADER* hdr;
	char* buffer;
	short length;
	unsigned long total_length;
	total_length = message_length;

	short retcode = 0;
	short tx_handle[10];

	memset(&tx_handle[0],0,20);
	retcode = TMF_GETTXHANDLE_(&tx_handle[0]);
	if (retcode == 0)
		TMF_BEGINTAG_FROM_TXHANDLE_(&tx_handle[0], &pnode->m_trans_begin_tag);
	else
		pnode->m_trans_begin_tag = 0;		

	RESET_ERRORS((long)pnode);

	buffer = pnode->m_wbuffer;
	hdr = (HEADER*)pnode->m_wbuffer;
	memcpy(hdr, &pnode->m_rhdr, sizeof(HEADER));
	hdr->total_length = message_length - sizeof(HEADER);
	if (hdr->compress_ind == COMP_YES && hdr->total_length > MIN_LENGTH_FOR_COMPRESSION)
	{
		total_length -= sizeof(HEADER);
		DoCompression(pnode, hdr, (unsigned char*)(buffer+sizeof(HEADER)), (unsigned long&)total_length);
		total_length += sizeof(HEADER);
	}
	else
		hdr->compress_type = 0;

	memcpy(&pnode->m_whdr, hdr, sizeof(HEADER));

	if (hdr->swap == SWAP_YES)
		HEADER_swap(hdr);

	while (total_length > 0)
	{
		if (total_length > TCPI_SRVR_SEND_BUFFER)
			length = TCPI_SRVR_SEND_BUFFER;
		else
			length = total_length;

		if (pnode->do_write(buffer, length, WRITE_TIMEOUT)== false)
		{
			pnode->send_error(SRVR_ERR_WRITE_OPERATION,0, NULL);
			return 1;
		}
		total_length -= length;
		buffer += length;
	}
	return 0;
}

void* PROCESS_TCPIP_REQUEST(CTCPIPSystemSrvr* pnode)
{
	if (READ_TCPIP_REQUEST(pnode) == false)
	{
		GTransport.m_TCPIPSystemSrvr_list->del_node(&pnode->m_call_id);
		return NULL;
	}
	BUILD_TCPIP_REQUEST(pnode);
	if (pnode->m_nSocketFnum < 0)
	{
		GTransport.m_TCPIPSystemSrvr_list->del_node(&pnode->m_call_id);
		return NULL;
	}
	pnode->cleanup();
	return pnode;
}

void BUILD_TCPIP_REQUEST(CTCPIPSystemSrvr* pnode)
{
	CEE_tag_def* objtag_ = (CEE_tag_def*)pnode;
	const CEE_handle_def *call_id_ = &pnode->m_call_id;
	short operation_id = pnode->m_rhdr.operation_id;

	if(pnode->m_trans_begin_tag != 0)
		RESUMETRANSACTION(pnode->m_trans_begin_tag);
	DISPATCH_TCPIPRequest(objtag_, call_id_, operation_id);
}

void DoCompression(CTCPIPSystemSrvr* pnode, HEADER* wheader, unsigned char* wbuffer, unsigned long& write_count)
{
	bool retcode = true;
	unsigned long uncmp_count = write_count;

	unsigned char* cmp_buf = (unsigned char*)new char[write_count];
	if (cmp_buf == NULL)
	{
		wheader->compress_type = 0;
		wheader->cmp_length = 0;
		return;
	}

	retcode = pnode->m_compression.compress((unsigned char*)wbuffer, uncmp_count, cmp_buf, write_count);
	if (retcode == false || uncmp_count <= write_count)
	{
		delete cmp_buf;
		wheader->compress_type = 0;
		wheader->cmp_length = 0;
		write_count = uncmp_count;
		return;
	}
	memcpy(wbuffer, cmp_buf, write_count);
	delete cmp_buf;
	wheader->compress_type = COMP_12;
	wheader->cmp_length = write_count;
}

bool DoExpand(CTCPIPSystemSrvr* pnode, HEADER& rheader, unsigned char* ibuffer, unsigned long& output_size)
{
	bool retcode = true;
	unsigned char* obuffer = (unsigned char*)new char[rheader.total_length];
	if (obuffer == NULL)
	{
		pnode->send_error(SRVR_ERR_MEMORY_ALLOCATE,(unsigned short)rheader.total_length, NULL);
		return false;
	}
	retcode = pnode->m_compression.expand(ibuffer, obuffer, output_size);
	if (retcode == false || output_size != rheader.total_length)
	{
		delete obuffer;
		pnode->send_error(SRVR_ERR_EXPAND_OPERATION,0, NULL);
		return false;
	}
	pnode->r_assign(obuffer, rheader.total_length);
	rheader.compress_type = 0;
	rheader.cmp_length = 0;
	return true;
}


