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
package org.trafodion.wms.master;

import java.net.InetAddress;

import java.io.*;
import java.util.*;
import java.util.concurrent.*;
import java.text.DateFormat;

import org.apache.zookeeper.*;
import org.apache.zookeeper.data.Stat;
import org.apache.hadoop.conf.Configuration;
import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import org.trafodion.wms.master.Metrics;
import org.trafodion.wms.script.ScriptManager;
import org.trafodion.wms.script.ScriptContext;
import org.trafodion.wms.Constants;
import org.trafodion.wms.zookeeper.ZkClient;
import org.trafodion.wms.util.WmsConfiguration;
import org.trafodion.wms.util.RetryCounter;
import org.trafodion.wms.util.RetryCounterFactory;

public class ServerManager implements Callable {
    private static final Log LOG = LogFactory.getLog(ServerManager.class);
    private WmsMaster wmsMaster;
    private Configuration conf;
    private ZkClient zkc = null;
    private InetAddress ia;
    private long startupTimestamp;
    private int maxRestartAttempts;
    private int retryIntervalMillis;
    private ExecutorService pool = null;
    private Metrics metrics;
    private String parentZnode;
    private RetryCounterFactory retryCounterFactory;
    private final ArrayList<String> configuredServers = new ArrayList<String>();
    private final ArrayList<String> runningServers = new ArrayList<String>();
    private final Queue<RestartHandler> restartQueue = new LinkedList<RestartHandler>();

    public ServerManager(WmsMaster wmsMaster) throws Exception {
        try {
            this.wmsMaster = wmsMaster;
            this.conf = wmsMaster.getConfiguration();
            this.zkc = wmsMaster.getZkClient();
            this.ia = wmsMaster.getInetAddress();
            this.startupTimestamp = wmsMaster.getStartTime();
            this.metrics = wmsMaster.getMetrics();
            maxRestartAttempts = conf
                    .getInt(Constants.WMS_MASTER_SERVER_RESTART_HANDLER_ATTEMPTS,
                            Constants.DEFAULT_WMS_MASTER_SERVER_RESTART_HANDLER_ATTEMPTS);
            retryIntervalMillis = conf
                    .getInt(Constants.WMS_MASTER_SERVER_RESTART_HANDLER_RETRY_INTERVAL_MILLIS,
                            Constants.DEFAULT_WMS_MASTER_SERVER_RESTART_HANDLER_RETRY_INTERVAL_MILLIS);
            retryCounterFactory = new RetryCounterFactory(maxRestartAttempts,
                    retryIntervalMillis);
            parentZnode = conf.get(Constants.ZOOKEEPER_ZNODE_PARENT,
                    Constants.DEFAULT_ZOOKEEPER_ZNODE_PARENT);
            pool = Executors.newSingleThreadExecutor();
        } catch (Exception e) {
            e.printStackTrace();
            LOG.error(e);
            throw e;
        }
    }

    class RestartHandler implements Callable<ScriptContext> {
        private ScriptContext scriptContext = new ScriptContext();
        private String znodePath;

        public RestartHandler(String znodePath) {
            this.znodePath = znodePath;
        }

