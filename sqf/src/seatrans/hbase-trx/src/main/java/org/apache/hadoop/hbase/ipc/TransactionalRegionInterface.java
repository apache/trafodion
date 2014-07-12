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
package org.apache.hadoop.hbase.ipc;

import java.io.IOException;
import java.util.List;
import java.util.Map;

import org.apache.hadoop.hbase.NotServingRegionException;
import org.apache.hadoop.hbase.client.Delete;
import org.apache.hadoop.hbase.client.Get;
import org.apache.hadoop.hbase.client.Put;
import org.apache.hadoop.hbase.client.Result;
import org.apache.hadoop.hbase.client.Scan;
import org.apache.hadoop.hbase.HRegionInfo;
import org.apache.hadoop.hbase.client.transactional.TransactionState;


/**
 * Interface for transactional region servers.
 * 
 * <p>
 * NOTE: if you change the interface, you must change the RPC version number in
 * HBaseRPCProtocolVersion
 * 
 */
public interface TransactionalRegionInterface extends HRegionInterface {

  /** Status code representing a transaction that can be committed. */
  final int COMMIT_OK = 1;
  /** Status code representing a read-only transaction that can be committed. */
  final int COMMIT_OK_READ_ONLY = 2;
  /** Status code representing a transaction that cannot be committed. */
  final int COMMIT_UNSUCCESSFUL = 3;
  /** Status code representing a transaction that cannot be committed due to conflict. */
  final int COMMIT_CONFLICT = 5;

  /**
   * Sent to initiate a transaction.
   * 
   * @param transactionId
   * @param regionName name of region
   * @throws IOException
   */
  void beginTransaction(long transactionId, final byte[] regionName)
      throws IOException;

  /**
   * Perform a transactional Get operation.
   * 
   * @param regionName name of region to get from
   * @param get Get operation
   * @return Result
   * @throws IOException
   */
  public Result get(long transactionId, byte[] regionName, Get get)
      throws IOException;

  /**
   * Transactional put data into the specified region
   * 
   * @param regionName
   * @param put the data to be put
   * @throws IOException
   */
  public void put(long transactionId, final byte[] regionName, final Put put)
      throws IOException;

  /**
   * Put an array of puts into the specified region
   * 
   * @param regionName
   * @param puts
   * @return result
   * @throws IOException
   */
  public int put(long transactionId, final byte[] regionName, final Put[] puts)
      throws IOException;

  /**
   * Deletes all the KeyValues that match those found in the Delete object, if
   * their ts <= to the Delete. In case of a delete with a specific ts it only
   * deletes that specific KeyValue.
   * 
   * @param regionName
   * @param delete
   * @throws IOException
   */
  public void delete(long transactionId, final byte[] regionName,
      final Delete delete) throws IOException;

  //
  // remote scanner interface
  //

  /**
   * Opens a remote transactional scanner with a RowFilter.
   * 
   * @param regionName name of region to scan
   * @param scan configured scan object
   * @return scannerId scanner identifier used in other calls
   * @throws IOException
   */
  public long openScanner(long transactionId, final byte[] regionName,
      final Scan scan) throws IOException;


// add a TRegionInterface for TM/trx to inkove recover request to each TRegion
//
  List<Long> recoveryRequest(final byte[] regionName, int tmId)
      throws IOException;

  /**
   * Ask if we can commit the given transaction.
   * 
   * @param regionName
   * @param transactionId
   * @return status of COMMIT_OK, COMMIT_READ_ONLY, or COMMIT_UNSUSESSFULL
   * @throws IOException
   */
  int commitRequest(final byte[] regionName, long transactionId)
      throws IOException;

  /**
   * Try to commit the given transaction. This is used when there is only one
   * participating region.
   * 
   * @param regionName
   * @param transactionId
   * @return true if committed
   * @throws IOException
   */
  boolean commitIfPossible(final byte[] regionName, long transactionId)
      throws IOException;

  /**
   * Commit the transaction.
   * 
   * @param regionName
   * @param transactionId
   * @throws IOException
   */
  void commit(final byte[] regionName, long transactionId) throws IOException;

  /**
   * Abort the transaction.
   * 
   * @param regionName
   * @param transactionId
   * @throws IOException
   */
  void abortTransaction(final byte[] regionName, long transactionId)
      throws IOException;


  /**Delete rows from the same region
 * @param transactionId
 * @param regionName
 * @param array
 * @throws IOException 
 * @throws NotServingRegionException 
 */
  int delete(long transactionId, byte[] regionName, Delete[] deletes) throws NotServingRegionException, IOException;

	/**
	 * Check before putting
	 * 
	 * @param transactionId
	 * @param regionName
	 * @param value
	 * @param qualifier
	 * @param family
	 * @param row
	 * @param put
	 * @throws NotServingRegionException
	 * @throws IOException 
	 */
	boolean checkAndPut(long transactionId, byte[] regionName, byte[] row,
			byte[] family, byte[] qualifier, byte[] value, Put put)
			throws NotServingRegionException, IOException;

	/**
	 * Check before deleting
	 * 
	 * @param transactionId
	 * @param regionName
	 * @param value
	 * @param qualifier
	 * @param family
	 * @param row
	 * @param delete
	 * @throws NotServingRegionException
	 * @throws IOException 
	 */
	boolean checkAndDelete(long transactionId, byte[] regionName, byte[] row,
			byte[] family, byte[] qualifier, byte[] value, Delete delete)
			throws NotServingRegionException, IOException;
	
	boolean isMoveable(final byte[] regionName)
	      throws IOException;

    /**
    * Checks if active transactions are present in regions
    *
    * @param regionName
    * @throws IOException
    */
    List<Long> getPendingTrans(byte [] regionName)
            throws IOException;

    /**
    * Checks committed transactions per region
    * 
    * @param regionName
    * @throws IOException
    */
    List<Long> getCommittedTrans(byte [] regionName)
            throws IOException;

    /**
    * Checks for in-doubt transactiosn per region
    *
    * @param regionName
    * @throws IOException
    */
    List<Long> getInDoubtTrans(byte [] regionName)
           throws IOException;

}
