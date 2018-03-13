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

import java.io.IOException;
import java.util.concurrent.Callable;
import java.util.concurrent.CompletionService;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.ExecutorCompletionService;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.Future;
import java.util.concurrent.TimeUnit;

import org.apache.commons.cli.CommandLine;
import org.apache.commons.cli.GnuParser;
import org.apache.commons.cli.Options;
import org.apache.commons.cli.ParseException;
import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.apache.hadoop.conf.Configuration;
import org.apache.zookeeper.CreateMode;
import org.apache.zookeeper.KeeperException;
import org.apache.zookeeper.ZooDefs;
import org.apache.zookeeper.data.Stat;
import org.trafodion.dcs.Constants;
import org.trafodion.dcs.master.listener.ListenerService;
import org.trafodion.dcs.master.listener.ListenerWorker;
import org.trafodion.dcs.util.DcsConfiguration;
import org.trafodion.dcs.util.DcsNetworkConfiguration;
import org.trafodion.dcs.util.InfoServer;
import org.trafodion.dcs.util.RetryCounter;
import org.trafodion.dcs.util.RetryCounterFactory;
import org.trafodion.dcs.util.VersionInfo;
import org.trafodion.dcs.zookeeper.ZKConfig;
import org.trafodion.dcs.zookeeper.ZkClient;

public class DcsMaster implements Callable<Integer> {
    private static final Log LOG = LogFactory.getLog(DcsMaster.class);
    private Thread thrd;
    private ZkClient zkc = null;
    private Configuration conf;
    private DcsNetworkConfiguration netConf;
    private String[] args;
    private String instance = null;
    private int port;
    private int portRange;
    private InfoServer infoServer;
    private String serverName;
    private int infoPort;
    private long startTime;
    private ServerManager serverManager;
    private ListenerService ls;
    public static final String MASTER = "master";
    private Metrics metrics;
    private String parentZnode;
    private ExecutorService pool = null;
    private JVMShutdownHook jvmShutdownHook;
    private static String trafodionHome;
    private CountDownLatch isLeader = new CountDownLatch(1);

    private MasterLeaderElection mle = null;

    private class JVMShutdownHook extends Thread {
        public void run() {
            LOG.debug("JVM shutdown hook is running");
            try {
                zkc.close();
            } catch (InterruptedException ie) {
            }
            ;
        }
    }

    public DcsMaster(String[] args) {
        this.args = args;
        conf = DcsConfiguration.create();
        port = conf.getInt(Constants.DCS_MASTER_PORT,
                Constants.DEFAULT_DCS_MASTER_PORT);
        portRange = conf.getInt(Constants.DCS_MASTER_PORT_RANGE,
                Constants.DEFAULT_DCS_MASTER_PORT_RANGE);
        parentZnode = conf.get(Constants.ZOOKEEPER_ZNODE_PARENT,
                Constants.DEFAULT_ZOOKEEPER_ZNODE_PARENT);
        trafodionHome = System.getProperty(Constants.DCS_TRAFODION_HOME);
        jvmShutdownHook = new JVMShutdownHook();
        Runtime.getRuntime().addShutdownHook(jvmShutdownHook);

        // 35000 * 15mins ~= 1 years
        RetryCounter retryCounter = RetryCounterFactory.create(35000, 15, TimeUnit.MINUTES);
        ExecutorService executorService = Executors.newFixedThreadPool(1);
        CompletionService<Integer> completionService = new ExecutorCompletionService<Integer>(executorService);

        while (true) {
            completionService.submit(this);
            Future<Integer> f = null;
            try {
                f = completionService.take();
                if (f != null) {
                    Integer status = f.get();
                    if (status <= 0) {
                        System.exit(status);
                    } else if (status == 1) {
                        if (retryCounter.shouldRetry()) {
                            retryCounter.sleepUntilNextRetry();
                            retryCounter.useRetry();
                        } else {
                            System.exit(-2);
                        }
                        // reset lock
                        isLeader = new CountDownLatch(1);
                        break;
                    } else {
                        //TODO for other unknown status
                    }
                }
            } catch (InterruptedException | ExecutionException e) {
                LOG.error(e.getMessage(), e);
            }
        }

    }