        @Override
        public ScriptContext call() throws Exception {
            try {
                Scanner scn = new Scanner(znodePath);
                scn.useDelimiter(":");
                String hostName = scn.next();// host name
                String instance = scn.next();// instance
                int infoPort = Integer.parseInt(scn.next());// UI info port
                long serverStartTimestamp = Long.parseLong(scn.next());
                scn.close();

                // Get the --config property from classpath...it's always first
                // in the classpath
                String cp = System.getProperty("java.class.path");
                scn = new Scanner(cp);
                scn.useDelimiter(":");
                String confDir = scn.next();
                scn.close();
                LOG.debug("conf dir [" + confDir + "]");

                // Get -Dwms.home.dir
                String wmsHome = System.getProperty("wms.home.dir");

                // If stop-wms.sh is executed and WMS_MANAGES_ZK then zookeeper
                // is stopped abruptly.
                // Second scenario is when ZooKeeper fails for some reason
                // regardless of whether WMS
                // manages it. When either happens the WmsServer running znodes
                // still exist in ZooKeeper
                // and we see them at next startup. When they eventually timeout
                // we get node deleted events for a server that no longer
                // exists. So, only recognize
                // WmsServer running znodes that have timestamps after last
                // WmsMaster startup.
                if (serverStartTimestamp > startupTimestamp) {
                    scriptContext.setHostName(hostName);
                    scriptContext.setScriptName("sys_shell.py");
                    if (hostName.equalsIgnoreCase(ia.getCanonicalHostName()))
                        scriptContext.setCommand("bin/wms-daemon.sh --config "
                                + confDir + " start server " + instance);
                    else
                        scriptContext.setCommand("pdsh -w " + hostName
                                + " \"cd " + wmsHome
                                + ";bin/wms-daemon.sh --config " + confDir
                                + " start server " + instance + "\"");

                    RetryCounter retryCounter = retryCounterFactory.create();
                    while (true) {
                        if (scriptContext.getStdOut().length() > 0)
                            scriptContext.getStdOut().delete(0,
                                    scriptContext.getStdOut().length());
                        if (scriptContext.getStdErr().length() > 0)
                            scriptContext.getStdErr().delete(0,
                                    scriptContext.getStdErr().length());
                        LOG.info("Restarting WmsServer [" + hostName + ":"
                                + instance + "], script [ "
                                + scriptContext.toString() + " ]");
                        ScriptManager.getInstance().runScript(scriptContext);

                        if (scriptContext.getExitCode() == 0) {
                            LOG.info("WmsServer [" + hostName + ":" + instance
                                    + "] restarted");
                            break;
                        } else {
                            StringBuilder sb = new StringBuilder();
                            sb.append("exit code ["
                                    + scriptContext.getExitCode() + "]");
                            if (!scriptContext.getStdOut().toString().isEmpty())
                                sb.append(", stdout ["
                                        + scriptContext.getStdOut().toString()
                                        + "]");
                            if (!scriptContext.getStdErr().toString().isEmpty())
                                sb.append(", stderr ["
                                        + scriptContext.getStdErr().toString()
                                        + "]");
                            LOG.error(sb.toString());

                            if (!retryCounter.shouldRetry()) {
                                LOG.error("WmsServer [" + hostName + ":"
                                        + instance + "] restart failed after "
                                        + retryCounter.getMaxRetries()
                                        + " retries");
                                break;
                            } else {
                                retryCounter.sleepUntilNextRetry();
                                retryCounter.useRetry();
                            }
                        }
                    }
                } else {
                    LOG.debug("No restart for "
                            + znodePath
                            + "\nbecause WmsServer start time ["
                            + DateFormat.getDateTimeInstance().format(
                                    new Date(serverStartTimestamp))
                            + "] was before WmsMaster start time ["
                            + DateFormat.getDateTimeInstance().format(
                                    new Date(startupTimestamp)) + "]");
                }
            } catch (Exception e) {
                e.printStackTrace();
                LOG.error(e);
            }

            return scriptContext;
        }
    }

    class RunningWatcher implements Watcher {
        public void process(WatchedEvent event) {
            if (event.getType() == Event.EventType.NodeChildrenChanged) {
                LOG.debug("Running children changed [" + event.getPath() + "]");
                try {
                    getZkRunning();
                } catch (Exception e) {
                    e.printStackTrace();
                    LOG.error(e);
                }
            } else if (event.getType() == Event.EventType.NodeDeleted) {
                String znodePath = event.getPath();
                LOG.debug("Running znode deleted [" + znodePath + "]");
                try {
                    restartServer(znodePath);
                } catch (Exception e) {
                    e.printStackTrace();
                    LOG.error(e);
                }
            }
        }
    }

