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

import org.apache.log4j.Logger;

import org.apache.hadoop.hbase.client.coprocessor.AggregationClient;
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
	private String tableName;

	private ResultScanner scanner = null;
	private ResultIterator resultIterator = null;
	KeyValue lastFetchedCell = null;
	Result lastFetchedRow = null;
	Result[] getResultSet = null;
	int getResultSetPos;
	String lastError;
        RMInterface table = null;
        ByteArrayList coprocAggrResult = null;
        private boolean writeToWAL = false;


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
		logger.debug("Enter HTableClient::setWriteBufferSize, size  : " + writeBufferSize);
	    table.setWriteBufferSize(writeBufferSize);
	    return true;
	  }
	 public long getWriteBufferSize() {
		 logger.debug("Enter HTableClient::getWriteBufferSize, size return : " + table.getWriteBufferSize());
		 return table.getWriteBufferSize();
	 }
	public boolean setWriteToWAL(boolean v) {
		logger.debug("Enter HTableClient::setWriteToWALL, size  : " + v);
	    writeToWAL = v;
	    return true;
	  }
 
	public boolean init(String tblName, Configuration config, 
              boolean useTRex) throws IOException 
        {
	    logger.debug("Enter HTableClient::init, tableName: " + tblName);
	    this.useTRex = useTRex;
	    tableName = tblName;

	    config.set("hbase.regionserver.class", "org.apache.hadoop.hbase.ipc.TransactionalRegionInterface");
	    config.set("hbase.regionserver.impl", "org.apache.hadoop.hbase.regionserver.transactional.TransactionalRegionServer");
	    config.set("hbase.hregion.impl", "org.apache.hadoop.hbase.regionserver.transactional.TransactionalRegion");
	    config.set("hbase.hlog.splitter.impl", "org.apache.hadoop.hbase.regionserver.transactional.THLogSplitter");
	    table = new RMInterface(config, tblName);
	    logger.debug("Exit HTableClient::init, table object: " + table);
	    return true;
	}

	public String getLastError() {
		return lastError;
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

        void resetAutoFlush()
        {
            table.setAutoFlush(true, true);
        }
	public boolean startScan(long transID, byte[] startRow, byte[] stopRow,
				 ByteArrayList columns, long timestamp,
				 boolean cacheBlocks, int numCacheRows,
				 ByteArrayList colNamesToFilter, 
				 ByteArrayList compareOpList, 
				 ByteArrayList colValuesToCompare,
				 float samplePercent) 
           throws IOException {

	    logger.trace("Enter startScan() " + tableName);
	    //	    logger.error("Enter startScan() " + tableName + " txid: " + transID);

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

			if (columns != null) {
				for (byte[] col : columns) {
					QualifiedColumn qc = new QualifiedColumn(col);
					scan.addColumn(qc.getFamily(), qc.getName());
				}
			}
			
			if (colNamesToFilter != null) 
			{
			    FilterList list = new FilterList(FilterList.Operator.MUST_PASS_ALL);

			    for (int i = 0; i < colNamesToFilter.size(); i++) 
				{
				    byte[] colName = colNamesToFilter.get(i);
				    QualifiedColumn qc = new QualifiedColumn(colName);
				
				    byte[] coByte = compareOpList.get(i);
				    byte[] colVal = colValuesToCompare.get(i);

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

			if (useTRex && (transID != 0)) {
			    scanner = table.getScanner(transID, scan);
			} else {
			    scanner = table.getScanner(scan);
			}
			logger.trace("startScan(). After getScanner. Scanner: " + scanner);
			resultIterator = new ResultIterator(scanner);
		logger.trace("Exit startScan().");
		return true;
	}

	public boolean startGet(long transID, byte[] rowID, 
                     ByteArrayList columns,
		     long timestamp) throws IOException {

	    logger.trace("Enter startGet(" + tableName + " rowID: " + new String(rowID));

			Get get = new Get(rowID);
			for (byte[] col : columns) {
			    logger.trace("startGet, col: " + new String(col));
				QualifiedColumn qc = new QualifiedColumn(col);
				get.addColumn(qc.getFamily(), qc.getName());
			}

			Result getResult;
			if (useTRex && (transID != 0)) {
			    getResult = table.get(transID, get);
			} else {
			    getResult = table.get(get);
			}
			if (getResult == null) {
				logger.trace("Exit 1 startGet.  Returning empty.");
				setLastError(null);
				return false;
			}

			logger.trace("startGet, result length: " + getResult.size());
			logger.trace("startGet, result: " + getResult);
			lastFetchedRow = getResult;
			resultIterator = new ResultIterator(getResult);
		logger.trace("Exit 2 startGet.");
		return true;
	}

	// The TransactionalTable class is missing the batch get operation,
	// so work around it.
    private Result[] batchGet(long transactionID, List<Get> gets)
			throws IOException {
		logger.trace("Enter batchGet(multi-row) " + tableName);
		Result [] results = new Result[gets.size()];
		int i=0;
		for (Get g : gets) {
			Result r = table.get(transactionID, g);
			if (r != null)
				results[i++] = r;
		}
	
		return results;
	}

	public boolean startGet(long transID, ByteArrayList rows,
			ByteArrayList columns, long timestamp) 
                        throws IOException {

		logger.trace("Enter startGet(multi-row) " + tableName);

			List<Get> listOfGets = new ArrayList<Get>();
			for (byte[] rowID : rows) {
				Get get = new Get(rowID);
				listOfGets.add(get);
			}
			for (byte[] col : columns) {
				QualifiedColumn qc = new QualifiedColumn(col);
				for (Get get : listOfGets)
					get.addColumn(qc.getFamily(), qc.getName());
			}
			
			if (useTRex && (transID != 0)) {
				getResultSet = batchGet(transID, listOfGets);
			} else {
				getResultSet = table.get(listOfGets);
			}
			if (getResultSet.length == 0)
				getResultSet = null;
			getResultSetPos = 0;
			lastFetchedRow = getResultSet[getResultSetPos];
			getResultSetPos++;
		return true;
	}

	public boolean scanFetch() throws IOException {
		logger.trace("Enter scanFetch() " + tableName);
		if (resultIterator == null) {
			String err = "  scanFetch() called before scanOpen().";
			logger.error(err);
			setLastError(err);
			return false;
		}

			lastFetchedCell = resultIterator.nextCell();
		if (lastFetchedCell == null) {
			setLastError(null);
			return false; // Done.
		}

		return true;
	}

	public boolean getFetch() throws IOException {
		logger.trace("Enter getFetch() " + tableName);
		return scanFetch();
	}

	public boolean fetchNextRow() throws IOException {
	    logger.trace("Enter fetchNextRow(). Table: " + tableName);
		if (scanner == null) {
			String err = "  fetchNextRow() called before scanOpen().";
			logger.error(err);
			setLastError(err);
			return false;
		}

		    logger.trace("fetchNextRow(). Calling scanner.next");
			lastFetchedRow = scanner.next();
			if (lastFetchedRow == null) {
			    logger.trace("Exit 1 fetchNextRow().");
				setLastError(null);
				return false;
			}
		logger.trace("Exit 2 fetchNextRow().");
		return true;
	}

	public ResultKeyValueList fetchRowVec() { 
	    logger.trace("Enter fetchRowVec() " + tableName);

		if (lastFetchedRow == null) {
			logger.trace("Exit 1 fetchRowVec.  Returning empty.");
			return null;
		}
		ResultKeyValueList result = new ResultKeyValueList(lastFetchedRow);
		lastFetchedRow = null;

		if (getResultSet != null) {
			if (getResultSetPos == getResultSet.length) {
				getResultSet = null;
			} else {
			        lastFetchedRow = getResultSet[getResultSetPos];
			        getResultSetPos++;
			}
		}

		logger.trace("Exit fetchRowVec()" + tableName + ". Result Size: " + result.getSize());
		return result;
	}

	public KeyValue getLastFetchedCell() {
		logger.trace("Enter getLastFetchedCell() ");
		if (lastFetchedCell == null)
			logger.trace("  Returning empty.");
		return lastFetchedCell;
	}

	public boolean deleteRow(long transID, byte[] rowID, 
				 ByteArrayList columns,
				 long timestamp) throws IOException {

		logger.trace("Enter deleteRow(" + new String(rowID) + ", "
			     + timestamp + ") ");

			Delete del;
			if (timestamp == -1)
				del = new Delete(rowID);
			else
				del = new Delete(rowID, timestamp);

			if (columns != null) {
				for (byte[] col : columns) {
				    QualifiedColumn qc = new QualifiedColumn(col);
				    del.deleteColumns(qc.getFamily(), qc.getName());
				}
			}

			if (useTRex && (transID != 0)) {
				table.delete(transID, del);
			} else {
				table.delete(del);
			}
		logger.trace("Exit deleteRow");
		return true;
	}

    public boolean deleteRows(long transID, RowsToInsert rows,
			      long timestamp) throws IOException {

	        logger.trace("Enter deleteRows() ");

		    List<Delete> listOfDeletes = new ArrayList<Delete>();
		    listOfDeletes.clear();
		    for (RowsToInsert.RowInfo row : rows) {

			byte[] rowId = row.rowId;
			Delete del;
			if (timestamp == -1)
			    del = new Delete(rowId);
			else
			    del = new Delete(rowId, timestamp);
			
			for (RowsToInsert.ColToInsert col : row.columns) {
			    QualifiedColumn qc = new QualifiedColumn(col.qualName);
			    del.deleteColumns(qc.getFamily(), qc.getName());
			}
			
			listOfDeletes.add(del);
		    }

		    if (useTRex && (transID != 0)) {

			/* TRX doesnt support list of deletes yet.

			transState = new TransactionState(transID);
			table.delete(transState, listOfDeletes);
			*/
			table.delete(listOfDeletes);
		    } else {
			table.delete(listOfDeletes);
		    }
		logger.trace("Exit deleteRow");
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

		logger.trace("Enter checkAndDeleteRow(" + new String(rowID) + ", "
			     + new String(columnToCheck) + ", " + new String(colValToCheck) + ", " + timestamp + ") ");

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

	public boolean insertRow(long transID, byte[] rowID, RowToInsert columns,
			long timestamp) throws IOException {

		logger.trace("Enter insertRow() ");

			Put put = new Put(rowID);

			for (RowToInsert.ColToInsert col : columns) {
				QualifiedColumn qc = new QualifiedColumn(col.qualName);
				put.add(qc.getFamily(), qc.getName(), col.colValue);
			}

			if (useTRex && (transID != 0)) {
				table.put(transID, put);
			} else {
				table.put(put);
			}
		return true;
	}

        public boolean insertRows(long transID, RowsToInsert rows,
                       long timestamp, boolean autoFlush)
                     throws IOException {

	        logger.trace("Enter insertRows() ");

                    List<Put> listOfPuts = new ArrayList<Put>();
                    listOfPuts.clear();
                    for (RowsToInsert.RowInfo row : rows) {

                        byte[] rowId = row.rowId;
                        Put put = new Put(rowId);

                        for (RowsToInsert.ColToInsert col : row.columns) {
                            QualifiedColumn qc = new QualifiedColumn(col.qualName);
                            put.add(qc.getFamily(), qc.getName(), col.colValue);
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
                         RowToInsert columns, 
			 long timestamp) throws IOException {
	    logger.trace("Enter checkAndInsertRow(), transID: " + transID + " table: " + tableName);
			Put put = new Put(rowID);

			int colNum = 0;
			byte[] family = null;
			byte[] qualifier = null;

			for (RowToInsert.ColToInsert col : columns)
			{
				QualifiedColumn qc = new QualifiedColumn(col.qualName);
				put.add(qc.getFamily(), qc.getName(), col.colValue);

				if (colNum == 0)
				    {
					family = qc.getFamily();
					qualifier = qc.getName();
				    }

				colNum++;
			}
			
			boolean res;
			if (useTRex && (transID != 0)) {
			    res = table.checkAndPut(transID, rowID, family, qualifier, null, put);
			} else {
			    res = table.checkAndPut(rowID, family, qualifier, null, put);
		    
			}

			if (res == false)
			    return false; // dup row
		return true;
	}

    public boolean checkAndUpdateRow(long transID, byte[] rowID, 
             RowToInsert columns, byte[] columnToCheck, byte[] colValToCheck,
             long timestamp) throws IOException, Throwable  {

		logger.trace("Enter checkAndUpdateRow(" + new String(rowID) + ", "
			     + new String(columnToCheck) + ", " + new String(colValToCheck) + ", " + timestamp + ") ");

			Put put = new Put(rowID);

			int colNum = 0;
			byte[] family = null;
			byte[] qualifier = null;

			for (RowToInsert.ColToInsert col : columns)
			{
				QualifiedColumn qc = new QualifiedColumn(col.qualName);
				put.add(qc.getFamily(), qc.getName(), col.colValue);

				if (colNum == 0)
				    {
					family = qc.getFamily();
					qualifier = qc.getName();
				    }

				colNum++;
			}

			if (columnToCheck.length > 0) {
				QualifiedColumn qc = new QualifiedColumn(columnToCheck);

				family = qc.getFamily();
				qualifier = qc.getName();
			}
			
			boolean res;
			if (useTRex && (transID != 0)) {
			    res = table.checkAndPut(transID, rowID, family, qualifier, colValToCheck, put);
			}
			else  {
			    res = table.checkAndPut(rowID, family, qualifier, colValToCheck, put);
			}

			if (res == false)
			    return false;
		return true;
	}

    public boolean coProcAggr(long transID, int aggrType, byte[] startRowID, 
              byte[] stopRowID, byte[] colFamily, byte[] colName, 
              boolean cacheBlocks, int numCacheRows) 
                          throws IOException, Throwable {

		    Configuration customConf = table.getConfiguration();

		    TransactionalAggregationClient aggregationClient = new TransactionalAggregationClient(customConf);
		    Scan scan = new Scan();
		    scan.addFamily(colFamily);
		    final ColumnInterpreter<Long, Long> ci = new LongColumnInterpreter();
		    byte[] tname = getTableName().getBytes();
		    long rowCount = aggregationClient.rowCount(transID, tname, ci, scan);

		    coprocAggrResult = new ByteArrayList();

		    byte[] rcBytes = ByteBuffer.allocate(8).order(ByteOrder.LITTLE_ENDIAN).putLong(rowCount).array();
		    coprocAggrResult.add(rcBytes);

		    /*
		    ByteBuffer byteBuffer = ByteBuffer.wrap(rcBytes);
		    LongBuffer longBuffer = byteBuffer.asLongBuffer();
		    long l[] = new long[longBuffer.capacity()];
		    longBuffer.get(l);

		    System.out.println(rowCount);
		    System.out.println(rcBytes);
		    System.out.println(l[0]);

		      scan.addColumn(TEST_FAMILY, TEST_QUALIFIER);
		      final ColumnInterpreter<Long, Long> ci = new LongColumnInterpreter();
		      SingleColumnValueFilter scvf = new SingleColumnValueFilter(TEST_FAMILY,
		      TEST_QUALIFIER, CompareOp.EQUAL,
		      Bytes.toBytes(4l));
		      scan.setFilter(scvf);
		    */
		return true;
    }

	public ByteArrayList coProcAggrGetResult() { 
		logger.trace("HBaseClient.coProcAggrGetResult() ");

		if (coprocAggrResult == null) {
			logger.trace("  Returning empty.");
			return null;
		}

		return coprocAggrResult;
	}

	public boolean flush() throws IOException {
			if (scanner != null) {
				scanner.close();
				scanner = null;
			}
			
			if (table != null)
				table.flushCommits();
				
			return true;
	}

	public boolean close(boolean clearRegionCache) throws IOException {
           logger.trace("Enter close() ");
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
	    logger.trace("Enter getEndKeys() " + tableName);
            ByteArrayList result = new ByteArrayList();
            if (table == null) {
                return null;
            }
            byte[][] htableResult = table.getEndKeys();

            // transfer the HTable result to ByteArrayList
            for (int i=0; i<htableResult.length; i++ ) {
                logger.trace("Inside getEndKeys(), result[i]: " + 
                             htableResult[i]);
                logger.trace("Inside getEndKeys(), result[i]: " + 
                             new String(htableResult[i]));
                result.add(htableResult[i]);
            }

            logger.trace("Exit getEndKeys(), result size: " + result.getSize());
            return result;
	}

    public ByteArrayList getStartKeys() throws IOException {
        logger.trace("Enter getStartKeys() " + tableName);
        ByteArrayList result = new ByteArrayList();
        if (table == null) {
            return null;
        }
        byte[][] htableResult = table.getStartKeys();

        // transfer the HTable result to ByteArrayList
        for (int i=0; i<htableResult.length; i++ ) {
            logger.trace("Inside getStartKeys(), result[i]: " + 
                         htableResult[i]);
            logger.trace("Inside getStartKeys(), result[i]: " + 
                         new String(htableResult[i]));
            result.add(htableResult[i]);
        }

        logger.trace("Exit getStartKeys(), result size: " + result.getSize());
        return result;
    }
    
 
}
