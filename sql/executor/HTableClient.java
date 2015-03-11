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
import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;
import java.nio.ByteBuffer;
import java.nio.LongBuffer;
import java.nio.ByteOrder;

import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.hbase.KeyValue;
import org.apache.hadoop.hbase.Cell;
import org.apache.hadoop.hbase.client.Delete;
import org.apache.hadoop.hbase.client.Get;
import org.apache.hadoop.hbase.client.Put;
import org.apache.hadoop.hbase.client.Result;
import org.apache.hadoop.hbase.client.ResultScanner;
import org.apache.hadoop.hbase.client.Scan;
import org.apache.hadoop.hbase.client.HConnection;
import org.apache.hadoop.hbase.util.Bytes;
import org.apache.hadoop.hbase.client.coprocessor.AggregationClient;
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

import org.apache.hadoop.hbase.client.TableSnapshotScanner;
import org.apache.hadoop.hbase.client.HBaseAdmin;
import org.apache.hadoop.hbase.HConstants;
import org.apache.hadoop.hbase.TableName;
import org.apache.hadoop.fs.FileSystem;
import org.apache.hadoop.fs.Path;
import org.apache.hadoop.fs.FileUtil;
import java.util.UUID;
import java.security.InvalidParameterException;

public class HTableClient {
	private boolean useTRex;
	private boolean useTRexScanner;
	private String tableName;

	private ResultScanner scanner = null;
        private ScanHelper scanHelper = null;
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
        static ExecutorService executorService = null;
        Future future = null;
	boolean preFetch = false;

	long jniObject = 0;
	SnapshotScanHelper snapHelper = null;

	 class SnapshotScanHelper
	 {
	   Path snapRestorePath = null;
	   HBaseAdmin admin  = null;
	   Configuration conf = null;
	   SnapshotDescription snpDesc = null;
	   String tmpLocation = null;
	   FileSystem fs  = null;

	   SnapshotScanHelper( Configuration cnfg , String tmpLoc, String snapName) 
	       throws IOException
	   {
	     conf = cnfg;
	     admin = new HBaseAdmin(conf);
	     tmpLocation = tmpLoc;
	     setSnapshotDescription(snapName);
	     Path rootDir = new Path(conf.get(HConstants.HBASE_DIR));
	     fs = rootDir.getFileSystem(conf);
	     setSnapRestorePath();
	   }

	   String getTmpLocation()
	   {
	     return tmpLocation;
	   }
	   String getSnapshotName()
	   {
	     if (snpDesc == null)
	       return null;
	     return snpDesc.getName();
	   }
	   void setSnapRestorePath() throws IOException
	   {
	     String restoreDirStr = tmpLocation + getSnapshotDescription().getName(); ;
	     snapRestorePath = new Path(restoreDirStr);
	     snapRestorePath = snapRestorePath.makeQualified(fs.getUri(), snapRestorePath);
	   }
	   Path getSnapRestorePath() throws IOException
	   {
	     return snapRestorePath;
	   }
	   boolean snapshotExists() throws IOException
	   {
	     if (logger.isTraceEnabled()) logger.trace("[Snapshot Scan] SnapshotScanHelper.snapshotExists() called. ");
	     return !admin.listSnapshots(snpDesc.getName()).isEmpty();
	   }
	   void deleteSnapshot() throws IOException
	   {
	     if (logger.isTraceEnabled()) logger.trace("[Snapshot Scan] SnapshotScanHelper.deleteSnapshot() called. ");
	     if (snapshotExists())
	     {
	       admin.deleteSnapshot(snpDesc.getName());
	       if (logger.isTraceEnabled()) logger.trace("[Snapshot Scan] SnapshotScanHelper.deleteSnapshot(). snapshot: " + snpDesc.getName() + " deleted.");
	     }
	     else
	     {
	       if (logger.isTraceEnabled()) logger.trace("[Snapshot Scan] SnapshotScanHelper.deleteSnapshot(). snapshot: " + snpDesc.getName() + " does not exist.");
	     }
	   }
	   void deleteRestorePath() throws IOException
	   {
	     if (logger.isTraceEnabled()) logger.trace("[Snapshot Scan] SnapshotScanHelper.deleteRestorePath() called. ");
	     if (fs.exists(snapRestorePath))
	     {
	       fs.delete(snapRestorePath, true);
	       if (logger.isTraceEnabled()) logger.trace("[Snapshot Scan] SnapshotScanHelper.deleteRestorePath(). restorePath: " + snapRestorePath + " deleted.");
	     }
	     else
	     {
	       if (logger.isTraceEnabled()) logger.trace("[Snapshot Scan] SnapshotScanHelper.deleteRestorePath(). restorePath: " + snapRestorePath  + " does not exist.");
	     }
	   }
	   
