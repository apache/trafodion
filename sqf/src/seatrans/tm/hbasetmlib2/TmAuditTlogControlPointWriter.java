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
import java.io.DataInputStream;
import java.io.IOException;

import org.apache.log4j.PropertyConfigurator;
import org.apache.log4j.Logger;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.fs.Path;
import org.apache.hadoop.hbase.HBaseConfiguration;
import org.apache.hadoop.hbase.client.HTable;
import org.apache.hadoop.hbase.client.Get;
import org.apache.hadoop.hbase.KeyValue;
import org.apache.hadoop.hbase.client.Put;
import org.apache.hadoop.hbase.client.Delete;
import org.apache.hadoop.hbase.client.Result;
import org.apache.hadoop.hbase.client.Scan;
import org.apache.hadoop.hbase.client.transactional.TransactionManager;
import org.apache.hadoop.hbase.client.transactional.TransactionState;
import org.apache.hadoop.hbase.client.transactional.CommitUnsuccessfulException;
import org.apache.hadoop.hbase.client.transactional.UnknownTransactionException;
import org.apache.hadoop.hbase.client.transactional.HBaseBackedTransactionLogger;
import org.apache.hadoop.hbase.client.transactional.TransactionRegionLocation;
import org.apache.hadoop.hbase.ipc.TransactionalRegionInterface;
import org.apache.hadoop.hbase.HRegionInfo;
import org.apache.hadoop.hbase.HRegionLocation;
import org.apache.hadoop.hbase.HTableDescriptor;
import org.apache.hadoop.hbase.TableExistsException;
import org.apache.hadoop.hbase.HColumnDescriptor;
import org.apache.hadoop.hbase.client.HBaseAdmin;
import org.apache.hadoop.hbase.util.Bytes;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.StringTokenizer;

import java.util.concurrent.ArrayBlockingQueue;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.Callable;
import java.util.concurrent.ConcurrentHashMap;

public class TmAuditTlogControlPointWriter implements Callable<Boolean> {

   static final Log LOG = LogFactory.getLog(TmAuditTlogControlPointWriter.class);
   static TmAuditTlog tLog;
   static HBaseAuditControlPoint controlPoint;
   static ConcurrentHashMap<Long, TransactionState> map;
   private static long   startTime;

   public TmAuditTlogControlPointWriter (TmAuditTlog tLog, final ConcurrentHashMap<Long, TransactionState> map ){

      LOG.trace("Enter TmAuditTlogControlPointWriter constructor.  Thread: " +  Thread.currentThread().getId());
      this.tLog = tLog;
      this.controlPoint = controlPoint;
      this.map = map;
      startTime = System.nanoTime();

      LOG.trace("Exit constructor()");
      return;
   }

   @Override
   public Boolean call() throws IOException, InterruptedException, Exception {
      LOG.trace("TmAuditTlogControlPointWriter in call()");
      boolean lvSuccess = false;
      try {
         long threadId = Thread.currentThread().getId();
         LOG.debug("TmAuditTlogControlPointWriter thread " + threadId + " still running with map size " + map.size());
         ArrayList<Put> lvBuffer = new ArrayList<Put>();
         for (Map.Entry<Long, TransactionState> e : map.entrySet()) {
            try {
               Long transid = e.getKey();
               TransactionState value = e.getValue();
               if (value.getStatus().compareTo("COMMITTED") == 0){
                  LOG.debug("TmAuditTlogControlPointWriter formatting trans state record for trans (" + transid + ") : state is " + value.getStatus());
                  Put lvPut = tLog.formatRecord(transid, value);
                  lvBuffer.add(lvPut);
               }
               else {
                  LOG.debug("TmAuditTlogControlPointWriter skipping trans state record for trans (" + transid + ") : state is " + value.getStatus());
               }
            }
            catch (Exception ex) {
               LOG.error("TmAuditTlogControlPointWriter formatRecord Exception " + ex);
               throw ex;
            }
         }

         try {
            LOG.error("TmAuditTlogControlPointWriter putting buffer of size " + lvBuffer.size());
            lvSuccess = tLog.putBuffer(lvBuffer, startTime);
         }
         catch (Exception e) {
            LOG.error("TmAuditTlogControlPointWriter Exception in Tlog.putBuffer " + e);
            throw e;
         }
      }
      catch (Exception e) {
         LOG.error("TmAuditTlogControlPointWriter Exception  creating the Buffer " + e);
         throw e;
      }

//           for (Map.Entry<Long, TransactionState> e : map.entrySet()) {
//              try {
//                 Long transid = e.getKey();
//                 TransactionState value = e.getValue();
//                 if (value.getStatus().compareTo("COMMITTED") == 0){
//                    LOG.debug("TmAuditTlogControlPointWriter writing trans state record for trans (" + transid + ") : state is " + value.getStatus());
//                    tLog.putRecord(transid, value.getStatus(), value.getParticipatingRegions());
//                 }
//                 else {
//                    LOG.debug("TmAuditTlogControlPointWriter skipping trans state record for trans (" + transid + ") : state is " + value.getStatus());
//                 }
//              }
//              catch (Exception ex) {
//                 LOG.error("TmAuditTlogControlPointWriter puting record Exception");
//                 throw ex;
//              }
//           }
      LOG.debug("TmAuditTlogControlPointWriter completed putBuffer");
//        } catch (Exception e) {
//           LOG.error("TmAuditTlogControlPointWriter putBuffer exception " + e);
//           throw e;
//        } //catch (InterruptedException e) {
//           LOG.debug("TmAuditTlogControlPointWriter child interrupted.");
//           throw e;
//        }
         // All state records are written, now update the conrol point number and table
      try {
        long lvAsn = tLog.asnGetAndIncrement();
        long lvCtrlPt = controlPoint.doControlPoint(lvAsn);
        LOG.debug("TmAuditTlogControlPointWriter tLogControlPoint.doControlPoint returned " + lvCtrlPt + " asn is " + lvAsn);
        if ((lvCtrlPt - 5) > 0) {
           try {
              LOG.debug("TmAuditTlogControlPointWriter calling tLogControlPoint.getRecord " + (lvCtrlPt - 5));
              long agedAsn = controlPoint.getRecord(String.valueOf(lvCtrlPt - 5));
              if (agedAsn > 0){
                 try {
                    LOG.debug("TmAuditTlogControlPointWriter attempting to remove TLOG writes older than asn " + agedAsn);
                    tLog.deleteAgedEntries(agedAsn);
                 }
                 catch (Exception e){
                    LOG.error("deleteAgedEntries Exception " + e);
                    throw e;
                 }
                 try {
                    LOG.debug("TmAuditTlogControlPointWriter - removing control point record " + (lvCtrlPt - 5));
                    controlPoint.deleteAgedRecords(lvCtrlPt - 5);
                 }
                 catch (Exception e){
                    LOG.debug("TmAuditTlogControlPointWriter - control point record not found ");
                 }
              } // ASN > 0
           }
           catch (Exception e){
              LOG.error("TmAuditTlogControlPointWriter tLogControlPoint.getRecord Exception " + e);
              throw e;
           }
        } // if ((lvCtrlPt - 5) > 0) {
     }
     catch (Exception e){
        LOG.error("TmAuditTlogControlPointWriter tLog.asnGetAndIncrement Exception " + e);
        throw e;
     }
     return Boolean.TRUE;
   }
}
