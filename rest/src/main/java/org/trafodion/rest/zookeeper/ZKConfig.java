/**
 *(C) Copyright 2015 Hewlett-Packard Development Company, L.P.
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

package org.trafodion.rest.zookeeper;

import java.io.IOException;
import java.io.InputStream;
import java.net.InetAddress;
import java.net.UnknownHostException;
import java.util.ArrayList;
import java.util.List;
import java.util.Properties;
import java.util.Map.Entry;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.apache.hadoop.conf.Configuration;
import org.trafodion.rest.Constants;
import org.apache.hadoop.util.StringUtils;

/**
 * Utility methods for reading, parsing, and building zookeeper configuration.
 */
public class ZKConfig {
  private static final Log LOG = LogFactory.getLog(ZKConfig.class);

  private static final String VARIABLE_START = "${";
  private static final int VARIABLE_START_LENGTH = VARIABLE_START.length();
  private static final String VARIABLE_END = "}";
  private static final int VARIABLE_END_LENGTH = VARIABLE_END.length();

  /**
   * Make a Properties object holding ZooKeeper config equivalent to zoo.cfg.
   * If there is a zoo.cfg in the classpath, simply read it in. Otherwise parse
   * the corresponding config options from the Dcs XML configs and generate
   * the appropriate ZooKeeper properties.
   * @param conf Configuration to read from.
   * @return Properties holding mappings representing ZooKeeper zoo.cfg file.
   */
  public static Properties makeZKProps(Configuration conf) {
    // First check if there is a zoo.cfg in the CLASSPATH. If so, simply read
    // it and grab its configuration properties.
    ClassLoader cl = ZkQuorumPeer.class.getClassLoader();
    final InputStream inputStream =
      cl.getResourceAsStream(Constants.ZOOKEEPER_CONFIG_NAME);
    if (inputStream != null) {
      try {
        return parseZooCfg(conf, inputStream);
      } catch (IOException e) {
        LOG.warn("Cannot read " + Constants.ZOOKEEPER_CONFIG_NAME +
                 ", loading from XML files", e);
      }
    }

    // Otherwise, use the configuration options from Dcs's XML files.
    Properties zkProperties = new Properties();

    // Directly map all of the Dcs.zookeeper.property.KEY properties.
    for (Entry<String, String> entry : conf) {
      String key = entry.getKey();
      if (key.startsWith(Constants.ZK_CFG_PROPERTY_PREFIX)) {
        String zkKey = key.substring(Constants.ZK_CFG_PROPERTY_PREFIX_LEN);
        String value = entry.getValue();
        // If the value has variables substitutions, need to do a get.
        if (value.contains(VARIABLE_START)) {
          value = conf.get(key);
        }
        zkProperties.put(zkKey, value);
      }
    }

    // If clientPort is not set, assign the default.
    if (zkProperties.getProperty(Constants.CLIENT_PORT_STR) == null) {
      zkProperties.put(Constants.CLIENT_PORT_STR,
          Constants.DEFAULT_ZOOKEEPER_CLIENT_PORT);
    }

    // Create the server.X properties.
    int peerPort = conf.getInt("rest.zookeeper.peerport", 2888);
    int leaderPort = conf.getInt("rest.zookeeper.leaderport", 3888);

    final String[] serverHosts = conf.getStrings(Constants.ZOOKEEPER_QUORUM,
                                                 Constants.LOCALHOST);
    for (int i = 0; i < serverHosts.length; ++i) {
      String serverHost = serverHosts[i];
      String address = serverHost + ":" + peerPort + ":" + leaderPort;
      String key = "server." + i;
      zkProperties.put(key, address);
    }

    return zkProperties;
  }

