// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2014 Hewlett-Packard Development Company, L.P.
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

package org.trafodion.dtm;

import java.io.ByteArrayOutputStream;
import java.io.DataOutputStream;
import java.io.IOException;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.NavigableMap;
import java.util.Set;
import java.util.StringTokenizer;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import org.apache.hadoop.conf.Configuration;

import org.apache.hadoop.hbase.Abortable;
import org.apache.hadoop.hbase.HRegionInfo;
import org.apache.hadoop.hbase.ServerName;

import org.apache.hadoop.hbase.client.HTable;
import org.apache.hadoop.hbase.client.transactional.TransactionRegionLocation;

import org.apache.hadoop.hbase.zookeeper.ZKUtil;
import org.apache.hadoop.hbase.zookeeper.ZooKeeperWatcher;
import org.apache.zookeeper.KeeperException;

/**
 * Zookeeper client-side communication class for the DTM
 */
public class HBaseTmZK implements Abortable{
	private final String baseNode = "/hbase/Trafodion/recovery/TM";
	static final Log LOG = LogFactory.getLog(HBaseTmZK.class);
	
	ZooKeeperWatcher zooKeeper;
	Set<String>     nodeList;	
	String zkNode;
	short dtmID;
	
	/**
	 * @param conf
	 * @throws Exception
	 */
	public HBaseTmZK(final Configuration conf) throws Exception {
                if (LOG.isTraceEnabled()) LOG.trace("HBaseTmZK(conf) -- ENTRY");
		this.dtmID = 0;
		this.zkNode = baseNode + "0";
		this.zooKeeper = new ZooKeeperWatcher(conf, "TM Recovery", this, true);
	}
	
	/**
	 * @param conf
	 * @param dtmID
	 * @throws Exception
	 */
	public HBaseTmZK(final Configuration conf, final short dtmID) throws Exception {
        if (LOG.isTraceEnabled()) LOG.trace("HBaseTmZK(conf, dtmID) -- ENTRY");
		this.dtmID = dtmID;
		this.zkNode = baseNode + String.format("%d", dtmID);
		this.zooKeeper = new ZooKeeperWatcher(conf, "TM Recovery", this, true);
	}

	/**
	 * @param znode
	 * @return
	 * @throws KeeperException
	 */
	private byte [] checkData (String znode) throws KeeperException, InterruptedException {
		return ZKUtil.getData(zooKeeper, znode);
	}
	
	/**
	 * @return
	 */
	public short getTMID() {
		return dtmID;
	}
	
	
	/**
	 * @return
	 * @throws KeeperException
	 */
	private List<String> getChildren() throws KeeperException {
		return ZKUtil.listChildrenNoWatch(zooKeeper, zkNode);
	}
	
	/**
	 * @return
	 * @throws KeeperException
	 */
	public Map<String,byte []> checkForRecovery() throws InterruptedException, KeeperException {		
        if (LOG.isTraceEnabled()) LOG.trace("checkForRecovery -- ENTRY");
		if(ZKUtil.nodeHasChildren(zooKeeper, zkNode)) {
			List<String> nodeChildren = new ArrayList<String>();
			Map<String, byte []> nodeDataMap = new HashMap<String, byte []>();
			nodeChildren = getChildren();
		 	
			for(String node : nodeChildren) {				

                if (LOG.isTraceEnabled()) LOG.trace("checkForRecovery -- found node: '" + node + "'");
				byte [] nodeData = checkData(zkNode +"/" + node);	
                if (LOG.isTraceEnabled()) LOG.trace("checkForRecovery -- found node: " + node + " node data " + nodeData.toString());		
				nodeDataMap.put(node, nodeData);
			}
            if (LOG.isTraceEnabled()) LOG.trace("checkForRecovery -- EXIT returning " + nodeDataMap.size() + " regions");
			return nodeDataMap;
		}
		else {
			if (LOG.isTraceEnabled()) LOG.trace(zkNode + " is currently not present.");
            if (LOG.isTraceEnabled()) LOG.trace("checkForRecovery -- EXIT -- node not present");
			return null;
		}		
	}

