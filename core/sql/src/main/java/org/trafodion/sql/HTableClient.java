// @@@ START COPYRIGHT @@@
//
// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.
//
// @@@ END COPYRIGHT @@@

package org.trafodion.sql;
import org.trafodion.sql.*;

import java.io.IOException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.HashMap;
import java.util.Map;
import java.util.NavigableSet;

import org.apache.commons.codec.binary.Hex;
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
import org.apache.hadoop.hbase.HConstants;
import org.apache.hadoop.hbase.KeyValue;
import org.apache.hadoop.hbase.Cell;
import org.apache.hadoop.hbase.client.Delete;
import org.apache.hadoop.hbase.client.Get;
import org.apache.hadoop.hbase.client.Put;
import org.apache.hadoop.hbase.client.Result;
import org.apache.hadoop.hbase.client.ResultScanner;
import org.apache.hadoop.hbase.client.Scan;
import org.apache.hadoop.hbase.client.Connection;
import org.apache.hadoop.hbase.client.Table;
import org.apache.hadoop.hbase.util.Bytes;
import org.apache.hadoop.hbase.client.coprocessor.AggregationClient;
import org.apache.hadoop.hbase.client.transactional.RMInterface;
import org.apache.hadoop.hbase.client.transactional.TransactionalAggregationClient;
import org.apache.hadoop.hbase.client.transactional.TransactionalTable;
import org.apache.hadoop.hbase.client.transactional.TransactionState;

import org.apache.log4j.Logger;

// H98 coprocessor needs
import java.util.*;
import org.apache.hadoop.hbase.*;
import org.apache.hadoop.hbase.client.*;
import org.apache.hadoop.hbase.client.coprocessor.*;
import org.apache.hadoop.hbase.coprocessor.*;
import org.apache.hadoop.hbase.ipc.*;
import org.apache.hadoop.hbase.protobuf.generated.HBaseProtos.*;
import org.apache.hadoop.hbase.util.*;

//import org.apache.hadoop.hbase.client.coprocessor.AggregationClient;
import org.apache.hadoop.hbase.coprocessor.ColumnInterpreter;
import org.apache.hadoop.hbase.client.coprocessor.LongColumnInterpreter;

// classes to do hbase pushdown filtering
import org.apache.hadoop.hbase.filter.Filter;
import org.apache.hadoop.hbase.filter.SingleColumnValueFilter;
import org.apache.hadoop.hbase.filter.CompareFilter.CompareOp;
import org.apache.hadoop.hbase.filter.FilterList;
import org.apache.hadoop.hbase.filter.RandomRowFilter;
import org.apache.hadoop.hbase.filter.FamilyFilter;
import org.apache.hadoop.hbase.filter.QualifierFilter;
import org.apache.hadoop.hbase.filter.SingleColumnValueExcludeFilter;
import org.apache.hadoop.hbase.filter.BinaryComparator;
import org.apache.hadoop.hbase.filter.BinaryPrefixComparator;
import org.apache.hadoop.hbase.filter.ByteArrayComparable;
import org.apache.hadoop.hbase.filter.NullComparator;

import org.apache.hadoop.hbase.client.TableSnapshotScanner;
import org.apache.hadoop.hbase.client.Admin;
import org.apache.hadoop.hbase.HConstants;
import org.apache.hadoop.hbase.TableName;
import org.apache.hadoop.fs.FileSystem;
import org.apache.hadoop.fs.Path;
import org.apache.hadoop.fs.FileUtil;
import java.util.UUID;
import java.security.InvalidParameterException;

public class HTableClient {
	private static final int GET_ROW = 1;
	private static final int BATCH_GET = 2;
	private static final int SCAN_FETCH = 3;
	private boolean useTRex;
	private boolean useTRexScanner;
        private static boolean envUseTRex;
        private static boolean envUseTRexScanner;
	private String tableName;
        private static Connection connection;
	private ResultScanner scanner = null;
        private ScanHelper scanHelper = null;
	Result[] getResultSet = null;
        RMInterface table = null;
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
        byte[][] kvFamArray = null;
        byte[][] kvQualArray = null;
        static ExecutorService executorService = null;
        Future future = null;
	boolean preFetch = false;
	int fetchType = 0;
	long jniObject = 0;
	SnapshotScanHelper snapHelper = null;
        static boolean enableHbaseScanForSkipReadConflict;

	 class SnapshotScanHelper
	 {
	   Path snapRestorePath = null;
	   Admin admin  = null;
	   Configuration conf = null;
	   SnapshotDescription snpDesc = null;
	   String tmpLocation = null;
	   FileSystem fs  = null;

	   SnapshotScanHelper( Configuration cnfg , String tmpLoc, String snapName) 
	       throws IOException
	   {
             conf = cnfg;
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
             Admin admin = connection.getAdmin();
	     boolean retcode = !(admin.listSnapshots(snpDesc.getName()).isEmpty());
             admin.close();
             return retcode; 
	   }

