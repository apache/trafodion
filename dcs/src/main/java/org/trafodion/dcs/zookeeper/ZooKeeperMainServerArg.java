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

package org.trafodion.dcs.zookeeper;

import java.util.Properties;
import java.util.Iterator;
import java.util.Map.Entry;

import org.apache.commons.cli.CommandLine;
import org.apache.commons.cli.CommandLineParser;
import org.apache.commons.cli.Options;
import org.apache.commons.cli.ParseException;
import org.apache.commons.cli.PosixParser;
import org.apache.hadoop.conf.Configuration;
import org.trafodion.dcs.util.DcsConfiguration;

/**
 * Tool for reading a ZooKeeper server from HBase XML configuration producing
 * the '-server host:port' argument to pass ZooKeeperMain.  This program
 * emits either '-server HOST:PORT" where HOST is one of the zk ensemble
 * members plus zk client port OR it emits '' if no zk servers found (Yes,
 * it emits '-server' too).
 */
public class ZooKeeperMainServerArg {
  public String parse(final Configuration c) {
    // Note that we do not simply grab the property
    // Constants.ZOOKEEPER_QUORUM from the DcsConfiguration because the
    // user may be using a zoo.cfg file.
    Properties zkProps = ZKConfig.makeZKProps(c);
    String host = null;
    String clientPort = null;
    for (Entry<Object, Object> entry: zkProps.entrySet()) {
      String key = entry.getKey().toString().trim();
      String value = entry.getValue().toString().trim();
      if (key.startsWith("server.") && host == null) {
        String[] parts = value.split(":");
        host = parts[0];
      } else if (key.endsWith("clientPort")) {
        clientPort = value;
      }
      if (host != null && clientPort != null) break;
    }
    return host != null && clientPort != null? host + ":" + clientPort: null;
  }

  /**
   * Run the tool.
   * @param args Command line arguments. First arg is path to zookeepers file.
   */
  public static void main(String args[]) {
		Configuration conf = DcsConfiguration.create();
		if (args.length > 0) {
			String clientPort = conf.get("dcs.zookeeper.property.clientPort");
			String[] quorums = conf.get("dcs.zookeeper.quorum").split(",");

			Options options = new Options();
			options.addOption("lq", false, "list quorums");
			options.addOption("la", false, "list all config");
			options.addOption("key", true, "list key value");
			CommandLineParser parser = new PosixParser();
			CommandLine cmd;
			try {
				cmd = parser.parse(options, args);
				if (cmd.hasOption("lq")) {
					StringBuffer str_lq = new StringBuffer();
					for (int i = 0; i < quorums.length; i++) {
						str_lq.append(quorums[i] + ":" + clientPort);
						if (i < quorums.length - 1)
							str_lq.append(",");
					}
					System.out.println(str_lq);
				}
				if (cmd.hasOption("la")) {
					StringBuffer str_la = new StringBuffer();
					Iterator<Entry<String, String>> iter = conf.iterator();
					while (iter.hasNext()) {
						Entry<String, String> entity = iter.next();
						str_la.append(entity.getKey() + ":" + entity.getValue() + "\n");
					}
					System.out.println(str_la);
				}
				if (cmd.hasOption("key")) {
					String key = cmd.getOptionValue("key");
					if (key == null) {
					} else {
						System.out.println(conf.get(key));
					}
				}
			} catch (ParseException e) {
				e.printStackTrace();
			}
		} else {
			String hostport = new ZooKeeperMainServerArg().parse(conf);
			System.out.println((hostport == null || hostport.length() == 0) ? "" : "-server " + hostport);
		}
	}
}
