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
import org.apache.hadoop.hbase.HBaseConfiguration;
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
import org.apache.hadoop.hbase.client.HBaseAdmin;
import org.apache.hadoop.hbase.HRegionInfo;
import org.apache.hadoop.hbase.HRegionLocation;
import org.apache.hadoop.hbase.HTableDescriptor;
import org.apache.hadoop.hbase.TableExistsException;
import org.apache.hadoop.hbase.ZooKeeperConnectionException;
import org.apache.hadoop.hbase.HColumnDescriptor;
import org.apache.hadoop.hbase.KeyValue;
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
    private static boolean useAutoFlush;
    private static boolean disableBlockCache;

    public HBaseAuditControlPoint(Configuration config) throws IOException {
      LOG.trace("Enter HBaseAuditControlPoint constructor()");
      CONTROL_POINT_TABLE_NAME = config.get("CONTROL_POINT_TABLE_NAME");
      HTableDescriptor desc = new HTableDescriptor(CONTROL_POINT_TABLE_NAME);
      HColumnDescriptor hcol = new HColumnDescriptor(CONTROL_POINT_FAMILY);

      disableBlockCache = false;
      try {
         String blockCacheString = System.getenv("TM_TLOG_DISABLE_BLOCK_CACHE");
         if (blockCacheString != null){
            disableBlockCache = (Integer.parseInt(blockCacheString) != 0);
            LOG.debug("disableBlockCache != null");
         }
      }
      catch (Exception e) {
         LOG.debug("TM_TLOG_DISABLE_BLOCK_CACHE is not in ms.env");
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
            LOG.debug("autoFlush != null");
         }
      }
      catch (Exception e) {
         LOG.debug("TM_TLOG_AUTO_FLUSH is not in ms.env");
      }
      LOG.info("useAutoFlush is " + useAutoFlush);

      boolean lvControlPointExists = admin.tableExists(CONTROL_POINT_TABLE_NAME);
      LOG.debug("HBaseAuditControlPoint lvControlPointExists " + lvControlPointExists);
      currControlPt = -1;
      try {
         LOG.debug("Creating the table " + CONTROL_POINT_TABLE_NAME);
         admin.createTable(desc);
         currControlPt = 1;
      }
      catch (TableExistsException e) {
         LOG.error("Table " + CONTROL_POINT_TABLE_NAME + " already exists");
      }

      try {
         LOG.debug("try new HTable");
         table = new HTable(config, desc.getName());
         table.setAutoFlush(this.useAutoFlush);
      }
      catch (IOException e) {
         LOG.error("new HTable IOException");
      }

      if (currControlPt == -1){
         try {
            currControlPt = getCurrControlPt();
         }
         catch (Exception e2) {
            LOG.debug("Exit getCurrControlPoint() exception " + e2);
         }
      }
      LOG.debug("currControlPt is " + currControlPt);

      LOG.trace("Exit constructor()");
      return;
    }

   public static long getCurrControlPt() throws Exception {
      LOG.trace("getCurrControlPt:  start");
      long highKey = -1;
      LOG.debug("new Scan");
      Scan s = new Scan();
      s.setCaching(10);
      s.setCacheBlocks(false);
      LOG.debug("resultScanner");
      ResultScanner ss = table.getScanner(s);
      try {
         long currKey;
         String rowKey;
         LOG.debug("entering for loop" );
         for (Result r : ss) {
            rowKey = new String(r.getRow());
            LOG.debug("rowKey is " + rowKey );
            currKey = Long.parseLong(rowKey);
            LOG.debug("value is " + Long.parseLong(Bytes.toString(r.value())));
            if (currKey > highKey) {
               LOG.debug("Setting highKey to " + currKey);
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
      LOG.debug("getCurrControlPt returning " + highKey);
      return highKey;
   }

   public static long putRecord(final long ControlPt, final long startingSequenceNumber) throws Exception {
      LOG.trace("putRecord starting sequence number ("  + String.valueOf(startingSequenceNumber) + ")");
      String controlPtString = new String(String.valueOf(ControlPt));
      Put p = new Put(Bytes.toBytes(controlPtString));
      p.add(CONTROL_POINT_FAMILY, ASN_HIGH_WATER_MARK, Bytes.toBytes(String.valueOf(startingSequenceNumber)));
      try {
         LOG.debug("try table.put with starting sequence number " + startingSequenceNumber);
         table.put(p);
         if (useAutoFlush == false) {
            LOG.debug("flushing controlpoint record");
            table.flushCommits();
         }
      }
      catch (Exception e) {
         LOG.error("HBaseAuditControlPoint:putRecord Exception" + e);
         throw e;
      }
      LOG.trace("HBaseAuditControlPoint:putRecord returning " + ControlPt);
      return ControlPt;
   }

   public static ArrayList<String> getRecordList(String controlPt) throws IOException {
      LOG.trace("getRecord");
      ArrayList<String> transactionList = new ArrayList<String>();
      Get g = new Get(Bytes.toBytes(controlPt));
      Result r = table.get(g);
      byte [] currValue = r.getValue(CONTROL_POINT_FAMILY, ASN_HIGH_WATER_MARK);
      String recordString = new String(currValue);
      LOG.debug("recordString is " + recordString);
      StringTokenizer st = new StringTokenizer(recordString, ",");
      while (st.hasMoreElements()) {
        String token = st.nextElement().toString() ;
        LOG.debug("token is " + token);
        transactionList.add(token);
      }

      LOG.trace("getRecord - exit with list size (" + transactionList.size() + ")");
      return transactionList;

    }

   public static long getRecord(final String controlPt) throws IOException {
      LOG.trace("getRecord " + controlPt);
      long lvValue = -1;
      Get g = new Get(Bytes.toBytes(controlPt));
      String recordString;
      try {
         Result r = table.get(g);
         byte [] currValue = r.getValue(CONTROL_POINT_FAMILY, ASN_HIGH_WATER_MARK);
         try {
            recordString = new String (Bytes.toString(currValue));
            LOG.debug("recordString is " + recordString);
            lvValue = Long.parseLong(recordString, 10);
         }
         catch (NullPointerException e){
            LOG.debug("control point " + controlPt + " is not in the table");
         }
      }
      catch (IOException e){
          LOG.error("getRecord IOException");
          throw e;
      }
      LOG.trace("getRecord - exit " + lvValue);
      return lvValue;

    }

   public static long getStartingAuditSeqNum() throws IOException {
      LOG.trace("getStartingAuditSeqNum");
      String controlPtString = new String(String.valueOf(currControlPt));
      long lvAsn;
      LOG.debug("getStartingAuditSeqNum new get for control point " + currControlPt);
      Get g = new Get(Bytes.toBytes(controlPtString));
      LOG.debug("getStartingAuditSeqNum setting result");
      Result r = table.get(g);
      LOG.debug("getStartingAuditSeqNum currValue CONTROL_POINT_FAMILY is "
                 + CONTROL_POINT_FAMILY + " ASN_HIGH_WATER_MARK " + ASN_HIGH_WATER_MARK);
      byte [] currValue = r.getValue(CONTROL_POINT_FAMILY, ASN_HIGH_WATER_MARK);
      LOG.debug("Starting asn setting recordString ");
      String recordString = "";
      try {
         recordString = new String(currValue);
      }
      catch (NullPointerException e) {
         LOG.debug("getStartingAuditSeqNum recordString is null");
         lvAsn = 1;
         LOG.debug("Starting asn is 1");
         return lvAsn;
      }
      LOG.debug("getStartingAuditSeqNum recordString is good");
      LOG.debug("Starting asn for control point " + currControlPt + " is " + recordString);
      lvAsn = Long.valueOf(recordString);
      LOG.trace("getStartingAuditSeqNum - exit returning " + lvAsn);
      return lvAsn;
    }

   public static long doControlPoint(final long sequenceNumber) throws IOException {
      LOG.trace("doControlPoint start");
      try {
         currControlPt++;
         LOG.debug("doControlPoint interval (" + currControlPt + "), sequenceNumber (" + sequenceNumber+ ") try putRecord");
         putRecord(currControlPt, sequenceNumber);
      }
      catch (Exception e) {
         LOG.error("doControlPoint Exception" + e);
      }

      LOG.trace("doControlPoint - exit");
      return currControlPt;
   }

   public static boolean deleteRecord(final long controlPoint) throws IOException {
      LOG.trace("deleteRecord start for control point " + controlPoint);
      String controlPtString = new String(String.valueOf(controlPoint));

      try {
         List<Delete> list = new ArrayList<Delete>();
         Delete del = new Delete(Bytes.toBytes(controlPtString));
         LOG.debug("deleteRecord  (" + controlPtString + ") ");
         table.delete(del);
      }
      catch (Exception e) {
         LOG.error("deleteRecord IOException");
      }

      LOG.trace("deleteRecord - exit");
      return true;
   }

   public static boolean deleteAgedRecords(final long controlPoint) throws IOException {
      LOG.trace("deleteAgedRecords start - control point " + controlPoint);
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
               LOG.debug("Adding  (" + rowKey + ") to delete list");
               Delete del = new Delete(rowKey.getBytes());
               deleteList.add(del);
            }
         }
         LOG.debug("attempting to delete list with " + deleteList.size() + " elements");
         table.delete(deleteList);
      }
      catch (Exception e) {
         LOG.error("deleteAgedRecords IOException");
      }finally {
         ss.close();
      }

      LOG.trace("deleteAgedRecords - exit");
      return true;
   }
}