	   void deleteSnapshot() throws IOException
	   {
	     if (logger.isTraceEnabled()) logger.trace("[Snapshot Scan] SnapshotScanHelper.deleteSnapshot() called. ");
	     if (snapshotExists())
	     {
               Admin admin = connection.getAdmin();
	       admin.deleteSnapshot(snpDesc.getName());
               admin.close();
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
	   
	   void createTableSnapshotScanner(int timeout, int slp, long nbre, Scan scan) throws InterruptedException, IOException
	   {
	     if (logger.isTraceEnabled()) logger.trace("[Snapshot Scan] SnapshotScanHelper.createTableSnapshotScanner() called. ");
	     int xx=0;
             IOException ioExc = null;
	     while (xx < timeout)
	     {
               xx++;
	       scanner = null;
	       try
	       {
                 ioExc = null;
	         scanner = new TableSnapshotScanner(connection.getConfiguration(), snapHelper.getSnapRestorePath(), snapHelper.getSnapshotName(), scan);
	       }
	       catch(IOException e )
	       {
                 ioExc = e;
	         if (logger.isTraceEnabled()) logger.trace("[Snapshot Scan] SnapshotScanHelper.createTableSnapshotScanner(). espNumber: " + nbre  + 
	             " snapshot " + snpDesc.getName() + " TableSnapshotScanner Exception :" + e);
	         Thread.sleep(slp);
	         continue;
	       }
	       if (logger.isTraceEnabled()) logger.trace("[Snapshot Scan] SnapshotScanHelper.createTableSnapshotScanner(). espNumber: " + 
	           nbre + " snapshot " + snpDesc.getName() +  " TableSnapshotScanner Done - Scanner:" + scanner );
	       break;
	     }
             if (ioExc != null)
                throw ioExc;
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
         if (logger.isTraceEnabled()) logger.trace("HTableClient.release(" + (tableName == null ? " tableName is null " : tableName) + ") called.");
	     if (admin != null)
	     {
	       admin.close();
	       admin = null;
	     }
	   }
	 }

        public HTableClient(Connection connection) {
           this.connection = connection;
        }

	class ScanHelper implements Callable {
            public Result[] call() throws IOException {
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
 
	public boolean init(String tblName,
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
 	         this.useTRex = envUseTRex;
	         this.useTRexScanner = envUseTRexScanner;
	    }
	    table = new RMInterface(tblName, connection);
	    if (logger.isDebugEnabled()) logger.debug("Exit HTableClient::init, useTRex: " + this.useTRex + ", useTRexScanner: "
	              + this.useTRexScanner + ", table object: " + table);
	    return true;
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

    private enum Op {
        EQUAL, EQUAL_NULL, NOT_EQUAL, NOT_EQUAL_NULL, LESS, LESS_NULL, LESS_OR_EQUAL, LESS_OR_EQUAL_NULL, GREATER, GREATER_NULL, 
        GREATER_OR_EQUAL, GREATER_OR_EQUAL_NULL, NO_OP, NO_OP_NULL,IS_NULL, IS_NULL_NULL, IS_NOT_NULL, IS_NOT_NULL_NULL, AND, OR};
        
    private Filter SingleColumnValueExcludeOrNotFilter(byte[] columnToFilter, 
                                                        CompareOp op,
                                                        ByteArrayComparable comparator, 
                                                        HashMap<String,Object> columnsToRemove, 
                                                        Boolean... filterIfMissing){
        Filter result;
        boolean fMissing = filterIfMissing.length>0?filterIfMissing[0]:false;//default to false 
        if ((columnsToRemove == null) || !columnsToRemove.containsKey(new String(columnToFilter))){
            result = new SingleColumnValueFilter(getFamily(columnToFilter), getName(columnToFilter), op, comparator);
            ((SingleColumnValueFilter)result).setFilterIfMissing(fMissing);
        }
        else{
            result= new SingleColumnValueExcludeFilter(getFamily(columnToFilter), getName(columnToFilter), op, comparator);
            ((SingleColumnValueExcludeFilter)result).setFilterIfMissing(fMissing);
        }
        return result;
    }
    
    // construct the hbase filter
    // optimizes for OR and AND associativity
    // optimizes for detection of a<? and a>? on nullable and non nullable column equivalent to a<>?
    // optimize for null check factorization (A not null and (A <op> ?)) or (A not null and A <op2> ?) -> A not null and (A <op> ? or A <op2> ?)
    //      this is an important optimzation for IN statement on non null column
    // uses the columnToRemove parametter to know if we need to use the SingleColumnValue Exclude or not method to limit returned columns
    
    private Filter constructV2Filter(Object[] colNamesToFilter, 
                                 Object[] compareOpList, 
                                 Object[] colValuesToCompare,
                                 HashMap<String,Object> columnsToRemove){
        LinkedList linkedList = new LinkedList();
        //populate the list with nodes in reverse polish notation order.
        int k=0;//column index
        int kk=0;//value index
        for (int i=1; i<compareOpList.length; i++){ // skip first one containing "V2" marker
            String opStr = new String((byte[])compareOpList[i]);
            switch(Op.valueOf(opStr)){
                
                case EQUAL:
                    linkedList.addLast(SingleColumnValueExcludeOrNotFilter(
                            (byte[])colNamesToFilter[k], 
                            CompareOp.EQUAL, 
                            new BinaryComparator((byte[])colValuesToCompare[kk]),
                            columnsToRemove
                            ));
                    k++;kk++;
                    break;
                case EQUAL_NULL:
                    linkedList.addLast(new FilterList(FilterList.Operator.MUST_PASS_ALL, //AND between if not null and the actual
                                           SingleColumnValueExcludeOrNotFilter(
                                                (byte[])colNamesToFilter[k],
                                                CompareOp.EQUAL, 
                                                new BinaryPrefixComparator(new byte[]{0x00}),//check for null indicator = 0 representing non null
                                                columnsToRemove,
                                                true    //filterIfMissing
                                                ),
                                           SingleColumnValueExcludeOrNotFilter(
                                                    (byte[])colNamesToFilter[k], 
                                                    CompareOp.EQUAL, 
                                                    new BinaryComparator((byte[])colValuesToCompare[kk]),
                                                    columnsToRemove)));
                    k++;kk++;
                    break;
                case NOT_EQUAL:
                    linkedList.addLast(SingleColumnValueExcludeOrNotFilter(
                            (byte[])colNamesToFilter[k], 
                            CompareOp.NOT_EQUAL, 
                            new BinaryComparator((byte[])colValuesToCompare[kk]),
                            columnsToRemove));
                    k++;kk++;
                    break;
                case NOT_EQUAL_NULL:
                    linkedList.addLast(new FilterList(FilterList.Operator.MUST_PASS_ALL, //AND between if not null and the actual
                            SingleColumnValueExcludeOrNotFilter(
                                    (byte[])colNamesToFilter[k],
                                    CompareOp.EQUAL, 
                                    new BinaryPrefixComparator(new byte[]{0x00}),//check for null indicator = 0 representing non null
                                    columnsToRemove,
                                    true), //filterIfMissing,
                            SingleColumnValueExcludeOrNotFilter(
                                    (byte[])colNamesToFilter[k], 
                                    CompareOp.NOT_EQUAL, 
                                    new BinaryComparator((byte[])colValuesToCompare[kk]),
                                    columnsToRemove)));
                    k++;kk++;
                    break;
                case LESS:
                    linkedList.addLast(SingleColumnValueExcludeOrNotFilter(
                            (byte[])colNamesToFilter[k], 
                            CompareOp.LESS, 
                            new BinaryComparator((byte[])colValuesToCompare[kk]),
                            columnsToRemove));
                    k++;kk++;
                    break;
                case LESS_NULL:
                    linkedList.addLast(new FilterList(FilterList.Operator.MUST_PASS_ALL, //AND between if not null and the actual
                            SingleColumnValueExcludeOrNotFilter(
                                    (byte[])colNamesToFilter[k],
                                    CompareOp.EQUAL, 
                                    new BinaryPrefixComparator(new byte[]{0x00}),//check for null indicator = 0 representing non null
                                    columnsToRemove,
                                    true), //filterIfMissing,
                            SingleColumnValueExcludeOrNotFilter(
                                    (byte[])colNamesToFilter[k], 
                                    CompareOp.LESS, 
                                    new BinaryComparator((byte[])colValuesToCompare[kk]),
                                    columnsToRemove)));
                    k++;kk++;
                    break;
                case LESS_OR_EQUAL:
                    linkedList.addLast(SingleColumnValueExcludeOrNotFilter(
                            (byte[])colNamesToFilter[k], 
                            CompareOp.LESS_OR_EQUAL, 
                            new BinaryComparator((byte[])colValuesToCompare[kk]),
                            columnsToRemove));
                    k++;kk++;
                    break;
                case LESS_OR_EQUAL_NULL:
                    linkedList.addLast(new FilterList(FilterList.Operator.MUST_PASS_ALL, //AND between if not null and the actual
                            SingleColumnValueExcludeOrNotFilter(
                                    (byte[])colNamesToFilter[k],
                                    CompareOp.EQUAL, 
                                    new BinaryPrefixComparator(new byte[]{0x00}),//check for null indicator = 0 representing non null
                                    columnsToRemove,
                                    true), //filterIfMissing,
                            SingleColumnValueExcludeOrNotFilter(
                                    (byte[])colNamesToFilter[k], 
                                    CompareOp.LESS_OR_EQUAL, 
                                    new BinaryComparator((byte[])colValuesToCompare[kk]),
                                    columnsToRemove)));
                    k++;kk++;                   
                    break;
                case GREATER:
                    linkedList.addLast(SingleColumnValueExcludeOrNotFilter(
                            (byte[])colNamesToFilter[k], 
                            CompareOp.GREATER, 
                            new BinaryComparator((byte[])colValuesToCompare[kk]),
                            columnsToRemove));
                    k++;kk++;
                    break;
                case GREATER_NULL:
                    linkedList.addLast(new FilterList(FilterList.Operator.MUST_PASS_ALL, //AND between if not null and the actual
                            SingleColumnValueExcludeOrNotFilter(
                                    (byte[])colNamesToFilter[k],
                                    CompareOp.EQUAL, 
                                    new BinaryPrefixComparator(new byte[]{0x00}),//check for null indicator = 0 representing non null
                                    columnsToRemove,
                                    true), //filterIfMissing, 
                            SingleColumnValueExcludeOrNotFilter(
                                    (byte[])colNamesToFilter[k], 
                                    CompareOp.GREATER, 
                                    new BinaryComparator((byte[])colValuesToCompare[kk]),
                                    columnsToRemove)));
                    k++;kk++;                   
                    break;
                case GREATER_OR_EQUAL:
                    linkedList.addLast(SingleColumnValueExcludeOrNotFilter(
                            (byte[])colNamesToFilter[k], 
                            CompareOp.GREATER_OR_EQUAL, 
                            new BinaryComparator((byte[])colValuesToCompare[kk]),
                            columnsToRemove));
                    k++;kk++;
                    break;
                case GREATER_OR_EQUAL_NULL:
                    linkedList.addLast(new FilterList(FilterList.Operator.MUST_PASS_ALL, //AND between if not null and the actual
                            SingleColumnValueExcludeOrNotFilter(
                                    (byte[])colNamesToFilter[k],
                                    CompareOp.EQUAL, 
                                    new BinaryPrefixComparator(new byte[]{0x00}),//check for null indicator = 0 representing non null
                                    columnsToRemove,
                                    true), //filterIfMissing,
                            SingleColumnValueExcludeOrNotFilter(
                                    (byte[])colNamesToFilter[k], 
                                    CompareOp.GREATER_OR_EQUAL, 
                                    new BinaryComparator((byte[])colValuesToCompare[kk]),
                                    columnsToRemove)));
                    k++;kk++;
                    break;
                case NO_OP:
                    linkedList.addLast(SingleColumnValueExcludeOrNotFilter(
                            (byte[])colNamesToFilter[k], 
                            CompareOp.NO_OP, 
                            new BinaryComparator((byte[])colValuesToCompare[kk]),
                            columnsToRemove));
                    k++;kk++;
                    break;
                case NO_OP_NULL:
                    linkedList.addLast(new FilterList(FilterList.Operator.MUST_PASS_ALL, //AND between if not null and the actual
                            SingleColumnValueExcludeOrNotFilter(
                                    (byte[])colNamesToFilter[k],
                                    CompareOp.EQUAL, 
                                    new BinaryPrefixComparator(new byte[]{0x00}),//check for null indicator = 0 representing non null
                                    columnsToRemove,
                                    true), //filterIfMissing,
                            SingleColumnValueExcludeOrNotFilter(
                                    (byte[])colNamesToFilter[k], 
                                    CompareOp.NO_OP, 
                                    new BinaryComparator((byte[])colValuesToCompare[kk]),
                                    columnsToRemove)));
                    k++;kk++;                   
                    break;
                case IS_NULL:
                    // is null on a non nullable column!
                    linkedList.addLast(SingleColumnValueExcludeOrNotFilter(
                            (byte[])colNamesToFilter[k], 
                            CompareOp.NO_OP, //exclude everything
                            new BinaryPrefixComparator((new byte[]{})),
                            columnsToRemove));
                    k++;
                    break;
                case IS_NULL_NULL:
                    // is_null on nullable column: is absent OR has the first byte set to FF indicating NULL.
                    linkedList.addLast(
                            new FilterList(FilterList.Operator.MUST_PASS_ONE, //OR
                                    SingleColumnValueExcludeOrNotFilter(
                                            (byte[])colNamesToFilter[k],
                                            CompareOp.EQUAL, 
                                            new NullComparator(),//is absent?
                                            columnsToRemove), 
                                    SingleColumnValueExcludeOrNotFilter(
                                            (byte[])colNamesToFilter[k],
                                            CompareOp.EQUAL, 
                                            new BinaryPrefixComparator(new byte[]{-1}),//0xFF has null prefix indicator
                                            columnsToRemove)));
                    k++;
                    break;
                case IS_NOT_NULL:
                    // is not null on a non nullable column!
                    // do nothing, always true
                    k++;
                    break;  
                case IS_NOT_NULL_NULL:
                    // is_not_null on nullable column: is not absent AND has the first byte not set to FF indicating NULL.
                    linkedList.addLast(SingleColumnValueExcludeOrNotFilter(
                            (byte[])colNamesToFilter[k],
                            CompareOp.NOT_EQUAL, 
                            new BinaryPrefixComparator(new byte[]{-1}),// 0xFF has null prefix indicator
                            columnsToRemove,
                            true));//filter if missing (if absent null)
                    k++;
                    break;
                case AND:
                    linkedList.addLast("AND");
                    break;
                case OR:
                    linkedList.addLast("OR");
                    break;
                    default:
            }//switch
        }//for
        //evaluate the reverse polish notation list
        while (linkedList.size()>1){// evaluate until only one element is left in the list
            //look for first operator (AND or OR)
            int j=0;
            while (j<linkedList.size() && !(linkedList.get(j) instanceof String)){
                j++;
            }
            //here j points on the first operator; (all operands are of type Filter)
            if (j==linkedList.size()){logger.error("j==linkedList.size()");return null;} // should not happen
            Filter leftOperand;
            Filter rightOperand;
            switch(Op.valueOf((String)linkedList.get(j))){
            case AND:
                FilterList filterListAnd = new FilterList(FilterList.Operator.MUST_PASS_ALL); //AND filterList
                //left operand
                leftOperand = (Filter)linkedList.get(j-2);
                if (leftOperand instanceof FilterList && ((FilterList)leftOperand).getOperator()==FilterList.Operator.MUST_PASS_ALL){//associativity of AND optimization
                    //for(Filter f:((FilterList)leftOperand).getFilters())
                    //  filterListAnd.addFilter(f);
                    filterListAnd = (FilterList)leftOperand; //more efficient than the 2 lines above (kept commented out for code lisibility)
                }else{
                    filterListAnd.addFilter(leftOperand);
                }
                // right operand
                rightOperand = (Filter)linkedList.get(j-1);
                if (rightOperand instanceof FilterList && ((FilterList)rightOperand).getOperator()==FilterList.Operator.MUST_PASS_ALL){//associativity of AND optimization
                    for(Filter f:((FilterList)rightOperand).getFilters())
                        filterListAnd.addFilter(f);                 
                }else{
                    filterListAnd.addFilter(rightOperand);
                }               
                // setup evaluated filter
                linkedList.set(j,filterListAnd); // replace the operator with the constructer filter
                linkedList.remove(j-1);// remove right operand
                linkedList.remove(j-2);// remove left operand. warning order matter 
                break;
            case OR:
                FilterList filterListOr = new FilterList(FilterList.Operator.MUST_PASS_ONE); //OR filterList
                leftOperand = (Filter)linkedList.get(j-2);
                rightOperand = (Filter)linkedList.get(j-1);
                //begin detection of null check factorization (A not null and (A <op> ?)) or (A not null and A <op2> ?) -> A not null and (A <op> ? or A <op2> ?)  
                //the code is doing more than just nullcheck, but any factorization where left operands are identical
                if (leftOperand instanceof FilterList && rightOperand instanceof FilterList && 
                    ((FilterList)leftOperand).getOperator() == FilterList.Operator.MUST_PASS_ALL &&
                    ((FilterList)rightOperand).getOperator() == FilterList.Operator.MUST_PASS_ALL &&
                    ((FilterList)leftOperand).getFilters().size() == 2 &&
                    ((FilterList)rightOperand).getFilters().size() == 2 &&
                    ((FilterList)leftOperand).getFilters().get(0) instanceof SingleColumnValueFilter && //cannot be SingleColumnValueExcludeFilter when we have the optimization scenario
                    ((FilterList)rightOperand).getFilters().get(0) instanceof SingleColumnValueFilter){//cannot be SingleColumnValueExcludeFilter when we have the optimization scenario
                    SingleColumnValueFilter scvfLeft = (SingleColumnValueFilter)((FilterList)leftOperand).getFilters().get(0);
                    SingleColumnValueFilter scvfRight = (SingleColumnValueFilter)((FilterList)rightOperand).getFilters().get(0);
                    if (scvfLeft.getOperator() == scvfRight.getOperator() && //more general case than just for null check (identical operands)
                        Arrays.equals(scvfLeft.getQualifier(),scvfRight.getQualifier()) &&
                        Arrays.equals(scvfLeft.getFamily(),scvfRight.getFamily()) &&
                        Arrays.equals(scvfLeft.getComparator().getValue(),scvfRight.getComparator().getValue()) &&
                        (scvfLeft.getFilterIfMissing() == scvfRight.getFilterIfMissing())){
                        Filter left = ((FilterList)leftOperand).getFilters().get(1);
                        Filter right = ((FilterList)rightOperand).getFilters().get(1);
                        if (left instanceof FilterList && ((FilterList)left).getOperator()==FilterList.Operator.MUST_PASS_ONE){//associativity of OR optimization
                            //for(Filter f:((FilterList)left).getFilters())
                            //  filterListOr.addFilter(f);
                            filterListOr = (FilterList)left; // more efficient than the 2 lines above (kept commented out for code lisibility)
                        }else{
                            filterListOr.addFilter(left);
                        }
                        // right operand                
                        if (right instanceof FilterList && ((FilterList)right).getOperator()==FilterList.Operator.MUST_PASS_ONE){//associativity of OR optimization
                            for(Filter f:((FilterList)right).getFilters())
                                filterListOr.addFilter(f);                  
                        }else{
                            filterListOr.addFilter(right);
                        }                                       
                        linkedList.set(j,new FilterList(FilterList.Operator.MUST_PASS_ALL,scvfLeft,filterListOr));//resulting factorized AND filter
                        linkedList.remove(j-1);// remove right operand
                        linkedList.remove(j-2);// remove left operand. warning order matter 
                        break;
                    }                                   
                }
                //end detection of null (and more) check factorization
                //begin detection of RangeSpec a<>? transformed to a<? or a>? to convert it back to a <> ? when we push down
                //check for <> on non nullable columns
                if (leftOperand instanceof SingleColumnValueFilter && rightOperand instanceof SingleColumnValueFilter){
                    SingleColumnValueFilter leftscvf = (SingleColumnValueFilter)leftOperand;
                    SingleColumnValueFilter rightscvf = (SingleColumnValueFilter)rightOperand;
                    if (leftscvf.getOperator() == CompareOp.LESS && rightscvf.getOperator()== CompareOp.GREATER && 
                            Arrays.equals(leftscvf.getQualifier(), rightscvf.getQualifier()) &&
                            Arrays.equals(leftscvf.getFamily(), rightscvf.getFamily()) &&
                            Arrays.equals(leftscvf.getComparator().getValue(),rightscvf.getComparator().getValue())
                        ){
                        // setup evaluated filter
                        linkedList.set(j,new SingleColumnValueFilter(leftscvf.getFamily(), leftscvf.getQualifier(), CompareOp.NOT_EQUAL, leftscvf.getComparator())); // replace the operator with the constructer filter
                        linkedList.remove(j-1);// remove right operand
                        linkedList.remove(j-2);// remove left operand. warning order matter                         
                        break;
                    }
                }
                //check for <> on nullable column
                if( leftOperand instanceof FilterList && rightOperand instanceof FilterList){
                    //no need to check FilterList size, as all possible case FilterList size is at least 2.
                    if (((FilterList)leftOperand).getFilters().get(1) instanceof SingleColumnValueFilter &&
                        ((FilterList)rightOperand).getFilters().get(1) instanceof SingleColumnValueFilter){
                        SingleColumnValueFilter leftscvf = (SingleColumnValueFilter)((FilterList)leftOperand).getFilters().get(1);
                        SingleColumnValueFilter rightscvf = (SingleColumnValueFilter)((FilterList)rightOperand).getFilters().get(1);
                        if (leftscvf.getOperator() == CompareOp.LESS && rightscvf.getOperator()== CompareOp.GREATER && 
                                Arrays.equals(leftscvf.getQualifier(), rightscvf.getQualifier()) &&
                                Arrays.equals(leftscvf.getFamily(), rightscvf.getFamily()) &&
                                Arrays.equals(leftscvf.getComparator().getValue(),rightscvf.getComparator().getValue())
                            ){
                            // setup evaluated filter
                            SingleColumnValueFilter nullCheck = new SingleColumnValueFilter(// null checker
                                    leftscvf.getFamily(), leftscvf.getQualifier(),
                                    CompareOp.EQUAL, 
                                    new BinaryPrefixComparator(new byte[]{0x00}));
                            nullCheck.setFilterIfMissing(true);
                            linkedList.set(j,new FilterList(FilterList.Operator.MUST_PASS_ALL, //AND between if not null and the actual
                                    nullCheck, 
                                    new SingleColumnValueFilter(
                                            leftscvf.getFamily(), leftscvf.getQualifier(), 
                                            CompareOp.NOT_EQUAL, 
                                            leftscvf.getComparator()))); 
                            linkedList.remove(j-1);// remove right operand
                            linkedList.remove(j-2);// remove left operand. warning order matter                         
                            break;
                        }                       
                    }
                }               
                //end detection of RangeSpec a<>?
                //now general case...
                //left operand              
                if (leftOperand instanceof FilterList && ((FilterList)leftOperand).getOperator()==FilterList.Operator.MUST_PASS_ONE){//associativity of OR optimization
                    //for(Filter f:((FilterList)leftOperand).getFilters())
                    //  filterListOr.addFilter(f);
                    filterListOr = (FilterList)leftOperand; // more efficient than the 2 lines above (kept commented out for code lisibility)
                }else{
                    filterListOr.addFilter(leftOperand);
                }
                // right operand                
                if (rightOperand instanceof FilterList && ((FilterList)rightOperand).getOperator()==FilterList.Operator.MUST_PASS_ONE){//associativity of OR optimization
                    for(Filter f:((FilterList)rightOperand).getFilters())
                        filterListOr.addFilter(f);                  
                }else{
                    filterListOr.addFilter(rightOperand);
                }               
                // setup evaluated filter
                linkedList.set(j,filterListOr); // replace the operator with the constructer filter
                linkedList.remove(j-1);// remove right operand
                linkedList.remove(j-2);// remove left operand. warning order matter 
                break;
            default:
                logger.error("operator different than OR or AND???");
                return null;//should never happen
            }           
        }
        // after evaluation, the linkedList contains only one element containing the filter built
        return (Filter)linkedList.pop();
    }
    
    
	public boolean startScan(long transID, byte[] startRow, byte[] stopRow,
                                 Object[]  columns, long timestamp,
                                 boolean cacheBlocks, boolean smallScanner, int numCacheRows,
                                 Object[] colNamesToFilter, 
                                 Object[] compareOpList, 
                                 Object[] colValuesToCompare,
                                 float dopParallelScanner,
                                 float samplePercent,
                                 boolean inPreFetch,
                                 boolean useSnapshotScan,
                                 int snapTimeout,
                                 String snapName,
                                 String tmpLoc,
                                 int espNum,
                                 int versions)
	        throws IOException {

	  Scan scan;

	  if (logger.isTraceEnabled()) logger.trace("Enter startScan() " + tableName + " txid: " + transID + " startRow="
			    + ((startRow != null) ? (Bytes.equals(startRow, HConstants.EMPTY_START_ROW) ? "INFINITE" : Hex.encodeHexString(startRow)) : "NULL")
			    + " stopRow=" + ((stopRow != null) ? (Bytes.equals(stopRow, HConstants.EMPTY_START_ROW) ? "INFINITE" : Hex.encodeHexString(stopRow)) : "NULL")
		        + " CacheBlocks: " + cacheBlocks + " numCacheRows: " + numCacheRows + " Bulkread: " + useSnapshotScan);

	  if (startRow != null && startRow.toString() == "")
	    startRow = null;
	  if (stopRow != null && stopRow.toString() == "")
	    stopRow = null;

	  if (startRow != null && stopRow != null)
	    scan = new Scan(startRow, stopRow);
	  else
	    scan = new Scan();

          if (versions != 0)
            {
              if (versions == -1)
                scan.setMaxVersions();
              else if (versions == -2)
                {
                  scan.setMaxVersions();
                  scan.setRaw(true);
                  columns = null;
                }
              else if (versions > 0)
               {
                 scan.setMaxVersions(versions);
               }
           }

          if (cacheBlocks == true) {
              scan.setCacheBlocks(true);
          }
	  else
              scan.setCacheBlocks(false);
          
      scan.setSmall(smallScanner);
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
	        FilterList list;
	        boolean narrowDownResultColumns = false; //to check if we need a narrow down column filter (V2 only feature)
	        if (compareOpList == null)return false;
	        if (new String((byte[])compareOpList[0]).equals("V2")){ // are we dealing with predicate pushdown V2
	            list = new FilterList(FilterList.Operator.MUST_PASS_ALL);
	            HashMap<String,Object> columnsToRemove = new HashMap<String,Object>();
	            //if columnsToRemove not null, we are narrowing down using the SingleColumnValue[Exclude]Filter method
	            //else we will use the explicit FamilyFilter and QualifierFilter
	            //the simplified logic is that we can use the first method if and only if each and every column in the
	            //pushed down predicate shows up only once.
	            for (int i = 0; i < colNamesToFilter.length; i++) {
	              byte[] colName = (byte[])colNamesToFilter[i];
	          
	              // check if the filter column is already part of the column list, if not add it if we are limiting columns (not *)
	              if(columns!=null && columns.length > 0){// if not *
	                  boolean columnAlreadyIn = false; //assume column not yet in the scan object
	                  for (int k=0; k<columns.length;k++){
	                      if (Arrays.equals(colName, (byte[])columns[k])){
	                          columnAlreadyIn = true;//found already exist
	                          break;//no need to look further
	                      }
	                  }
	                  if (!columnAlreadyIn){// column was not already in, so add it
	                      scan.addColumn(getFamily(colName),getName(colName));
	                      narrowDownResultColumns = true; //since we added a column for predicate eval, we need to remove it later out of result set
	                      String strColName = new String(colName);
	                      if (columnsToRemove != null && columnsToRemove.containsKey(strColName)){// if we already added this column, it means it shows up more than once
	                          columnsToRemove = null; // therefore, use the FamilyFilter/QualifierFilter method
	                      }else if (columnsToRemove != null)// else 
	                          columnsToRemove.put(strColName,null); // add it to the list of column that should be nuked with the Exclude version of the SingleColumnValueFilter
	                  }
	              }         
	            }
	            if (columnsToRemove != null)
	            { //we are almost done checking if Exclude version of SingleColumnnValueFilter can be used. Th elast check s about to know if there is a IS_NULL_NULL
	              //operation that cannot be using the Exclude method, as it is transformed in a filterList with OR, therefore we cannot guaranty that the SingleColumnValueExcludeFilter
	              //performing the exclusion will be reached.
	                boolean is_null_nullFound = false;
	                for (Object o:compareOpList ){
	                    if (new String((byte[])o).equals("IS_NULL_NULL")){
	                        is_null_nullFound = true;
	                        break;
	                    }                       
	                }
	                if (is_null_nullFound){
	                    columnsToRemove = null; // disable Exclude method version of SingleColumnnValueFilter
	                }else
	                    narrowDownResultColumns = false; // we will use the Exclude version of SingleColumnnValueFilter, so bypass the Family/QualifierFilter method
	            }
	            Filter f =constructV2Filter(colNamesToFilter,compareOpList,colValuesToCompare, columnsToRemove);
	            if (f==null) return false; // error logging done inside constructV2Filter
	            list.addFilter(f);
	        }//end V2
	        else{// deal with V1
	            list = new FilterList(FilterList.Operator.MUST_PASS_ALL);
	            
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
	        }//end V1
	        // if we added a column for predicate eval, we need to filter down result columns
	        FilterList resultColumnsOnlyFilter = null;
	        if (narrowDownResultColumns){           
	            HashMap<String,ArrayList<byte[]>> hm = new HashMap<String,ArrayList<byte[]>>(3);//use to deal with multiple family table
	            // initialize hm with list of columns requested for output
	                for (int i=0; i<columns.length; i++){ // if we are here we know columns is not null
	                    if (hm.containsKey(new String(getFamily((byte[])columns[i])))){
	                        hm.get(new String(getFamily((byte[])columns[i]))).add((byte[])columns[i]);
	                    }else{
	                        ArrayList<byte[]> al = new ArrayList<byte[]>();
	                        al.add((byte[])columns[i]);
	                        hm.put(new String(getFamily((byte[])columns[i])), al);
	                    }                   
	                }
	                
	            if (hm.size()==1){//only one column family
	                resultColumnsOnlyFilter = new FilterList(FilterList.Operator.MUST_PASS_ALL);
	                if (columns.length == 1){
	                    resultColumnsOnlyFilter.addFilter(new QualifierFilter(CompareOp.EQUAL, new BinaryComparator(getName((byte[])columns[0]))));                     
	                }else{// more than one column
	                    FilterList flColumns = new FilterList(FilterList.Operator.MUST_PASS_ONE);
	                    for(int i=0; i<columns.length;i++)
	                        flColumns.addFilter(new QualifierFilter(CompareOp.EQUAL, new BinaryComparator(getName((byte[])columns[i]))));                   
	                    resultColumnsOnlyFilter.addFilter(flColumns);
	                }                               
	                // note the optimization puting family check at the end
	                resultColumnsOnlyFilter.addFilter(new FamilyFilter(CompareOp.EQUAL, new BinaryComparator(getFamily((byte[])columns[0]))));
	            }else{//more than one column family
	                resultColumnsOnlyFilter = new FilterList(FilterList.Operator.MUST_PASS_ONE);
	                for (Map.Entry<String,ArrayList<byte[]>> entry : hm.entrySet()){//for each column family
	                    ArrayList<byte[]> alb = entry.getValue();
	                    if (alb.size() == 1){// when only one column for the family
	                        resultColumnsOnlyFilter.addFilter(
	                                new FilterList(FilterList.Operator.MUST_PASS_ALL,
	                                               new QualifierFilter(CompareOp.EQUAL, new BinaryComparator(getName(alb.get(0)))),
	                                               new FamilyFilter(CompareOp.EQUAL, new BinaryComparator(getFamily(alb.get(0)))))
	                        );
	                    }else{// when multiple columns for the family
	                        FamilyFilter familyFilter = null;
	                        FilterList filterListCol = new FilterList(FilterList.Operator.MUST_PASS_ONE);
	                        for(int j = 0; j<alb.size(); j++){
	                            if (familyFilter == null)
	                                familyFilter = new FamilyFilter(CompareOp.EQUAL, new BinaryComparator(getFamily(alb.get(0))));
	                            filterListCol.addFilter(new QualifierFilter(CompareOp.EQUAL, new BinaryComparator(getName(alb.get(j)))));                           
	                        }
	                        resultColumnsOnlyFilter.addFilter(new FilterList(FilterList.Operator.MUST_PASS_ALL,filterListCol,familyFilter));
	                    }
	                }
	            }
	            list.addFilter(resultColumnsOnlyFilter); // add column limiting filter
	        }//end narrowDownResultColumns
	    if (samplePercent > 0.0f)
	      list.addFilter(new RandomRowFilter(samplePercent));
        // last optimization is making sure we remove top level filter list if it is singleton MUST_PASS_ALL filterlist
        if (list.getFilters().size()==1){
            scan.setFilter(list.getFilters().get(0));
            if (logger.isTraceEnabled()) logger.trace("Pushed down filter:"+list.getFilters().get(0));
        }else{
	    scan.setFilter(list);
        if (logger.isTraceEnabled()) logger.trace("Pushed down filter:"+list );
        }
	  } else if (samplePercent > 0.0f) {
	    scan.setFilter(new RandomRowFilter(samplePercent));
	  }

	  if (!useSnapshotScan || transID != 0)
	  {
	    if (useTRexScanner && (transID != 0)) {
	      scanner = table.getScanner(transID, scan);
	    } else {
          scanner = table.getScanner(scan,dopParallelScanner);
        }
        if (logger.isTraceEnabled()) logger.trace("startScan(). After getScanner. Scanner: " + scanner+ " dop:"+
        		    dopParallelScanner + "TransID " + transID + " " + useTRexScanner + " " +  getTableName());
	  }
	  else
	  {
	    snapHelper = new SnapshotScanHelper(connection.getConfiguration(), tmpLoc,snapName);

	    if (logger.isTraceEnabled()) 
	      logger.trace("[Snapshot Scan] HTableClient.startScan(). useSnapshotScan: " + useSnapshotScan + 
	                   " espNumber: " + espNum + 
	                   " tmpLoc: " + snapHelper.getTmpLocation() + 
	                   " snapshot name: " + snapHelper.getSnapshotName());
	    
	    if (!snapHelper.snapshotExists())
	      throw new IOException ("Snapshot " + snapHelper.getSnapshotName() + " does not exist.");
 
            try { 
	      snapHelper.createTableSnapshotScanner(snapTimeout, 5, espNum, scan);
            }
            catch (InterruptedException ie) {
               throw new IOException(ie);
            }
	  }
    
          if (useSnapshotScan)
             preFetch = false;
          else
	     preFetch = inPreFetch;
	  if (preFetch)
	  {
	    scanHelper = new ScanHelper(); 
            future = executorService.submit(scanHelper);
	  }
          fetchType = SCAN_FETCH;
	  if (logger.isTraceEnabled()) logger.trace("Exit startScan().");
	  return true;
	}

	public int  startGet(long transID, byte[] rowID, 
                     Object[] columns,
		     long timestamp) throws IOException {

	    if (logger.isTraceEnabled()) logger.trace("Enter startGet(" + tableName + 
			     " #cols: " + ((columns == null) ? 0:columns.length ) +
			     " rowID: " + new String(rowID));
		fetchType = GET_ROW;
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
                        setJavaObject(jniObject);
			return 0;
		}
		if (logger.isTraceEnabled()) logger.trace("startGet, result: " + getResult);
		pushRowsToJni(getResult);
		return 1;

	}

	// The TransactionalTable class is missing the batch get operation,
	// so work around it.
	private Result[] batchGet(long transactionID, List<Get> gets)
			throws IOException {
		if (logger.isTraceEnabled()) logger.trace("Enter batchGet(multi-row) " + tableName);
		Result [] results = new Result[gets.size()];
		int i=0;
		for (Get g : gets) {
			Result r = table.get(transactionID, g);
			results[i++] = r;
		}
		return results;
	}

	public int startGet(long transID, Object[] rows,
			Object[] columns, long timestamp)
                        throws IOException {

		if (logger.isTraceEnabled()) logger.trace("Enter startGet(multi-row) " + tableName);

		List<Get> listOfGets = new ArrayList<Get>();
		for (int i = 0; i < rows.length; i++) {
			byte[] rowID = (byte[])rows[i]; 
			Get get = new Get(rowID);
			listOfGets.add(get);
			if (columns != null)
			{
				for (int j = 0; j < columns.length; j++ ) {
					byte[] col = (byte[])columns[j];
					get.addColumn(getFamily(col), getName(col));
				}
			}
		}
		if (columns != null)
			numColsInScan = columns.length;
		else
			numColsInScan = 0;
		if (useTRex && (transID != 0)) {
			getResultSet = batchGet(transID, listOfGets);
                        fetchType = GET_ROW; 
		} else {
			getResultSet = table.get(listOfGets);
			fetchType = BATCH_GET;
		}
		if (getResultSet != null && getResultSet.length > 0) {
                	 pushRowsToJni(getResultSet);
			return getResultSet.length;
		}
		else {
			setJavaObject(jniObject);
			return 0;
		}
	}

	public int getRows(long transID, short rowIDLen, Object rowIDs,
			Object[] columns)
                        throws IOException {
            
		if (logger.isTraceEnabled()) logger.trace("Enter getRows " + tableName);

		ByteBuffer bbRowIDs = (ByteBuffer)rowIDs;
		List<Get> listOfGets = new ArrayList<Get>();
		short numRows = bbRowIDs.getShort();
		short actRowIDLen ;
		byte rowIDSuffix;
		byte[] rowID;

		for (int i = 0; i < numRows; i++) {
                        rowIDSuffix  = bbRowIDs.get();
                        if (rowIDSuffix == '1')
		           actRowIDLen = (short)(rowIDLen+1);
                        else
                           actRowIDLen = rowIDLen; 	
			rowID = new byte[actRowIDLen];
			bbRowIDs.get(rowID, 0, actRowIDLen);
			Get get = new Get(rowID);
			listOfGets.add(get);
			if (columns != null) {
				for (int j = 0; j < columns.length; j++ ) {
					byte[] col = (byte[])columns[j];
					get.addColumn(getFamily(col), getName(col));
				}
			}
		}
		if (columns != null)
			numColsInScan = columns.length;
		else
			numColsInScan = 0;
		if (useTRex && (transID != 0)) {
			getResultSet = batchGet(transID, listOfGets);
     			fetchType = GET_ROW; 
		} else {
			getResultSet = table.get(listOfGets);
			fetchType = BATCH_GET;
		}
		if (getResultSet.length != numRows)
                   throw new IOException("Number of rows retunred is not equal to requested number of rows");
 		pushRowsToJni(getResultSet);
		return getResultSet.length;
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
                                throw new IOException("HTableClient.FetchRows() called before scanOpen().");
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
                        kvFamArray = new byte[numTotalCells][];
                        kvQualArray = new byte[numTotalCells][];
		}
               
		if (rowIDs == null || (rowIDs != null &&
				rowsReturned > rowIDs.length))
		{
			rowIDs = new byte[rowsReturned][];
			kvsPerRow = new int[rowsReturned];
		}
		int cellNum = 0;
		boolean colFound = false;
		for (int rowNum = 0; rowNum < rowsReturned ; rowNum++)
		{
			rowIDs[rowNum] = result[rowNum].getRow();
			kvList = result[rowNum].rawCells();
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
                                kvFamArray[cellNum] = kv.getFamilyArray();
                                kvQualArray[cellNum] = kv.getQualifierArray();
				colFound = true;
			}
		}
		int cellsReturned;
		if (colFound)
                	cellsReturned = cellNum++;
		else
			cellsReturned = 0;
		if (cellsReturned == 0)
			setResultInfo(jniObject, null, null,
				null, null, null, null,
				null, null, null, null, rowIDs, kvsPerRow, cellsReturned, rowsReturned);
		else 
			setResultInfo(jniObject, kvValLen, kvValOffset,
				kvQualLen, kvQualOffset, kvFamLen, kvFamOffset,
				kvTimestamp, kvBuffer, kvFamArray, kvQualArray, rowIDs, kvsPerRow, cellsReturned, rowsReturned);
		return rowsReturned;	
	}		
	
