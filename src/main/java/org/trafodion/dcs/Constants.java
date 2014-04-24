/**
 *(C) Copyright 2013 Hewlett-Packard Development Company, L.P.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/**
 * Copyright 2010 The Apache Software Foundation
 *
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package org.trafodion.dcs;

import java.util.Arrays;
import java.util.Collections;
import java.util.List;
import java.util.UUID;
import java.util.regex.Pattern;

import org.apache.commons.lang.ArrayUtils;
//import org.apache.hadoop.hbase.ipc.HRegionInterface;
import org.trafodion.dcs.util.Bytes;

/**
 * Constants holds a bunch of dcs-related constants
 */
public final class Constants {

  /** long constant for zero */
  public static final Long ZERO_L = Long.valueOf(0L);
  public static final String NINES = "99999999999999";
  public static final String ZEROES = "00000000000000";
 
  /** name of version file */
  public static final String VERSION_FILE_NAME = "dcs.version";
  
  /** long constant for thirty */
  public static final Long THIRTY_SECONDS = Long.valueOf(30000L);

  // Configuration parameters

  /** default host address */
  public static final String DEFAULT_HOST = "0.0.0.0";
  
  /** Configuration key for DCS master port */
  public static final String DCS_MASTER_PORT = "dcs.master.port";

  /** Default value for DCS master port */
  public static final int DEFAULT_DCS_MASTER_PORT = 18650;
  
  /** Configuration key for DCS master port range */
  public static final String DCS_MASTER_PORT_RANGE = "dcs.master.port.range";

  /** Default value for DCS master port range */
  public static final int DEFAULT_DCS_MASTER_PORT_RANGE = 100;
  
  /** Configuration key for DCS server restart handler attempts */
  public static final String DCS_MASTER_SERVER_RESTART_HANDLER_ATTEMPTS = "dcs.master.server.restart.handler.attempts";

  /** Default value for DCS server restart handler attempts */
  public static final int DEFAULT_DCS_MASTER_SERVER_RESTART_HANDLER_ATTEMPTS = 3;
  
  /** Configuration key for DCS server restart handler retry interval millis */
  public static final String DCS_MASTER_SERVER_RESTART_HANDLER_RETRY_INTERVAL_MILLIS = "dcs.master.server.restart.handler.retry.interval.millis";
  
  /** Default value for DCS server restart handler retry interval millis */
  public static final int DEFAULT_DCS_MASTER_SERVER_RESTART_HANDLER_RETRY_INTERVAL_MILLIS = 1000;
   
  /** User program feature is enabled */
  public static final String DCS_SERVER_USER_PROGRAM = "dcs.server.user.program";

  /** DcsServer doesn't start user program */
  public static final boolean DCS_SERVER_USER_PROGRAM_IS_NOT_ENABLED = false;

  /** DcsServer starts user program */
  public static final boolean DCS_SERVER_USER_PROGRAM_IS_ENABLED = true;

  /** Default value for DcsServer user program feature */
  public static final boolean DEFAULT_DCS_SERVER_USER_PROGRAM = DCS_SERVER_USER_PROGRAM_IS_ENABLED;
  
  /** DCS server user program command */
  public static final String DCS_SERVER_USER_PROGRAM_COMMAND = "dcs.server.user.program.command";

  /** Default value for DCS server user program command */
  public static final String DEFAULT_DCS_SERVER_USER_PROGRAM_COMMAND = "";
  
  /** Configuration key for DCS server user program connecting timeout */
  public static final String DCS_SERVER_USER_PROGRAM_CONNECTING_TIMEOUT = "dcs.server.user.program.connecting.timeout";
  
  /** Default value for DCS server user program connecting timeout */
  public static final int DEFAULT_DCS_SERVER_USER_PROGRAM_CONNECTING_TIMEOUT = 60; 
  
  /** Configuration key for DCS server user program zookeeper session timeout */
  public static final String DCS_SERVER_USER_PROGRAM_ZOOKEEPER_SESSION_TIMEOUT = "dcs.server.user.program.zookeeper.session.timeout";
  
  /** Default value for DCS server user program zookeeper session timeout */
  public static final int DEFAULT_DCS_SERVER_USER_PROGRAM_ZOOKEEPER_SESSION_TIMEOUT = 30000;  
 
  /** Configuration key for DCS server user program exit after disconnect */
  public static final String DCS_SERVER_USER_PROGRAM_EXIT_AFTER_DISCONNECT = "dcs.server.user.program.exit.after.disconnect";
  
