// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2013-2014 Hewlett-Packard Development Company, L.P.
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
// @@@ END COPYRIGHT @@@

package org.trafodion.sql.HBaseAccess;
import org.trafodion.sql.HBaseAccess.*;

import java.io.IOException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.HashMap;
import java.util.Map;
import java.util.NavigableSet;

import java.util.concurrent.Callable;
import java.util.concurrent.Future;
import java.util.concurrent.Executors;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.ExecutionException;
import java.nio.ByteBuffer;
import java.nio.LongBuffer;
import java.nio.ByteOrder;

import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.hbase.KeyValue;
import org.apache.hadoop.hbase.client.Delete;
import org.apache.hadoop.hbase.client.Get;
import org.apache.hadoop.hbase.client.Put;
import org.apache.hadoop.hbase.client.Result;
import org.apache.hadoop.hbase.client.ResultScanner;
import org.apache.hadoop.hbase.client.Scan;
import org.apache.hadoop.hbase.client.HConnection;
import org.apache.hadoop.hbase.util.Bytes;
import org.apache.hadoop.hbase.client.transactional.RMInterface;
import org.apache.hadoop.hbase.client.transactional.TransactionalAggregationClient;
import org.apache.hadoop.hbase.client.transactional.TransactionState;

import org.apache.log4j.Logger;

// H98 coprocessor needs
import java.util.*;
import org.apache.hadoop.hbase.*;
import org.apache.hadoop.hbase.client.*;
import org.apache.hadoop.hbase.client.coprocessor.*;
import org.apache.hadoop.hbase.coprocessor.*;
import org.apache.hadoop.hbase.coprocessor.example.*;
import org.apache.hadoop.hbase.ipc.*;
import org.apache.hadoop.hbase.protobuf.generated.HBaseProtos.*;
import org.apache.hadoop.hbase.util.*;

//import org.apache.hadoop.hbase.client.coprocessor.AggregationClient;
import org.apache.hadoop.hbase.coprocessor.ColumnInterpreter;
import org.apache.hadoop.hbase.client.coprocessor.LongColumnInterpreter;

// classes to do column value filtering
import org.apache.hadoop.hbase.filter.Filter;
import org.apache.hadoop.hbase.filter.SingleColumnValueFilter;
import org.apache.hadoop.hbase.filter.CompareFilter.CompareOp;
import org.apache.hadoop.hbase.filter.FilterList;
import org.apache.hadoop.hbase.filter.RandomRowFilter;

public class HTableClient {
	private boolean useTRex;
	private boolean useTRexScanner;
	private String tableName;

	private ResultScanner scanner = null;
	private ResultIterator resultIterator = null;
	KeyValue lastFetchedCell = null;
	Result[] getResultSet = null;
	String lastError;
        RMInterface table = null;
        ByteArrayList coprocAggrResult = null;
        private boolean writeToWAL = false;
	int numRowsCached = 1;
	int numColsInScan = 0;
	int[] kvValLen = null;
	int[] kvValOffset = null;
	int[] kvQualLen = null;
	int[] kvQualOffset = null;
	int[] kvFamLen = null;
	int[] kvFamOffset = null;
	long[] kvTimestamp = null;
	byte[][] kvBuffer = null;
	byte[][] rowIDs = null;
	int[] kvsPerRow = null;
        ExecutorService executorService = null;
        Future future = null;
	boolean preFetch = false;

	static Logger logger = Logger.getLogger(HTableClient.class.getName());;

	public class QualifiedColumn {
		byte[] family = null;
		byte[] name = null;

		// Once we move to HBase 0.95, this method is provided by the Bytes
		// class.
		int indexOf(byte[] array, byte target) {
			for (int i = 0; i < array.length; i++) {
				if (array[i] == target) {
					return i;
				}
			}
			return -1;
		}

		public QualifiedColumn(byte[] qc) {
			if (qc != null && qc.length > 0) {
				int pos = indexOf(qc, (byte) ':');
				if (pos == -1) {
					family = Bytes.toBytes("cf1");
					name = qc;
				} else {
					family = Arrays.copyOfRange(qc, 0, pos);
					name = Arrays.copyOfRange(qc, pos + 1, qc.length);
				}
			}
		}

		byte[] getFamily() {
			return family;
		}

		byte[] getName() {
			return name;
		}
	}

