// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2015 Hewlett-Packard Development Company, L.P.
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


package org.apache.hadoop.hbase.client.transactional;

import java.io.IOException;
import java.util.List;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.StringTokenizer;
import java.io.ByteArrayInputStream;


import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.hbase.HBaseConfiguration;
import org.apache.hadoop.hbase.client.HBaseAdmin;
import org.apache.hadoop.hbase.client.HConnection;
import org.apache.hadoop.hbase.client.HConnectionManager;
import org.apache.hadoop.hbase.client.HTable;
import org.apache.hadoop.hbase.client.HTableInterface;
import org.apache.hadoop.hbase.HTableDescriptor;
import org.apache.hadoop.hbase.client.Get;
import org.apache.hadoop.hbase.client.Put;
import org.apache.hadoop.hbase.client.Delete;
import org.apache.hadoop.hbase.client.Result;
import org.apache.hadoop.hbase.HColumnDescriptor;
import org.apache.hadoop.hbase.TableExistsException;
import org.apache.hadoop.hbase.TableName;
import org.apache.hadoop.hbase.HRegionInfo;
import org.apache.hadoop.hbase.util.Bytes;


public class TmDDL {

   static final Log LOG = LogFactory.getLog(TmDDL.class);
   private HBaseAdmin hbadmin;
   private Configuration config;
   private int dtmid;
   private static final byte[] TDDL_FAMILY = Bytes.toBytes("tddlcf");
   private static final byte[] TDDL_CREATE = Bytes.toBytes("createList");
   private static final byte[] TDDL_DROP = Bytes.toBytes("dropList");
   private static final byte[] TDDL_TRUNCATE = Bytes.toBytes("truncateList");
   private static final byte[] TDDL_STATE = Bytes.toBytes("state");
   private static Object tablePutLock;            // Lock for synchronizing table.put operations
   private static HTable table;

   public TmDDL (Configuration config) throws Exception {

      this.config = config;
      this.dtmid = Integer.parseInt(config.get("dtmid"));
      TableName tablename = TableName.valueOf("TRAFODION._DTM_.TDDL");

      if (LOG.isTraceEnabled()) LOG.trace("Enter TmDDL constructor for dtmid: " + dtmid);

      try {
         hbadmin = new HBaseAdmin(config);
      } catch(Exception e) {
         LOG.error("Unable to obtain HBaseAdmin accessor, exiting with exception: " + e);
         e.printStackTrace();
         System.exit(1);
      }

      boolean tDDLTableExists = hbadmin.tableExists(tablename);

      if(tDDLTableExists==false && dtmid ==0) {
         try {
            HTableDescriptor desc = new HTableDescriptor(tablename);
            desc.addFamily(new HColumnDescriptor(TDDL_FAMILY));
            hbadmin.createTable(desc);
         } catch(Exception e) {
            LOG.error("Unable to create TDDL table, exception: " + e);
            e.printStackTrace();
            throw e;
         }
      }

      tablePutLock = new Object();

      table = new HTable(config, tablename);
   }

