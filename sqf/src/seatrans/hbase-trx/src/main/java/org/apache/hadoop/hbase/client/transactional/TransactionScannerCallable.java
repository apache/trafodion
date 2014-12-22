/**
 * Copyright 2009 The Apache Software Foundation
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
package org.apache.hadoop.hbase.client.transactional;

import java.io.IOException;

import org.apache.hadoop.hbase.client.HConnection;
import org.apache.hadoop.hbase.client.Scan;
import org.apache.hadoop.hbase.client.ScannerCallable;

class TransactionScannerCallable extends ScannerCallable {

  private TransactionState transactionState;

  TransactionScannerCallable(final TransactionState transactionState,
      final HConnection connection, final byte[] tableName, Scan scan) {

	// setting scanner metrics to null, may want to set one up later
    super(connection, tableName,  scan, null);
    this.transactionState = transactionState;
  }

  @Override
  protected long openScanner() throws IOException {
	  if (transactionState.addRegion(location)) {
			// ((TransactionalRegionInterface)
			// server).beginTransaction(transactionState
			// .getTransactionId(), location.getRegionInfo().getRegionName());
	  }
	  // No server, need to revamp this
	  // needs to get the ID from just the coprocessor
	  long id = (long) 0;
	  //((TransactionalRegionInterface)this.server).openScanner(transactionState.getTransactionId(),
		//		this.location.getRegionInfo().getRegionName(), this.getScan());

	  return id;
  }
}