	public boolean setWriteBufferSize(long writeBufferSize) throws IOException {
		if (logger.isDebugEnabled()) logger.debug("Enter HTableClient::setWriteBufferSize, size  : " + writeBufferSize);
	    table.setWriteBufferSize(writeBufferSize);
	    return true;
	  }
	 public long getWriteBufferSize() {
		 if (logger.isDebugEnabled()) logger.debug("Enter HTableClient::getWriteBufferSize, size return : " + table.getWriteBufferSize());
		 return table.getWriteBufferSize();
	 }
	public boolean setWriteToWAL(boolean v) {
		if (logger.isDebugEnabled()) logger.debug("Enter HTableClient::setWriteToWALL, size  : " + v);
	    writeToWAL = v;
	    return true;
	  }
 
	public boolean init(String tblName, Configuration config, 
              boolean useTRex) throws IOException 
        {
	    if (logger.isDebugEnabled()) logger.debug("Enter HTableClient::init, tableName: " + tblName);
	    this.useTRex = useTRex;
	    tableName = tblName;
	    
	    if ( !this.useTRex ) {
		this.useTRexScanner = false;
	    }
	    else {

		// If the parameter useTRex is false, then do not go thru this logic

		String useTransactions = System.getenv("USE_TRANSACTIONS");
		if (useTransactions != null) {
		    int lv_useTransactions = (Integer.parseInt(useTransactions));
		    if (lv_useTransactions == 0) {
			this.useTRex = false;
		    }
		}
	    
		this.useTRexScanner = true;
		String useTransactionsScanner = System.getenv("USE_TRANSACTIONS_SCANNER");
		if (useTransactionsScanner != null) {
		    int lv_useTransactionsScanner = (Integer.parseInt(useTransactionsScanner));
		    if (lv_useTransactionsScanner == 0) {
			this.useTRexScanner = false;
		    }
		}
	    }

	    config.set("hbase.hregion.impl", "org.apache.hadoop.hbase.regionserver.transactional.TransactionalRegion");
	    table = new RMInterface(config, tblName);
	    //	    table = new HTable(config, tblName);
	    if (logger.isDebugEnabled()) logger.debug("Exit HTableClient::init, table object: " + table);
	    return true;
	}

	public String getLastError() {
		String ret = lastError;
		lastError = null;
		return ret;
	}

	void setLastError(String err) {
		lastError = err;
	}

	String getTableName() {
		return tableName;
	}

	String getHTableName() {
		if (table == null)
			return null;
		else
			return new String(table.getTableName());
	}

	void resetAutoFlush() {
		table.setAutoFlush(true, true);
	}

	public boolean startScan(long transID, byte[] startRow, byte[] stopRow,
				 Object[]  columns, long timestamp,
				 boolean cacheBlocks, int numCacheRows,
				 Object[] colNamesToFilter, 
				 Object[] compareOpList, 
				 Object[] colValuesToCompare,
				 float samplePercent,
				 boolean inPreFetch) 
           throws IOException {
	    if (logger.isTraceEnabled()) logger.trace("Enter startScan() " + tableName + " txid: " + transID);

		Scan scan;

		if (startRow != null && startRow.toString() == "")
			startRow = null;
		if (stopRow != null && stopRow.toString() == "")
			stopRow = null;

		if (startRow != null && stopRow != null)
			scan = new Scan(startRow, stopRow);
		else
			scan = new Scan();

		if (cacheBlocks == true)
			scan.setCacheBlocks(true);
		else
			scan.setCacheBlocks(false);

		scan.setCaching(numCacheRows);
		numRowsCached = numCacheRows;
		if (columns != null) {
			numColsInScan = columns.length;
			for (int i = 0; i < columns.length ; i++) {
				byte[] col = (byte[])columns[i];
				QualifiedColumn qc = new QualifiedColumn(col);
				scan.addColumn(qc.getFamily(), qc.getName());
			}
		}
		else
			numColsInScan = 0;
		if (colNamesToFilter != null) {
			FilterList list = new FilterList(FilterList.Operator.MUST_PASS_ALL);

			for (int i = 0; i < colNamesToFilter.length; i++) {
				byte[] colName = (byte[])colNamesToFilter[i];
				QualifiedColumn qc = new QualifiedColumn(colName);
					
				byte[] coByte = (byte[])compareOpList[i];
				byte[] colVal = (byte[])colValuesToCompare[i];

				if ((coByte == null) || (colVal == null)) {
					return false;
				}

				String coStr = new String(coByte);
				CompareOp co = CompareOp.valueOf(coStr);

				SingleColumnValueFilter filter1 = 
					new SingleColumnValueFilter(qc.getFamily(), qc.getName(), 
							co, colVal);
				list.addFilter(filter1);
			}

			if (samplePercent > 0.0f)
			      list.addFilter(new RandomRowFilter(samplePercent));
		    scan.setFilter(list);
		} else if (samplePercent > 0.0f) {
			scan.setFilter(new RandomRowFilter(samplePercent));
		}

		if (useTRexScanner && (transID != 0)) {
		    scanner = table.getScanner(transID, scan);
		} else {
		    scanner = table.getScanner(scan);
		}
		if (logger.isTraceEnabled()) logger.trace("startScan(). After getScanner. Scanner: " + scanner);
		resultIterator = new ResultIterator(scanner);
		
		preFetch = inPreFetch;
		if (preFetch)
		{
 			executorService = Executors.newFixedThreadPool(1);
			future = executorService.submit(new Callable<Result[]>() {
				public Result[] call() throws Exception {
					return scanner.next(numRowsCached);
				}
			});
		}

		if (logger.isTraceEnabled()) logger.trace("Exit startScan().");
		return true;
	}

