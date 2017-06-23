/**********************************************************************
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
**********************************************************************/
package org.trafodion.dcs.server;

import java.net.InetAddress;
import java.io.*;
import java.util.*;
import java.util.concurrent.Callable;
import java.util.concurrent.Executors;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.ExecutorCompletionService;
import java.util.concurrent.CompletionService;
import java.util.concurrent.Future;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.CountDownLatch;
import java.text.DateFormat;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.apache.hadoop.conf.Configuration;
import org.apache.zookeeper.CreateMode;
import org.apache.zookeeper.ZooDefs;
import org.apache.zookeeper.data.Stat;
import org.apache.zookeeper.Watcher;
import org.apache.zookeeper.WatchedEvent;
import org.apache.zookeeper.KeeperException;
import org.trafodion.dcs.Constants;
import org.trafodion.dcs.util.Bytes;
import org.trafodion.dcs.util.DcsConfiguration;
import org.trafodion.dcs.util.DcsNetworkConfiguration;
import org.trafodion.dcs.util.RetryCounter;
import org.trafodion.dcs.util.RetryCounterFactory;
import org.trafodion.dcs.zookeeper.ZkClient;
import org.trafodion.dcs.script.ScriptManager;
import org.trafodion.dcs.script.ScriptContext;

public final class ServerManager implements Callable {
    private static final Log LOG = LogFactory.getLog(ServerManager.class);
    private static Configuration conf;
    private static ZkClient zkc;
    private static boolean userProgEnabled;
    private static String userProgramHome;
    private static String userProgramCommand;
    private static String hostName;
    private static String masterHostName;
    private static long masterStartTime;
    private static int port;
    private static int portRange;
    private static DcsNetworkConfiguration netConf;
    private static int instance;
    private static int childServers;
    private static String parentZnode;
    private static int connectingTimeout;
    private static int zkSessionTimeout;
    private static int userProgExitAfterDisconnect;
    private static int infoPort;
    private static int maxHeapPctExit;
    private static int statisticsIntervalTime;
    private static int statisticsLimitTime;
    private static String statisticsType;
    private static String statisticsEnable;
    private static String sqlplanEnable;
    private static int userProgPortMapToSecs;
    private static int userProgPortBindToSecs;
    private ServerHandler[] serverHandlers;
    private int maxRestartAttempts;
    private int retryIntervalMillis;
    private int restartAttempts;
    private RetryCounterFactory retryCounterFactory;
    private String nid = null;

    class RegisteredWatcher implements Watcher {
        CountDownLatch startSignal;

        public RegisteredWatcher(CountDownLatch startSignal) {
            this.startSignal = startSignal;
        }

        public void process(WatchedEvent event) {
            if (event.getType() == Event.EventType.NodeDeleted) {
                String znodePath = event.getPath();
                LOG.debug("Registered znode deleted [" + znodePath + "]");
                try {
                    startSignal.countDown();
                } catch (Exception e) {
                    e.printStackTrace();
                    LOG.error(e);
                }
            }
        }
    }

    class ServerMonitor {
        ScriptContext scriptContext = new ScriptContext();
        int childInstance;
        String registeredPath;
        Stat stat = null;
        boolean isRunning = false;
        String nid;
        String pid;

        public ServerMonitor(int childInstance, String registeredPath) {
            this.childInstance = childInstance;
            this.registeredPath = registeredPath;
        }

        public boolean monitor() throws Exception {
            try {
                LOG.debug("registered path [" + registeredPath + "]");
                stat = zkc.exists(registeredPath, false);
                if (stat != null) { // User program znode found in
                                    // /registered...check pid
                    isRunning = isPidRunning();
                    LOG.debug("isRunning [" + isRunning + "]");
                }
            } catch (Exception e) {
                isRunning = false;
                return isRunning;
            }

            return isRunning;
        }