    // return value lesser than 0, means can't recover exception exit.
    //    -1 configure error
    //    -2 retry exhaust
    // return value greater than 0 , means exception can be recover.
    //    1 means network error, retry till network recover.
    // return value equals 0, means unknow exception, do exit now.
    //    change value other than 0 when confirm the exception real reason.
    public Integer call() {
        VersionInfo.logVersion();

        Options opt = new Options();
        CommandLine cmd;
        try {
            cmd = new GnuParser().parse(opt, args);
            String s = cmd.getArgList().get(0).toString();
            Integer i = Integer.parseInt(s);
            instance = s;
        } catch (NumberFormatException nfe) {
            instance = "1";
        } catch (NullPointerException e) {
            LOG.error("No args found: ", e);
            return -1;
        } catch (ParseException e) {
            LOG.error("Could not parse: ", e);
            return -1;
        }

        try {
            zkc = new ZkClient();
            zkc.connect();
            LOG.info("Connected to ZooKeeper");
        } catch (IOException | InterruptedException e) {
            LOG.error(e.getMessage(), e);
            return 1;
        }

        try {
            // Create the persistent DCS znodes
            Stat stat = zkc.exists(parentZnode, false);
            if (stat == null) {
                zkc.create(parentZnode, new byte[0],
                        ZooDefs.Ids.OPEN_ACL_UNSAFE, CreateMode.PERSISTENT);
            }
            stat = zkc.exists(parentZnode
                    + Constants.DEFAULT_ZOOKEEPER_ZNODE_PARENT, false);
            if (stat == null) {
                zkc.create(parentZnode
                        + Constants.DEFAULT_ZOOKEEPER_ZNODE_PARENT,
                        new byte[0], ZooDefs.Ids.OPEN_ACL_UNSAFE,
                        CreateMode.PERSISTENT);
            }
            stat = zkc.exists(parentZnode
                    + Constants.DEFAULT_ZOOKEEPER_ZNODE_MASTER, false);
            if (stat == null) {
                zkc.create(parentZnode
                        + Constants.DEFAULT_ZOOKEEPER_ZNODE_MASTER,
                        new byte[0], ZooDefs.Ids.OPEN_ACL_UNSAFE,
                        CreateMode.PERSISTENT);
            }
            stat = zkc.exists(parentZnode
                    + Constants.DEFAULT_ZOOKEEPER_ZNODE_MASTER_LEADER, false);
            if (stat == null) {
                zkc.create(parentZnode
                        + Constants.DEFAULT_ZOOKEEPER_ZNODE_MASTER_LEADER,
                        new byte[0], ZooDefs.Ids.OPEN_ACL_UNSAFE,
                        CreateMode.PERSISTENT);
            }
            stat = zkc.exists(parentZnode
                    + Constants.DEFAULT_ZOOKEEPER_ZNODE_SERVERS, false);
            if (stat == null) {
                zkc.create(parentZnode
                        + Constants.DEFAULT_ZOOKEEPER_ZNODE_SERVERS,
                        new byte[0], ZooDefs.Ids.OPEN_ACL_UNSAFE,
                        CreateMode.PERSISTENT);
            }
            stat = zkc.exists(parentZnode
                    + Constants.DEFAULT_ZOOKEEPER_ZNODE_SERVERS_RUNNING, false);
            if (stat == null) {
                zkc.create(parentZnode
                        + Constants.DEFAULT_ZOOKEEPER_ZNODE_SERVERS_RUNNING,
                        new byte[0], ZooDefs.Ids.OPEN_ACL_UNSAFE,
                        CreateMode.PERSISTENT);
            }
            stat = zkc.exists(parentZnode
                    + Constants.DEFAULT_ZOOKEEPER_ZNODE_SERVERS_REGISTERED,
                    false);
            if (stat == null) {
                zkc.create(parentZnode
                        + Constants.DEFAULT_ZOOKEEPER_ZNODE_SERVERS_REGISTERED,
                        new byte[0], ZooDefs.Ids.OPEN_ACL_UNSAFE,
                        CreateMode.PERSISTENT);
            }
        } catch (KeeperException.NodeExistsException e) {
            // do nothing...some other server has created znodes
            LOG.warn(e.getMessage(), e);
        } catch (Exception e) {
            LOG.error(e.getMessage(), e);
            return 0;
        }

        metrics = new Metrics();
        startTime = System.currentTimeMillis();

        try {
            netConf = new DcsNetworkConfiguration(conf);
            serverName = netConf.getHostName();
            if (serverName == null) {
                LOG.error("DNS Interface [" + conf.get(Constants.DCS_DNS_INTERFACE, Constants.DEFAULT_DCS_DNS_INTERFACE)
                        + "] configured in dcs.site.xml is not found!");
                return -1;
            }

            // Wait to become the leader of all DcsMasters
            mle = new MasterLeaderElection(this);
            isLeader.await();

            String path = parentZnode
                    + Constants.DEFAULT_ZOOKEEPER_ZNODE_MASTER + "/"
                    + netConf.getHostName() + ":" + port + ":" + portRange
                    + ":" + startTime;
            zkc.create(path, new byte[0], ZooDefs.Ids.OPEN_ACL_UNSAFE,
                    CreateMode.EPHEMERAL);
            // Add a check path here for session expired situation,
            // if there meets session expired, use the mark to compare with the exist znode,
            // if not match, that means a backup master take over the master role.
            zkc.setCheckPath(path);

            LOG.info("Created znode [" + path + "]");

            int requestTimeout = conf.getInt(
                    Constants.DCS_MASTER_LISTENER_REQUEST_TIMEOUT,
                    Constants.DEFAULT_LISTENER_REQUEST_TIMEOUT);
            int selectorTimeout = conf.getInt(
                    Constants.DCS_MASTER_LISTENER_SELECTOR_TIMEOUT,
                    Constants.DEFAULT_LISTENER_SELECTOR_TIMEOUT);
            ls = new ListenerService(zkc, netConf, port, portRange,
                    requestTimeout, selectorTimeout, metrics, parentZnode);
            LOG.info("Listening for clients on port [" + port + "]");
            serverName = netConf.getHostName();

            // Start the info server.
            infoPort = conf.getInt(Constants.DCS_MASTER_INFO_PORT,
                    Constants.DEFAULT_DCS_MASTER_INFO_PORT);
            if (infoPort >= 0) {
                String a = conf.get(Constants.DCS_MASTER_INFO_BIND_ADDRESS,
                        Constants.DEFAULT_DCS_MASTER_INFO_BIND_ADDRESS);
                infoServer = new InfoServer(MASTER, a, infoPort, false, conf);
                infoServer.addServlet("status", "/master-status",
                        MasterStatusServlet.class);
                infoServer.setAttribute(MASTER, this);
                infoServer.start();
            }

            pool = Executors.newSingleThreadExecutor();
            serverManager = new ServerManager(this, conf, zkc, netConf,
                    startTime, metrics);
            Future future = pool.submit(serverManager);
            future.get();// block

        } catch (Exception e) {
            LOG.error(e.getMessage(), e);
            try {
                FloatingIp floatingIp = FloatingIp.getInstance(this);
                floatingIp.unbindScript();
            } catch (Exception e1) {
                if (LOG.isErrorEnabled()) {
                    LOG.error("Error creating class FloatingIp [" + e1.getMessage() + "]", e1);
                }
            }

            if (pool != null) {
                try {
                    pool.shutdown();
                    LOG.info("Interrupt listenerService.");
                } catch (Exception e2) {
                    LOG.error("Error while shutdown ServerManager thread [" + e2.getMessage() + "]", e2);
                }
            }

            if (ls != null) {
                try {
                    ListenerWorker lw = ls.getWorker();
                    if (lw != null) {
                        lw.interrupt();
                        LOG.info("Interrupt listenerWorker.");
                    }
                    ls.interrupt();
                    LOG.info("Interrupt listenerService.");
                } catch (Exception e2) {
                    LOG.error("Error while shutdown ListenerService thread [" + e2.getMessage() + "]", e2);
                }
            }
            if (infoServer != null) {
                try {
                    infoServer.stop();
                    LOG.info("Stop infoServer.");
                } catch (Exception e2) {
                    LOG.error("Error while shutdown InfoServer thread [" + e2.getMessage(), e2);
                }
            }
            return 1;

        }
        return 0;
    }

    public String getServerName() {
        return serverName;
    }

    public int getInfoPort() {
        return infoPort;
    }

    public Configuration getConfiguration() {
        return conf;
    }

    public ServerManager getServerManager() {
        return serverManager;
    }

    public ListenerService getListenerService() {
        return ls;
    }

    public long getStartTime() {
        return startTime;
    }

    public int getPort() {
        return port;
    }

    public int getPortRange() {
        return portRange;
    }

    public String getZKQuorumServersString() {
        return ZKConfig.getZKQuorumServersString(conf);
    }

    public String getZKParentZnode() {
        return parentZnode;
    }

    public String getMetrics() {
        return metrics.toString();
    }

    public String getTrafodionHome() {
        return trafodionHome;
    }

    public ZkClient getZkClient() {
        return zkc;
    }

    public String getInstance() {
        return instance;
    }

    public boolean isFollower() {
        return mle.isFollower();
    }

    public void setIsLeader() {
        isLeader.countDown();
    }

    public DcsNetworkConfiguration getNetConf() {
        return netConf;
    }

    public static void main(String[] args) {
        DcsMaster server = new DcsMaster(args);
    }
}
