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
package org.trafodion.dcs.master;

import java.net.InetAddress;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.BufferedReader;
import java.io.FileReader;
import java.io.FileNotFoundException;
import java.util.Scanner;
import java.util.Collections;
import java.util.Iterator;
import java.util.LinkedList;
import java.util.Queue;
import java.util.List;
import java.util.ArrayList;
import java.util.concurrent.Callable;
import java.util.concurrent.Executors;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Future;
import java.util.concurrent.ExecutionException;
import java.util.Date;
import java.util.Comparator;
import java.util.Map;
import java.util.HashMap;
import java.text.DateFormat;

import org.apache.zookeeper.*;
import org.apache.zookeeper.data.Stat;
import org.apache.hadoop.conf.Configuration;
import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.trafodion.dcs.master.RunningServer;
import org.trafodion.dcs.master.RegisteredServer;
import org.trafodion.dcs.master.Metrics;
import org.trafodion.dcs.script.ScriptManager;
import org.trafodion.dcs.script.ScriptContext;
import org.trafodion.dcs.Constants;
import org.trafodion.dcs.zookeeper.ZkClient;
import org.trafodion.dcs.util.*;
import org.codehaus.jettison.json.JSONArray;
import org.codehaus.jettison.json.JSONException;
import org.codehaus.jettison.json.JSONObject;

public class ServerManager implements Callable {
    private static final Log LOG = LogFactory.getLog(ServerManager.class);
    private DcsMaster master;
    private Configuration conf;
    private DcsNetworkConfiguration netConf;
    private ZkClient zkc = null;
    private long startupTimestamp;
    private int maxRestartAttempts;
    private int retryIntervalMillis;
    private ExecutorService pool = null;
    private Metrics metrics;
    private String parentZnode;
    private RetryCounterFactory retryCounterFactory;
    private final ArrayList<String> configuredServers = new ArrayList<String>();
    private final Map<String, ServerPortMap> serverPortMap = new HashMap<String, ServerPortMap>();
    private final ArrayList<String> runningServers = new ArrayList<String>();
    private final ArrayList<String> registeredServers = new ArrayList<String>();
    private final Queue<RestartHandler> restartQueue = new LinkedList<RestartHandler>();
    private final ArrayList<ServerItem> serverItemList = new ArrayList<ServerItem>();
    private boolean trafodionQueryToolsEnabled;
    private JdbcT4Util jdbcT4Util = null;

    public ServerManager(DcsMaster master, Configuration conf, ZkClient zkc,
            DcsNetworkConfiguration netConf, long startupTimestamp,
            Metrics metrics) throws Exception {
        try {
            this.master = master;
            this.conf = conf;
            this.zkc = zkc;
            this.netConf = netConf;
            this.startupTimestamp = startupTimestamp;
            this.metrics = metrics;
            maxRestartAttempts = conf
                    .getInt(Constants.DCS_MASTER_SERVER_RESTART_HANDLER_ATTEMPTS,
                            Constants.DEFAULT_DCS_MASTER_SERVER_RESTART_HANDLER_ATTEMPTS);
            retryIntervalMillis = conf
                    .getInt(Constants.DCS_MASTER_SERVER_RESTART_HANDLER_RETRY_INTERVAL_MILLIS,
                            Constants.DEFAULT_DCS_MASTER_SERVER_RESTART_HANDLER_RETRY_INTERVAL_MILLIS);
            trafodionQueryToolsEnabled = conf.getBoolean(
                    Constants.DCS_MASTER_TRAFODION_QUERY_TOOLS,
                    Constants.DEFAULT_DCS_MASTER_TRAFODION_QUERY_TOOLS);
            if (trafodionQueryToolsEnabled)
                jdbcT4Util = new JdbcT4Util(conf, netConf);
            retryCounterFactory = new RetryCounterFactory(maxRestartAttempts,
                    retryIntervalMillis);
            parentZnode = conf.get(Constants.ZOOKEEPER_ZNODE_PARENT,
                    Constants.DEFAULT_ZOOKEEPER_ZNODE_PARENT);
            pool = Executors.newSingleThreadExecutor();
        } catch (Exception e) {
            e.printStackTrace();
            if (LOG.isErrorEnabled())
                LOG.error(e);
            throw e;
        }
    }

