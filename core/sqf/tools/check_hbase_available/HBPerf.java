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
import org.apache.log4j.PropertyConfigurator;

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

public class HBPerf{

    private static final Log LOG = LogFactory.getLog(HBPerf.class);

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

    public static void writeRow() throws IOException {

      table.put(new Put(ROW1).add(FAMILY, QUAL_A, Bytes.toBytes(1)));

    }
	
    public static void testGetAfterPut2(int numtx) throws IOException {
        
        PrintWriter out = new PrintWriter(new File("TWorkTest.txt"));

        out.println("\nTransactionWithWorkTest ENTRY");

        double begin_latency 	= 0;
        double end_latency 	= 0;
        double put_latency 	= 0;
	double get_latency  	= 0;
	double delete_latency 	= 0;
	double abort_latency    = 0;
        int newValue 		= 0;

        double hput_latency	= 0;
	double hget_latency 	= 0;
	double hdelete_latency 	= 0;

        for (newValue = 0; newValue < numtx; newValue++)
        {   
             // Begin
             long ts1 = System.nanoTime();
             long ts2 = System.nanoTime();
             long latency = ts2-ts1;
             double latency2 = (double)latency/1000000000;

             begin_latency = begin_latency + latency2;

             // Put 
             ts1 = System.nanoTime();
             table.put(new Put(ROW1).add(FAMILY, QUAL_A, Bytes.toBytes(newValue)));
             ts2 = System.nanoTime();
  	     latency = ts2-ts1;
             latency2 = (double)latency/1000000000;

             put_latency = put_latency + latency2;

	     // Abort
	     System.nanoTime();

     ts2 = System.nanoTime();
             latency = ts2-ts1;
             latency2 = (double)latency/1000000000;

             abort_latency = abort_latency + latency2;


             table.put(new Put(ROW1).add(FAMILY, QUAL_A, Bytes.toBytes(newValue)));
             
             // End
             ts1 = System.nanoTime();
             ts2 = System.nanoTime();
             latency = ts2-ts1;
             latency2 = (double)latency/1000000000;

             end_latency = end_latency + latency2;
             
             table.put(new Put(ROW1).add(FAMILY, QUAL_A, Bytes.toBytes(newValue)));
              
             // Get
             ts1 = System.nanoTime();
	     Result row1_A = table.get( new Get(ROW1).addColumn(FAMILY, QUAL_A));
	     ts2 = System.nanoTime();
             latency = ts2-ts1;
             latency2 = (double)latency/1000000000;

             get_latency = get_latency + latency2;          

             int expected = Bytes.toInt(row1_A.getValue(FAMILY, QUAL_A));
             if (newValue != expected)
                  System.out.println("\ntestGetAfterPut : Assert 1 FAILED : expected " + expected + "newValue " + newValue + "\n");


             table.put(new Put(ROW1).add(FAMILY, QUAL_A, Bytes.toBytes(newValue)));     

             // Delete
             ts1 = System.nanoTime();
             table.delete(new Delete(ROW1).deleteColumns(FAMILY, QUAL_A));
             ts2 = System.nanoTime();
             latency = ts2-ts1;
             latency2 = (double)latency/1000000000;
            
	     delete_latency = delete_latency + latency2;

        }

        out.println("\nAverage Begin latency : " + (double)(begin_latency/newValue));
        out.println("Average End latency  : " + (double)(end_latency/newValue));
        out.println("Average Abort latency : " + (double)(abort_latency/newValue));
        out.println("Average Put latency  : " + (double)(put_latency/newValue));
        out.println("Average Get latency  : " + (double)(get_latency/newValue));
	out.println("Average Delete latency : " + (double)(delete_latency/newValue));
        out.println("Average HPut latency  : " + (double)(hput_latency/newValue));
        out.println("Average HGet latency  : " + (double)(hget_latency/newValue));
	out.println("\ntestGetAfterPut EXIT");
        out.close();
    }

    public static void testGetAfterPut_no_delete(int numtx) throws IOException {
        
        PrintWriter out = new PrintWriter(new File("TWorkTest_no_delete.txt"));

        out.println("\nTransactionWithWorkTest ENTRY");

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

             // Get
             ts1 = System.nanoTime();
	     Result row1_A = table.get( new Get(Bytes.toBytes(rk)).addColumn(FAMILY, QUAL_A));
	     ts2 = System.nanoTime();
             latency = ts2-ts1;
             latency2 = (double)latency/1000000000;

             get_latency = get_latency + latency2;          

             int expected = Bytes.toInt(row1_A.getValue(FAMILY, QUAL_A));
             if (newValue != expected)
                  System.out.println("\ntestGetAfterPut : Assert 1 FAILED : expected " + expected + "newValue " + newValue + "\n");

        }

        out.println("Average Put latency  : " + (double)(put_latency/newValue));
        out.println("Average Get latency  : " + (double)(get_latency/newValue));
	out.println("\ntestGetAfterPut EXIT");
        out.close();
    }

    public static void main(String[] Args) {

      int numtx = 0;

      System.out.println("In the Main\n");

      try {
        numtx = Integer.parseInt(Args[0]);

        initialize();
        writeRow();
	//        testGetAfterPut2(numtx);
        testGetAfterPut_no_delete(numtx);
        System.out.println("\n Test DONE\n");
      }
      catch (Exception e) {
	  System.out.println(e);
      }
   }
}