        private boolean isPidRunning() throws Exception {
            String data = Bytes.toString(zkc.getData(registeredPath, false,
                    stat));
            Scanner scn = new Scanner(data);
            scn.useDelimiter(":");
            scn.next();// state
            scn.next();// timestamp
            scn.next();// dialogue Id
            nid = scn.next();// nid
            pid = scn.next();// pid
            scn.close();
            scriptContext.setHostName(hostName);
            scriptContext.setScriptName(Constants.SYS_SHELL_SCRIPT_NAME);
            scriptContext.setCommand("ps -p " + pid);
            ScriptManager.getInstance().runScript(scriptContext);
            return scriptContext.getExitCode() != 0 ? false : true;
        }
    }

    class ServerRunner {
        ScriptContext scriptContext;
        String registeredPath;
        int childInstance;

        public ServerRunner(int childInstance, String registeredPath) {
            this.scriptContext = new ScriptContext();
            this.childInstance = childInstance;
            this.registeredPath = registeredPath;
            scriptContext.setHostName(hostName);
            scriptContext.setScriptName(Constants.SYS_SHELL_SCRIPT_NAME);
            String command = userProgramCommand
                    .replace("-ZKHOST", "-ZKHOST " + zkc.getZkQuorum() + " ")
                    .replace(
                            "-RZ",
                            "-RZ " + hostName + ":" + instance + ":"
                                    + childInstance + " ")
                    .replace("-ZKPNODE",
                            "-ZKPNODE " + "\"" + parentZnode + "\"" + " ")
                    .replace("-CNGTO", "-CNGTO " + connectingTimeout + " ")
                    .replace("-ZKSTO", "-ZKSTO " + zkSessionTimeout + " ")
                    .replace("-EADSCO",
                            "-EADSCO " + userProgExitAfterDisconnect + " ")
                    .replace("-TCPADD",
                            "-TCPADD " + netConf.getExtHostAddress() + " ")
                    .replace("-MAXHEAPPCT",
                            "-MAXHEAPPCT " + maxHeapPctExit + " ")
                    .replace(
                            "-STATISTICSINTERVAL",
                            "-STATISTICSINTERVAL " + statisticsIntervalTime
                                    + " ")
                    .replace("-STATISTICSLIMIT",
                            "-STATISTICSLIMIT " + statisticsLimitTime + " ")
                    .replace("-STATISTICSTYPE",
                            "-STATISTICSTYPE " + statisticsType + " ")
                    .replace("-STATISTICSENABLE",
                            "-STATISTICSENABLE " + statisticsEnable + " ")
                    .replace("-SQLPLAN", "-SQLPLAN " + sqlplanEnable + " ")
                    .replace("-PORTMAPTOSECS",
                            "-PORTMAPTOSECS " + userProgPortMapToSecs + " ")
                    .replace("-PORTBINDTOSECS",
                            "-PORTBINDTOSECS " + userProgPortBindToSecs)
                    .replace("&lt;", "<").replace("&amp;", "&")
                    .replace("&gt;", ">");
            scriptContext.setCommand(command);
        }

        public void exec() throws Exception {
            cleanupZk();
            LOG.info("User program exec [" + scriptContext.getCommand() + "]");
            ScriptManager.getInstance().runScript(scriptContext);// This will
                                                                 // block while
                                                                 // user prog is
                                                                 // running
            LOG.info("User program exit [" + scriptContext.getExitCode() + "]");
            StringBuilder sb = new StringBuilder();
            sb.append("exit code [" + scriptContext.getExitCode() + "]");
            if (!scriptContext.getStdOut().toString().isEmpty())
                sb.append(", stdout [" + scriptContext.getStdOut().toString()
                        + "]");
            if (!scriptContext.getStdErr().toString().isEmpty())
                sb.append(", stderr [" + scriptContext.getStdErr().toString()
                        + "]");
            LOG.info(sb.toString());

            switch (scriptContext.getExitCode()) {
            case 3:
                LOG.error("Trafodion is not running");
                break;
            case 127:
                LOG.error("Cannot find user program executable");
                break;
            default:
            }
        }

        private void cleanupZk() {
            try {
                Stat stat = zkc.exists(registeredPath, false);
                if (stat != null)
                    zkc.delete(registeredPath, -1);
            } catch (Exception e) {
                e.printStackTrace();
                LOG.debug(e);
            }
        }
    }

