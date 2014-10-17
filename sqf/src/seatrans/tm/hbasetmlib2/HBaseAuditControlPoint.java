// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2014 Hewlett-Packard Development Company, L.P.
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

package org.trafodion.dtm;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.DataInputStream;
import java.io.IOException;
import java.io.ObjectOutputStream;

import org.apache.log4j.PropertyConfigurator;
import org.apache.log4j.Logger;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.fs.Path;
import org.apache.hadoop.hbase.client.HBaseAdmin;
import org.apache.hadoop.hbase.client.HTable;
import org.apache.hadoop.hbase.client.Get;
import org.apache.hadoop.hbase.client.Put;
import org.apache.hadoop.hbase.client.Delete;
import org.apache.hadoop.hbase.client.Result;
import org.apache.hadoop.hbase.client.ResultScanner;
import org.apache.hadoop.hbase.client.Scan;
import org.apache.hadoop.hbase.client.transactional.TransactionManager;
import org.apache.hadoop.hbase.client.transactional.TransactionState;
import org.apache.hadoop.hbase.client.transactional.CommitUnsuccessfulException;
import org.apache.hadoop.hbase.client.transactional.UnknownTransactionException;
import org.apache.hadoop.hbase.client.transactional.HBaseBackedTransactionLogger;
import org.apache.hadoop.hbase.client.transactional.TransactionRegionLocation;
import org.apache.hadoop.hbase.HBaseConfiguration;
import org.apache.hadoop.hbase.HColumnDescriptor;
import org.apache.hadoop.hbase.HRegionInfo;
import org.apache.hadoop.hbase.HRegionLocation;
import org.apache.hadoop.hbase.HTableDescriptor;
import org.apache.hadoop.hbase.KeyValue;
import org.apache.hadoop.hbase.TableExistsException;
import org.apache.hadoop.hbase.TableName;
import org.apache.hadoop.hbase.ZooKeeperConnectionException;
import org.apache.hadoop.hbase.util.Bytes;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.StringTokenizer;

import java.util.concurrent.ConcurrentHashMap;

import java.lang.NullPointerException;

public class HBaseAuditControlPoint {

    static final Log LOG = LogFactory.getLog(HBaseAuditControlPoint.class);
    private static long currControlPt;
    private static HBaseAdmin admin;
//    private static HBaseAuditContext tLogContext;
    private static String CONTROL_POINT_TABLE_NAME;
    private static final byte[] CONTROL_POINT_FAMILY = Bytes.toBytes("cpf");
    private static final byte[] ASN_HIGH_WATER_MARK = Bytes.toBytes("hwm");
    private static HTable table;
    private boolean useAutoFlush;
    private boolean disableBlockCache;

    public HBaseAuditControlPoint(Configuration config) throws IOException {
      if (LOG.isTraceEnabled()) LOG.trace("Enter HBaseAuditControlPoint constructor()");
      CONTROL_POINT_TABLE_NAME = config.get("CONTROL_POINT_TABLE_NAME");
      HTableDescriptor desc = new HTableDescriptor(TableName.valueOf(CONTROL_POINT_TABLE_NAME));
      HColumnDescriptor hcol = new HColumnDescriptor(CONTROL_POINT_FAMILY);

      disableBlockCache = false;
      try {
         String blockCacheString = System.getenv("TM_TLOG_DISABLE_BLOCK_CACHE");
         if (blockCacheString != null){
            disableBlockCache = (Integer.parseInt(blockCacheString) != 0);
            if (LOG.isDebugEnabled()) LOG.debug("disableBlockCache != null");
         }
      }
      catch (Exception e) {
         if (LOG.isDebugEnabled()) LOG.debug("TM_TLOG_DISABLE_BLOCK_CACHE is not in ms.env");
      }
      LOG.info("disableBlockCache is " + disableBlockCache);
      if (disableBlockCache) {
         hcol.setBlockCacheEnabled(false);
      }

      desc.addFamily(hcol);
      admin = new HBaseAdmin(config);

      useAutoFlush = true;
      try {
         String autoFlush = System.getenv("TM_TLOG_AUTO_FLUSH");
         if (autoFlush != null){
            useAutoFlush = (Integer.parseInt(autoFlush) != 0);
            if (LOG.isDebugEnabled()) LOG.debug("autoFlush != null");
         }
      }
      catch (Exception e) {
         if (LOG.isDebugEnabled()) LOG.debug("TM_TLOG_AUTO_FLUSH is not in ms.env");
      }
      LOG.info("useAutoFlush is " + useAutoFlush);

      boolean lvControlPointExists = admin.tableExists(CONTROL_POINT_TABLE_NAME);
      if (LOG.isDebugEnabled()) LOG.debug("HBaseAuditControlPoint lvControlPointExists " + lvControlPointExists);
      currControlPt = -1;
      try {
         if (LOG.isDebugEnabled()) LOG.debug("Creating the table " + CONTROL_POINT_TABLE_NAME);
         admin.createTable(desc);
         currControlPt = 1;
      }
      catch (TableExistsException e) {
         LOG.error("Table " + CONTROL_POINT_TABLE_NAME + " already exists");
      }

      try {
         if (LOG.isDebugEnabled()) LOG.debug("try new HTable");
         table = new HTable(config, desc.getName());
         table.setAutoFlushTo(this.useAutoFlush);
      }
      catch (IOException e) {
         LOG.error("new HTable IOException");
      }

      if (currControlPt == -1){
         try {
            currControlPt = getCurrControlPt();
         }
         catch (Exception e2) {
            if (LOG.isDebugEnabled()) LOG.debug("Exit getCurrControlPoint() exception " + e2);
         }
      }
      if (LOG.isDebugEnabled()) LOG.debug("currControlPt is " + currControlPt);

      if (LOG.isTraceEnabled()) LOG.trace("Exit constructor()");
      return;
    }

