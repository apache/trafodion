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

import java.io.IOException;
import java.io.InputStream;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.ServerSocket;
import java.net.NetworkInterface;
import java.nio.charset.Charset;
import java.util.Enumeration;
import java.util.*;
import java.util.concurrent.Executors;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Future;
import java.util.concurrent.ExecutionException;

import org.apache.commons.io.IOUtils;
import org.apache.commons.cli.CommandLine;
import org.apache.commons.cli.GnuParser;
import org.apache.commons.cli.Options;
import org.apache.commons.cli.ParseException;
import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.apache.hadoop.conf.Configuration;
import org.apache.zookeeper.CreateMode;
import org.apache.zookeeper.ZooDefs;
import org.apache.zookeeper.data.Stat;
import org.apache.zookeeper.KeeperException;
import org.apache.zookeeper.ZooKeeper.States;

import org.apache.hadoop.util.StringUtils;

import org.trafodion.wms.Constants;
import org.trafodion.wms.util.WmsConfiguration;
import org.trafodion.wms.util.InfoServer;
import org.trafodion.wms.util.VersionInfo;
import org.trafodion.wms.zookeeper.ZkClient;
import org.trafodion.wms.zookeeper.ZKConfig;
import org.trafodion.wms.rest.WmsRest;

public class WmsMaster implements Runnable {
    private static final Log LOG = LogFactory.getLog(WmsMaster.class);
    private Thread thrd;
    private ZkClient zkc = null;
    private Configuration conf;
    private String[] args;
    private InetAddress ia;
    private String instance = null;
    private InfoServer infoServer;
    private WmsRest restServer;
    private String serverName;
    private int infoPort;
    private long startTime;
    private ServerManager serverManager;
    public static final String MASTER = "master";
    private Metrics metrics;
    private String parentZnode;
    private ExecutorService pool = null;
    private JVMShutdownHook jvmShutdownHook;

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

    public WmsMaster(String[] args) {
        this.args = args;
        conf = WmsConfiguration.create();
        parentZnode = conf.get(Constants.ZOOKEEPER_ZNODE_PARENT,
                Constants.DEFAULT_ZOOKEEPER_ZNODE_PARENT);
        jvmShutdownHook = new JVMShutdownHook();
        Runtime.getRuntime().addShutdownHook(jvmShutdownHook);
        thrd = new Thread(this);
        thrd.start();
    }

    private static int findFreePort() throws IOException {
        ServerSocket server = new ServerSocket(0);
        int port = server.getLocalPort();
        server.close();
        return port;
    }

    public void run() {
        VersionInfo.logVersion();

        Options opt = new Options();
        CommandLine cmd;
        try {
            cmd = new GnuParser().parse(opt, args);
            instance = cmd.getArgList().get(0).toString();
        } catch (NullPointerException e) {
            LOG.error("No args found: ", e);
            System.exit(1);
        } catch (ParseException e) {
            LOG.error("Could not parse: ", e);
            System.exit(1);
        }

        try {
            zkc = new ZkClient();
            zkc.connect();
            LOG.info("Connected to ZooKeeper");
        } catch (Exception e) {
            LOG.error(e);
            System.exit(1);
        }

        try {
            // Create the persistent WMS znodes
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
                    + Constants.DEFAULT_ZOOKEEPER_ZNODE_SERVERS_LEADER, false);
            if (stat == null) {
                zkc.create(parentZnode
                        + Constants.DEFAULT_ZOOKEEPER_ZNODE_SERVERS_LEADER,
                        new byte[0], ZooDefs.Ids.OPEN_ACL_UNSAFE,
                        CreateMode.PERSISTENT);
            }
            stat = zkc.exists(parentZnode
                    + Constants.DEFAULT_ZOOKEEPER_ZNODE_CLIENTS, false);
            if (stat == null) {
                zkc.create(parentZnode
                        + Constants.DEFAULT_ZOOKEEPER_ZNODE_CLIENTS,
                        new byte[0], ZooDefs.Ids.OPEN_ACL_UNSAFE,
                        CreateMode.PERSISTENT);
            }
            stat = zkc.exists(parentZnode
                    + Constants.DEFAULT_ZOOKEEPER_ZNODE_WORKLOADS, false);
            if (stat == null) {
                zkc.create(parentZnode
                        + Constants.DEFAULT_ZOOKEEPER_ZNODE_WORKLOADS,
                        new byte[0], ZooDefs.Ids.OPEN_ACL_UNSAFE,
                        CreateMode.PERSISTENT);
            }
            stat = zkc.exists(parentZnode
                    + Constants.DEFAULT_ZOOKEEPER_ZNODE_STREAMS, false);
            if (stat == null) {
                zkc.create(parentZnode
                        + Constants.DEFAULT_ZOOKEEPER_ZNODE_STREAMS,
                        new byte[0], ZooDefs.Ids.OPEN_ACL_UNSAFE,
                        CreateMode.PERSISTENT);
            }
            stat = zkc.exists(parentZnode
                    + Constants.DEFAULT_ZOOKEEPER_ZNODE_RULES, false);
            if (stat == null) {
                zkc.create(parentZnode
                        + Constants.DEFAULT_ZOOKEEPER_ZNODE_RULES, new byte[0],
                        ZooDefs.Ids.OPEN_ACL_UNSAFE, CreateMode.PERSISTENT);
            }

            String schema = IOUtils.toString(getClass().getResourceAsStream(
                    Constants.PLATFORM_STATS_SCHEMA_FILENAME));
            stat = zkc.exists(parentZnode
                    + Constants.DEFAULT_ZOOKEEPER_ZNODE_STATS, false);
            if (stat == null) {
                zkc.create(parentZnode
                        + Constants.DEFAULT_ZOOKEEPER_ZNODE_STATS,
                        schema.getBytes(), ZooDefs.Ids.OPEN_ACL_UNSAFE,
                        CreateMode.PERSISTENT);
            }
            stat = zkc.exists(parentZnode
                    + Constants.DEFAULT_ZOOKEEPER_ZNODE_TRAFODION, false);
            if (stat == null) {
                zkc.create(parentZnode
                        + Constants.DEFAULT_ZOOKEEPER_ZNODE_TRAFODION,
                        new byte[0], ZooDefs.Ids.OPEN_ACL_UNSAFE,
                        CreateMode.PERSISTENT);
            }

            schema = IOUtils.toString(getClass().getResourceAsStream(
                    Constants.TRAFODION_RMS_SCHEMA_FILENAME));
            stat = zkc.exists(parentZnode
                    + Constants.DEFAULT_ZOOKEEPER_ZNODE_TRAFODION_RMS, false);
            if (stat == null) {
                zkc.create(parentZnode
                        + Constants.DEFAULT_ZOOKEEPER_ZNODE_TRAFODION_RMS,
                        schema.getBytes(), ZooDefs.Ids.OPEN_ACL_UNSAFE,
                        CreateMode.PERSISTENT);
            }

        } catch (KeeperException.NodeExistsException e) {
            // do nothing...some other server has created znodes
        } catch (Exception e) {
            LOG.error(e);
            System.exit(0);
        }