    @Override
    public Boolean call() throws Exception {

        long timeoutMillis = 5000;

        try {
            getServersFile();
            getZkRunning();

            while (true) {
                while (!restartQueue.isEmpty()) {
                    LOG.debug("Restart queue size [" + restartQueue.size()
                            + "]");
                    RestartHandler handler = restartQueue.poll();
                    Future<ScriptContext> runner = pool.submit(handler);
                    ScriptContext scriptContext = runner.get();// blocking call
                    if (scriptContext.getExitCode() != 0)
                        restartQueue.add(handler);
                }

                try {
                    Thread.sleep(timeoutMillis);
                } catch (InterruptedException e) {
                }
            }

        } catch (Exception e) {
            e.printStackTrace();
            LOG.error(e);
            pool.shutdown();
            throw e;
        }
    }

    private void getServersFile() throws Exception {
        InputStream is = this.getClass().getResourceAsStream("/servers");
        if (is == null)
            throw new IOException("Cannot find servers file");

        BufferedReader br = new BufferedReader(new InputStreamReader(is));
        configuredServers.clear();
        String line;
        while ((line = br.readLine()) != null) {
            configuredServers.add(line);
        }

        Collections.sort(configuredServers);

        if (configuredServers.size() < 1)
            throw new IOException("No entries found in servers file");

        int lnum = 1;
        for (int i = 0; i < configuredServers.size(); i++) {
            LOG.debug("servers file line " + lnum + " ["
                    + configuredServers.get(i) + "]");
            lnum++;
        }
    }

    private List<String> getChildren(String znode, Watcher watcher)
            throws Exception {
        List<String> children = null;
        children = zkc.getChildren(znode, watcher);
        if (!children.isEmpty())
            Collections.sort(children);
        return children;
    }

    private synchronized void getZkRunning() throws Exception {
        LOG.debug("Reading " + parentZnode
                + Constants.DEFAULT_ZOOKEEPER_ZNODE_SERVERS_RUNNING);
        List<String> children = getChildren(parentZnode
                + Constants.DEFAULT_ZOOKEEPER_ZNODE_SERVERS_RUNNING,
                new RunningWatcher());

        if (!children.isEmpty()) {
            for (String child : children) {
                // If stop-wms.sh is executed and WMS_MANAGES_ZK then zookeeper
                // is stopped abruptly.
                // Second scenario is when ZooKeeper fails for some reason
                // regardless of whether WMS
                // manages it. When either happens the WmsServer running znodes
                // still exist in ZooKeeper
                // and we see them at next startup. When they eventually timeout
                // we get node deleted events for a server that no longer
                // exists. So, only recognize
                // WmsServer running znodes that have timestamps after last
                // WmsMaster startup.
                Scanner scn = new Scanner(child);
                scn.useDelimiter(":");
                String hostName = scn.next();
                String instance = scn.next();
                int infoPort = Integer.parseInt(scn.next());
                long serverStartTimestamp = Long.parseLong(scn.next());
                scn.close();

                if (serverStartTimestamp < startupTimestamp)
                    continue;

                if (!runningServers.contains(child)) {
                    LOG.debug("Watching running [" + child + "]");
                    zkc.exists(parentZnode
                            + Constants.DEFAULT_ZOOKEEPER_ZNODE_SERVERS_RUNNING
                            + "/" + child, new RunningWatcher());
                    runningServers.add(child);
                }
            }
            metrics.setTotalRunning(runningServers.size());
        } else {
            metrics.setTotalRunning(0);
        }
    }

    private synchronized void restartServer(String znodePath) throws Exception {
        String child = znodePath.replace(parentZnode
                + Constants.DEFAULT_ZOOKEEPER_ZNODE_SERVERS_RUNNING + "/", "");
        Scanner scn = new Scanner(child);
        scn.useDelimiter(":");
        String hostName = scn.next();
        String instance = scn.next();
        int infoPort = Integer.parseInt(scn.next());
        long serverStartTimestamp = Long.parseLong(scn.next());
        scn.close();

        LOG.error("WmsServer [" + hostName + ":" + instance + "] failed.");

        if (runningServers.contains(child)) {
            LOG.debug("Found [" + child
                    + "], deleting from running servers list");
            runningServers.remove(child);
            metrics.setTotalRunning(runningServers.size());
        }

        RestartHandler handler = new RestartHandler(child);
        restartQueue.add(handler);
    }

