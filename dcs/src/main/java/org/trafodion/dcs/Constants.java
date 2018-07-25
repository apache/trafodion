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
*  http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing,
* software distributed under the License is distributed on an
* "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
* KIND, either express or implied.  See the License for the
* specific language governing permissions and limitations
* under the License.
*
* @@@ END COPYRIGHT @@@
 */
package org.trafodion.dcs;

import java.util.Arrays;
import java.util.Collections;
import java.util.List;
import java.util.UUID;
import java.util.regex.Pattern;

import org.apache.commons.lang.ArrayUtils;
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
    public static final int DEFAULT_DCS_MASTER_PORT = 23400;

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
    public static final String DEFAULT_DCS_SERVER_USER_PROGRAM_COMMAND = "cd ${dcs.user.program.home};. ./sqenv.sh;mxosrvr -ZKHOST -RZ -ZKPNODE -CNGTO -ZKSTO -EADSCO -TCPADD -MAXHEAPPCT -STATISTICSINTERVAL -STATISTICSLIMIT -STATISTICSTYPE -STATISTICSCACHESIZE -STATISTICSENABLE -SQLPLAN -PORTMAPTOSECS -PORTBINDTOSECS -TCPKEEPALIVESTATUS -TCPKEEPALIVEIDLETIME -TCPKEEPALIVEINTERVAL -TCPKEEPALIVERETRYCOUNT";

    /** Configuration key for DCS server user program connecting timeout */
    public static final String DCS_SERVER_USER_PROGRAM_CONNECTING_TIMEOUT = "dcs.server.user.program.connecting.timeout";

    /** Default value for DCS server user program connecting timeout */
    public static final int DEFAULT_DCS_SERVER_USER_PROGRAM_CONNECTING_TIMEOUT = 60;

    /** Configuration key for DCS server user program zookeeper session timeout */
    public static final String DCS_SERVER_USER_PROGRAM_ZOOKEEPER_SESSION_TIMEOUT = "dcs.server.user.program.zookeeper.session.timeout";

    /** Default value for DCS server user program zookeeper session timeout */
    public static final int DEFAULT_DCS_SERVER_USER_PROGRAM_ZOOKEEPER_SESSION_TIMEOUT = 180;

    /** Configuration key for DCS server user program exit after disconnect */
    public static final String DCS_SERVER_USER_PROGRAM_EXIT_AFTER_DISCONNECT = "dcs.server.user.program.exit.after.disconnect";

    /** Default value for DCS server user program exit after disconnect */
    public static final int DEFAULT_DCS_SERVER_USER_PROGRAM_EXIT_AFTER_DISCONNECT = 0;

    /** Configuration key for DCS server program mxosrvr keepalive STATUS*/
    public static final String  DEFAULT_DCS_SERVER_PROGRAM_TCP_KEEPALIVE_STATUS= "dcs.server.user.program.tcp.keepalive.status";

    /** Default value for DCS server program mxosrvr keepalive STATUS*/
    public static final String DCS_SERVER_PROGRAM_KEEPALIVE_STATUS = "true";

    /** Configuration key for DCS server program mxosrvr keepalive IDLETIME*/
    public static final String DEFAULT_DCS_SERVER_PROGRAM_TCP_KEEPALIVE_IDLETIME = "dcs.server.user.program.tcp.keepalive.idletime";

    /** Default value for DCS server program mxosrvr keepalive IDLETIME*/
    public static final int DCS_SERVER_PROGRAM_KEEPALIVE_IDLETIME = 300;

    /** Configuration key for DCS server program mxosrvr keepalive INTERTIME */
    public static final String DEFAULT_DCS_SERVER_PROGRAM_TCP_KEEPALIVE_INTERVALTIME = "dcs.server.user.program.tcp.keepalive.intervaltime";

    /** Default value for DCS server program mxosrvr keepalive INTERTIME */
    public static final int DCS_SERVER_PROGRAM_KEEPALIVE_INTERVALTIME = 5;

    /** Configuration key for DCS server program mxosrvr keepalive RETRYCNT*/
    public static final String DEFAULT_DCS_SERVER_PROGRAM_TCP_KEEPALIVE_RETRYCOUNT = "dcs.server.user.program.tcp.keepalive.retrycount";

    /** Default value for DCS server program mxosrvr keepalive RETRYCNT*/
    public static final int DCS_SERVER_PROGRAM_KEEPALIVE_RETRYCOUNT = 3;
    /**
     * Configuration key for DCS server user program exit when heap size becomes
     * too large
     */
    public static final String DCS_SERVER_USER_PROGRAM_MAX_HEAP_PCT_EXIT = "dcs.server.user.program.max.heap.pct.exit";

    /**
     * Default value for DCS server user program exit when heap size becomes too
     * large
     */
    public static final int DEFAULT_DCS_SERVER_USER_PROGRAM_MAX_HEAP_PCT_EXIT = 0;