	   void createTableSnapshotScanner(int timeout, int slp, long nbre, Scan scan) throws InterruptedException
	   {
	     if (logger.isTraceEnabled()) logger.trace("[Snapshot Scan] SnapshotScanHelper.createTableSnapshotScanner() called. ");
	     int xx=0;
	     while (xx < timeout)
	     {
         xx++;
	       scanner = null;
	       try
	       {
	         scanner = new TableSnapshotScanner(table.getConfiguration(), snapHelper.getSnapRestorePath(), snapHelper.getSnapshotName(), scan);
	       }
	       catch(IOException e )
	       {
	         if (logger.isTraceEnabled()) logger.trace("[Snapshot Scan] SnapshotScanHelper.createTableSnapshotScanner(). espNumber: " + nbre  + 
	             " snapshot " + snpDesc.getName() + " TableSnapshotScanner Exception :" + e);
	         Thread.sleep(slp);
	         continue;
	       }
	       if (logger.isTraceEnabled()) logger.trace("[Snapshot Scan] SnapshotScanHelper.createTableSnapshotScanner(). espNumber: " + 
	           nbre + " snapshot " + snpDesc.getName() +  " TableSnapshotScanner Done - Scanner:" + scanner );
	       break;
	     }
	   }
	   void setSnapshotDescription( String snapName)
	   {
       if (snapName == null )
         throw new InvalidParameterException ("snapshotName is null.");
       
	     SnapshotDescription.Builder builder = SnapshotDescription.newBuilder();
	     builder.setTable(Bytes.toString(table.getTableName()));
	     builder.setName(snapName);
	     builder.setType(SnapshotDescription.Type.FLUSH);
	     snpDesc = builder.build();
	   }
	   SnapshotDescription getSnapshotDescription()
	   {
	     return snpDesc;
	   }

	   public void release() throws IOException
	   {
	     if (admin != null)
	     {
	       admin.close();
	       admin = null;
	     }
	   }
	 }

	class ScanHelper implements Callable {
            public Result[] call() throws Exception {
                return scanner.next(numRowsCached);
            }
        }
	 
	static Logger logger = Logger.getLogger(HTableClient.class.getName());;

        static public  byte[] getFamily(byte[] qc) {
	   byte[] family = null;

	   if (qc != null && qc.length > 0) {
	       int pos = Bytes.indexOf(qc, (byte) ':');
	       if (pos == -1) 
	          family = Bytes.toBytes("cf1");
	       else
	          family = Arrays.copyOfRange(qc, 0, pos);
           }	
	   return family;
	}

        static public byte[] getName(byte[] qc) {
	   byte[] name = null;

	   if (qc != null && qc.length > 0) {
	      int pos = Bytes.indexOf(qc, (byte) ':');
	      if (pos == -1) 
	         name = qc;
	      else
	         name = Arrays.copyOfRange(qc, pos + 1, qc.length);
	   }	
	   return name;
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
	    boolean inPreFetch,
	    boolean useSnapshotScan,
	    int snapTimeout,
	    String snapName,
	    String tmpLoc,
	    int espNum) 
	        throws IOException, Exception {
	  if (logger.isTraceEnabled()) logger.trace("Enter startScan() " + tableName + " txid: " + transID+ " CacheBlocks: " + cacheBlocks + " numCacheRows: " + numCacheRows + " Bulkread: " + useSnapshotScan);

	  Scan scan;

	  if (startRow != null && startRow.toString() == "")
	    startRow = null;
	  if (stopRow != null && stopRow.toString() == "")
	    stopRow = null;

	  if (startRow != null && stopRow != null)
	    scan = new Scan(startRow, stopRow);
	  else
	    scan = new Scan();

		if (cacheBlocks == true) {
	    scan.setCacheBlocks(true);
			// Disable block cache for full table scan
			if (startRow == null && stopRow == null)
				scan.setCacheBlocks(false);
		}
	  else
	    scan.setCacheBlocks(false);
		
	  scan.setCaching(numCacheRows);
	  numRowsCached = numCacheRows;
	  if (columns != null) {
	    numColsInScan = columns.length;
	    for (int i = 0; i < columns.length ; i++) {
	      byte[] col = (byte[])columns[i];
	      scan.addColumn(getFamily(col), getName(col));
	    }
	  }
	  else
	    numColsInScan = 0;
	  if (colNamesToFilter != null) {
	    FilterList list = new FilterList(FilterList.Operator.MUST_PASS_ALL);

	    for (int i = 0; i < colNamesToFilter.length; i++) {
	      byte[] colName = (byte[])colNamesToFilter[i];
	      byte[] coByte = (byte[])compareOpList[i];
	      byte[] colVal = (byte[])colValuesToCompare[i];

	      if ((coByte == null) || (colVal == null)) {
	        return false;
	      }

	      String coStr = new String(coByte);
	      CompareOp co = CompareOp.valueOf(coStr);

	      SingleColumnValueFilter filter1 = 
	          new SingleColumnValueFilter(getFamily(colName), getName(colName), 
	              co, colVal);
	      list.addFilter(filter1);
	    }

	    if (samplePercent > 0.0f)
	      list.addFilter(new RandomRowFilter(samplePercent));
	    scan.setFilter(list);
	  } else if (samplePercent > 0.0f) {
	    scan.setFilter(new RandomRowFilter(samplePercent));
	  }

	  if (!useSnapshotScan || transID != 0)
	  {
	    if (useTRexScanner && (transID != 0)) {
	      scanner = table.getScanner(transID, scan);
	    } else {
	      scanner = table.getScanner(scan);
	    }
	    if (logger.isTraceEnabled()) logger.trace("startScan(). After getScanner. Scanner: " + scanner);
	  }
	  else
	  {
	    snapHelper = new SnapshotScanHelper(table.getConfiguration(), tmpLoc,snapName);

	    if (logger.isTraceEnabled()) 
	      logger.trace("[Snapshot Scan] HTableClient.startScan(). useSnapshotScan: " + useSnapshotScan + 
	                   " espNumber: " + espNum + 
	                   " tmpLoc: " + snapHelper.getTmpLocation() + 
	                   " snapshot name: " + snapHelper.getSnapshotName());
	    
	    if (!snapHelper.snapshotExists())
	      throw new Exception ("Snapshot " + snapHelper.getSnapshotName() + " does not exist.");

	    snapHelper.createTableSnapshotScanner(snapTimeout, 5, espNum, scan);
	    if (scanner==null)
	      throw new Exception("Cannot create Table Snapshot Scanner");
	  }
    
	  preFetch = inPreFetch;
	  if (preFetch)
	  {
	    scanHelper = new ScanHelper(); 
            future = executorService.submit(scanHelper);
	  }

	  if (logger.isTraceEnabled()) logger.trace("Exit startScan().");
	  return true;
	}