    public synchronized ArrayList<String> getServersList() {
        return runningServers;
    }

    /*
     * public synchronized ArrayList<Request> getWorkloadsList() {
     * ArrayList<Request> workloads = new ArrayList<Request>();
     * 
     * LOG.debug("Reading " + parentZnode +
     * Constants.DEFAULT_ZOOKEEPER_ZNODE_WORKLOADS);
     * 
     * try { List<String> children = getChildren(parentZnode +
     * Constants.DEFAULT_ZOOKEEPER_ZNODE_WORKLOADS, null);
     * 
     * if (!children.isEmpty()) { for (String child : children) { Request
     * request = new Request(); String workloadZnode = parentZnode +
     * Constants.DEFAULT_ZOOKEEPER_ZNODE_WORKLOADS + "/" + child; Stat stat =
     * zkc.exists(workloadZnode, false); if (stat != null) { byte[] bytes =
     * zkc.getData(workloadZnode, false, stat); try {
     * deserializer.deserialize(request, bytes); workloads.add(request); } catch
     * (TException e) { e.printStackTrace(); } } } } } catch (Exception e) {
     * e.printStackTrace(); }
     * 
     * return workloads; }
     */
    public synchronized ArrayList<String> getClientsList() {
        ArrayList<String> clients = new ArrayList<String>();

        LOG.debug("Reading " + parentZnode
                + Constants.DEFAULT_ZOOKEEPER_ZNODE_CLIENTS);

        try {
            List<String> children = getChildren(parentZnode
                    + Constants.DEFAULT_ZOOKEEPER_ZNODE_CLIENTS, null);
            if (!children.isEmpty()) {
                for (String child : children) {
                    clients.add(child);
                }
            }
        } catch (Exception e) {
            e.printStackTrace();
        }

        return clients;
    }
    /*
     * public synchronized ArrayList<Stream> getStreamsList() {
     * ArrayList<Stream> streams = new ArrayList<Stream>();
     * 
     * LOG.debug("Reading " + parentZnode +
     * Constants.DEFAULT_ZOOKEEPER_ZNODE_RULES);
     * 
     * try { List<String> children = getChildren(parentZnode +
     * Constants.DEFAULT_ZOOKEEPER_ZNODE_STREAMS, null);
     * 
     * if (!children.isEmpty()) { for (String child : children) { Stream stream
     * = new Stream(); String streamZnode = parentZnode +
     * Constants.DEFAULT_ZOOKEEPER_ZNODE_STREAMS + "/" + child; Stat stat =
     * zkc.exists(streamZnode, false); if (stat != null) { byte[] bytes =
     * zkc.getData(streamZnode, false, stat); try {
     * deserializer.deserialize(stream, bytes); streams.add(stream); } catch
     * (TException e) { e.printStackTrace(); } } } } } catch (Exception e) {
     * e.printStackTrace(); }
     * 
     * return streams; }
     */
    /*
     * public synchronized ArrayList<Rule> getRulesList() { ArrayList<Rule>
     * rules = new ArrayList<Rule>();
     * 
     * LOG.debug("Reading " + parentZnode +
     * Constants.DEFAULT_ZOOKEEPER_ZNODE_RULES);
     * 
     * try { List<String> children = getChildren(parentZnode +
     * Constants.DEFAULT_ZOOKEEPER_ZNODE_RULES, null);
     * 
     * if (!children.isEmpty()) { for (String child : children) { Rule rule =
     * new Rule(); String ruleZnode = parentZnode +
     * Constants.DEFAULT_ZOOKEEPER_ZNODE_RULES + "/" + child; Stat stat =
     * zkc.exists(ruleZnode, false); if (stat != null) { byte[] bytes =
     * zkc.getData(ruleZnode, false, stat); try { deserializer.deserialize(rule,
     * bytes); rules.add(rule); } catch (TException e) { e.printStackTrace(); }
     * } } } } catch (Exception e) { e.printStackTrace(); }
     * 
     * return rules; }
     */
}