/** Configuration key for DCS server user program statistics cache size */
    public static final String DCS_SERVER_USER_PROGRAM_STATISTICS_CACHE_SIZE = "dcs.server.user.program.statistics.cache.size";

    /** Default value for DCS server user program statistics cache size */
    public static final int DEFAULT_DCS_SERVER_USER_PROGRAM_STATISTICS_CACHE_SIZE = 1;

    /** Configuration key for DCS server user program statistics interval time */
    public static final String DCS_SERVER_USER_PROGRAM_STATISTICS_INTERVAL_TIME = "dcs.server.user.program.statistics.interval.time";

    /** T2 Driver Property key for DCS server user program statistics interval time */
    public static final String PROPERTY_STATISTICS_INTERVAL_TIME = "statisticsIntervalTime";

    /** Default value for DCS server user program statistics interval time */
    public static final int DEFAULT_DCS_SERVER_USER_PROGRAM_STATISTICS_INTERVAL_TIME = 60;

    /** Configuration key for DCS server user program statistics limit time */
    public static final String DCS_SERVER_USER_PROGRAM_STATISTICS_LIMIT_TIME = "dcs.server.user.program.statistics.limit.time";

    /** T2 Driver Property key for DCS server user program statistics limit time */
    public static final String PROPERTY_STATISTICS_LIMIT_TIME = "statisticsLimitTime";

    /** Default value for DCS server user program statistics limit time */
    public static final int DEFAULT_DCS_SERVER_USER_PROGRAM_STATISTICS_LIMIT_TIME = 60;

    /** Configuration key for DCS server user program statistics type */
    public static final String DCS_SERVER_USER_PROGRAM_STATISTICS_TYPE = "dcs.server.user.program.statistics.type";

    /** T2 Driver Property key for DCS server user program statistics type */
    public static final String PROPERTY_STATISTICS_TYPE = "statisticsType";

    /** Default value for DCS server user program statistics type */
    public static final String DEFAULT_DCS_SERVER_USER_PROGRAM_STATISTICS_TYPE = "aggregated";

    /** Configuration key for DCS server user program statistics enable */
    public static final String DCS_SERVER_USER_PROGRAM_STATISTICS_ENABLE = "dcs.server.user.program.statistics.enabled";

    /** T2 Driver Property key for DCS server user program statistics enable */
    public static final String PROPERTY_PROGRAM_STATISTICS_ENABLE = "programStatisticsEnabled";

    /** Default value for DCS server user program statistics enable */
    public static final String DEFAULT_DCS_SERVER_USER_PROGRAM_STATISTICS_ENABLE = "true";

    /**
     * Configuration key for DCS server user program to enable sqlplan for
     * queries
     */
    public static final String DCS_SERVER_USER_PROGRAM_STATISTICS_SQLPLAN_ENABLE = "dcs.server.user.program.statistics.sqlplan.enabled";

    /**
     * T2 Driver Property key for DCS server user program to enable sqlplan for
     * queries
     */
    public static final String PROPERTY_STATISTICS_SQLPLAN_ENABLE = "statisticsSqlPlanEnabled";

    /**
     * Default value for DCS server user program to enable sql plans for queries
     */
    public static final String DEFAULT_DCS_SERVER_USER_PROGRAM_STATISTICS_SQLPLAN_ENABLE = "true";

    /** Configuration key for DCS server user program port map timeout seconds */
    public static final String DCS_SERVER_USER_PROGRAM_PORT_MAP_TIMEOUT_SECONDS = "dcs.server.user.program.port.map.timeout.seconds";

    /** Default value for DCS server user program port map timeout seconds */
    public static final int DEFAULT_DCS_SERVER_USER_PROGRAM_PORT_MAP_TIMEOUT_SECONDS = -1;

    /** Configuration key for DCS server user program port bind timeout seconds */
    public static final String DCS_SERVER_USER_PROGRAM_PORT_BIND_TIMEOUT_SECONDS = "dcs.server.user.program.port.bind.timeout.seconds";

    /** Default value for DCS server tcp bind max retries */
    public static final int DEFAULT_DCS_SERVER_USER_PROGRAM_PORT_BIND_TIMEOUT_SECONDS = -1;

    /** Configuration key for user program restart handler attempts */
    public static final String DCS_SERVER_USER_PROGRAM_RESTART_HANDLER_ATTEMPTS = "dcs.server.user.program.restart.handler.attempts";

    /** Default value for user program restart handler attempts */
    public static final int DEFAULT_DCS_SERVER_USER_PROGRAM_RESTART_HANDLER_ATTEMPTS = 6;

    /** Configuration key for user program restart handler retry interval millis */
    public static final String DCS_SERVER_USER_PROGRAM_RESTART_HANDLER_RETRY_INTERVAL_MILLIS = "dcs.server.user.program.restart.handler.retry.interval.millis";

    /** Default value for user program restart handler retry interval millis */
    public static final int DEFAULT_DCS_SERVER_USER_PROGRAM_RESTART_HANDLER_RETRY_INTERVAL_MILLIS = 5000;

    /** Configuration key for user program restart handler attempts for mxosrvr*/
    public static final String DCS_SERVER_STARTUP_MXOSRVR_USER_PROGRAM_RESTART_HANDLER_ATTEMPTS = "dcs.server.startup.mxosrvr.user.program.restart.handler.attempts";

    /** Default value for user program restart handler attempts for mxosrvr */
    public static final int DEFAULT_DCS_SERVER_STARTUP_MXOSRVR_USER_PROGRAM_RESTART_HANDLER_ATTEMPTS = 6;

    /** Configuration key for user program restart handler retry timeout minutes */
    public static final String DCS_SERVER_STARTUP_MXOSRVR_USER_PROGRAM_RESTART_HANDLER_RETRY_TIMEOUT_MINUTES = "dcs.server.startup.mxosrvr.user.program.restart.handler.retry.timeout.minutes";

    /** Default value for user program restart handler retry timeout minutes for mxosrvr */
    public static final int DEFAULT_DCS_SERVER_STARTUP_MXOSRVR_USER_PROGRAM_RESTART_HANDLER_RETRY_TIMEOUT_MINUTES = 6;

    /** Name of6ZooKeeper quorum configuration parameter. */
    public static final String ZOOKEEPER_QUORUM = "dcs.zookeeper.quorum";

    /** Name of ZooKeeper config file in conf/ directory. */
    public static final String ZOOKEEPER_CONFIG_NAME = "zoo.cfg";

    /** Common prefix of ZooKeeper configuration properties */
    public static final String ZK_CFG_PROPERTY_PREFIX = "dcs.zookeeper.property.";

    public static final int ZK_CFG_PROPERTY_PREFIX_LEN = ZK_CFG_PROPERTY_PREFIX
            .length();

    /**
     * The ZK client port key in the ZK properties map. The name reflects the
     * fact that this is not an DCS configuration key.
     */
    public static final String CLIENT_PORT_STR = "clientPort";

    /** Parameter name for the client port that the zookeeper listens on */
    public static final String ZOOKEEPER_CLIENT_PORT = ZK_CFG_PROPERTY_PREFIX
            + CLIENT_PORT_STR;

    /** Default client port that the zookeeper listens on */
    public static final int DEFAULT_ZOOKEEPER_CLIENT_PORT = 2181;

    /** Parameter name for the wait time for the recoverable zookeeper */
    public static final String ZOOKEEPER_RECOVERABLE_WAITTIME = "dcs.zookeeper.recoverable.waittime";

    /** Default wait time for the recoverable zookeeper */
    public static final long DEFAULT_ZOOKEPER_RECOVERABLE_WAITIME = 10000;

    /** Parameter name for the root dir in ZK for this cluster */
    public static final String ZOOKEEPER_ZNODE_PARENT = "zookeeper.znode.parent";
    public static final String DEFAULT_ZOOKEEPER_ZNODE_PARENT = "/dcs";
    public static final String DEFAULT_ZOOKEEPER_ZNODE_MASTER = DEFAULT_ZOOKEEPER_ZNODE_PARENT
            + "/master";
    public static final String DEFAULT_ZOOKEEPER_ZNODE_MASTER_LEADER = DEFAULT_ZOOKEEPER_ZNODE_PARENT
            + "/leader";
    public static final String DEFAULT_ZOOKEEPER_ZNODE_SERVERS = DEFAULT_ZOOKEEPER_ZNODE_PARENT
            + "/servers";
    public static final String DEFAULT_ZOOKEEPER_ZNODE_SERVERS_RUNNING = DEFAULT_ZOOKEEPER_ZNODE_SERVERS
            + "/running";
    public static final String DEFAULT_ZOOKEEPER_ZNODE_SERVERS_REGISTERED = DEFAULT_ZOOKEEPER_ZNODE_SERVERS
            + "/registered";

    /**
     * Parameter name for the limit on concurrent client-side zookeeper
     * connections
     */
    public static final String ZOOKEEPER_MAX_CLIENT_CNXNS = ZK_CFG_PROPERTY_PREFIX
            + "maxClientCnxns";

    /** Parameter name for the ZK data directory */
    public static final String ZOOKEEPER_DATA_DIR = ZK_CFG_PROPERTY_PREFIX
            + "dataDir";

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
    // public static final int FOREVER = -1;
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
    public static final int DEFAULT_LISTENER_REQUEST_TIMEOUT = 30 * 1000; // 30
                                                                          // seconds

    /** Configuration key for Listener selector timeout */
    public static final String DCS_MASTER_LISTENER_SELECTOR_TIMEOUT = "dcs.master.listener.selector.timeout";
    /** Listener default selector timeout */
    public static final int DEFAULT_LISTENER_SELECTOR_TIMEOUT = 10 * 1000; // 10
                                                                           // seconds

    /** Configuration key for DCS master info port */
    public static final String DCS_MASTER_INFO_PORT = "dcs.master.info.port";
    /** Default value for DCS master info port */
    public static final int DEFAULT_DCS_MASTER_INFO_PORT = 24400;

    /** Configuration key for server Listener selector timeout */
    public static final String DCS_SERVER_LISTENER_SELECTOR_TIMEOUT = "dcs.server.listener.selector.timeout";
    /** Listener default server selector timeout */
    public static final int DEFAULT_SERVER_LISTENER_SELECTOR_TIMEOUT = 10 * 1000; // 10
                                                                                  // seconds

    /** Configuration key for server Listener request timeout */
    public static final String DCS_SERVER_LISTENER_REQUEST_TIMEOUT = "dcs.server.listener.request.timeout";
    /** Listener default server request timeout */
    public static final int DEFAULT_SERVER_LISTENER_REQUEST_TIMEOUT = 30 * 1000; // 30
                                                                                 // seconds

    /** Configuration key for server Listener CONNECTING timeout */
    public static final String DCS_SERVER_LISTENER_CONNECTING_TIMEOUT = "dcs.server.listener.request.timeout";
    /** Listener default server CONNECTING timeout */
    public static final int DEFAULT_SERVER_LISTENER_CONNECTING_TIMEOUT = 30 * 1000; // 30
                                                                                    // seconds

    /**
     * Configuration key for server Listener number attempts BINDING assigned
     * port
     */
    public static final String DCS_SERVER_CHECK_TCPIPPORT_ATTEMPTS = "dcs.server.check.tcpipport.attempts";
    /** server Listener default number attempts BINDING assigned port */
    public static final int DEFAULT_DCS_SERVER_CHECK_TCPIPPORT_ATTEMPTS = 6;
    /** Configuration key for server Listener interval between BINDING attempts */
    public static final String DCS_SERVER_CHECK_TCPIPPORT_RETRY_INTERVAL_MILLIS = "dcs.server.check.tcpipport.retry.interval.millis";
    /** server Listener default interval between BINDING attempts */
    public static final int DEFAULT_DCS_SERVER_CHECK_TCPIPPORT_RETRY_INTERVAL_MILLIS = 1000;

    /** Configuration key for maximum number of threads per DCS Server handler */
    public static final String DCS_SERVER_HANDLER_THREADS_MAX = "dcs.server.handler.threads.max";
    /** Default maximum number of threads per DCS Server handler */
    public static final int DEFAULT_DCS_SERVER_HANDLER_THREADS_MAX = 10;

    /** Configuration key for DCS server info port */
    public static final String DCS_SERVER_INFO_PORT = "dcs.server.info.port";
    /** Default value for DCS server info port */
    public static final int DEFAULT_DCS_SERVER_INFO_PORT = 40030;

    /** A flag that enables automatic selection of DCS server info port */
    public static final String DCS_SERVER_INFO_PORT_AUTO = "dcs.server.info.port.auto";

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

    public static final String DEFAULT_DCS_CLOUD_COMMAND = "nova list | grep -v '^+' | grep -w `hostname` | sed 's/.*=\\([0-9.]*\\), \\([0-9.]*\\).*$/\\1,\\2/'";

    /** User program feature is enabled */
    public static final String DCS_TRAFODION_HOME = "dcs.trafodion.home";

    /** The sys_shell script name */
    public static final String SYS_SHELL_SCRIPT_NAME = "sys_shell.py";

    /** DcsMaster Trafodion log feature */
    public static final String DCS_MASTER_TRAFODION_LOGS = "dcs.master.trafodion.logs";

    /** DcsMaster Trafodion logs is not enabled */
    public static final boolean DCS_MASTER_TRAFODION_LOGS_IS_NOT_ENABLED = false;

    /** DcsMaster Trafodion logs is enabled */
    public static final boolean DCS_MASTER_TRAFODION_LOGS_IS_ENABLED = true;

    /** Default value for DcsMaster Trafodion logs feature */
    public static final boolean DEFAULT_DCS_MASTER_TRAFODION_LOGS = DCS_MASTER_TRAFODION_LOGS_IS_NOT_ENABLED;

    /** DcsMaster Trafodion query tools feature */
    public static final String DCS_MASTER_TRAFODION_QUERY_TOOLS = "dcs.master.trafodion.query.tools";

    /** DcsMaster Trafodion query tools is not enabled */
    public static final boolean DCS_MASTER_TRAFODION_QUERY_TOOLS_IS_NOT_ENABLED = false;

    /** DcsMaster Trafodion query tools is enabled */
    public static final boolean DCS_MASTER_TRAFODION_QUERY_TOOLS_IS_ENABLED = true;

    /** Default value for DcsMaster Trafodion query tools feature */
    public static final boolean DEFAULT_DCS_MASTER_TRAFODION_QUERY_TOOLS = DCS_MASTER_TRAFODION_QUERY_TOOLS_IS_NOT_ENABLED;

    /** Names of the trafodion repository tables */
    public static final String TRAFODION_REPOS_CATALOG = "TRAFODION";
    public static final String TRAFODION_REPOS_SCHEMA = "_REPOS_";
    public static final String TRAFODION_REPOS_CATALOG_SCHEMA = TRAFODION_REPOS_CATALOG
            + "." + TRAFODION_REPOS_SCHEMA;
    public static final String TRAFODION_REPOS_METRIC_SESSION_TABLE = TRAFODION_REPOS_CATALOG
            + "." + TRAFODION_REPOS_SCHEMA + "." + "METRIC_SESSION_TABLE";
    public static final String TRAFODION_REPOS_METRIC_QUERY_TABLE = TRAFODION_REPOS_CATALOG
            + "." + TRAFODION_REPOS_SCHEMA + "." + "METRIC_QUERY_TABLE";
    public static final String TRAFODION_REPOS_METRIC_QUERY_AGGR_TABLE = TRAFODION_REPOS_CATALOG
            + "." + TRAFODION_REPOS_SCHEMA + "." + "METRIC_QUERY_AGGR_TABLE";

    /** T2 Driver name */
    public static final String T2_DRIVER_CLASS_NAME = "org.apache.trafodion.jdbc.t2.T2Driver";
    /** T2 Driver URL */
    public static final String T2_DRIVER_URL = "jdbc:t2jdbc:";
    /** T2 Driver trace file */
    public static final String T2_DRIVER_TRACE_FILE = "t2.driver.trace.file";
    /** Default value for T2 Driver trace file */
    public static final String DEFAULT_T2_DRIVER_TRACE_FILE = "${dcs.log.dir}/${dcs.log.file}";
    /** T2 Driver trace flag */
    public static final String T2_DRIVER_TRACE_FLAG = "t2.driver.trace.flag";
    /**
     * Default value for T2 Driver trace flag, 0 = No tracing. 1 = Traces
     * connection and statement pooling calls only. 2 = Traces the LOB-code path
     * only. 3 = Traces the entry point of all JDBC methods.
     */
    public static final String DEFAULT_T2_DRIVER_TRACE_FLAG = "0";
    /** CQD to turn table stats off */
    public static final String CQD_ESTIMATE_HBASE_ROW_COUNT_OFF = "CONTROL QUERY DEFAULT estimate_hbase_row_count 'OFF'";

    /** T4 Driver name */
    public static final String T4_DRIVER_CLASS_NAME = "org.trafodion.jdbc.t4.T4Driver";

    /** T4 Driver URL */
    public static final String T4_DRIVER_URL = "jdbc:t4jdbc:";

    /** DcsMaster base64 encoded username:password used in JdbcT4Util */
    public static final String T4_DRIVER_USERNAME_PASSWORD = "org.trafodion.jdbc.t4.T4Driver.username.password";

    /** DcsMaster default base64 encoded username:password used in JdbcT4Util */
    public static final String DEFAULT_T4_DRIVER_USERNAME_PASSWORD = "dHJhZm9kaW9uOnRyYWYxMjMK";

    /** DcsMaster minPoolSize used in JdbcT4Util */
    public static final String T4_DRIVER_MIN_POOL_SIZE = "t4.driver.min.pool.size";

    /** DcsMaster minPoolSize used in JdbcT4Util */
    public static final int DEFAULT_T4_DRIVER_MIN_POOL_SIZE = 1;

    /** DcsMaster minPoolSize used in JdbcT4Util */
    public static final String T4_DRIVER_MAX_POOL_SIZE = "t4.driver.max.pool.size";

    /** DcsMaster minPoolSize used in JdbcT4Util */
    public static final int DEFAULT_T4_DRIVER_MAX_POOL_SIZE = 0;

    /** Query for trafodion._REPOS_.metric_session_table */
    public static final String TRAFODION_REPOS_METRIC_SESSION_TABLE_QUERY = "trafodion.repos.metric.session.table.query";
    /** Default query for trafodion._REPOS_.metric_session_table */
    public static final String DEFAULT_TRAFODION_REPOS_METRIC_SESSION_TABLE_QUERY = "SELECT [first 500] "
            + "session_id,"
            + "user_name,"
            + "total_execution_time,"
            + "total_elapsed_time,"
            + "total_prepares,"
            + "total_executes,"
            + "total_fetches " + "FROM \"_REPOS_\".metric_session_table";

    /** Query for trafodion._REPOS_.metric_query_table */
    public static final String TRAFODION_REPOS_METRIC_QUERY_TABLE_QUERY = "trafodion.repos.metric_query.table.query";
    /** Default query for trafodion._REPOS_.metric_query_table */
    public static final String DEFAULT_TRAFODION_REPOS_METRIC_QUERY_TABLE_QUERY = "SELECT [first 500] "
            + "query_id,"
            + "user_name,"
            + "client_name,"
            + "application_name,"
            + "submit_utc_ts,"
            + "query_elapsed_time,"
            + "sql_process_busy_time,"
            + "total_mem_alloc,"
            + "max_mem_used,"
            + "query_text " + "FROM \"_REPOS_\".metric_query_table";

    /** Query for trafodion._REPOS_.metric_query_aggr_table */
    public static final String TRAFODION_REPOS_METRIC_QUERY_AGGR_TABLE_QUERY = "trafodion.repos.metric_query_aggr.table.query";
    /** Default query for trafodion._REPOS_.metric_query_aggr_table */
    public static final String DEFAULT_TRAFODION_REPOS_METRIC_QUERY_AGGR_TABLE_QUERY = "SELECT [first 500] "
            + "session_id,"
            + "user_name,"
            + "role_name,"
            + "client_name,"
            + "application_name,"
            + "total_selects,"
            + "total_inserts,"
            + "total_updates,"
            + "total_deletes,"
            + "delta_total_deletes,"
            + "delta_total_inserts,"
            + "delta_total_updates,"
            + "delta_total_deletes "
            + "FROM \"_REPOS_\".metric_query_aggr_table";

    /** DcsMaster authorization feature */
    public static final String DCS_MASTER_AUTHORIZATION = "dcs.master.authorization";

    /** DcsMaster authorization is not enabled */
    public static final boolean DCS_MASTER_AUTHORIZATION_IS_NOT_ENABLED = false;

    /** DcsMaster authorization is enabled */
    public static final boolean DCS_MASTER_AUTHORIZATION_IS_ENABLED = true;

    /** Default value for DcsMaster authorization feature */
    public static final boolean DEFAULT_DCS_MASTER_AUTHORIZATION = DCS_MASTER_AUTHORIZATION_IS_NOT_ENABLED;

    /** DcsMaster Floating IP feature */
    public static final String DCS_MASTER_FLOATING_IP = "dcs.master.floating.ip";

    /** DcsMaster Floating IP is not enabled */
    public static final boolean DCS_MASTER_FLOATING_IP_IS_NOT_ENABLED = false;

    /** DcsMaster Floating IP is enabled */
    public static final boolean DCS_MASTER_FLOATING_IP_IS_ENABLED = true;

    /** Default value for DcsMaster Floating IP feature */
    public static final boolean DEFAULT_DCS_MASTER_FLOATING_IP = DCS_MASTER_FLOATING_IP_IS_NOT_ENABLED;

    /** DcsMaster floating IP command */
    public static final String DCS_MASTER_FLOATING_IP_COMMAND = "dcs.master.floating.ip.command";

    /** Default value for DcsMaster floating IP command */
    public static final String DEFAULT_DCS_MASTER_FLOATING_IP_COMMAND = "cd ${dcs.home.dir};bin/scripts/dcsbind.sh -i -a -p";

    /** DcsMaster Floating IP external interface */
    public static final String DCS_MASTER_FLOATING_IP_EXTERNAL_INTERFACE = "dcs.master.floating.ip.external.interface";

    /** Default DcsMaster Floating IP external interface */
    public static final String DEFAULT_DCS_MASTER_FLOATING_IP_EXTERNAL_INTERFACE = "default";

    /** DcsMaster Floating IP external IP address */
    public static final String DCS_MASTER_FLOATING_IP_EXTERNAL_IP_ADDRESS = "dcs.master.floating.ip.external.ip.address";

    /** Default DcsMaster Floating IP external IP address */
    public static final String DEFAULT_DCS_MASTER_FLOATING_IP_EXTERNAL_IP_ADDRESS = "default";

    private Constants() {
        // Can't be instantiated with this ctor.
    }
}
