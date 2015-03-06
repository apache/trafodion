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

package org.trafodion.rest.util;

import java.util.Map.Entry;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.apache.hadoop.conf.Configuration;

/**
 * Adds rest configuration files to a Configuration
 */
public class RestConfiguration extends Configuration {

  private static final Log LOG = LogFactory.getLog(RestConfiguration.class);

  // a constant to convert a fraction to a percentage
  private static final int CONVERT_TO_PERCENTAGE = 100;
 
  public static Configuration addRestResources(Configuration conf) {
    conf.addResource("rest-default.xml");
    conf.addResource("rest-site.xml");

//    checkDefaultsVersion(conf);
//    checkForClusterFreeMemoryLimit(conf);
    return conf;
  }

  /**
   * Creates a Configuration with Rest resources
   * @return a Configuration with Rest resources
   */
  public static Configuration create() {
    Configuration conf = new Configuration();
    return addRestResources(conf);
  }

  /**
   * @param that Configuration to clone.
   * @return a Configuration created with the rest-*.xml files plus
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
    RestConfiguration.create().writeXml(System.out);
  }
}