	public boolean startGet(long transID, byte[] rowID, 
                     Object[] columns,
		     long timestamp, boolean directRow) throws IOException {

	    if (logger.isTraceEnabled()) logger.trace("Enter startGet(" + tableName + 
			     " #cols: " + ((columns == null) ? 0:columns.length ) +
			     " rowID: " + new String(rowID));

		Get get = new Get(rowID);
		if (columns != null)
		{
			for (int i = 0; i < columns.length; i++) {
				byte[] col = (byte[]) columns[i];
				QualifiedColumn qc = new QualifiedColumn(col);
				get.addColumn(qc.getFamily(), qc.getName());
			}
			numColsInScan = columns.length;
		}
		else
			numColsInScan = 0;
			
		Result getResult;
		if (useTRex && (transID != 0)) {
			getResult = table.get(transID, get);
		} else {
			getResult = table.get(get);
		}
		if (getResult == null
                    || getResult.isEmpty()) {
			return false;
		}
		if (logger.isTraceEnabled()) logger.trace("startGet, result: " + getResult);
		if (directRow) {
			getResultSet = new Result[1];
			getResultSet[0] = getResult;
		} else {
			resultIterator = new ResultIterator(getResult);
		}
		if (logger.isTraceEnabled()) logger.trace("Exit 2 startGet. size: " + getResult.size());
		return true;
	}

	// The TransactionalTable class is missing the batch get operation,
	// so work around it.
	private Result[] batchGet(long transactionID, List<Get> gets)
			throws IOException {
		if (logger.isTraceEnabled()) logger.trace("Enter batchGet(multi-row) " + tableName);
		Result [] results = new Result[gets.size()];
		int i=0;
		for (Get g : gets) {
		    //Result r = table.get(transactionID, g);
			Result r = table.get(g);
			if (r != null && r.isEmpty() == false)
				results[i++] = r;
		}
	
		return results;
	}

	public boolean startGet(long transID, Object[] rows,
			Object[] columns, long timestamp,
			boolean directRow) 
                        throws IOException {

		if (logger.isTraceEnabled()) logger.trace("Enter startGet(multi-row) " + tableName);

		List<Get> listOfGets = new ArrayList<Get>();
		for (int i = 0; i < rows.length; i++) {
			byte[] rowID = (byte[])rows[i]; 
			Get get = new Get(rowID);
			listOfGets.add(get);
		}
		if (columns != null)
		{
			for (int j = 0; j < columns.length; j++ ) {
				byte[] col = (byte[])columns[j];
				QualifiedColumn qc = new QualifiedColumn(col);
				for (Get get : listOfGets)
					get.addColumn(qc.getFamily(), qc.getName());
			}
			numColsInScan = columns.length;
		}
		else
			numColsInScan = 0;
		if (useTRex && (transID != 0)) {
			getResultSet = batchGet(transID, listOfGets);
		} else {
			getResultSet = table.get(listOfGets);
		}
		if (directRow) 
			return true;
		else {
			if (getResultSet.length > 0) {
			        resultIterator = new ResultIterator(getResultSet);
			} else
				resultIterator = null;
		}
		return true;
	}

