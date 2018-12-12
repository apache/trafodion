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
import java.util.Map.Entry;
import java.util.List;
import org.apache.hadoop.conf.Configuration;
import org.trafodion.dcs.util.DcsConfiguration;
import java.sql.Timestamp;
/**
 * Tool for reading ZooKeeper servers from dcs XML configuration and producing
 * a line-by-line list for use by bash scripts.
 */
public class ZKShellTool {
  /**
   * Run the tool.
   * @param args Command line arguments.
   */
  public static void main(String args[]) throws Exception{
      String username = System.getProperty("user.name");
      System.out.println("User: " + username);
      ZkClient zkc = new ZkClient();
      zkc.connect();

      String registeredPath = new String("/"+username + "/dcs/servers/registered");
      List<String> zks = zkc.getChildren(registeredPath, null);

      System.out.println("State\t\tNode\tProcessId\tProcessName\tIpAddress\tPort\tClientName\tClientApp\tClientIp\tClientPort");
      for (String nodeName: zks){
          String nodePath = registeredPath + "/" + nodeName;
          String currentData = new String(zkc.getData(nodePath, false, null));
          String[] node_status = currentData.split(":");
          for(int i = 0; i < node_status.length; i++){
              if (i != 1 && i != 2)
                  System.out.print(node_status[i]+"\t");
          }
          System.out.println();
      }
  }
}