   public static long getCurrControlPt() throws Exception {
      if (LOG.isTraceEnabled()) LOG.trace("getCurrControlPt:  start");
      long highKey = -1;
      if (LOG.isDebugEnabled()) LOG.debug("new Scan");
      Scan s = new Scan();
      s.setCaching(10);
      s.setCacheBlocks(false);
      if (LOG.isDebugEnabled()) LOG.debug("resultScanner");
      ResultScanner ss = table.getScanner(s);
      try {
         long currKey;
         String rowKey;
         if (LOG.isDebugEnabled()) LOG.debug("entering for loop" );
         for (Result r : ss) {
            rowKey = new String(r.getRow());
            if (LOG.isDebugEnabled()) LOG.debug("rowKey is " + rowKey );
            currKey = Long.parseLong(rowKey);
            if (LOG.isDebugEnabled()) LOG.debug("value is " + Long.parseLong(Bytes.toString(r.value())));
            if (currKey > highKey) {
               if (LOG.isDebugEnabled()) LOG.debug("Setting highKey to " + currKey);
               highKey = currKey;
            }
         }
      }
      catch (Exception e) {
        LOG.error("getCurrControlPt IOException" + e);
        e.printStackTrace();
      } finally {
         ss.close();
      }
      if (LOG.isDebugEnabled()) LOG.debug("getCurrControlPt returning " + highKey);
      return highKey;
   }

   public long putRecord(final long ControlPt, final long startingSequenceNumber) throws Exception {
      if (LOG.isTraceEnabled()) LOG.trace("putRecord starting sequence number ("  + String.valueOf(startingSequenceNumber) + ")");
      String controlPtString = new String(String.valueOf(ControlPt));
      Put p = new Put(Bytes.toBytes(controlPtString));
      p.add(CONTROL_POINT_FAMILY, ASN_HIGH_WATER_MARK, Bytes.toBytes(String.valueOf(startingSequenceNumber)));
      try {
         if (LOG.isDebugEnabled()) LOG.debug("try table.put with starting sequence number " + startingSequenceNumber);
         table.put(p);
         if (useAutoFlush == false) {
            if (LOG.isDebugEnabled()) LOG.debug("flushing controlpoint record");
            table.flushCommits();
         }
      }
      catch (Exception e) {
         LOG.error("HBaseAuditControlPoint:putRecord Exception" + e);
         throw e;
      }
      if (LOG.isTraceEnabled()) LOG.trace("HBaseAuditControlPoint:putRecord returning " + ControlPt);
      return ControlPt;
   }

   public ArrayList<String> getRecordList(String controlPt) throws IOException {
      if (LOG.isTraceEnabled()) LOG.trace("getRecord");
      ArrayList<String> transactionList = new ArrayList<String>();
      Get g = new Get(Bytes.toBytes(controlPt));
      Result r = table.get(g);
      byte [] currValue = r.getValue(CONTROL_POINT_FAMILY, ASN_HIGH_WATER_MARK);
      String recordString = new String(currValue);
      if (LOG.isDebugEnabled()) LOG.debug("recordString is " + recordString);
      StringTokenizer st = new StringTokenizer(recordString, ",");
      while (st.hasMoreElements()) {
        String token = st.nextElement().toString() ;
        if (LOG.isDebugEnabled()) LOG.debug("token is " + token);
        transactionList.add(token);
      }

      if (LOG.isTraceEnabled()) LOG.trace("getRecord - exit with list size (" + transactionList.size() + ")");
      return transactionList;

    }

