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

#include <platform_ndcs.h>
#include "errno.h"
#include "ceercv.h"

#include "Global.h"
#include "odbcCommon.h"
#include "Transport.h"
#include "DrvrSrvr.h"

#include "Listener.h"
#include "swap.h"
#include "TCPIPSystemSrvr.h"


CTCPIPSystemSrvr::CTCPIPSystemSrvr()
{
}
CTCPIPSystemSrvr::CTCPIPSystemSrvr(short nSocketFnum)
{
	SRVRTRACE_ENTER(FILE_TSS+21);
	m_wbuffer = NULL;
	m_rbuffer = NULL;
	m_rbuffer_length = 0;
	m_wbuffer_length = 0;
	m_curptr = NULL;
	m_curlength = 0;
	m_reply_length = 0;

	m_nSocketFnum = nSocketFnum;
	m_IObuffer = new char[MAX_TCP_BUFFER_LENGTH];
	if (m_IObuffer == NULL)
		exit(0);
	next = NULL;
	m_trans_begin_tag = 0;

	GTransport.m_TCPIPSystemSrvr_list->m_current_node = this;

	SRVRTRACE_EXIT(FILE_TSS+21);
}

CTCPIPSystemSrvr::~CTCPIPSystemSrvr()
{
	SRVRTRACE_ENTER(FILE_TSS+22);
	if (m_IObuffer != NULL)
		delete m_IObuffer;
	if (m_nSocketFnum > 0)
	{
        // We will need to get the socket file descriptor out
        // of the fd watch list which select() monitors
		GTransport.m_listener->closeTCPIPSession(m_nSocketFnum);
		m_nSocketFnum = -2;
	}
	if(GTransport.m_error_list.m_list_length)
		RESET_ERRORS((long)this);
	w_release();
	r_release();

	GTransport.m_TCPIPSystemSrvr_list->m_current_node = NULL;
	SRVRTRACE_EXIT(FILE_TSS+22);
//	PRINTSRVRTRC
}

void CTCPIPSystemSrvr::cleanup()
{
//	w_release();
//	r_release();
}

char* CTCPIPSystemSrvr::w_allocate( int size)
{
	SRVRTRACE_ENTER(FILE_TSS+23);
	if (m_wbuffer != NULL && size <= m_wbuffer_length)
	{
		SRVRTRACE_EXIT(FILE_TSS+23);
		return m_wbuffer;
	}

	if (m_wbuffer != NULL)
		delete m_wbuffer;
	m_wbuffer = new char[size];
	if (m_wbuffer != NULL)
		m_wbuffer_length = size;
	else
		m_wbuffer_length = 0;
	SRVRTRACE_EXIT(FILE_TSS+23);
	return m_wbuffer;
}

char* CTCPIPSystemSrvr::r_allocate( int size)
{
	SRVRTRACE_ENTER(FILE_TSS+24);
	if (m_rbuffer != NULL && size <= m_rbuffer_length)
	{
		SRVRTRACE_EXIT(FILE_TSS+24);
		return m_rbuffer;
	}

	if (m_rbuffer != NULL)
		delete m_rbuffer;
	m_rbuffer = new char[size];
	if (m_rbuffer != NULL)
		m_rbuffer_length = size;
	else
		m_rbuffer_length = 0;
	SRVRTRACE_EXIT(FILE_TSS+24);
	return m_rbuffer;
}

void CTCPIPSystemSrvr::r_assign(char* buffer, long length)
{
	SRVRTRACE_ENTER(FILE_TSS+25);
	if (m_rbuffer != NULL)
		delete m_rbuffer;
	m_rbuffer = buffer;
	m_rbuffer_length = length;
	SRVRTRACE_EXIT(FILE_TSS+25);
}

void CTCPIPSystemSrvr::w_assign(char* buffer, long length)
{
	SRVRTRACE_ENTER(FILE_TSS+26);
	if (m_wbuffer != NULL)
		delete m_wbuffer;
	m_wbuffer = buffer;
	m_wbuffer_length = length;
	SRVRTRACE_EXIT(FILE_TSS+26);
}

void CTCPIPSystemSrvr::w_release()
{
	SRVRTRACE_ENTER(FILE_TSS+27);
	if (m_wbuffer != NULL)
		delete m_wbuffer;
	m_wbuffer = NULL;
	m_wbuffer_length = 0;
	SRVRTRACE_EXIT(FILE_TSS+27);
}

