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
********************************************************************/

#include <platform_ndcs.h>
#include "TCPIPSystemSrvr.h"
#include "swap.h"
#include <errno.h>

extern fd_set read_fds;
extern int max_read_fd;

void CTCPIPSystemSrvr::error_cleanup()
{
	if (m_nSocketFnum > 0)
	{
	    GTransport.m_TCPIPSystemSrvr_list->del_node(m_nSocketFnum);
	}
}

void CTCPIPSystemSrvr::send_error(short error, short error_detail, const CEE_handle_def *call_id_)
{
	if (m_nSocketFnum >= 0)
    {
        m_rhdr.error = error;
        m_rhdr.error_detail = error_detail;
        m_rhdr.hdr_type = SRVR_TRANSPORT_ERROR;
        memcpy(m_IObuffer,&m_rhdr,sizeof(m_rhdr));
	    if (m_rhdr.swap == SWAP_YES)
		   HEADER_swap((HEADER*)m_IObuffer);
        if (send(m_nSocketFnum, m_IObuffer, sizeof(HEADER), 0) <= 0)
        {   // Error in send
            error_cleanup();
        }
    }

	if (GTransport.m_TempMemory_list.m_list_length)
		DEALLOCATE_TEMP_MEMORY(&m_call_id);
}

short int CTCPIPSystemSrvr::getMsgHeader(char* buf, int cread, int bufSize, int headerSize, long wtimeout)
{
	ssize_t wcount;
	char* pbuf;
	int inp_size;

	int already_read = cread;

	while (already_read < headerSize )
	{
		pbuf = buf + already_read;
		inp_size = bufSize - already_read;

		if ((wcount = recv(m_nSocketFnum, pbuf, inp_size, 0)) <= 0)
		{
			SET_ERROR((long)this, NSK, TCPIP, UNKNOWN_API, E_TCPIPROCESS, "getMsgHeader - TCP/IP connection closed be the user", O_DO_WRITE_READ, F_AWAITIOX, 0, 0);
            error_cleanup();
            return -1;
		}
		already_read += wcount;
	}
	return (short) already_read;
}



bool CTCPIPSystemSrvr::do_read(bool bfirstblock, HEADER*& hdr, char*& buffer, int& bufcount, long timeout)
{
	ssize_t wcount;
	long wtimeout = (timeout == 0)? -1: timeout * 100;
	wtimeout = -1;
	int sizeof_header = sizeof(HEADER);

	if (m_nSocketFnum < 0)
	{
		SET_ERROR((long)this, NSK, TCPIP, UNKNOWN_API, E_TCPIPROCESS, "do_read", O_DO_WRITE_READ, F_CHECKSOCKET, m_nSocketFnum, 0);
		goto bailout;
	}

	if (bfirstblock)
	{
		if (bufcount < sizeof_header)
		{
			bufcount = getMsgHeader(m_IObuffer, bufcount, MAX_TCP_BUFFER_LENGTH, sizeof_header, wtimeout);
			if (bufcount == -1)
				goto bailout;
		}
		bufcount = bufcount - sizeof(HEADER);

		buffer = m_IObuffer + sizeof(HEADER);
		hdr = (HEADER*)m_IObuffer;
	}
	else
	{
		if ((wcount = recv(m_nSocketFnum, m_IObuffer, MAX_TCP_BUFFER_LENGTH, 0)) <= 0)
        {
			SET_ERROR((long)this, NSK, TCPIP, hdr->operation_id, E_TCPIPROCESS, "do_read - TCP/IP connection closed be the user", O_DO_WRITE_READ, F_AWAITIOX, 0, 0);
			goto bailout;
		}
		bufcount = wcount;
		buffer = m_IObuffer; 
	}
	return true;
bailout:
    error_cleanup();
	return false;

}

bool CTCPIPSystemSrvr::do_write(char* buffer, int bufcount, short operation_id, long timeout)
{
	int count = bufcount;
	char* bp = 0;
	ssize_t wcount;

	if (m_nSocketFnum < 0)
	{
		SET_ERROR((long)this, NSK, TCPIP, operation_id, E_TCPIPROCESS, "do_write", O_DO_WRITE_READ, F_CHECKSOCKET, m_nSocketFnum, 0);
		return false;
	}

	for (bp = &buffer[0]; count > 0; count -= wcount)
	{
		int retries = 0;
		do
		{
			wcount = send(m_nSocketFnum, bp, count, 0);
		} while ((wcount < 0) && (errno == EINTR) && (retries++ < 3));

		if (wcount <= 0)
		{
			SET_ERROR((long)this, NSK, TCPIP, operation_id, E_TCPIPROCESS, "do_write", O_DO_WRITE_READ, F_SEND, errno, wcount);

            error_cleanup();

            return false;

		}
		bp += wcount;
	}
	return true;
}
