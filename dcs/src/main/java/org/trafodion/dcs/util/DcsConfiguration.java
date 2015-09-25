/**
/**
 * Copyright 2007 The Apache Software Foundation
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
package org.trafodion.dcs.util;

import java.util.Map.Entry;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.apache.hadoop.conf.Configuration;

/**
 * Adds dcs configuration files to a Configuration
 */
public class DcsConfiguration extends Configuration {

  private static final Log LOG = LogFactory.getLog(DcsConfiguration.class);

  // a constant to convert a fraction to a percentage
  private static final int CONVERT_TO_PERCENTAGE = 100;
 
  public static Configuration addWmsResources(Configuration conf) {
    conf.addResource("dcs-default.xml");
    conf.addResource("dcs-site.xml");

//    checkDefaultsVersion(conf);
//    checkForClusterFreeMemoryLimit(conf);
    return conf;
  }

  /**
   * Creates a Configuration with Dcs resources
   * @return a Configuration with Dcs resources
   */
  public static Configuration create() {
    Configuration conf = new Configuration();
    return addWmsResources(conf);
  }

  /**
   * @param that Configuration to clone.
   * @return a Configuration created with the dcs-*.xml files plus
   * the given configuration.
   */
  public static Configuration create(final Configuration that) {
    Configuration conf = create();
    merge(conf, that);
    return conf;
  }

  /**
   * Merge two configurations.
   * @param destConf the configuration that will be overwritten with items
   *                 from the srcConf
   * @param srcConf the source configuration
   **/
  public static void merge(Configuration destConf, Configuration srcConf) {
    for (Entry<String, String> e : srcConf) {
      destConf.set(e.getKey(), e.getValue());
    }
  }
  
  /** For debugging.  Dump configurations to system output as xml format.
   * Master and RS configurations can also be dumped using
   * http services. e.g. "curl http://master:60010/dump"
   */
  public static void main(String[] args) throws Exception {
    DcsConfiguration.create().writeXml(System.out);
  }
}