  /**
   * Parse ZooKeeper's zoo.cfg, injecting Dcs Configuration variables in.
   * This method is used for testing so we can pass our own InputStream.
   * @param conf DcsConfiguration to use for injecting variables.
   * @param inputStream InputStream to read from.
   * @return Properties parsed from config stream with variables substituted.
   * @throws IOException if anything goes wrong parsing config
   */
  public static Properties parseZooCfg(Configuration conf,
      InputStream inputStream) throws IOException {
    Properties properties = new Properties();
    try {
      properties.load(inputStream);
    } catch (IOException e) {
      final String msg = "fail to read properties from "
        + Constants.ZOOKEEPER_CONFIG_NAME;
      LOG.fatal(msg);
      throw new IOException(msg, e);
    }
    for (Entry<Object, Object> entry : properties.entrySet()) {
      String value = entry.getValue().toString().trim();
      String key = entry.getKey().toString().trim();
      StringBuilder newValue = new StringBuilder();
      int varStart = value.indexOf(VARIABLE_START);
      int varEnd = 0;
      while (varStart != -1) {
        varEnd = value.indexOf(VARIABLE_END, varStart);
        if (varEnd == -1) {
          String msg = "variable at " + varStart + " has no end marker";
          LOG.fatal(msg);
          throw new IOException(msg);
        }
        String variable = value.substring(varStart + VARIABLE_START_LENGTH, varEnd);

        String substituteValue = System.getProperty(variable);
        if (substituteValue == null) {
          substituteValue = conf.get(variable);
        }
        if (substituteValue == null) {
          String msg = "variable " + variable + " not set in system property "
                     + "or Dcs configs";
          LOG.fatal(msg);
          throw new IOException(msg);
        }

        newValue.append(substituteValue);

        varEnd += VARIABLE_END_LENGTH;
        varStart = value.indexOf(VARIABLE_START, varEnd);
      }

      newValue.append(value.substring(varEnd));
      properties.setProperty(key, newValue.toString());
    }
    return properties;
  }

  /**
   * Return the ZK Quorum servers string given zk properties returned by
   * makeZKProps
   * @param properties
   * @return Quorum servers String
   */
  public static String getZKQuorumServersString(Properties properties) {
    String clientPort = null;
    List<String> servers = new ArrayList<String>();

    // The clientPort option may come after the server.X hosts, so we need to
    // grab everything and then create the final host:port comma separated list.
    boolean anyValid = false;
    for (Entry<Object,Object> property : properties.entrySet()) {
      String key = property.getKey().toString().trim();
      String value = property.getValue().toString().trim();
      if (key.equals("clientPort")) {
        clientPort = value;
      }
      else if (key.startsWith("server.")) {
        String host = value.substring(0, value.indexOf(':'));
        servers.add(host);
        try {
          //noinspection ResultOfMethodCallIgnored
          InetAddress.getByName(host);
          anyValid = true;
        } catch (UnknownHostException e) {
          LOG.warn(StringUtils.stringifyException(e));
        }
      }
    }

    if (!anyValid) {
      LOG.error("no valid quorum servers found in " + Constants.ZOOKEEPER_CONFIG_NAME);
      return null;
    }

    if (clientPort == null) {
      LOG.error("no clientPort found in " + Constants.ZOOKEEPER_CONFIG_NAME);
      return null;
    }

    if (servers.isEmpty()) {
      LOG.fatal("No server.X lines found in conf/zoo.cfg. Dcs must have a " +
                "ZooKeeper cluster configured for its operation.");
      return null;
    }

    StringBuilder hostPortBuilder = new StringBuilder();
    for (int i = 0; i < servers.size(); ++i) {
      String host = servers.get(i);
      if (i > 0) {
        hostPortBuilder.append(',');
      }
      hostPortBuilder.append(host);
      hostPortBuilder.append(':');
      hostPortBuilder.append(clientPort);
    }

    return hostPortBuilder.toString();
  }

  /**
   * Return the ZK Quorum servers string given the specified configuration.
   * @param conf
   * @return Quorum servers
   */
  public static String getZKQuorumServersString(Configuration conf) {
    return getZKQuorumServersString(makeZKProps(conf));
  }
}