   public void putRow(final long transid, final String Operation, final String tableName) throws Exception {

        long threadId = Thread.currentThread().getId();
        if (LOG.isTraceEnabled()) LOG.trace("TmDDL putRow Operation, TxID: " + transid + "Thread ID:" + threadId 
                + "TableName:" + tableName + "Operation :" + Operation);
        byte [] value = null;
        StringBuilder tableString = null;
        Result r = null;
        Put p = new Put(Bytes.toBytes(transid));


        //Retrieve the row if it exists so we can append.
        Get g = new Get(Bytes.toBytes(transid));
        try {
                r = table.get(g);
        }
        catch(Exception e){
          LOG.error("TmDDL putRow method, Get Exception TxID:" + transid + "Exception:" + e);
          throw e;
        }

        //Check and set State
        if(! r.isEmpty())
        {
            value = r.getValue(TDDL_FAMILY, TDDL_STATE);
        }
        if((value != null) && (value.length > 0) && Bytes.toString(value).equals("INVALID"))
        {
            LOG.error("TmDDL putRow on invalid Transaction TxID:" + transid);
            throw new RuntimeException("TmDDL putRow on invalid Transaction State TxID :" + transid);
        }
        else
        {
            p.add(TDDL_FAMILY, TDDL_STATE, Bytes.toBytes("VALID"));
        }

        //Check and append table name
        if(Operation.equals("CREATE"))
        {
            if(! r.isEmpty())
            {
                value = r.getValue(TDDL_FAMILY, TDDL_CREATE);
            }
            if((value != null) && (value.length > 0) && Bytes.toString(value).equals("INVALID"))
            {
                LOG.error("TmDDL putRow on invalid Transaction TxID:" + transid);
                throw new RuntimeException("putRow on invalid Transaction State TxID:" + transid);
            }
            else
            {
                tableString = new StringBuilder();
                if(value != null && value.length > 0)
                {
                    tableString.append(Bytes.toString(value));
                    tableString.append(",");
                }
                tableString.append(tableName);
                p.add(TDDL_FAMILY, TDDL_CREATE, Bytes.toBytes(tableString.toString()));
            }

        }

        // Check and append table name
        if(Operation.equals("TRUNCATE"))
        {
            if(! r.isEmpty())
            {
                value = r.getValue(TDDL_FAMILY, TDDL_TRUNCATE);
            }
            if((value != null) && (value.length > 0) && Bytes.toString(value).equals("INVALID"))
            {
                LOG.error("putRow on invalid Transaction, TxID :" + transid);
                throw new RuntimeException("putRow on invalid Transaction State :" + transid);
            }
            else
            {
                tableString = new StringBuilder();
                if(value != null && value.length > 0)
                {
                    tableString.append(Bytes.toString(value));
                    tableString.append(",");
                }
                tableString.append(tableName);
                p.add(TDDL_FAMILY, TDDL_TRUNCATE, Bytes.toBytes(tableString.toString()));
            }
        }

        //Check and append table name
        if(Operation.equals("DROP"))
        {
            if(! r.isEmpty())
            {
                value = r.getValue(TDDL_FAMILY, TDDL_DROP);
            }
            if((value != null) && (value.length > 0) && Bytes.toString(value).equals("INVALID"))
            {
                LOG.error("TmDDL putRow invalid Transaction state, TxID :" + transid);
                throw new RuntimeException("putRow on invalid Transaction State :" + transid);
            }
            else
            {
                tableString = new StringBuilder();
                if(value != null && value.length > 0)
                {
                    tableString.append(Bytes.toString(value));
                    tableString.append(",");
                }
                tableString.append(tableName);
                p.add(TDDL_FAMILY, TDDL_DROP, Bytes.toBytes(tableString.toString()));
            }

        }

        try {
            synchronized (tablePutLock) {
            try {
                  if (LOG.isTraceEnabled()) LOG.trace("TmDDL table.put, TxID: " + transid + "Put :" + p );

                  table.put(p);
               }
               catch (Exception e2){
                  //Avoiding logging within a lock. Throwing Exception.
                  throw e2;
               }
            } // End global synchronization
         }
         catch (Exception e) {
             //create record of the exception
            LOG.error("TmDDL tablePutLock or Table.put Exception, TxID: " + transid + "Exception:" + e);
            throw e;
         }
      if (LOG.isTraceEnabled()) LOG.trace("TmDDL putRow exit, TxId:" + transid);
   }

