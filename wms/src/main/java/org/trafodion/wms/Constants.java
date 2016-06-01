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

package org.trafodion.wms;

import java.util.Arrays;
import java.util.Collections;
import java.util.List;
import java.util.UUID;
import java.util.regex.Pattern;

import org.apache.commons.lang.ArrayUtils;
//import org.apache.hadoop.hbase.ipc.HRegionInterface;
import org.trafodion.wms.util.Bytes;

/**
 * Constants holds a bunch of Wms-related constants
 */
public final class Constants {

  /** long constant for zero */
  public static final Long ZERO_L = Long.valueOf(0L);
  public static final String NINES = "99999999999999";
  public static final String ZEROES = "00000000000000";
 
  /** name of version file */
  public static final String VERSION_FILE_NAME = "wms.version";
  
  /** long constant for thirty */
  public static final long THIRTY_SECONDS = 30000L;

  //Workload Key strings
  public static final String OPERATION = "operation";
  public static final String USER_NAME = "userName";
  public static final String TYPE = "type";
  public static final String WORKLOAD_ID = "workloadId";
  public static final String STATE = "state";
  public static final String SUBSTATE = "subState";
  public static final String BEGIN_TIMESTAMP = "beginTimestamp";
  public static final String END_TIMESTAMP = "endTimestamp";
  
  public static final String RUNNING = "RUNNING";
  public static final String BEGIN = "BEGIN";
  public static final String UPDATE = "UPDATE";
  public static final String END = "END";
  public static final String COMPLETED = "COMPLETED";
  public static final String CLIENT_DISAPPEARED = "CLIENT DISAPPEARED";
   
  public static final String TRAFODION_RMS_SCHEMA_FILENAME = "/trafodion-rms.avsc";
  public static final String PLATFORM_STATS_SCHEMA_FILENAME = "/platform-stats.avsc";
  
  //Default CEP streams
  public static final String TRAFODION = "trafodion";
  public static final String VERTICA = "vertica";
  public static final String ACTION = "action";

  // Configuration parameters

  /** Cluster is in distributed mode or not */
  public static final String CLUSTER_DISTRIBUTED = "wms.cluster.distributed";

  /** Cluster is standalone or pseudo-distributed */
  public static final boolean CLUSTER_IS_LOCAL = false;

  /** Cluster is fully-distributed */
  public static final boolean CLUSTER_IS_DISTRIBUTED = true;

  /** Default value for cluster distributed mode */
  public static final boolean DEFAULT_CLUSTER_DISTRIBUTED = CLUSTER_IS_LOCAL;

  /** default host address */
  public static final String DEFAULT_HOST = "0.0.0.0";
  
  /** Configuration key for WMS server restart handler attempts */
  public static final String WMS_MASTER_SERVER_RESTART_HANDLER_ATTEMPTS = "wms.master.server.restart.handler.attempts";

  /** Default value for WMS server restart handler attempts */
  public static final int DEFAULT_WMS_MASTER_SERVER_RESTART_HANDLER_ATTEMPTS = 3;
  
  /** Configuration key for WMS server restart handler retry interval millis */
  public static final String WMS_MASTER_SERVER_RESTART_HANDLER_RETRY_INTERVAL_MILLIS = "wms.master.server.restart.handler.retry.interval.millis";
  
  /** Default value for WMS server restart handler retry interval millis */
  public static final int DEFAULT_WMS_MASTER_SERVER_RESTART_HANDLER_RETRY_INTERVAL_MILLIS = 1000;


  /** Name of ZooKeeper quorum configuration parameter. */
  public static final String ZOOKEEPER_QUORUM = "wms.zookeeper.quorum";

  /** Name of ZooKeeper config file in conf/ directory. */
  public static final String ZOOKEEPER_CONFIG_NAME = "zoo.cfg";

  /** Common prefix of ZooKeeper configuration properties */
  public static final String ZK_CFG_PROPERTY_PREFIX =
      "wms.zookeeper.property.";

  public static final int ZK_CFG_PROPERTY_PREFIX_LEN =
      ZK_CFG_PROPERTY_PREFIX.length();

  /**
   * The ZK client port key in the ZK properties map. The name reflects the
   * fact that this is not an HBase configuration key.
   */
  public static final String CLIENT_PORT_STR = "clientPort";

  /** Parameter name for the client port that the zookeeper listens on */
  public static final String ZOOKEEPER_CLIENT_PORT =
      ZK_CFG_PROPERTY_PREFIX + CLIENT_PORT_STR;