	/**
	 * @param toDelete
	 * @throws KeeperException
	 */
	public void deleteRegionEntry(Map.Entry<String,byte[]> toDelete ) throws KeeperException {
           LOG.info("deleteRegionEntry -- ENTRY -- key: " + toDelete.getKey() + " value: " + new String(toDelete.getValue()));
           ZKUtil.deleteNodeFailSilent(zooKeeper, zkNode + "/" + toDelete.getKey());
           LOG.info("deleteRegionEntry -- EXIT ");
        }

	/**
	 * @param node
	 * @param hostName
	 * @param portNumber
	 * @param encodedName
	 * @param data
	 * @throws IOException
	 */
        public void createRecoveryzNode(String hostName, int portNumber, String encodedName, byte [] data) throws IOException {
           LOG.info("HBaseTmZK:createRecoveryzNode: hostName: " + hostName + " port: " + portNumber +
                     " encodedName: " + encodedName + " data: " + new String(data));
           // default zNodePath for recovery
           String zNodeKey = hostName + "," + portNumber + "," + encodedName;

           LOG.info("HBaseTmZK:createRecoveryzNode: ZKW Post region recovery znode" + this.dtmID + " zNode Path " + zkNode);
           // create zookeeper recovery zNode, call ZK ...
           try {
              if (ZKUtil.checkExists(zooKeeper, zkNode) == -1) {
                 // create parent nodename
                 LOG.info("HBaseTmZK:createRecoveryzNode:: ZKW create parent zNodes " + zkNode);
                 ZKUtil.createWithParents(zooKeeper, zkNode);
              }
              ZKUtil.createAndFailSilent(zooKeeper, zkNode + "/" + zNodeKey, data);
           } catch (KeeperException e) {
              throw new IOException("HBaseTmZK:createRecoveryzNode: ZKW Unable to create recovery zNode: " + zkNode + " , throwing IOException " + e);
           }
        }

	/**
	 * @param node
	 * @param recovTable
	 * @throws IOException
	 */
        public void postAllRegionEntries(HTable recovTable) throws IOException {
           LOG.info("HBaseTmZK:postAllRegionEntries: recovTable: " + recovTable );
           NavigableMap<HRegionInfo, ServerName> regionMap = recovTable.getRegionLocations();
           Iterator<Map.Entry<HRegionInfo, ServerName>> it =  regionMap.entrySet().iterator();
           while(it.hasNext()) { // iterate entries.
              NavigableMap.Entry<HRegionInfo, ServerName> pairs = it.next();
              HRegionInfo region = pairs.getKey();
              LOG.info("postAllRegionEntries: region: " + region.getRegionNameAsString());
              ServerName serverValue = regionMap.get(region);
              String hostAndPort = new String(serverValue.getHostAndPort());
              StringTokenizer tok = new StringTokenizer(hostAndPort, ":");
              String hostName = new String(tok.nextElement().toString());
              int portNumber = Integer.parseInt(tok.nextElement().toString());
              byte [] lv_byte_region_info = region.toByteArray();
              try{
                 LOG.info("Calling createRecoveryzNode for encoded region: " + region.getEncodedName());
                 createRecoveryzNode(hostName, portNumber, region.getEncodedName(), lv_byte_region_info);
              }
              catch (Exception e2){
                 LOG.error("postAllRegionEntries exception in createRecoveryzNode " + region.getTable().getNameAsString() +
                           " exception: " + e2);
              }
           }// while
        }
	
	/* (non-Javadoc)
	 * @see org.apache.hadoop.hbase.Abortable#abort(java.lang.String, java.lang.Throwable)
	 */
	@Override
	public void abort(String arg0, Throwable arg1) {
		// TODO Auto-generated method stub
		
	}

	/* (non-Javadoc)
	 * @see org.apache.hadoop.hbase.Abortable#isAborted()
	 */
	@Override
	public boolean isAborted() {
		// TODO Auto-generated method stub
		return false;
	}
}
