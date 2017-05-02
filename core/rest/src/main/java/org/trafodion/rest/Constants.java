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
package org.trafodion.rest;


/**
 * Constants holds a bunch of rest-related constants
 */
public final class Constants {

  /** long constant for zero */
  public static final Long ZERO_L = Long.valueOf(0L);
  public static final String NINES = "99999999999999";
  public static final String ZEROES = "00000000000000";
 
  /** name of version file */
  public static final String VERSION_FILE_NAME = "rest.version";
  
  /** long constant for thirty */
  public static final Long THIRTY_SECONDS = Long.valueOf(30000L);

  // Configuration parameters

  /** default host address */
  public static final String DEFAULT_HOST = "0.0.0.0";
  
  /** Configuration key for DCS master port */
  public static final String DCS_MASTER_PORT = "dcs.master.port";

  /** Default value for DCS master port */
  public static final int DEFAULT_DCS_MASTER_PORT = 23400;
  
  /** Name of ZooKeeper quorum configuration parameter. */
  public static final String ZOOKEEPER_QUORUM = "rest.zookeeper.quorum";

  /** Name of ZooKeeper config file in conf/ directory. */
  public static final String ZOOKEEPER_CONFIG_NAME = "zoo.cfg";

  /** Common prefix of ZooKeeper configuration properties */
  public static final String ZK_CFG_PROPERTY_PREFIX =
      "rest.zookeeper.property.";

  public static final int ZK_CFG_PROPERTY_PREFIX_LEN =
      ZK_CFG_PROPERTY_PREFIX.length();

  /**
   * The ZK client port key in the ZK properties map. The name reflects the
   * fact that this is not an REST configuration key.
   */
  public static final String CLIENT_PORT_STR = "clientPort";

  /** Parameter name for the client port that the zookeeper listens on */
  public static final String ZOOKEEPER_CLIENT_PORT =
      ZK_CFG_PROPERTY_PREFIX + CLIENT_PORT_STR;

  /** Default client port that the zookeeper listens on */
  public static final int DEFAULT_ZOOKEEPER_CLIENT_PORT = 2181;

  /** Parameter name for the wait time for the recoverable zookeeper */
  public static final String ZOOKEEPER_RECOVERABLE_WAITTIME = "rest.zookeeper.recoverable.waittime";

  /** Default wait time for the recoverable zookeeper */
  public static final long DEFAULT_ZOOKEPER_RECOVERABLE_WAITIME = 10000;

  /** Parameter name for the dcs root dir in ZK for this cluster */
  public static final String ZOOKEEPER_ZNODE_PARENT = "zookeeper.znode.parent";
  public static final String DEFAULT_ZOOKEEPER_ZNODE_PARENT = "/dcs";
  public static final String DEFAULT_ZOOKEEPER_ZNODE_MASTER = DEFAULT_ZOOKEEPER_ZNODE_PARENT + "/master";
  public static final String DEFAULT_ZOOKEEPER_ZNODE_MASTER_LEADER = DEFAULT_ZOOKEEPER_ZNODE_PARENT + "/leader";
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
  public static final String ZOOKEEPER_USEMULTI = "rest.zookeeper.useMulti";

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
  
  /** Configuration key for REST DNS interface */
  public static final String REST_DNS_INTERFACE = "rest.dns.interface";
  /** Default value for REST DNS interface */
  public static final String DEFAULT_REST_DNS_INTERFACE = "default";
  
  /** REST cloud command */
  public static final String REST_CLOUD_COMMAND = "rest.cloud.command";
  
  /** Default value for REST cloud command */
  public static final String DEFAULT_REST_CLOUD_COMMAND = "nova list | grep -v '^+' | grep -w `hostname` | sed 's/.*=\\([0-9.]*\\), \\([0-9.]*\\).*$/\\1,\\2/'";
  
  /** User program feature is enabled */
  public static final String REST_TRAFODION_HOME = "rest.trafodion.home";
  
  /** The sys_shell script name */
  public static final String SYS_SHELL_SCRIPT_NAME = "sys_shell.py";
  
  /** Rest Trafodion query tools is not enabled */
  public static final boolean REST_MASTER_TRAFODION_QUERY_TOOLS_IS_NOT_ENABLED = false;

  /** T2 Driver name */
  public static final String T2_DRIVER_CLASS_NAME = "org.apache.trafodion.jdbc.t2.T2Driver";
  /** T2 Driver URL */
  public static final String T2_DRIVER_URL = "jdbc:t2jdbc:"; 
  /** CQD to turn table stats off */
  public static final String CQD_ESTIMATE_HBASE_ROW_COUNT_OFF = "CONTROL QUERY DEFAULT estimate_hbase_row_count 'OFF'";

  /** T4 Driver name */
  public static final String T4_DRIVER_CLASS_NAME = "org.trafodion.jdbc.t4.T4Driver";
 
  /** T4 Driver URL */
  public static final String T4_DRIVER_URL = "jdbc:t4jdbc:"; 
  
  /** Rest base64 encoded username:password used in JdbcT4Util */
  public static final String T4_DRIVER_USERNAME_PASSWORD = "org.trafodion.jdbc.t4.T4Driver.username.password";

  /** Rest default base64 encoded username:password used in JdbcT4Util */
  public static final String DEFAULT_T4_DRIVER_USERNAME_PASSWORD = "dHJhZm9kaW9uOnRyYWYxMjMK";
  
  /** Rest minPoolSize used in JdbcT4Util */
  public static final String T4_DRIVER_MIN_POOL_SIZE = "t4.driver.min.pool.size";
  
  /** Rest minPoolSize used in JdbcT4Util */
  public static final int DEFAULT_T4_DRIVER_MIN_POOL_SIZE = 1;
  
  /** Rest minPoolSize used in JdbcT4Util */
  public static final String T4_DRIVER_MAX_POOL_SIZE = "t4.driver.max.pool.size";
  
  /** Rest minPoolSize used in JdbcT4Util */
  public static final int DEFAULT_T4_DRIVER_MAX_POOL_SIZE = 0;
  
  /** Rest authorization feature */
  public static final String REST_MASTER_AUTHORIZATION = "rest.master.authorization";

  /** Rest authorization is not enabled */
  public static final boolean REST_MASTER_AUTHORIZATION_IS_NOT_ENABLED = false;

  /** Rest authorization is enabled */
  public static final boolean REST_MASTER_AUTHORIZATION_IS_ENABLED = true;
  
  /** Default value for Rest authorization feature */
  public static final boolean DEFAULT_REST_MASTER_AUTHORIZATION = REST_MASTER_AUTHORIZATION_IS_NOT_ENABLED;

   /** Rest ssl password */
  public static final String REST_SSL_PASSWORD = "rest.ssl.password";
  
  /** Rest ssl keystore*/
  public static final String REST_KEYSTORE = "rest.keystore";
  
  /** Default ssl keystore */
  public static final String DEFAULT_REST_KEYSTORE = "${rest.conf.dir}/rest-keystore";
    
  /** Rest keystore command */
  public static final String REST_KEYSTORE_COMMAND = "rest.keystore.command";
  
  /** Rest Keystore creation command */
  public static final String DEFAULT_REST_KEYSTORE_COMMAND =  "rm rest.keystore;cd ${rest.conf.dir};keytool -genkey -alias rest -keyalg RSA -dname \"cn=localhost,ou=rest,o=HP,l=Palo Alto,st=CA,c=US\" -keypass rest.ssl.password -storepass rest.ssl.password -keystore rest-keystore -validity 36500";
  
  private Constants() {
    // Can't be instantiated with this ctor.
  }
}
