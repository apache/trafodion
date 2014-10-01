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

import java.io.IOException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.hbase.HRegionLocation;
import org.apache.hadoop.hbase.KeyValue;
import org.apache.hadoop.hbase.client.Delete;
import org.apache.hadoop.hbase.client.Get;
import org.apache.hadoop.hbase.client.HConnection;
import org.apache.hadoop.hbase.client.HTable;
import org.apache.hadoop.hbase.client.Put;
import org.apache.hadoop.hbase.client.Result;
import org.apache.hadoop.hbase.client.ResultScanner;
import org.apache.hadoop.hbase.client.Scan;
import org.apache.hadoop.hbase.client.ServerCallable;
import org.apache.hadoop.hbase.ipc.TransactionalRegionInterface;
import org.apache.hadoop.hbase.regionserver.transactional.SingleVersionDeleteNotSupported;
import org.apache.hadoop.hbase.util.Bytes;

/**
 * Table with transactional support.
 */
public class TransactionalTable extends HTable {
    static final Log LOG = LogFactory.getLog(RMInterface.class);

    /**
     * @param conf
     * @param tableName
     * @throws IOException
     */
    public TransactionalTable(final Configuration conf, final String tableName) throws IOException {
        this(conf, Bytes.toBytes(tableName));
    }

    /**
     * @param conf
     * @param tableName
     * @throws IOException
     */
    public TransactionalTable(final Configuration conf, final byte[] tableName) throws IOException {
        super(conf, tableName);
        this.maxKeyValueSize = conf.getInt("hbase.client.keyvalue.maxsize", -1);
    }

    private static abstract class TransactionalServerCallable<T> extends ServerCallable<T> {

        protected TransactionState transactionState;

        protected TransactionalRegionInterface getTransactionServer() {
            return (TransactionalRegionInterface) server;
        }

        protected void recordServer() throws IOException {
	    LOG.trace("TransactionalTable.recordServer");
            if (transactionState.addRegion(location)) {
		LOG.trace("TransactionalTable.recordServer added region to TS. Beginning txn " + transactionState + " on server");
//				 getTransactionServer().beginTransaction(transactionState.getTransactionId(),
//				 location.getRegionInfo().getRegionName());
			}
		}

        /**
         * @param connection
         * @param tableName
         * @param row
         * @param transactionState
         */
        public TransactionalServerCallable(final HConnection connection, final byte[] tableName, final byte[] row,
                final TransactionState transactionState) {
            super(connection, tableName, row);
            this.transactionState = transactionState;
        }

    }

    /**
     * Method for getting data from a row
     * 
     * @param get the Get to fetch
     * @return the result
     * @throws IOException
     * @since 0.20.0
     */
    public Result get(final TransactionState transactionState, final Get get) throws IOException {
	LOG.trace("Enter TransactionalTable.get");
    	return new TransactionalServerCallable<Result>(super.getConnection(), super.getTableName(), get.getRow(),
    			       transactionState) {
    		           public Result call() throws IOException {
    		        	 recordServer();
				 LOG.trace("TransactionalTable.get after recordServer");
				 return getTransactionServer().get(transactionState.getTransactionId(),
								   location.getRegionInfo().getRegionName(), get);
    		           }
    		         }.withRetries();
    }
    

    /**
     * @param delete
     * @throws IOException
     * @since 0.20.0
     */
    public void delete(final TransactionState transactionState, final Delete delete) throws IOException {
        SingleVersionDeleteNotSupported.validateDelete(delete);
    	new TransactionalServerCallable<Boolean>(super.getConnection(), super.getTableName(), delete.getRow(),
    			transactionState) {
    		public Boolean call() throws IOException {
    			recordServer();
    		    getTransactionServer().delete(transactionState.getTransactionId(),
    		    		location.getRegionInfo().getRegionName(), delete);
    		    return null; // FindBugs NP_BOOLEAN_RETURN_NULL
    		}
        }.withRetries();
    }


    /**
     * Commit a Put to the table.
     * <p>
     * If autoFlush is false, the update is buffered.
     * 
     * @param put
     * @throws IOException
     * @since 0.20.0
     */
    public synchronized void put(final TransactionState transactionState, final Put put) throws IOException {
        validatePut(put);
	LOG.trace("TransactionalTable.put ENTRY");
            new TransactionalServerCallable<Object>(super.getConnection(), super.getTableName(), put.getRow(),
                    transactionState) {

                public Object call() throws IOException {
                    recordServer();
                    getTransactionServer().put(transactionState.getTransactionId(),
                        location.getRegionInfo().getRegionName(), put);
                    return null;
                }
            }.withRetries();
	LOG.trace("TransactionalTable.put EXIT");

    }

    public synchronized ResultScanner getScanner(final TransactionState transactionState, final Scan scan) throws IOException {
	LOG.trace("Enter TransactionalTable.getScanner");
        if (scan.getCaching() <= 0) {
            scan.setCaching(getScannerCaching());
        }

        TransactionalClientScanner scanner = new TransactionalClientScanner(transactionState, super.getConfiguration(), scan, 
        			super.getTableName(),super.getConnection()); 
        scanner.initialize();
        
        return scanner;         
    }