   public void setState(final long transid, final String state) throws Exception {
      long threadId = Thread.currentThread().getId();
      if (LOG.isTraceEnabled()) LOG.trace("TmDDL setState start in thread: " + threadId + "TxId:" + transid + "State :" + state);

      Put p = new Put(Bytes.toBytes(transid));
      p.add(TDDL_FAMILY, TDDL_STATE, Bytes.toBytes(state));
      try {
            synchronized (tablePutLock) {
          try {
                if (LOG.isTraceEnabled()) LOG.trace("TmDDL setState method. table.put. TxId: " + transid + "Put:" + p );

                table.put(p);
             }
             catch (Exception e2){
                //Avoiding logging within a lock. Throwing Exception.
                throw e2;
             }
          } // End global synchronization
       }
       catch (Exception e) {
           //create record of the exception
          LOG.error("TmDDL setState method. tablePutLock or Table.put Exception. TxID: " + transid + "Exception :" + e);
          throw e;
       }
      if (LOG.isTraceEnabled()) LOG.trace("TmDDL setState exit, TxID:" + transid);
   }

   public void getRow(final long lvTransid, StringBuilder state, ArrayList<String> createList, ArrayList<String> dropList, ArrayList<String> truncateList)
              throws IOException, Exception {
      if (LOG.isTraceEnabled()) LOG.trace("TmDDL getRow start, TxID: " + lvTransid);
      String recordString = null;
      StringTokenizer st = null;
      byte [] value = null;
      try {
            Get g = new Get(Bytes.toBytes(lvTransid));
            Result r = table.get(g);

            if(! r.isEmpty())
            {
                value = r.getValue(TDDL_FAMILY, TDDL_CREATE);
                if(value != null && value.length > 0)
                {
                    recordString =  new String (Bytes.toString(value));
                    st = new StringTokenizer(recordString, ",");

                    while (st.hasMoreElements())
                    {
                        createList.add(st.nextToken());
                    }
                }

                value = r.getValue(TDDL_FAMILY, TDDL_DROP);
                if(value != null && value.length > 0)
                {
                    recordString =  new String (Bytes.toString(value));
                    st = new StringTokenizer(recordString, ",");
                    while (st.hasMoreElements())
                    {
                        dropList.add(st.nextToken());
                    }
                }

                value = r.getValue(TDDL_FAMILY, TDDL_TRUNCATE);
                if(value != null && value.length > 0)
                {
                    recordString =  new String (Bytes.toString(value));
                    st = new StringTokenizer(recordString, ",");
                    while (st.hasMoreElements())
                    {
                        truncateList.add(st.nextToken());
                    }
                }

                value = r.getValue(TDDL_FAMILY, TDDL_STATE);
                if(value != null && value.length > 0)
                {
                    state.append(Bytes.toString(value));
                }
            }

        }
        catch(Exception e){
          LOG.error("TmDDL getRow Exception, TxId: " + lvTransid + "Exception:" + e);
          throw e;
        }
    }

    public void getState(final long lvTransid, StringBuilder state) throws IOException, Exception {
      if (LOG.isTraceEnabled()) LOG.trace("TmDDL getState start, TxID:" + lvTransid);
      byte [] value = null;
      try {
            Get g = new Get(Bytes.toBytes(lvTransid));
            Result r = table.get(g);

            if(! r.isEmpty())
            {
                value = r.getValue(TDDL_FAMILY, TDDL_STATE);
                if(value != null && value.length > 0)
                {
                    state.append(Bytes.toString(value));
                }
                else
                {
                    state.append("INVALID");
                }
            }
            else
            {
                state.append("INVALID");
            }
       }
        catch(Exception e){
          LOG.error("TmDDL getState Exception, TxID: " + lvTransid + "Exception: " + e);
          throw e;
        }
    }

    public void deleteRow(final long lvTransid) throws IOException, Exception {
      if (LOG.isTraceEnabled()) LOG.trace("TmDDL deleteRow start, TxID: " + lvTransid);
      try {
            Delete d = new Delete(Bytes.toBytes(lvTransid));
            table.delete(d);
        }
        catch(Exception e){
          LOG.error("TmDDL deleteRow Exception, TxID: " + lvTransid + "Exception:" + e);
          throw e;
        }
    }
 }