void CTCPIPSystemSrvr::r_release()
{
	SRVRTRACE_ENTER(FILE_TSS+28);
	if (m_rbuffer != NULL)
		delete m_rbuffer;
	m_rbuffer = NULL;
	m_rbuffer_length = 0;
	SRVRTRACE_EXIT(FILE_TSS+28);
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
	SRVRTRACE_ENTER(FILE_TSS+29);
	return m_rhdr.transport;
	SRVRTRACE_EXIT(FILE_TSS+29);
}

void CTCPIPSystemSrvr::process_swap( char* buffer )
{
	SRVRTRACE_ENTER(FILE_TSS+1);
// the server does not swap - keeping it in place just
// in case this is being used somewhere
	SRVRTRACE_EXIT(FILE_TSS+1);
}

CTCPIPSystemSrvr_list::CTCPIPSystemSrvr_list()
{
	list=NULL;
	m_current_node = NULL;
	m_node_iterator = NULL;
}
CTCPIPSystemSrvr_list::~CTCPIPSystemSrvr_list()
{
	cleanup();
}
void CTCPIPSystemSrvr_list::cleanup() 
{
	SRVRTRACE_ENTER(FILE_TSS+2);
	CTCPIPSystemSrvr* cnode = list;
	CTCPIPSystemSrvr* nnode;
	while( cnode != NULL ){
		nnode = cnode->next;
		delete cnode;
		cnode = nnode;
	}
	list=NULL;
	m_node_iterator=NULL;
	SRVRTRACE_EXIT(FILE_TSS+2);
}

bool CTCPIPSystemSrvr_list::isListEmpty()
{
	return list==NULL;
}

CTCPIPSystemSrvr* CTCPIPSystemSrvr_list::ins_node( short nSocketFnum )
{
	SRVRTRACE_ENTER(FILE_TSS+3);

	CTCPIPSystemSrvr* cnode = list;
	CTCPIPSystemSrvr* pnode = list;
	CTCPIPSystemSrvr* nnode;

	del_node(nSocketFnum);

	while(cnode!=NULL )
	{
		pnode=cnode;
		cnode=cnode->next;
	}
	if((nnode = (CTCPIPSystemSrvr*) new CTCPIPSystemSrvr(nSocketFnum))!=NULL)
	{
		nnode->next = cnode;
		if(pnode!=NULL) 
			pnode->next = nnode;
		else
			list = nnode;
	}
	SRVRTRACE_EXIT(FILE_TSS+3);
	return nnode;
}

bool CTCPIPSystemSrvr_list::del_node(CTCPIPSystemSrvr* ipnode)
{
	SRVRTRACE_ENTER(FILE_TSS+4);
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
	{
		SRVRTRACE_EXIT(FILE_TSS+4);
		return false;
	}
	if (pnode == list && cnode == list)
	{
		list = cnode->next;
		if(m_node_iterator==ipnode)
		{
			m_node_iterator = NULL;
		}
	}
	else
	{
		pnode->next = cnode->next;
		if(m_node_iterator==ipnode)
		{
			m_node_iterator = pnode;
		}
	}
	delete cnode;
	SRVRTRACE_EXIT(FILE_TSS+4);
	return true;
}

bool CTCPIPSystemSrvr_list::del_node(short nSocketFnum)
{
	SRVRTRACE_ENTER(FILE_TSS+4);

	CTCPIPSystemSrvr* cnode = list;
	CTCPIPSystemSrvr* pnode = list;

	while( cnode!= NULL )
	{
		if ( cnode->m_nSocketFnum == nSocketFnum )
			break;

		pnode = cnode;
		cnode = cnode->next;
	}

	if( cnode==NULL )
	{
		SRVRTRACE_EXIT(FILE_TSS+4);
		return false;
	}

	if (pnode == list && cnode == list)
	{
		list = cnode->next;
		if(m_node_iterator==cnode)
		{
			m_node_iterator = NULL;
		}
	}
	else
	{
		pnode->next = cnode->next;
		if(m_node_iterator==cnode)
		{
			m_node_iterator = pnode;
		}
	}
	delete cnode;

	SRVRTRACE_EXIT(FILE_TSS+4);
	return true;
}

