/**
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

package org.trafodion.dcs.zookeeper;

import java.util.Properties;
import java.util.Map.Entry;

import org.apache.hadoop.conf.Configuration;
import org.trafodion.dcs.util.DcsConfiguration;

/**
 * Tool for running ZookeeperMain from DCS by reading the Zookeeper quorum
 * from the xml configuration file.
 */
public class ZooKeeperMainServerArg {
  public String parse(final Configuration c) {
    return ZKConfig.getZKQuorumServersString(c);
  }

  /**
   * Run the tool.
   * @param args Command line arguments. First arg is path to zookeepers file.
   */
  public static void main(String args[]) {
    Configuration conf = DcsConfiguration.create();
    String hostport = new ZooKeeperMainServerArg().parse(conf);
    System.out.println((hostport == null || hostport.length() == 0)? "":
      "-server " + hostport);
  }
}
