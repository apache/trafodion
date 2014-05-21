package org.apache.hadoop.hbase.client.transactional;

import java.io.IOException;
import java.io.ByteArrayOutputStream;
import java.io.DataOutputStream;


import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.hbase.HBaseConfiguration;
import org.apache.hadoop.hbase.HRegionInfo;
import org.apache.hadoop.hbase.HRegionLocation;
import org.apache.hadoop.hbase.client.HConnection;
import org.apache.hadoop.hbase.client.HTable;
import org.apache.hadoop.hbase.client.Delete;
import org.apache.hadoop.hbase.client.Get;
import org.apache.hadoop.hbase.client.Put;
import org.apache.hadoop.hbase.client.Result;
import org.apache.hadoop.hbase.client.ResultScanner;
import org.apache.hadoop.hbase.client.Scan;
import org.apache.hadoop.hbase.util.Bytes;
import org.apache.hadoop.hbase.client.ServerCallable;
import org.apache.hadoop.hbase.client.transactional.TransactionManager;
import org.apache.hadoop.hbase.client.transactional.TransactionState;
import org.apache.hadoop.hbase.client.transactional.CommitUnsuccessfulException;
import org.apache.hadoop.hbase.client.transactional.UnknownTransactionException;
import org.apache.hadoop.hbase.client.transactional.HBaseBackedTransactionLogger;
import java.util.Map;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.concurrent.ConcurrentHashMap;

public class RMInterface extends TransactionalTable{
    static final Log LOG = LogFactory.getLog(RMInterface.class);

    Map<Long, TransactionState> mapTransactionStates;
    
    static {
        System.loadLibrary("stmlib");
   }

    private native void registerRegion(int port, byte[] hostname, byte[] regionInfo);
    
    public static void main(String[] args) {
      System.out.println("MAIN ENTRY");      
    }
    
    public RMInterface(final Configuration conf, final String tableName) throws IOException {
        super(conf, Bytes.toBytes(tableName));
        mapTransactionStates = new ConcurrentHashMap<Long, TransactionState>();
    }

    public synchronized TransactionState registerTransaction(final long transactionID, final byte[] row) throws IOException {
	LOG.trace("Enter registerTransaction, transaction ID: " + transactionID);
        boolean register = false;
        short ret = 0;

	TransactionState ts = mapTransactionStates.get(transactionID);
        
        // if we don't have a TransactionState for this ID we need to register it with the TM
        if (ts == null) {
            ts = new TransactionState(transactionID);
	    LOG.trace("registerTransaction, created TransactionState " + ts);
            mapTransactionStates.put(transactionID, ts);
            register = true;
        }
        HRegionLocation location = super.getRegionLocation(row, false /*reload*/);

        TransactionRegionLocation trLocation = new TransactionRegionLocation(location.getRegionInfo(),
                                                                             location.getHostname(), 
                                                                             location.getPort());
	LOG.trace("RMInterface:registerTransaction, created TransactionRegionLocation " + trLocation);
            
        // if this region hasn't been registered as participating in the transaction, we need to register it
        if (ts.addRegion(trLocation)) {
            register = true;
	    LOG.trace("registerTransaction, added TransactionRegionLocation to ts");
        }

        // register region with TM.
        if (register) {

	    byte [] lv_hostname = location.getHostname().getBytes();
	    int lv_port = location.getPort();

	    ByteArrayOutputStream lv_bos = new ByteArrayOutputStream();
	    DataOutputStream lv_dos = new DataOutputStream(lv_bos);
	    location.getRegionInfo().write(lv_dos);
	    lv_dos.flush();
	    byte [] lv_byte_region_info = lv_bos.toByteArray();
	    LOG.trace("RMInterface:registerTransaction, byte region info: " + new String(lv_byte_region_info));
	    registerRegion(lv_port, lv_hostname, lv_byte_region_info);
        }
        if ((ts == null) || (ret != 0)) {
            LOG.error("registerTransaction failed, TransactionState is NULL"); 
            throw new IOException("registerTransaction failed with error.");
        }

	LOG.trace("Exit registerTransaction, transaction ID: " + transactionID);
        return ts;
    }
   
    public synchronized void unregisterTransaction(TransactionState ts) {
        mapTransactionStates.remove(ts.getTransactionId());
    }
    
    public synchronized Result get(final long transactionID, final Get get) throws IOException {
        LOG.trace("get txid: " + transactionID);
        TransactionState ts = registerTransaction(transactionID, get.getRow());
        Result res = super.get(ts, get);
        LOG.trace("EXIT get -- result: " + res.toString());
        return res;	
    }
    
    public synchronized void delete(final long transactionID, final Delete delete) throws IOException {
        LOG.trace("delete txid: " + transactionID);
        TransactionState ts = registerTransaction(transactionID, delete.getRow());
        super.delete(ts, delete);
    }
    
    public synchronized ResultScanner getScanner(final long transactionID, final Scan scan) throws IOException {
        LOG.trace("getScanner txid: " + transactionID);
        TransactionState ts = registerTransaction(transactionID, scan.getStartRow());
        ResultScanner res = super.getScanner(ts, scan);
        LOG.trace("EXIT getScanner");
        return res;
    }
    
    public synchronized void put(final long transactionID, final Put put) throws IOException {
        LOG.trace("Enter Put txid: " + transactionID);
        TransactionState ts = registerTransaction(transactionID, put.getRow());
        super.put(ts, put);
        LOG.trace("Exit Put txid: " + transactionID);
    }

    public synchronized void put(final long transactionID, final List<Put> puts) throws IOException {
        LOG.trace("Enter put (list of puts) txid: " + transactionID);
	TransactionState ts;
	for (Put put : puts) {
	    ts = registerTransaction(transactionID, put.getRow());
	}
	ts = mapTransactionStates.get(transactionID);
        super.put(ts, puts);
        LOG.trace("Exit put (list of puts) txid: " + transactionID);
    }

    public synchronized boolean checkAndPut(final long transactionID, byte[] row, byte[] family, byte[] qualifier,
                       byte[] value, Put put) throws IOException {

        LOG.trace("Enter checkAndPut txid: " + transactionID);
	TransactionState ts = registerTransaction(transactionID, row);
	return super.checkAndPut(ts, row, family, qualifier, value, put);
    }

    public synchronized boolean checkAndDelete(final long transactionID, byte[] row, byte[] family, byte[] qualifier,
                       byte[] value, Delete delete) throws IOException {

        LOG.trace("Enter checkAndDelete txid: " + transactionID);
	TransactionState ts = registerTransaction(transactionID, row);
	return super.checkAndDelete(ts, row, family, qualifier, value, delete);
    }
}
