/**
* @@@ START COPYRIGHT @@@

Licensed to the Apache Software Foundation (ASF) under one
or more contributor license agreements.  See the NOTICE file
distributed with this work for additional information
regarding copyright ownership.  The ASF licenses this file
to you under the Apache License, Version 2.0 (the
"License"); you may not use this file except in compliance
with the License.  You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing,
software distributed under the License is distributed on an
"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
KIND, either express or implied.  See the License for the
specific language governing permissions and limitations
under the License.

* @@@ END COPYRIGHT @@@
 */
package org.trafodion.dcs.rest;

import java.io.IOException;
import java.net.InetSocketAddress;
import java.util.StringTokenizer;
import java.util.List;
import java.util.Iterator;
import java.util.HashMap; 
import java.util.Map;

//import org.apache.avro.ipc.NettyTransceiver;
//import org.apache.avro.ipc.Transceiver;
//import org.apache.avro.ipc.specific.SpecificRequestor;
//import org.apache.avro.AvroRemoteException;

import org.trafodion.dcs.Constants;
import org.trafodion.dcs.rest.RestConstants;
import org.trafodion.dcs.zookeeper.ZkClient;

import org.apache.zookeeper.KeeperException;
import org.apache.zookeeper.data.Stat;
import org.apache.zookeeper.ZooDefs;

import org.apache.hadoop.conf.Configuration;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;


public class ServerConnector {
	private static final Log LOG = LogFactory.getLog(ServerConnector.class);
//	private Transceiver transceiver;
//	private Workload client;
//	private WorkloadListRequest request;
/*
	public ServerConnector(String ipAddr,int port) throws IOException {
		try {
			transceiver = new NettyTransceiver(new InetSocketAddress(ipAddr,port));
			client = SpecificRequestor.getClient(Workload.class, transceiver);
//			request = WorkloadListRequest.newBuilder().build();

		} catch (AvroRemoteException e) {
			LOG.error(e);
		} 
	}
	
	public WorkloadListResponse getWorkloadListResponse() {
		WorkloadListResponse response = null;
		try {
			response = client.list(request);
		} catch (AvroRemoteException e) {
			LOG.error(e);
		} 
		return response;
	}
*/	
}
