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

import org.apache.hadoop.conf.Configuration;
import org.trafodion.dcs.util.DcsConfiguration;

/**
 * Tool that prints out a configuration.
 * Pass the configuration key on the command-line.
 */
public class DcsConfTool {
  public static void main(String args[]) {
    if (args.length < 1) {
      System.err.println("Usage: DcsConfTool <CONFIGURATION_KEY>");
      System.exit(1);
      return;
    }

    Configuration conf = DcsConfiguration.create();
    System.out.println(conf.get(args[0]));
  }
}