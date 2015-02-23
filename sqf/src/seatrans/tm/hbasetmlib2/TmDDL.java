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


package org.trafodion.dtm;

import java.io.IOException;

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
import org.apache.hadoop.hbase.HColumnDescriptor;
import org.apache.hadoop.hbase.TableExistsException;
import org.apache.hadoop.hbase.TableName;
import org.apache.hadoop.hbase.HRegionInfo;

public class TmDDL {

   static final Log LOG = LogFactory.getLog(TmDDL.class);
   private HBaseAdmin hbadmin;
   private Configuration config;
   private int dtmid;

   public TmDDL (Configuration config) throws IOException, RuntimeException {

      this.config = config;
      this.dtmid = Integer.parseInt(config.get("dtmid"));
      if (LOG.isTraceEnabled()) LOG.trace("Enter TmDDL constructor for dtmid: " + dtmid);

      try {
         hbadmin = new HBaseAdmin(config);
      } catch(Exception e) {
         LOG.error("Unable to obtain HBaseAdmin accessor, exiting with exception: " + e);
         e.printStackTrace();
         System.exit(1);
      }

      boolean tDDLTableExists = hbadmin.tableExists("TRAFODION._DTM_.TDDL");

      if(tDDLTableExists==false && dtmid ==0) {
         try {
            TableName tablename = TableName.valueOf("TRAFODION._DTM_.TDDL");
            HTableDescriptor desc = new HTableDescriptor(tablename);
            desc.addFamily(new HColumnDescriptor("cf1"));
            hbadmin.createTable(desc);
         } catch(Exception e) {
            LOG.error("Unable to create TDDL table, exception: " + e);
            e.printStackTrace();
            throw e;
         }
      }
   }
}