  /** Default client port that the zookeeper listens on */
  public static final int DEFAULT_ZOOKEEPER_CLIENT_PORT = 2181;

  /** Parameter name for the wait time for the recoverable zookeeper */
  public static final String ZOOKEEPER_RECOVERABLE_WAITTIME = "wms.zookeeper.recoverable.waittime";

  /** Default wait time for the recoverable zookeeper */
  public static final long DEFAULT_ZOOKEPER_RECOVERABLE_WAITIME = 10000;

  /** Parameter name for the root dir in ZK for this cluster */
  public static final String ZOOKEEPER_ZNODE_PARENT = "zookeeper.znode.parent";
  public static final String DEFAULT_ZOOKEEPER_ZNODE_PARENT = "/wms";
  public static final String DEFAULT_ZOOKEEPER_ZNODE_MASTER = DEFAULT_ZOOKEEPER_ZNODE_PARENT + "/master";
  public static final String DEFAULT_ZOOKEEPER_ZNODE_SERVERS = DEFAULT_ZOOKEEPER_ZNODE_PARENT + "/servers";
  public static final String DEFAULT_ZOOKEEPER_ZNODE_SERVERS_LEADER = DEFAULT_ZOOKEEPER_ZNODE_SERVERS + "/leader";
  public static final String DEFAULT_ZOOKEEPER_ZNODE_SERVERS_RUNNING = DEFAULT_ZOOKEEPER_ZNODE_SERVERS + "/running";
  public static final String DEFAULT_ZOOKEEPER_ZNODE_CLIENTS = DEFAULT_ZOOKEEPER_ZNODE_PARENT + "/clients";
  public static final String DEFAULT_ZOOKEEPER_ZNODE_STATS = DEFAULT_ZOOKEEPER_ZNODE_PARENT + "/stats";
  public static final String DEFAULT_ZOOKEEPER_ZNODE_TRAFODION = DEFAULT_ZOOKEEPER_ZNODE_PARENT + "/trafodion";
  public static final String DEFAULT_ZOOKEEPER_ZNODE_TRAFODION_RMS = DEFAULT_ZOOKEEPER_ZNODE_TRAFODION + "/rms";
  public static final String DEFAULT_ZOOKEEPER_ZNODE_WORKLOADS = DEFAULT_ZOOKEEPER_ZNODE_PARENT + "/workloads";
  public static final String DEFAULT_ZOOKEEPER_ZNODE_STREAMS = DEFAULT_ZOOKEEPER_ZNODE_PARENT + "/streams";
  public static final String DEFAULT_ZOOKEEPER_ZNODE_RULES = DEFAULT_ZOOKEEPER_ZNODE_PARENT + "/rules";


  /**
   * Parameter name for the limit on concurrent client-side zookeeper
   * connections
   */
  public static final String ZOOKEEPER_MAX_CLIENT_CNXNS =
      ZK_CFG_PROPERTY_PREFIX + "maxClientCnxns";

  /** Parameter name for the ZK data directory */
  public static final String ZOOKEEPER_DATA_DIR =
      ZK_CFG_PROPERTY_PREFIX + "dataDir";

  /** Default limit on concurrent client-side zookeeper connections */
  public static final int DEFAULT_ZOOKEPER_MAX_CLIENT_CNXNS = 300;

  /** Configuration key for ZooKeeper session timeout */
  public static final String ZK_SESSION_TIMEOUT = "zookeeper.session.timeout";

  /** Default value for ZooKeeper session timeout */
  public static final int DEFAULT_ZK_SESSION_TIMEOUT = 180 * 1000;
  
  /** Configuration key for ZooKeeper recovery retry */
  public static final String ZK_RECOVERY_RETRY = "zookeeper.recovery.retry";
  
  /** Default value for ZooKeeper recovery retry */
  public static final int DEFAULT_ZK_RECOVERY_RETRY = 3;
  
  /** Configuration key for ZooKeeper recovery retry interval millis */
  public static final String ZK_RECOVERY_RETRY_INTERVAL_MILLIS = "zookeeper.recovery.retry.intervalmillis";
  
  /** Default value for ZooKeeper recovery retry interval millis */
  public static final int DEFAULT_ZK_RECOVERY_RETRY_INTERVAL_MILLIS = 1000;

  /** Configuration key for whether to use ZK.multi */
  public static final String ZOOKEEPER_USEMULTI = "wms.zookeeper.useMulti";

