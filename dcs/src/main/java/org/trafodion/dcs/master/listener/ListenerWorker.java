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
import java.util.LinkedList;
import java.util.List;
import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.apache.zookeeper.ZooDefs;
import org.apache.zookeeper.data.Stat;
import org.apache.zookeeper.CreateMode;
import org.apache.zookeeper.ZooKeeper;
import org.trafodion.dcs.zookeeper.ZkClient;

public class ListenerWorker extends Thread {
	private static  final Log LOG = LogFactory.getLog(ListenerWorker.class);
	private List<DataEvent> queue = new LinkedList<DataEvent>();
	private ZkClient zkc=null;
	ConnectReply connectReplay = null;
	private String parentZnode;

	ListenerWorker(ZkClient zkc,String parentZnode){	
		this.zkc=zkc;
		this.parentZnode=parentZnode;
		connectReplay = new ConnectReply(zkc,parentZnode);
	}

	public void processData(ListenerService server, SelectionKey key) {
		synchronized(queue) {
			queue.add(new DataEvent(server, key));
			queue.notify();
		}
	}

	public void run() {
		DataEvent dataEvent;

		while(true) {
			// Wait for data to become available
			synchronized(queue) {
				while(queue.isEmpty()) {
					try {
						queue.wait();
					} catch (InterruptedException e) {
					}
				}
				dataEvent = queue.remove(0);
			}
			boolean cancelConnection = false;
			SelectionKey key = dataEvent.key;
			SocketChannel client = (SocketChannel) key.channel();
			Socket s = client.socket();
			ClientData clientData = (ClientData) key.attachment();
			ListenerService server = dataEvent.server;
			dataEvent.key = null;
			dataEvent.server = null;

			try {
				boolean replyException = buildConnectReply(clientData);
				key.attach(clientData);
				// Return to sender for writing
				if (replyException == false)
					server.send(new PendingRequest(key, ListenerConstants.REQUST_WRITE));
				else
					server.send(new PendingRequest(key, ListenerConstants.REQUST_WRITE_EXCEPTION));
			} catch (UnsupportedEncodingException ue){
				LOG.error("Exception in buildConnectReply: " + s.getRemoteSocketAddress() + ": " + ue.getMessage() );
				cancelConnection = true;
			} catch (IOException io){
				LOG.error("Exception in buildConnectReply: " + s.getRemoteSocketAddress() + ": " + io.getMessage());
				cancelConnection = true;
			}
			if (cancelConnection == true){
				// Return to sender for closing
				server.send(new PendingRequest(key, ListenerConstants.REQUST_CLOSE));
			}
		}
	}
	boolean buildConnectReply(ClientData clientData) throws UnsupportedEncodingException, IOException {

		boolean replyException = false;

		ByteBuffer header = clientData.header;
		ByteBuffer body = clientData.body;
		Header hdr = clientData.hdr;
		ConnectionContext conectContex = clientData.conectContex;
		SocketAddress clientSocketAddress = null;
		clientSocketAddress = clientData.clientSocketAddress;

		header.flip();
		hdr.extractFromByteArray(header);
		body.flip();
		conectContex.extractFromByteBuffer(body);

		replyException = connectReplay.buildConnectReply(hdr, conectContex, clientSocketAddress);

		header.clear();
		body.clear();

		switch(hdr.getVersion()){

			case ListenerConstants.CLIENT_HEADER_VERSION_BE: //from jdbc
				hdr.setSwap(ListenerConstants.YES);
				header.order(ByteOrder.BIG_ENDIAN);
				body.order(ByteOrder.LITTLE_ENDIAN);
				hdr.setVersion(ListenerConstants.SERVER_HEADER_VERSION_LE);
				break;
			case ListenerConstants.CLIENT_HEADER_VERSION_LE: //from odbc
				hdr.setSwap(ListenerConstants.NO);
				header.order(ByteOrder.LITTLE_ENDIAN);
				body.order(ByteOrder.LITTLE_ENDIAN);
				hdr.setVersion(ListenerConstants.SERVER_HEADER_VERSION_LE);
				break;
			default:
				throw new IOException(clientSocketAddress + ": " + "Wrong Header Version");
		}

		connectReplay.insertIntoByteBuffer(body);
		body.flip();

		hdr.setTotalLength(body.limit());
		hdr.insertIntoByteBuffer(header);
		header.flip();

		clientData.header = header;
		clientData.body = body;
		clientData.hdr = hdr;
		clientData.conectContex = conectContex;

		header = null;
		body = null;
		hdr = null;
		conectContex = null;
		clientSocketAddress = null;

		return replyException;
	}
}