    class ServerHandler implements Callable<Integer> {
        ServerMonitor serverMonitor;
        ServerRunner serverRunner;
        int childInstance;
        String registeredPath;
        CountDownLatch startSignal = new CountDownLatch(1);
        int restartAttempts;

        public int getRestartAttempts() {
            return restartAttempts;
        }

        public void setRestartAttempts(int time) {
            this.restartAttempts = time;
        }

        public ServerHandler(int childInstance) {
            this.childInstance = childInstance;
            this.registeredPath = parentZnode
                    + Constants.DEFAULT_ZOOKEEPER_ZNODE_SERVERS_REGISTERED
                    + "/" + hostName + ":" + instance + ":" + childInstance;
            serverMonitor = new ServerMonitor(childInstance, registeredPath);
            serverRunner = new ServerRunner(childInstance, registeredPath);
        }

        @Override
        public Integer call() throws Exception {
            Integer result = new Integer(childInstance);

            if (serverMonitor.monitor()) {
                LOG.info("Server handler [" + instance + ":" + childInstance
                        + "] is running");
                zkc.exists(registeredPath, new RegisteredWatcher(startSignal));
                LOG.debug("Waiting for start signal");
                startSignal.await();
                serverRunner.exec();
            } else {
                LOG.info("Server handler [" + instance + ":" + childInstance
                        + "] is not running");
                serverRunner.exec();
            }

            return result;
        }
    }

    public ServerManager(Configuration conf, ZkClient zkc,
            DcsNetworkConfiguration netConf, String instance, int infoPort,
            int childServers) throws Exception {
        this.conf = conf;
        this.zkc = zkc;
        this.netConf = netConf;
        this.hostName = netConf.getHostName();
        this.instance = Integer.parseInt(instance);
        this.infoPort = infoPort;
        this.childServers = childServers;
        this.parentZnode = this.conf.get(Constants.ZOOKEEPER_ZNODE_PARENT,
                Constants.DEFAULT_ZOOKEEPER_ZNODE_PARENT);
        this.connectingTimeout = this.conf.getInt(
                Constants.DCS_SERVER_USER_PROGRAM_CONNECTING_TIMEOUT,
                Constants.DEFAULT_DCS_SERVER_USER_PROGRAM_CONNECTING_TIMEOUT);
        this.zkSessionTimeout = this.conf
                .getInt(Constants.DCS_SERVER_USER_PROGRAM_ZOOKEEPER_SESSION_TIMEOUT,
                        Constants.DEFAULT_DCS_SERVER_USER_PROGRAM_ZOOKEEPER_SESSION_TIMEOUT);
        this.userProgExitAfterDisconnect = this.conf
                .getInt(Constants.DCS_SERVER_USER_PROGRAM_EXIT_AFTER_DISCONNECT,
                        Constants.DEFAULT_DCS_SERVER_USER_PROGRAM_EXIT_AFTER_DISCONNECT);
        this.maxHeapPctExit = this.conf.getInt(
                Constants.DCS_SERVER_USER_PROGRAM_MAX_HEAP_PCT_EXIT,
                Constants.DEFAULT_DCS_SERVER_USER_PROGRAM_MAX_HEAP_PCT_EXIT);
        this.statisticsIntervalTime = this.conf
                .getInt(Constants.DCS_SERVER_USER_PROGRAM_STATISTICS_INTERVAL_TIME,
                        Constants.DEFAULT_DCS_SERVER_USER_PROGRAM_STATISTICS_INTERVAL_TIME);
        this.statisticsLimitTime = this.conf
                .getInt(Constants.DCS_SERVER_USER_PROGRAM_STATISTICS_LIMIT_TIME,
                        Constants.DEFAULT_DCS_SERVER_USER_PROGRAM_STATISTICS_LIMIT_TIME);
        this.statisticsType = this.conf.get(
                Constants.DCS_SERVER_USER_PROGRAM_STATISTICS_TYPE,
                Constants.DEFAULT_DCS_SERVER_USER_PROGRAM_STATISTICS_TYPE);
        this.statisticsEnable = this.conf.get(
                Constants.DCS_SERVER_USER_PROGRAM_STATISTICS_ENABLE,
                Constants.DEFAULT_DCS_SERVER_USER_PROGRAM_STATISTICS_ENABLE);
        this.sqlplanEnable = this.conf
                .get(Constants.DCS_SERVER_USER_PROGRAM_STATISTICS_SQLPLAN_ENABLE,
                        Constants.DEFAULT_DCS_SERVER_USER_PROGRAM_STATISTICS_SQLPLAN_ENABLE);
        this.userProgPortMapToSecs = this.conf
                .getInt(Constants.DCS_SERVER_USER_PROGRAM_PORT_MAP_TIMEOUT_SECONDS,
                        Constants.DEFAULT_DCS_SERVER_USER_PROGRAM_PORT_MAP_TIMEOUT_SECONDS);
        this.userProgPortBindToSecs = this.conf
                .getInt(Constants.DCS_SERVER_USER_PROGRAM_PORT_BIND_TIMEOUT_SECONDS,
                        Constants.DEFAULT_DCS_SERVER_USER_PROGRAM_PORT_BIND_TIMEOUT_SECONDS);
        this.maxRestartAttempts = conf
                .getInt(Constants.DCS_SERVER_USER_PROGRAM_RESTART_HANDLER_ATTEMPTS,
                        Constants.DEFAULT_DCS_SERVER_USER_PROGRAM_RESTART_HANDLER_ATTEMPTS);
        this.retryIntervalMillis = conf
                .getInt(Constants.DCS_SERVER_USER_PROGRAM_RESTART_HANDLER_RETRY_INTERVAL_MILLIS,
                        Constants.DEFAULT_DCS_SERVER_USER_PROGRAM_RESTART_HANDLER_RETRY_INTERVAL_MILLIS);
        this.restartAttempts = this.conf
                .getInt(Constants.DCS_MASTER_STARTUP_MXOSRVR_RETRY_ATTEMPTS,
                        Constants.DEFAULT_DCS_MASTER_STARTUP_MXOSRVR_RETRY_ATTEMPTS);
        this.retryCounterFactory = new RetryCounterFactory(
                this.maxRestartAttempts, this.retryIntervalMillis);
        serverHandlers = new ServerHandler[this.childServers];
    }

