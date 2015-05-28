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

import java.util.Properties;
import java.util.Map.Entry;

import org.apache.hadoop.conf.Configuration;
import org.trafodion.rest.util.RestConfiguration;

/**
 * Tool for reading ZooKeeper servers from dcs XML configuration and producing
 * a line-by-line list for use by bash scripts.
 */
public class ZKServerTool {
  /**
   * Run the tool.
   * @param args Command line arguments.
   */
  public static void main(String args[]) {
    Configuration conf = RestConfiguration.create();
    // Note that we do not simply grab the property
    // Constants.ZOOKEEPER_QUORUM from the RestConfiguration because the
    // user may be using a zoo.cfg file.
    Properties zkProps = ZKConfig.makeZKProps(conf);
    for (Entry<Object, Object> entry : zkProps.entrySet()) {
      String key = entry.getKey().toString().trim();
      String value = entry.getValue().toString().trim();
      if (key.startsWith("server.")) {
        String[] parts = value.split(":");
        String host = parts[0];
        System.out.println("ZK host:" + host);
      }
    }
  }
}