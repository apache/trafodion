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
/**
 * Copyright 2007 The Apache Software Foundation
 *
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package org.trafodion.dcs.rest;

import java.io.IOException;
import java.net.InetSocketAddress;
import java.util.StringTokenizer;
import java.util.List;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.HashMap; 
import java.util.Map;

import org.trafodion.dcs.Constants;
import org.trafodion.dcs.rest.ServerConnector;
import org.trafodion.dcs.rest.RestConstants;
import org.trafodion.dcs.zookeeper.ZkClient;

import org.apache.zookeeper.KeeperException;
import org.apache.zookeeper.data.Stat;
import org.apache.zookeeper.ZooDefs;

import org.apache.hadoop.conf.Configuration;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
/**
 * Singleton class encapsulating global REST servlet state and functions.
 */
public class RESTServlet implements RestConstants {
	private static final Log LOG = LogFactory.getLog(RESTServlet.class);
	private static RESTServlet INSTANCE;
	private final Configuration conf;
	private static ZkClient zkc;
	private String parentZnode;
	private Map<String, ServerConnector> m = new HashMap<String, ServerConnector>();

	/**
	 * @return the RESTServlet singleton instance
	 * @throws IOException
	 */
	public synchronized static RESTServlet getInstance() 
	throws IOException {
		assert(INSTANCE != null);
		return INSTANCE;
	}

	/**
	 * @param conf Existing configuration to use in rest servlet
	 * @return the RESTServlet singleton instance
	 * @throws IOException
	 */
	public synchronized static RESTServlet getInstance(Configuration conf)
	throws IOException {
		if (INSTANCE == null) {
			INSTANCE = new RESTServlet(conf);
		}
		return INSTANCE;
	}
	
	public ZkClient getZk(){
		return this.zkc;
	}
	
	public String getParentZnode(){
		return this.parentZnode;
	}

	public synchronized static void stop() {
		if (INSTANCE != null)  INSTANCE = null;
	}

	/**
	 * Constructor with existing configuration
	 * @param conf existing configuration
	 * @throws IOException.
	 */
	RESTServlet(Configuration conf) throws IOException {
		this.conf = conf;
		this.parentZnode = conf.get(Constants.ZOOKEEPER_ZNODE_PARENT,Constants.DEFAULT_ZOOKEEPER_ZNODE_PARENT);	   	

	}

	private void openZk() throws IOException {
		try {
			if(zkc == null)  
				zkc = new ZkClient();//CTRL-C...set sessionTimeout,maxRetries,retryIntervalMillis
			zkc.connect();
		} catch (InterruptedException e) {
			LOG.error(e);
		} 
	}

	public synchronized List<String> getChildren(String znode) throws IOException {
		List<String> s = null;
		
		try {
			openZk();
			s = zkc.getChildren(znode,null);
		} catch (InterruptedException e) {
			LOG.error(e);
		} catch (KeeperException e) {
			LOG.error(e);
		} catch (NullPointerException e) {
			LOG.error(e);
		}
		
		return s;
	}
	
	public synchronized List<String> getMaster() throws IOException {
		return getChildren(parentZnode + Constants.DEFAULT_ZOOKEEPER_ZNODE_MASTER);
	} 
	public synchronized List<String> getRunning() throws IOException {
		return getChildren(parentZnode + Constants.DEFAULT_ZOOKEEPER_ZNODE_SERVERS_RUNNING);
	}	
	public synchronized List<String> getRegistered() throws IOException {
		return getChildren(parentZnode + Constants.DEFAULT_ZOOKEEPER_ZNODE_SERVERS_REGISTERED);
	} 

	Configuration getConfiguration() {
		return conf;
	}

	/**
	 * Helper method to determine if server should
	 * only respond to GET HTTP method requests.
	 * @return boolean for server read-only state
	 */
	boolean isReadOnly() {
		return getConfiguration().getBoolean("dcs.rest.readonly", false);
	}
}
