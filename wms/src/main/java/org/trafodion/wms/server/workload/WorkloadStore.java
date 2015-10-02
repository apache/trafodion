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

package org.trafodion.wms.server.workload;

import java.net.*;
import java.util.*;
import java.io.IOException;
import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.io.InputStream;
import java.io.ByteArrayOutputStream;
import java.nio.charset.Charset;

import static java.util.concurrent.TimeUnit.*;
import java.util.concurrent.Executors;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.ScheduledFuture;
import java.util.concurrent.ExecutionException;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import org.apache.zookeeper.CreateMode;
import org.apache.zookeeper.ZooDefs;
import org.apache.zookeeper.data.Stat;
import org.apache.zookeeper.KeeperException;
import org.apache.zookeeper.ZooKeeper.States;

import org.apache.hadoop.conf.Configuration;

import org.trafodion.wms.util.WmsConfiguration;
import org.trafodion.wms.zookeeper.ZkClient;
import org.trafodion.wms.server.ServerLeaderElection;
import org.trafodion.wms.Constants;
import org.trafodion.wms.util.Bytes;

public class WorkloadStore {
    private static final Log LOG = LogFactory.getLog(WorkloadStore.class);
    Configuration conf;
    ZkClient zkc = null;
    String parentZnode;
    ServerLeaderElection sle;
    long cleanerInitialDelay;
    long cleanerPeriod;
    final ScheduledExecutorService scheduler = Executors
            .newScheduledThreadPool(1);
    ScheduledFuture<?> cleanerHandle;
    List<String> children = new ArrayList<String>();
    List<String> clientIds = new ArrayList<String>();

    // List<Request> workloads = new ArrayList<Request>();

    public WorkloadStore(Configuration conf, ZkClient zkc, String parentZnode,
            ServerLeaderElection sle) {
        this.conf = conf;
        this.zkc = zkc;
        this.parentZnode = parentZnode;
        this.sle = sle;
        cleanerInitialDelay = conf.getInt(
                Constants.WMS_SERVER_WORKLOAD_CLEANER_INITIAL_DELAY,
                Constants.DEFAULT_WMS_SERVER_WORKLOAD_CLEANER_INITIAL_DELAY);
        cleanerPeriod = conf.getInt(
                Constants.WMS_SERVER_WORKLOAD_CLEANER_PERIOD,
                Constants.DEFAULT_WMS_SERVER_WORKLOAD_CLEANER_PERIOD);
        // startCleaner();
    }

