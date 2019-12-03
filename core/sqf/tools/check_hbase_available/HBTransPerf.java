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

import org.apache.log4j.PropertyConfigurator;
import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.hbase.HBaseConfiguration;
import org.apache.hadoop.hbase.HColumnDescriptor;
import org.apache.hadoop.hbase.HTableDescriptor;
import org.apache.hadoop.hbase.TableExistsException;
import org.apache.hadoop.hbase.client.Delete;
import org.apache.hadoop.hbase.client.Get;
import org.apache.hadoop.hbase.client.HBaseAdmin;
import org.apache.hadoop.hbase.client.Put;
import org.apache.hadoop.hbase.client.Result;
import org.apache.hadoop.hbase.client.ResultScanner;
import org.apache.hadoop.hbase.client.Scan;
import org.apache.hadoop.hbase.client.Result;
import org.apache.hadoop.hbase.client.ResultScanner;
import org.apache.hadoop.hbase.client.transactional.TransactionManager;
import org.apache.hadoop.hbase.client.transactional.TransactionalTable;
import org.apache.hadoop.hbase.client.transactional.CommitUnsuccessfulException;
import org.apache.hadoop.hbase.client.transactional.TransactionState;
import org.apache.hadoop.hbase.client.transactional.HBaseBackedTransactionLogger;
import org.apache.hadoop.hbase.client.transactional.LocalTransactionLogger;
import org.apache.hadoop.hbase.regionserver.transactional.SingleVersionDeleteNotSupported;
import org.apache.hadoop.hbase.util.Bytes;
import java.io.PrintWriter;
import java.io.File;


public class HBTransPerf{

    private static final Log LOG = LogFactory.getLog(HBTransPerf.class);

    private static final String TABLE_NAME = "table_t1";

    private static final byte[] FAMILY = Bytes.toBytes("family");
    private static final byte[] QUAL_A = Bytes.toBytes("a");
    private static final byte[] QUAL_B = Bytes.toBytes("b");

    private static final byte[] ROW1 = Bytes.toBytes("row1");
    private static final byte[] ROW2 = Bytes.toBytes("row2");
    private static final byte[] ROW3 = Bytes.toBytes("row3");

    private static HBaseAdmin admin;
    private static TransactionalTable table;
    private static TransactionManager transactionManager;

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

        table = new TransactionalTable(config, desc.getName());

