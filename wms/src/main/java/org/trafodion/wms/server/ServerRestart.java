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

package org.trafodion.wms.server;

import org.apache.zookeeper.*;
import org.apache.zookeeper.data.Stat;

import java.io.IOException;
import java.util.Collections;
import java.util.List;
import java.util.Scanner;
 
import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import org.trafodion.wms.script.ScriptManager;
import org.trafodion.wms.script.ScriptContext;
import org.trafodion.wms.Constants;
import org.trafodion.wms.zookeeper.ZkClient;

public class ServerRestart implements Runnable {
	private static  final Log LOG = LogFactory.getLog(ServerRestart.class);
   	private static ScriptContext ctx = new ScriptContext();
    private ZkClient zkc = null;
    
    public ServerRestart(ZkClient zkc) 
    	throws IOException, InterruptedException, KeeperException  {
    	this.zkc = zkc;
    }

    public void run() {
    	try {
    		restartServers();
    	} catch (IOException e) {
    		LOG.error(e);
    	} catch (InterruptedException e) {
    		LOG.error(e);
    	} catch (KeeperException e) {
    		LOG.error(e);
    	}
    }
    
    private void restartServers() throws IOException, InterruptedException, KeeperException {
    	//Read conf/servers files and compare against /wms/servers/running
    	//If not running invoked script to restart. Ignore any servers that
    	//were added dynamically by /bin/local-servers.sh
    	
    	List<String> nodes = zkc.getChildren(Constants.DEFAULT_ZOOKEEPER_ZNODE_SERVERS_RUNNING,null);
         
    	for(int i=0; i < nodes.size(); i++) {
    		/*
        	String nodePath = Constants.DEFAULT_ZOOKEEPER_ZNODE_SERVERS_RUNNING + "/" + nodes.get(i);
        	Scanner s = new Scanner(nodePath);
        	s.useDelimiter(":");
         	String hostName=s.next();
         	String instance=s.next();
        	s.close();
        	
        	//Setup script context and run
           	ctx.setHostName(hostName);
           	ctx.setScriptName("sys_shell.py");
           	ctx.setCommand("bin/start-wms.sh");
			ScriptManager.getInstance().runScript(ctx);
			LOG.info(ctx.toString());
			*/
    	}
    }

}