	public boolean startGet(long transID, byte[] rowID, 
                     Object[] columns,
		     long timestamp) throws IOException {

	    if (logger.isTraceEnabled()) logger.trace("Enter startGet(" + tableName + 
			     " #cols: " + ((columns == null) ? 0:columns.length ) +
			     " rowID: " + new String(rowID));

		Get get = new Get(rowID);
		if (columns != null)
		{
			for (int i = 0; i < columns.length; i++) {
				byte[] col = (byte[]) columns[i];
				get.addColumn(getFamily(col), getName(col));
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
		getResultSet = new Result[1];
		getResultSet[0] = getResult;
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
			Object[] columns, long timestamp)
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
				for (Get get : listOfGets)
					get.addColumn(getFamily(col), getName(col));
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
		return true;
	}

	public int fetchRows() throws IOException, 
			InterruptedException, ExecutionException {
		int rowsReturned = 0;

		if (logger.isTraceEnabled()) logger.trace("Enter fetchRows(). Table: " + tableName);
		if (getResultSet != null)
		{
			rowsReturned = pushRowsToJni(getResultSet);
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
				rowsReturned = pushRowsToJni(result);
				future = null;
				if ((rowsReturned <= 0 || rowsReturned < numRowsCached))
					return rowsReturned;
                                future = executorService.submit(scanHelper);
			}
			else
			{
				result = scanner.next(numRowsCached);
				rowsReturned = pushRowsToJni(result);
			}
			return rowsReturned;
		}
	}