        try
	{
           transactionManager = new TransactionManager(config);
        }
	catch(Exception e)
	{
		throw new RuntimeException(e);
	}

    }

    public static void writeRow() throws IOException {

      table.put(new Put(ROW1).add(FAMILY, QUAL_A, Bytes.toBytes(1)));

    }
	
    public static void testBegPutDel(int numtx) throws IOException, CommitUnsuccessfulException {
	PrintWriter bpe = new PrintWriter(new File("HbaseTrxBegPutEnd.csv"));

	int newValue;

	 for(int i=0; i<10; i++)
        {
           long ts1 = System.nanoTime();
           for(newValue = 0; newValue < numtx; newValue++)
           {
              TransactionState transactionState = transactionManager.beginTransaction();
              table.put(transactionState, new Put(ROW2).add(FAMILY, Bytes.toBytes(newValue), Bytes.toBytes(newValue)));
              transactionManager.tryCommit(transactionState);          
	   }
           long ts2 = System.nanoTime();
           long latency = ts2-ts1;
           double latency2 = (double)latency/1000000;
           bpe.println(latency2 + ",");
   	}
	bpe.close();	
    }

    public static void testBegPutEnd(int numtx) throws IOException, CommitUnsuccessfulException {
        PrintWriter beg = new PrintWriter(new File("testBegPutEnd_HbaseTrxBeg.csv"));
        PrintWriter put = new PrintWriter(new File("testBegPutEnd_HbaseTrxPut.csv"));
        PrintWriter end = new PrintWriter(new File("testBegPutEnd_HbaseTrxEnd.csv"));

 	TransactionState[] tState = new TransactionState[numtx];
	TransactionState transactionState;
	int newValue;

	for(int i=0; i<10; i++)
	{

           long ts1 = System.nanoTime();
	   for(newValue = 0; newValue < numtx; newValue++)
	   {
	      transactionState = transactionManager.beginTransaction();
   	      tState[newValue] = transactionState; 
	   }	
	   long ts2 = System.nanoTime();
           long latency = ts2-ts1;
           double latency2 = (double)latency/1000000;
           beg.println(latency2 + ",");

           ts1 = System.nanoTime();
	   for(newValue = 0; newValue < numtx; newValue++)
	   {       
              table.put(tState[newValue], new Put(ROW1).add(FAMILY, Bytes.toBytes(newValue), Bytes.toBytes(newValue)));
	   }	
           ts2 = System.nanoTime();
           latency = ts2-ts1;
           latency2 = (double)latency/1000000;
           put.println(latency2 + ",");

           ts1 = System.nanoTime();
	   for(newValue = 0; newValue < numtx; newValue++)
	   {
	      transactionManager.tryCommit(tState[newValue]);
	   }
           ts2 = System.nanoTime();
           latency = ts2-ts1;
           latency2 = (double)latency/1000000;
           end.println(latency2 + ",");

	}
        beg.close();
        put.close();
        end.close();
    }

    public static void testBegPutEndSplit(int numtx) throws IOException, CommitUnsuccessfulException {

        PrintWriter beg = new PrintWriter(new File("testBegPutEndSplit_HbaseTrxBeg.csv"));
        PrintWriter put = new PrintWriter(new File("testBegPutEndSplit_HbaseTrxPut.csv"));
        PrintWriter pcom = new PrintWriter(new File("testBegPutEndSplit_HbaseTrxPCommit.csv"));
        PrintWriter dcom = new PrintWriter(new File("testBegPutEndSplit_HbaseTrxDCommit.csv"));

        TransactionState[] tState = new TransactionState[numtx];
	int[] tStatus = new int[numtx];
        TransactionState transactionState;
        int newValue;

        for(int i=0; i<10; i++)
        {

           long ts1 = System.nanoTime();
           for(newValue = 0; newValue < numtx; newValue++)
           {
              transactionState = transactionManager.beginTransaction();
              tState[newValue] = transactionState;
           }
           long ts2 = System.nanoTime();
           long latency = ts2-ts1;
           double latency2 = (double)latency/1000000000.0;
           beg.println(latency2 + ",");


           ts1 = System.nanoTime();
           for(newValue = 0; newValue < numtx; newValue++)
           {
              table.put(tState[newValue], new Put(ROW1).add(FAMILY, Bytes.toBytes(newValue), Bytes.toBytes(newValue)));
              table.put(tState[newValue], new Put(ROW2).add(FAMILY, Bytes.toBytes(newValue), Bytes.toBytes(newValue)));
              boolean lv_cp_result = table.checkAndPut(tState[newValue],
						       ROW1,
						       FAMILY,
						       Bytes.toBytes(newValue),
						       Bytes.toBytes(newValue),
						       new Put(ROW1).add(FAMILY, Bytes.toBytes(newValue), Bytes.toBytes("Trafodion")));
	      LOG.debug("Transaction State: " + tState[newValue] + "; checkAndPut return status: " + lv_cp_result);
           }
           ts2 = System.nanoTime();
           latency = ts2-ts1;
           latency2 = (double)latency/1000000000.0;
           put.println(latency2 + ",");

	   ts1 = System.nanoTime();
	   for(newValue = 0; newValue < numtx; newValue++)
	   {	
		tStatus[newValue] = transactionManager.prepareCommit(tState[newValue]);   	
	   }
	   ts2 = System.nanoTime();
           latency = ts2-ts1;
           latency2 = (double)latency/1000000000.0;
           pcom.println(latency2 + ",");

	   ts1 = System.nanoTime();
           for(newValue = 0; newValue < numtx; newValue++)
           {
		   transactionManager.doCommit(tState[newValue]);
           }
           ts2 = System.nanoTime();
           latency = ts2-ts1;
           latency2 = (double)latency/1000000000.0;
           dcom.println(latency2 + ",");

           }
           beg.close();
           put.close();
	   pcom.close();
	   dcom.close();

    }

    public static void testGetPutDel(int numtx) throws IOException, CommitUnsuccessfulException {
        PrintWriter get = new PrintWriter(new File("HbaseTrxGet.csv"));
        PrintWriter put = new PrintWriter(new File("HbaseTrxPut.csv"));
        PrintWriter del = new PrintWriter(new File("HbaseTrxDel.csv"));
        PrintWriter beg = new PrintWriter(new File("HbaseTrxBeg.csv"));
        PrintWriter begend = new PrintWriter(new File("HbaseTrxBegEnd.csv"));
        PrintWriter bpe = new PrintWriter(new File("HbaseTrxBegPutEnd.csv"));

	double put_latency      = 0;
        double get_latency      = 0;
        double delete_latency   = 0;
	int newValue;

	for(int i=0; i<10; i++)
	{  
           long ts1 = System.nanoTime();
           for(newValue = 0; newValue < numtx; newValue++)
           {
              TransactionState transactionState = transactionManager.beginTransaction();
              table.put(transactionState, new Put(ROW1).add(FAMILY, Bytes.toBytes(newValue), Bytes.toBytes(newValue)));
              transactionManager.tryCommit(transactionState);
           }
           long ts2 = System.nanoTime();
           long latency = ts2-ts1;
           double latency2 = (double)latency/1000000;
           bpe.println(latency2 + ",");

           ts1 = System.nanoTime();      
           for(newValue = 0; newValue < numtx; newValue++)
           {
              TransactionState transactionState = transactionManager.beginTransaction();
	   }
           ts2 = System.nanoTime();
           latency = ts2-ts1;
           latency2 = (double)latency/1000000;
           beg.println(latency2 + ",");
          

           TransactionState transactionState = transactionManager.beginTransaction();

           ts1 = System.nanoTime();
	   for(newValue = 0; newValue < numtx; newValue++)
	   {
	      table.put(transactionState, new Put(ROW1).add(FAMILY, Bytes.toBytes(newValue), Bytes.toBytes(newValue)));
	   }
	   ts2 = System.nanoTime();
           latency = ts2-ts1;
           latency2 = (double)latency/1000000;
           put.println(latency2 + ",");

	
           ts1 = System.nanoTime();
           for(newValue = 0; newValue < numtx; newValue++)
	   {
              Result row1_A = table.get( new Get(ROW1).addColumn(FAMILY, Bytes.toBytes(newValue)));
	   }
           ts2 = System.nanoTime();
           latency = ts2-ts1;
           latency2 = (double)latency/1000000;
           get.println(latency2 + ",");


           ts1 = System.nanoTime();
           for(newValue = 0; newValue < numtx; newValue++)
	   {
              table.delete(transactionState, new Delete(ROW1).deleteColumns(FAMILY, Bytes.toBytes(newValue)));
	   }
           ts2 = System.nanoTime();
           latency = ts2-ts1;
           latency2 = (double)latency/1000000;
           del.println(latency2 + ",");
           transactionManager.tryCommit(transactionState);


           ts1 = System.nanoTime();
           for(newValue = 0; newValue < numtx; newValue++)
	   { 
              transactionState = transactionManager.beginTransaction();
              transactionManager.tryCommit(transactionState);
	   }
           ts2 = System.nanoTime();
           latency = ts2-ts1;
           latency2 = (double)latency/1000000;
           begend.println(latency2 + ",");

	}
         put.close();
         del.close();
         get.close();
	 beg.close();
	 bpe.close();
	 begend.close();
    }


    public static void testGetAfterPut3(int numtx) throws IOException, CommitUnsuccessfulException {
        
        int newValue 		= 0;
        for (newValue = 0; newValue < numtx; newValue++)
        {   
             // Begin
             TransactionState transactionState = transactionManager.beginTransaction(100);

             // Put 
	     Put lv_put = new Put(Bytes.toBytes("Alto"));
	     lv_put.add(FAMILY, QUAL_A, Bytes.toBytes("Palo"));
	     lv_put.add(FAMILY, QUAL_B, Bytes.toBytes("Alto"));
             table.put(transactionState, lv_put);

             // Get
	     Result row1_A = table.get( transactionState, new Get(Bytes.toBytes("Alto")).addColumn(FAMILY, QUAL_A));
	     System.out.println("Transactional Get result before put is committed:" + row1_A.size() + ":" + row1_A);

             // Get
	     Result row1_B = table.get( new Get(Bytes.toBytes("Alto")).addColumn(FAMILY, QUAL_A));
	     System.out.println("Normal Get result before put is committed:" + row1_B.size() + ":" + row1_B);

             // End
             transactionManager.tryCommit(transactionState);
             
             // Begin
             transactionState = transactionManager.beginTransaction(101);
             // Get
	     Result row1_C = table.get( new Get(Bytes.toBytes("Alto")).addColumn(FAMILY, QUAL_A));
	     System.out.println("Get after put was commited. Result: " + row1_C);

             // Get
	     row1_A = table.get( transactionState, new Get(Bytes.toBytes("Alto")).addColumn(FAMILY, QUAL_A));
	     System.out.println("Transactional Get after put was commited. Result : " + row1_A);

             // End
             transactionManager.tryCommit(transactionState);
             try {
                 transactionState.completeRequest();
             } catch (Exception e)
             {
                    System.out.println("\nCAUGHT\n");
             }
        }

    }

    public static void testGetAfterPut2(int numtx) throws IOException, CommitUnsuccessfulException {
        
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
             TransactionState transactionState = transactionManager.beginTransaction();
             long ts2 = System.nanoTime();
             long latency = ts2-ts1;
             double latency2 = (double)latency/1000000000;

             begin_latency = begin_latency + latency2;

             // Put 
             ts1 = System.nanoTime();
             table.put(transactionState, new Put(ROW1).add(FAMILY, QUAL_A, Bytes.toBytes(newValue)));
             ts2 = System.nanoTime();
  	     latency = ts2-ts1;
             latency2 = (double)latency/1000000000;

             put_latency = put_latency + latency2;

	     // Abort
	     System.nanoTime();
             transactionManager.abort(transactionState);
             try {
                 transactionState.completeRequest();
             } catch (Exception e)
             {
                  System.out.println("\nCAUGHT\n");
             }      
     ts2 = System.nanoTime();
             latency = ts2-ts1;
             latency2 = (double)latency/1000000000;

             abort_latency = abort_latency + latency2;


             transactionState = transactionManager.beginTransaction();
             table.put(transactionState, new Put(ROW1).add(FAMILY, QUAL_A, Bytes.toBytes(newValue)));
             
             // End
             ts1 = System.nanoTime();
             transactionManager.tryCommit(transactionState);
             try {
                 transactionState.completeRequest();
             } catch (Exception e)
             {
                  System.out.println("\nCAUGHT\n");
             }
             ts2 = System.nanoTime();
             latency = ts2-ts1;
             latency2 = (double)latency/1000000000;

             end_latency = end_latency + latency2;
             
             transactionState = transactionManager.beginTransaction();
             table.put(transactionState, new Put(ROW1).add(FAMILY, QUAL_A, Bytes.toBytes(newValue)));
              
             // Get
             ts1 = System.nanoTime();
	     Result row1_A = table.get( new Get(ROW1).addColumn(FAMILY, QUAL_A));
	     ts2 = System.nanoTime();
             latency = ts2-ts1;
             latency2 = (double)latency/1000000000;

             get_latency = get_latency + latency2;          

	     transactionManager.tryCommit(transactionState);
             try {
                 transactionState.completeRequest();
             } catch (Exception e)
             {
                  System.out.println("\nCAUGHT\n");
             }

             int expected = Bytes.toInt(row1_A.getValue(FAMILY, QUAL_A));
             if (newValue != expected)
                  System.out.println("\ntestGetAfterPut : Assert 1 FAILED : expected " + expected + "newValue " + newValue + "\n");


             transactionState = transactionManager.beginTransaction();
             table.put(transactionState, new Put(ROW1).add(FAMILY, QUAL_A, Bytes.toBytes(newValue)));     

             // Delete
             ts1 = System.nanoTime();
             table.delete(transactionState, new Delete(ROW1).deleteColumns(FAMILY, QUAL_A));
             ts2 = System.nanoTime();
             latency = ts2-ts1;
             latency2 = (double)latency/1000000000;
            
	     delete_latency = delete_latency + latency2;

             transactionManager.tryCommit(transactionState);
             try {
                 transactionState.completeRequest();
             } catch (Exception e)
             {
                  System.out.println("\nCAUGHT\n");
             }

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


    public static void testBeginEndEmpty(int numtx) throws IOException, CommitUnsuccessfulException {
        
	PrintWriter out = new PrintWriter(new File("TEmptyTest.txt"));
	out.println("\nTransactionEmptyTest ENTRY");

        double begin_latency = 0;
        double end_latency   = 0;
        double abort_latency = 0;
        int newValue = 0;

        for (newValue = 0; newValue < numtx; newValue++)
        {
             // Begin
             long ts1 = System.nanoTime();
             TransactionState transactionState = transactionManager.beginTransaction();
             long ts2 = System.nanoTime();
             long latency = ts2-ts1;
             double latency2 = (double)latency/1000000000;

             begin_latency = begin_latency + latency2;

             // End
             ts1 = System.nanoTime();
             transactionManager.tryCommit(transactionState);
             ts2 = System.nanoTime();
             latency = ts2-ts1;
             latency2 = (double)latency/1000000000;
            
             end_latency = end_latency + latency2;
             
             transactionState = transactionManager.beginTransaction();
            
             // Abort
             ts1 = System.nanoTime();
             transactionManager.abort(transactionState);
             ts2 = System.nanoTime();
             latency = ts2-ts1;
             latency2 = (double)latency/1000000000;
 
             abort_latency = abort_latency + latency2;    
 
        }

        out.println("\nAverage Begin latency : " + (double)(begin_latency/newValue));
        out.println("Average End latency  : " + (double)(end_latency/newValue));
        out.println("Average Abort latency  : " + (double)(abort_latency/newValue));
	out.println("newValue  : " + newValue);
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
	//        testGetAfterPut3(numtx);
        //testBeginEndEmpty(numtx);
        //testGetPutDel(numtx);
        //testBegPut2Get3(numtx);
	testBegPutEnd(numtx);
 	//testBegPutEndSplit(numtx);
        System.out.println("\n Test DONE\n");
      }
      catch (Exception e) {
	  System.out.println(e);
      }
   }
}