    public boolean checkAndDelete(final TransactionState transactionState,
    		final byte[] row, final byte[] family, final byte[] qualifier, final byte[] value,
                       final Delete delete) throws IOException {
      LOG.trace("Enter TransactionalTable.checkAndDelete row: " + row + " family: " + family + " qualifier: " + qualifier + " value: " + value);
      if (!Bytes.equals(row, delete.getRow())) {
              throw new IOException("Action's getRow must match the passed row");
      }
      
      return
      new TransactionalServerCallable<Boolean>(super.getConnection(),
				super.getTableName(), delete.getRow(), transactionState) {

			public Boolean call() throws IOException {
				recordServer();
				return
				getTransactionServer().checkAndDelete(
								      transactionState.getTransactionId(),
								      location.getRegionInfo().getRegionName(),
								      row,
								      family,
								      qualifier,
								      value,
								      delete);
			}
		}.withRetries();
     }

    
	public boolean checkAndPut(final TransactionState transactionState,
			final byte[] row, final byte[] family, final byte[] qualifier,
			final byte[] value, final Put put) throws IOException {

		LOG.trace("Enter TransactionalTable.checkAndPut row: " + row
				+ " family: " + family + " qualifier: " + qualifier
				+ " value: " + value);
		if (!Bytes.equals(row, put.getRow())) {
			throw new IOException("Action's getRow must match the passed row");
		}
		return
		new TransactionalServerCallable<Boolean>(super.getConnection(),
				super.getTableName(), put.getRow(), transactionState) {

			public Boolean call() throws IOException {
				recordServer();
				return getTransactionServer().checkAndPut(
									  transactionState.getTransactionId(),
									  location.getRegionInfo().getRegionName(),
									  row,
									  family,
									  qualifier,
									  value,
									  put);
			}
		}.withRetries();
	}

       /**
   	 * Looking forward to TransactionalRegion-side implementation
   	 * 
   	 * @param transactionState
   	 * @param deletes
   	 * @throws IOException
   	 */
   	public void delete(final TransactionState transactionState,
   			List<Delete> deletes) throws IOException {
   		LOG.trace("Enter TransactionalTable.delete[] row ");
   		// collect all rows from same region
   			final Map<HRegionLocation, List<Delete>> rows = new HashMap<HRegionLocation, List<Delete>>();
   			HRegionLocation location = null;
   			List<Delete> list = null;
   			for (Delete del : deletes) {
   				location = this.getRegionLocation(del.getRow(), false);
   				if (!rows.containsKey(location)) {
   					list = new ArrayList<Delete>();
   					rows.put(location, list);
   				} else {
   					list = rows.get(location);
   				}
   				list.add(del);
   			}

   			final List<Delete> rowsInSameRegion = new ArrayList<Delete>();
   			for (Map.Entry<HRegionLocation, List<Delete>> entry : rows.entrySet()) {
   				rowsInSameRegion.clear();
   				rowsInSameRegion.addAll(entry.getValue());
   				new TransactionalServerCallable<Object>(super.getConnection(),
   						super.getTableName(), rowsInSameRegion.get(0).getRow(),
   						transactionState) {

   					public Object call() throws IOException {
   						recordServer();

   						getTransactionServer().delete(
   								transactionState.getTransactionId(),
   								location.getRegionInfo().getRegionName(),
   								rowsInSameRegion.toArray(new Delete[rowsInSameRegion.size()]));
   						return null;
   					}
   				}.withRetries();
   			}
   	}

   	/**
	 * Put a set of rows
	 * 
	 * @param transactionState
	 * @param puts
	 * @throws IOException
	 */
	public void put(final TransactionState transactionState,
			final List<Put> puts) throws IOException {
		LOG.trace("Enter TransactionalTable.put[] row ");
		// collect all rows from same region
		final Map<HRegionLocation, List<Put>> rows = new HashMap<HRegionLocation, List<Put>>();
		HRegionLocation location = null;
		List<Put> list = null;
		for (Put put : puts) {
			validatePut(put);
			location = this.getRegionLocation(put.getRow(), false);
			if (!rows.containsKey(location)) {
				list = new ArrayList<Put>();
				rows.put(location, list);
			} else {
				list = rows.get(location);
			}
			list.add(put);
		}

		final List<Put> rowsInSameRegion = new ArrayList<Put>();
		for (Map.Entry<HRegionLocation, List<Put>> entry : rows.entrySet()) {
			rowsInSameRegion.clear();
			rowsInSameRegion.addAll(entry.getValue());
			new TransactionalServerCallable<Object>(super.getConnection(),
					super.getTableName(), rowsInSameRegion.get(0).getRow(),
					transactionState) {

				public Object call() throws IOException {
					recordServer();

					getTransactionServer().put(
							transactionState.getTransactionId(),
							location.getRegionInfo().getRegionName(),
							rowsInSameRegion.toArray(new Put[puts.size()]));
					return null;
				}
			}.withRetries();
		}
	}
	
	// validate for well-formedness
	private void validatePut(final Put put) throws IllegalArgumentException {
		if (put.isEmpty()) {
			throw new IllegalArgumentException("No columns to insert");
		}
		if (maxKeyValueSize > 0) {
			for (List<KeyValue> list : put.getFamilyMap().values()) {
				for (KeyValue kv : list) {
					if (kv.getLength() > maxKeyValueSize) {
						throw new IllegalArgumentException(
								"KeyValue size too large");
					}
				}
			}
		}
	}
	
	private int maxKeyValueSize;
	
}