	public boolean scanFetch() throws IOException {
		if (logger.isTraceEnabled()) logger.trace("Enter scanFetch() " + tableName);
		if (resultIterator == null) {
			return false;
		}

		lastFetchedCell = resultIterator.nextCell();
		if (lastFetchedCell == null) {
			return false; 
		}
		return true;
	}

	public boolean getFetch() throws IOException {
		if (logger.isTraceEnabled()) logger.trace("Enter getFetch() " + tableName);
		return scanFetch();
	}

	public int fetchRows(long jniObject) throws IOException, 
			InterruptedException, ExecutionException {
		int rowsReturned = 0;

		if (logger.isTraceEnabled()) logger.trace("Enter fetchRows(). Table: " + tableName);
		if (getResultSet != null)
		{
			rowsReturned = pushRowsToJni(jniObject, getResultSet);
			getResultSet = null;
			return rowsReturned;
		}
		else
		{
			if (scanner == null) {
				String err = "  fetchRows() called before scanOpen().";
				logger.error(err);
				setLastError(err);
				return -1;
			}
			Result[] result = null;
			if (preFetch)
			{
				result = (Result[])future.get();
				rowsReturned = pushRowsToJni(jniObject, result);
				future = null;
				if ((rowsReturned <= 0 || rowsReturned < numRowsCached))
					return rowsReturned;
				future = executorService.submit(new Callable<Result[]>() {
					public Result[] call() throws Exception {					
						return scanner.next(numRowsCached);
					}
				});
			}
			else
			{
				result = scanner.next(numRowsCached);
				rowsReturned = pushRowsToJni(jniObject, result);
			}
			return rowsReturned;
		}
	}

	protected int pushRowsToJni(long jniObject, Result[] result) 
			throws IOException {
		if (result == null || result.length == 0)
			return 0; 
		int rowsReturned = result.length;
		// There can be maximum of 2 versions per kv
		// So, allocate place holder to keep cell info
		// for that many KVs
		int numTotalCells = 2 * rowsReturned * numColsInScan;
		int numColsReturned;
		HashMap<String, Integer>  kvMap = null;
		List<KeyValue> kvList;
		KeyValue kv;

		if (kvValLen == null ||
	 		(kvValLen != null && numTotalCells > kvValLen.length))
		{
			kvValLen = new int[numTotalCells];
			kvValOffset = new int[numTotalCells];
			kvQualLen = new int[numTotalCells];
			kvQualOffset = new int[numTotalCells];
			kvFamLen = new int[numTotalCells];
			kvFamOffset = new int[numTotalCells];
			kvTimestamp = new long[numTotalCells];
			kvBuffer = new byte[numTotalCells][];
		}
		if (rowIDs == null || (rowIDs != null &&
				rowsReturned > rowIDs.length))
		{
			rowIDs = new byte[rowsReturned][];
			kvsPerRow = new int[rowsReturned];
		}
		int cellNum = 0;
		for (int rowNum = 0; rowNum < rowsReturned ; rowNum++)
		{
			if (result[rowNum] != null)
			{
				rowIDs[rowNum] = result[rowNum].getRow();
				kvList = result[rowNum].list();
			}
			else
			{
				rowIDs[rowNum] = null;
				kvList = null;
			}
 			if (kvList == null)
				numColsReturned = 0; 
			else
				numColsReturned = kvList.size();
			if ((cellNum + numColsReturned) > numTotalCells)
				throw new IOException("Insufficient cell array pre-allocated");
			kvsPerRow[rowNum] = numColsReturned;
			for (int colNum = 0 ; colNum < numColsReturned ; colNum++, cellNum++)
			{ 
				kv = kvList.get(colNum);
				kvValLen[cellNum] = kv.getValueLength();
				kvValOffset[cellNum] = kv.getValueOffset();
				kvQualLen[cellNum] = kv.getQualifierLength();
				kvQualOffset[cellNum] = kv.getQualifierOffset();
				kvFamLen[cellNum] = kv.getFamilyLength();
				kvFamOffset[cellNum] = kv.getFamilyOffset();
				kvTimestamp[cellNum] = kv.getTimestamp();
				kvBuffer[cellNum] = kv.getBuffer();
			}
		}
		int cellsReturned = cellNum++;
		setResultInfo(jniObject, kvValLen, kvValOffset,
			kvQualLen, kvQualOffset, kvFamLen, kvFamOffset,
			kvTimestamp, kvBuffer, rowIDs, kvsPerRow, cellsReturned);
		return rowsReturned;	
	}		
	
