/**
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
package org.trafodion.sql;

import org.apache.log4j.Logger;
import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.hbase.HBaseConfiguration;
import org.apache.hadoop.fs.Path;
import java.util.Iterator;
import java.util.Map;

/**
 * Adds Trafodion configuration files to a Configuration
 */
public class TrafConfiguration extends HBaseConfiguration {

  static Logger logger = Logger.getLogger(TrafConfiguration.class.getName());

  public static Configuration addTrafResources(Configuration conf) {
    Configuration lv_conf = new Configuration();
    String trafSiteXml = new String(System.getenv("TRAF_HOME") + "/etc/trafodion-site.xml");
    Path fileRes = new Path(trafSiteXml);
    lv_conf.addResource(fileRes);
    Iterator<Map.Entry<String,String>> iter = lv_conf.iterator();
    String key;
    while (iter.hasNext()) {
       Map.Entry<String,String> entry = iter.next();
       key = entry.getKey();
       if (key.startsWith("trafodion."))
          key = key.substring(10); // 10 - length of trafodion.
       conf.set(key, entry.getValue());
    }
    return conf;
  }

  /**
   * Creates a Configuration with Trafodion resources
   * @return a Configuration with Trafodion and HBase resources
   */
  public static Configuration create() {
    Configuration conf = HBaseConfiguration.create();
    return addTrafResources(conf);
  }

  /**
   * @param that Configuration to clone.
   * @return a Configuration created with the trafodion-site.xml files plus
   * the given configuration.
   */
  public static Configuration create(final Configuration that) {
    Configuration conf = create();
    merge(conf, that);
    return conf;
  }
}
