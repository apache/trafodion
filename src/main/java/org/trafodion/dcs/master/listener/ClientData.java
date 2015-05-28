/**
 *(C) Copyright 2013 Hewlett-Packard Development Company, L.P.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package org.trafodion.dcs.master.listener;

import java.net.*;
import java.io.*;
import java.nio.*;
import java.nio.channels.*;
import java.nio.channels.spi.*;
import java.util.*;
import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.trafodion.dcs.zookeeper.ZkClient;

public class ClientData {
	private static  final Log LOG = LogFactory.getLog(ClientData.class);

	ByteBuffer header = null;
	ByteBuffer body = null;
	ByteBuffer[] buf = null;
	int total_read;
	int total_write;
	int buffer_state = ListenerConstants.BUFFER_INIT;

	Header hdr = null;
	ConnectionContext conectContex = null;

	SocketAddress clientSocketAddress = null;

	ClientData(SocketAddress clientSocketAddress){

		header = ByteBuffer.allocate(ListenerConstants.HEADER_SIZE);
		body = ByteBuffer.allocate(ListenerConstants.BODY_SIZE);
		buf = new ByteBuffer[]{header,body};

		total_read = 0;
		total_write = 0;

		hdr = new Header();
		conectContex = new ConnectionContext();

		this.clientSocketAddress = clientSocketAddress;
	}

	ByteBuffer[] getByteBufferArray(){
		return buf;
	}

	void setByteBufferHeader(ByteBuffer header){
		this.header = header;
	}

	void setByteBufferBody(ByteBuffer body){
		this.body = body;
	}
}