	public KeyValue getLastFetchedCell() {
		if (logger.isTraceEnabled()) logger.trace("Enter getLastFetchedCell() ");
		if (lastFetchedCell == null)
			if (logger.isTraceEnabled()) logger.trace("  Returning empty.");
		return lastFetchedCell;
	}

	public boolean deleteRow(long transID, byte[] rowID, 
				 Object[] columns,
				 long timestamp) throws IOException {

		if (logger.isTraceEnabled()) logger.trace("Enter deleteRow(" + new String(rowID) + ", "
			     + timestamp + ") " + tableName);

			Delete del;
			if (timestamp == -1)
				del = new Delete(rowID);
			else
				del = new Delete(rowID, timestamp);

			if (columns != null) {
				for (int i = 0; i < columns.length ; i++) {
					byte[] col = (byte[]) columns[i];
					QualifiedColumn qc = new QualifiedColumn(col);
					del.deleteColumns(qc.getFamily(), qc.getName());
				}
			}

			if (useTRex && (transID != 0)) {
			    table.delete(transID, del);
			} else {
			    table.delete(del);
			}
		if (logger.isTraceEnabled()) logger.trace("Exit deleteRow");
		return true;
	}

	public boolean deleteRows(long transID, short rowIDLen, Object rowIDs,
		      long timestamp) throws IOException {

	        if (logger.isTraceEnabled()) logger.trace("Enter deleteRows() " + tableName);

		List<Delete> listOfDeletes = new ArrayList<Delete>();
		listOfDeletes.clear();
		ByteBuffer bbRowIDs = (ByteBuffer)rowIDs;
		short numRows = bbRowIDs.getShort();
                byte[] rowID;		
       
		for (short rowNum = 0; rowNum < numRows; rowNum++) {
			rowID = new byte[rowIDLen];
			bbRowIDs.get(rowID, 0, rowIDLen);

			Delete del;
			if (timestamp == -1)
			    del = new Delete(rowID);
			else
			    del = new Delete(rowID, timestamp);
			listOfDeletes.add(del);
		}

		if (useTRex && (transID != 0)) 
		    table.delete(transID, listOfDeletes);
		else
		    table.delete(listOfDeletes);
		if (logger.isTraceEnabled()) logger.trace("Exit deleteRows");
		return true;
	}


         public byte[] intToByteArray(int value) {
	     return new byte[] {
		 (byte)(value >>> 24),
		 (byte)(value >>> 16),
		 (byte)(value >>> 8),
		 (byte)value};
	 }
    
	public boolean checkAndDeleteRow(long transID, byte[] rowID, 
					 byte[] columnToCheck, byte[] colValToCheck,
					 long timestamp) throws IOException {

		if (logger.isTraceEnabled()) logger.trace("Enter checkAndDeleteRow(" + new String(rowID) + ", "
			     + new String(columnToCheck) + ", " + new String(colValToCheck) + ", " + timestamp + ") " + tableName);

			Delete del;
			if (timestamp == -1)
				del = new Delete(rowID);
			else
				del = new Delete(rowID, timestamp);

			byte[] family = null;
			byte[] qualifier = null;

			if (columnToCheck.length > 0) {
				QualifiedColumn qc = new QualifiedColumn(columnToCheck);
				//del.deleteColumns(qc.getFamily(), qc.getName());

				family = qc.getFamily();
				qualifier = qc.getName();
			}
			
			boolean res;
			if (useTRex && (transID != 0)) {
			    res = table.checkAndDelete(transID, rowID, family, qualifier, colValToCheck, del);
			} else {
			    res = table.checkAndDelete(rowID, family, qualifier, colValToCheck, del);
			}

			if (res == false)
			    return false;
		return true;
	}