	protected int pushRowsToJni(Result result) 
			throws IOException {
		int rowsReturned = 1;
		int numTotalCells;

		if (numColsInScan == 0)
			numTotalCells = result.size();
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
                        kvFamArray = new byte[numTotalCells][];
                        kvQualArray = new byte[numTotalCells][];
		}
		if (rowIDs == null)
		{
			rowIDs = new byte[rowsReturned][];
			kvsPerRow = new int[rowsReturned];
		}
		kvList = result.rawCells();
 		if (kvList == null)
			numColsReturned = 0; 
		else
			numColsReturned = kvList.length;
		if ((numColsReturned) > numTotalCells)
			throw new IOException("Insufficient cell array pre-allocated");
 		rowIDs[0] = result.getRow();
		kvsPerRow[0] = numColsReturned;
		for (int colNum = 0 ; colNum < numColsReturned ; colNum++)
		{ 
			kv = kvList[colNum];
			kvValLen[colNum] = kv.getValueLength();
			kvValOffset[colNum] = kv.getValueOffset();
			kvQualLen[colNum] = kv.getQualifierLength();
			kvQualOffset[colNum] = kv.getQualifierOffset();
			kvFamLen[colNum] = kv.getFamilyLength();
			kvFamOffset[colNum] = kv.getFamilyOffset();
			kvTimestamp[colNum] = kv.getTimestamp();
			kvBuffer[colNum] = kv.getValueArray();
                        kvFamArray[colNum] = kv.getFamilyArray();
                        kvQualArray[colNum] = kv.getQualifierArray();
		}
		if (numColsReturned == 0)
			setResultInfo(jniObject, null, null,
				null, null, null, null,
				null, null, null, null, rowIDs, kvsPerRow, numColsReturned, rowsReturned);
		else
			setResultInfo(jniObject, kvValLen, kvValOffset,
				kvQualLen, kvQualOffset, kvFamLen, kvFamOffset,
				kvTimestamp, kvBuffer, kvFamArray, kvQualArray, rowIDs, kvsPerRow, numColsReturned, rowsReturned);
		return rowsReturned;	
	}		
	
	public boolean deleteRow(final long transID, byte[] rowID, 
				 Object[] columns,
				 long timestamp,
                                 boolean asyncOperation,
                                 final boolean useRegionXn) throws IOException {

            if (logger.isTraceEnabled()) logger.trace("Enter deleteRow transID " + transID
                  + " (" + new String(rowID) + ", " + timestamp + ") " + tableName);
            
            final Delete del;
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
            if (asyncOperation) {
                future = executorService.submit(new Callable() {
                        public Object call() throws IOException {
                            boolean res = true;
                            if (useTRex && (transID != 0)) {
                                table.delete(transID, del);
                            }
                            else if (useRegionXn){
                                table.deleteRegionTx(del, /* auto-commit */ true);
                            }
                            else {
                                table.delete(del);
                            }
                            return new Boolean(res);
                        }
                    });
		    return true;
            }
            else {
                if (useTRex && (transID != 0)) {
                    table.delete(transID, del);
                }
                else if (useRegionXn){
                    table.deleteRegionTx(del, /* auto-commit */ true);
                }
                else {
                    table.delete(del);
                }
            }
            if (logger.isTraceEnabled()) logger.trace("Exit deleteRow");
            return true;
	}

	public boolean deleteRows(final long transID, short rowIDLen, Object rowIDs,
		      long timestamp,
                      boolean asyncOperation) throws IOException {

	        if (logger.isTraceEnabled()) logger.trace("Enter deleteRowsInt() transID "
	              + transID + " " + tableName);

		final List<Delete> listOfDeletes = new ArrayList<Delete>();
		listOfDeletes.clear();
		ByteBuffer bbRowIDs = (ByteBuffer)rowIDs;
		short numRows = bbRowIDs.getShort();
                byte[] rowID;		
		byte rowIDSuffix;
		short actRowIDLen;
       
		for (short rowNum = 0; rowNum < numRows; rowNum++) {
                        rowIDSuffix  = bbRowIDs.get();
                        if (rowIDSuffix == '1')
		           actRowIDLen = (short)(rowIDLen+1);
                        else
                           actRowIDLen = rowIDLen; 	
			rowID = new byte[actRowIDLen];
			bbRowIDs.get(rowID, 0, actRowIDLen);

			Delete del;
			if (timestamp == -1)
			    del = new Delete(rowID);
			else
			    del = new Delete(rowID, timestamp);
			listOfDeletes.add(del);
		}
                if (asyncOperation) {
                        future = executorService.submit(new Callable() {
                                public Object call() throws IOException {
                                    boolean res = true;
				   if (useTRex && (transID != 0)) 
				      table.delete(transID, listOfDeletes);
				   else
				      table.delete(listOfDeletes);
				   return new Boolean(res);
				}
			});
			return true;
		}
		else {
			if (useTRex && (transID != 0)) 
		    	   table.delete(transID, listOfDeletes);
			else
		  	   table.delete(listOfDeletes);
		}
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
					 long timestamp, final boolean useRegionXn) throws IOException {

            if (logger.isTraceEnabled()) logger.trace("Enter checkAndDeleteRow transID " + transID
                    + " (" + new String(rowID) + ", "  + new String(columnToCheck) + ", " + new String(colValToCheck) + ", " + timestamp + ") " + tableName);
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
            }
            else if (useRegionXn){
               res = table.checkAndDeleteRegionTx(rowID, family, qualifier, colValToCheck,
               		         del, /* autoCommit */ true);
            }
            else {
                res = table.checkAndDelete(rowID, family, qualifier, colValToCheck, del);
            }
            
            if (res == false)
                return false;
            return true;
	}
    
	public boolean putRow(final long transID, final byte[] rowID, Object row,
                              byte[] columnToCheck, final byte[] colValToCheck,
                              final short colIndexToCheck, 
			      final boolean checkAndPut, boolean asyncOperation,
                              final boolean useRegionXn) throws IOException, InterruptedException, 
                          ExecutionException 
	{
		if (logger.isTraceEnabled()) logger.trace("Enter putRow() " + tableName + 
							  " transID: " + transID +
							  " useTRex: " + useTRex +
							  " useRegionXn: " + useRegionXn);

	 	final Put put;
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
			if (checkAndPut && colIndex == colIndexToCheck) {
				family = getFamily(colName);
				qualifier = getName(colName);
			} 
		}
		if (columnToCheck != null && columnToCheck.length > 0) {
			family = getFamily(columnToCheck);
			qualifier = getName(columnToCheck);
		}
		final byte[] family1 = family;
		final byte[] qualifier1 = qualifier;
		if (asyncOperation) {
                    future = executorService.submit(new Callable() {
                            public Object call() throws IOException {
                                boolean res = true;
                                
                                if (checkAndPut) {
                                    if (useTRex && (transID != 0)){
                                        res = table.checkAndPut(transID, rowID,
                                                                family1, qualifier1, colValToCheck, put);
                                    }
                                    else if (useRegionXn){
                                        if (logger.isTraceEnabled()) logger.trace("checkAndPutRegionTx with regionTX ");
                                        res = table.checkAndPutRegionTx(rowID, 
                                                                        family1, qualifier1, colValToCheck, put, /* auto-commit */ true);
                                        
                                    }
                                    else {
                                        res = table.checkAndPut(rowID, 
                                                                family1, qualifier1, colValToCheck, put);
                                    }
                                }
                                else {
                                    if (useTRex && (transID != 0)){
                                        table.put(transID, put);
                                    }
                                    else if (useRegionXn){
                                        if (logger.isTraceEnabled()) logger.trace("putRow using putRegionTx");
                                        table.putRegionTx(put, /* auto-commit */ true);
                                    }else{ 
                                        table.put(put);
                                    }
                                }
                                return new Boolean(res);
                            }
			});
                    return true;
		} else {
                    boolean result = true;
                    if (checkAndPut) {
                        if (useTRex && (transID != 0)){
                            result = table.checkAndPut(transID, rowID, 
                                                       family1, qualifier1, colValToCheck, put);
                        }
                        else if (useRegionXn){
                            if (logger.isTraceEnabled()) logger.trace("checkAndPutRegionTx using regionTX ");
                            result = table.checkAndPutRegionTx(rowID, family1, qualifier1,
                                                               colValToCheck, put, /* auto-commit */ true);
                        }
                        else {
                            result = table.checkAndPut(rowID, 
                                                       family1, qualifier1, colValToCheck, put);
                        }
                    }
                    else {
                        if (useTRex && (transID != 0)){
                            table.put(transID, put);
                        }
                        else if (useRegionXn){
                            if (logger.isTraceEnabled()) logger.trace("putRow using putRegionTx");
                            table.putRegionTx(put, true /* also commit */);
                        }else{
                            table.put(put);
                        }
                    }
                    return result;
		}	
	}
    
    /* public boolean insertRow(long transID, byte[] rowID, 
                         Object row, 
			 long timestamp,
                         boolean asyncOperation) throws IOException, InterruptedException, ExecutionException {
	    return putRow(transID, rowID, row, null, null, 0, 
			  false, asyncOperation, false);
			  } */

	public boolean putRows(final long transID, short rowIDLen, Object rowIDs, 
                       Object rows,
                       long timestamp, boolean asyncOperation)
			throws IOException, InterruptedException, ExecutionException  {

		if (logger.isTraceEnabled()) logger.trace("Enter putRows() " + tableName +
							  " transID: " + transID + 
							  " useTRex: " + useTRex);
		Put put;
		ByteBuffer bbRows, bbRowIDs;
		short numCols, numRows;
		short colNameLen;
                int colValueLen;
		byte[] colName, colValue, rowID;
		byte rowIDSuffix;
                short actRowIDLen;
		bbRowIDs = (ByteBuffer)rowIDs;
		bbRows = (ByteBuffer)rows;

		final List<Put> listOfPuts = new ArrayList<Put>();
		numRows = bbRowIDs.getShort();
		
		for (short rowNum = 0; rowNum < numRows; rowNum++) {
                        rowIDSuffix  = bbRowIDs.get();
                        if (rowIDSuffix == '1')
		           actRowIDLen = (short)(rowIDLen+1);
                        else
                           actRowIDLen = rowIDLen; 	
			rowID = new byte[actRowIDLen];
			bbRowIDs.get(rowID, 0, actRowIDLen);
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
		if (asyncOperation) {
			future = executorService.submit(new Callable() {
				public Object call() throws IOException {
					boolean res = true;
					if (useTRex && (transID != 0)) 
						table.put(transID, listOfPuts);
					else 
						table.put(listOfPuts);
					return new Boolean(res);
				}
			});
		}
		else {
			if (useTRex && (transID != 0)) 
				table.put(transID, listOfPuts);
			else 
				table.put(listOfPuts);
		}
		return true;
	} 

	public boolean completeAsyncOperation(int timeout, boolean resultArray[]) 
			throws InterruptedException, ExecutionException
	{
		if (timeout == -1) {
			if (! future.isDone()) 
				return false;
		}
	 	try {			
			Boolean result = (Boolean)future.get(timeout, TimeUnit.MILLISECONDS);
                        // Need to enhance to return the result 
                        // for each Put object
			for (int i = 0; i < resultArray.length; i++)
			    resultArray[i] = result.booleanValue();
			future = null;
 		} catch(TimeoutException te) {
			return false;
		} 
		return true;
	}

    /* public boolean checkAndInsertRow(long transID, byte[] rowID, 
                         Object row, 
			 long timestamp,
                         boolean asyncOperation) throws IOException, InterruptedException, ExecutionException  {
	    return putRow(transID, rowID, row, null, null, 0, 
                              true, asyncOperation, false);
			      } */

	public boolean checkAndUpdateRow(long transID, byte[] rowID, 
             Object columns, byte[] columnToCheck, byte[] colValToCheck,
             long timestamp, boolean asyncOperation) throws IOException, InterruptedException, 
                                    ExecutionException, Throwable  {
	    short colIndexToCheck = 0; // overridden by columnToCheck
	    return putRow(transID, rowID, columns, columnToCheck, 
			  colValToCheck, colIndexToCheck,
			  true, asyncOperation, false);
	}

        public byte[] coProcAggr(long transID, int aggrType, 
		byte[] startRowID, 
              byte[] stopRowID, byte[] colFamily, byte[] colName, 
              boolean cacheBlocks, int numCacheRows) 
                          throws IOException, Throwable {

		    Configuration customConf = connection.getConfiguration();
                    long rowCount = 0;

       if (transID > 0) {
          TransactionalAggregationClient aggregationClient = 
                          new TransactionalAggregationClient(customConf, connection);
		      Scan scan = new Scan();
		      scan.addFamily(colFamily);
		      scan.setCacheBlocks(false);
		      final ColumnInterpreter<Long, Long, EmptyMsg, LongMsg, LongMsg> ci =
              new LongColumnInterpreter();
		      byte[] tname = getTableName().getBytes();
              TransactionalTable lv_ttable = new TransactionalTable(getTableName(), connection);
              TransactionState ts = table.registerTransaction(lv_ttable, transID, startRowID);

		      rowCount = aggregationClient.rowCount(transID, ts.getStartId(),
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

		    byte[] rcBytes = 
                      ByteBuffer.allocate(8).order(ByteOrder.LITTLE_ENDIAN).putLong(rowCount).array();
                    return rcBytes; 
	}


	public boolean release(boolean cleanJniObject) throws IOException {

           boolean retcode = false;
          // Complete the pending IO
           if (future != null) {
              try {
                 future.get(30, TimeUnit.SECONDS);
              } catch(TimeoutException e) {
		  logger.error("Asynchronous Thread is Cancelled (timeout), " + e);
                  retcode = true;
                  future.cancel(true); // Interrupt the thread
              } catch(InterruptedException e) {
		  logger.error("Asynchronous Thread is Cancelled (interrupt), " + e);
                  retcode = true;
                  future.cancel(true); // Interrupt the thread
              } catch (ExecutionException ee)
              {
              }
              future = null;
          }
	  if (scanner != null) {
	  	if (logger.isTraceEnabled()) logger.trace("scanner.close() " + tableName + " " + scanner + " " 
	  			 + retcode );
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
          table.close();
	  return retcode;
	}

	public boolean close(boolean clearRegionCache, boolean cleanJniObject) throws IOException {
           if (logger.isTraceEnabled()) logger.trace("Enter close() " + tableName);
           if (table != null) 
           {
/*
              if (clearRegionCache)
              {
                 connection.clearRegionCache(tableName.getBytes());
              }
*/
              table.close();
              table = null;
           }
           return true;
	}

    public byte[][] getStartKeys() throws IOException
    {
       return table.getStartKeys();
    }

    public byte[][] getEndKeys() throws IOException
    {
       return table.getEndKeys();
    }

    private void cleanScan()
    {
        if (fetchType == GET_ROW || fetchType == BATCH_GET)
           return;
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

    protected void setJniObject(long inJniObject) {
       jniObject = inJniObject;
    }    

    private native int setResultInfo(long jniObject,
				int[] kvValLen, int[] kvValOffset,
				int[] kvQualLen, int[] kvQualOffset,
				int[] kvFamLen, int[] kvFamOffset,
  				long[] timestamp, 
				byte[][] kvBuffer, 
                                byte[][] kvFamArray,
                                byte[][] kvQualArray,
                                byte[][] rowIDs,
				int[] kvsPerRow, int numCellsReturned,
				int rowsReturned);

   private native void cleanup(long jniObject);

   protected native int setJavaObject(long jniObject);
 
   static {
     envUseTRex = true;
     envUseTRexScanner = true;
     String useTransactions = System.getenv("USE_TRANSACTIONS");
     if (useTransactions != null) {
        int lv_useTransactions = (Integer.parseInt(useTransactions));
        if (lv_useTransactions == 0) 
           envUseTRex = false;
     }
     String useTransactionsScanner = System.getenv("USE_TRANSACTIONS_SCANNER");
     if (useTransactionsScanner != null) {
        int lv_useTransactionsScanner = (Integer.parseInt(useTransactionsScanner));
        if (lv_useTransactionsScanner == 0) 
           envUseTRexScanner = false;
     }
     executorService = Executors.newCachedThreadPool();
     System.loadLibrary("executor");
   }
}
