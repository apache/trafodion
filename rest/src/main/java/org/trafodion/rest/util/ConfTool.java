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

package org.trafodion.dcs.util;

import org.apache.hadoop.conf.Configuration;
import org.trafodion.rest.util.RestConfiguration;

/**
 * Tool that prints out a configuration.
 * Pass the configuration key on the command-line.
 */
public class ConfTool {
  public static void main(String args[]) {
    if (args.length < 1) {
      System.err.println("Usage: ConfTool <CONFIGURATION_KEY>");
      System.exit(1);
      return;
    }

    Configuration conf = RestConfiguration.create();
    System.out.println(conf.get(args[0]));
  }
}