	public boolean putRow(long transID, byte[] rowID, Object row,
		byte[] columnToCheck, byte[] colValToCheck,
		boolean checkAndPut) throws IOException 	{

		if (logger.isTraceEnabled()) logger.trace("Enter putRow() " + tableName);

		Put put;
		ByteBuffer bb;
		short numCols;
		short colNameLen, colValueLen;
		QualifiedColumn qc = null;
		byte[] family = null;
		byte[] qualifier = null;
		byte[] colName, colValue;

		bb = (ByteBuffer)row;
		put = new Put(rowID);
		numCols = bb.getShort();
		for (short colIndex = 0; colIndex < numCols; colIndex++)
		{
			colNameLen = bb.getShort();
			colName = new byte[colNameLen];
			bb.get(colName, 0, colNameLen);
			colValueLen = bb.getShort();	
			colValue = new byte[colValueLen];
			bb.get(colValue, 0, colValueLen);
			qc = new QualifiedColumn(colName);
			put.add(qc.getFamily(), qc.getName(), colValue); 
			if (checkAndPut && colIndex == 0) {
				family = qc.getFamily();
				qualifier = qc.getName();
			} 
		}
		if (columnToCheck != null && columnToCheck.length > 0) {
			qc = new QualifiedColumn(columnToCheck);
			family = qc.getFamily();
			qualifier = qc.getName();
		}
		boolean res = true;
		if (checkAndPut)
		{
		    if (useTRex && (transID != 0)) 
			res = table.checkAndPut(transID, rowID, 
						family, qualifier, colValToCheck, put);
		    else 
			res = table.checkAndPut(rowID, 
						family, qualifier, colValToCheck, put);
		}
		else
		{
		    if (useTRex && (transID != 0)) 
			table.put(transID, put);
		    else 
			table.put(put);
		}
		return res;
	}

	public boolean insertRow(long transID, byte[] rowID, 
                         Object row, 
			 long timestamp) throws IOException {
		return putRow(transID, rowID, row, null, null, 
				false);
	}

	public boolean putRows(long transID, short rowIDLen, Object rowIDs, 
                       Object rows,
                       long timestamp, boolean autoFlush)
			throws IOException {

		if (logger.isTraceEnabled()) logger.trace("Enter putRows() " + tableName);

		Put put;
		ByteBuffer bbRows, bbRowIDs;
		short numCols, numRows;
		short colNameLen, colValueLen;
		QualifiedColumn qc = null;
		byte[] colName, colValue, rowID;

		bbRowIDs = (ByteBuffer)rowIDs;
		bbRows = (ByteBuffer)rows;

		List<Put> listOfPuts = new ArrayList<Put>();
		numRows = bbRowIDs.getShort();
		
		for (short rowNum = 0; rowNum < numRows; rowNum++) {
			rowID = new byte[rowIDLen];
			bbRowIDs.get(rowID, 0, rowIDLen);
			put = new Put(rowID);
			numCols = bbRows.getShort();
			for (short colIndex = 0; colIndex < numCols; colIndex++)
			{
				colNameLen = bbRows.getShort();
				colName = new byte[colNameLen];
				bbRows.get(colName, 0, colNameLen);
				colValueLen = bbRows.getShort();	
				colValue = new byte[colValueLen];
				bbRows.get(colValue, 0, colValueLen);
				qc = new QualifiedColumn(colName);
				put.add(qc.getFamily(), qc.getName(), colValue); 
			}
			if (writeToWAL)  
				put.setWriteToWAL(writeToWAL);
			listOfPuts.add(put);
		}
		if (autoFlush == false)
			table.setAutoFlush(false, true);

		if (useTRex && (transID != 0)) {
		    table.put(transID, listOfPuts);
		} else {
		    table.put(listOfPuts);
		}
		return true;
	} 

	public boolean checkAndInsertRow(long transID, byte[] rowID, 
                         Object row, 
			 long timestamp) throws IOException {
		return putRow(transID, rowID, row, null, null, 
				true);
	}

	public boolean checkAndUpdateRow(long transID, byte[] rowID, 
             Object columns, byte[] columnToCheck, byte[] colValToCheck,
             long timestamp) throws IOException, Throwable  {
		return putRow(transID, rowID, columns, columnToCheck, 
			colValToCheck, 
				true);
	}

