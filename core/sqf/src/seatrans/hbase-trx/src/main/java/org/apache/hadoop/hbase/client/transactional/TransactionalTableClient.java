// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2013-2015 Hewlett-Packard Development Company, L.P.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//

package org.apache.hadoop.hbase.client.transactional;

import java.io.IOException;

import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.hbase.client.Delete;
import org.apache.hadoop.hbase.client.Get;
import org.apache.hadoop.hbase.client.HTable;
import org.apache.hadoop.hbase.client.Put;
import org.apache.hadoop.hbase.client.Result;
import org.apache.hadoop.hbase.client.ResultScanner;
import org.apache.hadoop.hbase.client.Scan;
import org.apache.hadoop.hbase.HRegionLocation;
import org.apache.hadoop.hbase.client.HConnection;
import java.io.InterruptedIOException;
import org.apache.hadoop.hbase.client.RetriesExhaustedWithDetailsException;



import org.apache.hadoop.hbase.client.transactional.TransactionState;
import org.apache.hadoop.hbase.client.transactional.SsccUpdateConflictException;

import java.util.List;
import java.util.Collection;

/**
 * Table with transactional support.
 */
public interface  TransactionalTableClient  {
    /**
     * Method for getting data from a row
     * 
     * @param get the Get to fetch
     * @return the result
     * @throws IOException
     * @since 0.20.0
     */
    
    Result get(final TransactionState transactionState, final Get get) throws IOException;

    Result get(final TransactionState transactionState, final Get get, final boolean bool_addLocation) throws IOException;
    
    /**
     * @param delete
     * @throws IOException
     * @since 0.20.0
     */
    void delete(final TransactionState transactionState, final Delete delete) throws IOException;

    void delete(final TransactionState transactionState, final Delete delete, final boolean bool_addLocation) throws IOException;

    /**
     * Commit a Put to the table.
     * <p>
     * If autoFlush is false, the update is buffered.
     * 
     * @param put
     * @throws IOException
     * @since 0.20.0
     */
    void put(final TransactionState transactionState, final Put put) throws IOException ;

    void put(final TransactionState transactionState, final Put put, final boolean bool_addLocation) throws IOException ;
    ResultScanner getScanner(final TransactionState transactionState, final Scan scan) throws IOException;

    boolean checkAndDelete(final TransactionState transactionState,
  		final byte[] row, final byte[] family, final byte[] qualifier, final byte[] value,
                     final Delete delete) throws IOException;
    
	boolean checkAndPut(final TransactionState transactionState,
			final byte[] row, final byte[] family, final byte[] qualifier,
			final byte[] value, final Put put) throws IOException ;

       /**
   	 * Looking forward to TransactionalRegion-side implementation
   	 * 
   	 * @param transactionState
   	 * @param deletes
   	 * @throws IOException
   	 */
   	void delete(final TransactionState transactionState,
   			List<Delete> deletes) throws IOException ;

   	/**
	 * Put a set of rows
	 * 
	 * @param transactionState
	 * @param puts
	 * @throws IOException
	 */
	void put(final TransactionState transactionState,
			final List<Put> puts) throws IOException ;

    HRegionLocation getRegionLocation(byte[] row, boolean f)
                                  throws IOException;
                                  
    void close() throws IOException ;
        
    void setAutoFlush(boolean autoFlush, boolean b);
    org.apache.hadoop.conf.Configuration getConfiguration();
    void flushCommits()
                  throws InterruptedIOException,
                RetriesExhaustedWithDetailsException ;
    HConnection getConnection();

    byte[][] getEndKeys()
                    throws IOException;

    byte[][] getStartKeys() throws IOException;

    void setWriteBufferSize(long writeBufferSize) throws IOException;

    long getWriteBufferSize();

    byte[] getTableName();

    ResultScanner getScanner(Scan scan) throws IOException;
    Result get(Get g) throws IOException; 

    
    Result[] get( List<Get> g) throws IOException;

    void delete(Delete d) throws IOException;

    void delete(List<Delete> deletes) throws IOException;

    boolean checkAndPut(byte[] row, byte[] family, byte[] qualifier, byte[] value, Put put) throws IOException;

    void put(Put p) throws  InterruptedIOException,RetriesExhaustedWithDetailsException;
    public void put(List<Put> p) throws  InterruptedIOException,RetriesExhaustedWithDetailsException;
    public boolean checkAndDelete(byte[] row, byte[] family, byte[] qualifier, byte[] value,  Delete delete) throws IOException;
}