	protected int pushRowsToJni(Result[] result) 
			throws IOException {
		if (result == null || result.length == 0)
			return 0; 
		int rowsReturned = result.length;
		int numTotalCells = 0;
		if (numColsInScan == 0)
		{
			for (int i = 0; i < result.length; i++) {	
				numTotalCells += result[i].size();
			}
		}
		else
		// There can be maximum of 2 versions per kv
		// So, allocate place holder to keep cell info
		// for that many KVs
			numTotalCells = 2 * rowsReturned * numColsInScan;
		int numColsReturned;
		Cell[] kvList;
		Cell kv;

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
				kvList = result[rowNum].rawCells();
			}
			else
			{
				rowIDs[rowNum] = null;
				kvList = null;
			}
 			if (kvList == null)
				numColsReturned = 0; 
			else
				numColsReturned = kvList.length;
			if ((cellNum + numColsReturned) > numTotalCells)
				throw new IOException("Insufficient cell array pre-allocated");
			kvsPerRow[rowNum] = numColsReturned;
			for (int colNum = 0 ; colNum < numColsReturned ; colNum++, cellNum++)
			{ 
				kv = kvList[colNum];
				kvValLen[cellNum] = kv.getValueLength();
				kvValOffset[cellNum] = kv.getValueOffset();
				kvQualLen[cellNum] = kv.getQualifierLength();
				kvQualOffset[cellNum] = kv.getQualifierOffset();
				kvFamLen[cellNum] = kv.getFamilyLength();
				kvFamOffset[cellNum] = kv.getFamilyOffset();
				kvTimestamp[cellNum] = kv.getTimestamp();
				kvBuffer[cellNum] = kv.getValueArray();
			}
		}
		int cellsReturned = cellNum++;
		setResultInfo(jniObject, kvValLen, kvValOffset,
			kvQualLen, kvQualOffset, kvFamLen, kvFamOffset,
			kvTimestamp, kvBuffer, rowIDs, kvsPerRow, cellsReturned);
		return rowsReturned;	
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
					del.deleteColumns(getFamily(col), getName(col));
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
				family = getFamily(columnToCheck);
				qualifier = getName(columnToCheck);
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
		short colNameLen;
                int colValueLen;
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
			colValueLen = bb.getInt();	
			colValue = new byte[colValueLen];
			bb.get(colValue, 0, colValueLen);
			put.add(getFamily(colName), getName(colName), colValue); 
			if (checkAndPut && colIndex == 0) {
				family = getFamily(colName);
				qualifier = getName(colName);
			} 
		}
		if (columnToCheck != null && columnToCheck.length > 0) {
			family = getFamily(columnToCheck);
			qualifier = getName(columnToCheck);
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
		short colNameLen;
                int colValueLen;
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
				colValueLen = bbRows.getInt();	
				colValue = new byte[colValueLen];
				bbRows.get(colValue, 0, colValueLen);
				put.add(getFamily(colName), getName(colName), colValue); 
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
                    long rowCount = 0;

                    if (transID > 0) {
		      TransactionalAggregationClient aggregationClient = 
                          new TransactionalAggregationClient(customConf);
		      Scan scan = new Scan();
		      scan.addFamily(colFamily);
		      scan.setCacheBlocks(false);
		      final ColumnInterpreter<Long, Long, EmptyMsg, LongMsg, LongMsg> ci =
			new LongColumnInterpreter();
		      byte[] tname = getTableName().getBytes();
		      rowCount = aggregationClient.rowCount(transID, 
                        org.apache.hadoop.hbase.TableName.valueOf(getTableName()),
                        ci,
                        scan);
                    }
                    else {
		      AggregationClient aggregationClient = 
                          new AggregationClient(customConf);
		      Scan scan = new Scan();
		      scan.addFamily(colFamily);
		      scan.setCacheBlocks(false);
		      final ColumnInterpreter<Long, Long, EmptyMsg, LongMsg, LongMsg> ci =
			new LongColumnInterpreter();
		      byte[] tname = getTableName().getBytes();
		      rowCount = aggregationClient.rowCount( 
                        org.apache.hadoop.hbase.TableName.valueOf(getTableName()),
                        ci,
                        scan);
                    }

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

	public boolean release(boolean cleanJniObject) throws IOException {

           boolean retcode = false;
          // Complete the pending IO
           if (future != null) {
              try {
                 future.get(30, TimeUnit.SECONDS);
              } catch(TimeoutException | InterruptedException e) {
		  logger.error("Pre-fetch Thread is Cancelled, " + e);
                  retcode = true;
                  future.cancel(true); // Interrupt the thread
              } catch (ExecutionException ee)
              {
              }
              future = null;
          }
	  if (table != null)
	    table.flushCommits();
	  if (scanner != null) {
	    scanner.close();
	    scanner = null;
	  }
	  if (snapHelper !=null)
	  {
	    snapHelper.release();
	    snapHelper = null;
	  }
	  cleanScan();		
	  getResultSet = null;
	  if (cleanJniObject) {
	    if (jniObject != 0)
	      cleanup(jniObject);
            tableName = null;
	  }
          scanHelper = null;
	  jniObject = 0;
	  return retcode;
	}

	public boolean close(boolean clearRegionCache, boolean cleanJniObject) throws IOException {
           if (logger.isTraceEnabled()) logger.trace("Enter close() " + tableName);
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

   	 private void setJniObject(long inJniObject) {
		jniObject = inJniObject;
	}    

    private native int setResultInfo(long jniObject,
				int[] kvValLen, int[] kvValOffset,
				int[] kvQualLen, int[] kvQualOffset,
				int[] kvFamLen, int[] kvFamOffset,
  				long[] timestamp, 
				byte[][] kvBuffer, byte[][] rowIDs,
				int[] kvsPerRow, int numCellsReturned);

   private native void cleanup(long jniObject);
 
   static {
     executorService = Executors.newCachedThreadPool();
     System.loadLibrary("executor");
   }
}
