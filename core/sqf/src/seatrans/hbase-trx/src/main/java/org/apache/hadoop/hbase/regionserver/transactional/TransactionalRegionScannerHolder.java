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

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.apache.hadoop.hbase.regionserver.Region;
import org.apache.hadoop.hbase.regionserver.RegionScanner;

/**
 * Holds a RegionScanner
 */
 public class TransactionalRegionScannerHolder {
    public RegionScanner s;
    public Region r;
    public long nextCallSeq;
    public long numberOfRows;
    public long rowsRemaining;
    public long transId;
    public long scannerId;
    public boolean hasMore;


    public TransactionalRegionScannerHolder(long transId,
                                            long scannerId,
                                            RegionScanner s, 
                                            Region r) {
      this.transId = transId;
      this.scannerId = scannerId;
      this.s = s;
      this.r = r;
      this.nextCallSeq = 0L;
      this.numberOfRows = 0L;
      this.rowsRemaining = 0L;
      this.hasMore = false;
    }

    public void cleanHolder() {
      this.r = null;
      this.s = null;
    }
  }