CTCPIPSystemSrvr* CTCPIPSystemSrvr_list::find_node(short nSocketFnum)
{
	SRVRTRACE_ENTER(FILE_TSS+7);
	CTCPIPSystemSrvr* cnode = list;

	while( cnode != NULL )
	{
		if ( cnode->m_nSocketFnum == nSocketFnum )
			break;
		cnode = cnode->next;
	}
	SRVRTRACE_EXIT(FILE_TSS+7);
	return cnode;
}

char* CTCPIPSystemSrvr_list::enum_nodes(char* obuffer, FILE* fp)
{
	SRVRTRACE_ENTER(FILE_TSS+9);
	CTCPIPSystemSrvr* cnode = list;
	char* pbuffer = obuffer;
	int	ip;

	ip=sprintf(pbuffer,"\t%s\n","<TCPIP NODES>");
	while( cnode != NULL )
	{
		pbuffer +=ip;ip=sprintf(pbuffer,"\t\t%15.15s\t\t=\t\t%d\n","SocketFnum",cnode->m_nSocketFnum);

		cnode = cnode->next;
	}
	SRVRTRACE_EXIT(FILE_TSS+9);
	return pbuffer+ip;
}

//void CTCPIPSystemSrvr::send_error(short error, short error_detail, const CEE_handle_def *call_id_)
// Moving this to platform specific implementation file

CEE_status CTCPIPSystemSrvr::send_response(char* buffer, unsigned long message_length, const CEE_handle_def *call_id_)
{
	SRVRTRACE_ENTER(FILE_TSS+11);
	CEE_status retcode = WRITE_TCPIP_RESPONSE(this, message_length);
	if (GTransport.m_TempMemory_list.m_list_length )
		DEALLOCATE_TEMP_MEMORY(&m_call_id);
	SRVRTRACE_EXIT(FILE_TSS+11);
	return retcode;
}

//short int CTCPIPSystemSrvr::getMsgHeader(char* buf, int cread, int bufSize, int headerSize, long wtimeout)
// Moving this to platform specific implementation file

//bool CTCPIPSystemSrvr::do_read(bool bfirstblock, HEADER*& hdr, char*& buffer, int& bufcount, long timeout)
// Moving this to platform specific implementation file

//bool CTCPIPSystemSrvr::do_write(char* buffer, int bufcount, short operation_id, long timeout)
// Moving this to platform specific implementation file

//====================================================================================

bool READ_TCPIP_REQUEST(CTCPIPSystemSrvr* pnode)
{
	SRVRTRACE_ENTER(FILE_TSS+15);
	HEADER* hdr;
	HEADER swappedHdr;
	char* buffer;
	int length;
	unsigned long total_length;

	if(GTransport.m_error_list.m_list_length)
		RESET_ERRORS((long)pnode);

	length = pnode->m_rlength;
	if (pnode->do_read(true, hdr, buffer, length, READ_TIMEOUT)== false)
	{
		// if there is error writing to a socket, makes no sense trying to write an error response to the socket
		// (if there is a send error, we'll actually cleanup the tcp/ip session, so there is no pnode either)
		SRVRTRACE_EXIT(FILE_TSS+15);
		return false;
	}

	if (hdr->signature != SIGNATURE)
	{
		memcpy(&swappedHdr,hdr,sizeof(HEADER));
		HEADER_swap(&swappedHdr);

		if(swappedHdr.signature != SIGNATURE)
		{
		pnode->send_error(SRVR_ERR_WRONG_MESSAGE_FORMAT, 0, NULL);
		SRVRTRACE_EXIT(FILE_TSS+15);
		return false;
	}
		else
			memcpy(&pnode->m_rhdr,&swappedHdr,sizeof(HEADER));

	}
	else
	memcpy(&pnode->m_rhdr,hdr,sizeof(HEADER));
	if(pnode->m_rhdr.operation_id == AS_API_GETOBJREF)
	{
		if(pnode->m_rhdr.version == CLIENT_HEADER_VERSION_LE)
			pnode->m_rhdr.swap = SWAP_NO;
		else if (pnode->m_rhdr.version == CLIENT_HEADER_VERSION_BE)
			pnode->m_rhdr.swap = SWAP_YES;
		else
		{
			// reject older clients
			pnode->send_error(SRVR_ERR_WRONG_MESSAGE_FORMAT, 0, NULL);
			SRVRTRACE_EXIT(FILE_TSS+15);
			return false;
		}
		pnode->m_rhdr.version = SERVER_HEADER_VERSION_LE;

	}

	hdr = &pnode->m_rhdr;
	if (pnode->m_rhdr.compress_ind == COMP_YES && pnode->m_rhdr.compress_type != COMP_NO_COMPRESSION)
		total_length = hdr->cmp_length;
	else
		total_length = hdr->total_length;
	if(pnode->r_allocate(total_length) == NULL)
	{
		pnode->send_error(SRVR_ERR_MEMORY_ALLOCATE,0, NULL);
		SRVRTRACE_EXIT(FILE_TSS+15);
		return false;
	}
	if (length < 0 || length > total_length)
	{
		pnode->send_error(SRVR_ERR_WRONG_MESSAGE_FORMAT,0, NULL);
		SRVRTRACE_EXIT(FILE_TSS+15);
		return false;
	}
	memcpy(pnode->m_rbuffer, buffer, length);
	pnode->m_curptr = pnode->m_rbuffer + length;
	total_length -= length;
	while(total_length > 0)
	{
		if (pnode->do_read(false, hdr, buffer, length, READ_TIMEOUT)== false)
		{
			// if there is error writing to a socket, makes no sense trying to write an error response to the socket
			// (if there is a send error, we'll actually cleanup the tcp/ip session, so there is no pnode either)
			SRVRTRACE_EXIT(FILE_TSS+15);
			return false;
		}

		if (length < 0 || length > total_length)
		{
			pnode->send_error(SRVR_ERR_WRONG_MESSAGE_FORMAT,0, NULL);
			SRVRTRACE_EXIT(FILE_TSS+15);
			return false;
		}
		memcpy(pnode->m_curptr, buffer, length);
		pnode->m_curptr += length;
		total_length -= length;
	}
	if (pnode->m_rhdr.compress_ind == COMP_YES && pnode->m_rhdr.compress_type != COMP_NO_COMPRESSION)
	{
		SRVRTRACE_EXIT(FILE_TSS+15);
		return DoExpand(pnode, pnode->m_rhdr, (unsigned char *)pnode->m_rbuffer, total_length);
	}
	SRVRTRACE_EXIT(FILE_TSS+15);
	return true;
}