    class RestartHandler implements Callable<ScriptContext> {
        private ScriptContext scriptContext = new ScriptContext();
        private String znodePath;
        private int childCount;

        public RestartHandler(String znodePath, int childCount) {
            this.znodePath = znodePath;
            this.childCount = childCount;
        }

        @Override
        public ScriptContext call() throws Exception {
            try {
                Scanner scn = new Scanner(znodePath);
                scn.useDelimiter(":");
                String hostName = scn.next();// host name
                String instance = scn.next();// instance
                int infoPort = Integer.parseInt(scn.next()); // UI port
                long serverStartTimestamp = Long.parseLong(scn.next());
                scn.close();

                // Get the --config property from classpath...it's always first
                // in the classpath
                String cp = System.getProperty("java.class.path");
                scn = new Scanner(cp);
                scn.useDelimiter(":");
                String confDir = scn.next();
                scn.close();
                if (LOG.isDebugEnabled())
                    LOG.debug("conf dir [" + confDir + "]");

                // Get -Ddcs.home.dir
                String dcsHome = System.getProperty("dcs.home.dir");

                // If stop-dcs.sh is executed and DCS_MANAGES_ZK then zookeeper
                // is stopped abruptly.
                // Second scenario is when ZooKeeper fails for some reason
                // regardless of whether DCS
                // manages it. When either happens the DcsServer running znodes
                // still exist in ZooKeeper
                // and we see them at next startup. When they eventually timeout
                // we get node deleted events for a server that no longer
                // exists. So, only recognize
                // DcsServer running znodes that have timestamps after last
                // DcsMaster startup.
                //
                // But, if we are DcsMaster follower that is taking over from
                // failed one then ignore timestamp issues described above.
                // See MasterLeaderElection.elect()
                if ((master.isFollower() == false && serverStartTimestamp > startupTimestamp)
                        || (master.isFollower() && runningServers.size() < configuredServers.size())) {
                    scriptContext.setHostName(hostName);
                    scriptContext
                            .setScriptName(Constants.SYS_SHELL_SCRIPT_NAME);

                    if (hostName.equalsIgnoreCase(netConf.getHostName()))
                        scriptContext.setCommand("bin/dcs-daemon.sh --config "
                                + confDir + " start server " + instance + " "
                                + childCount);
                    else
                        scriptContext.setCommand("pdsh -w " + hostName
                                + " \"cd " + dcsHome
                                + ";bin/dcs-daemon.sh --config " + confDir
                                + " start server " + instance + " "
                                + childCount + "\"");

                    RetryCounter retryCounter = retryCounterFactory.create();
                    while (true) {
                        if (scriptContext.getStdOut().length() > 0)
                            scriptContext.getStdOut().delete(0,
                                    scriptContext.getStdOut().length());
                        if (scriptContext.getStdErr().length() > 0)
                            scriptContext.getStdErr().delete(0,
                                    scriptContext.getStdErr().length());
                        LOG.info("Restarting DcsServer [" + hostName + ":"
                                + instance + "], script [ "
                                + scriptContext.toString() + " ]");
                        ScriptManager.getInstance().runScript(scriptContext);

                        if (scriptContext.getExitCode() == 0) {
                            LOG.info("DcsServer [" + hostName + ":" + instance
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
                            if (LOG.isErrorEnabled())
                                LOG.error(sb.toString());

                            if (!retryCounter.shouldRetry()) {
                                if (LOG.isErrorEnabled())
                                    LOG.error("DcsServer [" + hostName + ":"
                                            + instance
                                            + "] restart failed after "
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
                    StringBuffer sb = new StringBuffer();
                    sb.append("No restart for ").append(znodePath).append(System.getProperty("line.separator"));
                    sb.append("DCS Master isFollower [").append(master.isFollower()).append("], ");
                    sb.append("DCS Master start time [")
                            .append(DateFormat.getDateTimeInstance().format(new Date(startupTimestamp))).append("], ");
                    sb.append("DCS Server start time [")
                            .append(DateFormat.getDateTimeInstance().format(new Date(serverStartTimestamp))).append("], ");
                    sb.append("running DCS Server num is [").append(runningServers.size())
                            .append("], registered DCS Server num is [").append(registeredServers.size()).append("].");

                    LOG.info(sb.toString());
                }
            } catch (Exception e) {
                e.printStackTrace();
                if (LOG.isErrorEnabled())
                    LOG.error(e);
            }

            return scriptContext;
        }
    }

    class RunningWatcher implements Watcher {
        public void process(WatchedEvent event) {
            if (event.getType() == Event.EventType.NodeChildrenChanged) {
                if (LOG.isDebugEnabled())
                    LOG.debug("Running children changed [" + event.getPath()
                            + "]");
                try {
                    getZkRunning();
                } catch (Exception e) {
                    e.printStackTrace();
                    if (LOG.isErrorEnabled())
                        LOG.error(e);
                }
            } else if (event.getType() == Event.EventType.NodeDeleted) {
                String znodePath = event.getPath();
                if (LOG.isDebugEnabled())
                    LOG.debug("Running znode deleted [" + znodePath + "]");
                try {
                    restartServer(znodePath);
                } catch (Exception e) {
                    e.printStackTrace();
                    if (LOG.isErrorEnabled())
                        LOG.error(e);
                }
            }
        }
    }

    class RegisteredWatcher implements Watcher {
        public void process(WatchedEvent event) {
            if (event.getType() == Event.EventType.NodeChildrenChanged) {
                if (LOG.isDebugEnabled())
                    LOG.debug("Registered children changed [" + event.getPath()
                            + "]");
                try {
                    getZkRegistered();
                } catch (Exception e) {
                    e.printStackTrace();
                    if (LOG.isErrorEnabled())
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
            createServersPortMap();
            getZkRunning();
            getUnwathedServers();
            getZkRegistered();

            while (true) {
                while (!restartQueue.isEmpty()) {
                    if (LOG.isDebugEnabled())
                        LOG.debug("Restart queue size [" + restartQueue.size()
                                + "]");
                    RestartHandler handler = restartQueue.poll();
                    Future<ScriptContext> runner = pool.submit(handler);
                    ScriptContext scriptContext = runner.get();// blocking call
                    // In some situation, there may restart dcs server replicated.
                    // Exit code == -2 means dcs server had been started,
                    // no needs to add to restart queue.
                    if (scriptContext.getExitCode() != 0 && scriptContext.getExitCode() != -2) {
                        restartQueue.add(handler);
                    }
                }

                try {
                    Thread.sleep(timeoutMillis);
                } catch (InterruptedException e) {
                }
            }

        } catch (Exception e) {
            e.printStackTrace();
            if (LOG.isErrorEnabled())
                LOG.error(e);
            pool.shutdown();
            throw e;
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

    private void getServersFile() throws Exception {
        InputStream is = this.getClass().getResourceAsStream("/servers");
        if (is == null)
            throw new IOException("Cannot find servers file");

        BufferedReader br = new BufferedReader(new InputStreamReader(is));
        configuredServers.clear();
        String line;
        int lineNum = 1;
        while ((line = br.readLine()) != null) {
            Scanner scn = new Scanner(line);
            scn.useDelimiter(" ");
            String hostName = null;
            String serverCount = null;
            if (scn.hasNext())
                hostName = scn.next();// host name
            else
                hostName = new String("localhost");
            if (scn.hasNext())
                serverCount = scn.next();// optional
            else
                serverCount = "1";
            scn.close();
            if (hostName.equalsIgnoreCase("localhost")) {
                hostName = netConf.getHostName();
            }
            if (LOG.isDebugEnabled())
                LOG.debug("Adding to configured servers [" + hostName + ":"
                        + lineNum + ":" + serverCount + "]");
            configuredServers.add(hostName + ":" + lineNum + ":" + serverCount);
            lineNum++;
        }

        Collections.sort(configuredServers);

        if (configuredServers.size() < 1)
            throw new IOException("No entries found in servers file");

        int lnum = 1;
        for (int i = 0; i < configuredServers.size(); i++) {
            if (LOG.isDebugEnabled())
                LOG.debug("servers file line " + lnum + " ["
                        + configuredServers.get(i) + "]");
            lnum++;
        }
    }

    class ServerPortMap {
        int begPortNum = conf.getInt(Constants.DCS_MASTER_PORT,
                Constants.DEFAULT_DCS_MASTER_PORT) + 1;
        int endPortNum = begPortNum;
        StringBuilder sb = new StringBuilder();

        public void add(int instance, int childCount) {
            for (int i = 1; i <= childCount; i++) {
                if (endPortNum > begPortNum)
                    sb.append(":");
                sb.append(instance + ":" + i + ":" + endPortNum);
                endPortNum++;
            }
        }

        public String toString() {
            return sb.toString();
        }
    }

    private void createServersPortMap() throws Exception {
        serverPortMap.clear();

        for (String aServer : configuredServers) {
            Scanner scn = new Scanner(aServer);
            scn.useDelimiter(":");
            String hostName = scn.next();
            int instance = Integer.parseInt(scn.next());
            int childCount = Integer.parseInt(scn.next());
            scn.close();

            ServerPortMap spm = serverPortMap.get(hostName);
            if (spm == null)
                spm = new ServerPortMap();
            spm.add(instance, childCount);
            serverPortMap.put(hostName, spm);
        }

        StringBuilder sb = new StringBuilder();
        for (Map.Entry<String, ServerPortMap> entry : serverPortMap.entrySet()) {
            LOG.debug(("Key = " + entry.getKey() + ", Value = " + entry
                    .getValue()));
            sb.append(entry.getValue());
            sb.append(":");
        }

        LOG.debug("Setting " + parentZnode
                + Constants.DEFAULT_ZOOKEEPER_ZNODE_SERVERS_REGISTERED
                + " data [" + sb.toString() + "]");
        byte[] data = Bytes.toBytes(sb.toString());
        zkc.setData(parentZnode
                + Constants.DEFAULT_ZOOKEEPER_ZNODE_SERVERS_REGISTERED, data,
                -1);
    }

    private synchronized void getZkRunning() throws Exception {
        if (LOG.isDebugEnabled())
            LOG.debug("Reading " + parentZnode
                    + Constants.DEFAULT_ZOOKEEPER_ZNODE_SERVERS_RUNNING);
        List<String> children = getChildren(parentZnode
                + Constants.DEFAULT_ZOOKEEPER_ZNODE_SERVERS_RUNNING,
                new RunningWatcher());

        if (!children.isEmpty()) {
            for (String child : children) {
                // If stop-dcs.sh is executed and DCS_MANAGES_ZK then zookeeper
                // is stopped abruptly.
                // Second scenario is when ZooKeeper fails for some reason
                // regardless of whether DCS
                // manages it. When either happens the DcsServer running znodes
                // still exist in ZooKeeper
                // and we see them at next startup. When they eventually timeout
                // we get node deleted events for a server that no longer
                // exists. So, only recognize
                // DcsServer running znodes that have timestamps after last
                // DcsMaster startup.
                Scanner scn = new Scanner(child);
                scn.useDelimiter(":");
                String hostName = scn.next();
                String instance = scn.next();
                int infoPort = Integer.parseInt(scn.next());
                long serverStartTimestamp = Long.parseLong(scn.next());
                scn.close();

                // If we are DcsMaster follower that is taking over from failed
                // one then ignore timestamp issues described above.
                // See MasterLeaderElection.elect()
                if (master.isFollower() == false) {
                    if (serverStartTimestamp < startupTimestamp)
                        continue;
                }

                if (!runningServers.contains(child)) {
                    if (LOG.isDebugEnabled())
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

    private void getUnwathedServers() {
        // In some situation when open HA, if DCS Server does not have znode info in zookeeper
        // when DCS Master is starting, then server will never be watched by zookeeper,
        // and if it downs, it will never be restarted.

        // configuredServers
        // hostName + ":" + lineNum + ":" + serverCount
        // runningServers
        // hostName + ":" + instance + ":" + infoPort + ":" + serverStartTimestamp
        // eg : gy26.esgyncn.local:3:24413:1515056285028
        // RestartHandler need to know hostName, instanceNum(lineNum), serverStartTimestamp(for if condition)
        if (!master.isFollower() || runningServers.size() == configuredServers.size()) {
            if (LOG.isDebugEnabled()) {
                if (!master.isFollower()) {
                    LOG.debug("dcs master start normally, no need to add watchers");
                } else {
                    LOG.debug("backup master start, all dcs servers have started, no need to add watchers");
                }
            }
            return;
        }

        boolean found = false;
        for (String configured : configuredServers) {
            Scanner configuredScn = new Scanner(configured);
            configuredScn.useDelimiter(":");
            String hostName = configuredScn.next();
            int instance = Integer.parseInt(configuredScn.next());
            int serverCount = Integer.parseInt(configuredScn.next());
            configuredScn.close();
            for (String running : runningServers) {
                Scanner runningScn = new Scanner(running);
                runningScn.useDelimiter(":");
                String runningHostName = runningScn.next();

                runningScn.close();
                if (runningHostName.equals(hostName)) {
                    found = true;
                    break;
                }
            }
            if (found) {
                found = false;
                continue;
            } else {
                LOG.error("DcsServer [" + hostName + ":" + instance + "] does not started when starting DcsMaster [" + master.getServerName() + "] add to restart queue.");
                // add to the restart handler
                String simulatePath = hostName + ":" + instance + ":0:" + System.currentTimeMillis();
                RestartHandler handler = new RestartHandler(simulatePath, serverCount);
                restartQueue.add(handler);
            }
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

        if (LOG.isErrorEnabled())
            LOG.error("DcsServer [" + hostName + ":" + instance + "] failed.");

        if (runningServers.contains(child)) {
            LOG.debug("Found [" + child
                    + "], deleting from running servers list");
            runningServers.remove(child);
            metrics.setTotalRunning(runningServers.size());
        }

        // Extract the server count for the restarting instance
        int count = 1;
        boolean found = false;
        for (String aServer : configuredServers) {
            scn = new Scanner(aServer);
            scn.useDelimiter(":");
            String srvrHostName = scn.next();
            String srvrInstance = scn.next();
            int srvrCount = new Integer(scn.next()).intValue();
            scn.close();
            if (srvrHostName.equals(hostName) && srvrInstance.equals(instance)) {
                LOG.debug("Found [" + srvrHostName + ":" + srvrInstance + ":"
                        + srvrCount + "] in configured servers");
                found = true;
                if (srvrCount > 0)
                    count = srvrCount;
                break;
            }
        }

        // For local-servers.sh don't restart anything that's not in the servers
        // file
        if (!found) {
            LOG.info("DcsServer [" + hostName + ":" + instance
                    + "] not in servers file. Not restarting");
            return;
        }

        RestartHandler handler = new RestartHandler(child, count);
        restartQueue.add(handler);

    }

    private synchronized void getZkRegistered() throws Exception {
        if (LOG.isDebugEnabled())
            LOG.debug("Reading " + parentZnode
                    + Constants.DEFAULT_ZOOKEEPER_ZNODE_SERVERS_REGISTERED);
        List<String> children = getChildren(parentZnode
                + Constants.DEFAULT_ZOOKEEPER_ZNODE_SERVERS_REGISTERED,
                new RegisteredWatcher());

        if (!children.isEmpty()) {
            registeredServers.clear();
            for (String child : children) {
                if (LOG.isDebugEnabled())
                    LOG.debug("Registered [" + child + "]");
                registeredServers.add(child);
            }
            metrics.setTotalRegistered(registeredServers.size());
        } else {
            metrics.setTotalRegistered(0);
        }
    }

    public synchronized List<RunningServer> getServersList() {
        ArrayList<RunningServer> serverList = new ArrayList<RunningServer>();
        Stat stat = null;
        byte[] data = null;

        int totalAvailable = 0;
        int totalConnecting = 0;
        int totalConnected = 0;

        if (LOG.isDebugEnabled())
            LOG.debug("Begin getServersList()");

        if (!runningServers.isEmpty()) {
            for (String aRunningServer : runningServers) {
                RunningServer runningServer = new RunningServer();
                Scanner scn = new Scanner(aRunningServer);
                scn.useDelimiter(":");
                runningServer.setHostname(scn.next());
                runningServer.setInstance(scn.next());
                runningServer.setInfoPort(Integer.parseInt(scn.next()));
                runningServer.setStartTime(Long.parseLong(scn.next()));
                scn.close();

                if (!registeredServers.isEmpty()) {
                    for (String aRegisteredServer : registeredServers) {
                        if (aRegisteredServer.contains(runningServer
                                .getHostname()
                                + ":"
                                + runningServer.getInstance() + ":")) {
                            try {
                                RegisteredServer registeredServer = new RegisteredServer();
                                stat = zkc
                                        .exists(parentZnode
                                                + Constants.DEFAULT_ZOOKEEPER_ZNODE_SERVERS_REGISTERED
                                                + "/" + aRegisteredServer,
                                                false);
                                if (stat != null) {
                                    data = zkc
                                            .getData(
                                                    parentZnode
                                                            + Constants.DEFAULT_ZOOKEEPER_ZNODE_SERVERS_REGISTERED
                                                            + "/"
                                                            + aRegisteredServer,
                                                    false, stat);
                                    scn = new Scanner(new String(data));
                                    scn.useDelimiter(":");
                                    if (LOG.isDebugEnabled())
                                        LOG.debug("getDataRegistered ["
                                                + new String(data) + "]");
                                    registeredServer.setState(scn.next());
                                    String state = registeredServer.getState();
                                    if (state.equals("AVAILABLE"))
                                        totalAvailable += 1;
                                    else if (state.equals("CONNECTING"))
                                        totalConnecting += 1;
                                    else if (state.equals("CONNECTED"))
                                        totalConnected += 1;
                                    registeredServer.setTimestamp(Long
                                            .parseLong(scn.next()));
                                    registeredServer.setDialogueId(scn.next());
                                    registeredServer.setNid(scn.next());
                                    registeredServer.setPid(scn.next());
                                    registeredServer.setProcessName(scn.next());
                                    registeredServer.setIpAddress(scn.next());
                                    registeredServer.setPort(scn.next());
                                    registeredServer.setClientName(scn.next());
                                    registeredServer.setClientIpAddress(scn
                                            .next());
                                    registeredServer.setClientPort(scn.next());
                                    registeredServer.setClientAppl(scn.next());
                                    registeredServer.setIsRegistered();
                                    scn.close();
                                    runningServer.getRegistered().add(
                                            registeredServer);
                                }
                            } catch (Exception e) {
                                e.printStackTrace();
                                if (LOG.isErrorEnabled())
                                    LOG.error("Exception: " + e.getMessage());
                            }
                        }
                    }
                }

                serverList.add(runningServer);
            }
        }

        metrics.setTotalAvailable(totalAvailable);
        metrics.setTotalConnecting(totalConnecting);
        metrics.setTotalConnected(totalConnected);

        Collections.sort(serverList, new Comparator<RunningServer>() {
            public int compare(RunningServer s1, RunningServer s2) {
                if (s1.getInstanceIntValue() == s2.getInstanceIntValue())
                    return 0;
                return s1.getInstanceIntValue() < s2.getInstanceIntValue() ? -1
                        : 1;
            }
        });

        if (LOG.isDebugEnabled())
            LOG.debug("End getServersList()");

        return serverList;
    }

    public synchronized List<ServerItem> getServerItemList() {
        if (LOG.isDebugEnabled())
            LOG.debug("Begin getServerItemList()");

        serverItemList.clear();

        for (RunningServer aRunningServer : this.getServersList()) {
            for (RegisteredServer aRegisteredServer : aRunningServer
                    .getRegistered()) {
                ServerItem serverItem = new ServerItem();
                serverItem.setHostname(aRunningServer.getHostname());
                serverItem.setinfoPort(aRunningServer.getInfoPort() + "");
                serverItem.setInstance(aRunningServer.getInstance());
                serverItem.setStartTime(aRunningServer.getStartTimeAsDate());
                serverItem.setIsRegistered(aRegisteredServer.getIsRegistered());
                serverItem.setState(aRegisteredServer.getState());
                serverItem.setNid(aRegisteredServer.getNid());
                serverItem.setPid(aRegisteredServer.getPid());
                serverItem.setProcessName(aRegisteredServer.getProcessName());
                serverItem.setIpAddress(aRegisteredServer.getIpAddress());
                serverItem.setPort(aRegisteredServer.getPort());
                serverItem.setClientName(aRegisteredServer.getClientName());
                serverItem.setClientAppl(aRegisteredServer.getClientAppl());
                serverItem.setClientIpAddress(aRegisteredServer
                        .getClientIpAddress());
                serverItem.setClientPort(aRegisteredServer.getClientPort());
                serverItemList.add(serverItem);
            }
        }
        if (LOG.isDebugEnabled())
            LOG.debug("End getServerItemList()");
        return serverItemList;
    }

    public synchronized List<JSONObject> getRepositoryItemList(String command) {
        if (LOG.isDebugEnabled())
            LOG.debug("Begin getRepositoryItemList()");

        JSONArray reposList = null;
        reposList = getRepositoryListT4Driver(command);
        List<JSONObject> objList = new ArrayList<JSONObject>();

        if (reposList != null) {
            try {
                for (int i = 0; i < reposList.length(); i++) {
                    objList.add(reposList.getJSONObject(i));
                }
            } catch (Exception e) {
                e.printStackTrace();
                if (LOG.isErrorEnabled())
                    LOG.error(e.getMessage());
            }
        }

        if (LOG.isDebugEnabled())
            LOG.debug("End getRepositoryItemList()");

        return objList;
    }

    public synchronized JSONArray getRepositoryListT4Driver(String command) {
        if (LOG.isDebugEnabled())
            LOG.debug("Begin getRepositoryListT4Driver()");

        JSONArray reposList = null;

        StringBuilder sb = new StringBuilder();
        if (command.equals(Constants.TRAFODION_REPOS_METRIC_SESSION_TABLE)) {
            sb.append(conf
                    .get(Constants.TRAFODION_REPOS_METRIC_SESSION_TABLE_QUERY,
                            Constants.DEFAULT_TRAFODION_REPOS_METRIC_SESSION_TABLE_QUERY));
        } else if (command.equals(Constants.TRAFODION_REPOS_METRIC_QUERY_TABLE)) {
            sb.append(conf.get(
                    Constants.TRAFODION_REPOS_METRIC_QUERY_TABLE_QUERY,
                    Constants.DEFAULT_TRAFODION_REPOS_METRIC_QUERY_TABLE_QUERY));
        } else if (command
                .equals(Constants.TRAFODION_REPOS_METRIC_QUERY_AGGR_TABLE)) {
            sb.append(conf
                    .get(Constants.TRAFODION_REPOS_METRIC_QUERY_AGGR_TABLE_QUERY,
                            Constants.DEFAULT_TRAFODION_REPOS_METRIC_QUERY_AGGR_TABLE_QUERY));
        } else
            sb.append(command);

        if (LOG.isDebugEnabled())
            LOG.debug("command [" + sb.toString() + "]");
        // reposList = jdbcT4Util.executeQuery(sb.toString());

        if (LOG.isDebugEnabled())
            LOG.debug("End getRepositoryListT4Driver()");

        return reposList;
    }

    public String getZKParentZnode() {
        return parentZnode;
    }

    public ZkClient getZkClient() {
        return zkc;
    }

    public JdbcT4Util getJdbcT4Util() {
        return jdbcT4Util;
    }
}
