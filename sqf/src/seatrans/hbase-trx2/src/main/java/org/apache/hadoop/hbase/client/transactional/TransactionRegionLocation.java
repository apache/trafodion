/**
 * Copyright 2009 The Apache Software Foundation Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements. See the NOTICE file distributed with this work for additional information regarding
 * copyright ownership. The ASF licenses this file to you under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License. You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0 Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific language governing permissions and limitations under the
 * License.
 */
package org.apache.hadoop.hbase.client.transactional;

import java.util.HashSet;
import java.util.Set;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.apache.hadoop.hbase.HRegionLocation;
import org.apache.hadoop.hbase.HRegionInfo;

public class TransactionRegionLocation extends HRegionLocation {

   public TransactionRegionLocation(HRegionInfo regionInfo, final String hostname, final int port) {
      super(regionInfo, hostname, port);
   }

   @Override
   //public int compareTo(HRegionLocation o) {
   public int compareTo(HRegionLocation o) {
      int result = super.getHostname().compareTo(o.getHostname());
      if (result != 0) return result;
      result = super.getPort() - o.getPort();
      if (result != 0) return result;
      return super.getRegionInfo().compareTo(o.getRegionInfo());
   }
}