    private static boolean isTrafodionRunning(String nid) {

        // Check if the given Node is up and running
        // return true else return false.
        // Invoke sqcheck -n <nid> to check Node status. 
        // 
        //   sqcheck returns:
        //    -1 - Not up ($?=255) or node down
        //     0 - Fully up and operational or node up
        //     1 - Partially up and operational
        //     2 - Partially up and NOT operational
 
        ScriptContext scriptContext = new ScriptContext();
        scriptContext.setHostName(hostName);
        scriptContext.setScriptName(Constants.SYS_SHELL_SCRIPT_NAME);
        String command;
        if (nid != null)
           command = ("sqcheck -n " + nid);
        else
           command = ("sqcheck");
        scriptContext.setCommand(command);
        ScriptManager.getInstance().runScript(scriptContext);// This will
                                                             // block while
                                                             // script is
                                                             // running
        int exitCode = scriptContext.getExitCode();
        return (exitCode == 0 || exitCode == 1) ? true : false;
    }

    @Override
    public Boolean call() throws Exception {

        ExecutorService executorService = Executors
                .newFixedThreadPool(childServers);
        CompletionService<Integer> completionService = new ExecutorCompletionService<Integer>(
                executorService);

        try {
            getMaster();
            featureCheck();
            registerInRunning(instance);
            RetryCounter retryCounter = retryCounterFactory.create();
            while (!isTrafodionRunning(nid)) {
               if (!retryCounter.shouldRetry()) {
                  if (nid != null)
                     throw new IOException("Node " + nid + " is not Up");
                  else
                     throw new IOException("Trafodion is not running");
               } else {
                  retryCounter.sleepUntilNextRetry();
                  retryCounter.useRetry();
               }
            }

            // When started from bin/dcs-start.sh script childServers will
            // contain the
            // count passed in from the servers.sh script. However when
            // DcsServer is
            // killed or dies for any reason DcsMaster restarts it using
            // /bin/dcs-daemon script
            // which DOES NOT set childServers count.
            for (int childInstance = 1; childInstance <= childServers; childInstance++) {
                ServerHandler serverHandler = new ServerHandler(childInstance);
                serverHandler.setRestartAttempts(this.restartAttempts);
                serverHandlers[childInstance-1] = serverHandler;
                completionService.submit(serverHandlers[childInstance-1]);
                LOG.debug("Started server handler [" + instance + ":"
                        + childInstance + "]");
            }

            while (true) {
                LOG.debug("Waiting for any server handler to finish");
                Future<Integer> f = completionService.take();// blocks waiting
                                                             // for any
                                                             // ServerHandler to
                                                             // finish
                if (f != null) {
                    Integer result = f.get();
                    LOG.debug("Server handler [" + instance + ":" + result + "] finished");
                    int childInstance = result.intValue();
                    // get the node id
                    boolean isRunning = serverHandlers[childInstance - 1].serverMonitor.monitor();
                    String nid = serverHandlers[childInstance - 1].serverMonitor.nid;
                    String pid = serverHandlers[childInstance - 1].serverMonitor.pid;
                    int restartAttempts = serverHandlers[childInstance - 1].getRestartAttempts();

                    serverHandlers[childInstance - 1] = null;
                    retryCounter = retryCounterFactory.create();
                    while (!isTrafodionRunning(nid)) {
                        if (!retryCounter.shouldRetry()) {
                            throw new IOException("Node " + nid + " is not Up");
                        } else {
                            retryCounter.sleepUntilNextRetry();
                            retryCounter.useRetry();
                        }
                    }
                    if (isRunning) {
                        restartAttempts = this.restartAttempts;
                        LOG.info("mxosrvr " + nid + "," + pid + " still running");
                    }
                    else {
                        restartAttempts--;
                        LOG.info("mxosrvr " + nid + "," + pid + " exited, restarting, restart time : " + restartAttempts);
                    }
                    if (restartAttempts > 0) {
                        ServerHandler serverHandler = new ServerHandler(childInstance);
                        serverHandler.setRestartAttempts(restartAttempts);
                        serverHandlers[childInstance - 1] = serverHandler;
                        completionService.submit(serverHandlers[childInstance - 1]);
                    }
                }
            }
        } catch (Exception e) {
            e.printStackTrace();
            LOG.error(e);
            if (executorService != null)
                executorService.shutdown();
            throw e;
        }
        /*
         * ExecutorService pool = Executors.newSingleThreadExecutor();
         * 
         * try { getMaster(); registerInRunning(); featureCheck();
         * 
         * Callable<Boolean> serverMonitor = new ServerMonitor();
         * Callable<ScriptContext> serverRunner = new ServerRunner();
         * 
         * long timeoutMillis=5000;
         * 
         * while(true) { Future<Boolean> monitor = pool.submit(serverMonitor);
         * if(false == monitor.get().booleanValue()) { //blocking call
         * LOG.info("User program is not running"); Future<ScriptContext> runner
         * = pool.submit(serverRunner); ScriptContext scriptContext =
         * runner.get();//blocking call
         * 
         * StringBuilder sb = new StringBuilder(); sb.append("exit code [" +
         * scriptContext.getExitCode() + "]"); if(!
         * scriptContext.getStdOut().toString().isEmpty())
         * sb.append(", stdout [" + scriptContext.getStdOut().toString() + "]");
         * if(! scriptContext.getStdErr().toString().isEmpty())
         * sb.append(", stderr [" + scriptContext.getStdErr().toString() + "]");
         * LOG.info(sb.toString());
         * 
         * switch(scriptContext.getExitCode()) { case 3:
         * LOG.error("Trafodion is not running"); timeoutMillis=60000; break;
         * case 127: LOG.error("Cannot find user program executable");
         * timeoutMillis=60000; break; default: timeoutMillis=5000; }
         * 
         * } else { timeoutMillis=5000; }
         * 
         * try { Thread.sleep(timeoutMillis); } catch (InterruptedException e) {
         * } }
         * 
         * } catch (Exception e) { e.printStackTrace(); LOG.error(e);
         * pool.shutdown(); throw e; }
         */
    }