  /** Default value for DCS server user program exit after disconnect */
  public static final int DEFAULT_DCS_SERVER_USER_PROGRAM_EXIT_AFTER_DISCONNECT = 0;  
    
  /** Name of ZooKeeper quorum configuration parameter. */
  public static final String ZOOKEEPER_QUORUM = "dcs.zookeeper.quorum";

  /** Name of ZooKeeper config file in conf/ directory. */
  public static final String ZOOKEEPER_CONFIG_NAME = "zoo.cfg";

  /** Common prefix of ZooKeeper configuration properties */
  public static final String ZK_CFG_PROPERTY_PREFIX =
      "dcs.zookeeper.property.";

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
  public static final String ZOOKEEPER_RECOVERABLE_WAITTIME = "dcs.zookeeper.recoverable.waittime";

  /** Default wait time for the recoverable zookeeper */
  public static final long DEFAULT_ZOOKEPER_RECOVERABLE_WAITIME = 10000;

  /** Parameter name for the root dir in ZK for this cluster */
  public static final String ZOOKEEPER_ZNODE_PARENT = "zookeeper.znode.parent";
  public static final String DEFAULT_ZOOKEEPER_ZNODE_PARENT = "/dcs";
  public static final String DEFAULT_ZOOKEEPER_ZNODE_MASTER = DEFAULT_ZOOKEEPER_ZNODE_PARENT + "/master";
  public static final String DEFAULT_ZOOKEEPER_ZNODE_SERVERS = DEFAULT_ZOOKEEPER_ZNODE_PARENT + "/servers";
  public static final String DEFAULT_ZOOKEEPER_ZNODE_SERVERS_RUNNING = DEFAULT_ZOOKEEPER_ZNODE_SERVERS + "/running";
  public static final String DEFAULT_ZOOKEEPER_ZNODE_SERVERS_REGISTERED = DEFAULT_ZOOKEEPER_ZNODE_SERVERS + "/registered";
  
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
  public static final String ZOOKEEPER_USEMULTI = "dcs.zookeeper.useMulti";

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
  
  /** Configuration key for Listener request timeout */
  public static final String DCS_MASTER_LISTENER_REQUEST_TIMEOUT = "dcs.master.listener.request.timeout";
  /** Listener default request timeout */
  public static final int DEFAULT_LISTENER_REQUEST_TIMEOUT = 30 * 1000; // 30 seconds
  
  /** Configuration key for Listener selector timeout */
  public static final String DCS_MASTER_LISTENER_SELECTOR_TIMEOUT = "dcs.master.listener.selector.timeout";
  /** Listener default selector timeout */
  public static final int DEFAULT_LISTENER_SELECTOR_TIMEOUT = 10 * 1000; // 10 seconds
  
  /** Configuration key for DCS master info port */
  public static final String DCS_MASTER_INFO_PORT = "dcs.master.info.port";
  /** Default value for DCS master info port */
  public static final int DEFAULT_DCS_MASTER_INFO_PORT = 40010;
  
  /** Configuration key for DCS server info port */
  public static final String DCS_SERVER_INFO_PORT = "dcs.server.info.port";
  /** Default value for DCS server info port */
  public static final int DEFAULT_DCS_SERVER_INFO_PORT = 40030;
  
  /** A flag that enables automatic selection of regionserver info port */
  public static final String DCS_SERVER_INFO_PORT_AUTO =
    "dcs.server.info.port.auto";
  
  /** Configuration key for DCS master info bind address */
  public static final String DCS_MASTER_INFO_BIND_ADDRESS = "dcs.master.info.bindAddress";
  /** Default value for DCS master info bind address */
  public static final String DEFAULT_DCS_MASTER_INFO_BIND_ADDRESS = "0.0.0.0";
  
  /** Configuration key for DCS server info bind address */
  public static final String DCS_SERVER_INFO_BIND_ADDRESS = "dcs.server.info.bindAddress";
  /** Default value for DCS server info bind address */
  public static final String DEFAULT_DCS_SERVER_INFO_BIND_ADDRESS = "0.0.0.0";
  
  /** Configuration key for DCS DNS interface */
  public static final String DCS_DNS_INTERFACE = "dcs.dns.interface";
  /** Default value for DCS DNS interface */
  public static final String DEFAULT_DCS_DNS_INTERFACE = "default";
  
  /** DCS cloud command */
  public static final String DCS_CLOUD_COMMAND = "dcs.cloud.command";
  /** Default value for DCS cloud command */
  public static final String DEFAULT_DCS_CLOUD_COMMAND = "nova list";

  private Constants() {
    // Can't be instantiated with this ctor.
  }
}
