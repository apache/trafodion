/**
* @@@ START COPYRIGHT @@@
*
* Licensed to the Apache Software Foundation (ASF) under one
* or more contributor license agreements.  See the NOTICE file
* distributed with this work for additional information
* regarding copyright ownership.  The ASF licenses this file
* to you under the Apache License, Version 2.0 (the
* "License"); you may not use this file except in compliance
* with the License.  You may obtain a copy of the License at
*
*   http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing,
* software distributed under the License is distributed on an
* "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
* KIND, either express or implied.  See the License for the
* specific language governing permissions and limitations
* under the License.
*
* @@@ END COPYRIGHT @@@
**/

import java.io.IOException;
import java.sql.Timestamp;
import java.util.Date;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.hbase.HBaseConfiguration;
import org.apache.hadoop.hbase.HColumnDescriptor;
import org.apache.hadoop.hbase.HTableDescriptor;
import org.apache.hadoop.hbase.TableExistsException;
import org.apache.hadoop.hbase.client.*;
import org.apache.hadoop.hbase.client.Delete;
import org.apache.hadoop.hbase.client.Get;
import org.apache.hadoop.hbase.client.HBaseAdmin;
import org.apache.hadoop.hbase.client.Put;
import org.apache.hadoop.hbase.client.Result;
import org.apache.hadoop.hbase.client.ResultScanner;
import org.apache.hadoop.hbase.client.Scan;
import org.apache.hadoop.hbase.client.Result;
import org.apache.hadoop.hbase.client.ResultScanner;
import org.apache.hadoop.hbase.util.Bytes;
import java.io.PrintWriter;
import java.io.File;

import org.apache.log4j.PropertyConfigurator;

public class HBPerfWrite{

    private static final Log LOG = LogFactory.getLog(HBPerfWrite.class);

    private static final String TABLE_NAME = "table_t1";

    private static final byte[] FAMILY = Bytes.toBytes("family");
    private static final byte[] QUAL_A = Bytes.toBytes("a");
    private static final byte[] QUAL_B = Bytes.toBytes("b");

    private static final byte[] ROW1 = Bytes.toBytes("row1");
    private static final byte[] ROW2 = Bytes.toBytes("row2");
    private static final byte[] ROW3 = Bytes.toBytes("row3");

    private static HBaseAdmin admin;
    private static HTable table;


    static void setupLog4j() {
        System.out.println("In setupLog4J");
        System.setProperty("hostName", System.getenv("HOSTNAME"));      
        String confFile = System.getenv("PWD")
            + "/log4j.util.config";
        PropertyConfigurator.configure(confFile);
    }

    // Initialize and set up tables 
    public static void initialize() throws Exception {

	setupLog4j();

	Configuration config = HBaseConfiguration.create();

        HTableDescriptor desc = new HTableDescriptor(TABLE_NAME);
        desc.addFamily(new HColumnDescriptor(FAMILY));
        admin = new HBaseAdmin(config);
	try {
            System.out.println ("Creating the table " + TABLE_NAME);
	    admin.createTable(desc);
	}
	catch (TableExistsException e) {
            System.out.println("Table " + TABLE_NAME + " already exists");
        }

        table = new HTable(config, desc.getName());
    }

    public static void testPut(int numtx) throws IOException {
        
        PrintWriter out = new PrintWriter(new File("TWorkTest_Put.txt"));

        out.println("\ntestPut ENTRY");

        double put_latency 	= 0;
	double get_latency  	= 0;
        int newValue 		= 0;

        for (newValue = 0; newValue < numtx; newValue++)
        {   
             // Put 
	     long ts1 = System.nanoTime();
	     String rk = "row" + newValue;
             table.put(new Put(Bytes.toBytes(rk)).add(FAMILY, QUAL_A, Bytes.toBytes(newValue)));
             long ts2 = System.nanoTime();
  	     long latency = ts2-ts1;
             double latency2 = (double)latency/1000000000;

             put_latency = put_latency + latency2;
        }

        out.println("Average Put latency  : " + (double)(put_latency/newValue));
	out.println("\ntestPut EXIT");
        out.close();
    }

    public static void main(String[] Args) {

      int numtx = 0;

      System.out.println("In the Main\n");

      try {
        numtx = Integer.parseInt(Args[0]);

        initialize();
        testPut(numtx);
        System.out.println("\n Test DONE\n");
      }
      catch (Exception e) {
	  System.out.println(e);
      }
   }
}