int WRITE_TCPIP_RESPONSE(CTCPIPSystemSrvr* pnode, unsigned long message_length)
{
	SRVRTRACE_ENTER(FILE_TSS+16);
	HEADER* hdr;
	char* buffer;
	int length;
	unsigned long total_length;
	total_length = message_length;

	short retcode = 0;
	short tx_handle[10];

	memset(&tx_handle[0],0,20);

	retcode = GETTRANSID(&tx_handle[0]);

	if (retcode == 0)
		pnode->m_trans_begin_tag = tx_handle[0];
	else
		pnode->m_trans_begin_tag = 0;		

	if(GTransport.m_error_list.m_list_length)
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
		hdr->compress_type = COMP_NO_COMPRESSION;

	memcpy(&pnode->m_whdr, hdr, sizeof(HEADER));

	if (hdr->swap == SWAP_YES)
		HEADER_swap(hdr);

	while (total_length > 0)
	{
		if (total_length > MAX_TCP_BUFFER_LENGTH)
			length = MAX_TCP_BUFFER_LENGTH;
		else
			length = total_length;

		if (pnode->do_write(buffer, length, pnode->m_whdr.operation_id, WRITE_TIMEOUT)== false)
		{
			// if there is error writing to a socket, makes no sense trying to write an error response to the socket
			// (if there is a send error, we'll actually cleanup the tcp/ip session, so there is no pnode either)
			SRVRTRACE_EXIT(FILE_TSS+16);
			return 1;
		}

		total_length -= length;
		buffer += length;
	}
	SRVRTRACE_EXIT(FILE_TSS+16);
	return 0;
}

void* PROCESS_TCPIP_REQUEST(CTCPIPSystemSrvr* pnode)
{
	SRVRTRACE_ENTER(FILE_TSS+17);
	if (READ_TCPIP_REQUEST(pnode) == false)
	{
		GTransport.m_TCPIPSystemSrvr_list->del_node(pnode);
		SRVRTRACE_EXIT(FILE_TSS+17);
		return NULL;
	}
	BUILD_TCPIP_REQUEST(pnode);
	if (pnode->m_nSocketFnum < 0)
	{
		GTransport.m_TCPIPSystemSrvr_list->del_node(pnode);
		SRVRTRACE_EXIT(FILE_TSS+17);
		return NULL;
	}
//	pnode->cleanup();
	SRVRTRACE_EXIT(FILE_TSS+17);
	return pnode;
}

