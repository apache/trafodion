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
import java.util.concurrent.Executors;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Future;
import java.util.concurrent.ExecutionException;
 
import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import org.trafodion.wms.Constants;
import org.trafodion.wms.server.ServerManager;

public class ServerLeaderElection {
	private static  final Log LOG = LogFactory.getLog(ServerLeaderElection.class);
    private ServerManager sm = null;
    private String nodePath;
    private String parentZnode;
    private boolean isLeader = false;
    private ExecutorService es = null;
    private Future future = null;

    public ServerLeaderElection(ServerManager sm) throws IOException, InterruptedException, KeeperException  {
    	this.sm = sm;
    	this.parentZnode = sm.getZKParentZnode();
        
		setNodePath(sm.getZkClient().create(parentZnode + Constants.DEFAULT_ZOOKEEPER_ZNODE_SERVERS_LEADER + "/" + ":" + sm.getHostName() + ":" + sm.getInstance() + ":", new byte[0]/*no data yet*/,
                ZooDefs.Ids.OPEN_ACL_UNSAFE, CreateMode.EPHEMERAL_SEQUENTIAL));
        elect();
    }
    public void setNodePath(String nodePath) {
        this.nodePath = nodePath;
    }
    
    public boolean isLeader() {
        return isLeader;
    }
    // If curent leader is deleted, the immediate follower becomes leader.
    // If in between follower is deleted, link is broken, call this method again to re-establish that link.
    private void elect() throws IOException, InterruptedException, KeeperException {
        List<String> nodes =  sm.getZkClient().getChildren(parentZnode + Constants.DEFAULT_ZOOKEEPER_ZNODE_SERVERS_LEADER, new ElectionNodeWatcher());
        Collections.sort(nodes);
        
        for(int i=0; i < nodes.size(); i++) {
        	String nodePath = parentZnode + Constants.DEFAULT_ZOOKEEPER_ZNODE_SERVERS_LEADER + "/" + nodes.get(i);
        	if (nodePath.equals(this.nodePath)) {
        		if (i == 0) {
        			//I'm the first node and therefore the leader.
         			LOG.info("I'm the Leader [" + nodePath + "]");
        			if(nodes.size() > 1){
        				String previousNodePath = parentZnode + Constants.DEFAULT_ZOOKEEPER_ZNODE_SERVERS_LEADER + "/" + nodes.get(nodes.size() - 1);
        				LOG.info("Watching [" + previousNodePath + "]");
        				sm.getZkClient().exists(previousNodePath, new IndividualNodeWatcher());
        			}
        			isLeader=true;
        		} else {
           			//I'm a follower so avoid herd effect by setting watch on previous node.
        			LOG.info("I'm a follower [" + nodePath + "]");
        			String previousNodePath = parentZnode + Constants.DEFAULT_ZOOKEEPER_ZNODE_SERVERS_LEADER + "/" + nodes.get(i - 1);
        			LOG.info("Watching [" + previousNodePath + "]");
        			sm.getZkClient().exists(previousNodePath, new IndividualNodeWatcher());
        			isLeader=false;
        		}
        		break;
        	}
        }
    }		
    private class ElectionNodeWatcher implements Watcher {
    	//watches /LEADER node's children changes.
    	public void process(WatchedEvent event) {
    		if(event.getType() == Event.EventType.NodeChildrenChanged) {
    			LOG.info("Node changed [" + event.getPath() + "], re-electing new leader.");
    			try {
    				elect();
    			} catch (IOException e) {
                    LOG.error(e);
    			} catch (InterruptedException e) {
                    LOG.error(e);
    			} catch (KeeperException e) {
                    LOG.error(e);
    			}
    		}
    	}
    }
    private class IndividualNodeWatcher implements Watcher {
    	//watches /LEADER/ node's deleted changes.
    	public void process(WatchedEvent event) {
    		if(event.getType() == Event.EventType.NodeDeleted) {
    			LOG.info("Node deleted [" + event.getPath() + "], re-electing new leader.");
 
    			try {
 /*
    				if(isLeader){
    					if(es == null)
    						es = Executors.newSingleThreadExecutor();

    					if(future == null){
    						//first cycle of restart service
   							LOG.info("Starting first server restart service cycle");
    						future = es.submit(new ServerRestart(sm.getZkClient()));
     					} else {
     						//check to see if last cycle finished ok
     						//If so, start a new one
       						if(future.get() == null){
       							LOG.info("Starting new server restart service cycle");
    							future = es.submit(new ServerRestart(sm.getZkClient()));
       						} else {
       							LOG.info("Previous restart service cycle is running");
       						}
    					}
   					
    				}
 */
    				elect();
 /*   				
    				Scanner s = new Scanner(event.getPath());
    				s.useDelimiter(":");
    				s.next();//skip path
    				String hostName=s.next();
    				s.close();
     				
    				Stat stat = zkc.exists(parentZnode + Constants.DEFAULT_ZOOKEEPER_ZNODE_SERVERS_RESTART + "/" + ":" + hostName + ":",false);
    				if(stat == null) {
    					sm.getZkClient().create(Constants.DEFAULT_ZOOKEEPER_ZNODE_SERVERS_RESTART + "/" + ":" + hostName + ":", new byte[0],
    							ZooDefs.Ids.OPEN_ACL_UNSAFE, CreateMode.PERSISTENT);
    				}
*/
    			} catch (Exception e) {
    				LOG.error(e);
    			}
    		}
    	}
    }
}