    public WorkloadStore(String[] args) {
        if (zkc == null) {
            try {
                zkc = new ZkClient();
                zkc.connect();
                LOG.info("Connected to ZooKeeper");
                parentZnode = "/" + System.getProperty("user.name");
            } catch (Exception e) {
                LOG.error(e);
                System.exit(1);
            }
        }

        Configuration conf = WmsConfiguration.create();
        cleanerInitialDelay = conf.getInt(
                Constants.WMS_SERVER_WORKLOAD_CLEANER_INITIAL_DELAY,
                Constants.DEFAULT_WMS_SERVER_WORKLOAD_CLEANER_INITIAL_DELAY);
        cleanerPeriod = conf.getInt(
                Constants.WMS_SERVER_WORKLOAD_CLEANER_PERIOD,
                Constants.DEFAULT_WMS_SERVER_WORKLOAD_CLEANER_PERIOD);
        // startCleaner();
    }
    /*
     * final Runnable cleaner = new Runnable() { public void run() { clean(); }
     * };
     * 
     * public void startCleaner() { cleanerHandle =
     * scheduler.scheduleAtFixedRate(cleaner, cleanerInitialDelay,
     * cleanerPeriod, SECONDS); }
     * 
     * public void stopCleaner() { cleanerHandle.cancel(true); }
     * 
     * synchronized void clean() { LOG.debug("Workload cleaner is running");
     * 
     * if (!sle.isLeader()) return;
     * 
     * children.clear(); try { children = zkc.getChildren(parentZnode +
     * Constants.DEFAULT_ZOOKEEPER_ZNODE_CLIENTS, null); } catch (Exception e) {
     * LOG.error("Exception reading Constants.DEFAULT_ZOOKEEPER_ZNODE_CLIENTS, "
     * + e); }
     * 
     * if (children.isEmpty()) { LOG.debug("No children found in " + parentZnode
     * + Constants.DEFAULT_ZOOKEEPER_ZNODE_CLIENTS); } else { clientIds.clear();
     * for (String aChild : children) { StringTokenizer st = new
     * StringTokenizer(aChild, ":"); while (st.hasMoreTokens()) {
     * st.nextToken(); // skip ip address String timestamp = st.nextToken(); //
     * timestamp clientIds.add(timestamp); } } }
     * 
     * children.clear(); try { children = zkc.getChildren(parentZnode +
     * Constants.DEFAULT_ZOOKEEPER_ZNODE_WORKLOADS, null); } catch (Exception e)
     * {
     * LOG.error("Exception reading Constants.DEFAULT_ZOOKEEPER_ZNODE_WORKLOADS, "
     * + e); }
     * 
     * if (children.isEmpty()) { LOG.debug("No workloads found in " +
     * parentZnode + Constants.DEFAULT_ZOOKEEPER_ZNODE_WORKLOADS); return; }
     * else { // workloads.clear(); try { for (String aChild : children) { //
     * workloads.add(get(aChild)); } } catch (Exception e) {
     * e.printStackTrace(); } }
     * 
     * LOG.debug("Workloads found in " + parentZnode +
     * Constants.DEFAULT_ZOOKEEPER_ZNODE_WORKLOADS);
     * 
     * Date now = new Date(); for (Request aWorkload : workloads) {
     * LOG.debug("Workload=" + aWorkload); Map<String, KeyValue> reqkv =
     * aWorkload.getData().getKeyValues(); if
     * (reqkv.get(Constants.STATE).getStringValue() == null ||
     * reqkv.get(Constants.STATE).getStringValue()
     * .equalsIgnoreCase(Constants.COMPLETED)) { if (now.getTime() -
     * reqkv.get(Constants.BEGIN_TIMESTAMP).getLongValue() >=
     * Constants.THIRTY_SECONDS) { try { delete(reqkv.get(Constants.WORKLOAD_ID)
     * .getStringValue()); LOG.debug("Deleted workload[" +
     * reqkv.get(Constants.WORKLOAD_ID) .getStringValue() + "], Workload[" +
     * aWorkload + "]"); } catch (Exception e) { e.printStackTrace(); } } } else
     * { // For external workloads e.g., HIVE clients that connect using //
     * WmsClient object // and also don't have a client registered in
     * /wms/clients // change them // to COMPLETED/CLIENT_DISAPPEARED state.
     * This is mainly for // HIVE queries that have been ctrl-c'd // so they
     * have begun but will never have end from client. // if
     * (clientIds.contains(Long.valueOf(
     * aWorkload.getHeader().getClientTimestamp()).toString())) {
     * LOG.debug("Client ID found for [" +
     * aWorkload.getHeader().getClientTimestamp() + "], Workload[" + aWorkload +
     * "]"); continue; } else { LOG.debug("Client ID not found for [" +
     * aWorkload.getHeader().getClientTimestamp() + "], Workload[" + aWorkload +
     * "]"); // aWorkload.getData().setState(Constants.COMPLETED);
     * reqkv.put(Constants.STATE, new
     * KeyValue().setStringValue(Constants.COMPLETED)); //
     * aWorkload.getData().setSubState(Constants.CLIENT_DISAPPEARED);
     * reqkv.put(Constants.SUBSTATE, new KeyValue()
     * .setStringValue(Constants.CLIENT_DISAPPEARED)); try {
     * put(reqkv.get(Constants.WORKLOAD_ID).getStringValue(), aWorkload); }
     * catch (Exception e) { e.printStackTrace(); } } } } }
     * 
     * public String createWorkloadId() throws Exception { // see //
     * http://docs.oracle.com/javase/1.5.0/docs/api/java/util/Formatter.html
     * Date date = new Date(); String root = String.format(
     * "%1$s%2$tY%<tm%<td%3$s%2$tH%<tM%<tS%3$s%2$tL%3$s", "WMS_", date, "_");
     * String workloadZnode = parentZnode +
     * Constants.DEFAULT_ZOOKEEPER_ZNODE_WORKLOADS + "/" + root; String result =
     * zkc.create(workloadZnode, new byte[0], ZooDefs.Ids.OPEN_ACL_UNSAFE,
     * CreateMode.PERSISTENT_SEQUENTIAL); String id = result.replace(parentZnode
     * + Constants.DEFAULT_ZOOKEEPER_ZNODE_WORKLOADS + "/", "");
     * LOG.debug("Created workloadId=" + id); return id; }
     * 
     * public byte[] serialize(Request request) { byte[] bytes = null;
     * 
     * try { bytes = serializer.serialize(request); } catch (TException e) {
     * e.printStackTrace(); }
     * 
     * return bytes; }
     * 
     * public Request deserialize(byte[] bytes) { Request request = new
     * Request();
     * 
     * try { deserializer.deserialize(request, bytes); } catch (TException e) {
     * e.printStackTrace(); }
     * 
     * return request; }
     * 
     * public synchronized void put(String workloadId, Request request) throws
     * Exception { byte[] bytes = serialize(request); String workloadZnode =
     * parentZnode + Constants.DEFAULT_ZOOKEEPER_ZNODE_WORKLOADS + "/" +
     * workloadId; LOG.debug("znode=" + workloadZnode); Stat stat =
     * zkc.exists(workloadZnode, false); if (stat != null) {
     * zkc.setData(workloadZnode, bytes, -1); } else { zkc.create(workloadZnode,
     * bytes, ZooDefs.Ids.OPEN_ACL_UNSAFE, CreateMode.PERSISTENT); } }
     * 
     * public synchronized Request get(String workloadId) throws Exception {
     * Request request = new Request(); String workloadZnode = parentZnode +
     * Constants.DEFAULT_ZOOKEEPER_ZNODE_WORKLOADS + "/" + workloadId; Stat stat
     * = zkc.exists(workloadZnode, false); if (stat != null) { byte[] bytes =
     * zkc.getData(workloadZnode, false, stat); request = deserialize(bytes); }
     * return request; }
     * 
     * public synchronized void delete(String workloadId) throws Exception {
     * String workloadZnode = parentZnode +
     * Constants.DEFAULT_ZOOKEEPER_ZNODE_WORKLOADS + "/" + workloadId; Stat stat
     * = zkc.exists(workloadZnode, false); if (stat != null) {
     * zkc.delete(workloadZnode, -1); } }
     * 
     * public static void main(String[] args) { /* try { WorkloadStore store =
     * new WorkloadStore(args); String workloadId = store.createWorkloadId();
     * Request r = new Request(new Header(), new Data()); Map<String, KeyValue>
     * reqkv = r.getData().getKeyValues(); reqkv.put("workloadId", new
     * KeyValue().setStringValue(workloadId)); reqkv.put("operation", new
     * KeyValue() .setIntValue(Operation.OPERATION_BEGIN.getValue()));
     * reqkv.put("state", new KeyValue().setStringValue(Constants.RUNNING));
     * reqkv.put("subState", new KeyValue().setStringValue(Constants.BEGIN));
     * reqkv.put("beginTimestamp", new
     * KeyValue().setLongValue(System.currentTimeMillis()));
     * reqkv.put("endTimestamp", new
     * KeyValue().setLongValue(System.currentTimeMillis())); reqkv.put("type",
     * new KeyValue().setStringValue(Constants.TRAFODION)); reqkv.put(
     * "queryId", new KeyValue() .setStringValue(
     * "MXID11000001075212235857042874154000000000106U6553500_4_SQL_DATASOURCE_Q8"
     * )); reqkv.put("queryText", new
     * KeyValue().setStringValue("This is some query text"));
     * 
     * // serialize long beginTs = System.currentTimeMillis(); byte[] bytes =
     * store.serialize(r); long endTs = System.currentTimeMillis();
     * System.out.println("serialize took " + (endTs - beginTs) + " millis");
     * System.out.println("bytes=" + bytes + "\n");
     * 
     * // deserialize beginTs = System.currentTimeMillis(); Request dr =
     * store.deserialize(bytes); endTs = System.currentTimeMillis();
     * System.out.println("deserialize took " + (endTs - beginTs) + " millis");
     * System.out.println("deserialize=" + dr + "\n");
     * 
     * // put beginTs = System.currentTimeMillis(); store.put(workloadId, r);
     * endTs = System.currentTimeMillis(); System.out.println("put took " +
     * (endTs - beginTs) + " millis"); System.out.println("put=" + r + "\n");
     * 
     * // get beginTs = System.currentTimeMillis(); dr = store.get(workloadId);
     * endTs = System.currentTimeMillis(); System.out.println("get took " +
     * (endTs - beginTs) + " millis"); System.out.println("get=" + dr + "\n");
     * 
     * } catch (Exception e) { e.printStackTrace(); } }
     */
}