void BUILD_TCPIP_REQUEST(CTCPIPSystemSrvr* pnode)
{
	SRVRTRACE_ENTER(FILE_TSS+18);
	CEE_tag_def* objtag_ = (CEE_tag_def*)pnode;
	const CEE_handle_def *call_id_ = &pnode->m_call_id;
	short operation_id = pnode->m_rhdr.operation_id;
	short tx_handle[10];

	if(pnode->m_trans_begin_tag != 0 && GETTRANSID(&tx_handle[0]) != 0)
		RESUMETRANSACTION(pnode->m_trans_begin_tag);
	DISPATCH_TCPIPRequest(objtag_, call_id_, operation_id);
	SRVRTRACE_EXIT(FILE_TSS+18);
}

/* 
 *   write_count: on input  contains the input size (i.e. uncompressed size)
 *              : on output contains the size after compression
 */
void DoCompression(CTCPIPSystemSrvr* pnode, HEADER* wheader, unsigned char* wbuffer, unsigned long& write_count)
{
	SRVRTRACE_ENTER(FILE_TSS+19);
	bool retcode = true;
	unsigned long inCmpCount = write_count; // In number of bytes 

	unsigned char* cmp_buf = NULL;

	retcode = pnode->m_compression.compress((unsigned char*)wbuffer,  // input buffer (data to be compressed)
																  (unsigned int)inCmpCount,              // input number of bytes
																  (int)COMP_DEFAULT,
	                                               (unsigned char**)&cmp_buf,                 // output buffer containing compressed output
																  (unsigned long&)write_count);            // input/output param - input == max size, on output contains compressed size

	if (retcode == false)
	{
		delete[] cmp_buf;
		wheader->compress_type = COMP_NO_COMPRESSION;
		wheader->cmp_length = 0;
		write_count = inCmpCount;
		SRVRTRACE_EXIT(FILE_TSS+19);
		return;
	}
	memcpy(wbuffer, cmp_buf, write_count);

	delete[] cmp_buf; //allocated in m_compression.compress();
	wheader->compress_type=COMP_DEFAULT;
	wheader->cmp_length = write_count;
	SRVRTRACE_EXIT(FILE_TSS+19);
}

bool DoExpand(CTCPIPSystemSrvr* pnode, HEADER& rheader, unsigned char* ibuffer, unsigned long& output_size)
{
	SRVRTRACE_ENTER(FILE_TSS+20);
	bool retcode;
	unsigned char* obuffer;
	int error=0;
	char* tcp_obuffer=NULL;

	if(rheader.compress_ind == COMP_YES )
	{
	  /*
	   * check that the compression types is something that the driver understands
	   */ 
	   if((rheader.compress_type < COMP_DEFAULT) ||
		   (rheader.compress_type > COMP_BEST_COMPRESSION))
	   {
		   pnode->send_error(SRVR_ERR_EXPAND_OPERATION,0, NULL);
		   SRVRTRACE_EXIT(FILE_TSS+20);
		   return false;
	   }
	}
	obuffer = (unsigned char*)new char[rheader.total_length +512]; // +512 is just to be safe
	                                                               // (in case something goes wrong in decompression)
	if (obuffer == NULL)
	{
		pnode->send_error(SRVR_ERR_MEMORY_ALLOCATE,(unsigned short)rheader.total_length, NULL);
		SRVRTRACE_EXIT(FILE_TSS+20);
		return false;
	}

	output_size = rheader.total_length; 
	retcode = pnode->m_compression.expand((unsigned char*)ibuffer,            // input compressed buffer    
		                                          (unsigned long)rheader.cmp_length, // input compresses length
		                                          (unsigned char**)&obuffer,            // output buffer after decompression
															   (unsigned long&)output_size,
																(int&)error);       // output size after decompression (should be equal to rheader.total_length)
	//output_size could be 0 or rheader.total_length, no other value.
	if (retcode == false || output_size == 0 )
	{
		delete[] obuffer;
		pnode->send_error(SRVR_ERR_EXPAND_OPERATION,0, NULL);
		SRVRTRACE_EXIT(FILE_TSS+20);
		return false;
	}
	pnode->r_assign((char*)obuffer, output_size);
	rheader.compress_type = COMP_NO_COMPRESSION;
	rheader.cmp_length = 0;

	SRVRTRACE_EXIT(FILE_TSS+20);
	return true;
}