   public long getRecord(final String controlPt) throws IOException {
      if (LOG.isTraceEnabled()) LOG.trace("getRecord " + controlPt);
      long lvValue = -1;
      Get g = new Get(Bytes.toBytes(controlPt));
      String recordString;
      try {
         Result r = table.get(g);
         byte [] currValue = r.getValue(CONTROL_POINT_FAMILY, ASN_HIGH_WATER_MARK);
         try {
            recordString = new String (Bytes.toString(currValue));
            if (LOG.isDebugEnabled()) LOG.debug("recordString is " + recordString);
            lvValue = Long.parseLong(recordString, 10);
         }
         catch (NullPointerException e){
            if (LOG.isDebugEnabled()) LOG.debug("control point " + controlPt + " is not in the table");
         }
      }
      catch (IOException e){
          LOG.error("getRecord IOException");
          throw e;
      }
      if (LOG.isTraceEnabled()) LOG.trace("getRecord - exit " + lvValue);
      return lvValue;

    }

   public long getStartingAuditSeqNum() throws IOException {
      if (LOG.isTraceEnabled()) LOG.trace("getStartingAuditSeqNum");
      String controlPtString = new String(String.valueOf(currControlPt));
      long lvAsn;
      if (LOG.isDebugEnabled()) LOG.debug("getStartingAuditSeqNum new get for control point " + currControlPt);
      Get g = new Get(Bytes.toBytes(controlPtString));
      if (LOG.isDebugEnabled()) LOG.debug("getStartingAuditSeqNum setting result");
      Result r = table.get(g);
      if (LOG.isDebugEnabled()) LOG.debug("getStartingAuditSeqNum currValue CONTROL_POINT_FAMILY is "
                 + CONTROL_POINT_FAMILY + " ASN_HIGH_WATER_MARK " + ASN_HIGH_WATER_MARK);
      byte [] currValue = r.getValue(CONTROL_POINT_FAMILY, ASN_HIGH_WATER_MARK);
      if (LOG.isDebugEnabled()) LOG.debug("Starting asn setting recordString ");
      String recordString = "";
      try {
         recordString = new String(currValue);
      }
      catch (NullPointerException e) {
         if (LOG.isDebugEnabled()) LOG.debug("getStartingAuditSeqNum recordString is null");
         lvAsn = 1;
         if (LOG.isDebugEnabled()) LOG.debug("Starting asn is 1");
         return lvAsn;
      }
      if (LOG.isDebugEnabled()) LOG.debug("getStartingAuditSeqNum recordString is good");
      if (LOG.isDebugEnabled()) LOG.debug("Starting asn for control point " + currControlPt + " is " + recordString);
      lvAsn = Long.valueOf(recordString);
      if (LOG.isTraceEnabled()) LOG.trace("getStartingAuditSeqNum - exit returning " + lvAsn);
      return lvAsn;
    }

   public long doControlPoint(final long sequenceNumber) throws IOException {
      if (LOG.isTraceEnabled()) LOG.trace("doControlPoint start");
      try {
         currControlPt++;
         if (LOG.isDebugEnabled()) LOG.debug("doControlPoint interval (" + currControlPt + "), sequenceNumber (" + sequenceNumber+ ") try putRecord");
         putRecord(currControlPt, sequenceNumber);
      }
      catch (Exception e) {
         LOG.error("doControlPoint Exception" + e);
      }

      if (LOG.isTraceEnabled()) LOG.trace("doControlPoint - exit");
      return currControlPt;
   }

   public boolean deleteRecord(final long controlPoint) throws IOException {
      if (LOG.isTraceEnabled()) LOG.trace("deleteRecord start for control point " + controlPoint);
      String controlPtString = new String(String.valueOf(controlPoint));

      try {
         List<Delete> list = new ArrayList<Delete>();
         Delete del = new Delete(Bytes.toBytes(controlPtString));
         if (LOG.isDebugEnabled()) LOG.debug("deleteRecord  (" + controlPtString + ") ");
         table.delete(del);
      }
      catch (Exception e) {
         LOG.error("deleteRecord IOException");
      }

      if (LOG.isTraceEnabled()) LOG.trace("deleteRecord - exit");
      return true;
   }

   public boolean deleteAgedRecords(final long controlPoint) throws IOException {
      if (LOG.isTraceEnabled()) LOG.trace("deleteAgedRecords start - control point " + controlPoint);
      String controlPtString = new String(String.valueOf(controlPoint));

      Scan s = new Scan();
      s.setCaching(10);
      s.setCacheBlocks(false);
      ArrayList<Delete> deleteList = new ArrayList<Delete>();
      ResultScanner ss = table.getScanner(s);
      try {
         String rowKey;
         for (Result r : ss) {
            rowKey = new String(r.getRow());
            if (Long.parseLong(rowKey) < controlPoint) {
               if (LOG.isDebugEnabled()) LOG.debug("Adding  (" + rowKey + ") to delete list");
               Delete del = new Delete(rowKey.getBytes());
               deleteList.add(del);
            }
         }
         if (LOG.isDebugEnabled()) LOG.debug("attempting to delete list with " + deleteList.size() + " elements");
         table.delete(deleteList);
      }
      catch (Exception e) {
         LOG.error("deleteAgedRecords IOException");
      }finally {
         ss.close();
      }

      if (LOG.isTraceEnabled()) LOG.trace("deleteAgedRecords - exit");
      return true;
   }
}

