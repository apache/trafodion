/**
* @@@ START COPYRIGHT @@@
*
* Licensed to the Apache Software Foundation (ASF) under one
* or more contributor license agreements.  See the NOTICE file
* distributed with this work for additional information
* regarding copyright ownership.  The ASF licenses this file
* to you under the Apache License, Version 2.0 (the
* "License"); you may not use this file except in compliance
* with the License.  You may obtain a copy of the License at
*
*   http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing,
* software distributed under the License is distributed on an
* "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
* KIND, either express or implied.  See the License for the
* specific language governing permissions and limitations
* under the License.
*
* @@@ END COPYRIGHT @@@
**/

package org.apache.hadoop.hbase.regionserver.transactional;

import java.io.IOException;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import org.apache.hadoop.hbase.Chore;
import org.apache.hadoop.hbase.Stoppable;

import org.apache.hadoop.hbase.coprocessor.transactional.TrxRegionEndpoint;

/**
 * Manages the MemoryMXBean to determine a regionserver's memory usage.
 */
public class MemoryUsageChore extends Chore {

  private final TrxRegionEndpoint trx_Region;

  static final Log LOG = LogFactory.getLog(MemoryUsageChore.class);

  /**
   * @param trx_Region
   * @param timer        
   * @param stoppable    
   */
  public MemoryUsageChore(final TrxRegionEndpoint trx_Region,
                          final int timer,  
                          final Stoppable stoppable) {
    super("MemoryUsageChore", timer, stoppable);
    this.trx_Region = trx_Region;
  }

  @Override
  public void chore() {

      //if (LOG.isTraceEnabled()) LOG.trace("Trafodion MemoryUsage ChoreThread, calling checkMemoryUsage, CP Epoch " + trx_Region.controlPointEpoch.get());
      trx_Region.checkMemoryUsage();

  }
}