        public byte[] coProcAggr(long transID, int aggrType, 
		byte[] startRowID, 
              byte[] stopRowID, byte[] colFamily, byte[] colName, 
              boolean cacheBlocks, int numCacheRows) 
                          throws IOException, Throwable {

		    Configuration customConf = table.getConfiguration();

		    TransactionalAggregationClient aggregationClient = 
                        new TransactionalAggregationClient(customConf);
		    Scan scan = new Scan();
		    scan.addFamily(colFamily);
		    final ColumnInterpreter<Long, Long, EmptyMsg, LongMsg, LongMsg> ci =
			new LongColumnInterpreter();
		    byte[] tname = getTableName().getBytes();
		    long rowCount = aggregationClient.rowCount(transID, 
                      org.apache.hadoop.hbase.TableName.valueOf(getTableName()),
                      ci,
                      scan);

		    coprocAggrResult = new ByteArrayList();

		    byte[] rcBytes = 
                      ByteBuffer.allocate(8).order(ByteOrder.LITTLE_ENDIAN).putLong(rowCount).array();
                    return rcBytes; 
	}

	public boolean flush() throws IOException {
		if (table != null)
			table.flushCommits();
		return true;
	}

	public boolean release() throws IOException {
		if (table != null)
			table.flushCommits();
		if (scanner != null) {
			scanner.close();
			scanner = null;
		}
		cleanScan();		
		future = null;
		if (executorService != null) {
			executorService.shutdown();
			executorService = null;
		}
		resultIterator = null;
		lastFetchedCell = null;
		getResultSet = null;
		return true;
	}

	public boolean close(boolean clearRegionCache) throws IOException {
           if (logger.isTraceEnabled()) logger.trace("Enter close() " + tableName);
           if (scanner != null) {
              scanner.close();
              scanner = null;
           }
           if (table != null) 
           {
              if (clearRegionCache)
              {
                 HConnection connection = table.getConnection();
                 connection.clearRegionCache(tableName.getBytes());
              }
              table.close();
              table = null;
           }
           return true;
	}

	public ByteArrayList getEndKeys() throws IOException {
	    if (logger.isTraceEnabled()) logger.trace("Enter getEndKeys() " + tableName);
            ByteArrayList result = new ByteArrayList();
            if (table == null) {
                return null;
            }
            byte[][] htableResult = table.getEndKeys();

            // transfer the HTable result to ByteArrayList
            for (int i=0; i<htableResult.length; i++ ) {
                if (logger.isTraceEnabled()) logger.trace("Inside getEndKeys(), result[i]: " + 
                             htableResult[i]);
                if (logger.isTraceEnabled()) logger.trace("Inside getEndKeys(), result[i]: " + 
                             new String(htableResult[i]));
                result.add(htableResult[i]);
            }

            if (logger.isTraceEnabled()) logger.trace("Exit getEndKeys(), result size: " + result.getSize());
            return result;
	}

    public ByteArrayList getStartKeys() throws IOException {
        if (logger.isTraceEnabled()) logger.trace("Enter getStartKeys() " + tableName);
        ByteArrayList result = new ByteArrayList();
        if (table == null) {
            return null;
        }
        byte[][] htableResult = table.getStartKeys();

        // transfer the HTable result to ByteArrayList
        for (int i=0; i<htableResult.length; i++ ) {
            if (logger.isTraceEnabled()) logger.trace("Inside getStartKeys(), result[i]: " + 
                         htableResult[i]);
            if (logger.isTraceEnabled()) logger.trace("Inside getStartKeys(), result[i]: " + 
                         new String(htableResult[i]));
            result.add(htableResult[i]);
        }

        if (logger.isTraceEnabled()) logger.trace("Exit getStartKeys(), result size: " + result.getSize());
        return result;
    }

    private void cleanScan()
    {
        numRowsCached = 1;
        numColsInScan = 0;
        kvValLen = null;
        kvValOffset = null;
        kvQualLen = null;
        kvQualOffset = null;
        kvFamLen = null;
        kvFamOffset = null;
        kvTimestamp = null;
        kvBuffer = null;
        rowIDs = null;
        kvsPerRow = null;
    }


    private native int setResultInfo(long jniObject,
				int[] kvValLen, int[] kvValOffset,
				int[] kvQualLen, int[] kvQualOffset,
				int[] kvFamLen, int[] kvFamOffset,
  				long[] timestamp, 
				byte[][] kvBuffer, byte[][] rowIDs,
				int[] kvsPerRow, int numCellsReturned);
   static {
     System.loadLibrary("executor");
   }
    
 
}