  /** When we encode strings, we always specify UTF8 encoding */
  public static final String UTF8_ENCODING = "UTF-8";

  /**
   * Unlimited time-to-live.
   */
//  public static final int FOREVER = -1;
  public static final int FOREVER = Integer.MAX_VALUE;

  /**
   * Seconds in a week
   */
  public static final int WEEK_IN_SECONDS = 7 * 24 * 3600;

  /** Host name of the local machine */
  public static final String LOCALHOST = "localhost";

  /** window for calculating cpu busy */
  public static final int CPU_WINDOW = 30;

  /** delay before next calculation of memusage and cpubusy */
  public static final int PLATFORM_STATS_DELAY = 30 * 1000;

  /** Yarn Rest Client Configuration */
  public static final String YARN_REST_ENABLED = "yarn.rest.enabled";
  public static final String YARN_REST_URL = "yarn.rest.url";
  public static final String DEFAULT_YARN_REST_URL = "http://localhost:8088/ws/v1/cluster/apps";
  public static final String YARN_RM_ADDRESS = "yarn.resourcemanager.address";
  public static final int DEFAULT_RM_PORT = 8032;
  public static final String DEFAULT_YARN_RM_ADDRESS = "0.0.0.0:" + DEFAULT_RM_PORT;
  
  /** Configuration key for WMS master info port */
  public static final String WMS_MASTER_INFO_PORT = "wms.master.info.port";
  /** Default value for WMS master info port */
  public static final int DEFAULT_WMS_MASTER_INFO_PORT = 40040;
  
  /** Configuration key for WMS server info port */
  public static final String WMS_SERVER_INFO_PORT = "wms.server.info.port";
  /** Default value for WMS server info port */
  public static final int DEFAULT_WMS_SERVER_INFO_PORT = 40050;
  
  /** A flag that enables automatic selection of server info port */
  public static final String WMS_SERVER_INFO_PORT_AUTO = "wms.server.info.port.auto";
  
  /** Configuration key for WMS master info bind address */
  public static final String WMS_MASTER_INFO_BIND_ADDRESS = "wms.master.info.bindAddress";
  /** Default value for WMS master info bind address */
  public static final String DEFAULT_WMS_MASTER_INFO_BIND_ADDRESS = "0.0.0.0";
  
  /** Configuration key for WMS server info bind address */
  public static final String WMS_SERVER_INFO_BIND_ADDRESS = "wms.server.info.bindAddress";
  /** Default value for WMS server info bind address */
  public static final String DEFAULT_WMS_SERVER_INFO_BIND_ADDRESS = "0.0.0.0";
  
  /** Configuration key for WMS DNS interface */
  public static final String WMS_DNS_INTERFACE = "wms.dns.interface";
  /** Default value for WMS DNS interface */
  public static final String DEFAULT_WMS_DNS_INTERFACE = "default";
 
  /** Configuration key for WMS server workload store cleaner initial delay */
  public static final String WMS_SERVER_WORKLOAD_CLEANER_INITIAL_DELAY = "wms.server.workload.cleaner.inital.delay";
  /** Default value in seconds for Workload store cleaner initial delay */
  public static final int DEFAULT_WMS_SERVER_WORKLOAD_CLEANER_INITIAL_DELAY = 30;
  
  /** Configuration key for WMS server workload store cleaner period */
  public static final String WMS_SERVER_WORKLOAD_CLEANER_PERIOD = "wms.server.workload.cleaner.period";
  /** Default value in seconds for Workload store cleaner period */
  public static final int DEFAULT_WMS_SERVER_WORKLOAD_CLEANER_PERIOD = 30;
  
  /** Configuration key for CEP init retry attempts */
  public static final String WMS_CEP_WAIT_INIT_ATTEMPTS = "wms.cep.wait.init.attempts";

  /** Default value for CEP init retry attempts */
  public static final int DEFAULT_WMS_CEP_WAIT_INIT_ATTEMPTS = 3;
  
  /** Configuration key for CEP init retry interval millis */
  public static final String WMS_CEP_WAIT_INIT_RETRY_INTERVAL_MILLIS = "wms.cep.wait.init.retry.interval.millis";
  
  /** Default value for WMS server restart handler retry interval millis */
  public static final int DEFAULT_WMS_CEP_WAIT_INIT_RETRY_INTERVAL_MILLIS = 1000;

  
  private Constants() {
    // Can't be instantiated with this ctor.
  }
}
