/**
* @@@ START COPYRIGHT @@@
*
* Licensed to the Apache Software Foundation (ASF) under one
* or more contributor license agreements.  See the NOTICE file
* distributed with this work for additional information
* regarding copyright ownership.  The ASF licenses this file
* to you under the Apache License, Version 2.0 (the
* "License"); you may not use this file except in compliance
* with the License.  You may obtain a copy of the License at
*
*   http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing,
* software distributed under the License is distributed on an
* "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
* KIND, either express or implied.  See the License for the
* specific language governing permissions and limitations
* under the License.
*
* @@@ END COPYRIGHT @@@
**/

/*
package org.trafodion.wms.rest;

import java.io.IOException;
import java.util.List;
import org.mortbay.jetty.Connector;
import org.mortbay.jetty.Server;
import org.mortbay.jetty.nio.SelectChannelConnector;
import org.apache.hadoop.conf.Configuration;
import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.trafodion.wms.util.WmsConfiguration;
import org.trafodion.wms.util.VersionInfo;
import org.trafodion.wms.server.WorkloadsQueue;

public class RestServer implements Runnable {  
	private static final Log LOG = LogFactory.getLog(RestServer.class.getName());
	private Configuration conf;
	private WorkloadsQueue wlq;
	private Thread thrd;
	
	public RestServer(WorkloadsQueue wlq){
	    conf = WmsConfiguration.create();
	    this.wlq = wlq; 
		thrd = new Thread(this);
		thrd.start();
	}
	
	public void run() {
		VersionInfo.logVersion();
		
		Server server = new Server(conf.getInt("wms.rest.port", 9999));
		server.setSendServerVersion(false);
		server.setSendDateHeader(false);
		server.setStopAtShutdown(true);
		server.setHandler(new RestHandler(this.wlq));
		try {
			server.start();
			LOG.info("REST Server listening on port:" + conf.getInt("wms.rest.port", 9999));
			server.join();
		} catch (InterruptedException e) {
			LOG.error("InterruptedException " + e);
		} catch (Exception e) {
			LOG.error("Exception " + e);
		}

	}
}
*/