    private void featureCheck() {
        final String msg1 = "Property "
                + Constants.DCS_SERVER_USER_PROGRAM
                + " is false. "
                + "Please add to your dcs-site.xml file and set <value>false</value> to <value>true</value>.";
        final String msg2 = "Environment variable $TRAF_HOME is not set.";

        boolean ready = false;
        while (!ready) {
            userProgEnabled = conf.getBoolean(
                    Constants.DCS_SERVER_USER_PROGRAM,
                    Constants.DEFAULT_DCS_SERVER_USER_PROGRAM);
            userProgramHome = System.getProperty("dcs.user.program.home");
            userProgramCommand = conf.get(
                    Constants.DCS_SERVER_USER_PROGRAM_COMMAND,
                    Constants.DEFAULT_DCS_SERVER_USER_PROGRAM_COMMAND);

            if (userProgEnabled == true && userProgramHome.isEmpty() == false
                    && userProgramCommand.isEmpty() == false) {
                ready = true;
                continue;
            }

            if (userProgEnabled == false)
                LOG.error(msg1);
            if (userProgramHome.isEmpty())
                LOG.error(msg2);

            try {
                Thread.sleep(60000);
            } catch (InterruptedException e) {
            }
        }

        LOG.info("User program enabled");
    }

    private void getMaster() {
        boolean found = false;

        while (!found) {
            LOG.info("Checking DcsMaster znode [" + parentZnode
                    + Constants.DEFAULT_ZOOKEEPER_ZNODE_MASTER + "]");
            try {
                Stat stat = zkc.exists(parentZnode
                        + Constants.DEFAULT_ZOOKEEPER_ZNODE_MASTER, false);
                if (stat != null) {
                    List<String> nodes = zkc.getChildren(parentZnode
                            + Constants.DEFAULT_ZOOKEEPER_ZNODE_MASTER, null);
                    if (!nodes.isEmpty()) {
                        StringTokenizer st = new StringTokenizer(nodes.get(0),
                                ":");
                        while (st.hasMoreTokens()) {
                            masterHostName = st.nextToken();
                            port = Integer.parseInt(st.nextToken());
                            portRange = Integer.parseInt(st.nextToken());
                            masterStartTime = Long.parseLong(st.nextToken());
                        }
                        found = true;
                        LOG.info("DcsMaster znode [" + parentZnode
                                + Constants.DEFAULT_ZOOKEEPER_ZNODE_MASTER
                                + "] found");
                    }
                } else {
                    LOG.info("DcsMaster znode [" + parentZnode
                            + Constants.DEFAULT_ZOOKEEPER_ZNODE_MASTER
                            + "] not found");
                }

                if (!found) {
                    try {
                        Thread.sleep(5000);
                    } catch (InterruptedException e) {
                    }
                }

            } catch (Exception e) {
                e.printStackTrace();
                LOG.error(e);
            }
        }
    }

    private void registerInRunning(int instance) {
        String znode = parentZnode
                + Constants.DEFAULT_ZOOKEEPER_ZNODE_SERVERS_RUNNING + "/"
                + hostName + ":" + instance + ":" + infoPort + ":"
                + System.currentTimeMillis();
        try {
            Stat stat = zkc.exists(znode, false);
            if (stat == null) {
                zkc.create(znode, new byte[0], ZooDefs.Ids.OPEN_ACL_UNSAFE,
                        CreateMode.EPHEMERAL);
                LOG.info("Created znode [" + znode + "]");
            }
        } catch (KeeperException.NodeExistsException e) {
            // do nothing...leftover from previous shutdown
        } catch (Exception e) {
            e.printStackTrace();
            LOG.error(e);
        }
    }

    public String getMasterHostName() {
        return masterHostName;
    }

    public String getZKParentZnode() {
        return parentZnode;
    }

    public String getUserProgramHome() {
        return userProgramHome;
    }
}