        metrics = new Metrics();
        startTime = System.currentTimeMillis();

        try {

            ia = InetAddress.getLocalHost();

            String interfaceName = conf.get(Constants.WMS_DNS_INTERFACE,
                    Constants.DEFAULT_WMS_DNS_INTERFACE);
            if (interfaceName.equalsIgnoreCase("default")) {
                LOG.info("Using local host [" + ia.getCanonicalHostName() + ","
                        + ia.getHostAddress() + "]");
            } else {
                // For all nics get all hostnames and IPs
                // and try to match against dcs.dns.interface property
                Enumeration<NetworkInterface> nics = NetworkInterface
                        .getNetworkInterfaces();
                while (nics.hasMoreElements()) {
                    NetworkInterface ni = nics.nextElement();
                    Enumeration<InetAddress> rawAdrs = ni.getInetAddresses();
                    while (rawAdrs.hasMoreElements()) {
                        InetAddress inet = rawAdrs.nextElement();
                        LOG.info("Found interface [" + ni.getDisplayName()
                                + "," + inet.getCanonicalHostName() + ","
                                + inet.getHostAddress() + "]");
                        if (interfaceName.equalsIgnoreCase(ni.getDisplayName())
                                && inet.getCanonicalHostName().contains(".")) {
                            LOG.info("Using interface [" + ni.getDisplayName()
                                    + "," + inet.getCanonicalHostName() + ","
                                    + inet.getHostAddress() + "]");
                            ia = inet;
                            break;
                        }
                    }
                }
            }

            serverName = ia.getCanonicalHostName();

            // Register in zookeeper
            String path = parentZnode
                    + Constants.DEFAULT_ZOOKEEPER_ZNODE_MASTER + "/"
                    + ia.getCanonicalHostName() + ":" + startTime;
            zkc.create(path, new byte[0], ZooDefs.Ids.OPEN_ACL_UNSAFE,
                    CreateMode.EPHEMERAL);
            LOG.info("Created znode [" + path + "]");

            // Start the Web UI info server.
            infoPort = conf.getInt(Constants.WMS_MASTER_INFO_PORT,
                    Constants.DEFAULT_WMS_MASTER_INFO_PORT);
            if (infoPort >= 0) {
                String a = conf.get(Constants.WMS_MASTER_INFO_BIND_ADDRESS,
                        Constants.DEFAULT_WMS_MASTER_INFO_BIND_ADDRESS);
                infoServer = new InfoServer(MASTER, a, infoPort, false, conf);
                infoServer.addServlet("status", "/master-status",
                        MasterStatusServlet.class);
                infoServer.setAttribute(MASTER, this);
                infoServer.start();
            }

            // Start the REST service
            restServer = new WmsRest(conf);
            LOG.info("REST service listening on port ["
                    + conf.getInt("wms.rest.port", 8080) + "]");

            // Start the server manager
            pool = Executors.newSingleThreadExecutor();
            serverManager = new ServerManager(this);
            Future future = pool.submit(serverManager);
            future.get();// blocking call

        } catch (Exception e) {
            LOG.error(e);
            e.printStackTrace();
            pool.shutdown();
            System.exit(0);
        }
    }

    public InetAddress getInetAddress() {
        return ia;
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

    public long getStartTime() {
        return startTime;
    }

    public ZkClient getZkClient() {
        return zkc;
    }

    public String getZKQuorumServersString() {
        return ZKConfig.getZKQuorumServersString(conf);
    }

    public String getZKParentZnode() {
        return parentZnode;
    }

    public Metrics getMetrics() {
        return metrics;
    }

    public static void main(String[] args) {
        WmsMaster server = new WmsMaster(args);
    }
}
